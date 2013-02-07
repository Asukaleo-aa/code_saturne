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

subroutine cfini1
!================


!===============================================================================
!  FONCTION  :
!  ---------

!         INIT DES OPTIONS DES VARIABLES POUR
!              LE COMPRESSIBLE SANS CHOC
!   EN COMPLEMENT DE CE QUI A DEJA ETE FAIT DANS cs_user_parameters.f90

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
!bug Ba
use ihmpre

!===============================================================================

implicit none

! Local variables

integer          ipp , ii
integer          iok

!===============================================================================
!===============================================================================
! 0. VERIFICATION ISCALT, ISCSTH
!===============================================================================
!     L'utilisateur ne doit pas y avoir touche.

if(iscalt.ne.-1) then
  write(nfecra,1000)iscalt
  call csexit (1)
  !==========
endif
do ii = 1, nscapp
  if(iscsth(iscapp(ii)).ne.-10) then
    write(nfecra,1001)ii,iscapp(ii),iscapp(ii),iscsth(iscapp(ii))
    call csexit (1)
    !==========
  endif
enddo
!===============================================================================
! 1. VARIABLES TRANSPORTEES
!===============================================================================

! 1.1 Definition des scamin et des scamax des variables transportees
! ==================================================================


if(  (abs(scamin(irho  )+grand).gt.epzero).or.           &
     (abs(scamin(ienerg)+grand).gt.epzero).or.           &
     (abs(scamin(itempk)+grand).gt.epzero).or.           &
     (abs(scamax(irho  )-grand).gt.epzero).or.           &
     (abs(scamax(ienerg)-grand).gt.epzero).or.           &
     (abs(scamax(itempk)-grand).gt.epzero) ) then
  write(nfecra,2000)                                            &
       scamin(irho  ),scamax(irho  ),             &
       scamin(ienerg),scamax(ienerg),             &
       scamin(itempk),scamax(itempk)
  call csexit (1)
endif
!        SCAMIN(IRHO  )   = -GRAND
!        SCAMAX(IRHO  )   =  GRAND
!        SCAMIN(IENERG)   = -GRAND
!        SCAMAX(IENERG)   =  GRAND
!        SCAMIN(ITEMPK)   = -GRAND
!        SCAMAX(ITEMPK)   =  GRAND

! 1.2 Nature des scalaires transportes
! ====================================

! ---- Type de scalaire (0 passif, 1 temperature en K
!                                 -1 temperature en C
!                                  2 enthalpie en J
!                                  3 energie totale en J)
!      La distinction -1/1 sert pour le rayonnement

iscsth(irho  ) = 0
iscsth(ienerg) = 3
iscsth(itempk) = 0

iscalt = ienerg

!         - Schema convectif % schema 2ieme ordre
!           = 0 : upwind
!           = 1 : second ordre
do ii = 1, nvarmx
  blencv(ii) = 0.d0
enddo

!         Upwind necessaire pour le schema utilise

! --- Segregated or coupled solver for the velocity components:
!       only the segregated one is possible with the compressible module.
!       ivelco is imposed to 0.

if (ivelco.ne.0) then
  write(nfecra,6000) ivelco
  ivelco = 0
endif

! 1.3 Variable courante : nom, sortie chrono, suivi listing, sortie hist
! ======================================================================

!     Comme pour les autres variables,
!       si l'on n'affecte pas les tableaux suivants,
!       les valeurs par defaut seront utilisees

!     NOMVAR( ) = nom de la variable
!     ICHRVR( ) = sortie chono (oui 1/non 0)
!     ILISVR( ) = suivi listing (oui 1/non 0)
!     IHISVR( ) = sortie historique (nombre de sondes et numeros)
!     si IHISVR(.,1)  = -1 sortie sur toutes les sondes

!     NB : Les 8 premiers caracteres du noms seront repris dans le
!          listing 'developpeur'

! ======================================================================

ipp = ipprtp(isca(irho  ))
nomvar(ipp)  = 'Rho'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

ipp = ipprtp(isca(ienerg))
nomvar(ipp)  = 'EnergieT'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

ipp = ipprtp(isca(itempk))
nomvar(ipp)  = 'Temp K'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

!===============================================================================
! 2. PARAMETRES GLOBAUX
!===============================================================================

! --- Couplage vitesse/pression (0 : algorithme classique,
!                                1 : couplage instationnaire)
!     Uniquement en monophasique et en incompressible

