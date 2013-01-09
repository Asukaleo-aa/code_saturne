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

!> \file turbkw.f90
!>
!> \brief Solving the \f$ k - \omega \f$ SST for incompressible flows
!> or slightly compressible flows for one time step.
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     nvar          total number of variables
!> \param[in]     nscal         total number of scalars
!> \param[in]     ncepdp        number of cells with head loss
!> \param[in]     ncesmp        number of cells with mass source term
!> \param[in]     icepdc        index of the ncepdp cells with head loss
!> \param[in]     icetsm        index of cells with mass source term
!> \param[in]     itypsm        mass source type for the variables (cf. ustsma)
!> \param[in]     dt            time step (per cell)
!> \param[in,out] rtp           calculated variables at cell centers
!>                               (at the current time step)
!> \param[in]     rtpa          calculated variables at cell centers
!>                               (at the previous time step)
!> \param[in]     propce        physical properties at cell centers
!> \param[in]     propfa        physical properties at interior face centers
!> \param[in]     propfb        physical properties at boundary face centers
!> \param[in]     tslagr        coupling term of the lagangian module
!> \param[in]     coefa, coefb  boundary conditions
!>
!> \param[in]     ckupdc        work array for the head loss
!> \param[in]     smacel        values of the variables associated to the
!>                               mass source
!>                               (for ivar=ipr, smacel is the mass flux)
!_______________________________________________________________________________

subroutine turbkw &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   tslagr , coefa  , coefb  , ckupdc , smacel )

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use dimens, only: ndimfb
use numvar
use entsor
use cstnum
use cstphy
use optcal
use lagran
use pointe, only: s2kw, divukw, ifapat, dispar, coefau, coefbu
use parall
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp

integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itypsm(ncesmp,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision tslagr(ncelet,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision ckupdc(ncepdp,6), smacel(ncesmp,nvar)

! Local variables

character*80     chaine
integer          iel   , ifac  , init  , inc   , iccocg, ivar
integer          ii, iivar , iiun  , ifacpt
integer          iclipk, iclipw, isqrt
integer          nswrgp, imligp
integer          iclik , iclomg, iclikf, iclomf
integer          iclvar, iclvaf
integer          iconvp, idiffp, ndircp, ireslp
integer          nitmap, nswrsp, ircflp, ischcp, isstpp, iescap
integer          imgrp , ncymxp, nitmfp
integer          ipcrom, ipbrom, ipcvst, ipcvis, iflmas, iflmab
integer          iwarnp, ipp
integer          iptsta
integer          ipcroo, ipbroo, ipcvto, ipcvlo
integer          imucpp, idftnp, iswdyp
integer          ipcliu
logical          ilved

double precision rnorm , d2s3, divp23, epz2
double precision deltk , deltw, a11, a12, a22, a21
double precision unsdet, romvsd
double precision prdtur, xk, xw, xeps, xnu
double precision visct , rho, visclc, visctc, hint
double precision blencp, epsilp, epsrgp, climgp, extrap, relaxp
double precision thetp1, thetak, thetaw, thets, thetap, epsrsp
double precision tuexpk, tuexpw
double precision cdkw, xarg1, xxf1, xgamma, xbeta, sigma, produc
double precision var, vrmin, vrmax

double precision rvoid(1)

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: smbrk, smbrw
double precision, allocatable, dimension(:) :: tinstk, tinstw, xf1
double precision, allocatable, dimension(:,:) :: gradk, grado, grad
double precision, allocatable, dimension(:) :: w1, w2
double precision, allocatable, dimension(:) :: gdkgdw
double precision, allocatable, dimension(:) :: w5, w6
double precision, allocatable, dimension(:) :: prodk, prodw
double precision, allocatable, dimension(:) :: gamk, gamw
double precision, allocatable, dimension(:) :: usimpk, usimpw
double precision, allocatable, dimension(:) :: w7
double precision, allocatable, dimension(:) :: dpvar
double precision, allocatable, dimension(:) :: rotfct

!===============================================================================

!===============================================================================
! 1.Initialization
!===============================================================================

! Allocate temporary arrays for the turbulence resolution
allocate(viscf(nfac), viscb(nfabor))
allocate(smbrk(ncelet), smbrw(ncelet))
allocate(tinstk(ncelet), tinstw(ncelet), xf1(ncelet))

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet))
allocate(dpvar(ncelet))
allocate(gdkgdw(ncelet))
allocate(prodk(ncelet), prodw(ncelet))

epz2 = epzero**2

