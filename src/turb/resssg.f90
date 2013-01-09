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

subroutine resssg &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   ivar   , isou   , ipp    ,                                     &
   icepdc , icetsm , itpsmp ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , grdvit , gradro ,                            &
   ckupdc , smcelp , gamma  ,                                     &
   viscf  , viscb  ,                                              &
   tslage , tslagi ,                                              &
   smbr   , rovsdt )

!===============================================================================
! FONCTION :
! ----------

! RESOLUTION DES EQUATIONS CONVECTION DIFFUSION TERME SOURCE
!   POUR les modeles Rij SSG (31) et SSG avec ponderation
!   elliptique (EBRSM) (32)
! VAR  = R11 R22 R33 R12 R13 R23
! ISOU =  1   2   3   4   5   6

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncepdp           ! i  ! <-- ! number of cells with head loss                 !
! ncesmp           ! i  ! <-- ! number of cells with mass source term          !
! ivar             ! i  ! <-- ! variable number                                !
! isou             ! e  ! <-- ! numero de passage                              !
! ipp              ! e  ! <-- ! numero de variable pour sorties post           !
! icepdc(ncelet    ! te ! <-- ! numero des ncepdp cellules avec pdc            !
! icetsm(ncesmp    ! te ! <-- ! numero des cellules a source de masse          !
! itpsmp           ! te ! <-- ! type de source de masse pour la                !
! (ncesmp)         !    !     !  variables (cf. ustsma)                        !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! grdvit           ! tr ! --- ! tableau de travail pour terme grad             !
!  (ncelet,3,3)    !    !     !                         de vitesse             !
! gradro(ncelet,3) ! tr ! <-- ! tableau de travail pour grad rom               !
! ckupdc           ! tr ! <-- ! tableau de travail pour pdc                    !
!  (ncepdp,6)      !    !     !                                                !
! smcelp(ncesmp    ! tr ! <-- ! valeur de la variable associee a la            !
!                  !    !     !  source de masse                               !
! gamma(ncesmp)    ! tr ! <-- ! valeur du flux de masse                        !
! viscf(nfac)      ! tr ! --- ! visc*surface/dist aux faces internes           !
! viscb(nfabor     ! tr ! --- ! visc*surface/dist aux faces de bord            !
! tslage(ncelet    ! tr ! <-- ! ts explicite couplage retour lagr.             !
! tslagi(ncelet    ! tr ! <-- ! ts implicite couplage retour lagr.             !
! smbr(ncelet      ! tr ! --- ! tableau de travail pour sec mem                !
! rovsdt(ncelet    ! tr ! --- ! tableau de travail pour terme instat           !
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
use entsor
use optcal
use cstphy
use cstnum
use parall
use period
use lagran
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp
integer          ivar   , isou   , ipp

integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itpsmp(ncesmp)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision grdvit(ncelet,3,3)
double precision gradro(ncelet,3)
double precision ckupdc(ncepdp,6)
double precision smcelp(ncesmp), gamma(ncesmp)
double precision viscf(nfac), viscb(nfabor)
double precision tslage(ncelet),tslagi(ncelet)
double precision smbr(ncelet), rovsdt(ncelet)

! Local variables

integer          init  , ifac  , iel
integer          ii    , jj    , kk    , iiun  , iii   , jjj
integer          ipcrom, ipcvis, iflmas, iflmab, ipcroo
integer          iclvar, iclvaf, iclal , iclalf
integer          nswrgp, imligp, iwarnp
integer          iconvp, idiffp, ndircp, ireslp
integer          nitmap, nswrsp, ircflp, ischcp, isstpp, iescap
integer          imgrp , ncymxp, nitmfp
integer          iptsta
integer          inc, iccocg, iphydp, ll, kkk
integer          ipcvlo
integer          idimte, itenso
integer          imucpp, idftnp, iswdyp
integer          indrey(3,3)

double precision blencp, epsilp, epsrgp, climgp, extrap, relaxp
double precision epsrsp
double precision trprod, trrij , rctse , deltij
double precision tuexpr, thets , thetv , thetp1
double precision aiksjk, aikrjk, aii ,aklskl, aikakj
double precision xaniso(3,3), xstrai(3,3), xrotac(3,3), xprod(3,3), matrot(3,3)
double precision xrij(3,3), xnal(3), xnoral, xnnd, xnu
double precision d1s2, d1s3, d2s3, thetap
double precision alpha3
double precision xttke, xttkmg, xttdrb
double precision grdpx, grdpy, grdpz, grdsn, surfn2
double precision pij, phiij1, phiij2, phiij3, epsij
double precision phiijw, epsijw
double precision hint

double precision rvoid(1)

double precision, allocatable, dimension(:,:) :: grad
double precision, allocatable, dimension(:) :: w1, w2, w3
double precision, allocatable, dimension(:) :: w4, w5, w6, w7
double precision, allocatable, dimension(:) :: dpvar

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet))
allocate(dpvar(ncelet))

! Initialize variables to avoid compiler warnings

iclal = 0
iii = 0
jjj = 0

! Memoire

if(iwarni(ivar).ge.1) then
  write(nfecra,1000) nomvar(ipp)
endif

ipcrom = ipproc(irom  )
ipcvis = ipproc(iviscl)
iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))

if (iturb.eq.32) then
  iclal  = iclrtp(ial,icoef)
  iclalf = iclrtp(ial,icoeff)
endif

ipcrom = ipproc(irom)
ipcvis = ipproc(iviscl)
iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))

iclvar = iclrtp(ivar,icoef)
iclvaf = iclrtp(ivar,icoeff)

d1s2   = 1.d0/2.d0
d1s3   = 1.d0/3.d0
d2s3   = 2.d0/3.d0

deltij = 1.0d0
if(isou.gt.3) then
  deltij = 0.0d0
endif

!     S pour Source, V pour Variable
thets  = thetst
thetv  = thetav(ivar )

ipcroo = ipcrom
if(isto2t.gt.0.and.iroext.gt.0) then
  ipcroo = ipproc(iroma)
endif
iptsta = 0
if(isto2t.gt.0) then
  iptsta = ipproc(itstua)
endif

do iel = 1, ncel
  smbr(iel) = 0.d0
enddo
do iel = 1, ncel
  rovsdt(iel) = 0.d0
enddo

!===============================================================================
! 2. TERMES SOURCES  UTILISATEURS
!===============================================================================
!(le deuxieme argument GRDVIT est lu en PRODUC dans ustsri, mais ce
! tableau n'est dimensionne et utilise qu'en modele Rij standard)

call ustsri                                                       &
!==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   ivar   ,                                                       &
   icepdc , icetsm , itpsmp ,                                     &
   dt     , rtpa   , propce , propfa , propfb ,                   &
   ckupdc , smcelp , gamma  , grdvit , grdvit ,                   &
   smbr   , rovsdt )

!     Si on extrapole les T.S.
if(isto2t.gt.0) then
  do iel = 1, ncel
!       Sauvegarde pour echange
    tuexpr = propce(iel,iptsta+isou-1)
!       Pour la suite et le pas de temps suivant
    propce(iel,iptsta+isou-1) = smbr(iel)
!       Second membre du pas de temps precedent
!       On suppose -ROVSDT > 0 : on implicite
!          le terme source utilisateur (le reste)
    smbr(iel) = rovsdt(iel)*rtpa(iel,ivar)  - thets*tuexpr
!       Diagonale
    rovsdt(iel) = - thetv*rovsdt(iel)
  enddo
else
  do iel = 1, ncel
    smbr(iel)   = rovsdt(iel)*rtpa(iel,ivar) + smbr(iel)
    rovsdt(iel) = max(-rovsdt(iel),zero)
  enddo
endif

!===============================================================================
! 3. TERMES SOURCES  LAGRANGIEN : COUPLAGE RETOUR
!===============================================================================

!     Ordre 2 non pris en compte
 if (iilagr.eq.2 .and. ltsdyn.eq.1) then
   do iel = 1,ncel
     smbr(iel)   = smbr(iel)   + tslage(iel)
     rovsdt(iel) = rovsdt(iel) + max(-tslagi(iel),zero)
   enddo
 endif

!===============================================================================
! 4. TERME SOURCE DE MASSE
!===============================================================================


if (ncesmp.gt.0) then

!       Entier egal a 1 (pour navsto : nb de sur-iter)
  iiun = 1

!       On incremente SMBR par -Gamma RTPA et ROVSDT par Gamma (*theta)
  call catsma                                                     &
  !==========
 ( ncelet , ncel   , ncesmp , iiun   , isto2t , thetv  ,   &
   icetsm , itpsmp ,                                              &
   volume , rtpa(1,ivar) , smcelp , gamma  ,                      &
   smbr   ,  rovsdt , w1 )

!       Si on extrapole les TS on met Gamma Pinj dans PROPCE
  if(isto2t.gt.0) then
    do iel = 1, ncel
      propce(iel,iptsta+isou-1) =                                 &
      propce(iel,iptsta+isou-1) + w1(iel)
    enddo
!       Sinon on le met directement dans SMBR
  else
    do iel = 1, ncel
      smbr(iel) = smbr(iel) + w1(iel)
    enddo
  endif

endif

!===============================================================================
! 5. TERME D'ACCUMULATION DE MASSE -(dRO/dt)*VOLUME
!    ET TERME INSTATIONNAIRE
!===============================================================================

! ---> Ajout dans la diagonale de la matrice

do iel=1,ncel
  rovsdt(iel) = rovsdt(iel)                                       &
            + istat(ivar)*(propce(iel,ipcrom)/dt(iel))*volume(iel)
enddo


!===============================================================================
! 6. PRODUCTION, PHI1, PHI2, ET DISSIPATION
!===============================================================================

! ---> Terme source
!     -rho*epsilon*( Cs1*aij + Cs2*(aikajk -1/3*aijaij*deltaij))
!     -Cr1*P*aij + Cr2*rho*k*sij - Cr3*rho*k*sij*sqrt(aijaij)
!     +Cr4*rho*k(aik*sjk+ajk*sik-2/3*akl*skl*deltaij)
!     +Cr5*rho*k*(aik*rjk + ajk*rik)
!     -2/3*epsilon*deltaij

if(isou.eq.1)then
  iii = 1
  jjj = 1
elseif(isou.eq.2)then
  iii = 2
  jjj = 2
elseif(isou.eq.3)then
  iii = 3
  jjj = 3
elseif(isou.eq.4)then
  iii = 1
  jjj = 2
elseif(isou.eq.5)then
  iii = 1
  jjj = 3
elseif(isou.eq.6)then
  iii = 2
  jjj = 3
endif

! EBRSM
if (iturb.eq.32) then
  allocate(grad(ncelet,3))

  ! Compute the gradient of Alpha
  inc    = 1
  iccocg = 1
  nswrgp = nswrgr(ial)
  imligp = imligr(ial)
  iwarnp = iwarni(ial)
  epsrgp = epsrgr(ial)
  climgp = climgr(ial)
  extrap = extrag(ial)
  iphydp = 0

  call grdcel &
  !==========
 ( ial    , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,ial )    , coefa(1,iclal)  , coefb(1,iclal)  ,          &
   grad   )



