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

!===============================================================================
! Function :
! --------

!> \file clptrg.f90
!>
!> \brief Boundary conditions for rought walls (icodcl = 6).
!>
!> The wall functions may change the value of the diffusive flux.
!>
!> The values at a border face \f$ \fib \f$ stored in the face center
!> \f$ \centf \f$ of the variable \f$ P \f$ and its diffusive flux \f$ Q \f$
!> are written as:
!> \f[
!> P_\centf = A_P^g + B_P^g P_\centi
!> \f]
!> and
!> \f[
!> Q_\centf = A_P^f + B_P^f P_\centi
!> \f]
!> where \f$ P_\centi \f$ is the value of the variable \f$ P \f$ at the
!> neighbooring cell.
!>
!> Warning:
!>
!> - for a vector field such as the veclocity \f$ \vect{u} \f$ the boundary
!>   conditions may read:
!>   \f[
!>   \vect{u}_\centf = \vect{A}_u^g + \tens{B}_u^g \vect{u}_\centi
!>   \f]
!>   and
!>   \f[
!>   \vect{Q}_\centf = \vect{A}_u^f + \tens{B}_u^f \vect{u}_\centi
!>   \f]
!>   where \f$ \tens{B}_u^g \f$ and \f$ \tens{B}_u^f \f$ are 3x3 tensor matrix
!>   which coupled veclocity components next to a boundary. This is only
!>   available when the option ivelco is set to 1.
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     nvar          total number of variables
!> \param[in]     nscal         total number of scalars
!> \param[in]     isvhb         indicator to save exchange coeffient
!> \param[in,out] icodcl        face boundary condition code:
!>                               - 1 Dirichlet
!>                               - 3 Neumann
!>                               - 4 sliding and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 5 smooth wall and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 6 rought wall and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 9 free inlet/outlet
!>                                 (input mass flux blocked to 0)
!> \param[in]     dt            time step (per cell)
!> \param[in]     rtp, rtpa     calculated variables at cell centers
!>                               (at current and previous time steps)
!> \param[in]     propce        physical properties at cell centers
!> \param[in]     propfa        physical properties at interior face centers
!> \param[in]     propfb        physical properties at boundary face centers
!> \param[in,out] rcodcl        boundary condition values:
!>                               - rcodcl(1) value of the dirichlet
!>                               - rcodcl(2) value of the exterior exchange
!>                                 coefficient (infinite if no exchange)
!>                               - rcodcl(3) value flux density
!>                                 (negative if gain) in w/m2 or roughtness
!>                                 in m if icodcl=6
!>                                 -# for the velocity \f$ (\mu+\mu_T)
!>                                    \gradv \vect{u} \cdot \vect{n}  \f$
!>                                 -# for the pressure \f$ \Delta t
!>                                    \grad P \cdot \vect{n}  \f$
!>                                 -# for a scalar \f$ cp \left( K +
!>                                     \dfrac{K_T}{\sigma_T} \right)
!>                                     \grad T \cdot \vect{n} \f$
!> \param[in]     velipb        value of the velocity at \f$ \centip \f$
!>                               of boundary cells
!> \param[in]     rijipb        value of \f$ R_{ij} \f$ at \f$ \centip \f$
!>                               of boundary cells
!> \param[out]    coefa         explicit boundary condition coefficient
!> \param[out]    coefb         implicit boundary condition coefficient
!> \param[out]    visvdr        viscosite dynamique ds les cellules
!>                               de bord apres amortisst de v driest
!> \param[out]    hbord         coefficients d'echange aux bords
!>
!> \param[out]    thbord        boundary temperature in \f$ \centip \f$
!>                               (more exaclty the energetic variable)
!_______________________________________________________________________________

subroutine clptrg &
!================

 ( nvar   , nscal  ,                                              &
   isvhb  ,                                                       &
   icodcl ,                                                       &
   dt     , rtp    , rtpa   , propce , propfa , propfb , rcodcl , &
   velipb , rijipb , coefa  , coefb  , visvdr ,                   &
   hbord  , thbord )

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use optcal
use cstphy
use cstnum
use pointe
use entsor
use albase
use parall
use ppppar
use ppthch
use ppincl
use radiat
use cplsat
use mesh
use lagran

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          isvhb

