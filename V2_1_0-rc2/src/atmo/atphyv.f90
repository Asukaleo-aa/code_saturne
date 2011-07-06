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

subroutine atphyv &
!================

 ( nvar   , nscal  ,                                              &
   ibrom  , izfppp ,                                              &
   dt     , rtp    , rtpa   ,                                     &
   propce , propfa , propfb ,                                     &
   coefa  , coefb  )

!===============================================================================
! FONCTION :
! --------

! REMPLISSAGE DES VARIABLES PHYSIQUES : Atmospheric Version


! ATTENTION :
! =========

! Il est INTERDIT de modifier la viscosite turbulente VISCT ici
!        ========
!  (une routine specifique est dediee a cela : usvist)

!  Il FAUT AVOIR PRECISE ICP = 1
!     ==================
!    dans usini1 si on souhaite imposer une chaleur specifique
!    CP variable (sinon: ecrasement memoire).


!  Il FAUT AVOIR PRECISE IVISLS(Numero de scalaire) = 1
!     ==================
!     dans usini1 si on souhaite une diffusivite VISCLS variable
!     pour le scalaire considere (sinon: ecrasement memoire).




! Remarques :
! ---------

! Cette routine est appelee au debut de chaque pas de temps

!    Ainsi, AU PREMIER PAS DE TEMPS (calcul non suite), les seules
!    grandeurs initialisees avant appel sont celles donnees
!      - dans usini1 :
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


! Il est conseille de ne garder dans ce sous programme que
!    le strict necessaire.



! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ibrom            ! te ! <-- ! indicateur de remplissage de romb              !
!        !    !     !                                                !
! izfppp           ! te ! <-- ! numero de zone de la face de bord              !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
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
use cstphy
use entsor
use parall
use period
use ppppar
use atincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          ibrom
integer          izfppp(nfabor)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)

! Local variables

integer          ivart, iclvar, iel
integer          ipcrom, ipbrom, ipcvis, ipccp, ipctem
integer          ipcvsl, ith, iscal, ii
integer          iutile

double precision vara, varb, varc, varam, varbm, varcm, vardm
double precision                   varal, varbl, varcl, vardl
double precision                   varac, varbc
double precision xrtp,pp,zent

!===============================================================================



!===============================================================================
! 0. INITIALISATIONS A CONSERVER
!===============================================================================

! Initialize variables to avoid compiler warnings

ivart = -1

! --- Initialisation memoire



! This routine computes the density and the thermodynamic temperature.
! The computations require the pressure profile which is here taken from
! the meteo file. If no meteo file is used, the user should
! give the laws for RHO and T in usphyv.f90

if (imeteo.eq.0) return

!===============================================================================

!   Positions des variables, coefficients
!   -------------------------------------

! --- Numero de variable thermique
!       (et de ses conditions limites)
!       (Pour utiliser le scalaire utilisateur 2 a la place, ecrire
!          IVART = ISCA(2)

if (iscalt.gt.0) then
  ivart = isca(iscalt)
else
  write(nfecra,9010) iscalt
  call csexit (1)
endif

! --- Position des conditions limites de la variable IVART

iclvar = iclrtp(ivart,icoef)

! --- Rang de la masse volumique
!     dans PROPCE, prop. physiques au centre des elements       : IPCROM
!     dans PROPFB, prop. physiques au centre des faces de bord  : IPBROM

ipcrom = ipproc(irom)
ipbrom = ipprob(irom)
ipctem = ipproc(itempc)

! From potential temperature, compute:
! - Temperature in Celsius
! - Density
! ----------------------

do iel = 1, ncel

  xrtp = rtp(iel,ivart) !  The thermal scalar is potential temperature

  !   Pressure profile from meteo file:
  zent=xyzcen(3,iel)
  call intprf &
  !===========
  ( nbmett, nbmetm,                          &
    ztmet , tmmet , phmet , zent, ttcabs, pp )

  !   Temperature in Celsius in cell centers:
  !   ---------------------------------------
  !   law: T = theta * (p/psol) ** (Rair/Cp0)

  propce(iel, ipctem) = xrtp*(pp/p0)**(rair/cp0)
  propce(iel, ipctem) = propce(iel, ipctem) - tkelvi

  !   Density in cell centers:
  !   ------------------------
  !   law:    RHO       =   P / ( Rair * T(K) )

  propce(iel,ipcrom) = pp/(rair*xrtp)*(p0/pp)**(rair/cp0)

enddo


!===============================================================================
! FORMATS
!----

 9010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET LORS DU CALCUL DES GRANDEURS PHYSIQUES',/,&
'@    =========                                               ',/,&
'@    APPEL A csexit DANS LE SOUS PROGRAMME atphyv            ',/,&
'@                                                            ',/,&
'@    La variable dont dependent les proprietes physiques ne  ',/,&
'@      semble pas etre une variable de calcul.               ',/,&
'@    En effet, on cherche a utiliser la temperature alors que',/,&
'@      ISCALT = ',I10                                  ,/,&
'@    Le calcul ne sera pas execute.                          ',/,&
'@                                                            ',/,&
'@    Verifier le codage de usphyv (et le test lors de la     ',/,&
'@      definition de IVART).                                 ',/,&
'@    Verifier la definition des variables de calcul dans     ',/,&
'@      usini1. Si un scalaire doit jouer le role de la       ',/,&
'@      temperature, verifier que ISCALT a ete renseigne.     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

!----
! FIN
!----

return
end subroutine
