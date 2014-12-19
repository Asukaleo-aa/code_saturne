!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2014 EDF S.A.
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

subroutine ctphyv &
!================

 ( rtp    , propce )

!===============================================================================
! FONCTION :
! --------

!   REMPLISSAGE DES VARIABLES PHYSIQUES : Version Aerorefrigerants


! ATTENTION :
! =========


! Il est INTERDIT de modifier la viscosite turbulente VISCT ici
!        ========
!  (une routine specifique est dediee a cela : usvist)


!  Il FAUT AVOIR PRECISE ICP = 1
!     ==================
!    si on souhaite imposer une chaleur specifique
!    CP variable (sinon: ecrasement memoire).


!  Il FAUT AVOIR PRECISE IVISLS(Numero de scalaire) = 1
!     ==================
!     si on souhaite une diffusivite VISCLS variable
!     pour le scalaire considere (sinon: ecrasement memoire).




! Remarques :
! ---------

! Cette routine est appelee au debut de chaque pas de temps

!    Ainsi, AU PREMIER PAS DE TEMPS (calcul non suite), les seules
!    grandeurs initialisees avant appel sont celles donnees
!      - dans usipsu :
!             . la masse volumique (initialisee a RO0)
!             . la viscosite       (initialisee a VISCL0)
!      - dans usiniv :
!             . les variables de calcul  (initialisees a 0 par defaut
!             ou a la valeur donnee dans usiniv)

! On peut donner ici les lois de variation aux cellules
!     - de la masse volumique                      ROM    kg/m3
!         (et eventuellememt aux faces de bord     ROMB   kg/m3)
!     - de la viscosite moleculaire                VISCL  kg/(m s)
!     - de la chaleur specifique associee          CP     J/(kg degres)
!     - des "diffusivites" associees aux scalaires VISCLS kg/(m s)


! On dispose des types de faces de bord au pas de temps
!   precedent (sauf au premier pas de temps, ou les tableaux
!   ITYPFB et ITRIFB n'ont pas ete renseignes)




! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! rtp              ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current time step)                        !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
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
use optcal
use dimens, only: nvar
use cstnum
use cstphy
use entsor
use ppppar
use ppthch
use ppincl
use mesh
use field
!===============================================================================

implicit none

! Arguments

double precision rtp(ncelet,nflown:nvar)
double precision propce(ncelet,*)

! Local variables

integer          iel
integer          ipccp
integer          ivart

double precision rho   , r     , cpa   , cpe , cpv , del
double precision hv0 , hvti , rhoj , tti , xxi,  xsati , dxsati
double precision rho0 , t00 , p00 , t1
double precision, dimension(:), pointer ::  crom
integer          ipass
data             ipass /0/
save             ipass

!===============================================================================
!===============================================================================
! 0 - INITIALISATIONS A CONSERVER
!===============================================================================

! --- Initialisation memoire


ipass = ipass + 1

!===============================================================================
! 1 - MASSE VOLUMIQUE
!===============================================================================

!   Positions des variables, coefficients
!   -------------------------------------

! --- Numero de variable thermique
!       (et de ses conditions limites)

ivart = isca(itemp4)

! --- Rang de la masse volumique
!     dans PROPCE, prop. physiques au centre des elements       : IPCROM

call field_get_val_s(icrom, crom)

! --- Coefficients des lois choisis et imposes par l'utilisateur
!       Les valeurs donnees ici sont fictives

rho0 = 1.293d0
t00  = 273.15d0
p00  = 101325.d0
del  = 0.622d0
t1   = 273.15d0
r    = rr


!   Masse volumique au centre des cellules
!   ---------------------------------------

do iel = 1, ncel

  tti = rtp(iel,isca(itemp4))
  xxi = rtp(iel,isca(ihumid))

  call xsath(tti,xsati)
  !==========

  if (xxi .le. xsati) then

    rho = rho0*t00/(tti+t1)*del/(del+xxi)

  else

    if (tti.le.0.d0) then
      rhoj = 917.0d0
    else
      rhoj = 998.36d0 - 0.4116d0*(tti-20.d0)                    &
           - 2.24d0*(tti-20.d0)*(tti-70.d0)/625.d0
    endif

    rho = 1.0d0/((tti+t1)*p00/(t00*p00*rho0*del)                &
         *(del+xsati)+(xxi-xsati)/rhoj)

  endif

  if (rho .lt. 0.1d0) rho = 0.1d0
  crom(iel) = rho

enddo

!===============================================================================
! 2 - CHALEUR SPECIFIQUE
!===============================================================================

!   Positions des variables, coefficients
!   -------------------------------------

! --- Numero de variable thermique

ivart = isca(itemp4)

! --- Rang de la chaleur specifique
!     dans PROPCE, prop. physiques au centre des elements       : IPCCP

if(icp.gt.0) then
  ipccp  = ipproc(icp   )
else
  ipccp  = 0
endif

! --- Stop si CP n'est pas variable

if(ipccp.le.0) then
  write(nfecra,1000) icp
  call csexit (1)
endif


! --- Coefficients des lois choisis et imposes par l'utilisateur
!       Les valeurs donnees ici sont fictives

cpa    = 1006.0d0
cpv    = 1831.0d0
cpe    = 4179.0d0
hv0    = 2501600.0d0


!   Chaleur specifique J/(kg degres) au centre des cellules
!   --------------------------------------------------------

if (ippmod(iaeros).eq.1) then

  do iel = 1, ncel

    tti = rtp(iel,isca(itemp4))
    xxi = rtp(iel,isca(ihumid))

    call xsath(tti,xsati)
    !==========

    if (xxi .le. xsati) then
      propce(iel,ipccp) = cpa + xxi*cpv
    else
      hvti = (cpv-cpe)*tti + hv0
      call dxsath(tti,dxsati)
      !==========
      propce(iel,ipccp) = cpa + xsati*cpv +                     &
           (xxi-xsati)*cpe + dxsati*hvti
    endif

  enddo

elseif (ippmod(iaeros).eq.2) then

  do iel = 1, ncel

    tti = rtp(iel,isca(itemp4))

    call xsath(tti,xsati)
    !==========

    hvti = cpv*tti + hv0

    call dxsath(tti,dxsati)
    !==========

    propce(iel,ipccp) = cpa + xsati*cpv + dxsati*hvti

  enddo

endif


!===============================================================================
! 3 - ON PASSE LA MAIN A L'UTILISATEUR
!===============================================================================



! La masse volumique au bord est traitee dans phyvar (recopie de la valeur
!     de la cellule de bord).

!--------
! FORMATS
!--------

 1000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET LORS DU CALCUL DES GRANDEURS PHYSIQUES',/,&
'@    =========                                               ',/,&
'@    DONNEES DE CALCUL INCOHERENTES                          ',/,&
'@                                                            ',/,&
'@      la chaleur specifique est uniforme '                  ,/,&
'@        ICP = ',I10   ,' alors que                          ',/,&
'@      usphyv impose une chaleur specifique variable.        ',/,&
'@                                                            ',/,&
'@    Le calcul ne sera pas execute.                          ',/,&
'@                                                            ',/,&
'@    Modifier les parametres ou usphyv.                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

!----
! FIN
!----

return
end subroutine
