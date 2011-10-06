!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2011 EDF S.A.
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
   viscf  , viscb  , coefax ,                                     &
   tslage , tslagi ,                                              &
   smbr   , rovsdt )

!===============================================================================
! FONCTION :
! ----------

! RESOLUTION DES EQUATIONS CONVECTION DIFFUSION TERME SOURCE
!   POUR Rij modele SSG
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
! coefax(nfabor    ! tr ! --- ! tab de trav pour cond.lim. paroi               !
!                  ! tr ! --- !   attention : uniquement avec echo             !
!                  ! tr ! --- !   de paroi et abs(icdpar) = 1                  !
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
double precision viscf(nfac), viscb(nfabor), coefax(nfabor)
double precision tslage(ncelet),tslagi(ncelet)
double precision smbr(ncelet), rovsdt(ncelet)

! Local variables

integer          init  , ifac  , iel
integer          ii    , jj    , kk    , iiun  , iii   , jjj
integer          ipcrom, ipcvis, iflmas, iflmab, ipcroo
integer          iclvar, iclvaf
integer          nswrgp, imligp, iwarnp
integer          iconvp, idiffp, ndircp, ireslp
integer          nitmap, nswrsp, ircflp, ischcp, isstpp, iescap
integer          imgrp , ncymxp, nitmfp
integer          iptsta

double precision blencp, epsilp, epsrgp, climgp, extrap, relaxp
double precision epsrsp
double precision trprod, trrij , rctse , deltij
double precision tuexpr, thets , thetv , thetp1
double precision aiksjk, aikrjk, aii ,aklskl, aikakj
double precision xaniso(3,3), xstrai(3,3), xrotac(3,3), xprod(3,3)

double precision rvoid(1)

double precision, allocatable, dimension(:) :: w1, w2
double precision, allocatable, dimension(:) :: w7

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet))

! Initialize variables to avoid compiler warnings

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

iclvar = iclrtp(ivar,icoef)
iclvaf = iclrtp(ivar,icoeff)

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
   coefa  , coefb  , ckupdc , smcelp , gamma  , grdvit , grdvit , &
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
! 2. TERMES SOURCES  LAGRANGIEN : COUPLAGE RETOUR
!===============================================================================

!     Ordre 2 non pris en compte
 if (iilagr.eq.2 .and. ltsdyn.eq.1) then
   do iel = 1,ncel
     smbr(iel)   = smbr(iel)   + tslage(iel)
     rovsdt(iel) = rovsdt(iel) + max(-tslagi(iel),zero)
   enddo
 endif

!===============================================================================
! 3. TERME SOURCE DE MASSE
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
! 4. TERME D'ACCUMULATION DE MASSE -(dRO/dt)*VOLUME
!    ET TERME INSTATIONNAIRE
!===============================================================================

! ---> Calcul de mij

init = 1
call divmas(ncelet,ncel,nfac,nfabor,init,nfecra,                  &
               ifacel,ifabor,propfa(1,iflmas),propfb(1,iflmab),w1)

! ---> Ajout au second membre

do iel = 1, ncel
  smbr(iel) = smbr(iel)                                           &
              + iconv(ivar)*w1(iel)*rtpa(iel,ivar)
enddo

! ---> Ajout dans la diagonale de la matrice
!     Extrapolation ou non, meme forme par coherence avec bilsc2

do iel=1,ncel
  rovsdt(iel) = rovsdt(iel)                                       &
            + istat(ivar)*(propce(iel,ipcrom)/dt(iel))*volume(iel)&
            - iconv(ivar)*w1(iel)*thetv
enddo


!===============================================================================
! 5. PRODUCTION, PHI1, PHI2, ET DISSIPATION
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

