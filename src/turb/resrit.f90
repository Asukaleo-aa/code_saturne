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

!> \file resrit.f90
!>
!> \brief This subroutine perform the solving of the transport equation
!> of the turbulent heat fluxes.
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     nvar          total number of variables
!> \param[in]     nscal         total number of scalars
!> \param[in]     iscal         number of the scalar used
!> \param[in]     xcpp          Cp
!> \param[in,out] xut, xuta     calculated variables at cell centers
!>                               (at current and previous time steps)
!> \param[in]     dt            time step (per cell)
!> \param[in,out] rtp, rtpa     calculated variables at cell centers
!>                               (at current and previous time steps)
!> \param[in]     propce        physical properties at cell centers
!> \param[in]     propfa        physical properties at interior face centers
!> \param[in]     propfb        physical properties at boundary face centers
!> \param[in]     gradv         mean velocity gradient
!> \param[in]     gradt         mean temperature gradient
!_______________________________________________________________________________

subroutine resrit &
 ( nvar   , nscal  ,                                              &
   iscal  , xcpp   , xut    , xuta   ,                            &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   gradv  , gradt  )

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens, only: ndimfb
use numvar
use entsor
use optcal
use cstnum
use cstphy
use parall
use period
use pointe, only:visten
use field
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal , iscal

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision xcpp(ncelet), xut(3,ncelet), xuta(3,ncelet)
double precision gradv(ncelet,3,3)
double precision gradt(ncelet,3)

! Local variables

integer          init  , ifac  , iel   , inc
integer          ii    ,jj     , ivar
integer          iclvar, iclvaf, ipput, ippvt, ippwt
integer          ipcrom, ipcroo, ipcvis, ipcvst, iflmas, iflmab
integer          nswrgp, imligp, iwarnp
integer          iconvp, idiffp, ndircp, ireslp
integer          nitmap, nswrsp, ircflp, ischcp, isstpp, iescap
integer          imgrp , ncymxp, nitmfp
integer          iptsta
integer          ivisep
integer          ipcvsl
integer          isou, jsou
integer          itt
integer          idftnp, iswdyp
integer          f_id

double precision blencp, epsilp, epsrgp, climgp, extrap, relaxp
double precision epsrsp
double precision trrij , rctse
double precision grdpx,grdpy,grdpz,grdsn
double precision surfn2
double precision thets , thetv , thetp1
double precision xttke , prdtl
double precision hint
double precision grav(3)
double precision xrij(3,3),phiith(3)

double precision d1s2,cssca

double precision rvoid(1)

logical          ilved

character*80     fname

double precision, allocatable, dimension(:,:,:) :: gradut
double precision, allocatable, dimension(:,:) :: viscce
double precision, allocatable, dimension(:) :: viscb
double precision, allocatable, dimension(:,:,:) :: viscf
double precision, allocatable, dimension(:,:) :: smbrut
double precision, allocatable, dimension(:,:,:) :: fimp

double precision, dimension(:,:), pointer :: coefav, cofafv
double precision, dimension(:,:,:), pointer :: coefbv, cofbfv

!===============================================================================

!===============================================================================
! 1. Initialization
!===============================================================================

! Allocate work arrays
allocate(viscce(6,ncelet))
allocate(smbrut(3,ncelet))
allocate(fimp(3,3,ncelet))
allocate(viscf(3,3,nfac), viscb(nfabor))

d1s2 = 0.5d0

if ((itytur.eq.2).or.(itytur.eq.5).or.(iturb.eq.60)) then
  write(nfecra,*)'Utiliser un modele Rij avec ces modeles de thermiques'!FIXME
  call csexit(1)
endif

ipcrom = ipproc(irom)
ipcvis = ipproc(iviscl)
ipcvst = ipproc(ivisct)
iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))

ivar = isca(iscal)
ipput = ipprtp(ivar)
if (iwarni(ivar).ge.1) then
  write(nfecra,1000) nomvar(ipput)//'_turbulent_flux'!FIXME
endif

! S pour Source, V pour Variable
thets  = thetst
thetv  = thetav(ivar)

ipcroo = ipcrom
if (isto2t.gt.0.and.iroext.gt.0) then
  ipcroo = ipproc(iroma)
endif
if (isto2t.gt.0) then
  iptsta = ipproc(itstua)
else
  iptsta = 0
endif

if (ivisls(iscal).gt.0) then
  ipcvsl = ipproc(ivisls(iscal))
