!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2010 EDF S.A., France

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

subroutine mmtycl &
!================

 ( nvar   , nscal  ,                                              &
   itypfb , icodcl ,                                              &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   rcodcl )

!===============================================================================
! FONCTION :
! --------

! TRAITEMENT DES CODES DE CONDITIONS POUR UN MAILLAGE MOBILE
!   LORS D'UN COUPLAGE DE TYPE ROTOR/STATOR

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! itypfb           ! ia ! <-- ! boundary face types                            !
! icodcl           ! te ! <-- ! code de condition limites aux faces            !
!  (nfabor,nvar    !    !     !  de bord                                       !
!                  !    !     ! = 1   -> dirichlet                             !
!                  !    !     ! = 3   -> densite de flux                       !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! rcodcl           ! tr ! <-- ! valeur des conditions aux limites              !
!  (nfabor,nvar    !    !     !  aux faces de bord                             !
!                  !    !     ! rcodcl(1) = valeur du dirichlet                !
!                  !    !     ! rcodcl(2) = valeur du coef. d'echange          !
!                  !    !     !  ext. (infinie si pas d'echange)               !
!                  !    !     ! rcodcl(3) = valeur de la densite de            !
!                  !    !     !  flux (negatif si gain) w/m2                   !
!                  !    !     ! pour les vitesses (vistl+visct)*gradu          !
!                  !    !     ! pour la pression             dt*gradp          !
!                  !    !     ! pour les scalaires                             !
!                  !    !     !        cp*(viscls+visct/sigmas)*gradt          !
! depmob(nnod,3    ! tr ! <-- ! deplacement aux noeuds                         !
! xyzno1(3,nnod    ! tr ! <-- ! coordonnees noeuds maillage initial            !
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
use numvar
use optcal
use cstnum
use cstphy
use entsor
use parall
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          itypfb(nfabor)
integer          icodcl(nfabor,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision rcodcl(nfabor,nvar,3)
double precision depmob(nnod,3), xyzno1(3,nnod)

! Local variables

integer          ifac, iel
integer          ii, inod, icpt
double precision ddepx, ddepy, ddepz
double precision srfbnf, rnx, rny, rnz
double precision rcodcx, rcodcy, rcodcz, rcodsn
double precision vitbox, vitboy, vitboz


!===============================================================================

!===============================================================================
! 1.  INITIALISATIONS
!===============================================================================


!===============================================================================
! 2.  VITESSE DE DEFILEMENT POUR LES PAROIS FLUIDES ET SYMETRIES
!===============================================================================

! Pour les symetries on rajoute toujours la vitesse de maillage, car on
!   ne conserve que la vitesse normale
! Pour les parois, on prend la vitesse de maillage si l'utilisateur n'a
!   pas specifie RCODCL, sinon on laisse RCODCL pour la vitesse tangente
!   et on prend la vitesse de maillage pour la composante normale.
! On se base uniquement sur ITYPFB, a l'utilisateur de gere les choses
!   s'il rentre en CL non standards.

do ifac = 1, nfabor

  iel = ifabor(ifac)

  ! --- En turbomachine on conna�t la valeur exacte de la vitesse de maillage

  vitbox = omegay*cdgfbo(3,ifac) - omegaz*cdgfbo(2,ifac)
  vitboy = omegaz*cdgfbo(1,ifac) - omegax*cdgfbo(3,ifac)
  vitboz = omegax*cdgfbo(2,ifac) - omegay*cdgfbo(1,ifac)

  if (itypfb(ifac).eq.isymet) then
    rcodcl(ifac,iu,1) = vitbox
    rcodcl(ifac,iv,1) = vitboy
    rcodcl(ifac,iw,1) = vitboz
  endif

  if (itypfb(ifac).eq.iparoi) then
    ! Si une des composantes de vitesse de glissement a ete
    !    modifiee par l'utilisateur, on ne fixe que la vitesse
    !    normale
    if (rcodcl(ifac,iu,1).gt.rinfin*0.5d0 .and.              &
         rcodcl(ifac,iv,1).gt.rinfin*0.5d0 .and.              &
         rcodcl(ifac,iw,1).gt.rinfin*0.5d0) then
      rcodcl(ifac,iu,1) = vitbox
      rcodcl(ifac,iv,1) = vitboy
      rcodcl(ifac,iw,1) = vitboz
    else
      ! On met a 0 les composantes de RCODCL non specifiees
      if (rcodcl(ifac,iu,1).gt.rinfin*0.5d0) rcodcl(ifac,iu,1) = 0.d0
      if (rcodcl(ifac,iv,1).gt.rinfin*0.5d0) rcodcl(ifac,iv,1) = 0.d0
      if (rcodcl(ifac,iw,1).gt.rinfin*0.5d0) rcodcl(ifac,iw,1) = 0.d0

      srfbnf = surfbn(ifac)
      rnx = surfbo(1,ifac)/srfbnf
      rny = surfbo(2,ifac)/srfbnf
      rnz = surfbo(3,ifac)/srfbnf
      rcodcx = rcodcl(ifac,iu,1)
      rcodcy = rcodcl(ifac,iv,1)
      rcodcz = rcodcl(ifac,iw,1)
      rcodsn = (vitbox - rcodcx)*rnx                            &
           + (vitboy - rcodcy)*rny                            &
           + (vitboz - rcodcz)*rnz
      rcodcl(ifac,iu,1) = rcodcx + rcodsn*rnx
      rcodcl(ifac,iv,1) = rcodcy + rcodsn*rny
      rcodcl(ifac,iw,1) = rcodcz + rcodsn*rnz
    endif

  endif
enddo

!===============================================================================
! FORMATS
!===============================================================================

return
end subroutine
