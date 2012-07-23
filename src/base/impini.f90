!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2012 EDF S.A.
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

subroutine impini
!================


!===============================================================================
! Purpose:
!  ---------

! Print computation parameters after user changes in cs_user_parameters.f90

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use cstnum
use dimens
use numvar
use optcal
use cstphy
use entsor
use albase
use parall
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use radiat
use lagpar
use lagdim
use lagran
use mltgrd
use mesh

!===============================================================================

implicit none

! Arguments


! Local variables

character        name*300, chaine*80
integer          iok20 , iok21 , iok30 , iok31 , iok50 , iok51 , iok60
integer          iok32
integer          iok70
integer          ii    , jj    , ivar  , iiesca, iest
integer          ipp   , iwar  , imom
integer          nbccou, nbsucp, nbvocp, issurf, isvol

!===============================================================================


!===============================================================================
! 1. Introduction
!===============================================================================

write(nfecra,1000)

if (ippmod(icod3p).ne.-1) then
  write(nfecra,1010)
  write(nfecra,1020) ippmod(icod3p)
  write(nfecra,1060) indjon
else if (ippmod(icoebu).ne.-1) then
  write(nfecra,1010)
  write(nfecra,1030) ippmod(icoebu), cebu
  write(nfecra,1060) indjon
else if (ippmod(icp3pl).ne.-1) then
  write(nfecra,1010)
  write(nfecra,1040) ippmod(icp3pl)
else if (ippmod(icfuel).ne.-1) then
  write(nfecra,1010)
  write(nfecra,1050) ippmod(icfuel)
endif



#if defined(_CS_LANG_FR)

 1000 format(                                                     &
                                                                /,&
' ===========================================================', /,&
                                                                /,&
'               RESUME DES PARAMETRES DE CALCUL',               /,&
'               ===============================',               /,&
                                                                /,&
' -----------------------------------------------------------', /)

 9900 format(                                                     &
                                                                /,&
' -----------------------------------------------------------', /)
 1010 format(                                                     &
                                                                /,&
' ** PHYSIQUE PARTICULIERE :',                                  /,&
'    ---------------------',                                    /)
 1020 format(                                                     &
' --- Flamme de diffusion : Chimie 3 points',                   /,&
'       OPTION = ',4x,i10                                       /)
 1030 format(                                                     &
' --- Flamme premelangee : Modele EBU',                         /,&
'       OPTION = ',4x,i10,                                      /,&
'       CEBU   = ',e14.5                                        /)
 1040 format(                                                     &
' --- Charbon pulverise : Modele Combustible moyen local',      /,&
'       OPTION = ',4x,i10                                       /)
 1050 format(                                                     &
' --- Fuel              : Modele Combustible moyen local',       /&
'       OPTION = ',4x,i10                                       /)
 1060 format(                                                     &
' --- Janaf ou non (dans ce cas tabulation utilisateur)',       /,&
'       INDJON = ',4x,i10,    ' (1: Janaf, 0: utilisateur)',    /)

#else

 1000 format(                                                     &
                                                                /,&
' ===========================================================', /,&
                                                                /,&
'               CALCULATION PARAMETERS SUMMARY',                /,&
'               ==============================',                /,&
                                                                /,&
' -----------------------------------------------------------', /)

 9900 format(                                                     &
                                                                /,&
' -----------------------------------------------------------', /)
 1010 format(                                                     &
                                                                /,&
' ** SPECIFIC PHYSICS:',                                        /,&
'    ----------------',                                         /)
 1020 format(                                                     &
' --- Diffusion Flame: 3 Point Chemistry',                      /,&
'       OPTION = ',4x,i10                                       /)
 1030 format(                                                     &
' --- Premixed Flame: EBU Model',                               /,&
'       OPTION = ',4x,i10,                                      /,&
'       CEBU   = ',e14.5                                        /)
 1040 format(                                                     &
' --- Pulverized Coal: Local Mean Combustible Model',           /,&
'       OPTION = ',4x,i10                                       /)
 1050 format(                                                     &
' --- Fuel:            Local Mean Combustible Model',           /,&
'       OPTION = ',4x,i10                                       /)
 1060 format(                                                     &
' --- Janaf or not (user tabulation required in this case)',    /,&
'       INDJON = ',4x,i10,    ' (1: Janaf, 0: user)',           /)

#endif

!===============================================================================
! 2. DEFINITION GENERALE DU CAS
!===============================================================================

! --- Dimensions

write(nfecra,1500)
write(nfecra,1510) nprfml,nfml
write(nfecra,1520) nvar,nscal,nscaus,nscapp,                      &
                   nproce,nprofa,nprofb

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 1500 format(                                                     &
                                                                /,&
' ** DIMENSIONS',                                               /,&
'    ----------',                                               /)
 1510 format(                                                     &
' --- Geometrie',                                               /,&
'       NPRFML = ',4x,i10,    ' (Nb de proprietes de famille )',/,&
'       NFML   = ',4x,i10,    ' (Nb de familles              )',/)
 1520 format(                                                     &
' --- Physique',                                                /,&
'       NVAR   = ',4x,i10,    ' (Nb de variables             )',/,&
'       NSCAL  = ',4x,i10,    ' (Nb de scalaires             )',/,&
'       NSCAUS = ',4x,i10,    ' (Nb de scalaires utilisateur )',/,&
'       NSCAPP = ',4x,i10,    ' (Nb de scalaires phys. part. )',/,&
'       NPROCE = ',4x,i10,    ' (Nb de proprietes (cellules) )',/,&
'       NPROFA = ',4x,i10,    ' (Nb de proprietes (faces int))',/,&
'       NPROFB = ',4x,i10,    ' (Nb de proprietes (faces brd))',/)

#else

 1500 format(                                                     &
                                                                /,&
' ** DIMENSIONS',                                               /,&
'    ----------',                                               /)
 1510 format(                                                     &
' --- Geometry',                                                /,&
'       NPRFML = ',4x,i10,    ' (Nb max. family properties   )',/,&
'       NFML   = ',4x,i10,    ' (Nb families                 )',/)
 1520 format(                                                     &
' --- Physics',                                                 /,&
'       NVAR   = ',4x,i10,    ' (Nb variables                )',/,&
'       NSCAL  = ',4x,i10,    ' (Nb scalars                  )',/,&
'       NSCAUS = ',4x,i10,    ' (Nb user scalars             )',/,&
'       NSCAPP = ',4x,i10,    ' (Nb specific physics scalars )',/,&
'       NPROCE = ',4x,i10,    ' (Nb cell properties          )',/,&
'       NPROFA = ',4x,i10,    ' (Nb internal face properties )',/,&
'       NPROFB = ',4x,i10,    ' (Nb boundary face properties )',/)

#endif

!===============================================================================
! 3. MODELISATION PHYSIQUE
!===============================================================================

! --- Proprietes physiques

write(nfecra,2000)
write(nfecra,2010) gx,gy,gz
write(nfecra,2011) omegax, omegay, omegaz, icorio

write(nfecra,2020) ro0, viscl0, cp0, icp, p0, pred0, t0,  &
                   irovar,ivivar, (xyzp0(ii),ii=1,3)

if (ippmod(iphpar).ge.1) write(nfecra,2030) diftl0


write(nfecra,9900)


#if defined(_CS_LANG_FR)

 2000 format(                                                     &
                                                                /,&
' ** PROPRIETES PHYSIQUES',                                     /,&
'    --------------------',                                     /)
 2010 format(                                                     &
'       GX     = ', e14.5,    ' (Composante x de la gravite  )',/,&
'       GY     = ', e14.5,    ' (Composante y de la gravite  )',/,&
'       GZ     = ', e14.5,    ' (Composante z de la gravite  )',/)
 2011 format(                                                     &
'       OMEGAX = ', e14.5,    ' (Composante x du vecteur rot.)',/,&
'       OMEGAY = ', e14.5,    ' (Composante y du vecteur rot.)',/,&
'       OMEGAZ = ', e14.5,    ' (Composante z du vecteur rot.)',/,&
'       ICORIO = ', i10,      ' (Termes source de Coriolis   )',/)
 2020 format(                                                     &
'  -- Phase continue :',                                        /,&
                                                                /,&
'       RO0    = ', e14.5,    ' (Masse volumique     de ref. )',/,&
'       VISCL0 = ', e14.5,    ' (Visc. molec. dynam. de ref. )',/,&
'       CP0    = ', e14.5,    ' (Chal. Spec.     de reference)',/,&
'       ICP    = ',4x,i10,    ' (> 0 : CP variable   (usphyv))',/,&
'       P0     = ', e14.5,    ' (Pression totale de reference)',/,&
'       PRED0  = ', e14.5,    ' (Press. reduite  de reference)',/,&
'       T0     = ', e14.5,    ' (Temperature     de reference)',/,&
                                                                /,&
'       IROVAR = ',4x,i10,    ' (Masse vol.  cst (0) ou non(1)',/,&
'       IVIVAR = ',4x,i10,    ' (Visc molec. cst (0) ou non(1)',/,&
/,                                                          &
'       Point de reference initial pour la pression',           /,&
'       XYZP0  = ', e14.5, e14.5, e14.5                          )
 2030 format(                                                     &
'       DIFTL0 = ', e14.5,    ' (Diff. dynam.    de reference)',/)

#else

 2000 format(                                                     &
                                                                /,&
' ** PHYSICAL PROPERTIES',                                      /,&
'    -------------------',                                      /)
 2010 format(                                                     &
'       GX     = ', e14.5,    ' (Gravity x component         )',/,&
'       GY     = ', e14.5,    ' (Gravity y component         )',/,&
'       GZ     = ', e14.5,    ' (Gravity z component         )',/)
 2011 format(                                                     &
'       OMEGAX = ', e14.5,    ' (Rotation vector x component )',/,&
'       OMEGAY = ', e14.5,    ' (Rotation vector y component )',/,&
'       OMEGAZ = ', e14.5,    ' (Rotation vector z component )',/,&
'       ICORIO = ', i10,      ' (Coriolis source terms       )',/)
 2020 format(                                                     &
'  -- Continuous phase:',                                       /,&
                                                                /,&
'       RO0    = ', e14.5,    ' (Reference density           )',/,&
'       VISCL0 = ', e14.5,    ' (Ref. molecular dyn. visc.   )',/,&
'       CP0    = ', e14.5,    ' (Ref. specific heat          )',/,&
'       ICP    = ',4x,i10,    ' (> 0: variable CP (usphyv)   )',/,&
'       P0     = ', e14.5,    ' (Ref. total pressure         )',/,&
'       PRED0  = ', e14.5,    ' (Ref. reduced pressure       )',/,&
'       T0     = ', e14.5,    ' (Ref. temperature            )',/,&
                                                                /,&
'       IROVAR = ',4x,i10,    ' (Density constant(0) or not(1)',/,&
'       IVIVAR = ',4x,i10,    ' (Molec. visc cst.(0) or not(1)',/,&
/,                                                                &
'       Initial reference point for pressure',                  /,&
'       XYZP0  = ', e14.5, e14.5, e14.5                          )
 2030 format(                                                     &
'       DIFTL0 = ', e14.5,    ' (Ref. dynamic diffusivity    )',/)

#endif

! --- Turbulence

write(nfecra,2510)

!   - Modeles

write(nfecra,2515)                                              &
     iturb,ideuch,ypluli,ilogpo,                                &
     igrhok,iscalt
if(iturb.eq.10) then
  write(nfecra,2516)                                            &
       xlomlg
elseif(iturb.eq.20) then
  write(nfecra,2517)                                            &
       almax, uref,                                             &
       iclkep,ikecou,igrake
  if (ikecou.eq.0 .and. idtvar.ge.0) then
    write(nfecra,2527) relaxv(ik),relaxv(iep)
  else
    write(nfecra,2540)
  endif
elseif(iturb.eq.21) then
  write(nfecra,2518) almax, uref, iclkep,ikecou,igrake
  if (ikecou.eq.0.and. idtvar.ge.0) then
    write(nfecra,2527) relaxv(ik),relaxv(iep)
  else
    write(nfecra,2540)
  endif
elseif(iturb.eq.30) then
  write(nfecra,2519)                                            &
       almax, uref,                                             &
       irijnu,irijrb,irijec,                                    &
       idifre,igrari,iclsyr,iclptr
elseif(iturb.eq.31) then
  write(nfecra,2520) almax, uref, irijnu,irijrb, igrari,iclsyr,iclptr
elseif(iturb.eq.32) then
  write(nfecra,2525)                                            &
    almax, uref,                                                &
    irijnu,irijrb,                                              &
    igrari,iclsyr,iclptr
elseif(itytur.eq.4) then
  write(nfecra,2521)                                            &
       csmago,cwale, xlesfl,ales,                               &
       bles,idries,cdries, xlesfd,                              &
       smagmx, ivrtex
elseif(iturb.eq.50) then
  write(nfecra,2522) almax, uref, iclkep,ikecou,igrake
  if (ikecou.eq.0 .and. idtvar.ge.0) then
    write(nfecra,2527) relaxv(ik),relaxv(iep)
  else
    write(nfecra,2540)
  endif
elseif(iturb.eq.51) then
  write(nfecra,2524) almax, uref, iclkep,ikecou,igrake
  if (ikecou.eq.0 .and. idtvar.ge.0) then
    write(nfecra,2527) relaxv(ik),relaxv(iep)
  else
    write(nfecra,2529)
  endif
elseif(iturb.eq.60) then
  write(nfecra,2523) almax, uref, ikecou,igrake
  if (ikecou.eq.0 .and. idtvar.ge.0) then
    write(nfecra,2528) relaxv(ik),relaxv(iomg)
  else
    write(nfecra,2540)
  endif
elseif(iturb.eq.70) then
  write(nfecra,2529) almax,  uref,  relaxv(inusa)
endif

!   - Constantes

write(nfecra,2530)xkappa,cstlog,apow,bpow

iok20 = 0
iok21 = 0
iok30 = 0
iok31 = 0
iok32 = 0
iok50 = 0
iok51 = 0
iok60 = 0
iok70 = 0
if(iturb.eq.20) then
  iok20 = 20
endif
if(iturb.eq.21) then
  iok21 = 21
endif
if(iturb.eq.30) then
  iok30 = 30
endif
if(iturb.eq.31) then
  iok31 = 31
endif
if(iturb.eq.32) then
  iok32 = 32
endif
if(iturb.eq.50) then
  iok50 = 50
endif
if(iturb.eq.51) then
  iok51 = 51
endif
if(iturb.eq.60) then
  iok60 = 60
endif
if(iturb.eq.70) then
  iok70 = 70
endif
if(iok20.gt.0) then
  write(nfecra,2531)ce1,ce2,sigmak,sigmae,cmu
endif
if (iok21.gt.0) then
  write(nfecra,2532)ce1,ce2,sigmak,sigmae,cmu
endif
if (iok30.gt.0) then
  write(nfecra,2533)ce1,ce2,crij1,crij2,crij3,crijep,csrij,       &
                    crijp1,crijp2,cmu
endif
if (iok31.gt.0) then
  write(nfecra,2534)cssgs1,cssgs2,cssgr1,cssgr2,cssgr3,cssgr4,    &
       cssgr5,csrij,crij3,ce1,cssge2,sigmae,cmu
endif
if (iok32.gt.0) then
  write(nfecra,2539)cebms1,cebmr1,cebmr2,cebmr3,cebmr4,cebmr5,    &
                    csebm,cebmr6,cebme2,ce1,sigebm,xa1,sigmak,    &
                    xceta,xct
endif
if (iok50.gt.0) then
  write(nfecra,2535) cv2fa1,cv2fe2,sigmak,sigmae,cv2fmu,cv2fct,   &
       cv2fcl,cv2fet,cv2fc1,cv2fc2
endif
if(iok51.gt.0) then
  write(nfecra,2538) cpale1,cpale2,cpale3,cpale4,sigmak,cpalse,cpalmu,cpalct, &
       cpalcl,cpalet,cpalc1,cpalc2
endif
if (iok60.gt.0) then
  write(nfecra,2536) ckwsk1,ckwsk2,ckwsw1,ckwsw2,ckwbt1,ckwbt2,   &
       ckwgm1,ckwgm2,ckwa1,ckwc1,cmu
endif
if(iok70.gt.0) then
  write(nfecra,2537) csab1,csab2,csasig,csav1,csaw1,csaw2,csaw3
