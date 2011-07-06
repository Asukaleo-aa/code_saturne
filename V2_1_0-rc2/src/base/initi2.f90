!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2009 EDF S.A., France

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

subroutine initi2
!================

!===============================================================================
! FONCTION :
! ----------

! FIN INITIALISATION DES COMMONS

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
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
use optcal
use entsor
use cstphy
use mesh

!===============================================================================

implicit none

! Arguments

integer          jcelbr

! Local variables


!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================


!===============================================================================
! 3. TABLEAUX DE cstphy.f90
!===============================================================================

!---> COMMON TURBUL

! --- SI ALMAX < 0 , IL EST RECALCULE

write(nfecra,1000)

if (almax.le.0.d0) then
  almax = voltot**.333d0
  write(nfecra,1100) almax
  write(nfecra,1102)
  if(itytur.eq.2.or.itytur.eq.3                   &
       .or. itytur.eq.5 .or. iturb.eq.60          &
       .or. iturb.eq.70) then
    write(nfecra,1101)
  endif
endif

#if defined(_CS_LANG_FR)

 1000 format(                                                           &
'                                                             '  )
 1100 format(                                                           &
'       ALMAX  = ', E14.5,    ' (Longueur caracteristique    )'  )
 1101 format(                                                           &
'       ALMAX est la longueur utilisee pour initialiser       ',/,&
'                                               la turbulence.'  )
 1102 format(                                                           &
'       ALMAX est la racine cubique du volume du domaine.     ',/)

#else

 1000 format(                                                           &
'                                                             '  )
 1100 format(                                                           &
'       ALMAX  = ', E14.5,    ' (Characteristic length       )'  )
 1101 format(                                                           &
'       ALMAX is the length used to initialize the turbulence.'  )
 1102 format(                                                           &
'       ALMAX is the cubic root of the domain volume.'         ,/)

#endif

!===============================================================================
! 4. FIN
!===============================================================================

return
end subroutine
