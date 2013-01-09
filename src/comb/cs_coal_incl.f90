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

!> \file cs_coal_incl.f90
!> Module for coal combustion

module cs_coal_incl

  !=============================================================================

  use ppppar

  ! Combustion du coke par H2O

  integer, save :: ihth2o , ighh2o(nclcpm)

  ! Modele de NOx
  ! qpr : % d'azote libere pendant la devol.% de MV libere pendant la devol.
  ! fn : concentration en azote sur pur

  double precision, save :: qpr(ncharm), fn(ncharm), ipci(ncharm), xashsec(ncharm)

  !=============================================================================

end module cs_coal_incl
