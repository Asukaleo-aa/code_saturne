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
!    dans usipph si on souhaite imposer une chaleur specifique
!    CP variable (sinon: ecrasement memoire).


!  Il FAUT AVOIR PRECISE IVISLS(Numero de scalaire) = 1
!     ==================
!     dans usipsc si on souhaite une diffusivite VISCLS variable
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
use dimens, only: ndimfb
use numvar
use optcal
use cstphy
use cstnum, only: pi
use entsor
use parall
use period
use ppppar
use ppthch
use ppincl
use mesh
use atincl

!===============================================================================

implicit none

! Arguments

integer          nvar, nscal

integer          ibrom
integer          izfppp(nfabor)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)

! Local variables

integer          ivart, iclvar, iel
integer          ipcrom, ipbrom, ipcvis, ipccp, ipctem, ipcliq
integer          ipcvsl, ith, iscal, ii
integer          iutile

double precision vara, varb, varc, varam, varbm, varcm, vardm
double precision                   varal, varbl, varcl, vardl
double precision                   varac, varbc
double precision xrtp, rhum, rscp, pp, zent
double precision lrhum, lrscp
double precision qsl, esat
double precision deltaq
double precision qliq
double precision qwt
double precision tliq

logical activate

! External function

double precision qsatliq
external qsatliq
! call as: qsatliq(temperature,pressure)

!===============================================================================
! 0. INITIALISATIONS A CONSERVER
!===============================================================================

activate = .FALSE.

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

! Computes the perfect gaz constants according to the physics

rhum = rair
rscp = rair/cp0

lrhum = rair
lrscp = rair/cp0

do iel = 1, ncel

  xrtp = rtp(iel,ivart) !  The thermal scalar is potential temperature

  if (ippmod(iatmos).ge.2) then  ! humid atmosphere
    lrhum = rair*(1.d0 + (rvsra - 1.d0)*rtp(iel, isca(itotwt)))
    lrscp = (rair/cp0)*(1.d0 + (rvsra - cpvcpa)*                    &
            rtp(iel,isca(itotwt)))
  endif

  ! Pressure profile from meteo file:
  zent = xyzcen(3,iel)
  call intprf &
       ! ===========
     ( nbmett, nbmetm,                                            &
       ztmet , tmmet , phmet , zent, ttcabs, pp )

  ! Temperature in Celsius in cell centers:
  ! ---------------------------------------
  ! law: T = theta * (p/ps) ** (Rair/Cp0)

  propce(iel, ipctem) = xrtp*(pp/ps)**lrscp
  propce(iel, ipctem) = propce(iel, ipctem) - tkelvi

  !   Density in cell centers:
  !   ------------------------
  !   law:    RHO       =   P / ( Rair * T(K) )

  propce(iel,ipcrom) = pp/(lrhum*xrtp)*(ps/pp)**lrscp

enddo

if (ippmod(iatmos).ge.2) then ! humid atmosphere physics
  ipcliq = ipproc(iliqwt)

  if (moddis.eq.1)then ! all or nothing condensation scheme
    call all_or_nothing()
  elseif (moddis.ge.2)then ! gaussian subgrid condensation scheme
    call gaussian()
  endif
endif ! (ippmod(iatmos).ge.2)


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
'@      ISCALT = ',I10                                         ,/,&
'@    Le calcul ne sera pas execute.                          ',/,&
'@                                                            ',/,&
'@    Verifier le codage de usphyv (et le test lors de la     ',/,&
'@      definition de IVART).                                 ',/,&
'@    Verifier la definition des variables de calcul dans     ',/,&
'@      usipsu. Si un scalaire doit jouer le role de la       ',/,&
'@      temperature, verifier que ISCALT a ete renseigne.     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

!----
! FIN
!----

return
contains

! *******************************************************************
! *
! *******************************************************************

subroutine all_or_nothing()

lrhum = rhum

