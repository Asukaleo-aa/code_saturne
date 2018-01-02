!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2018 EDF S.A., France

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

!===============================================================================
! Purpose:
! --------

!> \file addfld.f90
!>
!> \brief Add additional fields based on user options.
!>
!> If the user has activated a drift for a scalar for instance,
!> additional fields are created, such an additional mass flux.
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!_______________________________________________________________________________


subroutine addfld

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens
use optcal
use cstphy
use numvar
use entsor
use pointe
use albase
use period
use ppppar
use ppthch
use ppincl
use cfpoin
use lagran
use ihmpre
use cplsat
use mesh
use post
use field
use cs_f_interfaces
use cs_c_bindings

!===============================================================================

implicit none

! Arguments

! Local variables

integer          ii
integer          ifcvsl, kbfid
integer          iflid, iopchr
integer          itycat, ityloc, idim1, idim3
integer          f_id, potr, poti, flag
integer          f_vis, f_log, iut, ialpha
integer          kturt, kfturt
integer          keycpl
integer          ivar
logical          iprev, inoprv, is_set

character(len=80) :: name, f_name, f_label, s_label, s_name
type(var_cal_opt) :: vcopt_dfm, vcopt_alpha

!===============================================================================

!===============================================================================
! 0. Definitions for fields
!===============================================================================

! The itycat variable is used to define field categories. It is used in Fortran
! code with hard-coded values, but in the C API, those values are based on
! (much clearer) category mask definitions in cs_field.h.

itycat = FIELD_INTENSIVE + FIELD_VARIABLE  ! for most variables
ityloc = 1 ! variables defined on cells
idim1  = 1
idim3  = 3
iprev  = .true.    ! variables have previous value
inoprv = .false.   ! variables have no previous value
iopchr = 1         ! Postprocessing level for variables

! Keys not stored globally
call field_get_key_id('turbulent_flux_model', kturt)
call field_get_key_id('turbulent_flux_id', kfturt)
call field_get_key_id('coupled', keycpl)

!===============================================================================
! 0. Initialization
!===============================================================================

call field_get_key_id("boundary_value_id", kbfid)

call field_get_key_id('log', keylog)
call field_get_key_id('label', keylbl)

!===============================================================================
! 1. Additional variable fields
!===============================================================================

! User variables
!---------------

do ii = 1, nscal

  if (isca(ii) .gt. 0) then

    ivar = isca(ii)
    f_id = ivarfl(ivar)

    call field_get_key_int(ivarfl(ivar), keyvis, f_vis)
    call field_get_key_int(ivarfl(ivar), keylog, f_log)

    if (ityturt(ii).gt.0) then
      call field_get_name (f_id, name)
      f_name = trim(name)//'_turbulent_flux'

      if (ityturt(ii).eq.3) then
        call add_variable_field(f_name, f_name, 3, iut)
        iflid = ivarfl(iut)

        call field_set_key_int(iflid, keycpl, 1)
        ! Tensorial diffusivity
        call field_get_key_struct_var_cal_opt(iflid, vcopt_dfm)
        vcopt_dfm%idften = 6
        call field_set_key_struct_var_cal_opt(iflid, vcopt_dfm)

      else
        itycat = FIELD_INTENSIVE + FIELD_PROPERTY  ! for properties

        call field_create(f_name, itycat, ityloc, idim3, iprev, iflid)

        call field_set_key_int(iflid, keyvis, f_vis)
        call field_set_key_int(iflid, keylog, f_log)
      endif

      call field_set_key_int(ivarfl(ivar), kturt, iturt(ii))
      call field_set_key_int(ivarfl(ivar), kfturt, iflid)

      ! Elliptic Blending (AFM or DFM)
      if (iturt(ii).eq.11 .or. iturt(ii).eq.21 .or. iturt(ii).eq.31) then
        f_name = trim(name)//'_alpha'

        call add_variable_field(f_name, f_name, 1, ialpha)
        iflid = ivarfl(ialpha)

        ! Elliptic equation (no convection, no time term)
        call field_get_key_struct_var_cal_opt(iflid, vcopt_alpha)
        vcopt_alpha%iconv = 0
        vcopt_alpha%istat = 0
        call field_set_key_struct_var_cal_opt(iflid, vcopt_alpha)

      endif

    endif

  endif

enddo

