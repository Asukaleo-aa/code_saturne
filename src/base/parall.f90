!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2012 EDF S.A.
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

!> \file parall.f90
!> Module for basic MPI and OpenMP parallelism-related values

module parall

  !=============================================================================

  ! thr_n_min : minimum number of elements for loops on threads

  integer   thr_n_min
  parameter(thr_n_min = 128)

  ! irangp : process rank
  !   = -1 in sequential mode
  !   =  r (0 < r < n_processes) in distributed parallel run
  ! nrangp : number of processes (=1 if sequental)
  ! nthrdi : maximum number of independent interior face subsets in a group
  ! nthrdi : maximum number of independent boundary face subsets in a group
  ! ngrpi  : number of interior face groups (> 1 with OpenMP, 1 otherwise)
  ! ngrpb  : number of boundary face groups (> 1 with OpenMP, 1 otherwise)
  ! iompli : per-thread bounds for interior faces
  ! iomplb : per-thread bounds for boundary faces
  !          (for group j and thread i, loops
  !           from iompl.(1, j, i) to iompl.(2, j, i)

  integer, save ::  irangp, nrangp, nthrdi, nthrdb, ngrpi, ngrpb

  integer, dimension(:,:,:), allocatable :: iompli
  integer, dimension(:,:,:), allocatable :: iomplb

  ! Global dimensions (i.e. independent of parallel partitioning)
  !   ncelgb : global number of cells
  !   nfacgb : global number of interior faces
  !   nfbrgb : global number of boundary faces
  !   nsomgb : global number of vertices

  integer(kind=8), save :: ncelgb, nfacgb, nfbrgb, nsomgb

  ! Forced vectorization flags (not used anymore at the moment)
  !   ivecti : force vectorization of interior face -> cell loops (0/1)
  !   ivectb : force vectorization of boundary face -> cell loops (0/1)

  integer, save :: ivecti , ivectb

contains

  !=============================================================================

  ! Initialize OpenMP-related values

  subroutine init_fortran_omp &
             (nfac, nfabor, nthrdi_in, nthrdb_in, &
              ngrpi_in, ngrpb_in, idxfi, idxfb)

    ! Arguments

    integer, intent(in) :: nfac, nfabor
    integer, intent(in) :: nthrdi_in, nthrdb_in, ngrpi_in, ngrpb_in
    integer, dimension(*), intent(in) :: idxfi, idxfb

    ! Local variables

    integer ii, jj
    integer err

    ! Set numbers of threads and groups

    nthrdi = nthrdi_in
    nthrdb = nthrdb_in
    ngrpi  = ngrpi_in
    ngrpb  = ngrpb_in

    if (.not.allocated(iompli)) then
      allocate(iompli(2, ngrpi, nthrdi), stat=err)
    endif

    if (err .eq. 0 .and. .not.allocated(iomplb)) then
      allocate(iomplb(2, ngrpb, nthrdb), stat=err)
    endif

    if (err /= 0) then
      write (*, *) "Error allocating thread/group index array."
      call csexit(err)
    endif

    ! For group j and thread i, loops on faces from
    ! iompl.(1, j, i) to iompl.(2, j, i).

    ! By default (i.e. without Open MP), 1 thread and one group

    iompli(1, 1, 1) = 1
    iompli(2, 1, 1) = nfac

    iomplb(1, 1, 1) = 1
    iomplb(2, 1, 1) = nfabor

    ! Numberings for OpenMP loops on interior faces

    if (nthrdi.gt.1 .or. ngrpi.gt.1) then

      do ii = 1, nthrdi
        do jj = 1, ngrpi
          iompli(1, jj, ii) = idxfi((ii-1)*ngrpi*2 + 2*jj - 1) + 1
          iompli(2, jj, ii) = idxfi((ii-1)*ngrpi*2 + 2*jj)
        enddo
      enddo

    endif

    ! Numberings for OpenMP loops on boundary faces

    if (nthrdb.gt.1 .or. ngrpb.gt.1) then

      do ii = 1, nthrdb
        do jj = 1, ngrpb
          iomplb(1, jj, ii) = idxfb((ii-1)*ngrpb*2 + 2*jj - 1) + 1
          iomplb(2, jj, ii) = idxfb((ii-1)*ngrpb*2 + 2*jj)
        enddo
      enddo

    endif

    return

  end subroutine init_fortran_omp

  !=============================================================================

  ! Free OpenMP-related arrays

  subroutine finalize_fortran_omp

    nthrdi = 0
    nthrdb = 0
    ngrpi  = 0
    ngrpb  = 0

    if (allocated(iompli)) then
      deallocate(iompli)
    endif

    if (allocated(iomplb)) then
      deallocate(iomplb)
    endif

    return

  end subroutine finalize_fortran_omp

  !=============================================================================

end module parall


