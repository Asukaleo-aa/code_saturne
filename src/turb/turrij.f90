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

subroutine turrij &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   tslagr ,                                                       &
   coefa  , coefb  , ckupdc , smacel )

!===============================================================================
! FONCTION :
! ----------

! RESOLUTION DES EQUATIONS Rij-EPS 1 PHASE INCOMPRESSIBLE OU
! RHO VARIABLE SUR UN PAS DE TEMPS

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncepdp           ! i  ! <-- ! number of cells with head loss                 !
! ncesmp           ! i  ! <-- ! number of cells with mass source term          !
! icepdc(ncelet    ! te ! <-- ! numero des ncepdp cellules avec pdc            !
! icetsm(ncesmp    ! te ! <-- ! numero des cellules a source de masse          !
! itypsm           ! te ! <-- ! type de source de masse pour les               !
! (ncesmp,nvar)    !    !     !  variables (cf. ustsma)                        !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! tslagr           ! tr ! <-- ! terme de couplage retour du                    !
!(ncelet,*)        !    !     !     lagrangien                                 !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! ckupdc           ! tr ! <-- ! tableau de travail pour pdc                    !
!  (ncepdp,6)      !    !     !                                                !
! smacel           ! tr ! <-- ! valeur des variables associee a la             !
! (ncesmp,*   )    !    !     !  source de masse                               !
!                  !    !     !  pour ivar=ipr, smacel=flux de masse           !
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
use dimens, only: ndimfb
use lagdim, only: ntersl
use numvar
use entsor
use cstphy
use optcal
use lagran
use pointe, only: coefau, coefbu
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp

integer          icepdc(ncepdp)
integer          icetsm(ncesmp)

integer, dimension(ncesmp,nvar), target :: itypsm

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision ckupdc(ncepdp,6)

double precision, dimension(ncesmp,nvar), target ::  smacel
double precision, dimension(ncelet,ntersl), target :: tslagr

! Local variables

integer          ifac  , iel   , ivar  , isou  , ii
integer          inc   , iccocg
integer          ipp   , iwarnp, iclip
integer          icliup, iclivp, icliwp
integer          nswrgp, imligp
integer          ipcrom, ipbrom, ipcroo, ipbroo, iivar
integer          iitsla
double precision epsrgp, climgp, extrap

logical          ilved

double precision, allocatable, dimension(:) :: viscf, viscb, coefax
double precision, allocatable, dimension(:) :: smbr, rovsdt
double precision, allocatable, dimension(:,:,:) :: grdvit
double precision, allocatable, dimension(:,:) :: produc
double precision, allocatable, dimension(:,:) :: gradu, gradv, gradw, gradro

integer,          pointer, dimension(:) :: itpsmp => null()
double precision, pointer, dimension(:) :: smcelp => null(), gammap => null()
double precision, pointer, dimension(:) :: tslage => null(), tslagi => null()

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================

! Allocate temporary arrays for the turbulence resolution
allocate(viscf(nfac), viscb(nfabor))
allocate(smbr(ncelet), rovsdt(ncelet))

! Allocate other arrays, depending on user options
if (abs(icdpar).eq.1.and.irijec.eq.1) then
  allocate(coefax(nfabor))
endif
if (iturb.eq.30) then
  allocate(produc(6,ncelet))
else
  allocate(grdvit(ncelet,3,3))
endif


icliup = iclrtp(iu,icoef)
iclivp = iclrtp(iv,icoef)
icliwp = iclrtp(iw,icoef)

ipcrom = ipproc(irom  )
ipbrom = ipprob(irom  )

if(iwarni(iep).ge.1) then
  if (iturb.eq.30) then
    write(nfecra,1000)
  elseif (iturb.eq.31) then
    write(nfecra,1001)
  else
    write(nfecra,1002)
  endif
endif


! Si iturb=30 (rij std) on stocke directement la production dans
! le tableau produc
! Si iturb=31 (SSG) ou 32 (EBRSM) on stocke le gradient de vitesse dans grdvit

!===============================================================================
! 2.a CALCUL DU TENSEUR DE PRODUCTION POUR LE RIJ STANDARD
!===============================================================================

if (iturb.eq.30) then

  !FIXME compute the velocity gradient in once.  ! Allocate temporary arrays for gradients calculation
  allocate(gradu(ncelet,3), gradv(ncelet,3), gradw(ncelet,3))

  do ii = 1 , 6
    do iel = 1, ncel
      produc(ii,iel) = 0.0d0
    enddo
  enddo

! CALCUL DU GRADIENT DES 3 COMPOSANTES DE LA VITESSE

  iccocg = 1
  inc    = 1

! GRADIENT SUIVANT X

  nswrgp = nswrgr(iu)
  imligp = imligr(iu)
  iwarnp = iwarni(iu)
  epsrgp = epsrgr(iu)
  climgp = climgr(iu)
  extrap = extrag(iu)

  call grdcel &
  !==========
 ( iu  , imrgra , inc    , iccocg , nswrgp , imligp ,             &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,iu)   , coefa(1,icliup) , coefb(1,icliup) ,             &
   gradu  )


  do iel = 1 , ncel

    produc(1,iel) = produc(1,iel)                                 &
         - 2.0d0*(rtpa(iel,ir11)*gradu(iel,1) +                   &
                  rtpa(iel,ir12)*gradu(iel,2) +                   &
                  rtpa(iel,ir13)*gradu(iel,3) )

    produc(4,iel) = produc(4,iel)                                 &
         - (rtpa(iel,ir12)*gradu(iel,1) +                         &
            rtpa(iel,ir22)*gradu(iel,2) +                         &
            rtpa(iel,ir23)*gradu(iel,3) )

    produc(5,iel) = produc(5,iel)                                 &
         - (rtpa(iel,ir13)*gradu(iel,1) +                         &
            rtpa(iel,ir23)*gradu(iel,2) +                         &
            rtpa(iel,ir33)*gradu(iel,3) )

  enddo

! Gradient suivant Y

  nswrgp = nswrgr(iv)
  imligp = imligr(iv)
  iwarnp = iwarni(iv)
  epsrgp = epsrgr(iv)
  climgp = climgr(iv)
  extrap = extrag(iv)

  call grdcel &
  !==========
 ( iv  , imrgra , inc    , iccocg , nswrgp , imligp ,             &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,iv)   , coefa(1,iclivp) , coefb(1,iclivp) ,             &
   gradv  )

  do iel = 1 , ncel

    produc(2,iel) = produc(2,iel)                                 &
         - 2.0d0*(rtpa(iel,ir12)*gradv(iel,1) +                   &
                  rtpa(iel,ir22)*gradv(iel,2) +                   &
                  rtpa(iel,ir23)*gradv(iel,3) )

    produc(4,iel) = produc(4,iel)                                 &
         - (rtpa(iel,ir11)*gradv(iel,1) +                         &
            rtpa(iel,ir12)*gradv(iel,2) +                         &
            rtpa(iel,ir13)*gradv(iel,3) )

    produc(6,iel) = produc(6,iel)                                 &
         - (rtpa(iel,ir13)*gradv(iel,1) +                         &
            rtpa(iel,ir23)*gradv(iel,2) +                         &
            rtpa(iel,ir33)*gradv(iel,3) )

  enddo