endif

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 2510 format(                                                     &
                                                                /,&
' ** TURBULENCE',                                               /,&
'    ----------',                                               /)
 2515 format(                                                     &
' --- Phase continue :',                                        /,&
                                                                /,&
'   - Communs',                                                 /,&
'       ITURB  = ',4x,i10,    ' (Modele de turbulence        )',/,&
'       IDEUCH = ',4x,i10,    ' (0: modele a une echelle     )',/,&
'                               (1: modele a deux echelles   )',/,&
'                               (2: loi de paroi invariante  )',/,&
'       YPLULI = ', e14.5,    ' (Y plus limite               )',/,&
'       ILOGPO = ',4x,i10,    ' (0: loi puissance (interdite',  /,&
'                                              en k-epsilon) )',/,&
'                               (1: loi log une echelle      )',/,&
'       IGRHOK = ',4x,i10,    ' (1: Grad (rho k ) calcule    )',/,&
'       ISCALT = ',4x,i10,    ' (Numero du scalaire temp     )',/)
 2516 format(                                                     &
'   - Longueur de melange (ITURB = 10)',                        /,&
'       XLOMLG = ', e14.5,    ' (Longueur caracteristique    )',/)
 2517 format(                                                     &
'   - k-epsilon           (ITURB = 20)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       ICLKEP = ',4x,i10,    ' (Mode de clipping k-epsilon  )',/,&
'       IKECOU = ',4x,i10,    ' (Mode de couplage k-epsilon  )',/,&
'       IGRAKE = ',4x,i10,    ' (Prise en compte de gravite  )')
 2518 format(                                                     &
'   - k-epsilon production lineaire (ITURB = 21)',              /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       ICLKEP = ',4x,i10,    ' (Mode de clipping k-epsilon  )',/,&
'       IKECOU = ',4x,i10,    ' (Mode de couplage k-epsilon  )',/,&
'       IGRAKE = ',4x,i10,    ' (Prise en compte de gravite  )')
 2519 format(                                                     &
'   - Rij-epsilon         (ITURB = 30)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       IRIJNU = ',4x,i10,    ' (Stabilisation matricielle   )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruction aux bords    )',/,&
'       IRIJEC = ',4x,i10,    ' (Termes d echo de paroi      )',/,&
'       IDIFRE = ',4x,i10,    ' (Traitmnt du tenseur de diff.)',/,&
'       IGRARI = ',4x,i10,    ' (Prise en compte de gravite  )',/,&
'       ICLSYR = ',4x,i10,    ' (Implicitation en symetrie   )',/,&
'       ICLPTR = ',4x,i10,    ' (Implicitation en paroi      )',/)
 2520 format(                                                     &
'   - Rij-epsilon SSG     (ITURB = 31)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       IRIJNU = ',4x,i10,    ' (Stabilisation matricielle   )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruction aux bords    )',/,&
'       IGRARI = ',4x,i10,    ' (Prise en compte de gravite  )',/,&
'       ICLSYR = ',4x,i10,    ' (Implicitation en symetrie   )',/,&
'       ICLPTR = ',4x,i10,    ' (Implicitation en paroi      )',/)
 2521 format(                                                     &
'   - LES                 (ITURB = 40, 41, 42)',                /,&
'                               (Modele de sous-maille       )',/,&
'                               (40 Modele de Smagorinsky    )',/,&
'                               (41 Modele dynamique         )',/,&
'                               (42 Modele WALE              )',/,&
'       CSMAGO = ', e14.5,    ' (Constante de Smagorinski    )',/,&
'       CWALE  = ', e14.5,    ' (Constante du modele WALE    )',/,&
'       XLESFL = ', e14.5,    ' (La largeur du filtre en une )',/,&
'       ALES   = ', e14.5,    ' (cellule s''ecrit            )',/,&
'       BLES   = ', e14.5,    ' (XLESFL*(ALES*VOLUME)**(BLES))',/,&
'       IDRIES = ',4x,i10,    ' (=1 Amortissement Van Driest )',/,&
'       CDRIES = ', e14.5,    ' (Constante de Van Driest     )',/,&
'       XLESFD = ', e14.5,    ' (Rapport entre le filtre     )',/,&
'                               (explicite et le filtre LES  )',/,&
'                               (valeur conseillee 1.5       )',/,&
'       SMAGMX = ', e14.5,    ' (Smagorinsky max dans le cas )',/,&
'                               (du modele dynamique         )',/,&
'       IVRTEX = ',4x,i10,    ' (Utilisation de la methode   )',/,&
'                               (des vortex                  )')
 2522 format(                                                     &
'   - v2f phi-model       (ITURB = 50)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       ICLKEP = ',4x,i10,    ' (Mode de clipping k-epsilon  )',/,&
'       IKECOU = ',4x,i10,    ' (Mode de couplage k-epsilon  )',/,&
'       IGRAKE = ',4x,i10,    ' (Prise en compte de gravite  )')
 2524 format(                                                     &
'   - v2f BL-v2/k         (ITURB = 51)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       ICLKEP = ',4x,i10,    ' (Mode de clipping k-epsilon  )',/,&
'       IKECOU = ',4x,i10,    ' (Mode de couplage k-epsilon  )',/,&
'       IGRAKE = ',4x,i10,    ' (Prise en compte de gravite  )')
 2523 format(                                                     &
'   - k-omega SST         (ITURB = 60)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       IKECOU = ',4x,i10,    ' (Mode de couplage k-omega    )',/,&
'       IGRAKE = ',4x,i10,    ' (Prise en compte de gravite  )')
 2525 format(                                                     &
'   - Rij-epsilon EBRSM     (ITURB = 32)',                      /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       IRIJNU = ',4x,i10,    ' (Stabilisation matricielle   )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruction aux bords    )',/,&
'       IGRARI = ',4x,i10,    ' (Prise en compte de gravite  )',/,&
'       ICLSYR = ',4x,i10,    ' (Implicitation en symetrie   )',/,&
'       ICLPTR = ',4x,i10,    ' (Implicitation en paroi      )',/)
 2527 format(                                                     &
'       RELAXV = ', e14.5,    ' pour k       (Relaxation)',     /,&
'       RELAXV = ', e14.5,    ' pour epsilon (Relaxation)',     /)
 2528 format(                                                     &
'       RELAXV = ', e14.5,    ' pour k     (Relaxation)',       /,&
'       RELAXV = ', e14.5,    ' pour omega (Relaxation)',       /)
 2529 format(                                                     &
'   - Spalart-Allmares    (ITURB = 70)',                        /,&
'       ALMAX  = ', e14.5,    ' (Longueur caracteristique    )',/,&
'       UREF   = ', e14.5,    ' (Vitesse  caracteristique    )',/,&
'       RELAXV = ', e14.5,    ' pour nu (Relaxation)',          /)

 2530 format(                                                     &
' --- Constantes',                                              /,&
                                                                /,&
'   - Communs',                                                 /,&
'       XKAPPA = ', e14.5,    ' (Constante de Von Karman     )',/,&
'       CSTLOG = ', e14.5,    ' (U+=Log(y+)/kappa +CSTLOG    )',/,&
'       APOW   = ', e14.5,    ' (U+=APOW (y+)**BPOW (W&W law))',/,&
'       BPOW   = ', e14.5,    ' (U+=APOW (y+)**BPOW (W&W law))',/)
 2531 format(                                                     &
'   - k-epsilon           (ITURB = 20)',                        /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1 : coef de Prod.  )',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2 : coef de Diss.  )',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relatif a k         )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relatif a epsilon   )',/,&
'       CMU    = ', e14.5,    ' (Constante Cmu               )',/)
 2532 format(                                                     &
'   - k-epsilon production lineaire (ITURB = 21)',              /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1 : coef de Prod.  )',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2 : coef de Diss.  )',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relatif a k         )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relatif a epsilon   )',/,&
'       CMU    = ', e14.5,    ' (Constante Cmu               )',/)
 2533 format(                                                     &
'   - Rij-epsilon std     (ITURB = 30)',                        /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1 : coef de Prod.  )',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2 : coef de Diss.  )',/,&
'       CRIJ1  = ', e14.5,    ' (Coef terme lent             )',/,&
'       CRIJ2  = ', e14.5,    ' (Coef terme rapide           )',/,&
'       CRIJ3  = ', e14.5,    ' (Coef terme de gravite       )',/,&
'       CRIJEP = ', e14.5,    ' (Coef diffusion epsilon      )',/,&
'       CSRIJ  = ', e14.5,    ' (Coef diffusion Rij          )',/,&
'       CRIJP1 = ', e14.5,    ' (Coef lent pour echo de paroi)',/,&
'       CRIJP2 = ', e14.5,    ' (Coef rapide    echo de paroi)',/,&
'       CMU    = ', e14.5,    ' (Constante Cmu               )',/)
 2534 format(                                                     &
'   - Rij-epsilon SSG     (ITURB = 31)',                        /,&
'       CSSGS1 = ', e14.5,    ' (Coef Cs1                    )',/,&
'       CSSGS2 = ', e14.5,    ' (Coef Cs2                    )',/,&
'       CSSGR1 = ', e14.5,    ' (Coef Cr1                    )',/,&
'       CSSGR2 = ', e14.5,    ' (Coef Cr2                    )',/,&
'       CSSGR3 = ', e14.5,    ' (Coef Cr3                    )',/,&
'       CSSGR4 = ', e14.5,    ' (Coef Cr4                    )',/,&
'       CSSGR5 = ', e14.5,    ' (Coef Cr5                    )',/,&
'       CRIJS  = ', e14.5,    ' (Coef Cs diffusion de Rij    )',/,&
'       CRIJ3  = ', e14.5,    ' (Coef terme de gravite       )',/,&
'       Ce1    = ', e14.5,    ' (Coef Ceps1                  )',/,&
'       CSSGE2 = ', e14.5,    ' (Coef Ceps2                  )',/,&
'       SIGMAE = ', e14.5,    ' (Coef sigma_eps              )',/,&
'       CMU    = ', e14.5,    ' (Constante Cmu               )',/)
 2535 format(                                                     &
'   - v2f phi-model       (ITURB = 50)',                        /,&
'       CV2FA1 = ', e14.5,    ' (a1 pour calculer Cepsilon1  )',/,&
'       CV2FE2 = ', e14.5,    ' (Cepsilon 2 : coef de Diss.  )',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relatif a k         )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relatif a epsilon   )',/,&
'       CV2FMU = ', e14.5,    ' (Constante Cmu               )',/,&
'       CV2FCT = ', e14.5,    ' (Constante CT                )',/,&
'       CV2FCL = ', e14.5,    ' (Constante CL                )',/,&
'       CV2FET = ', e14.5,    ' (Constante C_eta             )',/,&
'       CV2FC1 = ', e14.5,    ' (Constante C1                )',/,&
'       CV2FC2 = ', e14.5,    ' (Constante C2                )',/)
 2536 format(                                                     &
'   - k-omega SST         (ITURB = 60)',                        /,&
'       CKWSK1 = ', e14.5,    ' (Constante sigma_k1          )',/,&
'       CKWSK2 = ', e14.5,    ' (Constante sigma_k2          )',/,&
'       CKWSW1 = ', e14.5,    ' (Constante sigma_omega1      )',/,&
'       CKWSW2 = ', e14.5,    ' (Constante sigma_omega2      )',/,&
'       CKWBT1 = ', e14.5,    ' (Constante beta1             )',/,&
'       CKWBT2 = ', e14.5,    ' (Constante beta2             )',/,&
'       CKWGM1 = ', e14.5,    ' (Constante gamma1            )',/,&
'       CKWGM2 = ', e14.5,    ' (Constante gamma2            )',/,&
'       CKWA1  = ', e14.5,    ' (Cste a1 pour calculer mu_t  )',/,&
'       CKWC1  = ', e14.5,    ' (Cste c1 pour limiteur prod  )',/,&
'       CMU    = ', e14.5,    ' (Cste Cmu (ou Beta*) pour    )',/,&
'                                    conversion omega/epsilon)',/)
 2537 format( &
'   - Spalart-Allmaras    (ITURB = 70)',                        /,&
'       CSAB1  = ', e14.5,    ' (Constante b1                )',/,&
'       CSAB2  = ', e14.5,    ' (Constante b2                )',/,&
'       CSASIG = ', e14.5,    ' (Constante sigma             )',/,&
'       CSAV1  = ', e14.5,    ' (Constante v1                )',/,&
'       CSAW1  = ', e14.5,    ' (Constante w1                )',/,&
'       CSAW2  = ', e14.5,    ' (Constante w2                )',/,&
'       CSAW3  = ', e14.5,    ' (Constante w3                )',/)
 2538 format( &
'   - v2f BL-v2/k         (ITURB = 51)',                        /,&
'       CPALe1 = ', e14.5,    ' (Cepsilon 1 : coef de Prod.  )',/,&
'       CPALE2 = ', e14.5,    ' (Cepsilon 2 : coef de Diss.  )',/,&
'       CPALE3 = ', e14.5,    ' (Cepsilon 3 : coef terme E   )',/,&
'       CPALE4 = ', e14.5,    ' (Cepsilon 4 : coef Diss. mod.)',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relatif a k         )',/,&
'       CPALSE = ', e14.5,    ' (Prandtl relatif a epsilon   )',/,&
'       CPALMU = ', e14.5,    ' (Constante Cmu               )',/,&
'       CPALCT = ', e14.5,    ' (Constante CT                )',/,&
'       CPALCL = ', e14.5,    ' (Constante CL                )',/,&
'       CPALET = ', e14.5,    ' (Constante C_eta             )',/,&
'       CPALC1 = ', e14.5,    ' (Constante C1                )',/,&
'       CPALC2 = ', e14.5,    ' (Constante C2                )',/)
 2539 format( &
'   - Rij-epsilon EBRSM     (ITURB = 32)',                      /,&
'       CEBMS1 = ', e14.5,    ' (Coef Cs1                    )',/,&
'       CEBMR1 = ', e14.5,    ' (Coef Cr1                    )',/,&
'       CEBMR2 = ', e14.5,    ' (Coef Cr2                    )',/,&
'       CEBMR3 = ', e14.5,    ' (Coef Cr3                    )',/,&
'       CEBMR4 = ', e14.5,    ' (Coef Cr4                    )',/,&
'       CEBMR5 = ', e14.5,    ' (Coef Cr5                    )',/,&
'       CSEBM  = ', e14.5,    ' (Coef Cs diffusion de Rij    )',/,&
'       CEBMR6 = ', e14.5,    ' (Coef terme de gravite       )',/,&
'       CEBME2 = ', e14.5,    ' (Coef Ceps2                  )',/,&
'       Ce1    = ', e14.5,    ' (Coef Ceps1                  )',/,&
'       SIGEBM = ', e14.5,    ' (Coef sigma_eps              )',/,&
'       XA1    = ', e14.5,    ' (Coef A1                     )',/,&
'       SIGMAK = ', e14.5,    ' (Coef sigma_k                )',/,&
'       XCETA  = ', e14.5,    ' (Coef Ceta                   )',/,&
'       XCT    = ', e14.5,    ' (Coef CT                     )',/)

 2540 format(/)

