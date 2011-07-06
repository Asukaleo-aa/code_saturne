!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2009 EDF S.A., France

!     contact: saturne-support@edf.fr

!     The Code_Saturne Kernel is free software; you can redistribute it
!     and/or modify it under the terms of the GNU General Public License
!     as published by the Free Software Foundation; either version 2 of
!     the License, or (at your option) any later version.

!     The Code_Saturne Kernel is distributed in the hope that it will be
!     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
!     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!     GNU General Public License for more details.

!     You should have received a copy of the GNU General Public License
!     along with the Code_Saturne Kernel; if not, write to the
!     Free Software Foundation, Inc.,
!     51 Franklin St, Fifth Floor,
!     Boston, MA  02110-1301  USA

!-------------------------------------------------------------------------------

subroutine strdep &
!================

 ( itrale , italim , itrfin ,                                     &
   nvar   ,                                                       &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
   flmalf , flmalb , cofale , xprale )

!===============================================================================
! FONCTION :
! ----------

! DEPLACEMENT DES STRUCTURES MOBILES EN ALE EN COUPLAGE INTERNE

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! itrale           ! e  ! <-- ! numero d'iteration pour l'ale                  !
! italim           ! e  ! <-- ! numero d'iteration couplage implicite          !
! itrfin           ! e  ! <-- ! indicateur de derniere iteration de            !
!                  !    !     !                    couplage implicite          !
! nvar             ! i  ! <-- ! total number of variables                      !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! flmalf(nfac)     ! tr ! --> ! sauvegarde du flux de masse faces int          !
! flmalb(nfabor    ! tr ! --> ! sauvegarde du flux de masse faces brd          !
! cofale           ! tr ! --> ! sauvegarde des cl de p et u                    !
!    (nfabor,8)    !    !     !                                                !
! xprale(ncelet    ! tr ! --> ! sauvegarde de la pression, si nterup           !
!                  !    !     !    est >1                                      !
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
use ihmpre
use cstphy
use numvar
use optcal
use entsor
use pointe
use albase
use alstru
use alaste
use parall
use period
use mesh

!===============================================================================

implicit none

! Arguments

integer          itrale , italim , itrfin
integer          nvar


double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision flmalf(nfac), flmalb(nfabor), xprale(ncelet)
double precision cofale(nfabor,8)

! Local variables

integer          istr, ii, iel, ifac, ntab
integer          iflmas, iflmab, iclp, iclu, iclv, iclw
integer          indast
integer          icvext, icvint, icv

double precision delta

double precision, allocatable, dimension(:,:) :: forast

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================


iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))
iclp = iclrtp(ipr,icoef)
iclu = iclrtp(iu,icoef)
iclv = iclrtp(iv,icoef)
iclw = iclrtp(iw,icoef)

!===============================================================================
! 2.  CALCUL DES EFFORTS SUR LES STRUCTURES
!===============================================================================

do istr = 1, nbstru
  do ii = 1, ndim
    forsta(ii,istr) = forstr(ii,istr)
!-a tester          FORSTA(II,ISTR) = FORSTP(II,ISTR)
    forstr(ii,istr) = 0.d0
  enddo
enddo

! Allocate a temporary array
allocate(forast(3,nbfast))

indast = 0
do ifac = 1, nfabor
  istr = idfstr(ifac)
  if (istr.gt.0) then
    do ii = 1, 3
      forstr(ii,istr) = forstr(ii,istr) + forbr(ii,ifac)
    enddo
  else if (istr.lt.0) then
    indast = indast + 1
    do ii = 1, 3
      forast(ii,indast) = asddlf(ii,-istr)*forbr(ii,ifac)
    enddo
  endif
enddo

if (irangp.ge.0) then
  ntab = ndim*nbstru
  call parrsm(ntab,forstr)
endif

!     Calcul de l'effort envoye au structures internes
do istr = 1, nbstru
  do ii = 1, ndim
    forstp(ii,istr) = cfopre*forstr(ii,istr)+                     &
         (1.d0-cfopre)*forsta(ii,istr)
  enddo
enddo

!     Envoi de l'effort applique aux structures externes
if (nbaste.gt.0) then
  call astfor(ntcast, nbfast, forast)
  !==========
endif

! Free memory
deallocate(forast)

!     Si on est en phase d'initialisation du fluide
if (itrale.le.nalinf) then
  itrfin = -1
  return
endif

!===============================================================================
! 3.  DEFINITION UTILISATEUR DES CARACTERISTIQUES DE LA STRUCTURE
!===============================================================================


if (nbstru.gt.0) then

  ! - Interface Code_Saturne
  !   ======================

  if (iihmpr.eq.1) then

    call uistr2 &
    !==========
 ( xmstru, xcstru, xkstru,     &
   forstr,                     &
   dtref, ttcabs, ntcabs   )

  endif

  call usstr2                                                     &
  !==========
 ( nbstru ,                                                       &
   idfstr ,                                                       &
   dt     ,                                                       &
   xmstru , xcstru , xkstru , xstreq , xstr   , xpstr  , forstp , &
   dtstr  )

endif

!===============================================================================
! 4.  DEPLACEMENT DES STRUCTURES INTERNES
!===============================================================================