iclik  = iclrtp(ik ,icoef)
iclomg = iclrtp(iomg,icoef)
iclikf = iclrtp(ik ,icoeff)
iclomf = iclrtp(iomg,icoeff)

ipcrom = ipproc(irom  )
ipcvst = ipproc(ivisct)
ipcvis = ipproc(iviscl)
iflmas = ipprof(ifluma(ik))
iflmab = ipprob(ifluma(ik))
ipbrom = ipprob(irom  )

thets  = thetst

ipcroo = ipcrom
ipbroo = ipbrom
ipcvto = ipcvst
ipcvlo = ipcvis
if(isto2t.gt.0) then
  if (iroext.gt.0) then
    ipcroo = ipproc(iroma)
    ipbroo = ipprob(iroma)
  endif
  if(iviext.gt.0) then
    ipcvto = ipproc(ivista)
    ipcvlo = ipproc(ivisla)
  endif
endif

if(isto2t.gt.0) then
  iptsta = ipproc(itstua)
else
  iptsta = 0
endif

if(iwarni(ik).ge.1) then
  write(nfecra,1000)
endif

d2s3 = 2.d0/3.d0

ipcliu = iclrtp(iu,icoef)

!===============================================================================
! 2.1 Compute dk/dxj.dw/dxj
!     stored in gdkgdw
!===============================================================================

! Allocate temporary arrays for gradients calculation
allocate(gradk(ncelet,3), grado(ncelet,3))

iccocg = 1
inc = 1

nswrgp = nswrgr(ik)
imligp = imligr(ik)
iwarnp = iwarni(ik)
epsrgp = epsrgr(ik)
climgp = climgr(ik)
extrap = extrag(ik)

call grdcel &
!==========
 ( ik  , imrgra , inc    , iccocg , nswrgp , imligp ,             &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,ik)   , coefa(1,iclik)  , coefb(1,iclik)  ,             &
   gradk  )

nswrgp = nswrgr(iomg)
imligp = imligr(iomg)
iwarnp = iwarni(iomg)
epsrgp = epsrgr(iomg)
climgp = climgr(iomg)
extrap = extrag(iomg)

call grdcel &
!==========
 ( iomg , imrgra , inc    , iccocg , nswrgp , imligp ,            &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,iomg)  , coefa(1,iclomg) , coefb(1,iclomg) ,            &
   grado  )

do iel = 1, ncel
  gdkgdw(iel) = gradk(iel,1)*grado(iel,1) &
              + gradk(iel,2)*grado(iel,2) &
              + gradk(iel,3)*grado(iel,3)
enddo

! Free memory
deallocate(gradk, grado)

!===============================================================================
! 2.2. Compute the weight f1 (stored in xf1)
!===============================================================================

if (abs(icdpar).eq.2) then
  do iel = 1, ncel
    ifacpt = ifapat(iel)
    w2(iel) = (cdgfbo(1,ifacpt)-xyzcen(1,iel))**2 &
            + (cdgfbo(2,ifacpt)-xyzcen(2,iel))**2 &
            + (cdgfbo(3,ifacpt)-xyzcen(3,iel))**2
    w2(iel) = sqrt(w2(iel))
  enddo
else
  do iel = 1, ncel
    w2(iel) = max(dispar(iel),epzero)
  enddo
endif

! En cas d'ordre 2 on utilise les valeurs en n car le terme en (1-f1)*gdkgdw
! sera dans PROPCE. Du coup, on aura quand meme certaines "constantes"
! intervenant dans des termes en n+1/2 (ex sigma_k pour la diffusion) calcules
! a partir de f1 en n -> mais l'effet sur les "constantes" est faible
! -> a garder en tete si on fait vraiment de l'ordre 2 en temps en k-omega
do iel = 1, ncel
  rho = propce(iel,ipcroo)
  xnu = propce(iel,ipcvlo)/rho
  xk = rtpa(iel,ik)
  xw  = rtpa(iel,iomg)
  cdkw = 2*rho/ckwsw2/xw*gdkgdw(iel)
  cdkw = max(cdkw,1.d-20)
  xarg1 = max(sqrt(xk)/cmu/xw/w2(iel), 500.d0*xnu/xw/w2(iel)**2)
  xarg1 = min(xarg1, 4.d0*rho*xk/ckwsw2/cdkw/w2(iel)**2)
  xf1(iel) = tanh(xarg1**4)
enddo

!===============================================================================
! 3. Instationnary terms (stored in tinstk and tinstw)
!===============================================================================

