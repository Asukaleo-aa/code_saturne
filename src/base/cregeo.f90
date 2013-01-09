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

subroutine cregeo
!================

!===============================================================================
! Purpose:
! --------

! Complete creation of geometrical entities.

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
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
use entsor
use optcal
use cstphy
use ppppar
use ppthch
use ppincl
use ctincl
use mesh
use post

!===============================================================================

implicit none

! Arguments


! Local variables

integer          nbrsyr , nbzech
character        ficsui*32

!===============================================================================
! 1. Creation of extracted mesh coupled with SYRTHES
!    Send geometrical entities to SYRTHES if necessary
!===============================================================================

!     NOMBRE DE COUPLAGES SYRTHES DEFINIS

call nbcsyr(nbrsyr)
!==========

if (nbrsyr .gt. 0) then
  call geosyr
  !==========
endif

!===============================================================================
! 2. Create extruded mesh for cooling tower exchange zones
!===============================================================================

if (ippmod(iaeros).ge.0) then

  call usctdz
  !==========

  call nbzect(nbzech)
  !==========

  if (nbzech .gt. 0) then
    call geoct
    !=========
    if (ichrze.gt.0) then
      call pstict
      !==========
    endif
  endif

  if (ippmod(iaeros).ge.0.and.isuict.eq.1) then
     ficsui = 'cooling_towers'
     call lecctw (ficsui , len(ficsui))
     !==========
  endif

endif

!===============================================================================
! 3. Write time-independent post-processing meshes
!===============================================================================

call cs_post_write_meshes (ntcabs, ttcabs)

!===============================================================================
! 4. Filter extended neighborhood for least-squares gradients
!===============================================================================

if (imrgra.eq.3 .or. imrgra.eq.6 .or. imrgra.eq.-3 .or. imrgra.eq.-6) then
  call redvse (anomax)
  !==========
endif


return
end subroutine