endif

if (icorio.eq.1) then

  ! Compute the rotation matrix (dual antisymmetric matrix of the rotation vector)
  matrot(1,2) = -omegaz
  matrot(1,3) =  omegay
  matrot(2,3) = -omegax

  do ii = 1, 3
    matrot(ii,ii) = 0.d0
    do jj = ii+1, 3
      matrot(jj,ii) = -matrot(ii,jj)
    enddo
  enddo

else
  do ii = 1, 3
    do jj = 1, 3
      matrot(ii,jj) = 0.d0
    enddo
  enddo
endif

! Index of the Reynolds stress variables in rtpa array
indrey(1,1) = ir11
indrey(2,2) = ir22
indrey(3,3) = ir33
indrey(1,2) = ir12
indrey(1,3) = ir13
indrey(2,3) = ir23
indrey(2,1) = indrey(1,2)
indrey(3,1) = indrey(1,3)
indrey(3,2) = indrey(2,3)

do iel=1,ncel

  ! EBRSM
  if (iturb.eq.32) then
    ! Compute the magnitude of the Alpha gradient
    xnoral = ( grad(iel,1)*grad(iel,1)          &
           +   grad(iel,2)*grad(iel,2)          &
           +   grad(iel,3)*grad(iel,3) )
    xnoral = sqrt(xnoral)
   ! Compute the unitary vector of Alpha
    if (xnoral.le.epzero) then
      xnal(1) = 0.d0
      xnal(2) = 0.d0
      xnal(3) = 0.d0
    else
      xnal(1) = grad(iel,1)/xnoral
      xnal(2) = grad(iel,2)/xnoral
      xnal(3) = grad(iel,3)/xnoral
    endif
  endif

  ! Pij
  xprod(1,1) = -2.0d0*(rtpa(iel,ir11)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir12)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir13)*grdvit(iel,3,1) )
  xprod(1,2) = -(      rtpa(iel,ir11)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir12)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir13)*grdvit(iel,3,2) )         &
               -(      rtpa(iel,ir12)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir22)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir23)*grdvit(iel,3,1) )
  xprod(1,3) = -(      rtpa(iel,ir11)*grdvit(iel,1,3) +         &
                       rtpa(iel,ir12)*grdvit(iel,2,3) +         &
                       rtpa(iel,ir13)*grdvit(iel,3,3) )         &
               -(      rtpa(iel,ir13)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir23)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir33)*grdvit(iel,3,1) )
  xprod(2,2) = -2.0d0*(rtpa(iel,ir12)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir22)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir23)*grdvit(iel,3,2) )
  xprod(2,3) = -(      rtpa(iel,ir12)*grdvit(iel,1,3) +         &
                       rtpa(iel,ir22)*grdvit(iel,2,3) +         &
                       rtpa(iel,ir23)*grdvit(iel,3,3) )         &
               -(      rtpa(iel,ir13)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir23)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir33)*grdvit(iel,3,2) )
  xprod(3,3) = -2.0d0*(rtpa(iel,ir13)*grdvit(iel,1,3) +         &
                       rtpa(iel,ir23)*grdvit(iel,2,3) +         &
                       rtpa(iel,ir33)*grdvit(iel,3,3) )

  ! Rotating frame of reference => "Coriolis production" term
  if (icorio.eq.1) then
    do ii = 1, 3
      do jj = ii, 3
        do kk = 1, 3
          xprod(ii,jj) = xprod(ii,jj)                                   &
                       - 2.d0*( matrot(ii,kk)*rtpa(iel,indrey(jj,kk))   &
                              + matrot(jj,kk)*rtpa(iel,indrey(ii,kk)) )
        enddo
      enddo
    enddo
  endif

  xprod(2,1) = xprod(1,2)
  xprod(3,1) = xprod(1,3)
  xprod(3,2) = xprod(2,3)

  trprod = d1s2 * (xprod(1,1) + xprod(2,2) + xprod(3,3) )
  trrij  = d1s2 * (rtpa(iel,ir11) + rtpa(iel,ir22) + rtpa(iel,ir33))