if( ipucou.ne.0 ) then
  write(nfecra,3000) ipucou
  call csexit (1)
endif


! --- Estimateurs pour Navier-Stokes

!     Interdits en compressible

if( (iescal(iespre).ne.0) .or.                            &
     (iescal(iesder).ne.0) .or.                            &
     (iescal(iescor).ne.0) .or.                            &
     (iescal(iestot).ne.0) ) then
  write(nfecra,4000)
  call csexit (1)
endif

!===============================================================================
! 3. OPTIONS DE CALCUL PAR DEFAUT
!===============================================================================

! --> Conditions aux limites prenant en compte l'equilibre hydrostatique
!     (oui = 1 , non = 0)

icfgrp = 1


! ---> Masse volumique variable (pour les suites)
irovar = 1

!===============================================================================
! 4. ON REDONNE LA MAIN A L'UTLISATEUR
!===============================================================================

if (iihmpr.eq.1) then
  call cscfgp(icfgrp)
endif

call uscfx2
!==========

!===============================================================================
! 5. OPTIONS DE CALCUL OBLIGATOIRES
!     qui pourront etre remontees au dessus de uscfx1
!     selon les developpements
!===============================================================================

!     Pour chaque phase

idiff(isca(irho)) = 1

! --> Implicitation du terme de convection de l'equation de masse
!     (oui = 1 , non = 0)
!     On choisit 0 ; c'est la seule option qui a ete testee. Elle
!       facilite le codage pour le respect du flux de masse au bord.

iconv(isca(irho)) = 0

! --> Prise en compte de la pression predite pour resoudre Navier-Stokes
!     (oui = 1 , non = 0)

igrdpp = 1

! --> Prediction de pression par une equation d'evolution

!     ATTENTION   PAS ENCORE IMPLEMENTE
!========   LAISSER IPPRED = 0

ippred = 0


!===============================================================================
! 6. VERIFICATIONS
!===============================================================================

iok = 0
if(icfgrp.ne.0.and.icfgrp.ne.1) then
  write(nfecra,5000)'ICFGRP',icfgrp
  iok = 1
endif

if (iok.ne.0) then
  call csexit (1)
endif

!--------
! FORMATS
!--------

#if defined(_CS_LANG_FR)

 1000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@  La valeur de ISCALT est renseignee automatiquement.       ',/,&