#else

 2510 format(                                                     &
                                                                /,&
' ** TURBULENCE',                                               /,&
'    ----------',                                               /)
 2515 format(                                                     &
' --- Continuous phase:',                                       /,&
                                                                /,&
'   - Commons',                                                 /,&
'       ITURB  = ',4x,i10,    ' (Turbulence model            )',/,&
'       IDEUCH = ',4x,i10,    ' (0: one-scale model          )',/,&
'                               (1: two-scale model          )',/,&
'                               (2: invariant wall function  )',/,&
'       YPLULI = ', e14.5,    ' (Limit Y+                    )',/,&
'       ILOGPO = ',4x,i10,    ' (0: power law (forbidden for',  /,&
'                                              k-epsilon)    )',/,&
'                               (1: one-scale log law        )',/,&
'       IGRHOK = ',4x,i10,    ' (1: computed Grad(rho k)     )',/,&
'       ISCALT = ',4x,i10,    ' (Temperature salar number    )',/)
 2516 format(                                                     &
'   - Mixing length       (ITURB = 10)',                        /,&
'       XLOMLG = ', e14.5,    ' (Characteristic length       )',/)
 2517 format(                                                     &
'   - k-epsilon           (ITURB = 20)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       ICLKEP = ',4x,i10,    ' (k-epsilon clipping model    )',/,&
'       IKECOU = ',4x,i10,    ' (k-epsilon coupling mode     )',/,&
'       IGRAKE = ',4x,i10,    ' (Account for gravity         )')
 2518 format(                                                     &
'   - Linear production k-epsilon (ITURB = 21)',                /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       ICLKEP = ',4x,i10,    ' (k-epsilon clipping model    )',/,&
'       IKECOU = ',4x,i10,    ' (k-epsilon coupling mode     )',/,&
'       IGRAKE = ',4x,i10,    ' (Account for gravity         )')
 2519 format(                                                     &
'   - Rij-epsilon         (ITURB = 30)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       IRIJNU = ',4x,i10,    ' (Matrix stabilization        )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruct at boundaries   )',/,&
'       IRIJEC = ',4x,i10,    ' (Wall echo terms             )',/,&
'       IDIFRE = ',4x,i10,    ' (Handle diffusion tensor     )',/,&
'       IGRARI = ',4x,i10,    ' (Prise en compte de gravite  )',/,&
'       ICLSYR = ',4x,i10,    ' (Symmetry implicitation      )',/,&
'       ICLPTR = ',4x,i10,    ' (Wall implicitation          )',/)
 2520 format(                                                     &
'   - SSG Rij-epsilon     (ITURB = 31)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       IRIJNU = ',4x,i10,    ' (Matrix stabilization        )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruct at boundaries   )',/,&
'       IGRARI = ',4x,i10,    ' (Account for gravity         )',/,&
'       ICLSYR = ',4x,i10,    ' (Symmetry implicitation      )',/,&
'       ICLPTR = ',4x,i10,    ' (Wall implicitation          )',/)
 2521 format(                                                     &
'   - LES                 (ITURB = 40, 41, 42)',                /,&
'                               (Sub-grid scale model        )',/,&
'                               (40 Smagorinsky model        )',/,&
'                               (41 Dynamic model            )',/,&
'                               (42 WALE model               )',/,&
'       CSMAGO = ', e14.5,    ' (Smagorinsky constant        )',/,&
'       CWALE  = ', e14.5,    ' (WALE model constant         )',/,&
'       XLESFL = ', e14.5,    ' (Filter with in a cell is    )',/,&
'       ALES   = ', e14.5,    ' (written as                  )',/,&
'       BLES   = ', e14.5,    ' (XLESFL*(ALES*VOLUME)**(BLES))',/,&
'       IDRIES = ',4x,i10,    ' (=1 Van Driest damping       )',/,&
'       CDRIES = ', e14.5,    ' (Van Driest constant         )',/,&
'       XLESFD = ', e14.5,    ' (Ratio between the explicit  )',/,&
'                               (filter and LES filter       )',/,&
'                               (recommended value: 1.5      )',/,&
'       SMAGMX = ', e14.5,    ' (Max Smagonsky in the        )',/,&
'                               (dynamic model case          )',/,&
'       IVRTEX = ',4x,i10,    ' (Use the vortex method       )')
 2522 format(                                                     &
'   - v2f phi-model       (ITURB = 50)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       ICLKEP = ',4x,i10,    ' (k-epsilon clipping model    )',/,&
'       IKECOU = ',4x,i10,    ' (k-epsilon coupling mode     )',/,&
'       IGRAKE = ',4x,i10,    ' (Account for gravity         )')
 2523 format(                                                     &
'   - k-omega SST         (ITURB = 60)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       IKECOU = ',4x,i10,    ' (k-epsilon coupling mode     )',/,&
'       IGRAKE = ',4x,i10,    ' (Account for gravity         )')
 2524 format(                                                     &
'   - v2f BL-v2/k         (ITURB = 51)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       ICLKEP = ',4x,i10,    ' (k-epsilon clipping model    )',/,&
'       IKECOU = ',4x,i10,    ' (k-epsilon coupling mode     )',/,&
'       IGRAKE = ',4x,i10,    ' (Account for gravity         )')

 2525 format(                                                     &
'   - Rij-epsilon EBRSM     (ITURB = 32)',                      /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       IRIJNU = ',4x,i10,    ' (Matrix stabilization        )',/,&
'       IRIJRB = ',4x,i10,    ' (Reconstruct at boundaries   )',/,&
'       IGRARI = ',4x,i10,    ' (Account for gravity         )',/,&
'       ICLSYR = ',4x,i10,    ' (Symmetry implicitation      )',/,&
'       ICLPTR = ',4x,i10,    ' (Wall implicitation          )',/)
 2527 format(                                                     &
'       RELAXV = ', e14.5,    ' for k        (Relaxation)',     /,&
'       RELAXV = ', e14.5,    ' for epsilon  (Relaxation)',     /)
 2528 format(                                                     &
'       RELAXV = ', e14.5,    ' for k      (Relaxation)',       /,&
'       RELAXV = ', e14.5,    ' for omega  (Relaxation)',       /)
 2529 format(                                                     &
'   - Spalart-Allmaras    (ITURB = 70)',                        /,&
'       ALMAX  = ', e14.5,    ' (Characteristic length       )',/,&
'       UREF   = ', e14.5,    ' (Characteristic velocity     )',/,&
'       RELAXV = ', e14.5,    ' for nu (Relaxation)',           /)

 2530 format(                                                     &
' --- Constants',                                               /,&
                                                                /,&
'   - Commons',                                                 /,&
'       XKAPPA = ', e14.5,    ' (Von Karman constant         )',/,&
'       CSTLOG = ', e14.5,    ' (U+=Log(y+)/kappa +CSTLOG    )',/,&
'       APOW   = ', e14.5,    ' (U+=APOW (y+)**BPOW (W&W law))',/,&
'       BPOW   = ', e14.5,    ' (U+=APOW (y+)**BPOW (W&W law))',/)
 2531 format(                                                     &
'   - k-epsilon           (ITURB = 20)',                        /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1: production coef.)',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2: dissipat.  coef.)',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relative to k       )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relative to epsilon )',/,&
'       CMU    = ', e14.5,    ' (Cmu constant                )',/)
 2532 format(                                                     &
'   - Linear production k-epsilon (ITURB = 21)',                /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1: production coef.)',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2: dissipat.  coef.)',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relative to k       )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relative to epsilon )',/,&
'       CMU    = ', e14.5,    ' (Cmu constant                )',/)
 2533 format(                                                     &
'   - Rij-epsilon         (ITURB = 30)',                        /,&
'       Ce1    = ', e14.5,    ' (Cepsilon 1: production coef.)',/,&
'       CE2    = ', e14.5,    ' (Cepsilon 2: dissipat.  coef.)',/,&
'       CRIJ1  = ', e14.5,    ' (Slow term coefficient       )',/,&
'       CRIJ2  = ', e14.5,    ' (Fast term coefficient       )',/,&
'       CRIJ3  = ', e14.5,    ' (Gravity term coefficient    )',/,&
'       CRIJEP = ', e14.5,    ' (Epsilon diffusion coeff.    )',/,&
'       CSRIJ  = ', e14.5,    ' (Rij diffusion coeff.        )',/,&
'       CRIJP1 = ', e14.5,    ' (Slow coeff. for wall echo   )',/,&
'       CRIJP2 = ', e14.5,    ' (Fast coeff. for wall echo   )',/,&
'       CMU    = ', e14.5,    ' (Cmu constant                )',/)
 2534 format(                                                     &
'   - SSG Rij-epsilon     (ITURB = 31)',                        /,&
'       CSSGS1 = ', e14.5,    ' (Cs1 coeff.                  )',/,&
'       CSSGS2 = ', e14.5,    ' (Cs2 coeff.                  )',/,&
'       CSSGR1 = ', e14.5,    ' (Cr1 coeff.                  )',/,&
'       CSSGR2 = ', e14.5,    ' (Cr2 coeff.                  )',/,&
'       CSSGR3 = ', e14.5,    ' (Cr3 coeff.                  )',/,&
'       CSSGR4 = ', e14.5,    ' (Cr4 coeff.                  )',/,&
'       CSSGR5 = ', e14.5,    ' (Cr5 coeff.                  )',/,&
'       CRIJS  = ', e14.5,    ' (Rij Cs diffusion coeff.     )',/,&
'       CRIJ3  = ', e14.5,    ' (Gravity term coeff.         )',/,&
'       Ce1    = ', e14.5,    ' (Ceps1 coeff.                )',/,&
'       CSSGE2 = ', e14.5,    ' (Ceps2 coeff.                )',/,&
'       SIGMAE = ', e14.5,    ' (sigma_eps coeff.            )',/,&
'       CMU    = ', e14.5,    ' (Cmu constant                )',/)
 2535 format(                                                     &
'   - v2f phi-model       (ITURB = 50)',                        /,&
'       CV2FA1 = ', e14.5,    ' (a1 to calculate Cepsilon1   )',/,&
'       CV2FE2 = ', e14.5,    ' (Cepsilon 2: dissip. coeff.  )'/, &
'       SIGMAK = ', e14.5,    ' (Prandtl relative to k       )',/,&
'       SIGMAE = ', e14.5,    ' (Prandtl relative to epsilon )',/,&
'       CV2FMU = ', e14.5,    ' (Cmu constant                )',/,&
'       CV2FCT = ', e14.5,    ' (CT constant                 )',/,&
'       CV2FCL = ', e14.5,    ' (CL constant                 )',/,&
'       CV2FET = ', e14.5,    ' (C_eta constant              )',/,&
'       CV2FC1 = ', e14.5,    ' (C1 constant                 )',/,&
'       CV2FC2 = ', e14.5,    ' (C2 constant                 )',/)
 2536 format(                                                     &
'   - k-omega SST         (ITURB = 60)',                        /,&
'       CKWSK1 = ', e14.5,    ' (sigma_k1 constant           )',/,&
'       CKWSK2 = ', e14.5,    ' (sigma_k2 constant           )',/,&
'       CKWSW1 = ', e14.5,    ' (sigma_omega1 constant       )',/,&
'       CKWSW2 = ', e14.5,    ' (sigma_omega2 constant       )',/,&
'       CKWBT1 = ', e14.5,    ' (beta1 constant              )',/,&
'       CKWBT2 = ', e14.5,    ' (beta2 constant              )',/,&
'       CKWGM1 = ', e14.5,    ' (gamma1 constant             )',/,&
'       CKWGM2 = ', e14.5,    ' (gamma2 constant             )',/,&
'       CKWA1  = ', e14.5,    ' (a1 constant to compute mu_t )',/,&
'       CKWC1  = ', e14.5,    ' (c1 const. for prod. limiter )',/,&
'       CMU    = ', e14.5,    ' (Cmu (or Beta*) constant for )',/,&
'                                    omega/epsilon conversion)',/)
 2537 format(                                                     &
'   - Spalart-Allmaras    (ITURB = 70)',                        /,&
'       CSAB1  = ', e14.5,    ' (b1 constant                 )',/,&
'       CSAB2  = ', e14.5,    ' (b2 constant                 )',/,&
'       CSASIG = ', e14.5,    ' (sigma constant              )',/,&
'       CSAV1  = ', e14.5,    ' (v1 constant                 )',/,&
'       CSAW1  = ', e14.5,    ' (w1 constant                 )',/,&
'       CSAW2  = ', e14.5,    ' (w2 constant                 )',/,&
'       CSAW3  = ', e14.5,    ' (w3 constant                 )',/)
 2538 format( &
'   - v2f BL-v2/k         (ITURB = 51)',                        /,&
'       CPALe1 = ', e14.5,    ' (Cepsilon 1 : Prod. coeff.   )',/,&
'       CPALE2 = ', e14.5,    ' (Cepsilon 2 : Diss. coeff.   )',/,&
'       CPALE3 = ', e14.5,    ' (Cepsilon 3 : E term coeff.  )',/,&
'       CPALE4 = ', e14.5,    ' (Cepsilon 4 : Mod Diss. coef.)',/,&
'       SIGMAK = ', e14.5,    ' (Prandtl relative to k       )',/,&
'       CPALSE = ', e14.5,    ' (Prandtl relative to epsilon )',/,&
'       CPALMU = ', e14.5,    ' (Cmu constant               )',/,&
'       CPALCT = ', e14.5,    ' (CT constant                )',/,&
'       CPALCL = ', e14.5,    ' (CL constant                )',/,&
'       CPALET = ', e14.5,    ' (C_eta constant             )',/,&
'       CPALC1 = ', e14.5,    ' (C1 constant                )',/,&
'       CPALC2 = ', e14.5,    ' (C2 constant                )',/)
 2539  format( &
'   - EBRSM Rij-epsilon     (ITURB = 32)',                      /,&
'       CEBMS1 = ', e14.5,    ' (Cs1 coeff.                  )',/,&
'       CEBMR1 = ', e14.5,    ' (Cr1 coeff.                  )',/,&
'       CEBMR2 = ', e14.5,    ' (Cr2 coeff.                  )',/,&
'       CEBMR3 = ', e14.5,    ' (Cr3 coeff.                  )',/,&
'       CEBMR4 = ', e14.5,    ' (Cr4 coeff.                  )',/,&
'       CEBMR5 = ', e14.5,    ' (Cr5 coeff.                  )',/,&
'       CSEBM  = ', e14.5,    ' (Rij Cs diffusion coeff.     )',/,&
'       CEBMR6 = ', e14.5,    ' (Gravity term coeff.         )',/,&
'       CEBME2 = ', e14.5,    ' (Coef Ceps2                  )',/,&
'       Ce1    = ', e14.5,    ' (Coef Ceps1                  )',/,&
'       SIGEBM = ', e14.5,    ' (Coef sigma_eps              )',/,&
'       XA1    = ', e14.5,    ' (Coef A1                     )',/,&
'       SIGMAK = ', e14.5,    ' (Coef sigma_k                )',/,&
'       XCETA  = ', e14.5,    ' (Coef Ceta                   )',/,&
'       XCT    = ', e14.5,    ' (Coef CT                     )',/)

 2540 format(/)

#endif

! --- Viscosite secondaire

write(nfecra,2610)
write(nfecra,2620) ivisse

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 2610 format(                                                     &
                                                                /,&
' ** VISCOSITE SECONDAIRE',                                     /,&
'    --------------------',                                     /)
 2620 format(                                                     &
' --- Phase continue :', i10,                                   /,&
'       IVISSE = ',4x,i10,    ' (1 : pris en compte          )',/)

#else

 2610 format(                                                     &
                                                                /,&
' ** SECONDARY VISCOSITY',                                      /,&
'    -------------------',                                      /)
 2620 format(                                                     &
' --- Continuous phase:', i10,                                  /,&
'       IVISSE = ',4x,i10,    ' (1: accounted for            )',/)

#endif

! --- Rayonnement thermique

if (iirayo.gt.0) then

  write(nfecra,2630)

  write(nfecra,2640) iirayo, iscalt, iscsth(iscalt)

  write(nfecra,2650) isuird, nfreqr, ndirec,                      &
                     idiver, imodak, iimpar, iimlum
  write(nfecra,2660)

  do ii = 1,nbrayf
    if(irayvf(ii).eq.1) write(nfecra,2662)nbrvaf(ii)
  enddo

  write(nfecra,9900)

endif

#if defined(_CS_LANG_FR)

 2630 format(                                                     &
                                                                /,&
' ** TRANSFERTS THERMIQUES RADIATIFS',                          /,&
'    -------------------------------',                          /)
 2640 format(                                                     &
' --- Phase continue :',                                        /,&
'       IIRAYO = ',4x,i10,    ' (0 : non ; 1 : DOM ; 2 : P-1 )',/,&
'       ICSALT = ',4x,i10,    ' (Num du sca thermique associe)',/,&
'       ISCSTH = ',4x,i10,    ' (-1 : T(C) ; 1 : T(K) ; 2 : H)',/)
 2650 format(                                                     &
' --- Options :',                                               /,&
'       ISUIRD = ',4x,i10,    ' (0 : pas de suite ; 1 : suite)',/,&
'       NFREQR = ',4x,i10,    ' (Frequence pass. rayonnement )',/,&
'       NDIREC = ',4x,i10,    ' (32 ou 128 directions(si DOM))',/,&
'       IDIVER = ',4x,i10,    ' (0 1 ou 2: calcul TS radiatif)',/,&
'       IMODAK = ',4x,i10,    ' (1: modak coef absor; 0 sinon)',/,&
'       IIMPAR = ',4x,i10,    ' (0 1 ou 2: impr Tempera paroi)',/,&
'       IIMLUM = ',4x,i10,    ' (0 1 ou 2: impr infos solveur)',/)
 2660 format(                                                     &
' --- Sorties graphiques :                                    '  )
 2662 format(                                                     &
'       NBRVAF = ',4x,A40                                        )

#else

 2630 format(                                                     &
                                                                /,&
' ** RADIATIVE THERMAL TRANSFER',                               /,&
'    --------------------------',                               /)
 2640 format(                                                     &
' --- Continuous phase:',                                       /,&
'       IIRAYO = ',4x,i10,    ' (0: no; 1: DOM; 2: P-1       )',/,&
'       ICSALT = ',4x,i10,    ' (Assoc. thermal scalar num.  )',/,&
'       ISCSTH = ',4x,i10,    ' (-1: T(C); 1: T(K); 2: H     )',/)
 2650 format(                                                     &
' --- Options:',                                                /,&
'       ISUIRD = ',4x,i10,    ' (0: no restart; 1: restart   )',/,&
'       NFREQR = ',4x,i10,    ' (Radiation pass frequency    )',/,&
'       NDIREC = ',4x,i10,    ' (32 or 128 directions(if DOM))',/,&
'       IDIVER = ',4x,i10,    ' (0 1 or 2: compute radiat. ST)',/,&
'       IMODAK = ',4x,i10,    ' (1: modak absor coef; 0 else )',/,&
'       IIMPAR = ',4x,i10,    ' (0 1 or 2: print wall temp.  )',/,&
'       IIMLUM = ',4x,i10,    ' (0 1 or 2: print solver info )',/)
 2660 format(                                                     &
' --- Graphical output:'                                         )
 2662 format(                                                     &
'       NBRVAF = ',4x,A40                                        )