! Gradient suivant Z

  nswrgp = nswrgr(iw)
  imligp = imligr(iw)
  iwarnp = iwarni(iw)
  epsrgp = epsrgr(iw)
  climgp = climgr(iw)
  extrap = extrag(iw)

  call grdcel &
  !==========
 ( iw  , imrgra , inc    , iccocg , nswrgp , imligp ,             &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,iw)   , coefa(1,icliwp) , coefb(1,icliwp) ,             &
   gradw  )

  do iel = 1 , ncel

    produc(3,iel) = produc(3,iel)                                 &
         - 2.0d0*(rtpa(iel,ir13)*gradw(iel,1) +                   &
                  rtpa(iel,ir23)*gradw(iel,2) +                   &
                  rtpa(iel,ir33)*gradw(iel,3) )

    produc(5,iel) = produc(5,iel)                                 &
         - (rtpa(iel,ir11)*gradw(iel,1) +                         &
            rtpa(iel,ir12)*gradw(iel,2) +                         &
            rtpa(iel,ir13)*gradw(iel,3) )

    produc(6,iel) = produc(6,iel)                                 &
         - (rtpa(iel,ir12)*gradw(iel,1) +                         &
            rtpa(iel,ir22)*gradw(iel,2) +                         &
            rtpa(iel,ir23)*gradw(iel,3) )

  enddo

  ! Free memory
  deallocate(gradu, gradv, gradw)