'@                                                            ',/,&
'@  L''utilisateur ne doit pas la renseigner, or              ',/,&
'@    elle a ete affectee comme suit :                        ',/,&
'@    ISCALT = ',I10                                           ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les parametres.                                  ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1001 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@  Les valeurs de ISCSTH sont renseignees automatiquement.   ',/,&
'@                                                            ',/,&
'@  L''utilisateur ne doit pas les renseigner, or             ',/,&
'@    pour le scalaire ',I10   ,' correspondant au scalaire   ',/,&
'@    physique particuliere ',I10   ,' on a                   ',/,&
'@    ISCSTH(',I10   ,') = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les parametres.                                  ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 2000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@  Les bornes des variables rho, energie ou temperature      ',/,&
'@    ont ete modifiees :                                     ',/,&
'@                                                            ',/,&
'@                      SCAMIN        SCAMAX                  ',/,&
'@  rho         ',2E14.5                                       ,/,&
'@  energie     ',2E14.5                                       ,/,&
'@  temperature ',2E14.5                                       ,/,&
'@                                                            ',/,&
'@  Les bornes de ces variables ne doivent pas etre modifiees.',/,&
'@  On peut modifier les bornes des variables rho et energie  ',/,&
'@  dans uscfx1, mais ce n''est pas conseille.                ',/,&
'@  Il est preferable de gerer les depassements �ventuels     ',/,&
'@  au moyen du sous programme cfther (arret du calcul en fin ',/,&
'@  de pas de temps en cas de depassement).                   ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les parametres.                                  ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@  L''option IPUCOU = ',I10                                   ,/,&
'@    n''est pas compatible avec le module compressible       ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Imposer IPUCOU = 0.                                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@  Les estimateurs ne sont pas compatibles avec le module    ',/,&
'@    compressible.                                           ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Imposer IESCAL(.) = 0.                                    ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 5000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (COMPRESSIBLE) DEMANDEE           ',/,&
'@                                                            ',/,&
'@    ',A6,' DOIT ETRE UN ENTIER EGAL A 0 OU 1                ',/,&
'@    IL VAUT ICI ',I10                                        ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier uscfx2.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
6000 format(                                                      &
'@',                                                            /,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',                                                            /,&
'@ @@ ATTENTION :      A L''ENTREE DES DONNEES',                /,&
'@    =========',                                               /,&
'@   EN COMPRESSIBLE',                                          /,&
'@   CONCERNANT LE COUPLAGE DES COMPOSANTES DE VITESSE',        /,&
'@   LA SEULE VALEUR POSSIBLE POUR LE PARAMETRE IVELCO EST 0',  /,&
'@' ,                                                           /,&
'@   IVELCO A ETE IMPOSE ICI A', I10,                           /,&
'@   IL EST DONC REMIS A 0',                                    /,&
'@',                                                            /,&
'@  Le calcul sera execute',                                    /,&
'@',                                                            /,&
'@  Il est conseille de verifier les parametres donnes via',    /,&
'@  cs_user_parameters.f90.',                                   /,&
'@',                                                            /,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',                                                            /)

#else

 1000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@  The value of ISCALT is set automatically.                 ',/,&
'@                                                            ',/,&
'@  The user should not give a value for it, however          ',/,&
'@    it has been given the following value:                  ',/,&
'@    ISCALT = ',I10                                           ,/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Check parameters.                                         ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1001 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@  The values of ISCSTH are set automatically.               ',/,&
'@                                                            ',/,&
'@  The user should not set a value for them, however         ',/,&
'@    for the scalar ',I10   ,' corresponding to the specific ',/,&
'@    physics scalar ',I10   ,' we have                       ',/,&
'@    ISCSTH(',I10   ,') = ',I10                               ,/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Check parameters.                                         ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 2000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@  The bounds of the variables density, energy or temperature',/,&
'@    have been modified :                                    ',/,&
'@                                                            ',/,&
'@                      SCAMIN        SCAMAX                  ',/,&
'@  density     ',2E14.5                                       ,/,&
'@  energy      ',2E14.5                                       ,/,&
'@  temperature ',2E14.5                                       ,/,&
'@                                                            ',/,&
'@  The bounds of these variables should not be modified.     ',/,&
'@  It is possible to modify the bounds of the variables      ',/,&
'@  density or energy in uscfx2, but it is not recommended.   ',/,&
'@  It is advised to manage the possible overshoot by the     ',/,&
'@  use of the subroutine cfther (stop of the calculation     ',/,&
'@  at the end of the time step in case of an overshoot).     ',/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Check parameters.                                         ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@  The option IPUCOU = ',I10                                  ,/,&
'@    is not compatible with the compressible module          ',/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Impose IPUCOU = 0.                                        ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@  The error estimators are not compatible with the          ',/,&
'@    compressible module.                                    ',/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Impose IESCAL(.) = 0.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 5000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING : STOP WHILE READING INPUT DATAS                ',/,&
'@    =========                                               ',/,&
'@    SPECIFIC PHYSICS MODULES (COMPRESSIBLE) SET             ',/,&
'@                                                            ',/,&
'@    ',A6,' MUST BE AN INTEGER EGAL TO 0 OR 1                ',/,&
'@    IT HAS VALUE',I10                                        ,/,&
'@                                                            ',/,&
'@  The calculation could NOT run.                            ',/,&
'@                                                            ',/,&
'@  Check uscfx2.                                             ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 6000 format(                                                     &
'@',                                                            /,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',                                                            /,&
'@ @@   WARNING :      WHEN READING INPUT DATA',                /,&
'@    =========',                                               /,&
'@   FOR COMPRESSIBLE MODEL',                                   /,&
'@   AND THE CHOICE FOR VELOCITY COMPONENTS COUPLING',          /,&
'@   THE ONLY POSSIBLE VALUE FOR THE PARAMETER IVELCO IS 0',    /,&
'@' ,                                                           /,&
'@   IVELCO IS IMPOSED HERE AS', I10,                           /,&
'@   IT IS THEN REPLACED BY 0',                                 /,&
'@',                                                            /,&
'@  computation will go on',                                    /,&
'@',                                                            /,&
'@ Check the input data given in cs_user_parameters.f90.',      /,&
'@',                                                            /,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@',                                                            /)

#endif

return
end subroutine



