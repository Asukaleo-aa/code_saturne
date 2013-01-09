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

!===============================================================================
! Function:
! ---------

!> \file prehyd.f90
!>
!> \brief Compute a "a priori" hydrostatic pressure and its gradient associated
!> before the Navier Stokes equations (prediction and correction steps).
!>
!> This function computes a hysdrostatic pressure \f$ P_{hydro} \f$ solving the
!> a priori reduced momentum equation:
!> \f[
!> \rho^n \dfrac{(u^{hydro} - u^n)}{\Delta t} =
!>                                   \rho ^n\vect{g}^n - \nabla P_{hydro}
!> \f]
!> and using the mass conservative equation as following:
!> \f[
!> \rho ^n \divs \left(  \delta \vect{u}_{hydro} \right) = 0
!> \f]
!> with: \f$ \delta \vect{u}_{hydro} = ( \vect{u}^{hydro} - \vect{u}^n) \f$
!>
!> finally, we resolve the reduced momentum equation below:
!> \f[
!> \divs \left( k_t \grad P_{hydro} \right) = \divs \left(\vect{g}\right)
!> \f]
!> with the diffusion coefficient (\f$ k_t \f$) defined as :
!> \f[
!>      k_t := \dfrac{1}{\rho^n}
!> \f]
!> and the hydrostatic pressure boundary condition:
!> \f[
!> \left( k_t \grad P_{hydro} \cdot \vect{n}\right )_{b} =
!>                                   \left( \vect{g} \cdot \vec{n} \right)_{b}
!> \f]
!>
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     nvar          total number of variables
!> \param[in]     nscal         total number of scalars
!> \param[in]     dt            time step (per cell)
!> \param[in]     rtp, rtpa     calculated variables at cell centers
!>                                (at current and previous time steps)
!> \param[in]     propce        physical properties at interior face centers
!> \param[in]     propfa        physical properties at interior face centers
!> \param[in]     propfb        physical properties at boundary face centers
!> \param[in,out] prhyd         hydrostatic pressure predicted with
!>                              the a priori qdm equation reduced
!>                              \f$ P_{hydro} \f$
!> \param[out]    grdphd         the a priori hydrostatic pressure gradient
!>                              \f$ \partial _x (P_{hydro}) \f$
!_______________________________________________________________________________

subroutine prehyd &
!================

 ( nvar   , nscal  ,                                              &
   dt     , rtp    ,  rtpa  , propce , propfa , propfb ,          &
   prhyd , grdphd  )

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens, only: ndimfb
use numvar
use entsor
use cstphy
use cstnum
use optcal
use pointe, only: itypfb
use albase
use parall
use period
use mltgrd
use lagpar
use lagran
use cplsat
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision prhyd(ncelet), grdphd(ncelet,ndim)

! Local variables

integer          iccocg, inc, isym  , ipol  , isqrt
integer          iel   , ifac
integer          ireslp
integer          nswrgp, imligp, iwarnp
integer          ipcrom, iflmas, iflmab
integer          ipp
integer          idiffp, iconvp, ndircp
integer          nitmap, imgrp
integer          ibsize
integer          iescap, ircflp, ischcp, isstpp, ivar, ncymxp, nitmfp
integer          nswrsp
integer          imucpp, idftnp, iswdyp
integer          iharmo

double precision thetap
double precision epsrgp, climgp, extrap, epsilp
double precision snorm
double precision hint, qimp, epsrsp, blencp, relaxp

double precision rvoid(1)

double precision, allocatable, dimension(:) :: drtp

double precision, allocatable, dimension(:) :: coefap, cofafp, coefbp, cofbfp

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: xinvro
double precision, allocatable, dimension(:) :: dpvar
double precision, allocatable, dimension(:) :: smbr, rovsdt

!===============================================================================

!===============================================================================
! 1. Initializations
!===============================================================================

! Allocate temporary arrays

! Boundary conditions for delta P
allocate(coefap(nfabor), cofafp(nfabor), coefbp(nfabor), cofbfp(nfabor))

! --- Prints
ipp    = ipprtp(ipr)

! --- Physical properties
ipcrom = ipproc(irom  )
iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))

! --- Resolution options
isym  = 1
if (iconv (ipr).gt.0) then
  isym  = 2
endif

! --- Matrix block size
ibsize = 1

if (iresol(ipr).eq.-1) then
  ireslp = 0
  ipol   = 0
  if (iconv(ipr).gt.0) then
    ireslp = 1
    ipol   = 0
  endif
else
  ireslp = mod(iresol(ipr),1000)
  ipol   = (iresol(ipr)-ireslp)/1000
endif

isqrt = 1