!===============================================================================
! 2. Additional property fields
!===============================================================================

! Add a scalar diffusivity when defined as variable.
! The kivisl key should be equal to -1 for constant diffusivity,
! and f_id for a variable diffusivity defined by field f_id
! Assuming the first field created is not a diffusivity property
! (we define variables first), f_id > 0, so we use 0 to indicate
! the diffusivity is variable but its field has not been created yet.

do ii = 1, nscal
  f_id = ivarfl(isca(ii))
  call field_get_key_int(f_id, kivisl, ifcvsl)
  if (ifcvsl.eq.0 .and. iscavr(ii).le.0) then
    ! Build name and label, using a general rule, with a
    ! fixed name for temperature or enthalpy
    call field_get_name(f_id, s_name)
    call field_get_label(f_id, s_label)
    if (ii.eq.iscalt) then
      s_name = 'thermal'
      s_label = 'Th'
    endif
    if (iscacp(ii).gt.0) then
      f_name  = trim(s_name) // '_conductivity'
      f_label = trim(s_label) // ' Cond'
    else
      f_name  = trim(s_name) // '_diffusivity'
      f_label = trim(s_label) // ' Diff'
    endif
    ! Special case for electric arcs: real and imaginary electric
    ! conductivity is the same (and ipotr < ipoti)
    if (ippmod(ieljou).eq.2 .or. ippmod(ieljou).eq.4) then
      call field_get_id('elec_pot_r', potr)
      call field_get_id('elec_pot_i', poti)
      if (f_id.eq.potr) then
        f_name = 'elec_sigma'
        f_label = 'Sigma'
      else if (f_id.eq.poti) then
        call field_get_key_int(potr, kivisl, ifcvsl)
        call field_set_key_int(poti, kivisl, ifcvsl)
        cycle ! go to next scalar in loop, avoid creating property
      endif
    endif
    ! Now create matching property
    call add_property_field(f_name, f_label, 1, .false., ifcvsl)
    call field_set_key_int(ivarfl(isca(ii)), kivisl, ifcvsl)
  endif
enddo

! For variances, the diffusivity is that of the associated scalar,
! and must not be initialized first.

do ii = 1, nscal
  if (iscavr(ii).gt.0) then
    f_id = ivarfl(isca(ii))
    call field_get_key_int(ivarfl(isca(iscavr(ii))), kivisl, ifcvsl)
    call field_is_key_set(f_id, kivisl, is_set)
    if (is_set.eqv..true.) then
      write(nfecra,7040) f_id, ivarfl(isca(iscavr(ii))), ifcvsl
    else
      call field_set_key_int(f_id, kivisl, ifcvsl)
    endif
  endif
enddo

! Add a scalar density when defined as variable and different from the bulk.
! WARNING: it must be consitent with continuity equation, this is used
! for fluid solid computation with apssive scalars with different density in the solid.
! The kromsl key should be equal to -1 for constant diffusivity,
! and f_id for a variable diffusivity defined by field f_id
! Assuming the first field created is not a diffusivity property
! (we define variables first), f_id > 0, so we use 0 to indicate
! the density is variable but its field has not been created yet.

do ii = 1, nscal
  f_id = ivarfl(isca(ii))
  call field_get_key_int(f_id, kromsl, ifcvsl)
  if (ifcvsl.eq.0 .and. iscavr(ii).le.0) then
    ! Build name and label, using a general rule, with a
    ! fixed name for temperature or enthalpy
    call field_get_name(f_id, s_name)
    call field_get_label(f_id, s_label)
    f_name  = trim(s_name) // '_density'
    f_label = trim(s_label) // ' Rho'

    ! Now create matching property
    call add_property_field(f_name, f_label, 1, .false., ifcvsl)
    call field_set_key_int(ivarfl(isca(ii)), kromsl, ifcvsl)
  endif
enddo

! For variances, the diffusivity is that of the associated scalar,
! and must not be initialized first.

do ii = 1, nscal
  if (iscavr(ii).gt.0) then
    f_id = ivarfl(isca(ii))
    call field_get_key_int(ivarfl(isca(iscavr(ii))), kromsl, ifcvsl)
    call field_is_key_set(f_id, kromsl, is_set)
    if (is_set.eqv..true.) then
      write(nfecra,7040) f_id, ivarfl(isca(iscavr(ii))), ifcvsl
    else
      call field_set_key_int(f_id, kromsl, ifcvsl)
    endif
  endif