#endif

! --- Compressible

if (ippmod(icompf).ge.0) then
  write(nfecra,2700)
  write(nfecra,2710) icv, iviscv, viscv0, icfgrp

  write(nfecra,9900)

endif

#if defined(_CS_LANG_FR)

 2700 format(                                                     &
                                                                /,&
' ** COMPRESSIBLE : donnees complementaires',                   /,&
'    ------------',                                             /)
 2710 format(                                                     &
' --- Phase continue :',                                        /,&
'       ICV    = ',4x,i10,    ' (0 : Cv cst ; 1 : variable   )',/,&
'       IVISCV = ',4x,i10,    ' (0 : kappa cst ; 1 : variable', /,&
'                                kappa : viscosite en volume',  /,&
'                                en kg/(m s)                 )',/,&
'       VISCV0 = ',e14.5,     ' (Valeur de kappa si cst      )',/,&
'       ICFGRP = ',4x,i10,    ' (1 : C.L. pression avec effet', /,&
'                                hydrostatique dominant      )',/)

#else

 2700 format(                                                     &
                                                                /,&
' ** COMPRESSIBLE: additional data',                            /,&
'    ------------',                                             /)
 2710 format(                                                     &
' --- Continuous phase :',                                      /,&
'       ICV    = ',4x,i10,    ' (0: Cv cst; 1: variable      )',/,&
'       IVISCV = ',4x,i10,    ' (0: kappa cst; 1: variable',    /,&
'                                kappa: volume viscosity',      /,&
'                                in kg/(m.s)                 )',/,&
'       VISCV0 = ',e14.5,     ' (kappa value if constant     )',/,&
'       ICFGRP = ',4x,i10,    ' (1: pressure BC with dominant', /,&
'                                hydrostatic effect          )',/)

#endif

!===============================================================================
! 4. DISCRETISATION DES EQUATIONS
!===============================================================================

! --- Marche en temps

write(nfecra,3000)

!     Stationnaire
if (idtvar.lt.0) then

!   - Parametres du pas de temps

  write(nfecra,3010) idtvar, relxst

!   - Champ de vitesse fige

  write(nfecra,3030) iccvfg

!   - Coefficient de relaxation

  write(nfecra,3011)
  do ipp = 2, nvppmx
    ii = itrsvr(ipp)
    if(ii.ge.1) then
      chaine=nomvar(ipp)
      write(nfecra,3012) chaine(1:16),relaxv(ii)
    endif
  enddo
  write(nfecra,3013)

!     Instationnaire
else

!   - Parametres du pas de temps

  write(nfecra,3020) idtvar,iptlro,coumax,foumax,                 &
       varrdt,dtmin,dtmax,dtref

!   - Champ de vitesse fige

  write(nfecra,3030) iccvfg

!   - Coef multiplicatif du pas de temps

  write(nfecra,3040)
  do ipp = 2, nvppmx
    ii = itrsvr(ipp)
    if(ii.ge.1) then
      chaine=nomvar(ipp)
      write(nfecra,3041) chaine(1:16),istat(ii),cdtvar(ii)
    endif
  enddo
  write(nfecra,3042)


!   - Coefficient de relaxation de la masse volumique

  if (ippmod(iphpar).ge.2) write(nfecra,3050) srrom

!   - Ordre du schema en temps

  write(nfecra,3060)
  write(nfecra,3061) ischtp
  write(nfecra,3062)

endif

write(nfecra,9900)

#if defined(_CS_LANG_FR)

 3000 format(                                                     &
                                                                /,&
' ** MARCHE EN TEMPS',                                          /,&
'    ---------------',                                          /)
 3010 format(                                                     &
'    ALGORITHME STATIONNAIRE',                                  /,&
                                                                /,&
' --- Parametres globaux',                                      /,&
                                                                /,&
'       IDTVAR = ',4x,i10,    ' (-1: algorithme stationnaire )',/,&
'       RELXST = ', e14.5,    ' (Coef relaxation de reference)',/,&
                                                                /)
 3011 format(                                                     &
' --- Coefficient de relaxation par variable',                  /,&
                                                                /,&
'-----------------------------',                                /,&
' Variable          RELAXV',                                    /,&
'-----------------------------'                                   )
 3012 format(                                                     &
 1x,    a16,      e12.4                                           )
 3013 format(                                                     &
'----------------------------',                                 /,&
                                                                /,&
'       RELAXV =  [0.,1.]       (coeff de relaxation         )', /)
 3020 format(                                                     &
'    ALGORITHME INSTATIONNAIRE',                                /,&
                                                                /,&
' --- Parametres du pas de temps',                              /,&
                                                                /,&
'       IDTVAR = ',4x,i10,    ' (0 cst;1,2 var(tps,tps-espace)',/,&
'       IPTLRO = ',4x,i10,    ' (1 : clipping de DT lie a rho)',/,&
'       COUMAX = ', e14.5,    ' (Courant maximum cible       )',/,&
'       FOUMAX = ', e14.5,    ' (Fourier maximum cible       )',/,&
'       VARRDT = ', e14.5,    ' (En DT var, accroissement max)',/,&
'       DTMIN  = ', e14.5,    ' (Pas de temps min            )',/,&
'       DTMAX  = ', e14.5,    ' (Pas de temps max            )',/,&
'       DTREF  = ', e14.5,    ' (Pas de temps de reference   )',/,&
                                                                /,&
'       En pas de temps non constant (IDTVAR = 1 ou 2),',       /,&
'         lorsque la valeur de COUMAX ou FOUMAX est negative',  /,&
'         ou nulle, la limitation du pas de temps associee (au',/,&
'         nombre de Courant et de Fourier, respectivement)',    /,&
'         n ''est pas prise en compte.',                        /)
 3030 format(                                                     &
' --- Champ de vitesse fige',                                   /,&
                                                                /,&
'       ICCVFG = ',4x,i10,    ' (1 : champ de vitesse fige   )',/)
 3040 format(                                                     &
' --- Proprietes par variable',                                 /,&
                                                                /,&
'------------------------------------',                         /,&
' Variable          ISTAT      CDTVAR',                         /,&
'------------------------------------'                            )
 3041 format(                                                     &
 1x,    a16,    i7,      e12.4                                    )
 3042 format(                                                     &
'----------------------------',                                 /,&
                                                                /,&
'       ISTAT  =  0 ou  1       (1 pour instationnaire       )',/,&
'       CDTVAR >  0             (coeff mult. du pas de temps )',/)

 3050 format(                                                     &
'--- Coefficient de relaxation',                                /,&
'    RHO(n+1)=SRROM*RHO(n)+(1-SRROM)*RHO(n+1)',                 /,&
'       SRROM  = ',e14.5,                                       /)

 3060 format(                                                     &
' --- Ordre du schema en temps de base'                          )
 3061 format(                                                     &
'       ISCHTP = ',4x,i10,    ' (1 : ordre 1 ; 2 : ordre 2   )'  )
 3062 format(                                                     &
'                                                             '  )

#else

 3000 format(                                                     &
                                                                /,&
' ** TIME STEPPING',                                            /,&
'    -------------',                                            /)
 3010 format(                                                     &
'    STEADY ALGORITHM',                                         /,&
                                                                /,&
' --- Global parameters',                                       /,&
                                                                /,&
'       IDTVAR = ',4x,i10,    ' (-1: steady algorithm        )',/,&
'       RELXST = ', e14.5,    ' (Reference relaxation coeff. )',/,&
                                                                /)
 3011 format(                                                     &
' --- Per variable relaxation coefficient',                     /,&
                                                                /,&
'-----------------------------',                                /,&
' Variable          RELAXV',                                    /,&
'-----------------------------'                                   )
 3012 format(                                                     &
 1x,    a16,      e12.4                                           )
 3013 format(                                                     &
'----------------------------',                                 /,&
                                                                /,&
'       RELAXV =  [0.,1.]       (relaxation coefficient      )',/)
 3020 format(                                                     &
'    UNSTEADY ALGORITHM',                                       /,&
                                                                /,&
' --- Time step parameters',                                    /,&
                                                                /,&
'       IDTVAR = ',4x,i10,    ' (0 cst; 1,2 var (t, t-space  )',/,&
'       IPTLRO = ',4x,i10,    ' (1: rho-related DT clipping  )',/,&
'       COUMAX = ', e14.5,    ' (Maximum target CFL          )',/,&
'       FOUMAX = ', e14.5,    ' (Maximum target Fourier      )',/,&
'       VARRDT = ', e14.5,    ' (For var. DT, max. increase  )',/,&
'       DTMIN  = ', e14.5,    ' (Minimum time step           )',/,&
'       DTMAX  = ', e14.5,    ' (Maximum time step           )',/,&
'       DTREF  = ', e14.5,    ' (Reference time step         )',/,&
                                                                /,&
'       With a non-constant time step (IDTVAR = 1 or 2),',      /,&
'         when the value of COUMAX or FOUMAX is negative',      /,&
'         or zero, the associated time step limitation (for',   /,&
'         CFL and Fourier respectively) is ignored.',           /)
 3030 format(                                                     &
' --- Frozen velocity field',                                   /,&
                                                                /,&
'       ICCVFG = ',4x,i10,    ' (1: frozen velocity field    )',/)
 3040 format(                                                     &
' --- Per-variable properties',                                 /,&
                                                                /,&
'------------------------------------',                         /,&
' Variable          ISTAT      CDTVAR',                         /,&
'------------------------------------'                            )
 3041 format(                                                     &
 1x,    a16,    i7,      e12.4                                    )
 3042 format(                                                     &
'----------------------------',                                 /,&
                                                                /,&
'       ISTAT  =  0 ou  1       (1 for unsteady              )',/,&
'       CDTVAR >  0             (time step multiplier        )',/)

 3050 format(                                                     &
'--- Relaxation coefficient',                                   /,&
'    RHO(n+1)=SRROM*RHO(n)+(1-SRROM)*RHO(n+1)',                 /,&
'       SRROM  = ',e14.5,                                       /)

 3060 format(                                                     &
' --- Order of base time stepping scheme'                        )
 3061 format(                                                     &
'       ISCHTP = ',4x,i10,    ' (1: order 1; 2: order 2      )'  )
 3062 format(                                                     &
'                                                             '  )

#endif

! --- Convection Diffusion

write(nfecra,4000)

write(nfecra,4010)
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    chaine=nomvar(ipp)
    write(nfecra,4020) chaine(1:16),                              &
                       iconv(ii),idiff(ii),idifft(ii),            &
                       ischcv(ii),isstpc(ii),                     &
                       blencv(ii),thetav(ii)
  endif
enddo
write(nfecra,4030)

write(nfecra,9900)

! --- Stokes

write(nfecra,4110) ivelco, iphydr,icalhy,iprco,ipucou,nterup
write(nfecra,4111) irevmc
if (idtvar.ge.0) then
  write(nfecra,4112) relaxv(ipr),arak
else
  write(nfecra,4113) arak*relaxv(iu)
