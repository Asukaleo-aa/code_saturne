!-------------------------------------------------------------------------------

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

subroutine intprf &
!================

 ( nprofz , nproft ,                                   &
   profz  , proft  , profv  , xz     , temps  , var    )

!===============================================================================
!  FONCTION  :
!  ---------

!      INTERPOLATION VERTICALE-TEMPORELLE OPTIMISEE

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! vi               ! r  ! --> ! resultat de l'interpolation                    !
! zi               ! r  ! <-- ! altitude a laquelle on interpole               !
! ti               ! r  ! <-- ! temps physique auquel on interpole             !
! aprom            ! tr ! <-- ! variables des profils meteo a                  !
!                  !    !     !  interpoler                                    !
! tmprom           ! tr ! <-- ! instants des profils meteo a                   !
!                  !    !     !  interpoler                                    !
! zdprom           ! e  ! <-- ! altitudes des profils meteo a                  !
!                  !    !     !  interpoler                                    !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

!===============================================================================

implicit none

! Arguments

integer          nprofz, nprofv, nproft
double precision profz(nprofz), proft(nproft)
double precision profv(nprofz,nproft)
double precision xz, temps, var

! Local variables

integer          it, it1, it2
integer          iz, iz1, iz2
double precision alphaz, alphat, var1, var2

!===============================================================================

!===============================================================================
! 1. INTERPOLATION EN TEMPS
!===============================================================================

if (temps.le.proft(1)) then
  it1 = 1
  it2 = 1
  alphat = 1.d0
else if (temps.ge.proft(nproft)) then
  it1 = nproft
  it2 = nproft
  alphat = 1.d0
!     sinon on a forcement NPROFT>1
else
  it = 1
 102    continue
  if (temps.gt.proft(it+1)) then
    it = it + 1
    goto 102
  else
    it1 = it
    it2 = it+1
    alphat = (proft(it2)-temps)/(proft(it2)-proft(it1))
  endif
endif

!===============================================================================
! 2. INTERPOLATION VERTICALE EN ESPACE
!===============================================================================

if (xz.le.profz(1)) then
  iz1 = 1
  iz2 = 1
  alphaz = 1.d0
else if (xz.ge.profz(nprofz)) then
  iz1 = nprofz
  iz2 = nprofz
  alphaz = 1.d0
!     sinon on a forcement NPROFZ>1
else
  iz = 1
 103    continue
  if (xz.gt.profz(iz+1)) then
    iz = iz + 1
    goto 103
  else
    iz1 = iz
    iz2 = iz+1
    alphaz = (profz(iz2)-xz)/(profz(iz2)-profz(iz1))
  endif
endif

!===============================================================================
! 3. INTERPOLATION
!===============================================================================

var1 = alphaz*profv(iz1,it1) + (1.d0-alphaz)*profv(iz2,it1)
var2 = alphaz*profv(iz1,it2) + (1.d0-alphaz)*profv(iz2,it2)

var = alphat*var1 + (1.d0-alphat)*var2

end subroutine




