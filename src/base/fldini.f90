!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2012 EDF S.A., France

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

subroutine fldini
!================

!===============================================================================
! Purpose:
! --------

! Define main fields

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
!__________________.____._____.________________________________________________.

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

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

! Local variables

integer          ii, ippu, ippv, ippw, ivar, iprop
integer          imom, idtnm
integer          keyvis, keylbl, keycpl, iflid, ikeyid, ikeyvl, iopchr
integer          nfld, iinten, iexten, itycat, ityloc, idim1, idim3, ilved
integer          iprev, inoprv
integer          ifvar(nvppmx), iapro(npromx)

character*80     name
character*32     name1, name2, name3
character*80     fname(nvppmx)

!===============================================================================


!===============================================================================
! 1. Initialisation
!===============================================================================

iinten = 0 ! most variables are intensive, not extensive
iexten = 1 ! most variables are intensive, not extensive
itycat = 4 ! for variables
ityloc = 1 ! variables defined on cells
idim1  = 1
idim3  = 3
ilved  = 0   ! not interleaved by default
iprev = 1    ! variables have previous value
inoprv = 0   ! variables have no previous value

name = 'post_vis'
call fldkid(name, keyvis)
!==========

name = 'label'
call fldkid(name, keylbl)
!==========

name = 'coupled'
call fldkid(name, keycpl)
!==========

! Postprocessing level for variables

iopchr = 1
if (mod(ipstdv, ipstcl).eq.0) then
  iopchr = 1 + 2
endif

!===============================================================================
! 2. Mapping for post-processing
!===============================================================================

! Velocity and pressure
!----------------------

ivar = ipr
name = 'pressure'
call flddef(name, iinten, itycat, ityloc, idim1, ilved, iprev, ivarfl(ivar))
!==========
call fldsks(ivarfl(ivar), keylbl, nomvar(ipprtp(ivar)))
!==========
if (ichrvr(ipprtp(ivar)) .eq. 1) then
  call fldski(ivarfl(ivar), keyvis, iopchr)
  !==========
endif

ivar = iu
name = 'velocity'
call flddef(name, iinten, itycat, ityloc, idim3, ilved, iprev, ivarfl(iu))
!==========
! Change label for velocity to remove trailing coordinate name
name = nomvar(ipprtp(iu))
name1 = name(1:32)
name = nomvar(ipprtp(iv))
name2 = name(1:32)
name = nomvar(ipprtp(iw))
name3 = name(1:32)
call fldsnv (name1, name2, name3)
!==========
call fldsks(ivarfl(ivar), keylbl, name1)
!==========
if (ichrvr(ipprtp(ivar)) .eq. 1) then
  call fldski(ivarfl(ivar), keyvis, iopchr)
  !==========
endif
if (ivelco .eq. 1) then
  call fldski(ivarfl(ivar), keycpl, 1)
  !==========
endif

! All components point to same field
ivarfl(iv) = ivarfl(iu)
ivarfl(iw) = ivarfl(iu)

! Turbulence
!-----------

nfld = 0

if (itytur.eq.2) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  fname(nfld) = 'k'
  nfld = nfld + 1
  ifvar(nfld) = iep
  fname(nfld) = 'epsilon'
elseif (itytur.eq.3) then
  nfld = nfld + 1
  ifvar(nfld) = ir11
  fname(nfld) = 'r11'
  nfld = nfld + 1
  ifvar(nfld) = ir22
  fname(nfld) = 'r22'
  nfld = nfld + 1
  ifvar(nfld) = ir33
  fname(nfld) = 'r33'
  nfld = nfld + 1
  ifvar(nfld) = ir12
  fname(nfld) = 'r12'
  nfld = nfld + 1
  ifvar(nfld) = ir13
  fname(nfld) = 'r13'
  nfld = nfld + 1
  ifvar(nfld) = ir23
  fname(nfld) = 'r23'
  nfld = nfld + 1
  ifvar(nfld) = iep
  fname(nfld) = 'epsilon'
elseif (itytur.eq.5) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  fname(nfld) = 'k'
  nfld = nfld + 1
  ifvar(nfld) = iep
  fname(nfld) = 'epsilon'
  nfld = nfld + 1
  ifvar(nfld) = iphi
  fname(nfld) = 'phi'
  if (iturb.eq.50) then
    nfld = nfld + 1
    ifvar(nfld) = ifb
    fname(nfld) = 'f_bar'
  elseif (iturb.eq.51) then
    nfld = nfld + 1
    ifvar(nfld) = ial
    fname(nfld) = 'alpha'
  endif
elseif (iturb.eq.60) then
  nfld = nfld + 1
  ifvar(nfld) = ik
  fname(nfld) = 'k'
  nfld = nfld + 1
  ifvar(nfld) = iomg
  fname(nfld) = 'omega'