do iel = 1, ncel
  rho = propce(iel,ipcrom)
  romvsd = rho*volume(iel)/dt(iel)
  tinstk(iel) = istat(ik)*romvsd
  tinstw(iel) = istat(iomg)*romvsd
enddo

!===============================================================================
! 4. Compute production terms
!     stored in: prodk,prodw
!      En sortie de l'etape on conserve gdkgdw,xf1,prodk,tinstW
!===============================================================================

do iel = 1, ncel
  xk   = rtpa(iel,ik)
  xw   = rtpa(iel,iomg)
  xeps = cmu*xw*xk
  visct = propce(iel,ipcvto)
  rho = propce(iel,ipcroo)
  prodw(iel) = visct*s2kw(iel)                    &
             - d2s3*rho*xk*divukw(iel)

  tinstw(iel) = tinstw(iel)                                                    &
              + max(d2s3*rho*volume(iel)*(rho*xk/(visct*xw))*divukw(iel), 0.d0)

  ! Take the min between prodw and the low Reynold one
  if (prodw(iel).gt.ckwc1*rho*xeps) then
    prodk(iel) = ckwc1*rho*xeps
  else
    prodk(iel) = prodw(iel)
    tinstk(iel) = tinstk(iel) + max(d2s3*volume(iel)*rho*divukw(iel), 0.d0)
  endif
enddo

!===============================================================================
! 5. Take into account rotation/curvature correction, if necessary
!===============================================================================

! Spalart-Shur correction: the production terms are multiplied by a
! 'rotation function'

if (irccor.eq.1) then

  ! Allocate an array for the rotation function
  allocate(rotfct(ncel))

  ! Compute the rotation function (gdkgdw array not used)
  call rotcor &
  !==========
  ( dt     , rtpa   , propce , coefa , coefb , &
    rotfct , gdkgdw     )

  do iel = 1, ncel
    prodk(iel) = prodk(iel)*rotfct(iel)
    prodw(iel) = prodw(iel)*rotfct(iel)
  enddo

  ! rotfct array is used later in case of renforced coupling (ikecou = 1).
  ! The array is deallocated at the end of the subroutine.

endif

!===============================================================================
! 6. Compute buoyancy terms
!     stored in: prodk, prodw, w2
!===============================================================================

if (igrake.eq.1) then

  ! Allocate a temporary array for the gradient calculation
  allocate(grad(ncelet,3))

  ! --- Buoyant term:     G = Beta*g*GRAD(T)/PrT/rho
  !     Here is computed: G =-g*GRAD(rho)/PrT/rho

  iccocg = 1
  inc = 1

  nswrgp = nswrgr(ik)
  epsrgp = epsrgr(ik)
  imligp = imligr(ik)
  iwarnp = iwarni(ik)
  climgp = climgr(ik)
  extrap = extrag(ik)

  ! BCs on rho: Dirichlet ROMB
  !  NB: viscb is used as COEFB

  do ifac = 1, nfabor
    viscb(ifac) = 0.d0
  enddo

  iivar = 0

  call grdcel &
  !==========
 ( iivar  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   propce(1,ipcroo), propfb(1,ipbroo), viscb  ,                   &
   grad   )


  ! Buoyancy production
  !   prodk=min(P,c1*eps)+G
  !   prodw=P+(1-ce3)*G
  if (iscalt.gt.0.and.nscal.ge.iscalt) then
    prdtur = sigmas(iscalt)
  else
    prdtur = 1.d0
  endif

  do iel = 1, ncel
    rho = propce(iel,ipcroo)
    visct = propce(iel,ipcvto)

    w2(iel) = -(grad(iel,1)*gx + grad(iel,2)*gy + grad(iel,3)*gz) / &
               (rho*prdtur)

    prodw(iel) = prodw(iel)+visct*max(w2(iel),zero)
    prodk(iel) = prodk(iel)+visct*w2(iel)

    ! Implicit Buoyant terms when negativ
    tinstk(iel) = tinstk(iel)                                        &
                + max(-volume(iel)*visct/rtpa(iel,ik)*w2(iel), 0.d0)
  enddo

  ! Free memory
  deallocate(grad)

endif

!===============================================================================
! 7. Take user source terms into account
!     explicit parts stored in: smbrk, smbrw
!     implicit parts stored in: usimpk, usimpw
!===============================================================================

allocate(usimpk(ncelet), usimpw(ncelet))

do iel = 1, ncel
  smbrk(iel) = 0.d0
  smbrw(iel) = 0.d0
  usimpk(iel) = 0.d0
  usimpw(iel) = 0.d0
