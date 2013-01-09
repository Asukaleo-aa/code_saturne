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

subroutine clpalp &
!================

 ( ncelet , ncel   , nvar   ,                                     &
   propce , rtpa   , rtp    )

!===============================================================================
! FONCTION :
! ----------

! Clipping of Alpha

!-------------------------------------------------------------------------------
! Arguments
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
!    nom           !type!mode !                   role                         !
!__________________!____!_____!________________________________________________!
! ncelet           ! e  ! <-- ! nombre d'elements halo compris                 !
! ncel             ! e  ! <-- ! nombre de cellules                             !
! nvar             ! e  ! <-- ! nombre de variables                            !
! iclip            ! e  ! <-- ! indicateur = 1 on n'utilise pas rtpa           !
!                  !    !     !  (inivar)                                      !
!                  !    !     !            sinon on peut (turrij)              !
! propce           ! tr ! <-- ! tableaux des variables au pdt courant          !
!(ncelet,*         !    !     !                                                !
! rtpa             ! tr ! <-- ! tableaux des variables au pdt precedt          !
! (ncelet,nvar)    !    !     !                                                !
! rtp              ! tr ! <-- ! tableaux des variables au pdt courant          !
! (ncelet,nvar)    !    !     !                                                !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail

!-------------------------------------------------------------------------------
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use entsor
use numvar
use cstnum
use parall

!===============================================================================

implicit none

! Arguments

integer          nvar, ncelet, ncel, nphas
integer          iphas, iclip
double precision propce(ncelet,*)
double precision rtpa(ncelet,nvar)
double precision rtp(ncelet,nvar)

! VARIABLES LOCALES

integer          icleps, iel, ivar, ivar1, ivar2, isou, ipp
integer          iclalp
double precision vmin, vmax, var

!===============================================================================

!===============================================================================
!  ---> Stockage Min et Max pour listing
!===============================================================================

ivar = ial
ipp = ipprtp(ivar)

vmin =  grand
vmax = -grand
do iel = 1, ncel
  var = rtp(iel,ivar)
  vmin = min(vmin,var)
  vmax = max(vmax,var)
enddo
if (irangp.ge.0) then
  call parmin(vmin)
  !==========
  call parmax(vmax)
  !==========
endif
varmna(ipp) = vmin
varmxa(ipp) = vmax


! ---> Clipping (modif pour eviter les valeurs exactement nulles)


iclalp = 0
do iel = 1, ncel
  if (rtp(iel,ial).lt.0.d0) then
    iclalp = iclalp + 1
    rtp(iel,ial) = 0.d0
  elseif(rtp(iel,ial).gt.1.d0) then
    iclalp = iclalp + 1
    rtp(iel,ial) = 1.d0
  endif
enddo


! ---> Stockage nb de clippings pour listing

if (irangp.ge.0) then
  call parcpt (iclalp)
  !==========
endif

iclpmn(ipprtp(ial)) = iclalp

return

end
