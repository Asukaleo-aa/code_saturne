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

subroutine rayopt
!================

!===============================================================================
!  FONCTION  :
!  ---------

!   SOUS-PROGRAMME DU MODULE RAYONNEMENT :
!   --------------------------------------

!  1) Initialisation par defaut du parametrage du module de
!     transferts thermiques radiatifs
!  2) Lecture du parametrage utilisateur
!  3) Controle de coherence avec les physiques particulieres
!  4) Verifications du parametrage utilisateur

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

implicit none

!===============================================================================
! Common blocks
!===============================================================================

include "paramx.h"
include "dimens.h"
include "numvar.h"
include "entsor.h"
include "optcal.h"
include "cstphy.h"
include "ihmpre.h"
include "ppppar.h"
include "ppthch.h"
include "cpincl.h"
include "fuincl.h"
include "ppincl.h"
include "radiat.h"

!===============================================================================

! Local variables

integer          ii, jj, iphas, iok , iiscal, iscaok, ipp, iph, nmodpp, iverif
character        car4*4
character*2      num

!===============================================================================
!===============================================================================
! 0. REDEFINITION DU NOMBRE DE PHASES POUR LE CHARBON PULVERISE
!===============================================================================

!--> nphasc: for pulverized coal and fuel combustion:
!            nphasc = 1 (gaz) + number of classes (particles or droplets)

!--> For pulverized coal and fuel combustion:
if ( ippmod(icp3pl) .ge. 0 ) then
  nphasc = 1 + nclacp
else if ( ippmod(icfuel) .ge. 0 ) then
  nphasc = 1 + nclafu
else
  nphasc = 1
endif

nmodpp = 0
do ipp = 2, nmodmx
  if (ippmod(ipp).ne.-1) then
    nmodpp = nmodpp+1
  endif
enddo

!===============================================================================
! 1. INITIALISATIONS PAR DEFAUT DU MODULE DE TRANSFERTS RADIATIFS
!                        ^^^^^^
!===============================================================================


!-->  IIRAYO = 0 : PAS DE TRANSFERTS RADIATIFS
!            = 1 : TRANSFERTS RADIATIFS, METHODE DES ORDONNEES DISCRETES
!            = 2 : TRANSFERTS RADIATIFS, APPROXIMATION P-1
!     On initialise a -1 pour montrer que ce n'est pas initialise ...
!        (on fera un test apres usray1)
iirayo = -1

!--> IRAPHA : NUMERO DE LA PHASE POUR LAQUELLE ON FAIT DU RAYONNEMENT

irapha = 1

!-->  CALCUL DU COEFFICIENT D'ABSORPTION
!      IMODAK = 0 : sans utiliser modak
!               1 : a l'aide modak

imodak = 0

!-->  INDICATEUR SUITE DE CALCUL (LECTURE DU FICHIER SUITE)

isuird = -1

!-->  FREQUENCE DE PASSAGE DANS LE MODULE RAYONNEMENT

nfreqr = -1

!-->  NOMBRE DE DIRECTIONS : 32 OU 128

ndirec = -1

!-->  NOMBRE DE BANDES SPECTRALES (PAS UTILISE)

nbande = 1

!-->  POURCENTAGE DE CELLULES OU L'ON ADMET QUE LA LONGUEUR OPTIQUE DEPASSE
!       L'UNITE POUR LE MODELE P-1

xnp1mx = 10.d0

!-->  INITIALISATION DU MODE DE CALCUL DU TERME SOURCE RADIATIF EXPLICITE
!     IDIVER = 0 => CALCUL SEMI-ANALYTIQUE (OBLIGATOIRE SI TRANSPARENT)
!     IDIVER = 1 => CALCUL CONSERVATIF
!     IDIVER = 2 => CALCUL SEMI-ANALYTIQUE CORRIGE POUR ETRE CONSERVATIF
!     REMARQUE : SI TRANSPARENT IDIVER = -1 AUTOMATIQUEMENT DANS RAYDOM

idiver = -1

!--> NIVEAU D'AFFICHAGE (0,1,2) DES RENSEIGNEMENTS TEMPERATURE DE PAROI

iimpar = -1

