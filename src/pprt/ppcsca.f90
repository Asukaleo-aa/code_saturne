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

subroutine ppcsca
!================


!===============================================================================
!  FONCTION  :
!  ---------

!       CALCUL DE NSCAPP SUIVANT LA PHYSIQUE PARTICULIERE
!       DEMANDEE PAR l'UTILISATEUR

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
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
use dimens
use numvar
use optcal
use cstphy
use entsor
use cstnum
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use elincl
use cs_coal_incl
use cs_fuel_incl
use ppcpfu
use atincl

!===============================================================================

implicit none

! Local variables

integer          iok


!===============================================================================

!===============================================================================
! 1. MODELES DE COMBUSTION GAZ (ICO...)
!===============================================================================

! --> Flamme de diffusion

if ( ippmod(icod3p).eq.0 ) nscapp = 2
if ( ippmod(icod3p).eq.1 ) nscapp = 3

if ( ippmod(icodeq).eq.0 ) nscapp = 2
if ( ippmod(icodeq).eq.1 ) nscapp = 3

! --> Flamme de premelange (modele EBU)

if ( ippmod(icoebu).eq.0 ) nscapp = 1
if ( ippmod(icoebu).eq.1 ) nscapp = 2
if ( ippmod(icoebu).eq.2 ) nscapp = 2
if ( ippmod(icoebu).eq.3 ) nscapp = 3

! ---> Modele BML

if ( ippmod(icobml).eq.0 ) nscapp = 2
if ( ippmod(icobml).eq.1 ) nscapp = 3

! ---> Modele LWC

if ( ippmod(icolwc).eq.0 ) nscapp = 4
if ( ippmod(icolwc).eq.1 ) nscapp = 5
if ( ippmod(icolwc).eq.2 ) nscapp = 5
if ( ippmod(icolwc).eq.3 ) nscapp = 6
if ( ippmod(icolwc).eq.4 ) nscapp = 5
if ( ippmod(icolwc).eq.5 ) nscapp = 6

! ---nombre de Dirac pour le modele LWC

if ( ippmod(icolwc).eq.0 .or.                                     &
     ippmod(icolwc).eq.1           ) then
  ndirac = 2

else if ( ippmod(icolwc).eq.2 .or.                                &
          ippmod(icolwc).eq.3      ) then
  ndirac = 3

else if ( ippmod(icolwc).eq.4 .or.                                &
          ippmod(icolwc).eq.5      ) then
  ndirac = 4
endif

!===============================================================================
! 1.1 Soot model
!===============================================================================

if (isoot.ge.1) nscapp = nscapp + 2

!===============================================================================
! 2. MODELES DE COMBUSTION CHARBON PULVERISE (ICP...)
!===============================================================================

! --> Flamme charbon pulverise

! ------ Transport d'H2
if ( ippmod(icp3pl).eq.0 ) then

  nscapp = 1 + 2*ncharb + 2 + 4*nclacp + (noxyd-1)

  if ( ihtco2.eq. 1) nscapp = nscapp + 1
  if ( ieqco2.ge. 1) nscapp = nscapp + 1
  if ( ieqnox.ge. 1) nscapp = nscapp + 3

endif

! ------ Transport d'H2 + phase de sechage

if ( ippmod(icp3pl).eq.1 ) then

  nscapp = 1 + 2*ncharb + 3 +5*nclacp + (noxyd-1)

  if ( ihtco2.eq. 1) nscapp = nscapp + 1
  if ( ieqco2.eq. 1) nscapp = nscapp + 1
  if ( ieqnox.ge. 1) nscapp = nscapp + 3

endif
!
if ( ippmod(iccoal).ge.0 ) then
! enthalpie du melange
! phase disperse : (Np , Xch , Xck , h2  ) par classe
! phase gaz      : ( F1 , F2 ) par charbon
!                  F4 , F5 optionnel en fonction de Noxyd
!                  F7 , Variance
  nscapp = 1 + 4*nclacp + 2*ncharb + (noxyd-1) + 2

  if ( ippmod(iccoal) .eq. 1 ) then
!   humidite : f6
    nscapp = nscapp + nclacp + 1
  endif
! F8
  if ( ihtco2.eq. 1) nscapp = nscapp + 1
! F9
  if ( ihth2o.eq. 1) nscapp = nscapp + 1
! Y_CO ou Y_CO2
  if ( ieqco2.ge. 1) nscapp = nscapp + 1
! Y_HCN, Y_NO, Taire
  if ( ieqnox.ge. 1) nscapp = nscapp + 3
endif

! --> Flamme charbon pulverise couple Lagrangien

if ( ippmod(icpl3c).eq.0 ) then
  nscapp = 1 + 2*ncharb + 2
endif

!===============================================================================
! 3. MODELE COMPRESSIBLE SANS CHOC : rho, e, s
!===============================================================================

if ( ippmod(icompf).ge.0 ) nscapp = 3


!===============================================================================
! 4. MODELES ELECTRIQUES : Effet Joule        (IELJOU)
!                          Arc electrique     (IELARC)
!                          Conduction ionique (IELION)
!===============================================================================


! --> Effet Joule

  if ( ippmod(ieljou).eq.1 ) nscapp = 2 + ngazg-1
  if ( ippmod(ieljou).eq.2 ) nscapp = 3 + ngazg-1

  if ( ippmod(ieljou).eq.3 ) nscapp = 2 + ngazg-1
  if ( ippmod(ieljou).eq.4 ) nscapp = 3 + ngazg-1

! --> Arc electrique

  if ( ippmod(ielarc).eq.1 ) nscapp = 2 + ngazg-1
  if ( ippmod(ielarc).eq.2 ) nscapp = 5 + ngazg-1

! --> Conduction ionique

  if ( ippmod(ielion).eq.1 ) nscapp = 2 + ngazg-1

!===============================================================================
! 5. MODELES DE COMBUSTION FUEL (ICFUEL)
!===============================================================================

if ( ippmod(icfuel).ge.0 ) then
!
! enthalpie du melange
! phase gaz      : fvap , fhtf , Variance
!                  F4 , F5 optionnel en fonction de Noxyd
! phase disperse : ng , iyfol , ih2
!
  nscapp = 1 + 3  + (noxyd-1) + 3*nclafu
! Y_CO
  if ( ieqco2.ge. 1) nscapp = nscapp + 1
! Y_HCN, Y_NO, Hox
  if ( ieqnox.ge. 1) nscapp = nscapp + 3
!
endif

!===============================================================================
! 6. MODELE ATMOSPHERIQUE (IATMOS)
!===============================================================================

if ( ippmod(iatmos).eq.0 ) nscapp = 0
if ( ippmod(iatmos).eq.1 ) nscapp = 1
if ( ippmod(iatmos).eq.2 ) nscapp = 3

!===============================================================================
! 8. MODELISATION DES AEROREFRIGERANTS (IAEROS)
!===============================================================================

if ( ippmod(iaeros).ge.0 ) nscapp = 2

return
end subroutine