enddo

call ustskw &
!==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtpa   , propce , propfa , propfb ,                   &
   ckupdc , smacel , s2kw   , divukw ,                            &
   gdkgdw , w2     , xf1    ,                                     &
   smbrk  , smbrw  , usimpk , usimpw )

! If source terms are extrapolated over time
if (isto2t.gt.0) then

  thetak = thetav(ik)
  thetaw = thetav(iomg)

  do iel = 1, ncel

    ! Recover the value at time (n-1)
    tuexpk = propce(iel,iptsta)
    tuexpw = propce(iel,iptsta+1)

    ! Save the values for the next time-step
    propce(iel,iptsta) = smbrk(iel)
    propce(iel,iptsta+1) = smbrw(iel)

    ! Explicit Part
    smbrk(iel) = - thets*tuexpk
    smbrw(iel) = - thets*tuexpw
    ! It is assumed that (-usimpk > 0) and though this term is implicit
    smbrk(iel) = usimpk(iel)*rtpa(iel,ik) + smbrk(iel)
    smbrw(iel) = usimpw(iel)*rtpa(iel,iomg) + smbrw(iel)

    ! Implicit part
    tinstk(iel) = tinstk(iel) -usimpk(iel)*thetak
    tinstw(iel) = tinstw(iel) -usimpw(iel)*thetaw
  enddo

! If no extrapolation over time
else
  do iel = 1, ncel
    ! Explicit Part
    smbrk(iel) = smbrk(iel) + usimpk(iel)*rtpa(iel,ik)
    smbrw(iel) = smbrw(iel) + usimpw(iel)*rtpa(iel,iomg)

    ! Implicit part
    tinstk(iel) = tinstk(iel) + max(-usimpk(iel),zero)
    tinstw(iel) = tinstw(iel) + max(-usimpw(iel),zero)
  enddo
endif


!===============================================================================
! 8. Finalization of explicit and implicit source terms

!      Les termes sont stockes dans     smbrk, smbrw
!      En sortie de l'etape on conserve smbrk,smbrw,gdkgdw-4
!===============================================================================

do iel = 1, ncel

  visct  = propce(iel,ipcvto)
  rho    = propce(iel,ipcroo)
  xk     = rtpa(iel,ik)
  xw     = rtpa(iel,iomg)
  xxf1   = xf1(iel)
  xgamma = xxf1*ckwgm1 + (1.d0-xxf1)*ckwgm2
  xbeta  = xxf1*ckwbt1 + (1.d0-xxf1)*ckwbt2

  smbrk(iel) = smbrk(iel) + volume(iel)*(                         &
                                          prodk(iel)              &
                                        - cmu*rho*xw*xk )

  smbrw(iel) = smbrw(iel)                                                &
             + volume(iel)*(                                             &
                             rho*xgamma/visct*prodw(iel)                 &
                           - xbeta*rho*xw**2                             &
                           + 2.d0*rho/xw*(1.d0-xxf1)/ckwsw2*gdkgdw(iel)  &
                           )

enddo

! If the solving of k-omega is uncoupled, negative source terms are implicited
if (ikecou.eq.0) then
  do iel=1,ncel
    xw    = rtpa(iel,iomg)
    xxf1  = xf1(iel)
    xbeta = xxf1*ckwbt1 + (1.d0-xxf1)*ckwbt2
    rho = propce(iel,ipcrom)
    tinstk(iel) = tinstk(iel) + volume(iel)*cmu*rho*xw
    tinstw(iel) = tinstw(iel) + volume(iel)*xbeta*rho*xw
  enddo
endif

! Free memory
deallocate(gdkgdw)

!===============================================================================
! 9 Prise en compte des termes sources lagrangien
!   couplage retour
!===============================================================================

!     Ordre 2 non pris en compte
if (iilagr.eq.2 .and. ltsdyn.eq.1) then

  do iel = 1, ncel

    ! Termes sources explicte et implicte sur k
    smbrk(iel)  = smbrk(iel) + tslagr(iel,itske)

    ! Termes sources explicte sur omega : on reprend la constante CE4 directement
    !    du k-eps sans justification ... a creuser si necessaire
    smbrw(iel)  = smbrw(iel)                                      &
                + ce4 *tslagr(iel,itske) * propce(iel,ipcroo)     &
                /propce(iel,ipcvto)

    ! Termes sources implicite sur k
    tinstk(iel) = tinstk(iel) + max(-tslagr(iel,itsli),zero)

    ! Termes sources implicte sur omega
    tinstw(iel) = tinstw(iel)                                     &
                + max( (-ce4*tslagr(iel,itske)/rtpa(iel,ik)) , zero)
  enddo