do iel=1,ncel

  xprod(1,1) = -2.0d0*(rtpa(iel,ir11)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir12)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir13)*grdvit(iel,1,3) )
  xprod(1,2) = -(      rtpa(iel,ir11)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir12)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir13)*grdvit(iel,2,3) )         &
               -(      rtpa(iel,ir12)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir22)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir23)*grdvit(iel,1,3) )
  xprod(1,3) = -(      rtpa(iel,ir11)*grdvit(iel,3,1) +         &
                       rtpa(iel,ir12)*grdvit(iel,3,2) +         &
                       rtpa(iel,ir13)*grdvit(iel,3,3) )         &
               -(      rtpa(iel,ir13)*grdvit(iel,1,1) +         &
                       rtpa(iel,ir23)*grdvit(iel,1,2) +         &
                       rtpa(iel,ir33)*grdvit(iel,1,3) )
  xprod(2,2) = -2.0d0*(rtpa(iel,ir12)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir22)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir23)*grdvit(iel,2,3) )
  xprod(2,3) = -(      rtpa(iel,ir12)*grdvit(iel,3,1) +         &
                       rtpa(iel,ir22)*grdvit(iel,3,2) +         &
                       rtpa(iel,ir23)*grdvit(iel,3,3) )         &
               -(      rtpa(iel,ir13)*grdvit(iel,2,1) +         &
                       rtpa(iel,ir23)*grdvit(iel,2,2) +         &
                       rtpa(iel,ir33)*grdvit(iel,2,3) )
  xprod(3,3) = -2.0d0*(rtpa(iel,ir13)*grdvit(iel,3,1) +         &
                       rtpa(iel,ir23)*grdvit(iel,3,2) +         &
                       rtpa(iel,ir33)*grdvit(iel,3,3) )
  xprod(2,1) = xprod(1,2)
  xprod(3,1) = xprod(1,3)
  xprod(3,2) = xprod(2,3)

  trprod = 0.5d0 * (xprod(1,1) + xprod(2,2) + xprod(3,3) )
  trrij = 0.5d0 * (rtpa(iel,ir11) + rtpa(iel,ir22) + rtpa(iel,ir33))
!-----> aII = aijaij
  aii    = 0.d0
  aklskl = 0.d0
  aiksjk = 0.d0
  aikrjk = 0.d0
  aikakj = 0.d0
  xaniso(1,1) = rtpa(iel,ir11)/trrij-2.d0/3.d0
  xaniso(2,2) = rtpa(iel,ir22)/trrij-2.d0/3.d0
  xaniso(3,3) = rtpa(iel,ir33)/trrij-2.d0/3.d0
  xaniso(1,2) = rtpa(iel,ir12)/trrij
  xaniso(1,3) = rtpa(iel,ir13)/trrij
  xaniso(2,3) = rtpa(iel,ir23)/trrij
  xaniso(2,1) = xaniso(1,2)
  xaniso(3,1) = xaniso(1,3)
  xaniso(3,2) = xaniso(2,3)

  xstrai(1,1) = grdvit(iel,1,1)
  xstrai(1,2) = 0.5d0*(grdvit(iel,1,2)+grdvit(iel,2,1))
  xstrai(1,3) = 0.5d0*(grdvit(iel,1,3)+grdvit(iel,3,1))
  xstrai(2,1) = xstrai(1,2)
  xstrai(2,2) = grdvit(iel,2,2)
  xstrai(2,3) = 0.5d0*(grdvit(iel,2,3)+grdvit(iel,3,2))
  xstrai(3,1) = xstrai(1,3)
  xstrai(3,2) = xstrai(2,3)
  xstrai(3,3) = grdvit(iel,3,3)

  xrotac(1,1) = 0.d0
  xrotac(1,2) = 0.5d0*(grdvit(iel,1,2)-grdvit(iel,2,1))
  xrotac(1,3) = 0.5d0*(grdvit(iel,1,3)-grdvit(iel,3,1))
  xrotac(2,1) = -xrotac(1,2)
  xrotac(2,2) = 0.d0
  xrotac(2,3) = 0.5d0*(grdvit(iel,2,3)-grdvit(iel,3,2))
  xrotac(3,1) = -xrotac(1,3)
  xrotac(3,2) = -xrotac(2,3)
  xrotac(3,3) = 0.d0

  do ii=1,3
   do jj = 1,3
     aii    = aii+xaniso(ii,jj)*xaniso(ii,jj)
     aklskl = aklskl + xaniso(ii,jj)*xstrai(ii,jj)
   enddo
  enddo

  do kk = 1,3
     aiksjk = aiksjk + xaniso(iii,kk)*xstrai(jjj,kk)              &
              +xaniso(jjj,kk)*xstrai(iii,kk)
     aikrjk = aikrjk + xaniso(iii,kk)*xrotac(jjj,kk)              &
              +xaniso(jjj,kk)*xrotac(iii,kk)
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
  w1(iel) = propce(iel,ipcroo) * volume(iel) *(                   &
       xprod(iii,jjj) -rtpa(iel,iep)                            &
       *(cssgs1*xaniso(iii,jjj)+cssgs2*(                          &
       aikakj -1.d0/3.d0*deltij*aii))                             &
       -cssgr1*trprod*xaniso(iii,jjj)                             &
       +trrij*xstrai(iii,jjj)*(cssgr2-cssgr3*sqrt(aii))           &
       +cssgr4*trrij*(aiksjk-2.d0/3.d0*deltij*aklskl)             &
       +cssgr5*trrij*(aikrjk)-2.d0/3.d0*rtpa(iel,iep)*deltij)

  w2(iel) = volume(iel)/trrij*propce(iel,ipcrom)*(                &
         cssgs1*rtpa(iel,iep) + cssgr1*max(trprod,0.d0) )

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

  !     Si on extrapole les T.S. : PROPCE
  if(isto2t.gt.0) then
    do iel = 1, ncel
      propce(iel,iptsta+isou-1) = propce(iel,iptsta+isou-1) + w7(iel)
    enddo
  !     Sinon SMBR
  else
    do iel = 1, ncel
      smbr(iel) = smbr(iel) + w7(iel)
    enddo
  endif

  ! Free memory
  deallocate(w7)

