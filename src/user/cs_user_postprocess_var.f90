!-------------------------------------------------------------------------------

!VERS

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

subroutine usvpst &
!================

 ( ipart  ,                                                       &
   nvar   , nscal  , nvlsta ,                                     &
   ncelps , nfacps , nfbrps ,                                     &
   itypps ,                                                       &
   lstcel , lstfac , lstfbr ,                                     &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   statis )

!===============================================================================
! Purpose:
! -------

!    User subroutine.

!    Output additional variables on a postprocessing mesh.

! Several "automatic" postprocessing meshes may be defined:
! - The volume mesh (ipart=-1) if 'ichrvl' = 1
! - The boundary mesh (ipart=-2) if 'ichrbo' = 1
! - SYRTHES coupling surface (ipart < -2) if 'ichrsy' = 1
! - Cooling tower exchange zone meshes (ipart < -2) if 'ichrze' = 1
!
! Additional meshes (cells or faces) may also be defined through the GUI or
! using the cs_user_postprocess_meshes() function from the
! cs_user_postprocess.c file.

! This subroutine is called once for each post-processing mesh
! (with a different value of 'ipart') for each time step at which output
! on this mesh is active.

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
! itypps(3)        ! ia ! <-- ! global presence flag (0 or 1) for cells (1),   !
!                  !    !     ! interior faces (2), or boundary faces (3) in   !
!                  !    !     ! post-processing mesh                           !
! lstcel(ncelps)   ! ia ! <-- ! list of cells in post-processing mesh          !
! lstfac(nfacps)   ! ia ! <-- ! list of interior faces in post-processing mesh !
! lstfbr(nfbrps)   ! ia ! <-- ! list of boundary faces in post-processing mesh !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! statis           ! ra ! <-- ! statistic values (Lagrangian)                  !
!  (ncelet, nvlsta)!    !     !                                                !
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use cstnum
use pointe
use entsor
use optcal
use numvar
use parall
use period
use mesh
use field
use post

!===============================================================================

implicit none

! Arguments

integer          ipart
integer          nvar,   nscal , nvlsta
integer          ncelps, nfacps, nfbrps

integer          itypps(3)
integer          lstcel(ncelps), lstfac(nfacps), lstfbr(nfbrps)

