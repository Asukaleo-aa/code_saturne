!-------------------------------------------------------------------------------

!                      Code_Saturne version 3.0.0-betaR4048
!                      --------------------------
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

subroutine cs_user_initialization &
!================================

 ( nvar   , nscal  ,                                              &
   dt     , rtp    , propce , propfa , propfb )

!===============================================================================
! Purpose:
! -------

!    User subroutine.

!    Initialize variables

! This subroutine is called at beginning of the computation
! (restart or not) before the loop time step

! This subroutine enables to initialize or modify (for restart)
!     unkown variables and time step values

! rom and viscl values are equal to ro0 and viscl0 or initialize
! by reading the restart file
! viscls and cp variables (when there are defined) have no value
! excepted if they are read from a restart file

! Physical quantities are defined in the following arrays:
!  propce (physical quantities defined at cell center),
!  propfa (physical quantities defined at interior face center),
!  propfa (physical quantities defined at border face center).
!
! Examples:
!  propce(iel, ipproc(irom  )) means rom  (iel)
!  propce(iel, ipproc(iviscl)) means viscl(iel)
!  propce(iel, ipproc(icp   )) means cp   (iel)
!  propce(iel, ipproc(ivisls(iscal))) means visls(iel, iscal)
!  propfa(ifac, ipprof(ifluma(ivar))) means flumas(ifac, ivar)
!  propfb(ifac, ipprob(irom )) means romb  (ifac)
!  propfb(ifac, ipprob(ifluma(ivar))) means flumab(ifac, ivar)

! Modification of the behaviour law of physical quantities (rom, viscl,
! viscls, cp) is not done here. It is the purpose of the user subroutine
! usphyv

! Cells identification
! ====================

! Cells may be identified using the 'getcel' subroutine.
! The syntax of this subroutine is described in the
! 'cs_user_boundary_conditions' subroutine,
! but a more thorough description can be found in the user guide.


!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp(ncelet, *)   ! ra ! <-- ! computed variables at cell centers at current  !
!                  !    !     ! time steps                                     !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
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
use numvar
use optcal
use cstphy
use cstnum
use entsor
use parall
use period
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use atincl
use ctincl
use elincl
use ppcpfu
use cs_coal_incl
use cs_fuel_incl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

double precision dt(ncelet), rtp(ncelet,*), propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)

! Local variables

! INSERT_VARIABLE_DEFINITIONS_HERE

integer, allocatable, dimension(:) :: lstelt
integer  iel
integer  iccfth, iscal, imodif

double precision, allocatable, dimension(:) :: w1, w2, w3, w4

!===============================================================================


!===============================================================================
! Initialization
!===============================================================================

allocate(lstelt(ncel)) ! temporary array for cells selection
allocate(w1(ncelet), w2(ncelet), w3(ncelet),w4(ncelet))
imodif = 1
!===============================================================================
! Unknown variable initialization
!      for initial calculations (not in case of restart)
!===============================================================================

if ( isuite.eq.0 ) then

! --- Velocity components

  do iel = 1, ncel
    rtp(iel,iu) = 0.d0
    rtp(iel,iv) = 0.d0
    rtp(iel,iw) = 0.d0
  enddo


! --- User defined scalars

  ! If there are user defined scalars
  if(nscaus.gt.0) then
    ! For each scalar
    do iscal = 1, nscaus
      ! If the scalar is associated to the considered phase iphas
!      if(iphsca(iscal).eq.iphas) then

        ! Initialize each cell value
        do iel = 1, ncel
          rtp(iel,isca(iscal)) = 0.d0
        enddo

!      endif
    enddo
  endif
! --- Pressure, Density, Temperature, Total Energy

  ! Only 2 out of these 4 variables are independent: one may choose to
  ! initialize any pair of variables picked out of these 4, except
  ! (Temperature-Energy). The remaining 2 variables will be deduced
  ! automatically.


  ! Initialize 2 and only 2 variables

  !   To do so, set iutile=1 for each of the 2 selected variables
  !             and iutile=0 for each of the 2 others

  !   In the example provided below, Pressure and Temperature are
  !   initialized.


  ! iccfth indicates which variables have been set:
  !   it is completed automatically for each variable and
!     it must not be modified.
  iccfth = 10000


  ! 1. Pressure (Pa)
  if(.true.) then
    iccfth = iccfth*2
    do iel = 1, ncel
      rtp(iel,ipr) = p0
    enddo
  endif

  ! 2. Density (kg/m3)
  if(.true.) then
    iccfth = iccfth*3
    do iel = 1, ncel
      rtp(iel,isca(irho)) = ro0
    enddo
  endif

  ! 3. Temperature (K -- Warning: Kelvin)
  if(.false.) then
    iccfth = iccfth*5
    do iel = 1, ncel
      rtp(iel,isca(itempk)) = t0
    enddo
  endif

  ! 4. Total Energy (J/kg)
  if(.false.) then
    iccfth = iccfth*7
    do iel = 1, ncel
      rtp(iel,isca(ienerg)) = cv0*t0
    enddo
  endif


  ! ** The following subroutine returns automatically the values for the
  ! two remaining variables that need to be computed, using the
  ! indicator iccfth.

  call cfther                                                    &
  !==========
( nvar   , nscal  ,                                              &
  iccfth , imodif ,                                              &
  dt     , rtp    , rtp   , propce , propfa , propfb ,           &
  w1     , w2     , w3    , w4     )


endif

!--------
! Formats
!--------

!----
! End
!----

deallocate(lstelt) ! temporary array for cells selection
deallocate(w1, w2, w3, w4)

return
end subroutine cs_user_initialization