endif


!===============================================================================
! 9. TERMES DE DIFFUSION
!===============================================================================
! ---> Viscosite

if( idiff(ivar).ge. 1 ) then
  do iel = 1, ncel
    trrij = 0.5d0 * (rtpa(iel,ir11) + rtpa(iel,ir22) + rtpa(iel,ir33))
    rctse = 2.d0/3.d0 * propce(iel,ipcrom) * csrij * trrij**2 / rtpa(iel,iep)
    w1(iel) = propce(iel,ipcvis) + idifft(ivar)*rctse
  enddo

  call viscfa                                                     &
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
imgrp  = imgr  (ivar)
ncymxp = ncymax(ivar)
nitmfp = nitmgf(ivar)
!MO      IPP    =
iwarnp = iwarni(ivar)
blencp = blencv(ivar)
epsilp = epsilo(ivar)
epsrsp = epsrsm(ivar)
epsrgp = epsrgr(ivar)
climgp = climgr(ivar)
extrap = extrag(ivar)
relaxp = relaxv(ivar)

call codits                                                       &
!==========
 ( nvar   , nscal  ,                                              &
   idtvar , ivar   , iconvp , idiffp , ireslp , ndircp , nitmap , &
   imrgra , nswrsp , nswrgp , imligp , ircflp ,                   &
   ischcp , isstpp , iescap ,                                     &
   imgrp  , ncymxp , nitmfp , ipp    , iwarnp ,                   &
   blencp , epsilp , epsrsp , epsrgp , climgp , extrap ,          &
   relaxp , thetv  ,                                              &
   rtpa(1,ivar)    , rtpa(1,ivar)    ,                            &
                     coefa(1,iclvar) , coefb(1,iclvar) ,          &
                     coefa(1,iclvaf) , coefb(1,iclvaf) ,          &
                     propfa(1,iflmas), propfb(1,iflmab),          &
   viscf  , viscb  , viscf  , viscb  ,                            &
   rovsdt , smbr   , rtp(1,ivar)     ,                            &
   rvoid  )


!===============================================================================
! 11. IMPRESSIONS
!===============================================================================

! Free memory
deallocate(w1, w2)

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