else

!===============================================================================
! 2.b Calcul du gradient de vitesse pour le Rij SSG et EBRSM
!     ATTENTION: grdvit(iel,j,i) = dUi/dxj(IEL)
!===============================================================================

! CALCUL DU GRADIENT DES 3 COMPOSANTES DE LA VITESSE

  iccocg = 1
  inc    = 1

  nswrgp = nswrgr(iu)
  imligp = imligr(iu)
  iwarnp = iwarni(iu)
  epsrgp = epsrgr(iu)
  climgp = climgr(iu)
  extrap = extrag(iu)

  if (ivelco.eq.1) then

    ilved = .false.

    call grdvec &
    !==========
  ( iu     , imrgra , inc    , nswrgp , imligp ,                   &
    iwarnp , nfecra ,                                              &
    epsrgp , climgp , extrap ,                                     &
    ilved  ,                                                       &
    rtpa(1,iu) ,  coefau , coefbu,                                 &
    grdvit  )

  else

    call grdvni &
    !==========
  ( iu  , imrgra , inc    , iccocg , nswrgp , imligp ,             &
    iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
    rtpa(1,iu)   , coefa(1,icliup) , coefb(1,icliup) ,             &
    grdvit )

  endif

endif


!===============================================================================
! 3.  CALCUL DU GRADIENT DE ROM POUR LES TERMES DE GRAVITE
!===============================================================================

if(igrari.eq.1) then

  ! Allocate a temporary array for the gradient calculation
  allocate(gradro(ncelet,3))

! Conditions aux limites : Dirichlet ROMB
!   On utilise VISCB pour stocker le coefb relatif a ROM
!   On impose en Dirichlet (COEFA) la valeur ROMB

  do ifac = 1, nfabor
    viscb(ifac) = 0.d0
  enddo

! Le choix ci dessous a l'avantage d'etre simple

  nswrgp = nswrgr(ir11)
  imligp = imligr(ir11)
  iwarnp = iwarni(ir11)
  epsrgp = epsrgr(ir11)
  climgp = climgr(ir11)
  extrap = extrag(ir11)

  iivar = 0

!     Si on extrapole les termes sources et rho, on utilise cpdt rho^n
  ipcroo = ipcrom
  ipbroo = ipbrom
  if(isto2t.gt.0.and.iroext.gt.0) then
    ipcroo = ipproc(iroma)
    ipbroo = ipprob(iroma)
  endif

  call grdcel                                                     &
  !==========
 ( iivar  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   propce(1,ipcroo), propfb(1,ipbroo), viscb           ,          &
   gradro )

endif


!===============================================================================
! 4.  Boucle sur les variables Rij (6 variables)
!     L'ordre est R11 R22 R33 R12 R13 R23 (La place de ces variables
!     est IR11.    ..
!     On resout les equation dans une routine semblable a covofi.f90
!===============================================================================