do istr = 1, nbstru

  call newmrk                                                     &
  !==========
 ( istr  , alpnmk  , betnmk          , gamnmk          ,          &
   xmstru(1,1,istr), xcstru(1,1,istr), xkstru(1,1,istr),          &
   xstreq(1,istr)  ,                                              &
   xstr(1,istr)    , xpstr(1,istr)   , xppstr(1,istr)  ,          &
   xsta(1,istr)    , xpsta(1,istr)   , xppsta(1,istr)  ,          &
   forstp(1,istr)  , forsta(1,istr)  , dtstr(istr)     )

enddo

!===============================================================================
! 5.  TEST DE CONVERGENCE
!===============================================================================

icvext = 0
icvint = 0
icv    = 0

delta = 0.d0
do istr = 1, nbstru
  do ii = 1, 3
    delta = delta + (xstr(ii,istr)-xstp(ii,istr))**2
  enddo
enddo
if (nbstru.gt.0) then
  delta = sqrt(delta)/almax/nbstru
  if (delta.lt.epalim) icvint = 1
endif

if (nbaste.gt.0) call astcv1(ntcast, icvext)
                 !==========


if (nbstru.gt.0.and.nbaste.gt.0) then
   icv = icvext*icvint
elseif (nbstru.gt.0.and.nbaste.eq.0) then
   icv = icvint
elseif (nbaste.gt.0.and.nbstru.eq.0) then
   icv = icvext
endif

if (iwarni(iuma).ge.2) write(nfecra,1000) italim, delta

!     si convergence
if (icv.eq.1) then
  if (itrfin.eq.1) then
!       si ITRFIN=1 on sort
    if (iwarni(iuma).ge.1) write(nfecra,1001) italim, delta
    itrfin = -1
  else
!       sinon on refait une derniere iteration pour SYRTHES/T1D/rayonnement
!        et on remet ICV a 0 pour que Code_Aster refasse une iteration aussi
    itrfin = 1
    icv = 0
  endif
elseif (itrfin.eq.0 .and. italim.eq.nalimx-1) then
!       ce sera la derniere iteration
  itrfin = 1
elseif (italim.eq.nalimx) then
!       on a forcement ITRFIN=1 et on sort
  if (nalimx.gt.1) write(nfecra,1100) italim, delta
  itrfin = -1
!       On met ICV a 1 pour que Code_Aster s'arrete lui aussi
  icv = 1
endif

!     On renvoie l'indicateur de convergence final a Code_Aster
call astcv2(ntcast, icv)
!==========

!===============================================================================
! 6.  RETOUR AUX VALEURS ANTERIEURES SI NECESSAIRE
!===============================================================================

!     Si NTERUP    .GT.1, RTPA a ete touche apres NAVSTO, on doit donc
!       revenir a une valeur anterieure
if (itrfin.ne.-1) then
  do ii = 1, nvar
    if (ii.eq.ipr .and. nterup.gt.1) then
      do iel = 1, ncelet
        rtpa(iel,ii) = xprale(iel)
      enddo
    endif
    do iel = 1, ncelet
      rtp(iel,ii) = rtpa(iel,ii)
    enddo
  enddo
  do ifac = 1, nfac
     propfa(ifac,iflmas) = flmalf(ifac)
  enddo
  do ifac = 1, nfabor
     propfb(ifac,iflmab) = flmalb(ifac)
     coefa(ifac,iclp) = cofale(ifac,1)
     coefa(ifac,iclu) = cofale(ifac,2)
     coefa(ifac,iclv) = cofale(ifac,3)
     coefa(ifac,iclw) = cofale(ifac,4)
     coefb(ifac,iclp) = cofale(ifac,5)
     coefb(ifac,iclu) = cofale(ifac,6)
     coefb(ifac,iclv) = cofale(ifac,7)
     coefb(ifac,iclw) = cofale(ifac,8)
  enddo
endif

!----
! FORMATS
!----

#if defined(_CS_LANG_FR)

 1000 format (                                                          &
 '            ALE IMPLICITE : ITER=',I5,' DERIVE=',E12.5     )
 1001 format (                                                          &
 'CONVERGENCE ALE IMPLICITE : ITER=',I5,' DERIVE=',E12.5     )
 1100 format (                                                          &
'@                                                            ',/,&
'@ @@ ATTENTION : COUPLAGE IMPLICITE ALE                      ',/,&
'@    =========                                               ',/,&
'@  Nombre d''iterations maximal ',I10   ,' atteint           ',/,&
'@  Derive normee :',E12.5                                     ,/,&
'@                                                            '  )

#else

 1000 format (                                                          &
 '            IMPLICIT ALE: ITER=',I5,' DERIVE=',E12.5     )
 1001 format (                                                          &
 'CONVERGENCE IMPLICIT ALE: ITER=',I5,' DERIVE=',E12.5     )
 1100 format (                                                          &
'@                                                            ',/,&
'@ @@ WARNING: IMPLICIT ALE                                   ',/,&
'@    ========                                                ',/,&
'@  Maximum number of iterations ',I10   ,' reached           ',/,&
'@  Normed derive :',E12.5                                     ,/,&
'@                                                            '  )

#endif

!----
! FIN
!----

end subroutine
