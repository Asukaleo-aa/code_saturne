!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2013 EDF S.A., France

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

subroutine fldtri &
!================

 ( nproce ,                                                            &
   dt     , tpucou , rtpa   , rtp    , propce , propfa , propfb ,      &
   coefa  , coefb  )

!===============================================================================
! Purpose:
! --------

! Map fields

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nproce           ! i  ! <-- ! nombre de prop phy aux centres                 !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! tpucou(ncelet,3) ! ra ! <-- ! velocity-pressure coupling                     !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
!__________________.____._____.________________________________________________.

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens, only: ndimfb
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
use lagpar
use lagdim
use lagran
use ihmpre
use cplsat
use mesh
use field

!===============================================================================

implicit none

! Arguments

integer          nproce, nscal
double precision dt(ncelet), tpucou(ncelet,3), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision frcxt(ncelet,3)
double precision coefa(ndimfb,*), coefb(ndimfb,*)

! Local variables

integer          ii, ippu, ippv, ippw, ivar, iprop
integer          imom, idtnm
integer          iflid, nfld
integer          icondl, icondf
integer          f_id

integer          ifvar(nvppmx)

character*80     fname

!===============================================================================


!===============================================================================
! 1. Initialisation
!===============================================================================

nfld = 0

!===============================================================================
! 2. Mapping for post-processing
!===============================================================================

! Velocity and pressure
!----------------------

ivar = ipr
icondl = iclrtp(ivar, icoef)
icondf = iclrtp(ivar, icoeff)

call field_map_values(ivarfl(ivar), rtp(1,ivar), rtpa(1,ivar))
if (nfabor .gt. 0) then
  call field_map_bc_coeffs(ivarfl(ivar),                             &
                           coefa(1, icondl), coefb(1, icondl),       &
                           coefa(1, icondf), coefb(1, icondf))
else
  call field_allocate_bc_coeffs(ivarfl(ivar), .true., .false.)
endif

ivar = iu
call field_map_values(ivarfl(ivar), rtp(1,ivar), rtpa(1,ivar))

if (nfabor .gt. 0) then
  if (ivelco .eq. 0) then
    icondl = iclrtp(ivar, icoef)
    icondf = iclrtp(ivar, icoeff)
    call field_map_bc_coeffs(ivarfl(ivar),                           &
                             coefa(1, icondl), coefb(1, icondl),     &
                             coefa(1, icondf), coefb(1, icondf))
  else
    call field_map_bc_coeffs(ivarfl(ivar),                           &
                             coefau(1, 1), coefbu(1, 1, 1),          &
                             cofafu(1, 1), cofbfu(1, 1, 1))
  endif
else
  call field_allocate_bc_coeffs(ivarfl(ivar), .true., .false.)
endif

! Turbulence
!-----------

if (itytur.eq.2) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  nfld = nfld + 1
  ifvar(nfld) = iep
elseif (itytur.eq.3) then
  nfld = nfld + 1
  ifvar(nfld) = ir11
  nfld = nfld + 1
  ifvar(nfld) = ir22
  nfld = nfld + 1
  ifvar(nfld) = ir33
  nfld = nfld + 1
  ifvar(nfld) = ir12
  nfld = nfld + 1
  ifvar(nfld) = ir13
  nfld = nfld + 1
  ifvar(nfld) = ir23
  nfld = nfld + 1
  ifvar(nfld) = iep
elseif (itytur.eq.5) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  nfld = nfld + 1
  ifvar(nfld) = iep
  nfld = nfld + 1
  ifvar(nfld) = iphi
  if (iturb.eq.50) then
    nfld = nfld + 1
    ifvar(nfld) = ifb
  elseif (iturb.eq.51) then
    nfld = nfld + 1
    ifvar(nfld) = ial
  endif
elseif (iturb.eq.60) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  nfld = nfld + 1
  ifvar(nfld) = iomg
elseif (iturb.eq.70) then
  nfld = nfld + 1
  ifvar(nfld) = inusa
endif

! Map fields

do ii = 1, nfld
  ivar = ifvar(ii)
  icondl = iclrtp(ivar, icoef)
  icondf = iclrtp(ivar, icoeff)
  call field_map_values(ivarfl(ivar), rtp(1,ivar), rtpa(1,ivar))
  if (nfabor .gt. 0) then
    call field_map_bc_coeffs(ivarfl(ivar),                           &
                             coefa(1, icondl), coefb(1, icondl),     &
                             coefa(1, icondf), coefb(1, icondf))
  else
    call field_allocate_bc_coeffs(ivarfl(ivar), .true., .false.)
  endif
enddo

nfld = 0

! Mesh velocity
!--------------

if (iale.eq.1) then
  ivar = iuma
  call field_map_values(ivarfl(ivar), rtp(1,ivar), rtpa(1,ivar))
  if (nfabor .gt. 0) then
    if (ivelco .eq. 0) then
      icondl = iclrtp(ivar, icoef)
      icondf = iclrtp(ivar, icoeff)
      call field_map_bc_coeffs(ivarfl(ivar),                         &
                               coefa(1, icondl), coefb(1, icondl),   &
                               coefa(1, icondf), coefb(1, icondf))
    else
      call field_map_bc_coeffs(ivarfl(ivar),                         &
                               claale(1, 1), clbale(1, 1, 1),        &
                               cfaale(1, 1), cfbale(1, 1, 1))
    endif
  else
    call field_allocate_bc_coeffs(ivarfl(ivar), .true., .false.)
  endif
endif

! User variables
!---------------

nscal = nscaus + nscapp

do ii = 1, nscal
  if (isca(ii) .gt. 0) then
    ivar = isca(ii)
    icondl = iclrtp(ivar, icoef)
    icondf = iclrtp(ivar, icoeff)
    call field_map_values(ivarfl(ivar), rtp(1,ivar), rtpa(1,ivar))
    if (nfabor .gt. 0) then
      call field_map_bc_coeffs(ivarfl(ivar),                         &
                               coefa(1, icondl), coefb(1, icondl),   &
                               coefa(1, icondf), coefb(1, icondf))

      ! Boundary conditions of the turbulent fluxes T'u'
      if (ityturt(ii).eq.3) then
        call field_get_name(ivarfl(ivar), fname)
        ! Index of the corresponding turbulent flux
        call field_get_id(trim(fname)//'_turbulent_flux', f_id)
        call field_allocate_bc_coeffs(f_id, .true., .true.)
        call field_init_bc_coeffs(f_id, .true., .true.)
      endif
    else
      call field_allocate_bc_coeffs(ivarfl(ivar), .true., .false.)
    endif
  endif
enddo

! The choice made in VARPOS specifies that we will only be interested in
! properties at cell centers (no mass flux, nor density at the boundary).

do iprop = 1, nproce
  call field_map_values(iprpfl(iprop), propce(1, iprop), propce(1, iprop))
enddo

! Reserved fields whose ids are not saved (may be queried by name)
!-----------------------------------------------------------------

! Local time step

call field_get_id('dt', iflid)
call field_map_values(iflid, dt, dt)

! Transient velocity/pressure coupling

if (ipucou.ne.0) then
  call field_get_id('tpucou', iflid)
  call field_map_values(iflid, tpucou, tpucou)
endif

return
end subroutine