endif

!===============================================================================
! 10. Mass source terms (Implicit and explicit parts)

!===============================================================================

if (ncesmp.gt.0) then

  allocate(gamk(ncelet), gamw(ncelet))

  ! Entier egal a 1 (pour navsto : nb de sur-iter)
  iiun = 1

  ! On incremente SMBRS par -Gamma RTPA et ROVSDT par Gamma (*theta)
  ivar = ik

  call catsma &
  !==========
 ( ncelet , ncel   , ncesmp , iiun   ,                            &
   isto2t , thetav(ivar) ,                                        &
   icetsm , itypsm(1,ivar) ,                                      &
   volume , rtpa(1,ivar) , smacel(1,ivar) , smacel(1,ipr) ,       &
   smbrk  , tinstk , gamk )

  ivar = iomg

  call catsma &
  !==========
 ( ncelet , ncel   , ncesmp , iiun   ,                            &
   isto2t , thetav(ivar) ,                                        &
   icetsm , itypsm(1,ivar) ,                                      &
   volume , rtpa(1,ivar) , smacel(1,ivar) , smacel(1,ipr) ,       &
   smbrw  , tinstw , gamw )

  ! Si on extrapole les TS on met Gamma Pinj dans PROPCE
  if(isto2t.gt.0) then
    do iel = 1, ncel
      propce(iel,iptsta  ) = propce(iel,iptsta  ) + gamk(iel)
      propce(iel,iptsta+1) = propce(iel,iptsta+1) + gamw(iel)
    enddo
  !  Sinon on le met directement dans SMBR
  else
    do iel = 1, ncel
      smbrk(iel) = smbrk(iel) + gamk(iel)
      smbrw(iel) = smbrw(iel) + gamw(iel)
    enddo
  endif

  !Free memory
  deallocate(gamk, gamw)

endif

! Finalisation des termes sources
if (isto2t.gt.0) then
  thetp1 = 1.d0 + thets
  do iel = 1, ncel
    smbrk(iel) = smbrk(iel) + thetp1 * propce(iel,iptsta)
    smbrw(iel) = smbrw(iel) + thetp1 * propce(iel,iptsta+1)
  enddo
endif


!===============================================================================
! 11.1 Re-set Boundary conditions flux coefficient for k and omega

!     The definition of cofaf requires hint=(mu+muT/sigma)/distb where sigma
!     is not constant in the k-omega model (and not directly accessible)
!===============================================================================

do ifac = 1, nfabor

  iel = ifabor(ifac)

  ! --- Physical Propreties
  visclc = propce(iel,ipcvis)
  visctc = propce(iel,ipcvst)

  xxf1 = xf1(iel)

  ! k
  sigma = xxf1*ckwsk1 + (1.d0-xxf1)*ckwsk2
  hint = (visclc+visctc/sigma)/distb(ifac)

  ! Translate coefa into cofaf and coefb into cofbf
  coefa(ifac, iclikf) = -hint*coefa(ifac,iclik)
  coefb(ifac, iclikf) = hint*(1.d0-coefb(ifac,iclik))

  ! Omega
  sigma = xxf1*ckwsw1 + (1.d0-xxf1)*ckwsw2
  hint = (visclc+visctc/sigma)/distb(ifac)

  ! Translate coefa into cofaf and coefb into cofbf
  coefa(ifac, iclomf) = -hint*coefa(ifac,iclomg)
  coefb(ifac, iclomf) = hint*(1.d0-coefb(ifac,iclomg))

enddo

!===============================================================================
! 11.2 Prise en compte des termes de conv/diff dans le second membre pour le
!      couplage renforcé k-omega (ikecou == 1)

!      Tableaux de travail              w7
!      Les termes sont stockes dans     w5 ET w6, PUIS AJOUTES A smbrk, smbrw
!      En sortie de l'etape on conserve w2-6,smbrk,smbrw,usimpk
!===============================================================================