endif
write(nfecra,4114)istmpf,thetfl,     &
     iroext,thetro,                  &
     iviext,thetvi,                  &
     icpext,thetcp,                  &
     thetsn,thetst,epsup

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 4000 format(                                                     &
                                                                /,&
' ** CONVECTION - DIFFUSION',                                   /,&
'    ----------------------',                                   /)
 4010 format(                                                             &
'---------------------------------------------------------------------',/,&
' Variable          ICONV  IDIFF IDIFFT ISCHCV ISSTPC   BLENCV  THETAV',/,&
'---------------------------------------------------------------------'  )
 4020 format(                                                     &
 1x,    a16,    i7,    i7,    i7,    i7,    i7,    e9.2,    e9.2  )
 4030 format(                                                     &
'-------------------------------------------------------------',/,&
                                                                /,&
'       ICONV  =  0 ou  1       (1 pour convection branchee  )',/,&
'       IDIFF  =  0 ou  1       (1 pour diff. tot branchee   )',/,&
'       IDIFFT =  0 ou  1       (1 pour diff. turb. branchee )',/,&
'       ISCHCV =  0 ou  1       (SOLU ou CD                  )',/,&
'       ISSTPC =  0 ou  1       (1 : sans test de pente      )',/,&
'       BLENCV =  [0.;1.]       (1-proportion d upwind       )',/,&
'       THETAV =  [0.;1.]       (0.5 Crank-Nicolson/AB       )',/,&
'                               (theta pour les termes de    )',/,&
'                               (convection diffusion utilise)',/,&
'                               ((1-theta)ancien+theta nouveau',/)

 4110 format(                                                     &
                                                                /,&
' ** STOKES',                                                   /,&
'    ------',                                                   /,&
'       IVELCO = ',4x,i10,  ' (0 : resolution composante par',  /,&
'                ',14x,     '      composante de l''etape de',  /,&
'                ',14x,     '      prediction des vitesses',    /,&
'                ',14x,     '  1 : resolution couplee des',     /,&
'                ',14x,     '      composantes de vitesses   )',/,&
'       IPHYDR = ',4x,i10,  ' (1 : prise en compte explicite',  /,&
'                ',14x,     '      de l''equilibre entre grad', /,&
'                ',14x,     '      de pression et termes',      /,&
'                ',14x,     '      sources de gravite et de',   /,&
'                ',14x,     '      pertes de charge          )',/,&
'       ICALHY = ',4x,i10,  ' (1 : calcul de la pression',      /,&
'                ',14x,     '      hydrostatique pour les',     /,&
'                ',14x,     '      conditions de Dirichlet en', /,&
'                ',14x,     '      sortie sur la pression    )',/,&
'       IPRCO  = ',4x,i10,  ' (1 : avec pression-continuite  )',/,&
'       IPUCOU = ',4x,i10,  ' (1 : avec couplage U-P renforce)',/,&
'       NTERUP = ',4x,i10,  ' (n : avec n sweep sur navsto',    /,&
'                ',14x,     '      pour couplage vites/pressio',/)
 4111 format(                                                     &
'  -- Phase continue :',                                        /,&
                                                                /,&
'       IREVMC = ',4x,i10,    ' (Mode de reconstruction vites)',/)
 4112 format(                                                     &
'       RELAXV = ', e14.5,    ' pour la pression (relaxation)', /,&
'       ARAK   = ', e14.5,    ' (Facteur d Arakawa           )',/)
 4113 format(                                                     &
'       ARAK   = ', e14.5,    ' (Facteur d Arakawa           )',/)
 4114 format(                                                     &
'       ISTMPF = ',4x,i10,    ' (schema en temps pour le flux', /,&
'                ',14x,       ' (0 : explicite (THETFL = 0   )',/,&
'                ',14x,       ' (1 : schema std (Saturne 1.0 )',/,&
'                ',14x,       ' (2 : ordre 2   (THETFL = 0.5 )',/,&
'       THETFL = ', e14.5,    ' (theta pour flux de masse    )',/,&
'       IROEXT = ',4x,i10,    ' (extrap. masse volumique',      /,&
'                ',14x,       ' (0 : explicite',                /,&
'                ',14x,       ' (1 : n+thetro avec thetro=1/2', /,&
'                ',14x,       ' (2 : n+thetro avec thetro=1',   /,&
'       THETRO = ', e14.5,    ' (theta pour masse volumique',   /,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       IVIEXT = ',4x,i10,    ' (extrap. viscosite totale',     /,&
'                ',14x,       ' (0 : explicite',                /,&
'                ',14x,       ' (1 : n+thetvi avec thetro=1/2', /,&
'                ',14x,       ' (2 : n+thetvi avec thetro=1',   /,&
'       THETVI = ', e14.5,    ' (theta pour viscosite totale',  /,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       ICPEXT = ',4x,i10,    ' (extrap. chaleur specifique',   /,&
'                ',14x,       ' (0 : explicite',                /,&
'                ',14x,       ' (1 : n+thetcp avec thetro=1/2', /,&
'                ',14x,       ' (2 : n+thetcp avec thetro=1',   /,&
'       THETCP = ', e14.5,    ' (theta schema chaleur spec',    /,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       THETSN = ', e14.5,    ' (theta schema T.S. Nav-Stokes)',/,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       THETST = ', e14.5,    ' (theta schema T.S. Turbulence)',/,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       EPSUP  = ', e14.5,    ' (Test d''arret du couplage',    /,&
'                ',14x,       '  vitesse/pression            )',/)


#else

 4000 format(                                                     &
                                                                /,&
' ** CONVECTION - DIFFUSION',                                   /,&
'    ----------------------',                                   /)
 4010 format(                                                             &
'---------------------------------------------------------------------',/,&
' Variable          ICONV  IDIFF IDIFFT ISCHCV ISSTPC   BLENCV  THETAV',/,&
'---------------------------------------------------------------------'  )
 4020 format(                                                     &
 1x,    a16,    i7,    i7,    i7,    i7,    i7,    e9.2,    e9.2  )
 4030 format(                                                     &
'-------------------------------------------------------------',/,&
                                                                /,&
'       ICONV  =  0 ou  1       (1 for convection active     )',/,&
'       IDIFF  =  0 ou  1       (1 for total diffusion active)',/,&
'       IDIFFT =  0 ou  1       (1 for turbulent diff. active)',/,&
'       ISCHCV =  0 ou  1       (SOLU or CD                  )',/,&
'       ISSTPC =  0 ou  1       (1: no slope test            )',/,&
'       BLENCV =  [0.;1.]       (1-upwind proportion         )',/,&
'       THETAV =  [0.;1.]       (0.5 Crank-Nicolson/AB       )',/,&
'                               (theta for convection-       )',/,&
'                               (diffusion terms uses        )',/,&
'                               ((1-theta).old+theta.new     )',/)

 4110 format(                                                     &
                                                                /,&
' ** STOKES',                                                   /,&
'    ------',                                                   /,&
'       IVELCO = ',4x,i10,  ' (0: segragated solving of the',   /,&
'                ',14x,     '     velocity components during',  /,&
'                ',14x,     '     the prediction step',         /,&
'                ',14x,     '  1: coupled soulving of the',     /,&
'                ',14x,     '     velocity components       )', /,&
'       IPHYDR = ',4x,i10,  ' (1: account for explicit',        /,&
'                ',14x,     '     balance between pressure',    /,&
'                ',14x,     '     gradient, gravity source',    /,&
'                ',14x,     '     terms, and head losses     )',/,&
'       ICALHY = ',4x,i10,  ' (1: compute hydrastatic',        /, &
'                ',14x,     '     pressure for Dirichlet',      /,&
'                ',14x,     '     conditions for pressure',     /,&
'                ',14x,     '     on outlet                  )',/,&
'       IPRCO  = ',4x,i10,  ' (1: pressure-continuity        )',/,&
'       IPUCOU = ',4x,i10,  ' (1: reinforced U-P coupling    )',/,&
'       NTERUP = ',4x,i10,  ' (n: n sweeps on navsto for',      /,&
'                ',14x,     '     velocity/pressure coupling )',/)
 4111 format(                                                     &
'  -- Continuous phase:',                                       /,&
                                                                /,&
'       IREVMC = ',4x,i10,    ' (Velocity reconstruction mode)',/)
 4112 format(                                                     &
'       RELAXV = ', e14.5,    ' for pressure (relaxation)',     /,&
'       ARAK   = ', e14.5,    ' (Arakawa factor              )',/)
 4113 format(                                                     &
'       ARAK   = ', e14.5,    ' (Arakawa factor              )',/)
 4114 format(                                                     &
'       ISTMPF = ',4x,i10,    ' (time scheme for flow',         /,&
'                ',14x,       ' (0: explicit (THETFL = 0     )',/,&
'                ',14x,       ' (1: std scheme (Saturne 1.0  )',/,&
'                ',14x,       ' (2: 2nd-order (THETFL = 0.5  )',/,&
'       THETFL = ', e14.5,    ' (theta for mass flow         )',/,&
'       IROEXT = ',4x,i10,    ' (density extrapolation',        /,&
'                ',14x,       ' (0: explicit',                  /,&
'                ',14x,       ' (1: n+thetro with thetro=1/2',  /,&
'                ',14x,       ' (2: n+thetro with thetro=1',    /,&
'       THETRO = ', e14.5,    ' (theta for density',            /,&
'                               ((1+theta).new-theta.old',      /,&
'       IVIEXT = ',4x,i10,    ' (total viscosity extrapolation',/,&
'                ',14x,       ' (0: explicit',                  /,&
'                ',14x,       ' (1: n+thetvi with thetro=1/2',  /,&
'                ',14x,       ' (2: n+thetvi with thetro=1',    /,&
'       THETVI = ', e14.5,    ' (theta for total viscosity',    /,&
'                               ((1+theta).new-theta.old',      /,&
'       ICPEXT = ',4x,i10,    ' (specific heat extrapolation',  /,&
'                ',14x,       ' (0: explicit',                  /,&
'                ',14x,       ' (1: n+thetcp with thetro=1/2',  /,&
'                ',14x,       ' (2: n+thetcp with thetro=1',    /,&
'       THETCP = ', e14.5,    ' (specific heat theta-scheme',   /,&
'                               ((1+theta).new-theta.old',      /,&
'       THETSN = ', e14.5,    ' (Nav-Stokes S.T. theta scheme)',/,&
'                               ((1+theta).new-theta.old',      /,&
'       THETST = ', e14.5,    ' (Turbulence S.T. theta-scheme)',/,&
'                               ((1+theta).new-theta.old',      /,&
'       EPSUP  = ', e14.5,    ' (Velocity/pressure coupling',   /,&
'                ',14x,       '  stop test                   )',/)

#endif

! --- Calcul des gradients

write(nfecra,4500)

write(nfecra,4510) imrgra, anomax
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    chaine=nomvar(ipp)
    write(nfecra,4520) chaine(1:16),                               &
      nswrgr(ii),nswrsm(ii),epsrgr(ii),epsrsm(ii),extrag(ii)
  endif
enddo
write(nfecra,4511)
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    chaine=nomvar(ipp)
    write(nfecra,4521) chaine(1:16),                               &
      ircflu(ii),imligr(ii),climgr(ii)
  endif
enddo
write(nfecra,4530)

write(nfecra,9900)

! --- Interpolation face des viscosites

write(nfecra,4810) imvisf

write(nfecra,9900)

! --- Estimateurs d'erreurs pour Navier-Stokes

iiesca = 0
do iest = 1, nestmx
  if(iescal(iest).gt.0) then
    iiesca = 1
  endif
enddo

if(iiesca.gt.0) then
  write(nfecra,4820)
  write(nfecra,4821)
  do iest = 1, nestmx
    write(nfecra,4822)iest, iescal(iest)
  enddo
  write(nfecra,4823)
  write(nfecra,4824)iespre,iesder,iescor,iestot
  write(nfecra,9900)
endif

! --- Calcul de moyennes temporelles

write(nfecra,4900) nbmomt
if(nbmomt.gt.0) then
  write(nfecra,4901)
  do imom = 1, nbmomt
    write(nfecra,4920)imom,imoold(imom),                          &
         ntdmom(imom),(idfmom(jj,imom),jj=1,ndgmox)
  enddo
  write(nfecra,4930)
endif
write(nfecra,9900)


! --- Calcul de la distance a la paroi

if(ineedy.eq.1) then

  write(nfecra,4950) icdpar
  if(abs(icdpar).eq.1) then
    write(nfecra,4951)                                            &
        nitmay, nswrsy, nswrgy, imligy, ircfly, ischcy,           &
        isstpy, imgrpy, iwarny, ntcmxy,                           &
        blency, epsily, epsrsy, epsrgy, climgy, extray, coumxy,   &
        epscvy, yplmxy
  endif
  write(nfecra,9900)

endif


#if defined(_CS_LANG_FR)

 4500 format(                                                     &
                                                                /,&
' ** CALCUL DES GRADIENTS',                                     /,&
'    --------------------',                                     /)
 4510 format(                                                     &
'       IMRGRA = ',4x,i10,    ' (Mode de reconstruction      )',/,&
'       ANOMAX = ',e14.5,     ' (Angle de non ortho. limite  )',/,&
'                               (pour moindres carres etendu )',/,&
                                                                /,&
'-------------------------------------------------------------------',  /,&
' Variable         NSWRGR NSWRSM      EPSRGR      EPSRSM      EXTRAG',  /,&
'-------------------------------------------------------------------'    )
 4520 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,      e12.4,      e12.4     )
 4511 format(                                                     &
'-----------------------------------------------------------',  /,&
                                                                /,&
'-------------------------------------------',                  /,&
' Variable         IRCFLU IMLIGR      CLIMGR',                  /,&
'-------------------------------------------'                     )
 4521 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4                             )
 4530 format(                                                     &
'-----------------------------------',                          /,&
                                                                /,&
'       NSWRGR =                (nb sweep reconstruction grad)',/,&
'       NSWRSM =                (nb sweep reconstruction smb )',/,&
'       EPSRGR =                (precision reconstruction gra)',/,&
'       EPSRSM =                (precision reconstruction smb)',/,&
'       EXTRAG =  [0.;1.]       (extrapolation des gradients )',/,&
'       IRCFLU =  0 ou  1       (reconstruction des flux     )',/,&
'       IMLIGR =  < 0, 0 ou 1   (methode de limit. des grad  )',/,&
'       CLIMGR =  > 1 ou 1      (coef de limitation des grad )',/)

 4810 format(                                                     &
                                                                /,&
' ** INTERPOLATION FACE',                                       /,&
'    ------------------',                                       /,&
'       IMVISF = ',4x,i10,    ' (0 arithmetique              )',/)

 4820 format(                                                     &
                                                                /,&
' ** ESTIMATEURS D''ERREUR POUR NAVIER-STOKES',                 /,&
'    ----------------------------------------',                 /)
 4821 format(                                                     &
'----------------------------------------',                     /,&
' Estimateur      IESCAL (mode de calcul)',                     /,&
'----------------------------------------'                       )
 4822 format(                                                     &
 1x,     i10,2x,    i10                                          )
 4823 format(                                                     &
'----------------------------------------'                       )
 4824 format(                                                     &
                                                                /,&
' Estimateurs possibles :',                                     /,&
' ',i2,' =IESPRE : prediction',                                 /,&
'            L''estimateur est base sur la grandeur',           /,&
'            I = rho_n (u*-u_n)/dt + rho_n u_n grad u*',        /,&
'              - rho_n div (mu+mu_t)_n grad u* + grad P_n',     /,&
'              - reste du smb(u_n, P_n, autres variables_n)',   /,&
!     &'            Idealement nul quand les methodes de reconstruction
!     &'                sont parfaites et le systeme est resolu exactement
' ',i2,' =IESDER : derive',                                     /,&
'            L''estimateur est base sur la grandeur',           /,&
'            I = div (flux de masse corrige apres etape',       /,&
'                                               de pression)',  /,&
'            Idealement nul quand l''equation de Poisson est',  /,&
'              resolue exactement',                             /,&
' ',i2,' =IESCOR : correction',                                 /,&
'            L''estimateur est base sur la grandeur',           /,&
'            I = div (rho_n u_(n+1))',                          /,&
'            Idealement nul quand l''equation de Poisson est',  /,&
'              resolue exactement et que le passage des flux',  /,&
'              de masse aux faces vers les vitesses au centre', /,&
'              se fait dans un espace de fonctions',            /,&
'              a divergence nulle',                             /,&
' ',i2,' =IESTOT : total',                                      /,&
'            L''estimateur est base sur la grandeur',           /,&
'            I = rho_n (u_(n+1)-u_n)/dt',                       /,&
'                                 + rho_n u_(n+1) grad u_(n+1)',/,&
'              - rho_n div (mu+mu_t)_n grad u_(n+1)',           /,&
'                                               + gradP_(n+1)', /,&
'              - reste du smb(u_(n+1), P_(n+1),',               /,&
'                                          autres variables_n)',/,&
'             Le flux du terme convectif est calcule a partir', /,&
'               de u_(n+1) pris au centre des cellules (et',    /,&
'               non pas a partir du flux de masse aux faces',   /,&
'               actualise)',                                    /,&
                                                                /,&
' On evalue l''estimateur selon les valeurs de IESCAL :',       /,&
'   IESCAL = 0 : l''estimateur n''est pas calcule',             /,&
'   IESCAL = 1 : l''estimateur    est     calcule,',            /,&
'               sans contribution du volume  (on prend abs(I))',/,&
'   IESCAL = 2 : l''estimateur    est     calcule,',            /,&
'               avec contribution du volume ("norme L2")',      /,&
'               soit abs(I)*SQRT(Volume_cellule),',             /,&
'               sauf pour IESCOR : on calcule',                 /,&
'                 abs(I)*Volume_cellule pour mesurer',          /,&
'                 un ecart en kg/s',                            /)

 4900 format(                                                     &
                                                                /,&
' ** CALCUL DES MOYENNES TEMPORELLES (MOMENTS)',                /,&
'    -----------------------------------------',                /,&
                                                                /,&
'       NBMOMT = ',4x,i10,    ' (Nombre de moments           )'  )
 4901 format(                                                     &
                                                                /,&
'------------------------------------------------------',       /,&
' IMOM IMOOLD NDTMOM IDFMOM',                                   /,&
'------------------------------------------------------'         )
 4920 format(                                                     &
 1x,i4,    i7,    i7,5(i7)                                       )
 4930 format(                                                     &
'------------------------------------------------------',       /,&
                                                                /,&
'       IMOM   = 0 ou > 0       (numero du moment            )',/,&
'       IMOOLD =-1 ou > 0       (ancien moment correspondant )',/,&
'                               (  en suite de calcul ou     )',/,&
'                               (-1 si le moment est         )',/,&
'                               (  reinitialise              )',/,&
'       NDTMOM = 0 ou > 0       (numero du pas de temps de   )',/,&
'                               (debut de calcul du moment   )',/,&
'       IDFMOM = 0 ou > 0       (numero des variables        )',/,&
'                               (composant le moment         )',/)

 4950 format(                                                     &
                                                                /,&
' ** CALCUL DE LA DISTANCE A LA PAROI',                         /,&
'    --------------------------------',                         /,&
                                                                /,&
'       ICDPAR = ',4x,i10,    ' ( 1: std et relu      si suite',/,&
'                               (-1: std et recalcule si suite',/,&
'                               ( 2: old et relu      si suite',/,&
'                               (-2: old et recalcule si suite',/)
4951  format(                                                     &
                                                                /,&
'       NITMAY = ',4x,i10,    ' (Nb iter pour resolution iter.',/,&
'       NSWRSY = ',4x,i10,    ' (Nb iter pour reconstr. smb. )',/,&
'       NSWRGY = ',4x,i10,    ' (Nb iter pour reconstr. grd. )',/,&
'       IMLIGY = ',4x,i10,    ' (Methode de limitation grd.  )',/,&
'       IRCFLY = ',4x,i10,    ' (Reconst. flux conv. diff.   )',/,&
'       ISCHCY = ',4x,i10,    ' (Schema convectif            )',/,&
'       ISSTPY = ',4x,i10,    ' (Utilisation test de pente   )',/,&
'       IMGRPY = ',4x,i10,    ' (Algorithme multigrille      )',/,&
'       IWARNY = ',4x,i10,    ' (Niveau d''impression        )',/,&
'       NTCMXY = ',4x,i10,    ' (Nb iter pour convection stat.',/,&
                                                                /,&
'       BLENCY = ',e14.5,     ' (Prop. ordre 2 schema convect.',/,&
'       EPSILY = ',e14.5,     ' (Precision solveur iteratif  )',/,&
'       EPSRSY = ',e14.5,     ' (Precision reconst. smb.     )',/,&
'       EPSRGY = ',e14.5,     ' (Precision reconst. grd.     )',/,&
'       CLIMGY = ',e14.5,     ' (Coeff. pour limitation grd. )',/,&
'       EXTRAY = ',e14.5,     ' (Coeff. pour extrapolation grd',/,&
'       COUMXY = ',e14.5,     ' (Courant max pour convection )',/,&
'       EPSCVY = ',e14.5,     ' (Precision pour convect. stat.',/,&
'       YPLMXY = ',e14.5,     ' (y+ max avec influence amort.)',/)

#else

 4500 format(                                                     &
                                                                /,&
' ** GRADIENTS CALCULATION',                                    /,&
'    ---------------------',                                    /)
 4510 format(                                                     &
'       IMRGRA = ',4x,i10,    ' (Reconstruction mode         )',/,&
'       ANOMAX = ',e14.5,     ' (Non-ortho angle: limit for  )',/,&
'                               (least squares ext. neighbors)',/,&
                                                                /,&
'-------------------------------------------------------------------',  /,&
' Variable         NSWRGR NSWRSM      EPSRGR      EPSRSM      EXTRAG',  /,&
'-------------------------------------------------------------------'    )
 4520 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,      e12.4,      e12.4     )
 4511 format(                                                     &
'-----------------------------------------------------------',  /,&
                                                                /,&
'-------------------------------------------',                  /,&
' Variable         IRCFLU IMLIGR      CLIMGR',                  /,&
'-------------------------------------------'                     )
 4521 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4                             )
 4530 format(                                                     &
'-----------------------------------',                          /,&
                                                                /,&
'       NSWRGR =                (nb sweep gradient reconstr. )',/,&
'       NSWRSM =                (nb sweep rhs reconstrcution )',/,&
'       EPSRGR =                (grad. reconstruction prec.  )',/,&
'       EPSRSM =                (rhs   reconstruction prec.  )',/,&
'       EXTRAG =  [0.;1.]       (gradients extrapolation     )',/,&
'       IRCFLU =  0 ou  1       (flow reconstruction         )',/,&
'       IMLIGR =  < 0, 0 ou 1   (gradient limitation method  )',/,&
'       CLIMGR =  > 1 ou 1      (gradient limitation coeff.  )',/)

 4810 format(                                                     &
                                                                /,&
' ** FACE INTERPOLATION',                                       /,&
'    ------------------',                                       /,&
'       IMVISF = ',4x,i10,    ' (0 arithmetic                )',/)

 4820 format(                                                     &
                                                                /,&
' ** ERROR ESTIMATORS FOR NAVIER-STOKES',                       /,&
'    ----------------------------------',                       /)
 4821 format(                                                     &
'------------------------------------------',                   /,&
' Estimateur      IESCAL (calculation mode)',                   /,&
'------------------------------------------'                     )
 4822 format(                                                     &
 1x,     i10,2x,    i10                                          )
 4823 format(                                                     &
'----------------------------------------'                       )
 4824 format(                                                     &
                                                                /,&
' Possible estimators:',                                        /,&
' ',i2,' =IESPRE: prediction',                                  /,&
'            The estimatore is based on the quantity',          /,&
'            I = rho_n (u*-u_n)/dt + rho_n u_n grad u*',        /,&
'              - rho_n div (mu+mu_t)_n grad u* + grad P_n',     /,&
'              - remainder of rhs(u_n, P_n, other variables_n)',  &
' ',i2,' =IESDER: drift',                                       /,&
'            The estimator is based on quantity',               /,&
'            I = div (mass flow corrected after pressure step)',/,&
'            Ideally zero when Poisson''s equation is',         /,&
'              resolved exactly',                               /,&
' ',i2,' =IESCOR: correction',                                  /,&
'            The estimator is based on quantity',               /,&
'            I = div (rho_n u_(n+1))',                          /,&
'            Ideally zero when Poisson''s equation is',         /,&
'              resolved exactly and the passage from mass flow',/,&
'              at faces to velocity at cell centers is done',   /,&
'              in a function space with zero divergence',       /,&
' ',i2,' =IESTOT: total',                                       /,&
'            Estimator is based on the quantity',               /,&
'            I = rho_n (u_(n+1)-u_n)/dt',                       /,&
'                                 + rho_n u_(n+1) grad u_(n+1)',/,&
'              - rho_n div (mu+mu_t)_n grad u_(n+1)',           /,&
'                                               + gradP_(n+1)', /,&
'              - rmainder of rhs(u_(n+1), P_(n+1),',            /,&
'                                          other variables_n)',/, &
'             The convective term flow is calculated from',     /,&
'               u_(n+1) taken at the cell centers (and not',    /,&
'               from the updated mass flow at faces)',          /,&
                                                                /,&
' We evaluate the estimator based on values of IESCAL:',        /,&
'   IESCAL = 0: the estimator is not calculated',               /,&
'   IESCAL = 1: the estimator is calculated, with no',          /,&
'               contribution from the volume (we take abs(I))', /,&
'   IESCAL = 2: the estimator is calculated,',                  /,&
'               with contribution from the volume ("L2 norm")', /,&
'               that is abs(I)*SQRT(Cell_volume),',             /,&
'               except for IESCOR: we calculate',               /,&
'                 abs(I)*Cell_volume to measure',               /,&
'                 a difference in kg/s',                        /)

 4900 format(                                                     &
                                                                /,&
' ** CALCULATION OF TEMPORAL MEANS (MOMENTS)',                  /,&
'    ---------------------------------------',                  /,&
                                                                /,&
'       NBMOMT = ',4x,i10,    ' (Number of moments           )'  )
 4901 format(                                                     &
                                                                /,&
'------------------------------------------------------',       /,&
' IMOM IMOOLD NDTMOM IDFMOM',                                   /,&
'------------------------------------------------------'         )
 4920 format(                                                     &
 1x,i4,    i7,    i7,5(i7)                                       )
 4930 format(                                                     &
'------------------------------------------------------',       /,&
                                                                /,&
'       IMOM   = 0 ou > 0       (moment number               )',/,&
'       IMOOLD =-1 ou > 0       (old moment corresponding    )',/,&
'                               (  to calculation restart or )',/,&
'                               (-1 if the moment is         )',/,&
'                               (  reinitialized             )',/,&
'       NDTMOM = 0 ou > 0       (moment calculation starting )',/,&
'                               (time step number            )',/,&
'       IDFMOM = 0 ou > 0       (number of variables of      )',/,&
'                               (which the moment is composed)',/)

 4950 format(                                                     &
                                                                /,&
' ** WALL DISTANCE COMPUTATION',                                /,&
'    -------------------------',                                /,&
                                                                /,&
'       ICDPAR = ',4x,i10,    ' ( 1: std, reread if restart',   /,&
'                               (-1: std, recomputed if restrt',/,&
'                               ( 2: old, reread if restart',   /,&
'                               (-2: old, recomputed if restrt',/)
4951  format(                                                     &
                                                                /,&
'       NITMAY = ',4x,i10,    ' (Nb iter for iter resolution )',/,&
'       NSWRSY = ',4x,i10,    ' (Nb iter for rhs reconstr.   )',/,&
'       NSWRGY = ',4x,i10,    ' (Nb iter for grad. reconstr. )',/,&
'       IMLIGY = ',4x,i10,    ' (Gradient limitation method  )',/,&
'       IRCFLY = ',4x,i10,    ' (Conv. Diff. flow reconstr.  )',/,&
'       ISCHCY = ',4x,i10,    ' (Convective scheme           )',/,&
'       ISSTPY = ',4x,i10,    ' (Slope tet use               )',/,&
'       IMGRPY = ',4x,i10,    ' (Multigrid algorithm         )',/,&
'       IWARNY = ',4x,i10,    ' (Verbosity level             )',/,&
'       NTCMXY = ',4x,i10,    ' (Nb iter for steady convect. )',/,&
                                                                /,&
'       BLENCY = ',e14.5,     ' (2nd order conv. scheme prop.)',/,&
'       EPSILY = ',e14.5,     ' (Iterative solver precision  )',/,&
'       EPSRSY = ',e14.5,     ' (rhs reconstruction precision)',/,&
'       EPSRGY = ',e14.5,     ' (Gradient reconstr. precision)',/,&
'       CLIMGY = ',e14.5,     ' (Coeff. for grad. limitation )',/,&
'       EXTRAY = ',e14.5,     ' (Coeff. for grad. extrapolat.)',/,&
'       COUMXY = ',e14.5,     ' (Max CFL for convection      )',/,&
'       EPSCVY = ',e14.5,     ' (Precision for steady conv.  )',/,&
'       YPLMXY = ',e14.5,     ' (y+ max w. damping influence )',/)