enddo

! Boundary roughness
if (iwallf.ge.5) then
  call add_boundary_property_field_owner('boundary_roughness', 'Boundary Roughness', &
                                         iflid)
endif

!===============================================================================
! 3. Additional postprocessing fields
!===============================================================================

! Fields used to save postprocessing data

ityloc = 3 ! boundary faces

itycat = FIELD_INTENSIVE + FIELD_PROPERTY

! If postprocessing of boundary layer Nusselt required,
! create appropriate fields; check that a thermal variable is present first

if (iscalt.le.0) then
  ipstdv(ipstnu) = 0
endif

if (ipstdv(ipstnu).gt.0) then
  call field_find_or_create('tplus', itycat, ityloc, idim1, iflid)
  call field_find_or_create('tstar', itycat, ityloc, idim1, iflid)
endif

! In case of ALE or boundary efforts postprocessing, create appropriate field

if (iale.eq.1 .or. ipstdv(ipstfo).ne.0) then
  itycat = FIELD_EXTENSIVE + FIELD_POSTPROCESS
  call field_create('boundary_forces', itycat, ityloc, idim3, inoprv, &
                    iforbr)
endif

itycat = FIELD_INTENSIVE + FIELD_PROPERTY

! In case of condensation or y+ postprocessing, create appropriate field

if (icondb.ge.0.or.icondv.ge.0.or.ipstdv(ipstyp).ne.0) then
  call field_get_id_try('yplus', f_id) ! Test if pre-existing
  call field_find_or_create('yplus', itycat, ityloc, idim1, iyplbr)
  if (f_id .lt. 0) then                ! Set some properties if new
    call field_set_key_str(iyplbr, keylbl, 'Yplus')
    call field_set_key_int(iyplbr, keylog, 1)
  endif
  ! yplus postprocessed if required
  flag = POST_ON_LOCATION
  if (ipstdv(ipstyp).ne.0) call field_set_key_int(iyplbr, keyvis, flag)
endif

! Some mappings

call cs_field_pointer_map_boundary

! Cooling towers mappings
if (ippmod(iaeros).ge.0) then
  call cs_ctwr_field_pointer_map
endif

call field_get_id_try('boundary_temperature', itempb)

if (itempb .ge. 0) then
  call field_get_id_try('temperature', f_id)
  if (f_id .lt. 0) call field_get_id_try('t_gas', f_id)
  if (f_id .ge. 0) then
    call field_get_label(f_id, f_label)
    call field_set_key_str(itempb, keylbl, f_label)
  endif
endif

!---
! Formats
!---

#if defined(_CS_LANG_FR)

 7040 format(                                                     &
'@'                                                            ,/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@'                                                            ,/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES'               ,/,&
'@    ========='                                               ,/,&
'@'                                                            ,/,&
'@  Le champ ', i10, ' represente la variance'                 ,/,&
'@    des fluctuations du champ ', i10                         ,/,&
'@    d''apres la valeur du mot cle first_moment_id'           ,/,&
'@'                                                            ,/,&
'@  Le mot cle scalar_diffusivity_id'                          ,/,&
'@    ne doit pas etre renseigne.'                             ,/,&
'@  Il sera pris automatiquement egal a celui du scalaire'     ,/,&
'@    associe, soit ',i10                                      ,/,&
'@'                                                            ,/,&
'@  Le calcul ne sera pas execute.'                            ,/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@'                                                            ,/)

#else

 7040 format(                                                     &
'@'                                                            ,/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@'                                                            ,/,&
'@ @@ WARNING: STOP AT THE INITIAL DATA VERIFICATION'          ,/,&
'@    ======='                                                 ,/,&
'@'                                                            ,/,&
'@  The field ', i10, ' represents the variance'               ,/,&
'@    of fluctuations of the field ', i10                      ,/,&
'@    according to value of keyword first_moment_id'           ,/,&
'@'                                                            ,/,&
'@  The scalar_diffusivity_id keyword must not be set'         ,/,&
'@  It will be automatically set equal to that of the'         ,/,&
'@    associated scalar ',i10                                  ,/,&
'@'                                                            ,/,&
'@  The calculation cannot be executed.'                       ,/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@'                                                            ,/)

#endif

return
end subroutine addfld