do isou = 1, 6
  if    (isou.eq.1) then
    ivar   = ir11
  elseif(isou.eq.2) then
    ivar   = ir22
  elseif(isou.eq.3) then
    ivar   = ir33
  elseif(isou.eq.4) then
    ivar   = ir12
  elseif(isou.eq.5) then
    ivar   = ir13
  elseif(isou.eq.6) then
    ivar   = ir23
  endif
  ipp    = ipprtp(ivar)

  if (iilagr.eq.2) then
    iitsla = itsr11 + (isou-1)
    tslage => tslagr(1:ncelet,iitsla)
    tslagi => tslagr(1:ncelet,itsli)
  endif

  if (ncesmp.gt.0) then
    itpsmp => itypsm(1:ncesmp,ivar)
    smcelp => smacel(1:ncesmp,ivar)
    gammap => smacel(1:ncesmp,ipr)
  endif

  !     Rij-epsilon standard (LRR)
  if (iturb.eq.30) then
    call resrij                                                   &
    !==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   ivar   , isou   , ipp    ,                                     &
   icepdc , icetsm , itpsmp ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , produc , gradro ,                            &
   ckupdc , smcelp , gammap ,                                     &
   viscf  , viscb  , coefax ,                                     &
   tslage , tslagi ,                                              &
   smbr   , rovsdt )

  elseif (iturb.eq.31.or.iturb.eq.32) then
    ! Rij-epsilon SSG or EBRSM
    call resssg                                                   &
    !==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   ivar   , isou   , ipp    ,                                     &
   icepdc , icetsm , itpsmp ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , grdvit , gradro ,                            &
   ckupdc , smcelp , gammap ,                                     &
   viscf  , viscb  , coefax ,                                     &
   tslage , tslagi ,                                              &
   smbr   , rovsdt )
  endif

enddo

!===============================================================================
! 5.  RESOLUTION DE EPSILON
!===============================================================================

ivar   = iep
ipp    = ipprtp(ivar)
isou   = 7

if (ncesmp.gt.0) then
  itpsmp => itypsm(1:ncesmp,ivar)
  smcelp => smacel(1:ncesmp,ivar)
  gammap => smacel(1:ncesmp,ipr)
endif

call reseps                                                       &
!==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   ivar   , isou   , ipp    ,                                     &
   icepdc , icetsm , itpsmp ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , grdvit , produc , gradro ,                   &
   ckupdc , smcelp , gammap ,                                     &
   viscf  , viscb  ,                                              &
   tslagr ,                                                       &
   smbr   , rovsdt )

!===============================================================================
! 6. CLIPPING
!===============================================================================

if (iturb.eq.32) then
  iclip = 1
else
  iclip = 2
endif

call clprij                                                       &
!==========
 ( ncelet , ncel   , nvar   ,                                     &
   iclip  ,                                                       &
   propce , rtpa   , rtp    )


! Free memory
deallocate(viscf, viscb)
deallocate(smbr, rovsdt)
if (allocated(gradro)) deallocate(gradro)
if (allocated(coefax)) deallocate(coefax)
if (allocated(produc)) deallocate(produc)
if (allocated(grdvit)) deallocate(grdvit)

!--------
! FORMATS
!--------

#if defined(_CS_LANG_FR)

 1000 format(/,                                                   &
'   ** RESOLUTION DU Rij-EPSILON LRR             ',/,&
'      -----------------------------             ',/)
 1001 format(/,                                                   &
'   ** RESOLUTION DU Rij-EPSILON SSG             ',/,&
'      -----------------------------             ',/)
 1002 format(/,                                                   &
'   ** RESOLUTION DU Rij-EPSILON EBRSM                        ',/,&
'      --------------------------------------------           ',/)

#else

 1000 format(/,                                                   &
'   ** SOLVING Rij-EPSILON LRR'                   ,/,&
'      -----------------------'                   ,/)
 1001 format(/,                                                   &
'   ** SOLVING Rij-EPSILON SSG'                   ,/,&
'      -----------------------'                   ,/)
 1002 format(/,                                                   &
'   ** SOLVING Rij-EPSILON EBRSM                              ',/,&
'      --------------------------------------                 ',/)

#endif

!----
! FIN
!----

return

end subroutine
