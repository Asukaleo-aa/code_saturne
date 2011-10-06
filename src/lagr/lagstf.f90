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

subroutine lagstf &
!================

  ( ncelet , nfabor , nvisbr ,                                    &
    ivar   ,                                                      &
    gmin   , gmax   , gmoy   ,                                    &
    parbor , unsnbr )

!===============================================================================
! FONCTION :
! ----------

!   SOUS-PROGRAMME DU MODULE LAGRANGIEN :
!   -------------------------------------

!     CALCUL DES VALEURS MIN MAX ET MOYENNE POUR LES
!     STATISTIQUES AUX FRONTIERES.

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! ncelet           ! i  ! <-- ! number of extended (real + ghost) cells        !
! nfabor           ! i  ! <-- ! number of boundary faces                       !
! nvisbr           ! e  ! <-- ! nombre de statistiques aux frontieres          !
! ivar             ! e  ! <-- ! numero de la variable a integrer               !
! gmin...gmoy      ! e  ! --> ! variations min max et moyenne                  !
! parbor           ! tr ! <-- ! infos sur interaction des particules           !
!(nfabor,nvisbr    !    !     !   aux faces de bord                            !
! unsnbr(nfabor    ! e  ! <-- ! inverse du nombre interaction                  !
!                  !    !     !   frontiere pour moyenne                       !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use cstnum
use entsor
use lagpar
use lagran

!===============================================================================

implicit none

! Arguments

integer          ncelet , nfabor , nvisbr
integer          ivar
double precision gmax   , gmin   , gmoy
double precision parbor(nfabor, nvisbr), unsnbr(nfabor)

!  VARIABLES LOCALES

integer          nbrfac , ifac

!==============================================================================

nbrfac = 0

gmax = -grand
gmin =  grand
gmoy =  0.d0

if (imoybr(ivar).eq.2) then

  do ifac = 1,nfabor
    if (parbor(ifac,inbr).gt.seuilf) then
      nbrfac = nbrfac + 1
      gmax = max (gmax, parbor(ifac,ivar)*unsnbr(ifac))
      gmin = min (gmin, parbor(ifac,ivar)*unsnbr(ifac))
      gmoy = gmoy + (parbor(ifac,ivar)*unsnbr(ifac))
    endif
  enddo

else if (imoybr(ivar).eq.1) then

  do ifac = 1,nfabor
    if (parbor(ifac,inbr).gt.seuilf) then
      nbrfac = nbrfac + 1
      gmax = max (gmax, parbor(ifac,ivar)/tstatp)
      gmin = min (gmin, parbor(ifac,ivar)/tstatp)
      gmoy = gmoy + (parbor(ifac,ivar)/tstatp)
    endif
  enddo

else if (imoybr(ivar).eq.0) then

  do ifac = 1,nfabor
    if (parbor(ifac,inbr).gt.seuilf) then
      nbrfac = nbrfac + 1
      gmax = max (gmax, parbor(ifac,ivar))
      gmin = min (gmin, parbor(ifac,ivar))
      gmoy = gmoy + parbor(ifac,ivar)
    endif
  enddo

endif

if (nbrfac.gt.0) then
  gmoy = gmoy /dble(nbrfac)
else
  gmax =  0.d0
  gmin =  0.d0
  gmoy =  0.d0
endif

!-------
! FORMAT
!-------


!----
! FIN
!----

return

end subroutine
