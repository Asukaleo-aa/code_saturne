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

subroutine cpvosy &
!================

 ( nvar   , nscal  , isvtf  ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   hbord  , theipb )

!===============================================================================
! Purpose:
! --------

! Exchange data relative to a volume coupling with SYRTHES
! Compute a volume exchange coefficient for each cell implied in the coupling
! Compute the source term (implicit and/or explicit part)

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! isvtf            ! i  ! <-- ! indicateur de scalaire pour la temp. fluide    !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! hbord(nfabor)    ! ra ! <-- ! coefficients d'echange aux bords               !
! theipb(nfabor)   ! ra ! <-- ! temperatures aux bords                         !
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use entsor
use cstphy
use mesh
use optcal

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          isvtf

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*),propfa(nfac,*),propfb(nfabor,*)
double precision hbord(nfabor),theipb(nfabor)

! Local variables

integer          nbccou, inbcou, inbcoo, ncecpl, iloc, iel
integer          mode, isvol, ivart

integer, dimension(:), allocatable :: lcecpl
double precision, dimension(:), allocatable :: tfluid, hvol

!===============================================================================

!===============================================================================
! SYRTHES volume coupling
!===============================================================================

! Get number of coupling cases

call nbcsyr(nbccou)
!==========

!---> Loop on couplings

do inbcou = 1, nbccou

  inbcoo = inbcou

  ! Test if this coupling is a volume coupling
  ! This is a volume coupling if isvol = 1

  call tvolsy(inbcoo, isvol)
  !==========

  if (isvol.eq.1) then

    ! Sanity check : only temperature is possible when doing a
    ! volume coupling with SYRTHES

    if (iscalt.ne.isvtf) then
       write(nfecra, 1000)
       call csexit(1)
    endif

    if (iscalt.eq.isvtf) then
      if (abs(iscsth(iscalt)).ne.1) then
        write(nfecra, 1000)
        call csexit(1)
      endif
    endif

    mode = 1 ! Volume coupling
    ivart = isca(iscalt)

    ! Number of cells per coupling case

    call nbesyr(inbcoo, mode, ncecpl)
    !==========

    ! Memory management to build arrays
    allocate(lcecpl(ncecpl))
    allocate(tfluid(ncecpl))
    allocate(hvol(ncecpl))

    ! Get list of cells implied in this coupling

    inbcoo = inbcou
    call leltsy(inbcoo, mode, lcecpl)
    !==========

    ! Receive solid temperature. Temporary storage in tfluid.
    ! This temperature is stored in a C structure for a future
    ! use in source term definition

    inbcoo = inbcou
    call varsyi(inbcoo, mode, tfluid)
    !==========

    ! Loop on coupled cells to initialize arrays

    do iloc = 1, ncecpl

      iel = lcecpl(iloc)
      tfluid(iloc) = rtp(iel, ivart)
      hvol(iloc) = 0.0d0

    enddo

    call usvosy &
    !==========
  ( nvar   , nscal , inbcoo , ncecpl , iscalt ,             &
    dt     , rtp   , rtpa   , propce , propfa , propfb ,    &
    lcecpl , hvol  )

    ! Send fluid temperature and exchange coefficient

    inbcoo = inbcou
    call varsyo(inbcoo, mode, lcecpl, tfluid, hvol)
    !==========

    ! Free memory
    deallocate(hvol)
    deallocate(tfluid)
    deallocate(lcecpl)

  endif ! This coupling is a surface coupling

enddo ! Loop on all syrthes couplings

!===============================================================================
! End of boundary couplings
!===============================================================================

return

! Formats

#if defined(_CS_LANG_FR)

 1000 format(                                                     &
'@                                                            ',/,&
'@ @@ ATTENTION : COUPLAGE VOLUMIQUE SYRTHES AVEC UN SCALAIRE ',/,&
'@      QUI EST DIFFERENT DE LA TEMPERATURE                   ',/,&
'@    =========                                               ',/,&
'@      OPTION NON VALIDE                                     ',/,&
'@                                                            ')

#else

 1000 format(                                                     &
'@                                                            ',/,&
'@ @@ WARNING: SYRTHES VOLUME COUPLING WITH A SCALAR          ',/,&
'@       DIFFERENT FROM TEMPERATURE                           ',/,&
'@    ========                                                ',/,&
'@      OPTION NOT POSSIBLE                                   ',/,&
'@                                                            ')

#endif

end subroutine