!--> NIVEAU D'AFFICHAGE (0,1,2) DES RENSEIGNEMENTS RESOLUTION LUMINANCE

iimlum = -1

!   - Interface Code_Saturne
!     ======================

if (iihmpr.eq.1) then

  call uiray1(iirayo, isuird, ndirec, nfreqr, idiver, iimpar, iimlum)
  !==========

endif

call usray1
!==========

!===============================================================================
! 2. VERIFICATION LA COHERENCE D'UTILISATION DU MODULE DE RAYONNEMENT
!    AVEC LA THERMIQUE OU LES PHYSIQUES PARTICULIERES (COMBUSTION)
!===============================================================================

iok = 0

!--> IIRAYO = 0 (pas de rayonnement).

if(iirayo.eq.-1) then
  iirayo = 0
endif

if (iirayo.ne.0 .and. iirayo.ne.1 .and. iirayo.ne.2) then
  write(nfecra,1010) iirayo
  iok = iok + 1
endif

if (imodak.ne.0 .and. imodak.ne.1) then
  write(nfecra,1020) imodak
  iok = iok + 1
endif

!--> ISCSTH

!     Si physique particuliere avec fichier parametrique,
!       ISCSTH a ete renseigne
!       dans ppini1 ou coini1 (a verifier en elec).
!     Si physique classique et ISCSTH pas modifie dans USRAY1
!                           ou pas de variable thermique
!       STOP.

if(ippmod(iphpar).ge.2) then

!     Il y a une seule phase ; si on rayonne
    if (iirayo.eq.1 .or. iirayo.eq.2) then
!        On cherche s'il y a un scalaire thermique
      iscaok = 0
      do iiscal = 1, nscal
        if (iiscal.eq.iscalt(irapha)) then
          iscaok = 1
!           Et on regarde si on a dit enthalpie
          if (iscsth(iiscal).ne.2) then
            write(nfecra,3000) irapha,iiscal,iiscal
            iok = iok + 1
          endif
        endif
      enddo
      if (iscaok.eq.0) then
        write(nfecra,3001) irapha,irapha
        iok = iok + 1
      endif
    endif

else

!     Pour la phase qui rayonne
    if (iirayo.eq.1 .or. iirayo.eq.2) then

!        On cherche s'il y a un scalaire thermique
      iscaok = 0
      do iiscal = 1, nscal
        if (iiscal.eq.iscalt(irapha)) then
          iscaok = 1

!           Et on regarde si on a dit temp C, K ou enthalpie
          if (abs(iscsth(iiscal)).ne.1.and.                       &
                  iscsth(iiscal) .ne.2      ) then
            write(nfecra,3010) irapha,iiscal,iiscal
            iok = iok + 1
          endif

        endif
      enddo
      if(iscaok.eq.0)then
        write(nfecra,3011)irapha,irapha
        iok = iok + 1
      endif

    endif

endif

!--> Stop si erreur.

if(iok.ne.0) then
  call csexit (1)
  !==========
endif

 1010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    IIRAYO NE PEUT PRENDRE POUR VALEURS QUE 0 1 OU 2        ',/,&