!===============================================================================
! 2. Solving a diffusion equation with source term to obtain
!    the a priori hydrostatic pressure
!===============================================================================

! --- Allocate temporary arrays
allocate(drtp(ncelet))
allocate(dpvar(ncelet))
allocate(viscf(nfac), viscb(nfabor))
allocate(xinvro(ncelet))
allocate(smbr(ncelet), rovsdt(ncelet))

! --- Initialization of the variable to solve from the interior cells
do iel = 1, ncel
  xinvro(iel) = 1.d0/propce(iel,ipcrom)
  rovsdt(iel) = 0.d0
  drtp(iel)   = 0.d0
  smbr(iel)   = 0.d0
enddo

! --- Viscosity (k_t := 1/rho )
iharmo = 1
call viscfa (iharmo, xinvro, viscf, viscb)

! Neumann boundary condition for the pressure increment
!------------------------------------------------------

do ifac = 1, nfabor

  iel = ifabor(ifac)

  ! Prescribe the pressure gradient: kt.grd(Phyd)|_b = (g.n)|_b

  hint = 1.d0 /(propce(iel,ipcrom)*distb(ifac))
  qimp = - (gx*surfbo(1,ifac)                &
           +gy*surfbo(2,ifac)                &
           +gz*surfbo(3,ifac))/(surfbn(ifac))

  call set_neumann_scalar &
  !======================
  ( coefap(ifac), cofafp(ifac),             &
    coefbp(ifac), cofbfp(ifac),             &
    qimp        , hint )

enddo

! --- Solve the diffusion equation

!--------------------------------------------------------------------------
! We use a conjugate gradient to solve the diffusion equation (ireslp = 0)

! By default, the hydrostatic pressure variable is resolved with 5 sweeps for
! the reconstruction gradient. Here we make the assumption that the mesh
! is orthogonal (any reconstruction gradient is done for the hydrostatic
! pressure variable)

! We do not yet use the multigrid to resolve the hydrostatic pressure
!--------------------------------------------------------------------------

!TODO later: define argument additionnal to pass to codits for work variable
! like prhyd to obtain the warning with namewv(ipwv) = 'Prhydo'

ivar   = ipr
iconvp = 0
idiffp = 1
ireslp = 0           ! conjugate gradient use to solve prhyd
ipol   = 0
ndircp = 0
nitmap = nitmax(ivar)
nswrsp = 1           ! no reconstruction gradient
nswrgp = nswrgr(ivar)
imligp = imligr(ivar)
ircflp = ircflu(ivar)
ischcp = ischcv(ivar)
isstpp = isstpc(ivar)
iescap = 0
imucpp = 0
idftnp = 1
iswdyp = iswdyn(ivar)
imgrp  = 0           ! we do not use multigrid
ncymxp = ncymax(ivar)
nitmfp = nitmgf(ivar)
ipp    = ipprtp(ivar)
iwarnp = iwarni(ivar)
blencp = blencv(ivar)
epsilp = epsilo(ivar)
epsrsp = epsrsm(ivar)
epsrgp = epsrgr(ivar)
climgp = climgr(ivar)
extrap = 0.d0
relaxp = relaxv(ivar)
thetap = thetav(ivar)

! --- Solve the diffusion equation

call codits &
!==========
( nvar   , nscal  ,                                              &
  idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
  imrgra , nswrsp , nswrgp , imligp , ircflp ,                   &
  ischcp , isstpp , iescap , imucpp , idftnp , iswdyp ,          &
  imgrp  , ncymxp , nitmfp , ipp    , iwarnp ,                   &
  blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
  relaxp , thetap ,                                              &
  prhyd  , prhyd  ,                                              &
  coefap , coefbp ,                                              &
  cofafp , cofbfp ,                                              &
  propfa(1,iflmas), propfb(1,iflmab),                            &
  viscf  , viscb  , rvoid  , viscf  , viscb  , rvoid  ,          &
  rvoid  , rvoid  ,                                              &
  rovsdt , smbr   , prhyd  , dpvar  ,                            &
  rvoid  , rvoid  )

! Free memory
deallocate(dpvar)

inc    = 1
iccocg = 1
nswrgp = 1
extrap = 0.d0

call grdpre &
!==========
 ( ivar   , imrgra , inc    , iccocg , nswrgp , imligp ,         &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                  &
   prhyd  , xinvro , coefap , coefbp ,                           &
   grdphd   )

!===============================================================================
! Free memory
!===============================================================================

deallocate(coefap, cofafp, coefbp, cofbfp)
deallocate(viscf, viscb)
deallocate(xinvro)
deallocate(smbr, rovsdt)

!--------
! Formats
!--------

!----
! End
!----

return

end subroutine