double precision dt(ncelet), rtpa(ncelet,*), rtp(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision statis(ncelet,nvlsta)

! Local variables

character*32     namevr

integer          ntindp
integer          iel, ifac, iloc, ivar
integer          idimt, ii , jj
logical          ientla, ivarpr
integer          imom1, imom2, ipcmo1, ipcmo2, idtcm
double precision pnd
double precision rvoid(1)

double precision, dimension(:), allocatable :: scel, sfac, sfbr
double precision, dimension(:,:), allocatable :: vcel, vfac, vfbr
double precision, dimension(:), pointer :: coefap, coefbp

integer          intpst
data             intpst /0/
save             intpst

!===============================================================================

! TEST_TO_REMOVE_FOR_USE_OF_SUBROUTINE_START
!===============================================================================

if(1.eq.1) return

!===============================================================================
! TEST_TO_REMOVE_FOR_USE_OF_SUBROUTINE_END

!===============================================================================
! Increment call counter once per time step (possibly used in some tests)
!===============================================================================

if (ipart .eq. -1) then
  intpst = intpst + 1
endif

!===============================================================================
! 1. Handle variables to output
!    MUST BE FILLED IN by the user at indicated places
!===============================================================================

! The ipart argument matches a post-processing maehs id (using the EnSight
! vocabulary; the MED and CGNS equivalents are "mesh" and "base" respectively).
! The user will have defined post-processing meshes using the GUI or the
! cs_user_postprocess_meshes() function from the cs_user_postprocess.c
! file.

! This subroutine is called once for each post-processing mesh
! (with a different value of 'ipart') for each time step at which output
! on this mesh is active. For each mesh and for all variables we wish to
! post-process here, we must define certain parameters and pass them to
! the 'post_write_var' subroutine, which is in charge of the actual output.
! These parameters are:

! namevr <-- variable name
! idimt  <-- variable dimension
!            (1: scalar, 3: vector, 6: symmetric tensor, 9: tensor)
! ientla <-- when idimt >1, this flag specifies if the array containing the
!            variable values is interlaced when ientla = .true.
!            (x1, y1, z1, x2, y2, z2, x3, y3, z3...), or non-interlaced when
!            ientla = .false. (x1, x2, x3,...,y1, y2, y3,...,z1, z2, z3,...).
! ivarpr <-- specifies if the array containing the variable is defined on
!            the "parent" mesh or locally.
!            Even if the 'ipart' post-processing mesh contains all the
!            elements of its parent mesh, their numbering may be different,
!            especially when different element types are present.
!            A local array passed as an argument to 'post_write_var' is built
!            relative to the numbering of the 'ipart' post-processing mesh.
!            To post-process a variable contained for example in the 'user'
!            array, it should first be re-ordered, as shown here:
!              do iloc = 1, ncelps
!                iel = lstcel(iloc)
!                scel(iloc) = user(iel)
!              enddo
!            An alternative option is provided, to avoid unnecessary copies:
!            an array defined on the parent mesh, such our 'user' example,
!            may be passed directly to 'post_write_var', specifying that values
!            are defined on the parent mesh instead of the post-processing mesh,
!            by setting the 'ivarpr' argument of 'post_write_var' to .true..

! Note: be cautious with variable name lengths.

! We allow up to 32 characters here, but names may be truncted depending on the
! output format.

! The name length is not limited internally, so in case of 2 variables whoses
! names differ only after the truncation character, the corresponding names will
! both appear in the ".case" file; simply renaming one of the field descriptors
! in this text file will correct the output.

! Whitespace at the beginning or the end of a line is truncated automatically.
! Depending on the format used, prohibited characters (under EnSight, characters
! (  ) ] [ + - @           ! # * ^ $ / as well as white spaces and tabulations
! are automatically replaced by the _ character.

! Examples:

!   For post-processing mesh 2, we output the velocity, pressure, and prescribed
!   temperature at boundary faces (as well as 0 on possible interior faces)

!   For post-processing mesh 1, we output all the variables usually
!   post-processed, using a more compact coding.

!   Examples given here correspond to the meshes defined in
!   cs_user_postprocess.c

!===============================================================================
! Examples of volume variables on the main volume mesh (ipart = -1)
!===============================================================================

if (ipart .eq. -1) then

  ! Output of k=1/2(R11+R22+R33) for the Rij-epsilon model
  ! ------------------------------------------------------

  if (itytur .eq. 3) then

    allocate(scel(ncelps))

    do iloc = 1, ncelps
      iel = lstcel(iloc)
      scel(iloc) = 0.5d0*(  rtp(iel,ir11)  &
                          + rtp(iel,ir22)  &
                          + rtp(iel,ir33))
    enddo

    idimt = 1        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
    ientla = .true.  ! dimension 1 here, so no effect
    ivarpr = .false. ! defined on the work array, not on the parent

    ! Output values; as we have no face values, we can pass a
    ! trivial array rvoid for those.
    call post_write_var(ipart, 'Turb energy', idimt, ientla, ivarpr,  &
                        ntcabs, ttcabs, scel, rvoid, rvoid)

    deallocate(scel)

  endif


  ! Output of a combination of moments
  ! ----------------------------------

  ! We assume in this example that we have 2 temporal means (moments):
  !   <u>  for imom=1
  !   <uu> for imom=2
  ! We seek to plot <u'u'>=<uu>-<U>**2

  if (nbmomt .ge. 2) then

    ! Moment numbers:
    imom1 = 1
    imom2 = 2

    ! Position in 'propce' of the array of temporal accumulation for moments,
    ! propce(iel,ipcmom)
    ipcmo1 = ipproc(icmome(imom1))
    ipcmo2 = ipproc(icmome(imom2))

    ! The temporal accumulation for moments must be divided by the accumulated
    ! time, which id an array of size ncel or a single real number:
    ! - array of size ncel if idtmom(imom) > 0 : propce(iel, idtcm)
    ! - or simple real     if idtmom(imom) < 0 : dtcmom(idtcm)

    ! To improve this example's readability, we assume moments imom1 and imom2
    ! have been computed on the same time window.

    allocate(scel(ncelps))

    if (idtmom(imom1).gt.0) then
      idtcm = ipproc(icdtmo(idtmom(imom1)))
      do iloc = 1, ncelps
        iel = lstcel(iloc)
        scel(iloc) =    propce(iel,ipcmo2)/max(propce(iel,idtcm),epzero)      &
                     - (propce(iel,ipcmo1)/max(propce(iel,idtcm),epzero))**2
      enddo
    else if (idtmom(imom1).lt.0) then
      idtcm = -idtmom(imom1)
      do iloc = 1, ncelps
        iel = lstcel(iloc)
        scel(iloc) =    propce(iel,ipcmo2)/max(dtcmom(idtcm),epzero)      &
                     - (propce(iel,ipcmo1)/max(dtcmom(idtcm),epzero))**2
      enddo
    endif

    idimt = 1        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
    ientla = .true.  ! dimension 1 here, so no effect
    ivarpr = .false. ! defined on the work array, not on the parent

    ! Output values; as we have no face values, we can pass a
    ! trivial array for those.
    call post_write_var(ipart, '<upup>', idimt, ientla, ivarpr,  &
                        ntcabs, ttcabs, scel, rvoid, rvoid)

    deallocate(scel)

  endif

!===============================================================================
! Examples of volume variables on the boundary mesh (ipart = -2)
!===============================================================================

else if (ipart .eq. -2) then

  ! Output of the density at the boundary
  ! -------------------------------------

  idimt = 1        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
  ientla = .true.  ! dimension 1 here, so no effect
  ivarpr = .true.  ! we use the propfb array defined on the parent mesh

  ! Output values; as we have no cell or interior face values, we can pass a
  ! trivial array for those.
  call post_write_var(ipart, 'Density at boundary', idimt, ientla, ivarpr,    &
                      ntcabs, ttcabs, rvoid, rvoid, propfb(1,ipprob(irom)))

!===============================================================================
! Examples of volume variables on user meshes 1 or 2
!===============================================================================

else if (ipart.eq.1 .or. ipart.eq.2) then

  ! Output of the velocity
  ! ----------------------

  ! Compute variable values on interior faces.
  ! In this example, we use a simple linear interpolation.
  ! For parallel calculations, if neighbors are used, they must be synchronized
  ! first. This also applies for periodicity.

  if (irangp.ge.0.or.iperio.eq.1) then
    call synvec(rtp(1,iu), rtp(1,iv), rtp(1,iw))
    !==========
  endif

  allocate(vfac(3,nfacps), vfbr(3,nfbrps))

  do iloc = 1, nfacps

    ifac = lstfac(iloc)
    ii = ifacel(1, ifac)
    jj = ifacel(2, ifac)
    pnd = pond(ifac)

    vfac(1,iloc) = pnd  * rtp(ii,iu) + (1.d0 - pnd) * rtp(jj,iu)
    vfac(2,iloc) = pnd  * rtp(ii,iv) + (1.d0 - pnd) * rtp(jj,iv)
    vfac(3,iloc) = pnd  * rtp(ii,iw) + (1.d0 - pnd) * rtp(jj,iw)

  enddo

  ! Compute variable values on boundary faces.
  ! In this example, we use a simple copy of the adjacent cell value.

  do iloc = 1, nfbrps

    ifac = lstfbr(iloc)
    ii = ifabor(ifac)

    vfbr(1,iloc) = rtp(ii, iu)
    vfbr(2,iloc) = rtp(ii, iv)
    vfbr(3,iloc) = rtp(ii, iw)

  enddo

  idimt = 3        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
  ientla = .true.  ! interleaved
  ivarpr = .false. ! defined on the work array, not on the parent

  ! Output values; as we have no cell values, we can pass a
  ! trivial array for those.
  call post_write_var(ipart, 'Interpolated velocity', idimt, ientla, ivarpr,  &
                      ntcabs, ttcabs, rvoid, vfac, vfbr)

  deallocate(vfac, vfbr)

  ! Output of the pressure
  ! ----------------------

  ! Variable number
  ivar = ipr

  ! Compute variable values on interior faces.
  ! In this example, we use a simple linear interpolation.
  ! For parallel calculations, if neighbors are used, they must be synchronized
  ! first. This also applies for periodicity.

  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(rtp(1,ivar))
    !==========
  endif

  allocate(sfac(nfacps), sfbr(nfbrps))

  do iloc = 1, nfacps

    ifac = lstfac(iloc)
    ii = ifacel(1, ifac)
    jj = ifacel(2, ifac)
    pnd = pond(ifac)

    sfac(iloc) =           pnd  * rtp(ii, ivar)  &
                 + (1.d0 - pnd) * rtp(jj, ivar)

  enddo

  ! Compute variable values on boundary faces.
  ! In this example, we use a simple copy of the adjacent cell value.

  do iloc = 1, nfbrps

    ifac = lstfbr(iloc)
    ii = ifabor(ifac)

    sfbr(iloc) = rtp(ii, ivar)

  enddo

  idimt = 1        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
  ientla = .true.  ! dimension 1 here, so no effect
  ivarpr = .false. ! defined on the work array, not on the parent

  ! Output values; as we have no cell values, we can pass a
  ! trivial array for those.
  call post_write_var(ipart, 'Interpolated pressure', idimt, ientla, ivarpr,  &
                      ntcabs, ttcabs, rvoid, sfac, sfbr)

  deallocate(sfac, sfbr)

  ! The examples below illustrate how to output a same variable in different
  ! ways (interlaced or not, using an indirection or not).


  ! Output of the centers of gravity, interlaced
  ! --------------------------------

  if (intpst.eq.1) then

    allocate(vfac(3,nfacps), vfbr(3,nfbrps))

    do iloc = 1, nfacps

      ifac = lstfac(iloc)

      vfac(1,iloc) = cdgfac(1, ifac)
      vfac(2,iloc) = cdgfac(2, ifac)
      vfac(3,iloc) = cdgfac(3, ifac)

    enddo

    ! Compute variable values on boundary faces

    do iloc = 1, nfbrps

      ifac = lstfbr(iloc)

      vfbr(1, iloc) = cdgfbo(1, ifac)
      vfbr(2, iloc) = cdgfbo(2, ifac)
      vfbr(3, iloc) = cdgfbo(3, ifac)

    enddo

    ! We assign a negative time step and output this variable once only
    ! to avoid duplicating it at each output time (assuming a fixed mesh).
    ntindp = -1

    idimt = 3        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
    ientla = .true.  ! interleaved
    ivarpr = .false. ! defined on the work array, not on the parent

    ! Output values; as we have no cell values, we can pass a
    ! trivial array for those.
    call post_write_var(ipart, 'face cog (interlaced)', idimt,               &
                        ientla, ivarpr,                                      &
                        ntindp, ttcabs, rvoid, vfac, vfbr)

    deallocate(vfac, vfbr)

  endif

  ! Output of the centers of gravity, non-interlaced, time independent
  ! --------------------------------

  if (intpst.eq.1) then

    allocate(vfac(nfacps, 3), vfbr(nfbrps, 3))

    do iloc = 1, nfacps

      ifac = lstfac(iloc)

      vfac(iloc,1) = cdgfac(1, ifac)
      vfac(iloc,2) = cdgfac(2, ifac)
      vfac(iloc,3) = cdgfac(3, ifac)

    enddo

    ! Compute variable values on boundary faces

    do iloc = 1, nfbrps

      ifac = lstfbr(iloc)

      vfbr(iloc,1) = cdgfbo(1, ifac)
      vfbr(iloc,2) = cdgfbo(2, ifac)
      vfbr(iloc,3) = cdgfbo(3, ifac)

    enddo

    ! We assign a negative time step and output this variable once only
    ! to avoid duplicating it at each output time (assuming a fixed mesh).
    ntindp = -1

    idimt = 3         ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
    ientla = .false.  ! not interleaved
    ivarpr = .false.  ! defined on the work array, not on the parent

    ! Output values; as we have no cell values, we can pass a
    ! trivial array for those.
    call post_write_var(ipart, 'face cog (non interlaced)', idimt,           &
                        ientla, ivarpr,                                      &
                        ntindp, ttcabs, rvoid, vfac, vfbr)

    deallocate(vfac, vfbr)

  endif

  ! Output of the centers of gravity, with indirection (parent-based)
  ! --------------------------------

  if (intpst.eq.1) then

    ! We assign a negative time step and output this variable once only
    ! to avoid duplicating it at each output time (assuming a fixed mesh).
    ntindp = -1

    idimt = 3        ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
    ientla = .true.  ! interleaved
    ivarpr = .true.  ! defined on the parent

    ! Output values; as we have no cell values, we can pass a
    ! trivial array for those.
    call post_write_var(ipart, 'face cog (parent)', idimt, ientla, ivarpr,   &
                        ntindp, ttcabs, rvoid, cdgfac, cdgfbo)

  endif

!===============================================================================
! Examples of volume variables on user meshes 3 or 4
!===============================================================================

else if (ipart.ge.3 .and. ipart.le.4) then

  ! Output of the velocity
  ! ----------------------

  ! Compute variable values on interior faces.
  ! In this example, we use a simple linear interpolation.
  ! For parallel calculations, if neighbors are used, they must be synchronized
  ! first. This also applies for periodicity.

  if (irangp.ge.0.or.iperio.eq.1) then
    call synvec(rtp(1,iu), rtp(1,iv), rtp(1,iw))
    !==========
  endif

  allocate(vfac(3,nfacps), vfbr(3,nfbrps))

  do iloc = 1, nfacps

    ifac = lstfac(iloc)
    ii = ifacel(1, ifac)
    jj = ifacel(2, ifac)
    pnd = pond(ifac)

    vfac(1,iloc) =            pnd  * rtp(ii, iu)   &
                    + (1.d0 - pnd) * rtp(jj, iu)
    vfac(2,iloc) =            pnd  * rtp(ii, iv)   &
                    + (1.d0 - pnd) * rtp(jj, iv)
    vfac(3,iloc) =            pnd  * rtp(ii, iw)   &
                    + (1.d0 - pnd) * rtp(jj, iw)

  enddo

  ! Compute variable values on boundary faces.
  ! In this example, we use a simple copy of the adjacent cell value.

  do iloc = 1, nfbrps

    ifac = lstfbr(iloc)
    ii = ifabor(ifac)

    vfbr(1,iloc) = rtp(ii, iu)
    vfbr(2,iloc) = rtp(ii, iv)
    vfbr(3,iloc) = rtp(ii, iw)

  enddo

  idimt = 3         ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
  ientla = .true.   ! interleaved
  ivarpr = .false.  ! defined on the work array

  ! Output values; as we have no cell values, we can pass a
  ! trivial array for those.
  call post_write_var(ipart, 'Velocity', idimt, ientla, ivarpr,              &
                      ntcabs, ttcabs, rvoid, vfac, vfbr)

  deallocate(vfac, vfbr)

  ! Output of the pressure
  ! ----------------------

  ! Variable number
  ivar = ipr

  ! Compute variable values on interior faces.
  ! In this example, we use a simple linear interpolation.
  ! For parallel calculations, if neighbors are used, they must be synchronized
  ! first. This also applies for periodicity.

  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(rtp(1,ivar))
    !==========
  endif

  allocate(sfac(nfacps), sfbr(nfbrps))

  do iloc = 1, nfacps

    ifac = lstfac(iloc)
    ii = ifacel(1, ifac)
    jj = ifacel(2, ifac)
    pnd = pond(ifac)

    sfac(iloc)  =           pnd  * rtp(ii, ivar)   &
                  + (1.d0 - pnd) * rtp(jj, ivar)

  enddo

  ! Compute variable values on boundary faces.
  ! In this example, we use a simple copy of the adjacent cell value.

  do iloc = 1, nfbrps

    ifac = lstfbr(iloc)
    ii = ifabor(ifac)

    sfbr(iloc) = rtp(ii, ivar)

  enddo

  idimt = 1         ! 1: scalar, 3: vector, 6/9: symm/non-symm tensor
  ientla = .true.   ! interleaved
  ivarpr = .false.  ! defined on the work array

  ! Output values; as we have no cell values, we can pass a
  ! trivial array for those.
  call post_write_var(ipart, 'Pressure', idimt, ientla, ivarpr,              &
                      ntcabs, ttcabs, rvoid, sfac, sfbr)

  deallocate(sfac, sfbr)

endif ! end of test on post-processing mesh number

return

end subroutine usvpst