else
  ipcvsl = 0
endif

do iel = 1, ncelet
  do isou = 1, 3
    smbrut(isou,iel) = 0.d0
    do jsou = 1, 3
      fimp(isou,jsou,iel) = 0.d0
    enddo
  enddo
enddo

! Find the corresponding variance of the scalar iscal
itt = -1
if ((abs(gx)+abs(gy)+abs(gz)).gt.0) then
  grav(1) = gx
  grav(2) = gy
  grav(3) = gz
  do ii = 1, nscal
    if (iscavr(ii).eq.iscal) itt = ii
  enddo
endif

!===============================================================================
! 2. Mass source terms FIXME
!===============================================================================

if(isto2t.gt.0) then
  do iel = 1, ncel
    do isou = 1,3
      smbrut(isou,iel) = fimp(isou,isou,iel)*xuta(isou,iel)
      fimp(isou,isou,iel) = - thetv*fimp(isou,isou,iel)
    enddo
  enddo
! Si on n'extrapole pas les TS :
else
  do iel = 1, ncel
    do isou = 1, 3
      ! Terme source utilisateur
      smbrut(isou,iel) = smbrut(isou,iel) + fimp(isou,isou,iel)*xuta(isou,iel)
      ! Diagonale
      fimp(isou,isou,iel) = max(-fimp(isou,isou,iel),zero)
    enddo
  enddo
endif

!===============================================================================
! 3. Instationary term
!===============================================================================

do iel = 1, ncel
  do isou = 1, 3
    fimp(isou,isou,iel) = fimp(isou,isou,iel)                                  &
                        + istat(ivar)*(propce(iel,ipcrom)/dt(iel))*volume(iel)
  enddo
enddo

!===============================================================================
! 4. Right Hand Side of the thermal fluxes:
!     rho*(Pit + Git + Phi*_it - eps_it)
!===============================================================================

do iel = 1, ncel
  trrij  = d1s2*(rtp(iel,ir11)+rtp(iel,ir22)+rtp(iel,ir33))
  ! --- calcul de l echelle de temps de Durbin
  xttke  = trrij/rtp(iel,iep)

  xrij(1,1) = rtp(iel,ir11)
  xrij(2,2) = rtp(iel,ir22)
  xrij(3,3) = rtp(iel,ir33)
  xrij(1,2) = rtp(iel,ir12)
  xrij(1,3) = rtp(iel,ir13)
  xrij(2,3) = rtp(iel,ir23)
  xrij(2,1) = xrij(1,2)
  xrij(3,1) = xrij(1,3)
  xrij(3,2) = xrij(2,3)

  do isou = 1, 3
    phiith(isou) = -c1trit/xttke*xuta(isou,iel)                     &
                 + c2trit*(xuta(1,iel)*gradv(iel,1,isou)            &
                          +xuta(2,iel)*gradv(iel,2,isou)            &
                          +xuta(3,iel)*gradv(iel,3,isou))           &
                 + c4trit*(-xrij(isou,1)*gradt(iel,1)               &
                           -xrij(isou,2)*gradt(iel,2)               &
                           -xrij(isou,3)*gradt(iel,3))
    if (itt.gt.0) then
      phiith(isou) = phiith(isou)                                              &
             + c3trit*(propce(iel,ipproc(ibeta))*grav(isou)*rtp(iel,isca(itt)))
    endif

    ! Pressure/thermal fluctuation correlation term
    !----------------------------------------------
    smbrut(isou,iel) = smbrut(isou,iel) +                        &
                volume(iel)*propce(iel,ipcrom)*(phiith(isou) )

    fimp(isou,isou,iel) = fimp(isou,isou,iel) -                     &
                volume(iel)*propce(iel,ipcrom)*(                    &
              -c1trit/xttke+c2trit*gradv(iel,isou,isou) )

    ! Production terms
    !-----------------
    smbrut(isou,iel) = smbrut(isou,iel)                              &
                     + volume(iel)*propce(iel,ipcrom)                &
                       ! Production term due to the mean velcoity
                       *( -xuta(1,iel)*gradv(iel,1,isou)             &
                          -xuta(2,iel)*gradv(iel,2,isou)             &
                          -xuta(3,iel)*gradv(iel,3,isou)             &
                       ! Production term due to the mean temperature
                         -xrij(isou,1)*gradt(iel,1)                  &
                         -xrij(isou,2)*gradt(iel,2)                  &
                         -xrij(isou,3)*gradt(iel,3)                  &
                        )

    ! Production term due to the gravity
    if (itt.gt.0) then
      smbrut(isou,iel) = smbrut(isou,iel)                            &
                       + volume(iel)*propce(iel,ipcrom)*(            &
               -grav(isou)*propce(iel,ipproc(ibeta))*rtpa(iel,isca(itt)))
    endif
  enddo
