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

subroutine cfvarp
!================


!===============================================================================
!  FONCTION  :
!  ---------

!              INIT DES POSITIONS DES VARIABLES
!            POUR LE COMPRESSIBLE SANS CHOC SELON
! REMPLISSAGE DES PARAMETRES (DEJA DEFINIS) POUR LES SCALAIRES PP

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
use ppincl

!===============================================================================

implicit none

! Local variables

integer          ii, iprop, iok, iccfth, imodif
double precision dblpre(1)

!===============================================================================
!===============================================================================
! 1. DEFINITION DES POINTEURS
!===============================================================================


if ( ippmod(icompf).ge.0 ) then

  iprop =0

! ---- Masse volumique
  iprop = iprop + 1
  irho = iscapp(iprop)
!     Alias pour les C.L.
  irun = irho

! ---- Energie totale
  iprop = iprop + 1
  ienerg = iscapp(iprop)
!     Alias pour les C.L.
  irunh = ienerg

! ---- Temperature (post)
  iprop = iprop + 1
  itempk = iscapp(iprop)

! ---- Viscosite dynamique de reference relative au scalaire IRHO
  ivisls(irho  ) = 0
  visls0(irho  ) = epzero

! ---- Viscosite dynamique de reference relative au scalaire ITEMPK
  ivisls(itempk) = 0
  visls0(itempk) = epzero

! ---- Initialisation par defaut de la viscosite en volume (cste)
  iviscv = 0
  viscv0 = 0.d0


!===============================================================================
! 2. OPTIONS DE CALCUL
!===============================================================================

! --> Cv constant ou variable (par defaut : constant)
  icv = 0
  cv0 = 0.d0

  iccfth = -1
  imodif = 0
  ii     = 1
  dblpre(1) = 0.d0
  call uscfth                                                   &
  !==========
 ( ii , ii ,                                                      &
   iccfth , imodif  ,                                             &
   dblpre , dblpre , dblpre , dblpre , dblpre , dblpre ,          &
   dblpre , dblpre ,                                              &
   dblpre , dblpre , dblpre , dblpre )

! --> Utilisation d'un flux de masse specifique pour la vitesse

!     ATTENTION   PAS ENCORE IMPLEMENTE
!========   LAISSER IFLMAU = 0

  iflmau = 0

!===============================================================================
! 3. ON REDONNE LA MAIN A L'UTILISATEUR
!===============================================================================

  call uscfx2
  !==========


!===============================================================================
! 4. TRAITEMENT ET VERIFICATION DES DONNEES FOURNIES PAR L'UTILISATEUR
!===============================================================================

! ---- Viscosite dynamique de reference relative au scalaire IENERG
  if(ivisls(itempk).gt.0 .or. icv.gt.0) then
    ivisls(ienerg) = 1
  else
    ivisls(ienerg) = 0
  endif

  visls0(ienerg) = epzero

  iok = 0

  if(visls0(itempk).le.0.d0) then
    write(nfecra,1000) visls0(itempk)
    iok = 1
  endif

  if(viscv0.lt.0.d0) then
    write(nfecra,2000) viscv0
    iok = 1
  endif

  if(iok.gt.0) call csexit (1)

endif

!--------
! FORMATS
!--------

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========   MODULE COMPRESSIBLE                         ',/,&
'@                                                            ',/,&
'@    LA CONDUCTIVITE THERMIQUE DOIT ETRE                     ',/,&
'@    UN REEL POSITIF STRICTEMENT                             ',/,&
'@    ELLE A POUR VALEUR ',E12.4                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier uscfx2.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 2000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========   MODULE COMPRESSIBLE                         ',/,&
'@                                                            ',/,&
'@    LA VISCOSITE EN VOLUME DOIT ETRE                        ',/,&
'@    UN REEL POSITIF                                         ',/,&
'@    ELLE A POUR VALEUR ',E12.4                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier uscfx2.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

return
end subroutine

