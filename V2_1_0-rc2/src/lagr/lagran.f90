!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2011 EDF S.A., France

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

! Module for Lagrangian: non dimensions

module lagran

  !===========================================================================

  !         Trois modules complementaires
  !                            lagran qui porte les non dimensions
  !                            lagdim qui porte les dimensions variables
  !                            lagpar qui porte les parametres

  use lagpar

  !=============================================================================
  !  1. Base

  !     IILAGR = 0 : PAS DE CALCUL LAGRANGIEN
  !            = 1 : DIPHASIQUE LAGRANGIEN SANS COUPLAGE RETOUR
  !            = 2 : DIPHASIQUE LAGRANGIEN AVEC COUPLAGE RETOUR

  !     ISUILA = 0 : PAS SUITE LAGRANGIEN
  !            = 1 :     SUITE LAGRANGIEN

  !     ISTTIO = 0 : calcul instationnaire pour le lagrangien
  !            = 1 : calcul stationnaire   pour le lagrangien

  integer, save ::           iilagr , isuila , isttio

  !=============================================================================
  ! 2. Compteurs de particules (sans et avec poids statistique)

  !     NBPART/DNBPAR : NOMBRE DE PARTICULES PRESENTES DANS LE DOMAINE
  !                        A CHAQUE ITERATION

  !     NBPNEW/DNBPNW : NOMBRE DE NOUVELLES PARTICULES ENTRANTES

  !     NBPERR/DNBPER : NOMBRE DE PARTICULES ELIMINEES EN ERREUR

  !     NBPDEP/DNBDEP : NOMBRE DE PARTICULES DEPOSEES

  !     NBPERT : NOMBRE DE PARTICULES ELIMINEES EN ERREUR DANS
  !                LE CALCUL DEPUIS LE DEBUT, SUITES COMPRISES

  !     NBPTOT : NOMBRE DE PARTICULES TOTAL INJECTE DANS
  !                LE CALCUL DEPUIS LE DEBUT SUITE COMPRISE

  !     NBPOUT/DNBPOU : Contient les particules sorties de facon normal,
  !                       plus les particules sorties en erreur de reperage.

  !     NDEPOT : Nombre de particules deposees definitivement
  !               dont on garde une trace en memoire pour le
  !               post-processing en mode deplacement.

  !     NPCLON/DNPCLO : NOMBRE DE NOUVELLES PARTICULES PAR CLONNAGE

  !     NPKILL/DNPKIL : NOMBRE DE PARTICULES VICTIMES DE LA ROULETTE RUSSE

  !     NPCSUP/DNPCSU : NOMBRE DE PARTICULES QUI ON SUBIT LE CLONNAGE


  integer, save ::           nbpart , nbpnew , nbperr , nbptot , nbpout ,    &
                             nbpert , ndepot , nbpdep
  double precision, save ::  dnbpar , dnbpnw , dnbper , dnbpou, dnbdep

  integer, save ::           npclon , npkill , npcsup
  double precision, save ::  dnpclo , dnpkil , dnpcsu

  !=============================================================================
  ! 4. Physiques particulieres

  !       SI IPHYLA = 1 ALORS

  !          ITPVAR : EQUATION SUR LA TEMPERATURE
  !          IDPVAR : EQUATION SUR LE DIAMETRE
  !          IMPVAR : EQUATION SUR LA MASSE

  integer, save ::           iphyla, itpvar, idpvar, impvar

  !       SI SUITE ET ENCLENCHEMENT ITPVAR =1 EN COUR DE CALCUL

  !          TPART  : Temperature d initialisation en degres Celsius
  !          CPPART : Chaleur massique specifique (J/kg/K)

  double precision, save ::  tpart , cppart

  ! 4.1 Particules deposition submodel (Guingo & Minier, 2008)
  !==================================


  !     IDEPST = 0 : no deposition submodel activated
  !            = 1 : deposition submodel used

  integer, save ::     idepst

  !     NGEOL : geometric parameters stored

  integer ngeol
  parameter (ngeol = 13)

  ! Additional pointers in the ITEPA array
  ! ITEPA contains the particule state

  integer, save ::   jimark , jdiel  , jdfac , jdifel , jtraj , jptdet , jinjst
  integer, save ::   jryplu , jrinpf

  !=============================================================================
  ! 5. Pas de temps Lagrangien

  !    IPLAS : NOMBRE DE PASSAGES ABSOLUS DANS LE MODULE LAGRANGIEN
  !    IPLAR : NOMBRE DE PASSAGES RELATIFS DANS LE MODULE LAGRANGIEN

  integer, save ::           iplas , iplar

  !    DTP :  duree d une iteration lagrangienne
  !    TTCLAG : temps courant physique lagrangien

  double precision, save ::  dtp , ttclag

  !=============================================================================
  ! 6. Indicateur d erreur

  integer, save :: ierr

  !=============================================================================
  ! 3. Pointeurs particules

  !   Tableau ETTP
  !   ^^^^^^^^^^^^

  !    JXP,JYP,JZP  : COORDONNES DE LA POSITION DE LA PARTICULE
  !    JUP,JVP,JWP  : COMPOSANTES DE LA VITESSE ABSOLUE
  !    JUF,JVF,JWF  : COMPOSANTES DE LA VITESSE DU FLUIDE VU

  !    JMP,JDP      : MASSE, DIAMETRE
  !    JTP,JTF,JCP  : TEMPERATURE PARTICULE ET FLUIDE ET CHALEUR SPECIFIQUE
  !    JVLS(NUSVAR) : VARIABLE SUPPLEMENTAIRES

  !   Charbon
  !   -------
  !    JHP          : TEMPERATURE DES GRAINS DE CHARBON
  !    JMCH         : MASSE DE CHARBON REACTIF
  !    JMCK         : MASSE DE COKE

  integer, save ::           jxp , jyp , jzp ,                               &
                             jup , jvp , jwp ,                               &
                             juf , jvf , jwf ,                               &
                             jmp , jdp , jtp , jtf , jcp ,                   &
                             jhp , jmch, jmck,                               &
                             jvls(nusvar)

  !   Tableau TEPA
  !   ^^^^^^^^^^^^

  !     JRTSP       : TEMPS DE SEJOUR DES PARTICULES
  !     JRPOI       : POIDS DES PARTICULES
  !     JREPS       : EMISSIVITE DES PARTICULES

  !   Charbon
  !   -------
  !     JRDCK       : DIAMETRE DU COEUR RETRECISSANT
  !     JRD0P       : DIAMETRE INITIAL DES PARTICULES
  !     JRR0P       : MASSE VOLUMIQUE INITIALE DES PARTICULES

  integer, save ::           jrtsp, jrpoi, jreps, jrd0p, jrr0p, jrdck

  !   Tableau ITEPA
  !   ^^^^^^^^^^^^^

  !     JISOR       : MAILLE D ARRIVEE

  !   Statistique par classe
  !   ----------------------

  !     JCLST       : classe (statique ) a laquelle la particule appartient

  !   Charbon
  !   -------
  !     JINCH       : NUMERO DU CHARBON DE LA PARTICULE

  integer, save ::           jisor, jinch , jclst

  !    NVLS         : NOMBRE DE VARIABLES UTILISATEUR SUPPLEMENTAIRES
  !                   (DEJA CONTENU DANS NVP et NVP1)

  integer, save ::           nvls

  !=============================================================================
  ! 7. Conditions aux limites

  !     TABLEAUX POUR LES CONDITIONS AUX LIMITES
  !     ----------------------------------------

  !     NFRLAG  : nbr de zones frontieres
  !     INJCON  : INJECTION CONTINUE OU NON
  !     ILFLAG  : liste des numeros des zones frontieres
  !     IUSNCL  : nbr de classes par zones
  !     IUSCLB  : conditions au bord pour les particules
  !          = IENTRL
  !          = ISORTL -> particule sortie du domaine par une face fluide
  !          = IREBOL -> rebond elastique
  !          = IDEPO1 -> deposition definitive (particule eliminee de la memoire)
  !          = IDEPO2 -> deposition definitive (part. non eliminee de la memoire)
  !          = IDEPO3 -> deposition temporaire (remise en suspension possible)
  !          = IENCRL -> encrassement (Charbon uniquement IPHYLA = 2)
  !          = JBORD1 -> interactions utilisateur
  !          = JBORD2 -> interactions utilisateur
  !          = JBORD3 -> interactions utilisateur
  !          = JBORD4 -> interactions utilisateur
  !          = JBORD5 -> interactions utilisateur
  !     IUSMOY  : tableau si on fait une moyenne par zone sur la zone considere
  !     IUSLAG  : tableau d info par classe et par frontieres
  !     DEBLAG  : debit massique par zone

  integer, save ::           nfrlag, injcon,                                 &
                             ilflag(nflagm),                                 &
                             iusncl(nflagm),                                 &
                             iusclb(nflagm),                                 &
                             iusmoy(nflagm),                                 &
                             iuslag(nclagm, nflagm, ndlaim)

  double precision, save ::  deblag(nflagm)

  !     IJNBP  : nbr de part par classe et zones frontieres
  !     IJFRE  : frequence d injection
  !               (si < 0 : on ne rentre des particles qu a la 1ere iter)
  !     IJUVW  : type de condition vitesse
  !          = -1 vitesse fluide imposee
  !          =  0 vitesse imposee selon la direction normale
  !               a la face de bord et de norme IUNO
  !          =  1 vitesse imposee : on donne IUPT IVPT IWPT
  !          =  2 profil de vitesse donne par l'utilisateur
  !     IJPRPD = 1 distribution uniforme
  !            = 2 profil de taux de presence donne par l'utilisateur
  !     IJPRTP = 1 profil plat de temperature donne par la valeur dans uslag2
  !            = 2 profil de temperature donne par l'utilisateur
  !     IJPRDP = 1 profil plat de diametre donne par la valeur dans uslag2
  !            = 2 profil dediametre donne par l'utilisateur
  !     INUCHL : numero du charbon de la particule (si IPHYLA=2)
  !     ICLST  : numero du groupe de statistiques

  integer, save ::           ijnbp, ijfre, ijuvw, ijprtp, ijprdp, ijprpd
  integer, save ::           inuchl, iclst

  !     RUSLAG  : tableau d info par classe et par frontieres

  double precision, save ::  ruslag(nclagm, nflagm, ndlagm)

  !     IUNO  : Norme de la vitesse
  !     IUPT  : U par classe et zones
  !     IVPT  : V par classe et zones
  !     IWPT  : W par classe et zones
  !     IDEBT : Debit
  !     IPOIT : Poids de la particule
  !     IDPT  : Diametre
  !     IVDPT : Variance du diametre
  !     ITPT  : Temperature
  !     ICPT  : Cp
  !     IEPSI : Emissivite des particules
  !     IROPT : Masse volumique
  !     IHPT  : Temperature
  !     IMCHT : Masse de charbon reactif
  !     IMCKT : Masse de coke
  !     IDCKT : Diametre du coeur retrecissant

  integer, save ::           iuno, iupt, ivpt, iwpt,                         &
                             itpt, idpt, ivdpt, iropt,                       &
                             icpt, ipoit, idebt, iepsi,                      &
                             ihpt, imcht, imckt, idckt

  !=============================================================================
  ! 8. Statistiques

  !     POINTEURS POUR LES STATISTIQUES
  !     -------------------------------

  !     ILVX,ILVY,ILVZ    : Vitesse
  !     ILFV              : Concentration volumique
  !     ILPD              : Somme des poids statistiques
  !     ILTS              : Temps de sejour

  !     ILTP              : Temperature
  !     ILDP              : Diametre
  !     ILMP              : Masse

  !     ILHP              : Temperature
  !     ILMCH             : Masse de charbon reactif
  !     ILMCK             : Masse de coke
  !     ILDCK             : Diametre du coeur retrecissant

  !     ILVU(NUSSTA)      : Statistiques supplementaires utilisateur

  integer, save ::           ilvx  , ilvy  , ilvz  ,                         &
                             ilpd  , ilfv  , ilts  ,                         &
                             iltp  , ildp  , ilmp  ,                         &
                             ilhp  , ilmch , ilmck , ildck ,                 &
                             ilvu(nussta)

  !     DONNEES POUR LES STATISTIQUES VOLUMIQUES
  !     ----------------------------------------

  !      ISTALA : Calcul statistiques       si  >= 1 sinon pas de stat
  !      ISUIST : Suite calcul statistiques si  >= 1 sinon pas de stat
  !      NVLSTS : NOMBRE DE VARIABLES STATISTIQUES SUPPLEMENTAIRES
  !               UTILISATEUR (CONTENU DANS NVLSTA)
  !      IDSTNT : Numero du pas de temps pour debut statistque
  !      NSTIST : Debut calcul stationnaire
  !      NPST   : Nombre de pas de temps pour le cumul des stats
  !      NPSTT  : Nombre de pas de temps total des stats depuis le debut
  !               du calcul, partie instationnaire comprise
  !      TSTAT  : Temps physique des stats volumiques
  !      SEUIL  : Seuil en POIDS STAT de particules pour les stats

  integer, save ::           istala , isuist , nvlsts ,                      &
                             idstnt , nstist ,                               &
                             npst   , npstt

  double precision, save ::  tstat , seuil

  !     NOMS DES VARIABLES STATISTIQUES (MOYENNES ET VARIANCES)
  !     -------------------------------------------------------
  !     Taille limitee par le fait qu on utilise NOMBRD dans
  !       l ecriture des fichiers suites (lagout)

  character*32, save ::      nomlag(nvplmx) , nomlav(nvplmx)

  !     OPTION POUR LES HISTORIQUES SUR LES STATS
  !     -----------------------------------------

  integer, save ::           ihslag(nvplmx)

  !     STATISTIQUE PAR ZONE ET PAR CLASSE
  !     ----------------------------------

  integer, save ::           nbclst

  !===============================================================================
  ! 9. Termes Sources

  !     OPTION TERMES SOURCES
  !     ---------------------
  !       Dynamique
  !       Masse
  !       Thermique

  integer, save ::          ltsdyn , ltsmas , ltsthe

  !     POINTEURS POUR LES TERMES SOURCES
  !     ---------------------------------

  !    ITSVX,ITSVY,ITVZ    : Termes sources sur la vitesse
  !    ITSLI               : Terme source implicite (vitesse+turbulence)
  !    ITSKE               : Terme source sur la turbulence en k-eps
  !    ITSR11,ITR12,ITSR13 : Termes sources sur la turbulence en Rij-Eps
  !    ITSR22,ITR23,ITSR33
  !    ITSTE, ITSTI        : Termes sources pour la thermique
  !    ITSMAS              : Terme source pour la masse
  !    ITSMV1              : Terme source sur F1 (MV legeres)
  !    ITSMV2              : Terme source sur F2 (MV loudres)
  !    ITSCO               : Terme source sur F3 (C sous forme de CO)
  !    ITSFP4              : Variance du traceur relatif a l air

  integer, save ::           itsvx  , itsvy  , itsvz  , itsli ,              &
                             itske  ,                                        &
                             itsr11 , itsr12 , itsr13 ,                      &
                             itsr22 , itsr23 , itsr33 ,                      &
                             itste  , itsti  ,                               &
                             itsmas , itsmv1(ncharm2), itsmv2(ncharm2) ,     &
                             itsco  , itsfp4

  !     DONNEES POUR LES TERMES SOURCES
  !     -------------------------------

  !     NSTITS : debut calcul terme source stationnaire
  !     NPTS   : nombre de pas de temps pour le cumul des termes sources
  !     NTXERR : nombre de cellules qui un taux vol > 0.8
  !     VMAX   : taux volumique max atteint
  !     TMAMAX : taux massique max atteint

  integer, save ::           nstits , npts , ntxerr

  double precision, save ::  vmax , tmamax

  !=============================================================================
  ! 10. Clonage/fusion des particules

  !     INDICATEUR D ACTIVATION DE LA ROULETTE RUSSE

  integer, save ::           iroule

  !=============================================================================
  ! 11. Encrassement

  !     DONNEES POUR L ENCRASSEMENT

  integer, save ::           iencra , npencr

  double precision, save ::  enc1(ncharm2) , enc2(ncharm2) ,                 &
                             tprenc(ncharm2) , visref(ncharm2) , dnpenc

  !=============================================================================
  ! 12. Forces chimiques

  !       1) FORCES DE VAN DER WAALS
  !       2) FORCES ELECTROSTATIQUES

  integer, save ::           ladlvo

  !      CSTHAM : constante d'Hamaker
  !      CSTFAR : constant de FARADET
  !      EPSEAU : Constante dielectrique de l'eau
  !      EPSEAU : Constante dielectrique du vide
  !      PHI1   : potentiel solide 1
  !      PHI1   : potentiel solide 2
  !      FION   : force ionique
  !      GAMASV : energie de surface
  !      DPARMN : distance entre particule/paroi minimum

  double precision, save ::  cstham , epseau  , epsvid , phi1 , phi2
  double precision, save ::  fion   , gamasv  , dcoup  , sigch
  double precision, save ::  cstfar , dparmn

  !=============================================================================
  ! 13. Mouvement brownien

  !     ACTIVATION DU MOUVEMENT BROWNIEN :

  integer, save :: lamvbr

  double precision kboltz
  parameter          (kboltz = 1.38d-23)

  !=============================================================================
  ! 14. Schema en temps, dispersion turbulente et equation de poisson

  !     NOR    : numero du sous-pas Lagrangien (1 ou 2)

  !     NORDRE : ordre de la methode d integration (1 ou 2)

  !     MODCPL : = 0 pour le modele incomplet
  !              > 0 pour le modele complet, est egal au nombre de
  !                 passages avant mise en route du modele complet

  !     IDIRLA : = 1 ou 2 ou 3 direction du modele complet

  !     IDISTU : = 0 pas de prise en compte de la dispersion turbulente (la
  !                  vitesse instantanee est egale a la vitesse moyenne)
  !              > 0 prise en compte de la dispersion turbulente (si k-eps
  !                  ou Rij-eps)

  !     IDIFFL : =1 la dispersion turbulente de la particule est celle de
  !                 la particule fluide (=0 sinon)

  !     ILAPOI : = 0 Pas de correction de pression
  !              = 1 Correction de pression

  integer, save ::           nor , nordre , modcpl , idirla ,                &
                             idistu , idiffl , ilapoi

  !=============================================================================
  ! 15. Traitement des statistiques interactions particules/frontieres

  !     DONNEES POUR LES STATISTIQUES AUX FRONTIERES
  !     --------------------------------------------

  !      NUSBOR : NOMBRE DE VARIABLES A ENREGISTRER SUR LES FRONTIERES
  !               SUPPLEMENTAIRES UTILISATEUR (CONTENU DANS NVISBR)
  !      NSTBOR : debut calcul stationnaire
  !      NPSTF  : nombre de pas de temps pour le cumul des stats
  !      NPSTF  : nombre de pas de temps total des stats depuis le debut
  !               du calcul, partie instationnaire comprise
  !      TSTATP : Temps physique des stats aux frontieres stationnaires
  !      SEUILF : Seuil en POIDS STAT de particules pour les stats
  !      IMOYBR : Type de moyenne applicable pour affichage et
  !               post-procesing

  integer, save ::           nusbor , nstbor ,                               &
                             npstf  , npstft ,                               &
                             inbrbd , iflmbd , iangbd , ivitbd , iencbd ,    &
                             inbr   , iflm   , iang   , ivit   , ienc   ,    &
                             iusb(nusbrd)    , imoybr(nusbrd+10)

  double precision, save ::  tstatp , seuilf

  !     NOMS DES VARIABLES STATISTIQUES
  !     -------------------------------
  !     Taille limitee par le fait qu on utilise NOMBRD dans
  !       l ecriture des fichiers suites (lagout)

  character*50, save ::      nombrd(nvplmx)

  ! IIFRLA Pointeur dans IA sur IFRLAG pour reperage des zones
  !          frontieres associees aux faces de bord

  integer, save ::           iifrla

  !=============================================================================
  ! 16. Visu

  !... NBVIS  : nombre de particules a visualiser a l instant t
  !    LISTE  : numero des particules a visualiser
  !    LIST0  : sauvegarde de LISTE pour post-processing trajectoires
  !    NPLIST : nombre d enregistrement par particule
  !    NVISLA : periode d aquisition

  integer, save ::           nbvis, liste(nliste), list0(nliste),            &
                             nplist(nliste), nvisla

  !... Type de visualisation :
  !    IENSI1 : trajectoires
  !    IENSI2 : deplacements
  !    IENSI3 : interaction particules/frontieres

  integer, save ::           iensi1 , iensi2 , iensi3

  !... Contenu des flichiers resultats


  integer, save ::           ivisv1 , ivisv2 , ivistp ,                      &
                             ivisdm , iviste , ivismp ,                      &
                             ivishp , ivisch , ivisck , ivisdk

  !... visualisation de type deplacement
  !    ITLAG : nombre d enregistrement
  !    TIMLAG : temps physiques lagrangien pour la visualisation


  integer, save ::           itlag
  double precision, save ::  timlag(9999)

  !=============================================================================

end module lagran
