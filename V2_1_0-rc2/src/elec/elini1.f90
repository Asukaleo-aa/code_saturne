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

subroutine elini1
!================


!===============================================================================
!  FONCTION  :
!  ---------

!   INIT DES OPTIONS DES VARIABLES POUR LE MODULE ELECTRIQUE
!      EN COMPLEMENT DE CE QUI A DEJA ETE FAIT DANS USINI1

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
use elincl

!===============================================================================

implicit none

! Local variables

integer          idimve , iesp
integer          ipp , ii , iok
integer          isc , ivar

!===============================================================================
!===============================================================================
! 0. VERIFICATION ISCALT, ISCSTH
!===============================================================================
!     L'utilisateur ne doit pas y avoir touche.

iok = 0

if(iscalt.ne.-1) then
  write(nfecra,1000)iscalt
  iok = iok + 1
endif
do ii = 1, nscapp
  if(iscsth(iscapp(ii)).ne.-10) then
    write(nfecra,1001)ii,iscapp(ii),iscapp(ii),iscsth(iscapp(ii))
    iok = iok + 1
  endif
enddo

if(iok.ne.0) then
  call csexit (1)
  !==========
endif

!===============================================================================
! 1. VARIABLES TRANSPORTEES
!===============================================================================

! 1.1 Definition des scamin et des scamax des variables transportees
! ==================================================================

! --> Dans toutes les versions electriques
!     Enthalpie
scamin(ihm)   = -grand
scamax(ihm)   = +grand
!     Potentiel reel
scamin(ipotr) = -grand
scamax(ipotr) = +grand
!     Fractions massiques des constituants
if ( ngazg .gt. 1 ) then
  do iesp = 1, ngazg-1
    scamin(iycoel(iesp)) = 0.d0
    scamax(iycoel(iesp)) = 1.d0
  enddo
endif

! --> Effet Joule (cas potentiel imaginaire)
!     Potentiel imaginaire

if ( ippmod(ieljou).eq.2 .or. ippmod(ieljou).eq.4 ) then
  scamin(ipoti) = -grand
  scamax(ipoti) = +grand
endif

! --> Arc electrique
!     Potentiel vecteur
if ( ippmod(ielarc).ge.2 ) then
  do idimve = 1, ndimve
    scamin(ipotva(idimve)) = -grand
    scamax(ipotva(idimve)) = +grand
  enddo
endif

! --> Conduction ionique (a developper)

! 1.2 Nature des scalaires transportes
! ====================================

! ---- Type de scalaire (0 passif, 1 temperature en K
!                                 -1 temperature en C
!                                  2 enthalpie)
!      La distinction -1/1 sert pour le rayonnement

!     Par defaut, scalaire "passif"
do isc = 1, nscapp
  iscsth(iscapp(isc)) = 0
enddo

!     Pour l'enthalpie
iscalt = ihm
iscsth(ihm)   = 2

! 1.4 Donnees physiques ou numeriques propres aux scalaires ELECTRIQUES
! =====================================================================


! --> Conditions associees aux potentiels
!     (les autres variables ont des comportements par defaut)
ivar = isca(ipotr)
iconv (ivar) = 0
istat (ivar) = 0
idiff (ivar) = 1
idifft(ivar) = 0
idircl(ivar) = 1
imgr  (ivar) = 1

if(ippmod(ieljou).eq.2 .or. ippmod(ieljou).eq.4) then
  ivar = isca(ipoti)
  iconv (ivar) = 0
  istat (ivar) = 0
  idiff (ivar) = 1
  idifft(ivar) = 0
  idircl(ivar) = 1
  imgr  (ivar) = 1
endif

if(ippmod(ielarc).ge.2) then
  do idimve = 1, ndimve
    ivar = isca(ipotva(idimve))
    iconv (ivar) = 0
    istat (ivar) = 0
    idiff (ivar) = 1
    idifft(ivar) = 0
    idircl(ivar) = 1
    imgr  (ivar) = 0
  enddo
endif