if (ikecou.eq.1) then

  allocate(w5(ncelet), w6(ncelet))
  allocate(w7(ncelet))

  do iel = 1, ncel
    w5 (iel) = 0.d0
    w6 (iel) = 0.d0
  enddo

  ! ---> Traitement de k
  ivar   = ik

  ipp    = ipprtp(ivar)

  iclvar = iclrtp(ivar,icoef )
  iclvaf = iclrtp(ivar,icoeff)
  chaine = nomvar(ipp)

  if( idiff(ivar).ge. 1 ) then

    do iel = 1, ncel
      xxf1 = xf1(iel)
      sigma = xxf1*ckwsk1 + (1.d0-xxf1)*ckwsk2
      w7(iel) = propce(iel,ipcvis)                                &
              + idifft(ivar)*propce(iel,ipcvst)/sigma
    enddo
    call viscfa &
    !==========
 ( imvisf ,                                                       &
   w7     ,                                                       &
   viscf  , viscb  )

  else

    do ifac = 1, nfac
      viscf(ifac) = 0.d0
    enddo
    do ifac = 1, nfabor
      viscb(ifac) = 0.d0
    enddo

  endif

  iccocg = 1
  inc    = 1
  imucpp = 0
  idftnp = 1 ! no tensorial diffusivity
  iconvp = iconv (ivar)
  idiffp = idiff (ivar)
  nswrgp = nswrgr(ivar)
  imligp = imligr(ivar)
  ircflp = ircflu(ivar)
  ischcp = ischcv(ivar)
  isstpp = isstpc(ivar)
  iwarnp = iwarni(ivar)
  blencp = blencv(ivar)
  epsrgp = epsrgr(ivar)
  climgp = climgr(ivar)
  extrap = extrag(ivar)
  relaxp = relaxv(ivar)
  thetap = thetav(ivar)

  call bilsca &
  !==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , nswrgp , imligp , ircflp , &
   ischcp , isstpp , inc    , imrgra , iccocg ,                   &
   ipp    , iwarnp , imucpp , idftnp ,                            &
   blencp , epsrgp , climgp , extrap , relaxp , thetap ,          &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
   coefa(1,iclvar) , coefb(1,iclvar) ,                            &
   coefa(1,iclvaf) , coefb(1,iclvaf) ,                            &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   viscf  , viscb  , rvoid  , rvoid  ,                            &
   rvoid  , rvoid  ,                                              &
   w5     )

  if (iwarni(ivar).ge.2) then
    isqrt = 1
    call prodsc(ncel,isqrt,smbrk,smbrk,rnorm)
    write(nfecra,1100) chaine(1:8) ,rnorm
  endif

  ! ---> Traitement de omega
  ivar   = iomg

  ipp    = ipprtp(ivar)

  iclvar = iclrtp(ivar,icoef )
  iclvaf = iclrtp(ivar,icoeff)
  chaine = nomvar(ipp)

  if( idiff(ivar).ge. 1 ) then
    do iel = 1, ncel
      xxf1 = xf1(iel)
      sigma = xxf1*ckwsw1 + (1.d0-xxf1)*ckwsw2
      w7(iel) = propce(iel,ipcvis)                                &
              + idifft(ivar)*propce(iel,ipcvst)/sigma
    enddo
    call viscfa &
    !==========
 ( imvisf ,                                                       &
   w7     ,                                                       &
   viscf  , viscb  )

  else

    do ifac = 1, nfac
      viscf(ifac) = 0.d0
    enddo
    do ifac = 1, nfabor
      viscb(ifac) = 0.d0
    enddo

  endif

  iccocg = 1
  inc    = 1
  imucpp = 0
  idftnp = 1 ! no tensorial diffusivity
  iconvp = iconv (ivar)
  idiffp = idiff (ivar)
  nswrgp = nswrgr(ivar)
  imligp = imligr(ivar)
  ircflp = ircflu(ivar)
  ischcp = ischcv(ivar)
  isstpp = isstpc(ivar)
  iwarnp = iwarni(ivar)
  blencp = blencv(ivar)
  epsrgp = epsrgr(ivar)
  climgp = climgr(ivar)
  extrap = extrag(ivar)
  relaxp = relaxv(ivar)
  thetap = thetav(ivar)

  call bilsca &
  !==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , nswrgp , imligp , ircflp , &
   ischcp , isstpp , inc    , imrgra , iccocg ,                   &
   ipp    , iwarnp , imucpp , idftnp ,                            &
   blencp , epsrgp , climgp , extrap , relaxp , thetap ,          &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
   coefa(1,iclvar) , coefb(1,iclvar) ,                            &
   coefa(1,iclvaf) , coefb(1,iclvaf) ,                            &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   viscf  , viscb  , rvoid  , rvoid  ,                            &
   rvoid  , rvoid  ,                                              &
   w6     )

  if (iwarni(ivar).ge.2) then
    isqrt = 1
    call prodsc(ncel,isqrt,smbrw,smbrw,rnorm)
    write(nfecra,1100) chaine(1:8) ,rnorm
  endif

  do iel = 1,ncel
    smbrk(iel) = smbrk(iel) + w5(iel)
    smbrw(iel) = smbrw(iel) + w6(iel)
  enddo

