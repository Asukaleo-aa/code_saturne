!-------------------------------------------------------------------------------

!                      Code_Saturne version 2.1.0-alpha1
!                      --------------------------

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

subroutine usmpst &
!================

 ( ipart  ,                                                       &
   nvar   , nscal  , nvlsta ,                                     &
   ncelps , nfacps , nfbrps ,                                     &
   imodif ,                                                       &
   itypps ,                                                       &
   lstcel , lstfac , lstfbr ,                                     &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  , statis ,                                     &
   tracel , trafac , trafbr )

!===============================================================================
! Purpose:
! -------

!    User subroutine.

! Modify list of cells or faces defining an existing post-processing
! output mesh; this subroutine is called for true (non-alias) user meshes,
! for each time step at which output on this mesh is active, and only if
! all writers associated with this mesh allow mesh modification
! (i.e. were defined with 'indmod' = 2 or 12).

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! ipart            ! i  ! <-- ! number of the post-processing mesh (< 0 or > 0)!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! nvlsta           ! i  ! <-- ! number of Lagrangian statistical variables     !
! ncelps           ! i  ! <-- ! number of cells in post-processing mesh        !
! nfacps           ! i  ! <-- ! number of interior faces in post-process. mesh !
! nfbrps           ! i  ! <-- ! number of boundary faces in post-process. mesh !
! imodif           ! i  ! --> ! 0 if the mesh was not modified by this call,   !
!                  !    !     ! 1 if it has been modified.                     !
! itypps(3)        ! ia ! <-- ! global presence flag (0 or 1) for cells (1),   !
!                  !    !     ! interior faces (2), or boundary faces (3) in   !
!                  !    !     ! post-processing mesh                           !
! lstcel(ncelps)   ! ia ! --> ! list of cells in post-processing mesh          !
! lstfac(nfacps)   ! ia ! --> ! list of interior faces in post-processing mesh !
! lstfbr(nfbrps)   ! ia ! --> ! list of boundary faces in post-processing mesh !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! statis           ! ra ! <-- ! statistic means                                !
!  (ncelet, nvlsta)!    !     !                                                !
! tracel(*)        ! ra ! --- ! work array for post-processed cell values      !
! trafac(*)        ! ra ! --- ! work array for post-processed face values      !
! trafbr(*)        ! ra ! --- ! work array for post-processed boundary face v. !
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use pointe
use entsor
use optcal
use numvar
use parall
use period
use mesh

!===============================================================================

implicit none

! Arguments

integer          ipart
integer          nvar   , nscal  , nvlsta
integer          ncelps , nfacps , nfbrps
integer          imodif

integer          itypps(3)
integer          lstcel(ncelps), lstfac(nfacps), lstfbr(nfbrps)

double precision dt(ncelet), rtpa(ncelet,*), rtp(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision statis(ncelet,nvlsta)
double precision tracel(ncelps*3)
double precision trafac(nfacps*3), trafbr(nfbrps*3)

! Local variables

integer          ii

!===============================================================================

! Note:

! The 'itypps" array allows determining if the mesh contains at first cells,
! interior faces, or boundary faces (in a global sense when in parallel).

! This enables using "generic" selection criteria, which may function on any
! post-processing mesh, but if such a mesh is empty for a given call to this
! function, we will not know at the next call if it contained cells of faces.
! In this case, it may be preferable to use its number to decide if it should
! contain cells or faces.


!===============================================================================
!     1. TRAITEMENT DES MAILLAGES POST A REDEFINIR
!         A RENSEIGNER PAR L'UTILISATEUR aux endroits indiques
!===============================================================================

! Example: mesh 2 : cells where T < 21 degrees

if (ipart.eq.2) then

  imodif = 1

  ncelps = 0
  nfacps = 0
  nfbrps = 0


  ! If the mesh contains cells
  ! --------------------------

  if (itypps(1) .eq. 1) then

    do ii = 1, ncel

      if (rtp(ii,isca(1)) .le. 21.d0) then
        ncelps = ncelps + 1
        lstcel(ncelps) = ii
      endif

    enddo
  endif

endif ! end of test on post-processing mesh number

return

end subroutine