!-----> aII = aijaij
  aii    = 0.d0
  aklskl = 0.d0
  aiksjk = 0.d0
  aikrjk = 0.d0
  aikakj = 0.d0
  ! aij
  xaniso(1,1) = rtpa(iel,ir11)/trrij - d2s3
  xaniso(2,2) = rtpa(iel,ir22)/trrij - d2s3
  xaniso(3,3) = rtpa(iel,ir33)/trrij - d2s3
  xaniso(1,2) = rtpa(iel,ir12)/trrij
  xaniso(1,3) = rtpa(iel,ir13)/trrij
  xaniso(2,3) = rtpa(iel,ir23)/trrij
  xaniso(2,1) = xaniso(1,2)
  xaniso(3,1) = xaniso(1,3)
  xaniso(3,2) = xaniso(2,3)
  ! Sij
  xstrai(1,1) = grdvit(iel,1,1)
  xstrai(1,2) = d1s2*(grdvit(iel,2,1)+grdvit(iel,1,2))
  xstrai(1,3) = d1s2*(grdvit(iel,3,1)+grdvit(iel,1,3))
  xstrai(2,1) = xstrai(1,2)
  xstrai(2,2) = grdvit(iel,2,2)
  xstrai(2,3) = d1s2*(grdvit(iel,3,2)+grdvit(iel,2,3))
  xstrai(3,1) = xstrai(1,3)
  xstrai(3,2) = xstrai(2,3)
  xstrai(3,3) = grdvit(iel,3,3)
  ! omegaij
  xrotac(1,1) = 0.d0
  xrotac(1,2) = d1s2*(grdvit(iel,2,1)-grdvit(iel,1,2))
  xrotac(1,3) = d1s2*(grdvit(iel,3,1)-grdvit(iel,1,3))
  xrotac(2,1) = -xrotac(1,2)
  xrotac(2,2) = 0.d0
  xrotac(2,3) = d1s2*(grdvit(iel,3,2)-grdvit(iel,2,3))
  xrotac(3,1) = -xrotac(1,3)
  xrotac(3,2) = -xrotac(2,3)
  xrotac(3,3) = 0.d0

  ! Rotating frame of reference => "absolute" vorticity
  if (icorio.eq.1) then
    do ii = 1, 3
      do jj = 1, 3
        xrotac(ii,jj) = xrotac(ii,jj) + matrot(ii,jj)
      enddo
    enddo
  endif

  do ii=1,3
    do jj = 1,3
      ! aii = aij.aij
      aii    = aii+xaniso(ii,jj)*xaniso(ii,jj)
      ! aklskl = aij.Sij
      aklskl = aklskl + xaniso(ii,jj)*xstrai(ii,jj)
    enddo
  enddo

  do kk = 1,3
    ! aiksjk = aik.Sjk+ajk.Sik
    aiksjk = aiksjk + xaniso(iii,kk)*xstrai(jjj,kk)              &
              +xaniso(jjj,kk)*xstrai(iii,kk)
    ! aikrjk = aik.Omega_jk + ajk.omega_ik
    aikrjk = aikrjk + xaniso(iii,kk)*xrotac(jjj,kk)              &
              +xaniso(jjj,kk)*xrotac(iii,kk)
    ! aikakj = aik*akj
    aikakj = aikakj + xaniso(iii,kk)*xaniso(kk,jjj)
  enddo

