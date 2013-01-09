!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2013 EDF S.A.
!
! This program is free software; you can redistribute it and/or modify it under
! the terms of the GNU General Public License as published by the Free Software
! Foundation; either version 2 of the License, or (at your option) any later
! version.
!
! This program is distributed in the hope that it will be useful, but WITHOUT
! ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
! FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
! details.
!
! You should have received a copy of the GNU General Public License along with
! this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
! Street, Fifth Floor, Boston, MA 02110-1301, USA.

!-------------------------------------------------------------------------------

subroutine calhyd &
!================

 ( nvar   , nscal  ,                                              &
   indhyd ,                                                       &
   fextx  , fexty  , fextz  ,                                     &
   dfextx , dfexty , dfextz ,                                     &
   phydr  , flumas , flumab ,                                     &
   coefap , coefbp ,                                              &
   cofafp , cofbfp ,                                              &
   viscf  , viscb  ,                                              &
   dam    , xam    ,                                              &
   drtp   , smbr   )

!===============================================================================
! FONCTION :
! ----------

! RESOLUTION D'UNE EQUATION DE POISSON SUR LA PRESSION HYDROSTATIQUE
!                DIV( GRAD(P) ) = DIV( F )
!                     ----             -
!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! indhyd           ! e  ! --> ! indicateur de mise a jour de phydr             !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! phydr(ncelet)    ! tr ! <-- ! increment de pression hydrostatique            !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefap,coefbp    ! tr ! <-- ! conditions aux limites aux                     !
!  (nfabor)        !    !     !    faces de bord                               !
! cof*fp           ! tr ! <-- ! conditions aux limites aux                     !
!  (nfabor)        !    !     !    faces de bord                               !
! fextx,fexty      ! tr ! <-- ! force exterieure generant la pression          !
! fextz(ncelet)    !    !     !    hydrostatique                               !
! dfextx,dfexty    ! tr ! <-- ! increment de force exterieure                  !
! dfextz(ncelet    !    !     !    generant la pression hydrostatique          !
! viscf(nfac)      ! tr ! --- ! 1*surface/dist aux faces internes              !
! viscb(nfabor     ! tr ! --- ! 1*surface/dist aux faces de bord               !
! dam(ncelet       ! tr ! --- ! tableau de travail pour matrice                !
! xam(nfac,*)      ! tr ! --- ! tableau de travail pour matrice                !
! drtp(ncelet      ! tr ! --- ! tableau de travail pour increment              !
! smbr  (ncelet    ! tr ! --- ! tableau de travail pour sec mem                !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use entsor
use cstnum
use optcal
use parall
use period
use mltgrd
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          indhyd

double precision fextx(ncelet),fexty(ncelet),fextz(ncelet)
double precision dfextx(ncelet),dfexty(ncelet),dfextz(ncelet)
double precision phydr(ncelet)
double precision flumas(nfac), flumab(nfabor)
double precision coefap(nfabor), coefbp(nfabor)
double precision cofafp(nfabor), cofbfp(nfabor)
double precision viscf(nfac), viscb(nfabor)
double precision dam(ncelet), xam(nfac,2)
double precision drtp(ncelet)
double precision smbr(ncelet)

! Local variables

character*80     chaine
integer          lchain
integer          iccocg, inc   , init  , isym  , ipol  , isqrt
integer          iel   , ical
integer          ireslp, nswmpr
integer          isweep, niterf, icycle
integer          iphydp
integer          nswrgp, imligp, iwarnp
integer          iinvpe
integer          idiffp, iconvp, ndircp
integer          nitmap, imgrp , ncymap, nitmgp
integer          nagmax, npstmg
integer          ibsize, iesize
integer          imucpp

double precision residu, rnorm , rnrmf , rnrmdf
double precision epsrgp, climgp, extrap, epsilp
double precision precre, precab, thetap

double precision rvoid(1)

double precision, allocatable, dimension(:) :: w1, w7, w10

!===============================================================================

!===============================================================================
! 1.  INITIALISATIONS
!===============================================================================

! Allocate temporary arrays
allocate(w1(ncelet), w7(ncelet), w10(ncelet))

! --- Memoire

! --- Options de resolution
!     Symetrique
!     Preconditionnement diagonal par defaut
isym  = 1
if (iresol(ipr).eq.-1) then
  ireslp = 0
  ipol   = 0
else
  ireslp = mod(iresol(ipr),1000)
  ipol   = (iresol(ipr)-ireslp)/1000
endif

isqrt = 1

! Matrix block size
ibsize = 1
iesize = 1

!     TEST DE VARIATION DE LA PRESSION HYDROSTATIQUE EN SORTIE


!     on regarde si les terme source ont varie
!     on ne passe dans calhyd que si on a des faces de sortie std
!     la precision pour les tests est a peu pres arbitraire.
precre = sqrt(epzero)
precab = 1.d2*epzero

ical = 0
do iel = 1, ncel
  rnrmf  = fextx(iel)**2+fexty(iel)**2+fextz(iel)**2
  rnrmdf = dfextx(iel)**2+dfexty(iel)**2+dfextz(iel)**2
  if ((rnrmdf.ge.precre*rnrmf).and.(rnrmdf.ge.precab)) then
    ical = 1
  endif
enddo
if (irangp.ge.0) then
  call parcpt (ical)
endif
if (ical.eq.0) then
  do iel = 1,ncel
    phydr(iel) = 0.d0
  enddo
  indhyd = 0
  return
endif

if ( mod(ntcabs,ntlist).eq.0 .or. iwarni(iu) .ge.0 )           &
     write(nfecra,1000)

#if defined(_CS_LANG_FR)

 1000 format(                                                           &
'  Calcul de la pression hydrostatique : ',/,               &
'         mise a jour des Dirichlets en sortie (CALHYD)',/)

#else

 1000 format(                                                           &
'  Hydrostatic pressure computation: ',/,                   &
'         updating the Dirichlets at the end (CALHYD)',/)

#endif

indhyd = 1

!===============================================================================
! 2.  PREPARATION DE LA MATRICE DU SYSTEME A RESOUDRE
!===============================================================================

! ---> TERME INSTATIONNAIRE

do iel = 1, ncel
  w1(iel) = 0.d0
enddo

! ---> "VITESSE" DE DIFFUSION FACETTE

do iel = 1, ncel
  w10(iel) = 1.d0
enddo

call viscfa                                                       &
!==========
 ( imvisf ,                                                       &
   w10    ,                                                       &
   viscf  , viscb  )


iconvp = 0
idiffp = 1
!  On resout avec des CL de flux nul partout
ndircp = 0

thetap = 1.d0
imucpp = 0

call matrix &
!==========
 ( ncelet , ncel   , nfac   , nfabor ,                            &
   iconvp , idiffp , ndircp ,                                     &
   isym   , nfecra ,                                              &
   thetap , imucpp ,                                              &
   ifacel , ifabor ,                                              &
   coefbp , cofbfp , w1     ,                                     &
   flumas , flumab , viscf  , viscb  ,                            &
   rvoid  , dam    , xam    )

!===============================================================================
! 4.  INITIALISATION DU FLUX DE MASSE
!===============================================================================


!     PROJECTION AUX FACES DES TERMES SOURCES
init   = 1
inc    = 0
iccocg = 1
nswrgp = nswrgr(ipr)
imligp = imligr(ipr)
iwarnp = iwarni(ipr)
epsrgp = epsrgr(ipr)
climgp = climgr(ipr)

call projts                                                       &
!==========
 ( nvar   , nscal  ,                                              &
   init   , inc    , imrgra , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp ,                                              &
   dfextx , dfexty , dfextz ,                                     &
   cofbfp ,                                                       &
   flumas, flumab ,                                               &
   viscf  , viscb  ,                                              &
   w10    , w10    , w10    )

init = 1
call divmas(ncelet,ncel,nfac,nfabor,init,nfecra,                  &
            ifacel,ifabor,flumas,flumab,w7)
call prodsc(ncel,isqrt,w7,w7,rnorm)


!===============================================================================
! 5.  PREPARATION DU MULTIGRILLE ALGEBRIQUE
!===============================================================================

if (imgr(ipr).gt.0) then

!   --- Creation de la hierarchie de maillages

  chaine = 'PresHydr'
  iwarnp = iwarni(ipr)
  nagmax = nagmx0(ipr)
  npstmg = ncpmgr(ipr)
  lchain = 16

  call clmlga &
  !==========
 ( chaine(1:16) ,    lchain ,                                     &
   isym   , ibsize , iesize , nagmax , npstmg , iwarnp ,          &
   ngrmax , ncegrm ,                                              &
   rlxp1  ,                                                       &
   dam    , xam    )

endif

!===============================================================================
! 6.  BOUCLES SUR LES NON ORTHOGONALITES (RESOLUTION)
!===============================================================================

! --- Nombre de sweeps
nswmpr = nswrsm(ipr)

! --- Mise a zero des variables
!       RTP(.,IPR) sera l'increment de pression cumule
!       DRTP       sera l'increment d'increment a chaque sweep
!       W7         sera la divergence du flux de masse predit
do iel = 1,ncel
  phydr(iel) = 0.d0
  drtp(iel) = 0.d0
  smbr(iel) = 0.d0
enddo


! --- Boucle de reconstruction : debut
do isweep = 1, nswmpr

! --- Mise a jour du second membre
!     (signe "-" a cause de celui qui est implicitement dans la matrice)
  do iel = 1, ncel
    smbr(iel) = - w7(iel) - smbr(iel)
  enddo

! --- Test de convergence du calcul

  call prodsc(ncel,isqrt,smbr,smbr,residu)
  if (iwarni(ipr).ge.2) then
     chaine = 'PresHydr'
     write(nfecra,1400)chaine(1:16),isweep,residu
  endif

!MO IL FAUDRA VERIFIER LA PERTINENCE DU TEST

  if( residu .le. 10.d0*epsrsm(ipr)*rnorm ) then
!     Si convergence,  sortie

    goto 101

  endif

! --- Resolution implicite sur l'increment d'increment DRTP
  do iel = 1, ncel
    drtp(iel) = 0.d0
  enddo

  chaine = 'PresHydr'
  nitmap = nitmax(ipr)
  imgrp  = imgr  (ipr)
  ncymap = ncymax(ipr)
  nitmgp = nitmgf(ipr)
  iwarnp = iwarni(ipr)
  epsilp = epsilo(ipr)
  iinvpe = 1
  ibsize = 1
  iesize = 1

  call invers &
  !==========
 ( chaine(1:16)    , isym   , ibsize , iesize ,                   &
   ipol   , ireslp , nitmap , imgrp  ,                            &
   ncymap , nitmgp ,                                              &
   iwarnp , nfecra , niterf , icycle , iinvpe ,                   &
   epsilp , rnorm  , residu ,                                     &
   dam    , xam    , smbr   , drtp   )


  if( isweep.eq.nswmpr ) then
!     Mise a jour de l'increment de pression
    do iel = 1, ncel
      phydr(iel) = phydr(iel) + drtp(iel)
    enddo


  else

! --- Si ce n'est pas le dernier sweep
!       Mise a jour de l'increment de pression et calcul direct de la
!       partie en gradient d'increment de pression du second membre
!       (avec reconstruction)

    do iel = 1, ncel
      phydr(iel) = phydr(iel) + drtp(iel)
    enddo

    iccocg = 1
    init = 1
    inc  = 1
    nswrgp = nswrgr(ipr)
    imligp = imligr(ipr)
    iwarnp = iwarni(ipr)
    epsrgp = epsrgr(ipr)
    climgp = climgr(ipr)
    extrap = 0.d0
    iphydp = 1

    call itrgrp &
    !==========
 ( nvar   , nscal  ,                                              &
   init   , inc    , imrgra , iccocg , nswrgp , imligp , iphydp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   dfextx , dfexty , dfextz ,                                     &
   phydr  ,                                                       &
   coefap , coefbp ,                                              &
   cofafp , cofbfp ,                                              &
   viscf  , viscb  ,                                              &
   w10         , w10         , w10         ,                      &
   smbr   )

  endif

enddo
! --- Boucle de reconstruction : fin

if(iwarni(ipr).ge.2) then
   chaine = 'PresHydr'
   write( nfecra,1600)chaine(1:16),nswmpr
endif

 101  continue

! Free memory
deallocate(w1, w7, w10)

!===============================================================================
! 7.  SUPPRESSION DE LA HIERARCHIE DE MAILLAGES
!===============================================================================

if (imgr(ipr).gt.0) then
  chaine = 'PresHydr'
  lchain = 16
  call dsmlga(chaine(1:16), lchain)
  !==========
endif

!--------
! FORMATS
!--------

#if defined(_CS_LANG_FR)

 1400 format(1X,A16,' : SWEEP = ',I5,' NORME SECOND MEMBRE = ',E14.6)
 1600 format(                                                           &
'@                                                            ',/,&
'@ @@ ATTENTION : ', A16,' ETAPE DE PRESSION HYDROSTATIQUE    ',/,&
'@    =========                                               ',/,&
'@  Nombre d''iterations maximal ',I10   ,' atteint           ',/,&
'@                                                            '  )

#else

 1400 format(1X,A16,' : SWEEP = ',I5,' RIGHT HAND SIDE NORM = ',E14.6)
 1600 format(                                                           &
'@                                                            ',/,&
'@ @@ WARNING: ', A16,' HYDROSTATIC PRESSURE STEP             ',/,&
'@    ========                                                ',/,&
'@  Maximum number of iterations ',I10   ,' reached           ',/,&
'@                                                            '  )

#endif

!----
! FIN
!----

return

end subroutine