do iel = 1, ncel

  !   Pressure profile from meteo file:
  zent = xyzcen(3,iel)
  call intprf &
       !   ===========
     ( nbmett, nbmetm,                                            &
       ztmet , tmmet , phmet , zent, ttcabs, pp )

  xrtp = rtp(iel,ivart) ! thermal scalar: liquid potential temperature
  tliq = xrtp*(pp/ps)**rscp ! liquid temperature
  qwt  = rtp(iel, isca(itotwt)) !total water content
  qsl = qsatliq(tliq, pp) ! saturated vapor content
  deltaq = qwt - qsl

  if (activate) then
    write(nfecra,*)"atphyv::all_or_nothing::xrtp = ",xrtp
    write(nfecra,*)"atphyv::all_or_nothing::tliq = ",tliq
    write(nfecra,*)"atphyv::all_or_nothing::qwt = ",qwt
    write(nfecra,*)"atphyv::all_or_nothing::qsl = ",qsl
    write(nfecra,*)"atphyv::all_or_nothing::qwt,qsl,deltaq = ",qwt,qsl,deltaq
    write(nfecra,*)"atphyv::all_or_nothing::zc = ",xyzcen(3,iel)
    write(nfecra,*)"atphyv::all_or_nothing::pp = ",pp
    write(nfecra,*)"atphyv::all_or_nothing::p0 = ",ps
    write(nfecra,*)"atphyv::all_or_nothing::zent = ",zent
  endif

  if (deltaq.le.0.d0) then ! unsaturated air parcel
    lrhum = rair*(1.d0 + (rvsra - 1.d0)*qwt)
    !Celcius temperature of the air parcel
    propce(iel, ipctem) = tliq - tkelvi
    !density of the air parcel
    propce(iel,ipcrom) = pp/(lrhum*tliq)
    !liquid water content
    propce(iel,ipcliq) = 0.d0
    nebdia(iel) = 0.d0
    nn(iel) = 0.d0
  else ! saturated (ie. with liquid water) air parcel
    qliq = deltaq/ &
         (1.d0 + qsl*clatev**2/(rair*rvsra*cp0*tliq**2))
    lrhum = rair*(1.d0 - qliq + (rvsra - 1.d0)*(qwt - qliq))
    ! liquid water content
    propce(iel,ipcliq) = qliq
    ! Celcius temperature of the air parcel
    propce(iel, ipctem) = tliq + (clatev/cp0)*qliq - tkelvi
    ! density
    propce(iel,ipcrom) = pp/(lrhum*(tliq + (clatev/cp0)*qliq))
    nebdia(iel) = 1.d0
    nn(iel) = 0.d0
  endif

enddo ! iel = 1, ncel
end subroutine all_or_nothing

! *******************************************************************
! *
! *******************************************************************

subroutine gaussian()
! subgrid condensation scheme assuming a gaussian distribution for the
! fluctuations of both qw and thetal.
double precision, dimension(:,:), allocatable :: dtlsd
double precision, dimension(:,:), allocatable :: dqsd

double precision a_const
double precision a_coeff
double precision alpha,al
double precision sig_flu ! standard deviation of qw'-alpha*theta'
double precision var_tl,var_q,cov_tlq
double precision q1,qsup

! rvap = rair*rvsra

allocate(dtlsd(ncelet,3))
allocate(dqsd(ncelet,3))

! ---------------------------
! computation of grad(thetal)
! ---------------------------
call grad_thetal(dtlsd)

! ---------------------------
! computation of grad(qw)
! ---------------------------
call grad_qw(dqsd)

! -------------------------------------------------------------
! gradients are used for estimating standard deviations of the
! subgrid fluctuations
! -------------------------------------------------------------

lrhum = rhum