#endif


!===============================================================================
! 5. SOLVEURS
!===============================================================================

! --- Solveurs iteratifs de base

write(nfecra,5010)
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    chaine=nomvar(ipp)
    write(nfecra,5020) chaine(1:16),iresol(ii),                    &
                     nitmax(ii),epsilo(ii),idircl(ii)
  endif
enddo
write(nfecra,5030)

write(nfecra,9900)


! --- Multigrille

write(nfecra,5510)ncegrm, ngrmax
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    chaine=nomvar(ipp)
    write(nfecra,5520) chaine(1:16),                               &
      imgr(ii),ncymax(ii),nitmgf(ii)
  endif
enddo
write(nfecra,5530)

call clmimp
!==========

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 5010 format(                                                     &
                                                                /,&
' ** SOLVEURS ITERATIFS DE BASE',                               /,&
'    --------------------------',                               /,&
                                                                /,&
'--------------------------------------------------',           /,&
' Variable         IRESOL NITMAX      EPSILO IDIRCL',           /,&
'--------------------------------------------------'              )
 5020 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,    i7                      )
 5030 format(                                                     &
'-----------------------------------',                          /,&
                                                                /,&
'       IRESOL =            -1  (choix automatique du solveur)',/,&
'                IPOL*1000 + 0  (gradient conjugue           )',/,&
'                            1  (jacobi                      )',/,&
'                IPOL*1000 + 2  (bigradient conjugue         )',/,&
'                  avec IPOL    (degre du preconditionnement )',/,&
'       NITMAX =                (nb d iterations max         )',/,&
'       EPSILO =                (precision resolution        )',/,&
'       IDIRCL = 0 ou 1         (decalage de la diagonale si',  /,&
'                                ISTAT=0 et pas de Dirichlet )',/)

 5510 format(                                                     &
                                                                /,&
' ** MULTIGRILLE',                                              /,&
'    -----------',                                              /,&
                                                                /,&
'       NCEGRM = ',4x,i10,    ' (Nb cell max mail grossier   )',/,&
'       NGRMAX = ',4x,i10,    ' (Nb max de niveaux de mail   )',/,&
'--------------------------------------',                       /,&
' Variable           IMGR NCYMAX NITMGF',                       /,&
'--------------------------------------                        '  )
 5520 format(                                                     &
 1x,    a16,    i7,    i7,    i7                                  )
 5530 format(                                                     &
'------------------------------',                               /,&
                                                                /,&
'       IMGR   =  0 ou 1        (1 : activation du mltgrd    )',/,&
'       NCYMAX =                (Nb max de cycles            )',/,&
'       NITMGF =                (Nb max d iter sur mail fin  )',/)

#else

 5010 format(                                                     &
                                                                /,&
' ** BASE ITERATIVE SOLVERS',                                   /,&
'    ----------------------',                                   /,&
                                                                /,&
'--------------------------------------------------',           /,&
' Variable         IRESOL NITMAX      EPSILO IDIRCL',           /,&
'--------------------------------------------------'              )
 5020 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,    i7                      )
 5030 format(                                                     &
'-----------------------------------',                          /,&
                                                                /,&
'       IRESOL =            -1  (automatic solver choice     )',/,&
'                IPOL*1000 + 0  (p conjuguate gradient       )',/,&
'                            1  (Jacobi                      )',/,&
'                IPOL*1000 + 2  (bicgstab                    )',/,&
'                  avec IPOL    (preconditioning degree      )',/,&
'       NITMAX =                (max number of iterations    )',/,&
'       EPSILO =                (resolution precision        )',/,&
'       IDIRCL = 0 ou 1         (shift diagonal if   ',         /,&
'                                ISTAT=0 and no Dirichlet    )',/)

 5510 format(                                                     &
                                                                /,&
' ** MULTIGRID',                                                /,&
'    ---------',                                                /,&
                                                                /,&
'       NCEGRM = ',4x,i10,    ' (Max nb cells coarsest grid  )',/,&
'       NGRMAX = ',4x,i10,    ' (Max number of levels        )',/,&
'--------------------------------------',                       /,&
' Variable           IMGR NCYMAX NITMGF',                       /,&
'--------------------------------------                       '  )
 5520 format(                                                     &
 1x,    a16,    i7,    i7,    i7                                 )
 5530 format(                                                     &
'------------------------------',                               /,&
                                                                /,&
'       IMGR   =  0 ou 1        (1: multigrid activated      )',/,&
'       NCYMAX =                (Max number  of cycles       )',/,&
'       NITMGF =                (Max nb iter on coarsest grid)',/)

#endif

!===============================================================================
! 6. SCALAIRES
!===============================================================================

! --- Scalaires

if(nscal.ge.1) then
  write(nfecra,6000)
  write(nfecra,6010)itbrrb
  write(nfecra,6011)
  do ii = 1, nscal
    chaine=nomvar(ipprtp(isca(ii)))
    write(nfecra,6021) chaine(1:16),ii,iscsth(ii),      &
                       ivisls(ii),visls0(ii),sigmas(ii)
  enddo
  write(nfecra,6031)
  write(nfecra,6012)
  do ii = 1, nscal
    chaine=nomvar(ipprtp(isca(ii)))
    write(nfecra,6022) chaine(1:16),ii,iscavr(ii),      &
                       rvarfl(ii)
  enddo
  write(nfecra,6032)
  write(nfecra,6013)
  do ii = 1, nscal
    chaine=nomvar(ipprtp(isca(ii)))
    write(nfecra,6023) chaine(1:16),ii,iclvfl(ii),      &
                       scamin(ii),scamax(ii)
  enddo
  write(nfecra,6033)
  write(nfecra,6030)
  write(nfecra,6040)
  do ii = 1, nscal
    write(nfecra,6041) ii,thetss(ii),ivsext(ii),thetvs(ii)
  enddo
  write(nfecra,6042)

  write(nfecra,9900)

endif


#if defined(_CS_LANG_FR)

 6000 format(                                                     &
                                                                /,&
' ** SCALAIRES',                                                /,&
'    ---------',                                                /)
 6010 format(                                                     &
'       ITBRRB = ',4x,i10,    ' (Reconstruction T ou H au brd)',/)
 6011 format(                                                     &
'--------------------------------------------------------------',/,&
' Variable         Numero ISCSTH IVISLS      VISLS0      SIGMAS',/,&
'--------------------------------------------------------------'  )
 6021 format(                                                     &
 1x,    a16,    i7,    i7,    i7,      e12.4,      e12.4  )
 6031 format(                                                     &
'------------------------------------------------------',/)
 6012 format(                                                     &
'-------------------------------------------',                  /,&
' Variable         Numero ISCAVR      RVARFL',                  /,&
'-------------------------------------------'                     )
 6022 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4                     )
 6032 format(                                                     &
'-----------------------------------',                   /)
 6013 format(                                                     &
'-------------------------------------------------------',      /,&
' Variable         Numero ICLVFL      SCAMIN      SCAMAX',      /,&
'-------------------------------------------------------'         )
 6023 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,      e12.4         )
 6033 format(                                                     &
'-----------------------------------------------',       /)
 6030 format(                                                     &
'-------------------------------------------------------------',/,&
                                                                /,&
'       Le numero indique pour chaque scalaire le rang',        /,&
'         dans la liste de tous les scalaires. Les scalaires',  /,&
'         utilisateurs sont places en tete, de 1 a NSCAUS. Les',/,&
'         scalaires physique particuliere sont a la fin, de',   /,&
'         NSCAUS+1 a NSCAPP+NSCAUS=NSCAL.',                     /,&
                                                                /,&
'       ISCSTH = -1,0, 1 ou 2   (T (C), Passif, T (K) ou H   )',/,&
'       IVISLS = 0 ou >0        (Viscosite constante ou non  )',/,&
'       VISLS0 = >0             (Viscosite de reference      )',/,&
'       SIGMAS = >0             (Schmidt                     )',/,&
'       ISCAVR = 0 ou >0        (Scalaire associe si variance)',/,&
'       RVARFL = >0             (Rf, cf dissipation variance )',/,&
'       ICLVFL = 0, 1 ou 2      (Mode de clipping variance   )',/,&
'       SCAMIN =                (Valeur min autorisee        )',/,&
'       SCAMAX =                (Valeur max autorisee        )',/,&
'        Pour les variances, SCAMIN est ignore et SCAMAX n est',/,&
'          pris en compte que si ICLVFL = 2',                   /)
 6040 format(                                                     &
'------------------------------------------------------',       /,&
'   Scalaire      THETSS    IVSEXT      THETVS',                /,&
'------------------------------------------------------'         )
 6041 format(                                                     &
 1x,     i10,      e12.4,      i10,      e12.4                   )
 6042 format(                                                     &
'------------------------------------------------------',       /,&
                                                                /,&
'       THETSS =                (theta pour termes sources   )',/,&
'                               ((1+theta)nouveau-theta ancien',/,&
'       IVSEXT =                (extrap. viscosite totale    )',/,&
'                               (0 : explicite               )',/,&
'                               (1 : n+thetvs avec thetvs=1/2', /,&
'                               (2 : n+thetvs avec thetvs=1  )',/,&
'       THETVS =                (theta pour diffusiv. scalaire',/,&
'                               ((1+theta)nouveau-theta ancien',/)