integer          icodcl(nfabor,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision rcodcl(nfabor,nvar,3)
double precision velipb(nfabor,ndim), rijipb(nfabor,6)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision visvdr(ncelet)
double precision hbord(nfabor),thbord(nfabor)

! Local variables

integer          ifac, iel, ivar, isou, ii, jj, kk, isvhbl
integer          ihcp, iscal
integer          modntl
integer          iuntur, iuiptn
integer          iclu  , iclv  , iclw  , iclk  , iclep
integer          iclnu
integer          icl11 , icl22 , icl33 , icl12 , icl13 , icl23
integer          icl11r, icl22r, icl33r, icl12r, icl13r, icl23r
integer          iclphi, iclfb , iclal , iclomg
integer          iclvar, iclvrr
integer          icluf , iclvf , iclwf , iclkf , iclepf
integer          iclnuf
integer          icl11f, icl22f, icl33f, icl12f, icl13f, icl23f
integer          iclphf, iclfbf, iclalf, iclomf
integer          iclvaf
integer          ipcrom, ipcvis, ipcvst, ipccp , ipccv
integer          ipcvsl
double precision rnx, rny, rnz, rxnn
double precision tx, ty, tz, txn, txn0, t2x, t2y, t2z
double precision utau, upx, upy, upz, usn
double precision uiptn, uiptnf, uiptmn, uiptmx
double precision uetmax, uetmin, ukmax, ukmin, yplumx, yplumn
double precision uk, uet, tet, yplus, uplus
double precision gredu, rib, lmo, q0, e0
double precision cfnnu, cfnns, cfnnk, cfnne
double precision sqrcmu, clsyme, ek
double precision xmutlm
double precision rcprod, rcflux
double precision cpp, rkl,  prdtl
double precision hflui, hredui, hint, hext, pimp, heq, qimp
double precision und0, deuxd0
double precision eloglo(3,3), alpha(6,6)
double precision rcodcx, rcodcy, rcodcz, rcodcn
double precision visclc, visctc, romc  , distbf, srfbnf, cpscv
double precision cofimp, ypup
double precision distb0, rugd  , rugt  , ydep  , act
double precision dsa0

!===============================================================================

!===============================================================================
! 1. Initializations
!===============================================================================

! Initialize variables to avoid compiler warnings
iclep = 0
iclk = 0
iclomg = 0
iclfb = 0
iclphi = 0
iclnu = 0
icl11 = 0
icl22 = 0
icl33 = 0
icl12 = 0
icl13 = 0
icl23 = 0
iclvar = 0
ipccv = 0

ek = 0.d0

! --- Constants
uet = 1.d0
utau = 1.d0
sqrcmu = sqrt(cmu)

! --- Constantes de relaxation en atmosphere non neutre
cfnnu=1.d0
cfnns=1.d0
cfnnk=1.d0
cfnne=1.d0

und0   = 1.d0
deuxd0 = 2.d0

! --- Gradient Boundary Conditions
iclu   = iclrtp(iu,icoef)
iclv   = iclrtp(iv,icoef)
iclw   = iclrtp(iw,icoef)
if (itytur.eq.2) then
  iclk   = iclrtp(ik ,icoef)
  iclep  = iclrtp(iep,icoef)
elseif (itytur.eq.3) then
  icl11  = iclrtp(ir11,icoef)
  icl22  = iclrtp(ir22,icoef)
  icl33  = iclrtp(ir33,icoef)
  icl12  = iclrtp(ir12,icoef)
  icl13  = iclrtp(ir13,icoef)
  icl23  = iclrtp(ir23,icoef)
  ! Boundary conditions for the momentum equation
  icl11r = iclrtp(ir11,icoefr)
  icl22r = iclrtp(ir22,icoefr)
  icl33r = iclrtp(ir33,icoefr)
  icl12r = iclrtp(ir12,icoefr)
  icl13r = iclrtp(ir13,icoefr)
  icl23r = iclrtp(ir23,icoefr)
  iclep  = iclrtp(iep,icoef)
  if (iturb.eq.32) iclal = iclrtp(ial,icoef)
elseif (itytur.eq.5) then
  iclk   = iclrtp(ik ,icoef)
  iclep  = iclrtp(iep,icoef)
  iclphi = iclrtp(iphi,icoef)
  if (iturb.eq.50) then
    iclfb  = iclrtp(ifb,icoef)
  elseif (iturb.eq.51) then
    iclal  = iclrtp(ial,icoef)
  endif
elseif (iturb.eq.60) then
  iclk   = iclrtp(ik ,icoef)
  iclomg = iclrtp(iomg,icoef)
elseif (iturb.eq.70) then
  iclnu  = iclrtp(inusa,icoef)
endif

! --- Flux Boundary Conditions
icluf  = iclrtp(iu,icoeff)
iclvf  = iclrtp(iv,icoeff)
iclwf  = iclrtp(iw,icoeff)
if (itytur.eq.2) then
  iclkf  = iclrtp(ik ,icoeff)
  iclepf = iclrtp(iep,icoeff)
elseif (itytur.eq.3) then
  icl11f = iclrtp(ir11,icoeff)
  icl22f = iclrtp(ir22,icoeff)
  icl33f = iclrtp(ir33,icoeff)
  icl12f = iclrtp(ir12,icoeff)
  icl13f = iclrtp(ir13,icoeff)
  icl23f = iclrtp(ir23,icoeff)
  iclepf = iclrtp(iep,icoeff)
  if (iturb.eq.32) iclalf = iclrtp(ial,icoeff)
elseif (itytur.eq.5) then
  iclkf  = iclrtp(ik ,icoeff)
  iclepf = iclrtp(iep,icoeff)
  iclphf = iclrtp(iphi,icoeff)
  if (iturb.eq.50) then
    iclfbf = iclrtp(ifb,icoeff)
  elseif (iturb.eq.51) then
    iclalf = iclrtp(ial,icoeff)
  endif
elseif (iturb.eq.60) then
  iclkf  = iclrtp(ik ,icoeff)
  iclomf = iclrtp(iomg,icoeff)
elseif (iturb.eq.70) then
  iclnuf = iclrtp(inusa,icoeff)
endif

! --- Physical quantities
ipcrom = ipproc(irom)
ipcvis = ipproc(iviscl)
ipcvst = ipproc(ivisct)
if (icp.gt.0) then
  ipccp  = ipproc(icp)
else
  ipccp = 0
endif

! --- Compressible
if (ippmod(icompf) .ge. 0) then
  if (icv.gt.0) then
    ipccv  = ipproc(icv)
  else
    ipccv = 0
  endif
endif

! min. and max. of wall tangential velocity
uiptmx = -grand
uiptmn =  grand

! min. and max. of wall friction velocity
uetmax = -grand
uetmin =  grand
ukmax  = -grand
ukmin  =  grand

! min. and max. of y+
yplumx = -grand
yplumn =  grand

! Counter (reversal)
iuiptn = 0


! With v2f type model, (phi-fbar et BL-v2/k) u=0 is set directly, so
! uiptmx and uiptmn are necessarily 0
if (itytur.eq.5) then
  uiptmx = 0.d0
  uiptmn = 0.d0
endif

! --- Loop on boundary faces
do ifac = 1, nfabor

  ! Test on the presence of a rought wall
  if (icodcl(ifac,iu).eq.6) then

    iel = ifabor(ifac)

    ! Physical properties
    visclc = propce(iel,ipcvis)
    visctc = propce(iel,ipcvst)
    romc   = propce(iel,ipcrom)

    ! Geometric quantities
    distbf = distb(ifac)
    srfbnf = surfbn(ifac)

    !===========================================================================
    ! 1. Local framework
    !===========================================================================

    ! Unit normal

    rnx = surfbo(1,ifac)/srfbnf
    rny = surfbo(2,ifac)/srfbnf
    rnz = surfbo(3,ifac)/srfbnf

    ! Handle displacement velocity

    rcodcx = rcodcl(ifac,iu,1)
    rcodcy = rcodcl(ifac,iv,1)
    rcodcz = rcodcl(ifac,iw,1)

    ! If we are not using ALE, force the displacement velocity for the face
    !  to be tangential (and update rcodcl for possible use)
    if (iale.eq.0.and.imobil.eq.0) then
      rcodcn = rcodcx*rnx+rcodcy*rny+rcodcz*rnz
      rcodcx = rcodcx -rcodcn*rnx
      rcodcy = rcodcy -rcodcn*rny
      rcodcz = rcodcz -rcodcn*rnz
      rcodcl(ifac,iu,1) = rcodcx
      rcodcl(ifac,iv,1) = rcodcy
      rcodcl(ifac,iw,1) = rcodcz
    endif

    ! Relative tangential velocity

    upx = velipb(ifac,1) - rcodcx
    upy = velipb(ifac,2) - rcodcy
    upz = velipb(ifac,3) - rcodcz

    usn = upx*rnx+upy*rny+upz*rnz
    tx  = upx -usn*rnx
    ty  = upy -usn*rny
    tz  = upz -usn*rnz
    txn = sqrt(tx**2 +ty**2 +tz**2)
    utau= txn

    ! Unit tangent

    if (txn.ge.epzero) then

      txn0 = 1.d0

      tx  = tx/txn
      ty  = ty/txn
      tz  = tz/txn

    elseif (itytur.eq.3) then

      ! If the velocity is zero, vector T is normal and random;
      !   we need it for the reference change for Rij, and we cancel the velocity.

      txn0 = 0.d0

      if (abs(rny).ge.epzero.or.abs(rnz).ge.epzero)then
        rxnn = sqrt(rny**2+rnz**2)
        tx  =  0.d0
        ty  =  rnz/rxnn
        tz  = -rny/rxnn
      elseif (abs(rnx).ge.epzero.or.abs(rnz).ge.epzero)then
        rxnn = sqrt(rnx**2+rnz**2)
        tx  =  rnz/rxnn
        ty  =  0.d0
        tz  = -rnx/rxnn
      else
        write(nfecra,1000)ifac,rnx,rny,rnz
        call csexit (1)
      endif

    else

      ! If the velocity is zero, and we are not using Reynolds Stresses,
      !  Tx, Ty, Tz is not used (we cancel the velocity), so we assign any
      !  value (zero for example)

      txn0 = 0.d0

      tx  = 0.d0
      ty  = 0.d0
      tz  = 0.d0

    endif

    ! Complete if necessary for Rij-Epsilon

    if (itytur.eq.3) then

      ! --> T2 = RN X T (where X is the cross product)

      t2x = rny*tz - rnz*ty
      t2y = rnz*tx - rnx*tz
      t2z = rnx*ty - rny*tx

      ! --> Orthogonal matrix for change of reference frame ELOGLOij
      !     (from local to global reference frame)

      !                      |TX  -RNX  T2X|
      !             ELOGLO = |TY  -RNY  T2Y|
      !                      |TZ  -RNZ  T2Z|

      !    Its transpose ELOGLOt is its inverse

      eloglo(1,1) =  tx
      eloglo(1,2) = -rnx
      eloglo(1,3) =  t2x
      eloglo(2,1) =  ty
      eloglo(2,2) = -rny
      eloglo(2,3) =  t2y
      eloglo(3,1) =  tz
      eloglo(3,2) = -rnz
      eloglo(3,3) =  t2z

      ! --> Commpute alpha(6,6)

      ! Let f be the center of the boundary faces and
      !   I the center of the matching cell

      ! We noteE Rg (resp. Rl) indexed by f or by I
      !   the Reynolds Stress tensor in the global basis (resp. local)

      ! The alpha matrix applied to the global vector in I'
      !   (Rg11,I'|Rg22,I'|Rg33,I'|Rg12,I'|Rg13,I'|Rg23,I')t
      !    must provide the values to prescribe to the face
      !   (Rg11,f |Rg22,f |Rg33,f |Rg12,f |Rg13,f |Rg23,f )t
      !    except for the Dirichlet boundary conditions (added later)

      ! We define it by computing Rg,f as a function of Rg,I' as follows

      !   RG,f = ELOGLO.RL,f.ELOGLOt (matrix products)

      !                     | RL,I'(1,1)     B*U*.Uk     C*RL,I'(1,3) |
      !      with    RL,f = | B*U*.Uk       RL,I'(2,2)       0        |
      !                     | C*RL,I'(1,3)     0         RL,I'(3,3)   |

      !             with    RL,I = ELOGLOt.RG,I'.ELOGLO
      !                     B = 0
      !              and    C = 0 at the wall (1 with symmetry)

      ! We compute in fact  ELOGLO.projector.ELOGLOt

      clsyme=0.d0
      call clca66 (clsyme , eloglo , alpha)
      !==========

    endif

    !===========================================================================
    ! 2. Friction velocities
    !===========================================================================

    ! ---> Compute Uet depending if we are in the log zone or not
    !      in 1 or 2 velocity scales
    !      and uk based on ek


    if (abs(utau).le.epzero) utau = epzero

    ! rugd: rugosite de paroi pour les variables dynamiques
    !       seule la valeur stockee pour iu est utilisee
    rugd=rcodcl(ifac,iu,3)

    ! NB: for rought walls, yplus is computed from the roughtness
    !     and not uk.
    yplus=distbf/rugd

    ! Pseudo decalage de la paroi de rugd ((distbf+rugd)/rugd)
    uet = utau/log(yplus+1.d0)*xkappa

    ! Dimensionless velocity
    uplus = log(yplus+1.d0)/xkappa

    if (ideuch.eq.0) then
      uk = uet

    ! Si ideuch=1 ou 2 on calcule uk et uet
    else

      if (itytur.eq.2 .or. itytur.eq.5 .or. iturb.eq.60) then
        ek = rtp(iel,ik)
      else if(itytur.eq.3) then
        ek = 0.5d0*(rtp(iel,ir11)+rtp(iel,ir22)+rtp(iel,ir33))
      endif

      uk = cmu025*sqrt(ek)

    endif

    if (ippmod(iatmos).ge.1) then

      ! Compute reduced gravity for non horizontal walls :
      gredu = gx*rnx + gy*rny + gz*rnz

      call atmcls & !FIXME TODO
      !==========
    ( nvar   , nscal  ,                                              &
      ifac   , iel    ,                                              &
      uk     , utau   , yplus  ,                                     &
      uet    ,                                                       &
      gredu  , q0     , e0     , rib    ,lmo     ,                   &
      cfnnu  , cfnns  , cfnnk  , cfnne  ,                            &
      icodcl ,                                                       &
      dt     , rtp    , propce , propfa , propfb , rcodcl )

    endif

    if (ideuch.eq.0) then
      uk = uet
    endif

    uetmax = max(uet,uetmax)
    uetmin = min(uet,uetmin)
    ukmax  = max(uk,ukmax)
    ukmin  = min(uk,ukmin)
    yplumx = max(yplus,yplumx)
    yplumn = min(yplus,yplumn)

    ! Sauvegarde de la vitesse de frottement et de la viscosite turbulente
    ! apres amortissement de van Driest pour la LES
    ! On n'amortit pas mu_t une seconde fois si on l'a deja fait
    ! (car une cellule peut avoir plusieurs faces de paroi)
    ! ou
    ! Sauvegarde de la vitesse de frottement et distance a la paroi yplus
    ! si le modele de depot de particules est active.

    if (itytur.eq.4.and.idries.eq.1) then
      uetbor(ifac) = uet
      if (visvdr(iel).lt.-900.d0) then
        propce(iel,ipcvst) = propce(iel,ipcvst)
        ! FIXME amortissement de van Driest a revoir en rugueux :
        !    &             *(1.D0-EXP(-YPLUS/CDRIES))**2
        visvdr(iel) = propce(iel,ipcvst)
        visctc = propce(iel,ipcvst)
      endif
    else if (iilagr.gt.0.and.idepst.gt.0) then
      uetbor(ifac) = uet
    endif


    ! Save yplus is post-processed
    if (mod(ipstdv,ipstyp).eq.0) then
      yplbr(ifac) = yplus
    endif

    !===========================================================================
    ! 3. Velocity boundary conditions
    !===========================================================================

    ! uiptn respecte la production de k
    !  de facon conditionnelle --> Coef RCPROD
    ! uiptnf respecte le flux
    !  de facon conditionnelle --> Coef RCFLUX

    ! --> All turbulence models (except v2f and EBRSM)
    !-------------------------------------------------
    if (itytur.eq.2 .or. iturb.eq.60 .or.        &
         iturb.eq.0 .or. iturb.eq.10 .or.        &
         iturb.eq.30.or. iturb.eq.31 .or.        &
        itytur.eq.4 .or.                         &
         iturb.eq.70        ) then

      if (visctc.gt.epzero) then

        ! Pseudo decalage de la paroi de la distance rugd :
        ! TODO modified for non neutral boundary layer (cfnnu)
        distb0=distbf+rugd
        xmutlm = xkappa*uk*distb0*romc

        rcprod = max(distbf/distb0,                                &
                     deuxd0*distbf/distb0*sqrt(xmutlm/visctc)      &
                     - 1.d0/(2.d0+rugd/distb0))

        rcflux = max(xmutlm,visctc)/(visclc+visctc)*distbf/distb0

        ! FIXME: it should be uet/xkappa and not uk!
        ! FIXME the "min" is useless.
        uiptn  = min(utau,max(utau - uk/xkappa*rcprod*cfnnu,0.d0))
        uiptnf = utau - uet/xkappa*rcflux*cfnnu
        iuntur = 1

        ! Coupled solving of the velocity components
        if (ivelco.eq.1) then
          ! The boundary term for velocity gradient is implicit
          ypup = distbf*uk*romc/visclc/uplus
          cofimp  = max(1.d0 - 1.d0/(xkappa*uplus)*rcprod*cfnnu, 0.d0)
          ! The term (rho*uet*uk) is implicit
          hflui = visclc / distbf * ypup
        endif

      ! In the viscous sub-layer
      else
        uiptn  = 0.d0
        uiptnf = 0.d0
        iuntur = 0

        ! Coupled solving of the velocity components
        if (ivelco.eq.1) then
          cofimp  = 0.d0
          hflui = visclc / distbf
        endif

      endif

      ! Clipping :
      ! On borne U_f,grad entre 0 et Utau (il y a surement mieux...)
      ! - 0    : on interdit le retournement en face de bord, qui est en
      !          contradiction avec l'hypoth�se de loi log.
      ! - Utau : la production turbulente ne peut etre nulle
      ! On empeche U_f,flux d'etre negatif

      if (uiptnf.lt.epzero) uiptnf = 0.d0

    ! --> v2f and EBRSM!FIXME EBRSM
    !------------------
    elseif (itytur.eq.5) then

      ! Avec ces conditions, pas besoin de calculer uiptmx, uiptmn
      ! et iuiptn qui sont nuls (valeur d'initialisation)
      iuntur = 0
      uiptn  = 0.d0
      uiptnf = 0.d0

      ! Coupled solving of the velocity components
      if(ivelco.eq.1) then
        hflui = (visclc + visctc) / distbf
        cofimp = 0.d0
      endif

    endif

    ! Min and Max and counter of reversal layer
    uiptmn = min(uiptn*iuntur,uiptmn)
    uiptmx = max(uiptn*iuntur,uiptmx)
    if (uiptn*iuntur.lt.-epzero) iuiptn = iuiptn + 1

    if (itytur.eq.3) then
      hint =  visclc          /distbf
    else
      hint = (visclc + visctc)/distbf
    endif

    coefa(ifac,iclu)   = uiptn *tx*iuntur*txn0 + rcodcx
    coefa(ifac,iclv)   = uiptn *ty*iuntur*txn0 + rcodcy
    coefa(ifac,iclw)   = uiptn *tz*iuntur*txn0 + rcodcz
    coefa(ifac,icluf)  = -hint*(uiptnf*tx*iuntur*txn0 + rcodcx)
    coefa(ifac,iclvf)  = -hint*(uiptnf*ty*iuntur*txn0 + rcodcy)
    coefa(ifac,iclwf)  = -hint*(uiptnf*tz*iuntur*txn0 + rcodcz)

    coefb(ifac,iclu)   = 0.d0
    coefb(ifac,iclv)   = 0.d0
    coefb(ifac,iclw)   = 0.d0
    coefb(ifac,icluf)  = hint
    coefb(ifac,iclvf)  = hint
    coefb(ifac,iclwf)  = hint

    ! Coupled solving of the velocity components
    if (ivelco.eq.1) then

      ! Gradient boundary conditions
      !-----------------------------

      coefau(1,ifac) = rcodcx
      coefau(2,ifac) = rcodcy
      coefau(3,ifac) = rcodcz

      ! Projection in order to have the velocity parallel to the wall
      ! B = cofimp * ( IDENTITY - n x n )

      coefbu(1,1,ifac) = cofimp*(1.d0-rnx**2)
      coefbu(2,2,ifac) = cofimp*(1.d0-rny**2)
      coefbu(3,3,ifac) = cofimp*(1.d0-rnz**2)
      coefbu(1,2,ifac) = -cofimp*rnx*rny
      coefbu(1,3,ifac) = -cofimp*rnx*rnz
      coefbu(2,1,ifac) = -cofimp*rny*rnx
      coefbu(2,3,ifac) = -cofimp*rny*rnz
      coefbu(3,1,ifac) = -cofimp*rnz*rnx
      coefbu(3,2,ifac) = -cofimp*rnz*rny

      ! Flux boundary conditions
      !-------------------------
      rcodcn = rcodcx*rnx+rcodcy*rny+rcodcz*rnz

      cofafu(1,ifac)   = -hflui*(rcodcx - rcodcn*rnx)
      cofafu(2,ifac)   = -hflui*(rcodcy - rcodcn*rny)
      cofafu(3,ifac)   = -hflui*(rcodcz - rcodcn*rnz)

      ! Projection in order to have the shear stress parallel to the wall
      !  B = hflui*( IDENTITY - n x n )

      cofbfu(1,1,ifac) = hflui*(1.d0-rnx**2)
      cofbfu(2,2,ifac) = hflui*(1.d0-rny**2)
      cofbfu(3,3,ifac) = hflui*(1.d0-rnz**2)

      cofbfu(1,2,ifac) = - hflui*rnx*rny
      cofbfu(1,3,ifac) = - hflui*rnx*rnz
      cofbfu(2,1,ifac) = - hflui*rny*rnx
      cofbfu(2,3,ifac) = - hflui*rny*rnz
      cofbfu(3,1,ifac) = - hflui*rnz*rnx
      cofbfu(3,2,ifac) = - hflui*rnz*rny

    endif

    !===========================================================================
    ! 4. Boundary conditions on k and espilon
    !===========================================================================

    ydep = distbf*0.5d0+rugd

    if (itytur.eq.2) then

      ! Dirichlet Boundary Condition on k
      !----------------------------------

      iclvar = iclk
      iclvaf = iclkf

      pimp = uk**2/sqrcmu*cfnnk
      hint = (visclc+visctc/sigmak)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Neumann Boundary Condition on epsilon
      !--------------------------------------

      iclvar = iclep
      iclvaf = iclepf

      hint = (visclc+visctc/sigmae)/distbf

      pimp = uk**3/(xkappa*ydep**2)*distbf*cfnne
      qimp = -pimp*hint !TODO transform it, it is only to be fully equivalent

      call set_neumann_scalar &
           !==================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           qimp              , hint )

    !===============================================================================
    ! 5. Boundary conditions on Rij-epsilon
    !===============================================================================

    elseif (itytur.eq.3) then

      ! ---> Tensor Rij (Partially implicited)

      do isou = 1, 6

        if (isou.eq.1) then
          iclvar = icl11
          iclvrr = icl11r
        elseif (isou.eq.2) then
          iclvar = icl22
          iclvrr = icl22r
        elseif (isou.eq.3) then
          iclvar = icl33
          iclvrr = icl33r
        elseif (isou.eq.4) then
          iclvar = icl12
          iclvrr = icl12r
        elseif (isou.eq.5) then
          iclvar = icl13
          iclvrr = icl13r
        elseif (isou.eq.6) then
          iclvar = icl23
          iclvrr = icl23r
        endif

        coefa(ifac,iclvar) = 0.0d0
        coefb(ifac,iclvar) = 0.0d0
        coefa(ifac,iclvrr) = 0.0d0
        coefb(ifac,iclvrr) = 0.0d0

      enddo

      do isou = 1,6

        if (isou.eq.1) then
          iclvar = icl11
          iclvrr = icl11r
          jj = 1
          kk = 1
        else if (isou.eq.2) then
          iclvar = icl22
          iclvrr = icl22r
          jj = 2
          kk = 2
        else if (isou.eq.3) then
          iclvar = icl33
          iclvrr = icl33r
          jj = 3
          kk = 3
        else if (isou.eq.4) then
          iclvar = icl12
          iclvrr = icl12r
          jj = 1
          kk = 2
        else if (isou.eq.5) then
          iclvar = icl13
          iclvrr = icl13r
          jj = 1
          kk = 3
        else if (isou.eq.6) then
          iclvar = icl23
          iclvrr = icl23r
          jj = 2
          kk = 3
        endif

        if (iclptr.eq.1) then
          do ii = 1, 6
            if(ii.ne.isou) then
              coefa(ifac,iclvar) = coefa(ifac,iclvar) +           &
                   alpha(isou,ii) * rijipb(ifac,ii)
            endif
          enddo
          coefb(ifac,iclvar) = alpha(isou,isou)
        else
          do ii = 1, 6
            coefa(ifac,iclvar) = coefa(ifac,iclvar) +             &
                 alpha(isou,ii) * rijipb(ifac,ii)
          enddo
          coefb(ifac,iclvar) = 0.d0
        endif
        ! Boundary conditions for the momentum equation
        coefa(ifac,iclvrr) = coefa(ifac,iclvar)
        coefb(ifac,iclvrr) = coefb(ifac,iclvar)

        coefa(ifac,iclvar) = coefa(ifac,iclvar)  -                &
                           (eloglo(jj,1)*eloglo(kk,2)+            &
                            eloglo(jj,2)*eloglo(kk,1))*uet*uk*cfnnk

        ! WARNING
        ! Translate coefa into cofaf and coefb into cofbf Done in resssg.f90
      enddo

      ! ---> Epsilon
      ! same treatment as k-epsilon

      ! Neumann Boundary Condition on epsilon
      !--------------------------------------

      iclvar = iclep
      iclvaf = iclepf

      hint = (visclc+visctc/sigmae)/distbf

      pimp = uk**3/(xkappa*ydep**2)*distbf*cfnne
      qimp = -pimp*hint !TODO transform it, it is only to be fully equivalent

      call set_neumann_scalar &
           !==================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           qimp              , hint )

    !===========================================================================
    ! 6a.Boundary conditions on k, epsilon, f_bar and phi in the phi_Fbar model
    !===========================================================================

    elseif (iturb.eq.50) then

      ! Dirichlet Boundary Condition on k
      !----------------------------------

      iclvar = iclk
      iclvaf = iclkf

      pimp = 0.d0
      hint = (visclc+visctc/sigmak)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on epsilon
      !----------------------------------------

      iclvar = iclep
      iclvaf = iclepf

      pimp = 2.0d0*visclc/romc*rtp(iel,ik)/distbf**2
      hint = (visclc+visctc/sigmae)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on Phi
      !------------------------------------

      iclvar = iclphi
      iclvaf = iclphf

      pimp = 0.d0
      hint = (visclc+visctc/sigmak)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on Fb
      !-----------------------------------

      iclvar = iclfb
      iclvaf = iclfbf

      pimp = 0.d0
      hint = 1.d0/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

    !===========================================================================
    ! 6b.Boundary conditions on k, epsilon, phi and alpha in the Bl-v2/k model
    !===========================================================================

    elseif (iturb.eq.51) then

      ! Dirichlet Boundary Condition on k
      !----------------------------------

      iclvar = iclk
      iclvaf = iclkf

      pimp = 0.d0
      hint = (visclc+visctc/sigmak)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on epsilon
      !----------------------------------------

      iclvar = iclep
      iclvaf = iclepf
      pimp = visclc/romc*rtp(iel,ik)/distbf**2
      hint = (visclc+visctc/sigmae)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on Phi
      !------------------------------------

      iclvar = iclphi
      iclvaf = iclphf

      pimp = 0.d0
      hint = (visclc+visctc/sigmak)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Dirichlet Boundary Condition on alpha
      !--------------------------------------

      iclvar = iclal
      iclvaf = iclalf

      pimp = 0.d0
      hint = 1.d0/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )


    !===========================================================================
    ! 7. Boundary conditions on k and omega
    !===========================================================================

    elseif (iturb.eq.60) then

      ! Always out of the viscous sub-layer

      ! Dirichlet Boundary Condition on k
      !----------------------------------

      iclvar = iclk
      iclvaf = iclkf

      pimp = uk**2/sqrcmu

      !FIXME it is wrong because sigma is computed within the model
      ! see turbkw.f90
      hint = (visclc+visctc/ckwsk2)/distbf

      call set_dirichlet_scalar &
           !====================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           pimp              , hint              , rinfin )

      ! Neumann Boundary Condition on omega
      !------------------------------------

      iclvar = iclomg
      iclvaf = iclomf

      !FIXME it is wrong because sigma is computed within the model
      ! see turbkw.f90 (So the flux is not the one we impose!)
      hint = (visclc+visctc/ckwsw2)/distbf

      pimp = distbf*4.d0*uk**3*romc**2/           &
            (sqrcmu*xkappa*visclc**2*yplus**2)
      qimp = -pimp*hint !TODO transform it, it is only to be fully equivalent

      call set_neumann_scalar &
           !==================
         ( coefa(ifac,iclvar), coefa(ifac,iclvaf),             &
           coefb(ifac,iclvar), coefb(ifac,iclvaf),             &
           qimp              , hint )

    !===========================================================================
    ! 7.1 Boundary conditions on the Spalart Allmaras turbulence model
    !===========================================================================

    elseif (iturb.eq.70) then

      iclvar = iclnu
      iclvaf = iclnuf

      dsa0 = rugd

      coefa(ifac,iclvar) = 0.d0
      coefb(ifac,iclvar) = dsa0/(dsa0+distbf) !TODO

    endif

    !===========================================================================
    ! 8. Boundary conditions on the other scalars
    !    (Specific treatment for the variances of the scalars next to walls:
    !     see condli)
    !===========================================================================

    if (nscal.ge.1) then

      do iscal = 1, nscal

        if (iscavr(iscal).le.0) then

          ivar = isca(iscal)
          iclvar = iclrtp(ivar,icoef)
          iclvaf = iclrtp(ivar,icoeff)

          isvhbl = 0
          if (iscal.eq.isvhb) then
            isvhbl = isvhb
          endif

          ihcp = 0
          if (iscsth(iscal).eq.0.or.     &
              iscsth(iscal).eq.2.or.     &
              iscsth(iscal).eq.3) then
            ihcp = 0
          elseif (abs(iscsth(iscal)).eq.1) then
            if (ipccp.gt.0) then
              ihcp = 2
            else
              ihcp = 1
            endif
          endif

          cpp = 1.d0
          if (ihcp.eq.0) then
            cpp = 1.d0
          elseif (ihcp.eq.2) then
            cpp = propce(iel,ipccp )
          elseif (ihcp.eq.1) then
            cpp = cp0
          endif

          if (ivisls(iscal).gt.0) then
            ipcvsl = ipproc(ivisls(iscal))
          else
            ipcvsl = 0
          endif
          ! FIXME useless
          if (ipcvsl.le.0) then
            rkl = visls0(iscal)
            if (abs(iscsth(iscal)).eq.1) then
              prdtl = cpp*visclc/rkl
            else
              prdtl = visclc/rkl
            endif
          else
            rkl = propce(iel,ipcvsl)
            if (abs(iscsth(iscal)).eq.1) then
              prdtl = cpp*visclc/rkl
            else
              prdtl = visclc/rkl
            endif
          endif

          ! --> Compressible module:
          !  On suppose que le nombre de Pr doit etre
          !  defini de la meme facon que l'on resolve
          !  en enthalpie ou en energie, soit Mu*Cp/Lambda.
          !  Si l'on resout en energie, on a calcule ci-dessus
          !  Mu*Cv/Lambda.

          if (ippmod(icompf).ge.0) then
            if (iscsth(iscal).eq.3) then
              if (ipccp.gt.0) then
                prdtl = prdtl*propce(iel,ipccp )
              else
                prdtl = prdtl*cp0
              endif
              if (ipccv.gt.0) then
                prdtl = prdtl/propce(iel,ipccv )
              else
                prdtl = prdtl/cv0
              endif
            endif
          endif

          ! Turbulent
          if (iturb.ne.0) then
            ! En compressible, pour l'energie LAMBDA/CV+CP/CV*(MUT/SIGMAS)
            if (ippmod(icompf) .ge. 0) then
              if (ipccp.gt.0) then
                cpscv = propce(iel,ipproc(icp))
              else
                cpscv = cp0
              endif
              if (ipccv.gt.0) then
                cpscv = cpscv/propce(iel,ipproc(icv))
              else
                cpscv = cpscv/cv0
              endif
              hint = (rkl+cpp*cpscv*visctc/sigmas(iscal))/distbf
            else
              hint = (rkl+cpp*visctc/sigmas(iscal))/distbf
            endif

          ! Laminar
          else
            hint  = rkl/distbf
          endif

          ! Loi rugueuse, on recalcule le coefficient d'echange fluide - paroi
          if (iturb.ne.0.and.icodcl(ifac,ivar).eq.6)then

            rugt = rcodcl(ifac,iv,3)
            act = xkappa/log((distbf+rugt)/rugt)
            hflui = romc*uet*act*cfnns
          else
            hflui = hint
          endif

          if (isvhbl.gt.0) hbord(ifac) = hflui

          ! --->  C.L DE TYPE DIRICHLET AVEC OU SANS COEFFICIENT D'ECHANGE

          ! Si on a deux types de conditions aux limites (ICLVAR, ICLVAF)
          !   il faut que le flux soit traduit par ICLVAF.
          ! Si on n'a qu'un type de condition, peu importe (ICLVAF=ICLVAR)
          ! Pour le moment, dans cette version compressible, on impose un
          !   flux nul pour ICLVAR, lorsqu'il est different de ICLVAF (cette
          !   condition ne sert qu'a la reconstruction des gradients et
          !   s'applique a l'energie totale qui inclut l'energie cinetique :


          if (icodcl(ifac,ivar).eq.6) then
            hext = rcodcl(ifac,ivar,2)
            pimp = rcodcl(ifac,ivar,1)

            if (abs(hext).gt.rinfin*0.5d0) then

              ! Gradient BCs
              hredui = hint/hflui
              coefa(ifac,iclvar) = pimp/hredui
              coefb(ifac,iclvar) = (hredui-1.d0)/hredui
              !FIXME ou un flux nul
              !coefa(ifac,iclvar) = 0.d0
              !coefb(ifac,iclvar) = 1.d0

              ! Flux BCs
              coefa(ifac,iclvaf) = -hflui*pimp
              coefb(ifac,iclvaf) =  hflui

            else

              ! Gradient BCs
              hredui = hint/hflui
              coefa(ifac,iclvar) = hext*pimp/(hint+hext*hredui)
              coefb(ifac,iclvar) = (hint-(1.d0-hredui)*hext)/       &
                                 (hint+hext*hredui)
              !FIXME or zero flux?
              !coefa(ifac,iclvar) = 0.d0
              !coefb(ifac,iclvar) = 1.d0

              ! Flux BCs
              heq = hflui*hext/(hflui+hext)
              coefa(ifac,iclvaf) = -heq*pimp
              coefb(ifac,iclvaf) =  heq

            endif

            !--> Radiative module:

            ! On stocke le coefficient d'echange lambda/distance
            ! (ou son equivalent en turbulent) quelle que soit la
            ! variable thermique transportee (temperature ou enthalpie)
            ! car on l'utilise pour realiser des bilans aux parois qui
            ! sont faits en temperature (on cherche la temperature de
            ! paroi quelle que soit la variable thermique transportee pour
            ! ecrire des eps sigma T4.

!     donc :

!       lorsque la variable transportee est la temperature
!         ABS(ISCSTH(II)).EQ.1 : RA(IHCONV-1+IFAC) = HINT
!         puisque HINT = VISLS * CP / DISTBR
!                      = lambda/distance en W/(m2 K)

!       lorsque la variable transportee est l'enthalpie
!         ISCSTH(II).EQ.2 : RA(IHCONV-1+IFAC) = HINT*CPR
!         avec
!            IF (IPCCP.GT.0) THEN
!              CPR = PROPCE(IEL,IPCCP )
!            ELSE
!              CPR = CP0
!            ENDIF
!         puisque HINT = VISLS / DISTBR
!                      = lambda/(CP * distance)

!       lorsque la variable transportee est l'energie (compressible)
!         ISCSTH(II).EQ.3 :
!         on procede comme pour l'enthalpie avec CV au lieu de CP
!         (rq : il n'y a pas d'hypothese, sf en non orthogonal :
!               le flux est le bon et le coef d'echange aussi)

!      De meme dans condli.

!               Si on rayonne et que
!                  le scalaire est la variable energetique

            if (iirayo.ge.1 .and. iscal.eq.iscalt) then

              ! We compute the exchange coefficient in W/(m2 K)

              ! Enthalpy
              if (iscsth(iscal).eq.2) then
                ! If Cp is variable
                if (ipccp.gt.0) then
                  propfb(ifac,ipprob(ihconv)) = hflui*propce(iel, ipccp)
                else
                  propfb(ifac,ipprob(ihconv)) = hflui*cp0
                endif

              ! Energie (compressible module)
              elseif (iscsth(iscal).eq.3) then
                ! If Cv is variable
                if (ipccv.gt.0) then
                  propfb(ifac,ipprob(ihconv)) = hflui*propce(iel,ipccv)
                else
                  propfb(ifac,ipprob(ihconv)) = hflui*cv0
                endif

              ! Temperature
              elseif (abs(iscsth(iscal)).eq.1) then
                propfb(ifac,ipprob(ihconv)) = hflui
              endif

              ! The outgoing flux is stored (Q = h(Ti'-Tp): negative if
              !  gain for the fluid) in W/m2
              propfb(ifac,ipprob(ifconv)) = coefa(ifac,iclvaf)              &
                                          + coefb(ifac,iclvaf)*thbord(ifac)
            endif

          endif

        endif

      enddo

    endif


  endif
  ! Test on the presence of a rought wall (End)

enddo
! --- End of loop over faces

if (irangp.ge.0) then
  call parmin (uiptmn)
  !==========
  call parmax (uiptmx)
  !==========
  call parmin (uetmin)
  !==========
  call parmax (uetmax)
  !==========
  call parmin (ukmin)
  !==========
  call parmax (ukmax)
  !==========
  call parmin (yplumn)
  !==========
  call parmax (yplumx)
  !==========
  call parcpt (iuiptn)
  !==========
endif

!===============================================================================
! 9. Writtings
!===============================================================================

!     Remarque : afin de ne pas surcharger les listings dans le cas ou
!       quelques yplus ne sont pas corrects, on ne produit le message
!       qu'aux deux premiers pas de temps ou le message apparait et
!       aux deux derniers pas de temps du calcul, ou si IWARNI est >= 2
!       On indique aussi le numero du dernier pas de temps auquel on
!       a rencontre des yplus hors bornes admissibles

if (iwarni(iu).ge.0) then
  if (ntlist.gt.0) then
    modntl = mod(ntcabs,ntlist)
  elseif (ntlist.eq.-1.and.ntcabs.eq.ntmabs) then
    modntl = 0
  else
    modntl = 1
  endif

  if (modntl.eq.0 .or. iwarni(iu).ge.2) then
    write(nfecra,2010) &
         uiptmn,uiptmx,uetmin,uetmax,ukmin,ukmax,yplumn,yplumx,   &
         iuiptn
  endif

endif

!===============================================================================
! 10. Formats
!===============================================================================

#if defined(_CS_LANG_FR)

 1000 format(/,' LA NORMALE A LA FACE DE BORD DE PAROI ',I10,/,   &
         ' EST NULLE ; COORDONNEES : ',3E12.5)

 2010 format(/,                                                   &
 3X,'** CONDITIONS AUX LIMITES EN PAROI RUGUEUSE',/,        &
 '   -------------------------------------------',/,        &
 '------------------------------------------------------------',/,&
 '                                         Minimum     Maximum',/,&
 '------------------------------------------------------------',/,&
 '   Vitesse rel. en paroi    uiptn : ',2E12.5                 ,/,&
 '   Vitesse de frottement    uet   : ',2E12.5                 ,/,&
 '   Vitesse de frottement    uk    : ',2E12.5                 ,/,&
 '   Distance adim. rugueuse  yplus : ',2E12.5                 ,/,&
 '   ------------------------------------------------------   ',/,&
 '   Nbre de retournements de la vitesse en paroi : ',I10      ,/,&
 '------------------------------------------------------------',  &
 /,/)


#else

 1000 format(/,' THE NORMAL TO THE WALL BOUNDARY FACE ',I10,/,    &
         ' IS NULL; COORDINATES: ',3E12.5)

 2010 format(/,                                                   &
 3X,'** BOUNDARY CONDITIONS FOR ROUGH WALLS',/,             &
 '   --------------------------------------',/,             &
 '------------------------------------------------------------',/,&
 '                                         Minimum     Maximum',/,&
 '------------------------------------------------------------',/,&
 '   Rel velocity at the wall uiptn : ',2E12.5                 ,/,&
 '   Friction velocity        uet   : ',2E12.5                 ,/,&
 '   Friction velocity        uk    : ',2E12.5                 ,/,&
 '   Rough dimensionless dist yplus : ',2E12.5                 ,/,&
 '   ------------------------------------------------------   ',/,&
 '   Nb of reversal of the velocity at the wall   : ',I10      ,/,&
 '------------------------------------------------------------',  &
 /,/)


#endif

!----
! End
!----

return
end subroutine