enddo

!===============================================================================
! 5. Tensorial diffusion
!===============================================================================

do iel = 1, ncel
  if (ipcvsl.gt.0) then
    prdtl = propce(iel,ipcvis)*xcpp(iel)/propce(iel,ipproc(ivisls(iscal)))
  else
    prdtl = propce(iel,ipcvis)*xcpp(iel)/visls0(iscal)
  endif

  do isou = 1, 6
    if (isou.le.3) then
      viscce(isou,iel) = d1s2*(propce(iel,ipcvis)*(1.d0+1.d0/prdtl))    &
                       + ctheta(iscal)*visten(isou,iel)
    else
      viscce(isou,iel) = ctheta(iscal)*visten(isou,iel)
    endif
  enddo
enddo

call vistnv &
!==========
 ( imvisf ,                                                       &
   viscce ,                                                       &
   viscf  , viscb  )

!===============================================================================
! 6. Vectorial solving of the turbulent thermal fluxes
!===============================================================================

if (isto2t.gt.0) then
  thetp1 = 1.d0 + thets
  do iel = 1, ncel
    do isou = 1, 3
      smbrut(isou,iel) = smbrut(isou,iel) + thetp1*propce(iel,iptsta+isou-1) !FIXME
    enddo
  enddo
endif

! Name of the scalar ivar
call field_get_name(ivarfl(ivar), fname)

! Index of the corresponding turbulent flux
call field_get_id(trim(fname)//'_turbulent_flux', f_id)

call field_get_coefa_v(f_id,coefav)
call field_get_coefb_v(f_id,coefbv)
call field_get_coefaf_v(f_id,cofafv)
call field_get_coefbf_v(f_id,cofbfv)

iconvp = iconv (ivar)
idiffp = idiff (ivar)
ireslp = iresol(ivar)
ndircp = ndircl(ivar)
nitmap = nitmax(ivar)
nswrsp = nswrsm(ivar)
nswrgp = nswrgr(ivar)
imligp = imligr(ivar)
ircflp = ircflu(ivar)
ischcp = ischcv(ivar)
isstpp = isstpc(ivar)
iescap = 0
idftnp = 6
iswdyp = iswdyn(ivar)
imgrp  = imgr  (ivar)
ncymxp = ncymax(ivar)
nitmfp = nitmgf(ivar)
iwarnp = iwarni(ivar)
blencp = blencv(ivar)
epsilp = epsilo(ivar)
epsrsp = epsrsm(ivar)
epsrgp = epsrgr(ivar)
climgp = climgr(ivar)
extrap = extrag(ivar)
relaxp = relaxv(ivar)

ipput = ipprtp(ivar)!FIXME
ippvt = ipprtp(ivar)
ippwt = ipprtp(ivar)

! We do not take into account transpose of grad
ivisep = 0

call coditv &
!==========
(nvar   , nscal  ,                                              &
 idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
 imrgra , nswrsp , nswrgp , imligp , ircflp , ivisep ,          &
 ischcp , isstpp , iescap , idftnp , iswdyp ,                   &
 imgrp  , ncymxp , nitmfp , ipput  , ippvt  , ippwt  , iwarnp , &
 blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
 relaxp , thetv  ,                                              &
 xuta   , xuta   ,                                              &
 coefav , coefbv , cofafv , cofbfv ,                            &
 propfa(1,iflmas), propfb(1,iflmab),                            &
 viscf  , viscb  , viscf  , viscb  , rvoid  , rvoid  ,          &
 fimp   ,                                                       &
 smbrut ,                                                       &
 xut    ,                                                       &
 rvoid  )

!===============================================================================
! 7. Writtings
!===============================================================================

! Free memory
deallocate(viscce)
deallocate(viscf, viscb)
deallocate(smbrut)
deallocate(fimp)

!--------
! Formats
!--------

#if defined(_CS_LANG_FR)

 1000 format(/,'           Resolution pour la variable ',A23,/)

#else

 1000 format(/,'           Solving variable ',A23           ,/)
#endif

!----
! End
!----

return
end subroutine