#else

 6000 format(                                                     &
                                                                /,&
' ** SCALARS',                                                  /,&
'    -------',                                                  /)
 6010 format(                                                     &
'       ITBRRB = ',4x,i10,    ' (T or H reconstruction at bdy)',/)
 6011 format(                                                     &
'--------------------------------------------------------------',/,&
' Variable         Number ISCSTH IVISLS      VISLS0      SIGMAS',/,&
'--------------------------------------------------------------'  )
 6021 format(                                                     &
 1x,    a16,    i7,    i7,    i7,      e12.4,      e12.4  )
 6031 format(                                                     &
'------------------------------------------------------',/)
 6012 format(                                                     &
'-------------------------------------------',                  /,&
' Variable         Number ISCAVR      RVARFL',                  /,&
'-------------------------------------------'                    )
 6022 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4                     )
 6032 format(                                                     &
'-----------------------------------',                   /)
 6013 format(                                                     &
'-------------------------------------------------------',      /,&
' Variable         Number ICLVFL      SCAMIN      SCAMAX',      /,&
'-------------------------------------------------------'        )
 6023 format(                                                     &
 1x,    a16,    i7,    i7,      e12.4,      e12.4         )
 6033 format(                                                     &
'-----------------------------------------------',       /)
 6030 format(                                                     &
'-------------------------------------------------------------',/,&
                                                                /,&
'       For each scalar, the number indicates it''s rank',      /,&
'         in the list of all scalars. User scalars are placed', /,&
'         first, from 1 to NSCAUS. Specific physics scalars',   /,&
'         are placed at the end, from',                         /,&
'         NSCAUS+1 to NSCAPP+NSCAUS=NSCAL.',                    /,&
                                                                /,&
'       ISCSTH = -1,0, 1 ou 2   (T (C), Passive, T (K) or H  )',/,&
'       IVISLS = 0 ou >0        (Viscosity: constant or not  )',/,&
'       VISLS0 = >0             (Reference viscosity         )',/,&
'       SIGMAS = >0             (Schmidt                     )',/,&
'       ISCAVR = 0 ou >0        (Associat. scalar if variance)',/,&
'       RVARFL = >0             (Rf, cf variance dissipation )',/,&
'       ICLVFL = 0, 1 ou 2      (Variance clipping mode      )',/,&
'       SCAMIN =                (Min authorized value        )',/,&
'       SCAMAX =                (Max authorized value        )',/,&
'        For variances, SCAMIN is ignored and SCAMAX is used',  /,&
'          only if ICLVFL = 2',                                 /)
 6040 format(                                                     &
'------------------------------------------------------',       /,&
'   Scalar        THETSS    IVSEXT      THETVS',                /,&
'------------------------------------------------------'         )
 6041 format(                                                     &
 1x,     i10,      e12.4,      i10,      e12.4                   )
 6042 format(                                                     &
'------------------------------------------------------',       /,&
                                                                /,&
'       THETSS =                (theta for source terms      )',/,&
'                               ((1+theta).new-theta.old     )',/,&
'       IVSEXT =                (extrap. total viscosity     )',/,&
'                               (0: explicit                 )',/,&
'                               (1: n+thetvs with thetvs=1/2 )',/,&
'                               (2: n+thetvs with thetvs=1   )',/,&
'       THETVS =                (theta for scalar diffusivity', /,&
'                               ((1+theta).new-theta.old     )',/)

#endif

!===============================================================================
! 7. GESTION DU CALCUL
!===============================================================================

! --- Gestion du calcul

write(nfecra,7000)

!   - Suite de calcul

write(nfecra,7010) isuite, ileaux, iecaux
if(isuite.eq.1.and.nscal.ge.1) then
  write(nfecra,7020)
  do ii = 1, nscal
    ivar = isca(ii)
    chaine=nomvar(ipprtp(ivar))
    write(nfecra,7030) chaine(1:16),ii,iscold(ii)
  enddo
  write(nfecra,7040)
endif

!   - Duree du calcul

write(nfecra,7110) inpdt0,ntmabs

!   - Marge en temps CPU

write(nfecra,7210) tmarus

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 7000 format(                                                     &
                                                                /,&
' ** GESTION DU CALCUL',                                        /,&
'    -----------------',                                        /)
 7010 format(                                                     &
' --- Suite de calcul',                                         /,&
'       ISUITE = ',4x,i10,    ' (1 : suite de calcul         )',/,&
'       ILEAUX = ',4x,i10,    ' (1 : lecture  de suiamx aussi)',/,&
'       IECAUX = ',4x,i10,    ' (1 : ecriture de suiavx aussi)',/,&
                                                                /,&
'       suiamx et suiavx sont les fichiers suite auxiliaires.)',/)
 7020 format(                                                     &
'       ISCOLD(I) : Dans le calcul precedent, numero du',       /,&
'                   scalaire correspondant au scalaire I du',   /,&
'                   calcul courant :',                          /,&
                                                                /,&
'-----------------------------------------------------------',  /,&
' Scalaire         Numero   <-   Numero de l''ancien scalaire', /,&
'-----------------------------------------------------------  '  )
 7030 format(                                                     &
 1x,    a16,    i7,    7x,    i7                                 )
 7040 format(                                                     &
'---------------------------------------------------',          /,&
                                                                /,&
'   La table precedente (ISCOLD) donne la correspondance des',  /,&
'     scalaires du calcul courant avec ceux du calcul',         /,&
'     precedent apres intervention eventuelle de l utilisateur',/,&
'   Il s''agit de numeros de 1 a NSCAL qui reperent le',        /,&
'     scalaire dans la liste de tous les scalaires',            /,&
'     utilisateur+physique particuliere.',                      /,&
'     .-999 est la valeur par defaut si l utilisateur n est',   /,&
'           intervenu. Le numero des correspondant sera',       /,&
'           complete a la lecture du fichier suite, selon',     /,&
'           le nombre de scalaires disponibles, en utilisant',  /,&
'           la loi (nouveau scalaire ii <- ancien scalaire ii)',/,&
'     .   0 si l utilisateur souhaite que le scalaire du',      /,&
'           calcul courant n ait pas de correspondant (i.e.',   /,&
'           soit un nouveau scalaire).',                        /,&
'     .   n > 0 si l utilisateur souhaite que le scalaire du',  /,&
'           calcul courant ait pour correspondant le scalaire', /,&
'           n du calcul precedent.',                            /)
 7110 format(                                                     &
' --- Duree du calcul',                                         /,&
'     La numerotation des pas de temps et la mesure du temps',  /,&
'       physique simule sont des valeurs absolues',             /,&
'       et non pas des valeurs relatives au calcul en cours.',  /,&
                                                                /,&
'       INPDT0 = ',4x,i10,    ' (1 : calcul a zero pas de tps)',/,&
'       NTMABS = ',4x,i10,    ' (Pas de tps final demande    )',/)
 7210 format(                                                     &
' --- Marge en temps CPU',                                      /,&
'       TMARUS = ', e14.5,    ' (Marge CPU avant arret       )',/)
#else

 7000 format(                                                     &
                                                                /,&
' ** CALCULATION MANAGEMENT',                                   /,&
'    ----------------------',                                   /)
 7010 format(                                                     &
' --- Restarted calculation',                                   /,&
'       ISUITE = ',4x,i10,    ' (1: restarted calculuation   )',/,&
'       ILEAUX = ',4x,i10,    ' (1: also read  suiamx        )',/,&
'       IECAUX = ',4x,i10,    ' (1: also write suiavx        )',/,&
                                                                /,&
'       suiamx and suiavx are the auxiliary restart files.',    /)
 7020 format(                                                     &
'       ISCOLD(I): In the previous calculation, number of',     /,&
'                  the scalar corresponding to scalar I in',    /,&
'                  the current calculation:',                   /,&
                                                                /,&
'-----------------------------------------------------------',  /,&
' Scalar           Number   <-   Old scalar number',            /,&
'-----------------------------------------------------------'    )
 7030 format(                                                     &
 1x,    a16,    i7,    7x,    i7                                 )
 7040 format(                                                     &
'---------------------------------------------------',          /,&
                                                                /,&
'   The preceding array (ISCOLD) defines the correspondance',   /,&
'     of scalars in the current calculation with those of the', /,&
'     previous calculation after possible user intervention.',  /,&
'   It consists of numbers 1 to NSCAL which locate the scalar', /,&
'     in the list of all user+specific physics scalars.',       /,&
'     .-999 is the default value if the user has not',          /,&
'           intervened. The correspondant''s numbers will',     /,&
'           be completed when reading the restart file, based', /,&
'           on the number of available scalars, using the',     /,&
'           rule (new scalar ii <- old scalar ii)',             /,&
'     .   0 if the user does not want the scalar of the',       /,&
'           current calculation to have a correspondant (i.e.', /,&
'           it is a new scalar).',                              /,&
'     .   n > 0 if the user wants the scalar of the current',   /,&
'           calculation to correspond to scalar n in the',      /,&
'           previous calculation.',                             /)
 7110 format(                                                     &
' --- Calculation time',                                        /,&
'     The numbering of time steps and the measure of simulated',/,&
'       physical time are absolute values, and not values',     /,&
'       relative to the current calculation.',                  /,&
                                                                /,&
'       INPDT0 = ',4x,i10,    ' (1: 0 time step calcuation   )',/,&
'       NTMABS = ',4x,i10,    ' (Final time step required    )',/)
 7210 format(                                                     &
' --- CPU time margin',                                         /,&
'       TMARUS = ', e14.5,    ' (CPU time margin before stop )',/)

#endif

!===============================================================================
! 8. ENTREES SORTIES
!===============================================================================

write(nfecra,7500)

!   - Fichier suite

write(nfecra,7510) ntsuit

!   - Fichiers Ensight

write(nfecra,7520)
do ii = 2, nvppmx
  if(ichrvr(ii).eq.1) then
    name = nomvar(ii)
    write(nfecra,7521) ii,name  (1:16)
  endif
enddo
write(nfecra,7522)

!   - Fichiers historiques
write(nfecra,7530) nthist,frhist,ncapt,nthsav
do ii = 2, nvppmx
  if(ihisvr(ii,1).ne.0) then
    name = nomvar(ii)
    write(nfecra,7531) ii,name  (1:16),ihisvr(ii,1)
  endif
enddo
write(nfecra,7532)

!   - Fichiers listing

write(nfecra,7540) ntlist
do ipp = 2, nvppmx
  ii = itrsvr(ipp)
  if(ii.ge.1) then
    iwar = iwarni(ii)
  else
    iwar = -999
  endif
  if(ilisvr(ipp).eq.1) then
    name = nomvar(ipp)
    write(nfecra,7531) ipp,name  (1:16),iwar
  endif
enddo
write(nfecra,7532)

!   - Post-traitement automatique (bord)

write(nfecra,7550)   'IPSTDV',ipstdv,                             &
                     'IPSTYP',ipstyp,                             &
                     'IPSTCL',ipstcl,                             &
                     'IPSTFT',ipstft,                             &
                     'IPSTFO',ipstfo,                             &
                     'IPSTDV'

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 7500 format(                                                     &
                                                                /,&
' ** ENTREES SORTIES',                                          /,&
'    ---------------',                                          /)
 7510 format(                                                     &
' --- Fichier suite',                                           /,&
'       NTSUIT = ',4x,i10,    ' (Periode de sauvegarde)',       /)
 7520 format(                                                     &
' --- Variables post-traitees',                                 /,&
                                                                /,&
'       Numero Nom'                                              )
 7521 format(i10,1X,          A16                                )
 7522 format(                                                     &
'         --           --',                                     /)
 7530 format(                                                     &
' --- Fichiers historiques',                                    /,&
'       NTHIST = ',4x,i10,    ' (Periode de sortie    )',       /,&
'       FRHIST = ',4x,e11.5,  ' (Periode de sortie (s))',       /,&
'       NCAPT  = ',4x,i10,    ' (Nombre de capteurs   )',       /,&
'       NTHSAV = ',4x,i10,    ' (Periode de sauvegarde)',       /,&
                                                                /,&
'       Numero Nom                   Nb. sondes (-1 : toutes)'   )
 7531 format(i10,1X,          A16,6X,         i10                )
 7532 format(                                                     &
'         --           --                --',                   /)
 7540 format(                                                     &
' --- Fichiers listing',                                        /,&
'       NTLIST = ',4x,i10,    ' (Periode de sortie    )',       /,&
                                                                /,&
'       Numero Nom                 Niveau d''impression IWARNI',/,&
'                                      (-999 : non applicable)',/)
 7550 format(                                                     &
' --- Variables supplementaires en post-traitement',            /,&
'       ',a6,' = ',4x,i10,    ' (Produit des valeurs suivantes',/,&
'                                selon activation ou non',      /,&
'       ',a6,' = ',4x,i10,    ' (Yplus          au bord',       /,&
'       ',a6,' = ',4x,i10,    ' (Variables      au bord',       /,&
'       ',a6,' = ',4x,i10,    ' (Flux thermique au bord',       /,&
'       ',a6,' = ',4x,i10,    ' (Force exercee  au bord',       /,&
'  et   ',a6,' =              1 (Pas de sortie supplementaire', /)

#else

 7500 format(                                                     &
                                                                /,&
' ** INPUT-OUTPUT',                                             /,&
'    ------------',                                             /)
 7510 format(                                                     &
' --- Restart file',                                            /,&
'       NTSUIT = ',4x,i10,    ' (Checkpoint frequency )',       /)
 7520 format(                                                     &
' --- Post-processed variables',                                /,&
                                                                /,&
'       Number Name'                                             )
 7521 format(i10,1X,          A16                                )
 7522 format(                                                     &
'         --           --',                                     /)
 7530 format(                                                     &
' --- Probe history files',                                     /,&
'       NTHIST = ',4x,i10,    ' (Output frequency     )',       /,&
'       FRHIST = ',4x,e11.5,  ' (Output frequency (s) )',       /,&
'       NCAPT  = ',4x,i10,    ' (Number of probes     )',       /,&
'       NTHSAV = ',4x,i10,    ' (Checkpoint frequency )',       /,&
                                                                /,&
'       Number Name                  Nb. probes (-1: all)'       )
 7531 format(i10,1X,          A16,6X,         i10                )
 7532 format(                                                     &
'         --           --                --',                   /)
 7540 format(                                                     &
' --- Log files',                                               /,&
'       NTLIST = ',4x,i10,    ' (Output frequency     )',       /,&
                                                                /,&
'       Number Name                IWARNI verbosity level',     /,&
'                                      (-999: not applicable)', /)
 7550 format(                                                     &
' --- Additional post-processing variables',                    /,&
'       ',a6,' = ',4x,i10,    ' (Product of the following',     /,&
'                                values based on activation  )',/,&
'       ',a6,' = ',4x,i10,    ' (Yplus          on boundary  )',/,&
'       ',a6,' = ',4x,i10,    ' (Variables      on boundary  )',/,&
'       ',a6,' = ',4x,i10,    ' (Thermal flow   on boundary  )',/,&
'       ',a6,' = ',4x,i10,    ' (Force exerted  on boundary  )',/,&
'  and  ',a6,' =              1 (No additional output        )',/)

#endif

!===============================================================================
! 9. COUPLAGES
!===============================================================================


! --- Couplage SYRTHES

!     RECUPERATION DU NOMBRE DE CAS DE COUPLAGE

call nbcsyr (nbccou)
!==========

if (nbccou .ge. 1) then

  write(nfecra,8000)
  write(nfecra,8010) nbccou

  nbsucp = 0
  nbvocp = 0

  do ii = 1, nbccou

     ! Add a new surface coupling if detected
     issurf = 0
     call tsursy(ii, issurf)
     nbsucp = nbsucp + issurf

     ! Add a new volume coupling if detected
     isvol = 0
     call tvolsy(ii, isvol)
     nbvocp = nbvocp + isvol

  enddo

  write(nfecra,8020) nbsucp, nbvocp
  write(nfecra,8030)
  do ii = 1, nscal
    chaine=nomvar(ipprtp(isca(ii)))
    write(nfecra,8031) chaine(1:16),ii,icpsyr(ii)
  enddo
  write(nfecra,8032)

  write(nfecra,9900)

endif


#if defined(_CS_LANG_FR)

 8000 format(                                                     &
                                                                /,&
' ** COUPLAGE SYRTHES',                                         /,&
'    ----------------',                                         /)
 8010 format(                                                     &
'       NBCCOU = ',4x,i10,    ' (Nombre de couplages         )',/)
 8020 format(                                                     &
'       dont', 8x,i10, ' couplage(s) surfacique(s)',/,            &
'       dont', 8x,i10, ' couplage(s) volumique(s)',/)
 8030 format(                                                     &
                                                                /,&
'  -- Scalaires couples',                                       /,&
'-------------------------------',                              /,&
' Scalaire         Numero ICPSYR',                              /,&
'-------------------------------'                                )
 8031 format(                                                     &
 1x,    a16,    i7,    i7                                        )
 8032 format(                                                     &
'-----------------------',                                      /,&
                                                                /,&
'       ICPSYR = 0 ou 1         (1 : scalaire couple SYRTHES )',/)