! --> "Viscosite" associee au potentiel vecteur
!     (c'est la seule qui est constante)
if ( ippmod(ielarc).ge.2 ) then
  visls0(ipotva(1)) = 1.d0
  visls0(ipotva(2)) = 1.d0
  visls0(ipotva(3)) = 1.d0
endif

! --> Schmidt ou Prandtl turbulent
!     (pour les potentiels, c'est inutile puisque IDIFFT=0)

do isc = 1, nscapp
  sigmas(iscapp(isc)) = 0.7d0
enddo

! ---> Pour tous les scalaires

do isc = 1, nscapp

! ----- Niveau de detail des impressions pour les variables et
!          donc les scalaires (valeurs 0 ou 1)
!          Si = -10000 non modifie par l'utilisateur -> niveau 1

  ivar = isca(iscapp(isc))
  if(iwarni(ivar).eq.-10000) then
    iwarni(ivar) = 1
  endif

! ----- Informations relatives a la resolution des scalaires

!       - Facteur multiplicatif du pas de temps

  cdtvar(ivar) = 1.d0

!         - Schema convectif % schema 2ieme ordre
!           = 0 : upwind
!           = 1 : second ordre
  blencv(ivar) = 1.d0

!         - Type de schema convectif second ordre (utile si BLENCV > 0)
!           = 0 : Second Order Linear Upwind
!           = 1 : Centre
  ischcv(ivar) = 1

!         - Test de pente pour basculer d'un schema centre vers l'upwind
!           = 0 : utlisation automatique du test de pente
!           = 1 : calcul sans test de pente
  isstpc(ivar) = 0

!         - Reconstruction des flux de convection et de diffusion aux faces
!           = 0 : pas de reconstruction
  ircflu(ivar) = 1

enddo


! 1.5 Variable courante : nom, sortie chrono, suivi listing, sortie histo
! =======================================================================

!     Comme pour les autres variables,
!       si l'on n'affecte pas les tableaux suivants,
!       les valeurs par defaut seront utilisees

!     NOMVAR( ) = nom de la variable
!     ICHRVR( ) = sortie chono (oui 1/non 0)
!     ILISVR( ) = suivi listing (oui 1/non 0)
!     IHISVR( ) = sortie historique (nombre de sondes et numeros)
!     si IHISVR(.,1)  = -1 sortie sur toutes les sondes

!     NB : Les 8 premiers caracteres du nom seront repris dans le
!          listing 'developpeur'

! =======================================================================

! --> Variables communes aux versions electriques

ipp = ipprtp(isca(ihm))
NOMVAR(IPP)  = 'Enthalpy'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

ipp = ipprtp(isca(ipotr))
NOMVAR(IPP)  = 'POT_EL_R'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

if ( ngazg .gt. 1 ) then
  do iesp = 1, ngazg-1
    ipp = ipprtp(isca(iycoel(iesp)))
    WRITE(NOMVAR(IPP),'(A6,I2.2)')'YM_ESL',IESP
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  enddo
endif

! --> Version effet Joule

if ( ippmod(ieljou).eq.2 .or. ippmod(ieljou).eq.4) then
  ipp = ipprtp(isca(ipoti))
  NOMVAR(IPP)  = 'POT_EL_I'
  ichrvr(ipp)  = 1
  ilisvr(ipp)  = 1
  ihisvr(ipp,1)= -1
endif

! --> Version arc electrique

if ( ippmod(ielarc).ge.2 ) then
  do idimve = 1, ndimve
    ipp = ipprtp(isca(ipotva(idimve)))
    WRITE(NOMVAR(IPP),'(A7,I1.1)')'POT_VEC',IDIMVE
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  enddo
endif

! --> Version conduction ionique

!===============================================================================
! 2. VARIABLES ALGEBRIQUES OU D'ETAT
!===============================================================================

ipp = ipppro(ipproc(itemp) )
NOMVAR(IPP)  = 'Temper'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

ipp = ipppro(ipproc(iefjou) )
NOMVAR(IPP)  = 'PuisJoul'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

do idimve = 1, ndimve
  ipp = ipppro(ipproc(idjr(idimve)) )
  WRITE(NOMVAR(IPP),'(A7,I1.1)')'Cour_re',IDIMVE
  ichrvr(ipp)  = 1
  ilisvr(ipp)  = 1
  ihisvr(ipp,1)= -1
enddo

if ( ippmod(ieljou).eq.4 ) then
  do idimve = 1, ndimve
    ipp = ipppro(ipproc(idji(idimve)) )
    WRITE(NOMVAR(IPP),'(A7,I1.1)')'CouImag',IDIMVE
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  enddo
endif

if ( ippmod(ielarc).ge.1 ) then
  do idimve = 1, ndimve
    ipp = ipppro(ipproc(ilapla(idimve)) )
    WRITE(NOMVAR(IPP),'(A7,I1.1)')'For_Lap',IDIMVE
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  enddo

  if ( ixkabe .eq.1 ) then
    ipp = ipppro(ipproc(idrad) )
    NOMVAR(IPP)  = 'Coef_Abso'
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  endif

  if ( ixkabe .eq.2 ) then
    ipp = ipppro(ipproc(idrad) )
    NOMVAR(IPP)  = 'TS_radia'
    ichrvr(ipp)  = 1
    ilisvr(ipp)  = 1
    ihisvr(ipp,1)= -1
  endif

endif

if ( ippmod(ielion).ge.1 ) then
  ipp = ipppro(ipproc(iqelec) )
  NOMVAR(IPP)  = 'Charge'
  ichrvr(ipp)  = 1
  ilisvr(ipp)  = 1
  ihisvr(ipp,1)= -1
endif

! Conductivite Electrique

ipp = ipppro(ipproc(ivisls(ipotr)) )
NOMVAR(IPP)  = 'Sigma'
ichrvr(ipp)  = 1
ilisvr(ipp)  = 1
ihisvr(ipp,1)= -1

!     Conductivite electrique imaginaire :
!     La conductivite reelle et imaginaire sont dans le meme tableau.
!     Il convient donc de ne pas renseigner NOMVAR ICHRVR ILISVR IHISVR
!     pour IPP = IPPPRO(IPPROC(IVISLS(IPOTI)) )
!     puisque IPPROC(IVISLS(IPOTI)) = IPPROC(IVISLS(IPOTR))

!===============================================================================
! 3. INFORMATIONS COMPLEMENTAIRES
!===============================================================================

! --> Coefficient de relaxation de la masse volumique
!      a partir du 2ieme pas de temps, on prend :
!      RHO(n+1) = SRROM * RHO(n) + (1-SRROM) * RHO(n+1)
srrom = 0.d0

! --> Recalage des variables electriques
!      IELCOR = 0 : pas de correction
!      IELCOR = 1 : correction
ielcor = 0

!     Intensite de courant imposee (arc electrique) ou
!                Puissance imposee (Joule)
couimp = 0.d0
puisim = 0.d0

!     Differentiel de potentiel Initial en arc (et Joule)
dpot = 0.d0

!     Coefficient pour la correction en Joule
coejou = 1.d0

! ---> Masse volumique variable et viscosite variable (pour les suites)
irovar = 1
ivivar = 1

!===============================================================================
! 4. ON REDONNE LA MAIN A L'UTLISATEUR
!===============================================================================

call useli1
!==========

!===============================================================================
! 5. VERIFICATION DES DONNEES ELECTRIQUES
!===============================================================================

iok = 0

call elveri (iok)
!==========

if(iok.gt.0) then
  write(nfecra,9999)iok
  call csexit (1)
  !==========
else
  write(nfecra,9998)
endif

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (JOULE) DEMANDEE                  ',/,&
'@                                                            ',/,&
'@  La valeur de ISCALT est renseignee automatiquement.       ',/,&
'@                                                            ',/,&
'@  L''utilisateur ne doit pas la renseigner dans usini1, or  ',/,&
'@    elle a ete affectee comme suit :                        ',/,&
'@    ISCALT = ',I10                                           ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1001 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE (JOULE) DEMANDEE                  ',/,&
'@                                                            ',/,&
'@  Les valeurs de ISCSTH sont renseignees automatiquement.   ',/,&
'@                                                            ',/,&
'@  L''utilisateur ne doit pas les renseigner dans usini1, or ',/,&
'@    pour le scalaire ',I10   ,' correspondant au scalaire   ',/,&
'@    physique particuliere ',I10   ,' on a                   ',/,&
'@    ISCSTH(',I10   ,') = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 9998 format(                                                           &
'                                                             ',/,&
' Pas d erreur detectee lors de la verification des donnees   ',/,&
'                                                    (useli1).',/)
 9999 format(                                                           &
'@                                                            ',/,&
'@                                                            ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    LES PARAMETRES DE CALCUL SONT INCOHERENTS OU INCOMPLETS ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute (',I10,' erreurs).          ',/,&
'@                                                            ',/,&
'@  Se reporter aux impressions precedentes pour plus de      ',/,&
'@    renseignements.                                         ',/,&
'@  Verifier useli1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

return
end subroutine