'@    IIRAYO vaut ',I10                                        ,/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Arret dans rayopt.                                        ',/,&
'@  Verifier usray1 ou l''interface graphique.                ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1020 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    IMODAK NE PEUT PRENDRE POUR VALEURS QUE 0 OU 1          ',/,&
'@    IMODAK vaut',I10                                         ,/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Arret dans rayopt.                                        ',/,&
'@  Verifier usray1 ou l''interface graphique.                ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE ACTIVEE : ENTHALPIE NECESSAIRE    ',/,&
'@                                                            ',/,&
'@  Avec rayonnement, pour la phase ',I10   ,', il faut       ',/,&
'@    preciser la variable energetique representee par le     ',/,&
'@    scalaire ',I10   ,' en renseignant ISCSTH(',I10   ,')   ',/,&
'@    dans usini1 : soit                                      ',/,&
'@               -1 temperature en C                          ',/,&
'@                1 temperature en K                          ',/,&
'@                2 enthalpie                                 ',/,&
'@                                                            ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Avec physique particuliere, cette initialisation aurait   ',/,&
'@    du etre automatique.                           ~~~~~~   ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3001 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    PHYSIQUE PARTICULIERE ACTIVEE : ENTHALPIE NECESSAIRE    ',/,&
'@                                                            ',/,&
'@  Lorsque le rayonnement est utilise (phase ',I10   ,'), il ',/,&
'@    faut indiquer qu''un scalaire represente la variable    ',/,&
'@    energetique (enthalpie) en renseignant                  ',/,&
'@    ISCALT(',I10   ,') dans usini1 (numero du scalaire).    ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Avec physique particuliere, cette initialisation aurait   ',/,&
'@    du etre automatique.                           ~~~~~~   ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    ISCSTH DOIT ETRE RENSEIGNE OBLIGATOIREMENT DANS USINI1  ',/,&
'@                                                            ',/,&
'@  Avec rayonnement, pour la phase ',I10   ,', il faut       ',/,&
'@    preciser la variable energetique representee par le     ',/,&
'@    scalaire ',I10   ,' en renseignant ISCSTH(',I10   ,')   ',/,&
'@    dans usini1 : soit                                      ',/,&
'@               -1 temperature en C                          ',/,&
'@                1 temperature en K                          ',/,&
'@                2 enthalpie                                 ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3011 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    IL FAUT UTILISER UNE VARIABLE ENERGETIQUE.              ',/,&
'@                                                            ',/,&
'@  Lorsque le rayonnement est utilise (phase ',I10   ,', il  ',/,&
'@    faut indiquer qu''un scalaire represente la variable    ',/,&
'@    energetique (temperature ou enthalpie) en renseignant   ',/,&
'@    ISCALT(',I10   ,') dans usini1 (numero du scalaire).    ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

