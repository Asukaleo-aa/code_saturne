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

subroutine strpre &
!================

 ( itrale , italim , ineefl ,                                     &
   impale ,                                                       &
   rtp    , rtpa   , propce , propfa , propfb ,                   &
   coefa  , coefb  ,                                              &
   flmalf , flmalb , xprale , cofale , depale )

!===============================================================================
! FONCTION :
! ----------

! PREDICTION DU DEPLACEMENT DES STRUCTURES EN ALE

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! itrale           ! e  ! <-- ! numero d'iteration pour l'ale                  !
! italim           ! e  ! <-- ! numero d'iteration couplage implicite          !
! ineedf           ! e  ! <-- ! indicateur de sauvegarde des flux              !
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
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
! depale(nnod,3    ! tr ! <-- ! deplacement aux noeuds                         !
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
use optcal
use numvar
use pointe
use albase, only: nalimx
use alstru
use alaste
use parall
use period
use entsor
use mesh

!===============================================================================

implicit none

! Arguments

integer          itrale , italim , ineefl

integer          impale(nnod)

double precision rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision flmalf(nfac), flmalb(nfabor), xprale(ncelet)
double precision cofale(nfabor,8)
double precision depale(nnod,3)

! Local variables

integer          istr, ii, ifac, inod, iel, indast
integer          iflmas, iflmab,iclp,iclu,iclv,iclw

integer, allocatable, dimension(:) :: lstfac

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
! 2. PREDICTION DU DEPLACEMENT DES STRUCTURES
!===============================================================================

! 2.1 STRUCTURES INTERNES :
! -----------------------

! Lors de la phase d'initialisation de l'ALE (ITRALE=0), XSTP contient
!    - la valeur du deplacement initial des structures si l'utilisateur
!        les a touchees (suite ou pas)
!    - 0 si debut de calcul avec les structures
!    - le deplacement utilise pour le calcul precedent si suite sans
!        modification utilisateur
!   Il faut cependant transferer sa valeur dans XSTR (qui est utilise
!   par Newmark)
! Lors des iterations suivantes (ITRALE>0) on utilise les schemas standard
!   de calcul de XSTP

if (nbstru.gt.0) then

  if (itrale.eq.0) then

    do istr = 1, nbstru
      do ii = 1, ndim
        xstr(ii,istr) = xstp(ii,istr)
      enddo
    enddo

  else

! 2.1.1 : SCHEMA DE COUPLAGE EXPLICITE
!---------------------------------------------
    if (nalimx.eq.1) then
      do istr = 1, nbstru
        do ii = 1, 3
          xstp(ii,istr) = xstr(ii,istr)                           &
             + aexxst*dtstr(istr)*xpstr(ii,istr)                  &
             + bexxst*dtstr(istr)*(xpstr(ii,istr)-xpsta(ii,istr))
        enddo
      enddo

! 2.1.2 : SCHEMA DE COUPLAGE IMPLICITE
!---------------------------------------------
    else
      do istr = 1, nbstru
        do ii = 1, 3
          xstp(ii,istr)  = xstr(ii,istr)
        enddo
      enddo
    endif

  endif

  do ifac = 1, nfabor
    istr = idfstr(ifac)
    if (istr.gt.0) then
      do ii = ipnfbr(ifac), ipnfbr(ifac+1)-1
        inod = nodfbr(ii)
        impale(inod) = 1
        depale(inod,1) = xstp(1,istr)
        depale(inod,2) = xstp(2,istr)
        depale(inod,3) = xstp(3,istr)
      enddo
    endif
  enddo

endif


! 2.2 STRUCTURES EXTERNES (COUPLAGE CODE_ASTER) :
! -----------------------


if (nbaste.gt.0) then

  do ifac = 1, nfabor
    istr = idfstr(ifac)
    if (istr.lt.0) then
      do ii = ipnfbr(ifac), ipnfbr(ifac+1)-1
        inod = nodfbr(ii)
        impale(inod) = 1
      enddo
    endif
  enddo

! Si ITRALE = 0, on ne fait rien pour l'instant, mais il faudrait
! prevoir la reception de deplacement initiaux venant de Code_Aster
  if (itrale.gt.0) then

    ntcast = ntcast + 1

! Reception des deplacements predits et remplissage de depale

    ! Allocate a temporary array
    allocate(lstfac(nbfast))

    indast = 0
    do ifac = 1, nfabor
      istr = idfstr(ifac)
      if (istr.lt.0) then
        indast = indast + 1
        lstfac(indast) = ifac
      endif
    enddo

    call astcin(ntcast, nbfast, lstfac, depale)
    !==========

    ! Free memory
    deallocate(lstfac)

  endif

endif

!===============================================================================
! 3. DEPLACEMENT AU PAS DE TEMPS PRECEDENT ET SAUVEGARDE FLUX ET DE P
!===============================================================================

if (italim.eq.1) then
  do istr = 1, nbstru
    do ii = 1, 3
      xsta(ii,istr)   = xstr(ii,istr)
      xpsta(ii,istr)  = xpstr(ii,istr)
      xppsta(ii,istr) = xppstr(ii,istr)
    enddo
  enddo
  if (ineefl.eq.1) then
    do ifac = 1, nfac
      flmalf(ifac) = propfa(ifac,iflmas)
    enddo
    do ifac = 1, nfabor
      flmalb(ifac) = propfb(ifac,iflmab)
      cofale(ifac,1) = coefa(ifac,iclp)
      cofale(ifac,2) = coefa(ifac,iclu)
      cofale(ifac,3) = coefa(ifac,iclv)
      cofale(ifac,4) = coefa(ifac,iclw)
      cofale(ifac,5) = coefb(ifac,iclp)
      cofale(ifac,6) = coefb(ifac,iclu)
      cofale(ifac,7) = coefb(ifac,iclv)
      cofale(ifac,8) = coefb(ifac,iclw)
    enddo
    if (nterup.gt.1) then
      do iel = 1, ncelet
        xprale(iel) = rtpa(iel,ipr)
      enddo
    endif
  endif
endif

!----
! FORMATS
!----



!----
! FIN
!----

end subroutine