!     Si on extrapole les TS (rarissime), on met tout dans PROPCE.
!     On n'implicite pas le terme en Cs1*aij ni le terme en Cr1*P*aij.
!     Sinon, on met tout dans SMBR et on peut impliciter Cs1*aij
!     et Cr1*P*aij. Ici on stocke le second membre et le terme implicite
!     dans W1 et W2, pour eviter d'avoir un test IF(ISTO2T.GT.0)
!     dans la boucle NCEL
!     Dans le terme en W1, qui a vocation a etre extrapole, on utilise
!     naturellement IPCROO.
!     L'implicitation des deux termes pourrait se faire aussi en cas
!     d'extrapolation, en isolant ces deux termes et les mettant dans
!     SMBR et pas PROPCE et en utilisant IPCROM ... a modifier si le
!     besoin s'en fait vraiment sentir           !

  if (iturb.eq.31) then

    pij = xprod(iii,jjj)
    phiij1 = -rtpa(iel,iep)* &
       (cssgs1*xaniso(iii,jjj)+cssgs2*(aikakj-d1s3*deltij*aii))
    phiij2 = - cssgr1*trprod*xaniso(iii,jjj)                             &
           +   trrij*xstrai(iii,jjj)*(cssgr2-cssgr3*sqrt(aii))           &
           +   cssgr4*trrij*(aiksjk-d2s3*deltij*aklskl)                  &
           +   cssgr5*trrij* aikrjk
    epsij = -d2s3*rtpa(iel,iep)*deltij

    w1(iel) = propce(iel,ipcroo)*volume(iel)*(pij+phiij1+phiij2+epsij)

    w2(iel) = volume(iel)/trrij*propce(iel,ipcrom)*(                &
           cssgs1*rtpa(iel,iep) + cssgr1*max(trprod,0.d0) )

  ! EBRSM
  else

    xrij(1,1) = rtpa(iel,ir11)
    xrij(2,2) = rtpa(iel,ir22)
    xrij(3,3) = rtpa(iel,ir33)
    xrij(1,2) = rtpa(iel,ir12)
    xrij(1,3) = rtpa(iel,ir13)
    xrij(2,3) = rtpa(iel,ir23)
    xrij(2,1) = xrij(1,2)
    xrij(3,1) = xrij(1,3)
    xrij(3,2) = xrij(2,3)

    ! Compute the explicit term

    ! Calcul des termes de proches parois et quasi-homgene de phi et
    ! epsilon

    ! Calcul du terme de proche paroi \Phi_{ij}^w --> W3
    phiijw = 0.d0
    xnnd = d1s2*( xnal(iii)*xnal(jjj) + deltij )
    do kk = 1, 3
      phiijw = phiijw + xrij(iii,kk)*xnal(jjj)*xnal(kk)
      phiijw = phiijw + xrij(jjj,kk)*xnal(iii)*xnal(kk)
      do ll = 1, 3
        phiijw = phiijw - xrij(kk,ll)*xnal(kk)*xnal(ll)*xnnd
      enddo
    enddo
    phiijw = -5.d0*rtpa(iel,iep)/trrij * phiijw

    ! Calcul du terme quasi-homogene \Phi_{ij}^h --> W4
    phiij1 = -rtpa(iel,iep)*cebms1*xaniso(iii,jjj)
    phiij2 = -cebmr1*trprod*xaniso(iii,jjj)                       &
               +trrij*xstrai(iii,jjj)*(cebmr2-cebmr3*sqrt(aii))   &
               +cebmr4*trrij   *(aiksjk-d2s3*deltij*aklskl)       &
               +cebmr5*trrij   * aikrjk

    ! Calcul de \e_{ij}^w --> W5 (Rotta model)
    ! Rij/k*epsilon
    epsijw =  xrij(iii,jjj)/trrij   *rtpa(iel,iep)

    ! Calcul de \e_{ij}^h --> W6
    epsij =  d2s3*rtpa(iel,iep)*deltij

    ! Calcul du terme source explicite de l'equation des Rij
    !   [ P_{ij} + (1-\alpha^3)\Phi_{ij}^w + \alpha^3\Phi_{ij}^h
    !            - (1-\alpha^3)\e_{ij}^w   - \alpha^3\e_{ij}^h  ] --> W1
    alpha3 = rtp(iel,ial)**3

    w1(iel) = volume(iel)*propce(iel,ipcrom)*(                    &
               xprod(iii,jjj)                                     &
            + (1.d0-alpha3)*phiijw + alpha3*(phiij1+phiij2)       &
            - (1.d0-alpha3)*epsijw - alpha3*epsij)

    !  Implicite term

    ! le terme ci-dessous correspond a la partie implicitee du SSG
    ! dans le cadre de la ponderation elliptique, il est multiplie par
    ! \alpha^3
    w2(iel) = volume(iel)*propce(iel,ipcrom)*(                    &
              cebms1*rtpa(iel,iep)/trrij*alpha3                   &
             +cebmr1*max(trprod/trrij,0.d0)*alpha3                &
    ! Implicitation de epsijw
    ! (le facteur 5 apparait lorsqu'on fait Phi_{ij}^w - epsijw)
            + 5.d0 * (1.d0-alpha3)*rtpa(iel,iep)/trrij            &
            +        (1.d0-alpha3)*rtpa(iel,iep)/trrij)
  endif

enddo


if(isto2t.gt.0) then

  do iel = 1, ncel
    propce(iel,iptsta+isou-1) = propce(iel,iptsta+isou-1)         &
         + w1(iel)
  enddo

else

  do iel = 1, ncel
    smbr(iel) = smbr(iel) + w1(iel)
    rovsdt(iel) = rovsdt(iel) + w2(iel)
  enddo

endif


!===============================================================================
! 7. TERMES DE GRAVITE
!===============================================================================

if(igrari.eq.1) then

  ! Allocate a work array
  allocate(w7(ncelet))

  do iel = 1, ncel
    w7(iel) = 0.d0
  enddo

  call rijthe                                                     &
  !==========
 ( nvar   , nscal  ,                                              &
   ivar   , isou   , ipp    ,                                     &
   rtp    , rtpa   , propce , propfa , propfb ,                   &
   coefa  , coefb  , gradro , w7     )

  ! Si on extrapole les T.S. : PROPCE
  if(isto2t.gt.0) then
    do iel = 1, ncel
      propce(iel,iptsta+isou-1) = propce(iel,iptsta+isou-1) + w7(iel)
    enddo
  ! Sinon SMBR
  else
    do iel = 1, ncel
      smbr(iel) = smbr(iel) + w7(iel)
    enddo
  endif

  ! Free memory
  deallocate(w7)

endif

if (iturb.eq.31) then
!===============================================================================
! 8. TERMES DE DIFFUSION (STANDARD SSG)
!===============================================================================
! ---> Viscosite

  if( idiff(ivar).ge. 1 ) then
    do iel = 1, ncel
      trrij = 0.5d0 * (rtpa(iel,ir11) + rtpa(iel,ir22) + rtpa(iel,ir33))
      rctse = d2s3 * propce(iel,ipcrom) * csrij * trrij**2 / rtpa(iel,iep)
      w1(iel) = propce(iel,ipcvis) + idifft(ivar)*rctse
    enddo

    call viscfa                                                     &
    !==========
   ( imvisf ,                                                       &
     w1     ,                                                       &
     viscf  , viscb  )

    ! Translate coefa into cofaf and coefb into cofbf
    do ifac = 1, nfabor

      iel = ifabor(ifac)

      hint = w1(iel)/distb(ifac)

      ! Translate coefa into cofaf and coefb into cofbf
      coefa(ifac, iclvaf) = -hint*coefa(ifac,iclvar)
      coefb(ifac, iclvaf) = hint*(1.d0-coefb(ifac,iclvar))

    enddo


  else

    do ifac = 1, nfac
      viscf(ifac) = 0.d0
    enddo
    do ifac = 1, nfabor
      viscb(ifac) = 0.d0

      ! Translate coefa into cofaf and coefb into cofbf
      coefa(ifac, iclvaf) = 0.d0
      coefb(ifac, iclvaf) = 0.d0
    enddo

  endif

else
!===============================================================================
! 8. TERMES DE DIFFUSION  A.grad(Rij) : PARTIE EXTRADIAGONALE EXPLICITE
!    (Daly Harlow: generalized gradient hypothesis method for the EBRSM)
!===============================================================================
! ---> Calcul du grad(Rij)

  allocate(w4(ncelet), w5(ncelet), w6(ncelet))

  iccocg = 1
  inc = 1

  nswrgp = nswrgr(ivar)
  imligp = imligr(ivar)
  iwarnp = iwarni(ivar)
  epsrgp = epsrgr(ivar)
  climgp = climgr(ivar)
  extrap = extrag(ivar)
  iphydp = 0

  call grdcel                                                     &
  !==========
 ( ivar   , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtpa(1,ivar )   , coefa(1,iclvar) , coefb(1,iclvar) ,          &
   grad   )

    ! ---> Calcul des termes extradiagonaux de A.grad(Rij)

  do iel = 1, ncel
    trrij     = d1s2*( rtpa(iel,ir11)                           &
                      +rtpa(iel,ir22)                           &
                      +rtpa(iel,ir33) )
    ! Calcul de l echelle de temps de Durbin
    xttke  = trrij   /rtpa(iel,iep)
    xttkmg = xct*sqrt( propce(iel,ipcvis)/propce(iel,ipcrom)      &
                                         /rtpa(iel,iep) )
    xttdrb = max(xttke,xttkmg)
    rctse  = propce(iel,ipcroo) * csebm *xttdrb
    w4(iel) = rctse * ( rtpa(iel,ir12) * grad(iel,2)                &
                       +rtpa(iel,ir13) * grad(iel,3) )
    w5(iel) = rctse * ( rtpa(iel,ir12) * grad(iel,1)                &
                       +rtpa(iel,ir23) * grad(iel,3) )
    w6(iel) = rctse * ( rtpa(iel,ir13) * grad(iel,1)                &
                       +rtpa(iel,ir23) * grad(iel,2) )
  enddo

  ! ---> Assemblage de { A.grad(Rij) } .S aux faces

  call vectds &
  !==========
( w4     , w5     , w6     ,                                      &
  viscf  , viscb  )

  init = 1

  call divmas &
  !==========
( ncelet  , ncel   , nfac   , nfabor , init     , nfecra ,         &
  ifacel  , ifabor , viscf  , viscb  , w4)

  ! Si on extrapole les termes sources
  if(isto2t.gt.0) then
    do iel = 1, ncel
      propce(iel,iptsta+isou-1) =                                   &
      propce(iel,iptsta+isou-1) + w4(iel)
    enddo
  else
    do iel = 1, ncel
      smbr(iel) = smbr(iel) + w4(iel)
    enddo
  endif


!===============================================================================
! 9. TERMES DE DIFFUSION  A.grad(Rij) : PARTIE DIAGONALE
!===============================================================================
!     Implicitation de (grad(Rij).n)n en gradient facette
!     Si IDIFRE=1, terme correctif explicite
!        grad(Rij)-(grad(Rij).n)n calcule en gradient cellule
!     Les termes de bord sont uniquement pris en compte dans la partie
!        en (grad(Rij).n)n

!     Attention en periodicite on syngra-ise le gradient comme si c'etait
!       un vecteur (alors que dans grdcel on l'a fait comme si c'etait
!       un tenseur ...).
!     A modifier eventuellement. Pour le moment on conserve donc
!       SYNGRA.

  if (idifre.eq.1) then

    do iel = 1, ncel
      trrij  = d1s2*( rtpa(iel,ir11)                             &
                     +rtpa(iel,ir22)                             &
                     +rtpa(iel,ir33) )
      ! Calcul de l echelle de temps de Durbin
      xttke  = trrij/rtpa(iel,iep)
      xttkmg = xct*sqrt( propce(iel,ipcvis)/propce(iel,ipcrom)     &
                                           /rtpa(iel,iep) )
      xttdrb = max(xttke,xttkmg)
      rctse  = propce(iel,ipcroo) * csebm * xttdrb
      w4(iel) = rctse*rtpa(iel,ir11)
      w5(iel) = rctse*rtpa(iel,ir22)
      w6(iel) = rctse*rtpa(iel,ir33)
    enddo

    ! Periodicity and parallelism treatment

    if (irangp.ge.0.or.iperio.eq.1) then
      call syndia(w4, w5, w6)
    endif

    do ifac = 1, nfac

      ii = ifacel(1,ifac)
      jj = ifacel(2,ifac)

      surfn2 =surfan(ifac)**2

      grdpx = d1s2*(grad(ii,1)+grad(jj,1))
      grdpy = d1s2*(grad(ii,2)+grad(jj,2))
      grdpz = d1s2*(grad(ii,3)+grad(jj,3))
      grdsn = grdpx*surfac(1,ifac)+grdpy*surfac(2,ifac)             &
             +grdpz*surfac(3,ifac)
      grdpx = grdpx-grdsn*surfac(1,ifac)/surfn2
      grdpy = grdpy-grdsn*surfac(2,ifac)/surfn2
      grdpz = grdpz-grdsn*surfac(3,ifac)/surfn2

      viscf(ifac)= d1s2*(                                           &
            (w4(ii)+w4(jj))*grdpx*surfac(1,ifac)                    &
           +(w5(ii)+w5(jj))*grdpy*surfac(2,ifac)                    &
           +(w6(ii)+w6(jj))*grdpz*surfac(3,ifac))

    enddo

    do ifac = 1, nfabor
      viscb(ifac) = 0.d0
    enddo

    init = 1
    call divmas                                                     &
    !==========
  (ncelet  , ncel   , nfac  , nfabor , init , nfecra ,              &
   ifacel  , ifabor , viscf , viscb  , w1)

    ! Si on extrapole les termes sources
    if(isto2t.gt.0) then
      do iel = 1, ncel
        propce(iel,iptsta+isou-1) =                                 &
        propce(iel,iptsta+isou-1) + w1(iel)
      enddo
    else
      do iel = 1, ncel
        smbr(iel) = smbr(iel) + w1(iel)
      enddo
    endif

  endif

  ! Free memory
  deallocate(grad)
  deallocate(w4, w5, w6)


  ! ---> Viscosite orthotrope pour partie implicite

  if (idiff(ivar).ge.1) then

    allocate(w3(ncelet))

    do iel = 1, ncel
      trrij  = d1s2*( rtpa(iel,ir11)                             &
                     +rtpa(iel,ir22)                             &
                     +rtpa(iel,ir33) )
      ! Calcul de l echelle de temps de Durbin
      xttke  = trrij/rtpa(iel,iep)
      xttkmg = xct*sqrt( propce(iel,ipcvis)/propce(iel,ipcrom)   &
                                           /rtpa(iel,iep) )
      xttdrb = max(xttke,xttkmg)
      rctse  = propce(iel,ipcrom) * csebm * xttdrb
      w1(iel) = propce(iel,ipcvis)                               &
              + idifft(ivar)*rctse*rtpa(iel,ir11)
      w2(iel) = propce(iel,ipcvis)                               &
              + idifft(ivar)*rctse*rtpa(iel,ir22)
      w3(iel) = propce(iel,ipcvis)                               &
              + idifft(ivar)*rctse*rtpa(iel,ir33)
    enddo

    call visort &
    !==========
   ( imvisf ,                                                    &
     w1     , w2     , w3     ,                                  &
     viscf  , viscb  )

    ! Translate coefa into cofaf and coefb into cofbf
    do ifac = 1, nfabor

      iel = ifabor(ifac)

      hint = ( w1(iel)*surfbo(1,ifac)*surfbo(1,ifac)                            &
             + w2(iel)*surfbo(2,ifac)*surfbo(2,ifac)                            &
             + w3(iel)*surfbo(3,ifac)*surfbo(3,ifac))/surfbn(ifac)**2/distb(ifac)

      ! Translate coefa into cofaf and coefb into cofbf
      coefa(ifac, iclvaf) = -hint*coefa(ifac,iclvar)
      coefb(ifac, iclvaf) = hint*(1.d0-coefb(ifac,iclvar))

    enddo

    ! Free memory
    deallocate(w3)

  else

    do ifac = 1, nfac
      viscf(ifac) = 0.d0
    enddo
    do ifac = 1, nfabor
      viscb(ifac) = 0.d0

      ! Translate coefa into cofaf and coefb into cofbf
      coefa(ifac, iclvaf) = 0.d0
      coefb(ifac, iclvaf) = 0.d0
    enddo

  endif
endif

!===============================================================================
! 10. RESOLUTION
!===============================================================================

if(isto2t.gt.0) then
  thetp1 = 1.d0 + thets
  do iel = 1, ncel
    smbr(iel) = smbr(iel) + thetp1*propce(iel,iptsta+isou-1)
  enddo
endif

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
idftnp = idften(ivar)
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

call codits &
!==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
   imrgra , nswrsp , nswrgp , imligp , ircflp ,                   &
   ischcp , isstpp , iescap , imucpp , idftnp , iswdyp ,          &
   imgrp  , ncymxp , nitmfp , ipp    , iwarnp ,                   &
   blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
   relaxp , thetv  ,                                              &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
   coefa(1,iclvar) , coefb(1,iclvar) ,                            &
   coefa(1,iclvaf) , coefb(1,iclvaf) ,                            &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   viscf  , viscb  , rvoid  , viscf  , viscb  , rvoid  ,          &
   rvoid  , rvoid  ,                                              &
   rovsdt , smbr   , rtp(1,ivar)     , dpvar  ,                   &
   rvoid  , rvoid  )

!===============================================================================
! 11. IMPRESSIONS
!===============================================================================

! Free memory
deallocate(w1, w2)
deallocate(dpvar)

!--------
! FORMATS
!--------

#if defined(_CS_LANG_FR)

 1000 format(/,'           RESOLUTION POUR LA VARIABLE ',A8,/)

#else

 1000 format(/,'           SOLVING VARIABLE ',A8           ,/)

#endif

!12345678 : MAX: 12345678901234 MIN: 12345678901234 NORM: 12345678901234
!----
! FIN
!----

return

end subroutine
