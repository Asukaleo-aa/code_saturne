!-------------------------------------------------------------------------------

!VERS

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2011 EDF S.A.
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

subroutine uscfx2
!================


! Purpose:
! -------

!    User subroutine.

!    Set options for viscosity and conductivity for compressible flow.

!    In addition to options set in the user subroutine 'usini1' (or in
!    the GUI): this subroutine allows to set switches to indicate if the
!    volumetric viscosity and the conductivity are constants. If they are,
!    the subroutines allows to set their values.


!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
!    nom           !type!mode !                   role                         !
!__________________!____!_____!________________________________________________!
!__________________!____!_____!________________________________________________!


!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens
use numvar
use optcal
use cstphy
use entsor
use cstnum
use ppppar
use ppthch
use ppincl

!===============================================================================

implicit none

! Arguments

! Local variables

!===============================================================================

! TEST_TO_REMOVE_FOR_USE_OF_SUBROUTINE_START
!===============================================================================
! 0.  This test allows the user to ensure that the version of this subroutine
!       used is that from his case definition, and not that from the library.
!     This subroutine is  mandatory for compressible flow,
!       thus the default (library reference) version stops immediately.
!===============================================================================

if(1.eq.1) then
  write(nfecra,9000)
  call csexit (1)
endif

 9000 format(                                                     &
'@',/,                                                            &
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',/,                                                            &
'@ @@ WARNING:    stop in data input for compressible flow',/,    &
'@    =======',/,                                                 &
'@     The user subroutine ''uscfx2'' must be completed',/,       &
'@',/,                                                            &
'@  The calculation will not be run.',/,                          &
'@',/,                                                            &
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',/)


! TEST_TO_REMOVE_FOR_USE_OF_SUBROUTINE_END

!===============================================================================
! 1. Physical properties
!===============================================================================

! --> Molecular thermal conductivity

!       constant  : ivisls = 0
!       variable  : ivisls = 1

ivisls(itempk) = 0

!       Reference molecular thermal conductivity
!       visls0 = lambda0  (molecular thermal conductivity, W/(m K))

!       WARNING: visls0 must be strictly positive
!         (set a realistic value here even if conductivity is variable)

visls0(itempk) = 3.d-2

!       If the molecular thermal conductivity is variable, its values
!         must be provided in the user subroutine 'uscfpv'


! --> Volumetric molecular viscosity

!       Reference volumetric molecular viscosity

!       viscv0 = kappa0  (volumetric molecular viscosity, kg/(m s))
!       iviscv = 0 : uniform  in space and constant in time
!              = 1 : variable in space and time

iviscv = 0
viscv0 = 0.d0

!       If the volumetric molecular viscosity is variable, its values
!         must be provided in the user subroutine 'uscfpv'


!----
! End
!----

return
end subroutine