!===============================================================================
! 3. VERIFICATIONS (uniquement s'il y a du rayonnement)
!===============================================================================

iok = 0

if (iirayo.gt.0) then

! Positionnement des pointeurs
! 5eme passage (les autres sont dans iniusi)

  call varpos(nmodpp)
  !==========

! --> ISUIRD

  if (isuird.ne.0 .and. isuird.ne.1 ) then
    write(nfecra,4000) isuird
    iok = iok + 1
  endif

! --> NFREQR

  if (nfreqr.le.0) then
    write(nfecra,4010) nfreqr
    iok = iok + 1
  endif

! --> NDIREC
!     Choix entre 32 et 128 directions (cf raysol)
  if (iirayo.eq.1) then
    if (ndirec.ne.32 .and. ndirec.ne.128 ) then
      write(nfecra,4020) ndirec
      iok = iok + 1
    endif
  endif

! --> IDIVER
!     Choix entre 0  1 et 2
  if (idiver.ne.0 .and. idiver.ne.1 .and. idiver.ne.2) then
    write(nfecra,4030) idiver
    iok = iok + 1
  endif

! --> IIMPAR
!     Choix entre 0  1 et 2
  if (iimpar.ne.0 .and. iimpar.ne.1 .and. iimpar.ne.2) then
    write(nfecra,4040) iimpar
    iok = iok + 1
  endif

! --> IIMLUM
!     Choix entre 0  1 et 2
  if (iimlum.ne.0 .and. iimlum.ne.1 .and. iimlum.ne.2) then
    write(nfecra,4050) iimlum
    iok = iok + 1
  endif

else
  return
endif

!--> Stop si erreur.

if(iok.ne.0) then
  call csexit (1)
  !==========
endif

!===============================================================================
! 5. POST-PROCESSING
!===============================================================================

!--> INITIALISATION DES DONNEES POST-PROCESSING
!    NBRAYF : NOMBRE MAX DES SORTIES DE VARIABLES FACETTES DE BORD

do ii = 1, nbrayf
  WRITE(CAR4,'(I4.4)') II
  NBRVAF(II) = 'RAYTFB'//CAR4
  irayvf(ii) = 0
enddo

!--> LUMINENCE

ipp = ipppro(ipproc(ilumin))
NOMVAR(IPP)   = 'Lumin'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = 0
ilisvr(ipp)   = 0

!--> VECTEUR DENSITE DE FLUX RADIATIF

!     composante x
ipp = ipppro(ipproc(iqx))
NOMVAR(IPP)   = 'Qxrad'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!     composante y
ipp = ipppro(ipproc(iqy))
NOMVAR(IPP)   = 'Qyrad'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!      composante z
ipp = ipppro(ipproc(iqz))
NOMVAR(IPP)   = 'Qzrad'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!--> TERME SOURCE IMPLICITE

ipp = ipppro(ipproc(itsri(1)))
NOMVAR(IPP)   = 'ITSRI'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = 0
ilisvr(ipp)   = 0

!--> TERME SOURCE RADIATIF (ANALYTIQUE/CONSERVATIF/SEMI-ANALYTIQUE)

ipp = ipppro(ipproc(itsre(1)))
NOMVAR(IPP)   = 'Srad'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!--> PART DE L'ABSORPTION DANS LE TERME SOURCE RADIATIF

ipp = ipppro(ipproc(iabs(1)))
NOMVAR(IPP)   = 'Absorp'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!--> PART DE L'EMISSION DANS LE TERME SOURCE RADIATIF

ipp = ipppro(ipproc(iemi(1)))
NOMVAR(IPP)   = 'Emiss'
ichrvr(ipp)   = 0
ihisvr(ipp,1) = -1
ilisvr(ipp)   = 0

!--> COEFFICIENT D'ABSORPTION DU MILIEU SEMI-TRANSPARENT

ipp = ipppro(ipproc(icak(1)))
NOMVAR(IPP)   = 'CoefAb'
ichrvr(ipp)   = 0
ilisvr(ipp)   = 0
ihisvr(ipp,1) = -1


do iphas = 1, nphasc-1

  WRITE(NUM,'(I1)') IPHAS

!--> TERME SOURCE IMPLICITE

  ipp = ipppro(ipproc(itsri(iphas)))
  NOMVAR(IPP)   = 'ITSRI_'//NUM
  ichrvr(ipp)   = 0
  ihisvr(ipp,1) = 0
  ilisvr(ipp)   = 0


!--> TERME SOURCE RADIATIF (ANALYTIQUE/CONSERVATIF/SEMI-ANALYTIQUE)

  ipp = ipppro(ipproc(itsre(iphas)))
  NOMVAR(IPP)   = 'Srad_'//NUM
  ichrvr(ipp)   = 0
  ihisvr(ipp,1) = -1
  ilisvr(ipp)   = 0

!--> PART DE L'ABSORPTION DANS LE TERME SOURCE RADIATIF

  ipp = ipppro(ipproc(iabs(iphas)))
  NOMVAR(IPP)   = 'Absorp_'//NUM
  ichrvr(ipp)   = 0
  ihisvr(ipp,1) = -1
  ilisvr(ipp)   = 0

!--> PART DE L'EMISSION DANS LE TERME SOURCE RADIATIF

  ipp = ipppro(ipproc(iemi(iphas)))
  NOMVAR(IPP)   = 'Emiss_'//NUM
  ichrvr(ipp)   = 0
  ihisvr(ipp,1) = -1
  ilisvr(ipp)   = 0

!--> COEFFICIENT D'ABSORPTION DU MILIEU SEMI-TRANSPARENT

  ipp = ipppro(ipproc(icak(iphas)))
  NOMVAR(IPP)   = 'CoefAb_'//NUM
  ichrvr(ipp)   = 0
  ilisvr(ipp)   = 0
  ihisvr(ipp,1) = -1

enddo

!===============================================================================
!  6. INITIALISATIONS UTILISATEURS
!                    ^^^^^^^^^^^^
!===============================================================================

!   - Code_Saturne GUI
!     ================

if (iihmpr.eq.1) then

  ! properties on boundaries
  do ii = 1, nbrayf
    call fcnmra(nbrvaf(ii), len(nbrvaf(ii)), ii)
  enddo

   call uiray4(nbrayf, nphas, iirayo, irayvf)
   !==========

  do ii = 1, nbrayf
    call cfnmra(nbrvaf(ii), len(nbrvaf(ii)), ii)
  enddo

  ! properties on cells
  do ii = 1,nvppmx
    call fcnmva (nomvar(ii), len(nomvar(ii)), ii)
    !==========
  enddo

  call csenso                                                     &
  !==========
     ( nvppmx, ncapt,  nthist, ntlist,                            &
       ichrvl, ichrbo, ichrsy, ichrmd,                            &
       fmtchr, len(fmtchr), optchr, len(optchr),                  &
       ntchr,  iecaux,                                            &
       ipstdv, ipstyp, ipstcl, ipstft, ipstfo,                    &
       ichrvr, ilisvr, ihisvr, isca, iscapp,                      &
       ipprtp, xyzcap )

  do ii = 1,nvppmx
    call cfnmva(nomvar(ii), len(nomvar(ii)), ii)
    !==========
  enddo

  call nvamem
  !==========

  ! take into acount users modifications by subroutine usini1
  iverif = 0
  call usipes(nmodpp, iverif)
  !==========

endif

call usray1
!==========

! --> IRAYVF
!     Choix entre -1 et 1
if (iirayo.eq.1 .or. iirayo.eq.2) then
  do ii = 1, nbrayf
    if (irayvf(ii).ne.1 .and. irayvf(ii).ne.-1) then
      write(nfecra,4070) nbrvaf(ii), irayvf(ii)
      iok = iok + 1
    endif
  enddo
endif

!--> Stop si erreur.

if(iok.ne.0) then
  call csexit (1)
  !==========
endif

 4000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    INDICATEUR DE SUITE DE CALCUL NON ADMISSIBLE            ',/,&
'@                                                            ',/,&
'@  L''indicateur de suite de calcul doit etre 0 ou 1 (ISUIRD)',/,&
'@    Il vaut ici ISUIRD = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@     FREQUENCE DE PASSAGE DANS LE MODULE DE RAYONNEMENT     ',/,&
'@     NON ADMISSIBLE                                         ',/,&
'@                                                            ',/,&
'@  La frequence de passage doit etre superieure ou egale a 1 ',/,&
'@    Elle vaut ici NFREQR = ',I10                             ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4020 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    NOMBRE DE DIRECTIONS NON ADMISSIBLE                     ',/,&
'@                                                            ',/,&
'@  Le nombre de directions doit etre 32 ou 128 (NDIREC)      ',/,&
'@    Il vaut ici NDIREC = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4030 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ARRET A L''ENTREE DES DONNEES ',/,&
'@    =========                                               ',/,&
'@    INDICATEUR DU MODE DE CALCUL DU TERME SOURCE RADIATIF   ',/,&
'@    EXPLICITE NON ADMISSIBLE                                ',/,&
'@                                                            ',/,&
'@  L''indicateur du mode de calcul doit etre 0, 1 ou 2       ',/,&
'@    Il vaut ici IDIVER = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4040 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ERREUR A L''ENTREE DES DONNEES',/,&
'@    =========                                               ',/,&
'@    NIVEAU D''AFFICHAGE DES RENSIGNEMENTS DES               ',/,&
'@    TEMPERATURE DE PAROI NON ADMISSIBLE                     ',/,&
'@                                                            ',/,&
'@  Le niveau d''affichage doit etre 0, 1 ou 2  (IIMPAR)      ',/,&
'@    Il vaut ici IIMPAR = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4050 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ERREUR A L''ENTREE DES DONNEES',/,&
'@    =========                                               ',/,&
'@    NIVEAU D''AFFICHAGE DES RENSIGNEMENTS SUR LA            ',/,&
'@    RESOLUTION DE LA LUMINANCE NON ADMISSIBLE               ',/,&
'@                                                            ',/,&
'@  Le niveau d''affichage doit etre 0, 1 ou 2  (IIMLUM)      ',/,&
'@    Il vaut ici IIMLUM = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 4070 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : RAYONNEMENT : ERREUR A L''ENTREE DES DONNEES',/,&
'@    =========                                               ',/,&
'@    INDICATEUR DE SORTIE EN POSTPROCESSING                  ',/,&
'@    POUR ',A40                                               ,/,&
'@    NON ADMISSIBLE                                          ',/,&
'@                                                            ',/,&
'@  L''indicateur de postprocessing doit etre -1 ou 1         ',/,&
'@    Il vaut ici IRAYVF = ',I10                               ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier usray1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)


return

end subroutine