endif

!===============================================================================
! 11.3 k-omega coupling (ikecou == 1)

!===============================================================================

!  Ordre 2 non pris en compte
if(ikecou.eq.1) then

  ! Take into account, if necessary, the Spalart-Shur rotation/curvature
  ! correction of the production term
  if (irccor.eq.2) then
    do iel = 1, ncel
      w1(iel) = rotfct(iel)
    enddo
  else
    do iel = 1, ncel
      w1(iel) = 1.d0
    enddo
  endif

  do iel = 1, ncel

    rho = propce(iel,ipcrom)

    ! RESOLUTION COUPLEE

    romvsd     = 1.d0/(rho*volume(iel))
    smbrk(iel) = smbrk(iel)*romvsd
    smbrw(iel) = smbrw(iel)*romvsd
    divp23     = d2s3*max(divukw(iel),zero)
    produc     = w1(iel)*s2kw(iel)+w2(iel)
    xk         = rtpa(iel,ik)
    xw         = rtpa(iel,iomg)
    xxf1       = xf1(iel)
    xgamma     = xxf1*ckwgm1 + (1.d0-xxf1)*ckwgm2
    xbeta      = xxf1*ckwbt1 + (1.d0-xxf1)*ckwbt2

    a11 = 1.d0/dt(iel)                                            &
         - 1.d0/xw*min(produc,zero)+divp23+cmu*xw
    a12 = cmu*xk
    a21 = 0.d0
    a22 = 1.d0/dt(iel)+xgamma*divp23+2.d0*xbeta*xw

    unsdet = 1.d0/(a11*a22 -a12*a21)

    deltk = ( a22*smbrk(iel) -a12*smbrw(iel) )*unsdet
    deltw = (-a21*smbrk(iel) +a11*smbrw(iel) )*unsdet

    ! NOUVEAU TERME SOURCE POUR CODITS

    romvsd = rho*volume(iel)/dt(iel)

    smbrk(iel) = romvsd*deltk
    smbrw(iel) = romvsd*deltw

  enddo


  ! on enleve la convection/diffusion au temps n a SMBRK et SMBRW
  ! s'ils ont ete calcules
  do iel = 1, ncel
    smbrk(iel) = smbrk(iel) - w5(iel)
    smbrw(iel) = smbrw(iel) - w6(iel)
  enddo

  ! Free memory
  deallocate(w5, w6)
  deallocate(w7)

endif

!===============================================================================
! 14. Solving
!===============================================================================

! ---> turbulent kinetic (k) energy treatment
ivar = ik
iclvar = iclrtp(ivar,icoef )
iclvaf = iclrtp(ivar,icoeff)

ipp    = ipprtp(ivar)

! Face viscosity
if (idiff(ivar).ge. 1) then

  do iel = 1, ncel
    xxf1 = xf1(iel)
    sigma = xxf1*ckwsk1 + (1.d0-xxf1)*ckwsk2
    w1(iel) = propce(iel,ipcvis)                                  &
             + idifft(ivar)*propce(iel,ipcvst)/sigma
  enddo
  call viscfa &
  !==========
 ( imvisf ,                                                       &
   w1     ,                                                       &
   viscf  , viscb  )

else

  do ifac = 1, nfac
    viscf(ifac) = 0.d0
  enddo
  do ifac = 1, nfabor
    viscb(ifac) = 0.d0
  enddo

endif

! Solving k
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
imucpp = 0
idftnp = 1 ! no tensorial diffusivity
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
thetap = thetav(ivar)

call codits &
!==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
   imrgra , nswrsp , nswrgp , imligp , ircflp ,                   &
   ischcp , isstpp , iescap , imucpp , idftnp , iswdyp ,          &
   imgrp  , ncymxp , nitmfp , ipp    , iwarnp ,                   &
   blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
   relaxp , thetap ,                                              &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
   coefa(1,iclvar) , coefb(1,iclvar) ,                            &
   coefa(1,iclvaf) , coefb(1,iclvaf) ,                            &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   viscf  , viscb  , rvoid  , viscf  , viscb  , rvoid  ,          &
   rvoid  , rvoid  ,                                              &
   tinstk , smbrk  , rtp(1,ivar)     , dpvar  ,                   &
   rvoid  , rvoid  )