a_const = 2.d0*cmu/2.3d0
do iel = 1, ncel

  a_coeff = a_const*rtp(iel, ik )**3/rtp(iel,iep)**2 ! 2 cmu/c2 * k**3 / eps**2
  var_tl= a_coeff*(dtlsd(iel,1)**2 + dtlsd(iel,2)**2 + dtlsd(iel,3)**2)
  var_q = a_coeff*( dqsd(iel,1)**2 + dqsd(iel,2)**2 + dqsd(iel,3)**2)
  cov_tlq = a_coeff*(dtlsd(iel,1)*dqsd(iel,1) + dtlsd(iel,2)*dqsd(iel,2)        &
          + dtlsd(iel,3)*dqsd(iel,3))
  zent = xyzcen(3,iel)

  call intprf &
     ( nbmett, nbmetm,                                                          &
       ztmet , tmmet , phmet , zent, ttcabs, pp )

  xrtp = rtp(iel,ivart) ! thermal scalar: liquid potential temperature
  tliq = xrtp*(pp/ps)**rscp ! liquid temperature
  qwt  = rtp(iel, isca(itotwt)) ! total water content
  qsl = qsatliq(tliq, pp) ! saturated vapor content
  deltaq = qwt - qsl
  alpha = (clatev*qsl/(rvap*tliq**2))*(pp/ps)**rscp
  sig_flu = sqrt(var_q + alpha**2*var_tl - 2.d0*alpha*cov_tlq)

  if (sig_flu.lt.1.d-30) sig_flu = 1.d-30
  q1 = deltaq/sig_flu
  al = 1.d0/(1.d0 + qsl*clatev**2/(rair*rvsra*cp0*tliq**2))
  qsup = qsl/sig_flu

  nebdia(iel) = 0.5d0*(1.d0 + erf(q1/sqrt(2.d0)))

  qliq = (sig_flu                                                               &
        /(1.d0 + qsl*clatev**2/(rvap*cp0*tliq**2)))                             &
        *(nebdia(iel)*q1 + exp(-q1**2/2.d0)/sqrt(2.d0*pi))
  qliq = max(qliq,1d-15)
  nn(iel) = nebdia(iel) - (nebdia(iel)*q1                                       &
          + exp(-q1**2/2.d0)/sqrt(2.d0*pi))*exp(-q1**2/2.d0)/sqrt(2.d0*pi)

  if(qwt.lt.qliq)then
    ! go back to all or nothing
    if (deltaq.le.0.d0) then ! unsaturated air parcel
      lrhum = rair*(1.d0 + (rvsra-1.d0)*qwt)
      !Celcius temperature of the air parcel
      propce(iel, ipctem) = tliq - tkelvi
      !density of the air parcel
      propce(iel,ipcrom) = pp/(lrhum*tliq)
      !liquid water content
      propce(iel,ipcliq) = 0.d0
      nebdia(iel) = 0.d0
      nn(iel) = 0.d0
    else ! saturated (ie. with liquid water) air parcel
      qliq = deltaq                                                             &
            /(1.d0 + qsl*clatev**2/(rair*rvsra*cp0*tliq**2))
      lrhum = rair*(1.d0 - qliq + (rvsra - 1.d0)*(qwt - qliq))
      ! liquid water content
      propce(iel,ipcliq) = qliq
      ! Celcius temperature of the air parcel
      propce(iel,ipctem) = tliq+(clatev/cp0)*qliq - tkelvi
      ! density
      propce(iel,ipcrom) = pp/(lrhum*(tliq + (clatev/cp0)*qliq))
      nebdia(iel) = 1.d0
      nn(iel) = 0.d0
    endif
  else ! coherent subgrid diagnostic
    lrhum = rair*(1.d0 - qliq + (rvsra - 1.d0)*(qwt - qliq))
    ! liquid water content
    propce(iel,ipcliq) = qliq
    !Celcius temperature of the air parcel
    propce(iel, ipctem) = tliq + (clatev/cp0)*qliq - tkelvi
    !density
    propce(iel,ipcrom) = pp/(lrhum*(tliq + (clatev/cp0)*qliq))
  endif ! qwt.lt.qliq

enddo

! when properly finished deallocate dtlsd
deallocate(dtlsd)
deallocate(dqsd)

end subroutine gaussian

! *******************************************************************
! *
! *******************************************************************

subroutine grad_thetal(dtlsd)
double precision dtlsd(ncelet,3)

double precision climgp
double precision epsrgp
double precision extrap

integer    iccocg
integer    icltpp
integer    iivar
integer    imligp
integer    inc
integer    iphydp
integer    itpp
integer    iwarnp
integer    nswrgp

! Computation of the gradient of the potential temperature

itpp = isca(iscalt)
icltpp = iclrtp(itpp,icoef)

! options for gradient calculation

iccocg = 1
inc = 1

nswrgp = nswrgr(itpp)
epsrgp = epsrgr(itpp)
imligp = imligr(itpp)
iwarnp = iwarni(itpp)
climgp = climgr(itpp)
extrap = extrag(itpp)

iivar = itpp

call grdcel                                                     &
     !==========
   ( iivar  , imrgra , inc    , iccocg , nswrgp ,imligp,            &
     iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
     rtpa(1,itpp), coefa(1,icltpp) , coefb(1,icltpp) ,              &
     dtlsd  )

end subroutine grad_thetal

! *******************************************************************
! *
! *******************************************************************

subroutine grad_qw(dqsd)
double precision dqsd(ncelet,3)

double precision climgp
double precision epsrgp
double precision extrap

integer    iccocg
integer    iclqw
integer    iivar
integer    imligp
integer    inc
integer    iphydp
integer    iqw
integer    iwarnp
integer    nswrgp

! ----------------------------------------------------------------
! now gradient of total humidity
! ----------------------------------------------------------------

iccocg = 1
inc = 1

iqw = isca(itotwt)
iclqw = iclrtp(iqw,icoef)
nswrgp = nswrgr(iqw)
epsrgp = epsrgr(iqw)
imligp = imligr(iqw)
iwarnp = iwarni(iqw)
climgp = climgr(iqw)
extrap = extrag(iqw)

iivar = iqw

call grdcel                                                     &
     !==========
    (iivar  , imrgra , inc    , iccocg , nswrgp ,imligp,            &
     iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
     rtpa(1,iqw), coefa(1,iclqw) , coefb(1,iclqw) ,              &
     dqsd   )

end subroutine grad_qw
end subroutine atphyv
