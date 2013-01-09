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

subroutine resalp &
!================

 ( nvar   , nscal  ,                                              &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  )

!===============================================================================
! Function :
! ----------

! Solving the equation on Alpha in the framwork of the Rij-EBRSM model.
! written from the equation of F_BARRE)

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
!    nom           !type!mode !                   role                         !
!__________________!____!_____!________________________________________________!
! nvar             ! e  ! <-- ! nombre total de variables                      !
! nscal            ! e  ! <-- ! nombre total de scalaires                      !
! dt(ncelet)       ! tr ! <-- ! pas de temps                                   !
! rtp, rtpa        ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules (instant courant ou prec)          !
! propce           ! tr ! <-- ! proprietes physiques au centre des             !
! (ncelet,*)       !    !     !    cellules                                    !
! propfa           ! tr ! <-- ! proprietes physiques au centre des             !
!  (nfac,*)        !    !     !    faces internes                              !
! propfb           ! tr ! <-- ! proprietes physiques au centre des             !
!  (nfabor,*)      !    !     !    faces de bord                               !
! coefa, coefb     ! tr ! <-- ! conditions aux limites aux                     !
!  (nfabor,*)      !    !     !    faces de bord                               !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail
!-------------------------------------------------------------------------------
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use dimens, only: ndimfb
use paramx
use numvar
use entsor
use optcal
use cstnum
use cstphy
use pointe
use period
use parall
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)

! Local variables

integer          ivar  , iel
integer          iclvar, iclvaf
integer          ipcrom, ipcroo, ipcvis, ipcvlo, ipcvst, ipcvso
integer          iflmas, iflmab
integer          nswrgp, imligp, iwarnp, iphydp, ipp
integer          iconvp, idiffp, ndircp, ireslp
integer          nitmap, nswrsp, ircflp, ischcp, isstpp, iescap
integer          imgrp , ncymxp, nitmfp
integer          imucpp, idftnp, iswdyp
double precision blencp, epsilp, epsrsp, epsrgp, climgp, extrap, relaxp
double precision thetv , thetap
double precision d1s4, d3s2, d1s2
double precision xk, xnu, xrom, l2
double precision xllke, xllkmg, xlldrb

double precision rvoid(1)

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: smbr, rovsdt
double precision, allocatable, dimension(:) :: w1
double precision, allocatable, dimension(:) :: dpvar

!===============================================================================

!===============================================================================
! 1. Initialisation
!===============================================================================

allocate(smbr(ncelet), rovsdt(ncelet), w1(ncelet))
allocate(viscf(nfac), viscb(nfabor))
allocate(dpvar(ncelet))

ipcrom = ipproc(irom)
ipcvis = ipproc(iviscl)
ipcvst = ipproc(ivisct)
iflmas = ipprof(ifluma(iu))
iflmab = ipprob(ifluma(iu))

d1s2 = 1.d0/2.d0
d1s4 = 1.d0/4.d0
d3s2 = 3.d0/2.d0

!  test sur alpha qui ne doit pas etre superieur a 1
if (iwarni(ial).ge.1) then
  write(nfecra,1000)
endif

!===============================================================================
! 2. Resolution de l'equation de ALPHA
!===============================================================================

ivar = ial
iclvar = iclrtp(ial,icoef)
iclvaf = iclrtp(ial,icoeff)
ipp    = ipprtp(ivar)

if(iwarni(ivar).ge.1) then
  write(nfecra,1100) nomvar(ipp)
endif

thetv  = thetav(ivar)

ipcroo = ipcrom
ipcvlo = ipcvis
if(isto2t.gt.0) then
  if (iroext.gt.0) then
    ipcroo = ipproc(iroma)
  endif
  if(iviext.gt.0) then
    ipcvlo = ipproc(ivisla)
  endif
endif

do iel = 1, ncel
  smbr(iel) = 0.d0
enddo
do iel = 1, ncel
  rovsdt(iel) = 0.d0
enddo

!===============================================================================
! 2.2 Terme source de ALPHA
!     SMBR=1/L^2*(alpha) - 1/L^2
!  En fait on met un signe "-" car l'eq resolue est
!    -div(grad ) alpha = SMBR
!===============================================================================

! ---> Matrice

if (isto2t.gt.0) then
  thetap = thetv
else
  thetap = 1.d0
endif

!FIXME the source term extrapolation is not well done!!!!
do iel=1,ncel

  xk = d1s2*(rtpa(iel,ir11)+rtpa(iel,ir22)+rtpa(iel,ir33))
  xnu  = propce(iel,ipcvis)/propce(iel,ipcrom)

  ! Echelle de longueur integrale
  xllke = xk**d3s2/rtpa(iel,iep)

  ! Echelle de longueur de Kolmogorov
  xllkmg = xceta*(xnu**3/rtpa(iel,iep))**d1s4

  ! Echelle de longueur de Durbin
  xlldrb = xcl*max(xllke,xllkmg)

  l2      = xlldrb**2

! Terme explicite
  smbr(iel) = volume(iel)*(1.d0 -rtpa(iel,ial)) / l2

! Terme implicite
  rovsdt(iel) = (rovsdt(iel) + volume(iel)*thetap) / l2

enddo

! Calcul de viscf et viscb pour codits

do iel = 1, ncel
  w1(iel) = 1.d0
enddo

call viscfa                                                       &
!==========
 ( imvisf ,                                                       &
   w1     ,                                                       &
   viscf  , viscb  )

!===============================================================================
! 2.3 Resolution effective de l'equation de ALPHA
!===============================================================================

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
! 3. Clipping
!===============================================================================
   call clpalp                                                    &
   !==========
 ( ncelet , ncel   , nvar   ,                                     &
   propce , rtpa   , rtp )

! Free memory
deallocate(smbr, rovsdt, w1)
deallocate(viscf, viscb)
deallocate(dpvar)

!--------
! FORMATS
!--------

 1000    format(/,                                                &
'   ** RESOLUTION DU ALPHA                                    ',/,&
'      -----------------------------------------------        ',/)
 1100    format(/,'           RESOLUTION POUR LA VARIABLE ',A8,/)

!----
! FIN
!----

return

end