elseif (iturb.eq.70) then
  nfld = nfld + 1
  ifvar(nfld) = inusa
  fname(nfld) = 'nu_tilda'
endif

! Map fields

do ii = 1, nfld
  ivar = ifvar(ii)
  name = nomvar(ipprtp(ivar))
  call flddef(name, iinten, itycat, ityloc, idim1, ilved, iprev, ivarfl(ivar))
  !==========
  call fldsks(ivarfl(ivar), keylbl, nomvar(ipprtp(ivar)))
  !==========
  if (ichrvr(ipprtp(ivar)) .eq. 1) then
    call fldski(ivarfl(ivar), keyvis, iopchr)
    !==========
  endif
enddo

nfld = 0

! Mesh velocity
!--------------

if (iale.eq.1) then
  ivar = iuma
  name = 'mesh_velocity'
  call flddef(name, iinten, itycat, ityloc, idim3, ilved, iprev, ivarfl(ivar))
  !==========
  call fldsks(ivarfl(ivar), keylbl, nomvar(ipprtp(ivar)))
  !==========
  if (ichrvr(ipprtp(ivar)) .eq. 1) then
    call fldski(ivarfl(ivar), keyvis, iopchr)
    !==========
  endif
  if (ivelco .eq. 1) then
    call fldski(ivarfl(ivar), keycpl, 1)
    !==========
  endif
  ivarfl(ivma) = ivarfl(iuma)
  ivarfl(iwma) = ivarfl(iwma)
endif

! User variables
!---------------

do ii = 1, nscal

  if (isca(ii) .gt. 0) then
    ivar = isca(ii)
    if (ii .eq. iscalt) then
      if (iscsth(iscalt) .eq. 2) then
        name = 'enthalpy'
      else
        name = 'temperature'
      endif
    else
      name = nomvar(ipprtp(ivar))
    endif
    call flddef(name, iinten, itycat, ityloc, idim1, ilved, iprev, ivarfl(ivar))
    !==========
    call fldsks(ivarfl(ivar), keylbl, nomvar(ipprtp(ivar)))
    !==========
    if (ichrvr(ipprtp(ivar)) .eq. 1) then
      call fldski(ivarfl(ivar), keyvis, iopchr)
      !==========
    endif
  endif

enddo

! Flag moments

do ii = 1, npromx
  iapro(ii ) = 0
enddo

! For moments, this key defined the division by time mode
!  = 0: no division
!  > 0: field id for cumulative dt (property)
!  < 0: -id in dtcmom of cumulative dt (uniform)

do imom = 1, nbmomt
  ! property id matching moment
  iprop = ipproc(icmome(imom))
  ! dt type and number
  idtnm = idtmom(imom)
  if (idtnm.gt.0) then
    icdtmo(idtnm) = 1
  elseif(idtnm.lt.0) then
    iapro(iprop) = 1
  endif
enddo

! The choice made in VARPOS specifies that we will only be interested in
! properties at cell centers (no mass flux, nor density at the boundary).

do iprop = 1, nproce
  name = nomvar(ipppro(iprop))
  if (name(1:4) .eq. '    ') then
    write(name, '(a, i3.3)') 'property_', iprop
  endif
  if (iapro(iprop).eq.0) then
    itycat = 8
  else
    itycat = 8 + 16
  endif
  call flddef(name, iinten, itycat, ityloc, idim1, ilved, inoprv, iprpfl(iprop))
  !==========
  call fldsks(iprpfl(iprop), keylbl, name)
  !==========
  if (ichrvr(ipppro(iprop)) .eq. 1) then
    call fldski(iprpfl(iprop), keyvis, ichrvr(ipppro(iprop)))
    !==========
  endif
enddo

! Add moment accumulators metadata
!---------------------------------

name = 'moment_dt'
call fldkid(name, ikeyid)
!==========

do imom = 1, nbmomt
  ! property id matching moment
  iprop = ipproc(icmome(imom))
  ! dt type and number
  idtnm = idtmom(imom)
  ikeyvl = -1
  if(idtnm.gt.0) then
    ikeyvl = iprpfl((icdtmo(idtnm)))
  elseif(idtnm.lt.0) then
    ikeyvl = idtnm - 1
  endif
  call fldski(iprpfl(iprop), ikeyid, ikeyvl)
  !==========
enddo

! Reserved fields whose ids are not saved (may be queried by name)
!-----------------------------------------------------------------

itycat = 0

! Local time step

name = 'dt'
call flddef(name, iexten, itycat, ityloc, idim1, ilved, inoprv, iflid)
!==========

! Transient velocity/pressure coupling

if (ipucou.ne.0) then
  name = 'tpucou'
  call flddef(name, iexten, itycat, ityloc, idim3, ilved, inoprv, iflid)
  !==========
endif

return

end subroutine