#else

 8000 format(                                                     &
                                                                /,&
' ** SYRTHES COUPLING',                                         /,&
'    ----------------',                                         /)
 8010 format(                                                     &
'       NBCCOU = ',4x,i10,    ' (Number of couplings         )',/)
 8020 format(                                                     &
'       with', 8x,i10, ' surface coupling(s)',/,                  &
'       with', 8x,i10, ' volume coupling(s)',/)
 8030 format(                                                     &
                                                                /,&
'  -- Coupled scalars',                                         /,&
'-------------------------------',                              /,&
' Scalar           Number ICPSYR',                              /,&
'-------------------------------'                                )
 8031 format(                                                     &
 1x,    a16,    i7,    i7                                        )
 8032 format(                                                     &
'-----------------------',                                      /,&
                                                                /,&
'       ICPSYR = 0 or 1         (1: scalar coupled to SYRTHES)',/)

#endif

!===============================================================================
! 10. Lagrangien
!===============================================================================

! --- Lagrangien

if (iilagr.ne.0) then
  write(nfecra,8100) iilagr, isuila, isuist, iphyla

  if (iphyla.eq.1) then
    write(nfecra,8105) idpvar, itpvar, impvar
  endif

  write(nfecra,8106) nbpmax, nvls, isttio, injcon, iroule

  if (iphyla.eq.2) then
    write(nfecra,8111) iencra
    do ii = 1,  ncharb
      write(nfecra,8112) ii, tprenc(ii),  ii
    enddo
    do ii = 1,  ncharb
      write(nfecra,8113) ii, visref(ii), ii
    enddo
  endif

  if (iilagr.eq.2) then
    write(nfecra,8120) nstits, ltsdyn, ltsmas, ltsthe
  endif

  write(nfecra,8130) istala
  if (istala.eq.1) then
    write(nfecra,8135) seuil, idstnt, nstist, nvlsts
  endif

  write(nfecra,8140) idistu, idiffl, modcpl

  if (modcpl.gt.0) write(nfecra,8141) idirla

  write(nfecra,8142) nordre, ilapoi

  write(nfecra,8150) iensi1, iensi2
  if (iensi1.eq.1 .or. iensi2.eq.1) then
    write(nfecra,8155) nbvis, nvisla, ivisv1, ivisv2,             &
                       ivistp, ivisdm, iviste, ivismp
    if (iphyla.eq.2) then
      write(nfecra,8156) ivishp, ivisdk, ivisch, ivisck
    endif
  endif

  write(nfecra,8160) iensi3
  if (iensi3.eq.1) then
    write(nfecra,8165) seuilf, nstbor,                            &
          inbrbd, iflmbd, iangbd, ivitbd, iencbd, nusbor
  endif

  write(nfecra,9900)

endif

#if defined(_CS_LANG_FR)

 8100 format(                                                     &
                                                                /,&
' ** ECOULEMENT DIPHASIQUE LAGRANGIEN',                         /,&
'    --------------------------------',                         /,&
' --- Phase continue :',                                        /,&
'       IILAGR = ',4x,i10,    ' (0 : Lagrangien desactive',     /,&
'                ',14x,       '  1 : one way coupling',         /,&
'                ',14x,       '  2 : two way coupling',         /,&
'                ',14x,       '  3 : sur champs figes        )',/,&
'       ISUILA = ',4x,i10,    ' (0 : pas de suite ; 1 : suite)',/,&
'       ISUIST = ',4x,i10,    ' (1 : suite de calcul stats et', /,&
'                ',14x,       '      TS de couplage retour   )',/,&
' --- Physique particuliere associee aux particules :',         /,&
'       IPHYLA = ',4x,i10,    ' (0 : pas d eqn supplementaires',/,&
'                ',14x,       '  1 : eqns sur Dp Tp Mp',        /,&
'                ',14x,       '  2 : particules de charbon   )'  )
 8105 format(                                                     &
'       IDPVAR = ',4x,i10,    ' (1 eqn diametre Dp,   0 sinon)',/,&
'       ITPVAR = ',4x,i10,    ' (1 eqn temperature Tp,0 sinon)',/,&
'       IMPVAR = ',4x,i10,    ' (1 eqn masse Mp,      0 sinon)'  )
 8106 format(                                                     &
' --- Parametres Globaux :',                                    /,&
'       NBPMAX = ',4x,i10,    ' (nb max de part par iteration)',/,&
'       NVLS   = ',4x,i10,    ' (nb var particulaires suppl. )',/,&
'       ISTTIO = ',4x,i10,    ' (1 phase porteuse stationnair)',/,&
'       INJCON = ',4x,i10,    ' (1 injection continue,0 sinon)',/,&
'       IROULE = ',4x,i10,    ' (2 clonage/fusion avec calc Y+',/,&
'                                1 clonage/fusion sans calc Y+',/,&
'                                0 sinon                     )'  )

 8111 format(                                                     &
' --- Options Charbon :',                                       /,&
'       IENCRA = ',4x,i10,    ' (1 : encrassement si charbon )'  )

 8112 format(                                                     &
'       TPRENC(',i1,') = ', e11.5,                                &
                              ' (temp seuil pour encrassement', /,&
'                ',14x,       '  charbon', i1,8x,'           )'  )

 8113 format(                                                     &
'       VISREF(',i1,') = ', e11.5,                                &
                         ' (viscosite critique charbon', i1,')'  )

 8120 format(                                                     &
' --- Options Couplage Retour :',                               /,&
'       NSTITS = ',4x,i10,    ' (iter de debut moy. en temps )',/,&
'       LTSDYN = ',4x,i10,    ' (1 couplage retour dynamique )',/,&
'       LTSMAS = ',4x,i10,    ' (1 couplage retour massique  )',/,&
'       LTSTHE = ',4x,i10,    ' (1 couplage retour thermique )'  )

 8130 format(                                                     &
' --- Options Statistiques :',                                  /,&
'       ISTALA = ',4x,i10,    ' (1 : calcul de statistiques  )'  )

 8135 format(                                                     &
'       SEUIL  = ', e14.5,    ' (val min de prise en compte  )',/,&
'       IDSTNT = ',4x,i10,    ' (iter de debut de calcul stat)',/,&
'       NSTIST = ',4x,i10,    ' (iter de debut moy. en temps )',/,&
'       NVLSTS = ',4x,i10,    ' (nb var statistiques suppl.  )'  )

 8140 format(                                                     &
' --- Options Dispersion Turbulente :',                         /,&
'       IDISTU = ',4x,i10,    ' (1 : prise en compte; 0 sinon)',/,&
'       IDIFFL = ',4x,i10,    ' (1 dispersion =diffusion turb)',/,&
'       MODCPL = ',4x,i10,    ' (iter lag debut model complet)'  )
 8141 format(                                                     &
'       IDIRLA = ',4x,i10,    ' (1 2 ou 3 : dir principal ect)'  )
 8142 format(                                                     &
' --- Options Numeriques :',                                    /,&
'       NORDRE = ',4x,i10,    ' (1 ou 2 ordre schema en temps)',/,&
'       ILAPOI = ',4x,i10,    ' (1 corr. vit instantannees   )'  )

 8150 format(                                                     &
' --- Options Postprocessing Trajectoires/Deplacement :',       /,&
'       IENSi1 = ',4x,i10,    ' (1 : post mode trajectoires  )',/,&
'       IENSi2 = ',4x,i10,    ' (1 : post mode deplacements  )'  )

 8155 format(                                                     &
'       NBVIS  = ',4x,i10,    ' (nb part max visualisables   )',/,&
'       NVISLA = ',4x,i10,    ' (periode d acquisition, 0 non)',/,&
'       IVISV1 = ',4x,i10,    ' (1 : vitesse fluide vu, 0 non)',/,&
'       IVISV2 = ',4x,i10,    ' (1 : vitesse particule, 0 non)',/,&
'       IVISTP = ',4x,i10,    ' (1 : temps de sejour,   0 non)',/,&
'       IVISDM = ',4x,i10,    ' (1 : diametre part.,    0 non)',/,&
'       IVISTE = ',4x,i10,    ' (1 : temperature part., 0 non)',/,&
'       IVISMP = ',4x,i10,    ' (1 : masse particule,   0 non)'  )

 8156 format(                                                     &
'       IVISHP = ',4x,i10,    ' (1 : temp/enthal pour charbon)',/,&
'       IVISDK = ',4x,i10,    ' (1 : diam coeur retrecissant )',/,&
'       IVISCH = ',4x,i10,    ' (1 : masse de charbon actif  )',/,&
'       IVISCK = ',4x,i10,    ' (1 : masse de coke           )'  )

 8160 format(                                                     &
' --- Options Stat des Interactions Particules/Frontieres :',   /,&
'       IENSI3 = ',4x,i10,    ' (1 calcul stat parietales    )'  )

 8165 format(                                                     &
'       SEUILF = ', e14.5,    ' (val min de prise en compte  )',/,&
'       NSTBOR = ',4x,i10,    ' (iter de debut moy. en temps )',/,&
'       INBRBD = ',4x,i10,    ' (1 : enr. nb d interactions  )',/,&
'       IFLMBD = ',4x,i10,    ' (1 : enr. flux de masse part.)',/,&
'       IANGBD = ',4x,i10,    ' (1 : enr. angle d interaction)',/,&
'       IVITBD = ',4x,i10,    ' (1 : enr. vitesse interaction)',/,&
'       IENCBD = ',4x,i10,    ' (1 : masse de charbon encrass)',/,&
'       NUSBOR = ',4x,i10,    ' (1 : enr. infos user suppl.  )'  )

#else

 8100 format(                                                     &
                                                                /,&
' ** TWO-PHASE LANGRANGIEN FLOW',                               /,&
'    --------------------------',                               /,&
' --- Continuous phase:',                                       /,&
'       IILAGR = ',4x,i10,    ' (0: Lagrangian deactivated',    /,&
'                ',14x,       '  1: one way coupling',          /,&
'                ',14x,       '  2: two way coupling',          /,&
'                ',14x,       '  3: on frozen fields         )',/,&
'       ISUILA = ',4x,i10,    ' (0: no restart; 1: restart   )',/,&
'       ISUIST = ',4x,i10,    ' (1: restart stats and return',  /,&
'                ',14x,       '     coupling ST              )',/,&
' --- Specific physics associated with particles:',             /,&
'       IPHYLA = ',4x,i10,    ' (0: no additional equations',   /,&
'                ',14x,       '  1: equations on Dp Tp Mp',     /,&
'                ',14x,       '  2: coal particles           )'  )
 8105 format(                                                     &
'       IDPVAR = ',4x,i10,    ' (1 eqn diameter Dp,      or 0)',/,&
'       ITPVAR = ',4x,i10,    ' (1 eqn temperature Tp,   or 0)',/,&
'       IMPVAR = ',4x,i10,    ' (1 eqn mass Mp,          or 0)'  )
 8106 format(                                                     &
' --- Global parameters:',                                      /,&
'       NBPMAX = ',4x,i10,    ' (nb max parts per iteration  )',/,&
'       NVLS   = ',4x,i10,    ' (nb add. suppl. variables    )',/,&
'       ISTTIO = ',4x,i10,    ' (1 steady carrier phase      )',/,&
'       INJCON = ',4x,i10,    ' (1 continuous injection, or 0)',/,&
'       IROULE = ',4x,i10,    ' (2 clone/merge with Y+ calc',   /,&
'                                1 clone/merge without Y+ calc',/,&
'                                0 otherwise                 )'  )

 8111 format(                                                     &
' --- Coal options:''''''''',                                   /,&
'       IENCRA = ',4x,i10,    ' (1: fouling if coal          )'  )

 8112 format(                                                     &
'       TPRENC(',i1,') = ', e11.5,                                &
                              ' (threshold temp. for coal',     /,&
'                ',14x,       '  fouling', i1,8x,'           )'  )

 8113 format(                                                     &
'       VISREF(',i1,') = ', e11.5,                                &
                         ' (critical coal viscosity',    i1,')'  )

 8120 format(                                                     &
' --- Return coupling options:',                                /,&
'       NSTITS = ',4x,i10,    ' (start iter for time average )',/,&
'       LTSDYN = ',4x,i10,    ' (1 dynamic return coupling   )',/,&
'       LTSMAS = ',4x,i10,    ' (1 mass return coupling      )',/,&
'       LTSTHE = ',4x,i10,    ' (1 thermal return coupling   )'  )

 8130 format(                                                     &
' --- Statistics options:',                                     /,&
'       ISTALA = ',4x,i10,    ' (1: compute statistics       )'  )

 8135 format(                                                     &
'       SEUIL  = ', e14.5,    ' (minimum value for handling  )',/,&
'       IDSTNT = ',4x,i10,    ' (start iter for stat calc    )',/,&
'       NSTIST = ',4x,i10,    ' (start iter for time avergage)',/,&
'       NVLSTS = ',4x,i10,    ' (nb add statistics variables )'  )

 8140 format(                                                     &
' --- Turbulent dispersion options:',                           /,&
'       IDISTU = ',4x,i10,    ' (1: accounted for; 0 otherws.)',/,&
'       IDIFFL = ',4x,i10,    ' (1 dispersion = turb diffus. )',/,&
'       MODCPL = ',4x,i10,    ' (complete model lag start it.)'  )
 8141 format(                                                     &
'       IDIRLA = ',4x,i10,    ' (1 2 ou 3: main ect dir)'        )
 8142 format(                                                     &
' --- Numerical options:',                                      /,&
'       NORDRE = ',4x,i10,    ' (1 or 2 time scheme order    )',/,&
'       ILAPOI = ',4x,i10,    ' (1 inst. velcity corr.       )'  )

 8150 format(                                                     &
' --- Trajectory/displacement postprocessing options:',         /,&
'       IENSi1 = ',4x,i10,    ' (1: post trajectories mode   )',/,&
'       IENSi2 = ',4x,i10,    ' (1: post displcaments mode   )'  )

 8155 format(                                                     &
'       NBVIS  = ',4x,i10,    ' (max nb visualizatlbe parts  )',/,&
'       NVISLA = ',4x,i10,    ' (acquisition period, 0 none  )',/,&
'       IVISV1 = ',4x,i10,    ' (1: fluid velocity vu, 0 none)', /&
'       IVISV2 = ',4x,i10,    ' (1: particle velocity, 0 none)',/,&
'       IVISTP = ',4x,i10,    ' (1: resident time,     0 none)',/,&
'       IVISDM = ',4x,i10,    ' (1: particle diameter, 0 none)',/,&
'       IVISTE = ',4x,i10,    ' (1: part. temperature, 0 none)',/,&
'       IVISMP = ',4x,i10,    ' (1: particle mass,     0 none)'  )

 8156 format(                                                     &
'       IVISHP = ',4x,i10,    ' (1: temp/enthalpy for coal   )',/,&
'       IVISDK = ',4x,i10,    ' (1: shrinking core diameter  )',/,&
'       IVISCH = ',4x,i10,    ' (1: active coal mass         )',/,&
'       IVISCK = ',4x,i10,    ' (1: coke mass                )'  )

 8160 format(                                                     &
' --- Statistics options for particles/boundary interaction:',  /,&
'       IENSI3 = ',4x,i10,    ' (1 calculate wall stats      )'  )

 8165 format(                                                     &
'       SEUILF = ', e14.5,    ' (minimul value for handlin   )',/,&
'       NSTBOR = ',4x,i10,    ' (start iter for time average )',/,&
'       INBRBD = ',4x,i10,    ' (1: nb interactions rec.     )',/,&
'       IFLMBD = ',4x,i10,    ' (1: particle mass flow rec.  )',/,&
'       IANGBD = ',4x,i10,    ' (1: interaction angle rec.   )',/,&
'       IVITBD = ',4x,i10,    ' (1: interaction velocity rec.)',/,&
'       IENCBD = ',4x,i10,    ' (1: fouling coal mass        )',/,&
'       NUSBOR = ',4x,i10,    ' (1: additional user info rec.)'  )

#endif

!===============================================================================
! 11. METHODE ALE
!===============================================================================
! --- Activation de la methode ALE

write(nfecra,8210)
write(nfecra,8220) iale, nalinf

write(nfecra,9900)


#if defined(_CS_LANG_FR)

 8210 format(                                                     &
                                                                /,&
' ** METHODE ALE (MAILLAGE MOBILE)',                            /,&
'    -----------',                                              /)
 8220 format(                                                     &
'       IALE   = ',4x,i10,    ' (1 : activee                 )',/ &
'       NALINF = ',4x,i10,    ' (Iterations d''initialisation', / &
'                                                   du fluide)',/)

#else

 8210 format(                                                     &
                                                                /,&
' ** ALE METHOD (MOVING MESH)',                                 /,&
'    -----------',                                              /)
 8220 format(                                                     &
'       IALE   = ',4x,i10,    ' (1: activated                )',/ &
'       NALINF = ',4x,i10,    ' (Fluid initialization',         / &
'                                                  iterations)',/)

#endif

!===============================================================================
! 12. FIN
!===============================================================================

return
end subroutine