! ---> Omega treatment
ivar = iomg
iclvar = iclrtp(ivar,icoef )
iclvaf = iclrtp(ivar,icoeff)

ipp    = ipprtp(ivar)

! Face viscosity
if (idiff(ivar).ge. 1) then
  do iel = 1, ncel
    xxf1 = xf1(iel)
    sigma = xxf1*ckwsw1 + (1.d0-xxf1)*ckwsw2
    w1(iel) = propce(iel,ipcvis)                                  &
                        + idifft(ivar)*propce(iel,ipcvst)/sigma
  enddo
  call viscfa &
  !==========
 ( imvisf ,                                                       &
   w1     ,                                                       &
   viscf  , viscb  )

else

  do ifac = 1, nfac
    viscf(ifac) = 0.d0
  enddo
  do ifac = 1, nfabor
    viscb(ifac) = 0.d0
  enddo

endif

! Solving omega
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
imucpp = 0
idftnp = 1 ! no tensorial diffusivity
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
thetap = thetav(ivar)

call codits &
!==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
   imrgra , nswrsp , nswrgp , imligp , ircflp ,                   &
   ischcp , isstpp , iescap , imucpp , idftnp , iswdyp ,          &
   imgrp  , ncymxp , nitmfp , ipp    , iwarnp ,                   &
   blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
   relaxp , thetap ,                                              &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
   coefa(1,iclvar) , coefb(1,iclvar) ,                            &
   coefa(1,iclvaf) , coefb(1,iclvaf) ,                            &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   viscf  , viscb  , rvoid  , viscf  , viscb  , rvoid  ,          &
   rvoid  , rvoid  ,                                              &
   tinstw , smbrw  , rtp(1,ivar)     , dpvar  ,                   &
   rvoid  , rvoid  )

!===============================================================================
! 15. Clipping
!===============================================================================

! Calcul des Min/Max avant clipping, pour affichage
do ii = 1, 2
  if(ii.eq.1) then
    ivar = ik
  elseif(ii.eq.2) then
    ivar = iomg
  endif
  ipp  = ipprtp(ivar)

  vrmin =  grand
  vrmax = -grand
  do iel = 1, ncel
    var = rtp(iel,ivar)
    vrmin = min(vrmin,var)
    vrmax = max(vrmax,var)
  enddo
  if (irangp.ge.0) then
    call parmax (vrmax)
    !==========
    call parmin (vrmin)
    !==========
  endif
  varmna(ipp) = vrmin
  varmxa(ipp) = vrmax

enddo

! On clippe simplement k et omega par valeur absolue
iclipk = 0
iclipw = 0
do iel = 1, ncel
  xk = rtp(iel,ik )
  xw = rtp(iel,iomg)
  if (abs(xk).le.epz2) then
    iclipk = iclipk + 1
    rtp(iel,ik) = max(rtp(iel,ik),epz2)
  elseif(xk.le.0.d0) then
    iclipk = iclipk + 1
    rtp(iel,ik) = -xk
  endif
  if (abs(xw).le.epz2) then
    iclipw = iclipw + 1
    rtp(iel,iomg) = max(rtp(iel,iomg),epz2)
  elseif(xw.le.0.d0) then
    iclipw = iclipw + 1
    rtp(iel,iomg) = -xw
  endif
enddo

if (irangp.ge.0) then
  call parcpt (iclipk)
  !==========
  call parcpt (iclipw)
  !==========
endif

! ---  Stockage nb de clippings pour listing

iclpmn(ipprtp(ik)) = iclipk
iclpmn(ipprtp(iomg)) = iclipw

! Free memory
deallocate(viscf, viscb)
deallocate(smbrk, smbrw)
deallocate(tinstk, tinstw, xf1)
deallocate(w1, w2, usimpk, usimpw)
deallocate(dpvar)
deallocate(prodk, prodw)

if (allocated(rotfct))  deallocate(rotfct)

!--------
! Formats
!--------

#if defined(_CS_LANG_FR)

 1000 format(/, &
'   ** RESOLUTION DU K-OMEGA'                     ,/,&
'      ---------------------'                     ,/)
 1100 format(1X,A8,' : BILAN EXPLICITE = ',E14.5)

#else

 1000 format(/, &
'   ** SOLVING K-OMEGA'                           ,/,&
'      ---------------'                           ,/)
 1100 format(1X,A8,' : EXPLICIT BALANCE = ',E14.5)

#endif

!----
! End
!----

return

end subroutine
