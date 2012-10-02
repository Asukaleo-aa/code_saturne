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

!> \file radiat.f90
!> Module for Radiation

module radiat

  !===========================================================================

  use ppppar

  !===========================================================================

  !-->  IIRAYO = 0 pas de rayonnement, 1 DOM, 2 P-1
  !-->  NRPHAS = 1 (phase qui rayonne) augmentee eventuellement
  !              du nombre de classe (Charbon, Fioul)
  !-->  IIMPAR = 0,1,2 niveau d'impression du calcul des temperatures de paroi
  !-->  IIMLUM = 0,1,2 niveau d'impression de la resolution luminance
  !-->  IMODAK = 1 calcul du coefficient d'absorption a l'aide de Modak
  !            = 0 on n'utilise pas Modak

  integer, save :: iirayo, nrphas, iimpar, iimlum, imodak

  !--> pointeur dans le macrotableau PROPCE :

  !                       ITSRE --> Terme source explicite
  !                       ITSRI --> Terme source implicite
  !                       IQX,IQY,IQZ --> Composantes du vecteur densite de flux radiatif
  !                       IABS --> part d'absorption dans le terme source explicite
  !                       IEMI --> part d'emission dans le terme source explicite
  !                       ICAK --> coefficient d'absorption
  !                       ILUMIN --> POINTEUR QUI PERMET DE REPERER L INTEGRALE DE LA
  !                                  LUMINANCE DANS LA TABLEAU PROPCE

  integer, save ::  itsre(1+nclcpm) , itsri(1+nclcpm) ,                      &
                    iqx   ,   iqy   , iqz   ,                                &
                    iabs(1+nclcpm)  , iemi(1+nclcpm)  , icak(1+nclcpm)  ,    &
                    ilumin

  !--> pointeur dans le macrotableau PROPFB :
  !                       ITPARO --> temperature de paroi
  !                       IQINCI --> densite de flux incident radiatif
  !                       IXLAM  --> conductivite thermique de la paroi
  !                       IEPA   --> epaisseur de la paroi
  !                       IEPS   --> emissivite de la paroi
  !                       IFNET  --> Flux Net radiatif
  !                       IFCONV --> Flux Convectif
  !                       IHCONV --> Coef d'echange fluide

  integer, save ::  itparo, iqinci, ixlam, iepa, ieps, ifnet,               &
                    ifconv, ihconv

  !--> XNP1MX : pour le modele P-1,
  !     pourcentage de cellules pour lesquelles on admet que l'epaisseur
  !     optique depasse l'unite bien que ce ne soit pas souhaitable

  double precision, save ::  xnp1mx

  !--> ISTPP1 : pour le modele P-1,
  !     indicateur d'arret mis a 1 dans ppcabs si le pourcentage de cellules
  !     pour lesquelles l'epaisseur optique depasse l'unite est superieur a
  !     XNP1MX  (on s'arrete a la fin du pas de temps)

  integer, save ::           istpp1

  !--> IDIVER =0 1 ou 2 suivant le calcul du terme source explicite

  integer, save ::           idiver

  !--> parametre sur le nombre de directions de discretisation angulaire

  integer     ndirs8
  parameter ( ndirs8 = 16 )

  !--> suite de calcul (0 : non, 1 : oui)

  integer, save ::           isuird

  !--> frequence de passage dans le module (=1 si tous les pas de temps)

  integer, save ::           nfreqr

  !--> nombre de bandes spectrales

  integer, save :: nbande

  !--> nombre de directions de discretisation angulaire

  integer, save :: ndirec

  !--> Informations sur les zones frontieres

  ! NBZRDM Nombre max. de  zones frontieres
  ! NOZRDM Numero max. des zones frontieres

  integer    nbzrdm
  parameter (nbzrdm=2000)
  integer    nozrdm
  parameter (nozrdm=2000)

  ! NZFRAD Nombre de zones de bord (sur le proc courant)
  ! ILZRAY Liste des numeros de zone de bord (du proc courant)
  ! NOZARM Numero de zone de bord atteint max
  !   exemple zones 1 4 2 : NZFRAD=3,NOZARM=4

  integer, save ::           nozarm, nzfrad, ilzrad(nbzrdm)

  !--> Types de condition pour les temperatures de paroi :
  !       ITPIMP Profil de temperature imposee
  !       IPGRNO Parois grises ou noires
  !       IPREFL Parois reflechissante
  !       IFGRNO Flux de conduction impose dans la paroi
  !                   ET paroi non reflechissante (EPS non nul)
  !       IFREFL Flux de conduction impose dans la paroi
  !                   ET paroi reflechissante     (EPS = 0)

  integer   itpimp   , ipgrno   , iprefl   , ifgrno   , ifrefl
  parameter(itpimp=1 , ipgrno=21, iprefl=22, ifgrno=31, ifrefl=32)

  !--> sortie postprocessing sur les facettes de bord

  integer     nbrayf
  parameter ( nbrayf = 8 )

  character*80, save ::       nbrvaf(nbrayf)
  integer, save ::            irayvf(nbrayf)

  integer     itparp   , iqincp   , ixlamp   , iepap    , &
              iepsp    , ifnetp   , ifconp   , ihconp
  parameter ( itparp=1 , iqincp=2 , ixlamp=3 , iepap=4  , &
              iepsp=5  , ifnetp=6 , ifconp=7 , ihconp=8 )

  !=============================================================================

end module radiat
