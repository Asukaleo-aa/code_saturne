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

subroutine visdyn &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , ckupdc , smacel ,                            &
   smagor )

!===============================================================================
! FONCTION :
! --------

! CALCUL DE LA VISCOSITE "TURBULENTE" POUR
! UN MODELE LES SMAGORINSKI DYNAMIQUE

! SMAGO = LijMij/MijMij

! PROPCE(1,IVISCT) = ROM * SMAGO  * L**2 * SQRT ( 2 * Sij.Sij )
!       Sij = (DUi/Dxj + DUj/Dxi)/2

! On dispose des types de faces de bord au pas de temps
!   precedent (sauf au premier pas de temps, ou les tableaux
!   ITYPFB et ITRIFB n'ont pas ete renseignes)

! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncepdp           ! i  ! <-- ! number of cells with head loss                 !
! ncesmp           ! i  ! <-- ! number of cells with mass source term          !
! icepdc(ncelet    ! te ! <-- ! numero des ncepdp cellules avec pdc            !
! icetsm(ncesmp    ! te ! <-- ! numero des cellules a source de masse          !
! itypsm           ! te ! <-- ! type de source de masse pour les               !
! (ncesmp,nvar)    !    !     !  variables (cf. ustsma)                        !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! ckupdc           ! tr ! <-- ! tableau de travail pour pdc                    !
!  (ncepdp,6)      !    !     !                                                !
! smacel           ! tr ! <-- ! valeur des variables associee a la             !
! (ncesmp,*   )    !    !     !  source de masse                               !
!                  !    !     !  pour ivar=ipr, smacel=flux de masse           !
! smagor(ncelet)   ! tr ! <-- ! constante de smagorinsky dans le cas           !
!                  !    !     ! d'un modlele dynamique                         !
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
use cstnum
use optcal
use cstphy
use entsor
use parall
use period
use pointe, only: coefau, coefbu
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
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision ckupdc(ncepdp,6), smacel(ncesmp,nvar)
double precision smagor(ncelet)

! Local variables

integer          ii, iel, iccocg, inc, isou, jsou
integer          ipcliu, ipcliv, ipcliw
integer          ipcrom, ipcvst
integer          iclipc

double precision coef, radeux, deux, delta, deltaf
double precision s11, s22, s33, s11f, s22f, s33f
double precision dudy, dudz, dvdx, dvdz, dwdx, dwdy
double precision dudyf, dudzf, dvdxf, dvdzf, dwdxf, dwdyf
double precision xfil, xa, xb, xfil2, xsmgmx
double precision aij, bij
double precision xl11, xl22, xl33, xl12, xl13, xl23
double precision xm11, xm22, xm33, xm12, xm13, xm23
double precision smagma, smagmn, smagmy

logical          ilved

double precision, allocatable, dimension(:) :: w1, w2, w3
double precision, allocatable, dimension(:) :: w4, w5, w6
double precision, allocatable, dimension(:) :: w7, w8, w9
double precision, allocatable, dimension(:) :: w10
double precision, allocatable, dimension(:,:) :: xmij
double precision, dimension(:,:,:), allocatable :: gradv, gradvf

!===============================================================================

!===============================================================================
! 1.  INITIALISATION
!===============================================================================

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet), w3(ncelet))
allocate(w4(ncelet), w5(ncelet), w6(ncelet))
allocate(w7(ncelet), w8(ncelet), w9(ncelet))
allocate(w10(ncelet))
allocate(xmij(ncelet,6))

! --- Memoire

! --- Rang des variables dans PROPCE (prop. physiques au centre)
ipcvst = ipproc(ivisct)
ipcrom = ipproc(irom  )

! --- Rang des c.l. des variables dans COEFA COEFB
!        (c.l. std, i.e. non flux)
ipcliu = iclrtp(iu,icoef)
ipcliv = iclrtp(iv,icoef)
ipcliw = iclrtp(iw,icoef)

! --- Pour le calcul de la viscosite de sous-maille
xfil   = xlesfl
xfil2  = xlesfd
xa     = ales
xb     = bles
deux   = 2.d0
radeux = sqrt(deux)
xsmgmx = smagmx

!===============================================================================
! 2.  CALCUL DES GRADIENTS DE VITESSE ET DE
!       S11**2+S22**2+S33**2+2*(S12**2+S13**2+S23**2)
!===============================================================================

!     Les RTPA ont ete echange pour les calculs en parallele,
!       au debut du pas de temps (donc pas utile de le refaire ici)

! Allocate temporary arrays for gradients calculation
allocate(gradv(ncelet,3,3), gradvf(ncelet,3,3))

iccocg = 1
inc = 1

! Compute the velocity gradient

if (ivelco.eq.1) then

  ilved = .false.

  call grdvec &
  !==========
( iu  , imrgra , inc    ,                               &
  nswrgr(iu) , imligr(iu) , iwarni(iu) ,                &
  nfecra , epsrgr(iu) , climgr(iu) , extrag(iu) ,       &
  ilved  ,                                              &
  rtpa(1,iu) ,  coefau , coefbu,                        &
  gradv  )

else

  call grdvni &
  !==========
( iu  , imrgra , inc    , iccocg ,                      &
  nswrgr(iu) , imligr(iu) , iwarni(iu) ,                &
  nfecra , epsrgr(iu) , climgr(iu) , extrag(iu) ,       &
  rtpa(1,iu) , coefa(1,ipcliu) , coefb(1,ipcliu) ,      &
  gradv  )

endif

! Filter the velocity gradient on the extended neighborhood

do isou = 1, 3
  do jsou = 1, 3
    call cfiltr &
    !==========
 ( gradv(1,isou,jsou), gradvf(1,isou,jsou), w1     , w2      )
  enddo
enddo

do iel = 1, ncel

  s11   = gradv(iel,1,1)
  s22   = gradv(iel,2,2)
  s33   = gradv(iel,3,3)
  dudy  = gradv(iel,2,1)
  dudz  = gradv(iel,3,1)
  dvdx  = gradv(iel,1,2)
  dvdz  = gradv(iel,3,2)
  dwdx  = gradv(iel,1,3)
  dwdy  = gradv(iel,2,3)

  s11f  = gradvf(iel,1,1)
  s22f  = gradvf(iel,2,2)
  s33f  = gradvf(iel,3,3)
  dudyf = gradvf(iel,2,1)
  dudzf = gradvf(iel,3,1)
  dvdxf = gradvf(iel,1,2)
  dvdzf = gradvf(iel,3,2)
  dwdxf = gradvf(iel,1,3)
  dwdyf = gradvf(iel,2,3)

  xmij(iel,1) = s11
  xmij(iel,2) = s22
  xmij(iel,3) = s33
  xmij(iel,4) = 0.5d0*(dudy+dvdx)
  xmij(iel,5) = 0.5d0*(dudz+dwdx)
  xmij(iel,6) = 0.5d0*(dvdz+dwdy)

  propce(iel,ipcvst) = radeux*sqrt(                               &
                       s11**2 + s22**2 + s33**2                   &
                     + 0.5d0*( (dudy+dvdx)**2                     &
                             + (dudz+dwdx)**2                     &
                             + (dvdz+dwdy)**2 )  )

  w9(iel) = radeux*sqrt(                                          &
                       s11f**2 + s22f**2 + s33f**2                &
                     + 0.5d0*( (dudyf+dvdxf)**2                   &
                             + (dudzf+dwdxf)**2                   &
                             + (dvdzf+dwdyf)**2 )  )
enddo

! Free memory
deallocate(gradv, gradvf)

!     Ici XMIJ contient Sij
!         PROPCE(IEL,IPCVST) contient ||S||
!            SQRT(2)*SQRT(S11^2+S22^2+S33^2+2(S12^2+S13^2+S23^2))
!         W9                 contient ||SF||
!            SQRT(2)*SQRT(S11F^2+S22F^2+S33F^2+2(S12F^2+S13F^2+S23F^2))

!===============================================================================
! 3.  CALCUL DE Mij
!===============================================================================

do iel = 1, ncel
  w7(iel) = xfil *(xa*volume(iel))**xb
enddo

do ii = 1, 6

  call cfiltr &
  !==========
 ( xmij(1,ii) , w1     , w2     , w3     )

  do iel = 1, ncel
    delta = w7(iel)
    w2(iel) = -deux*delta**2*propce(iel,ipcvst)*xmij(iel,ii)
  enddo

  call cfiltr &
  !==========
 ( w2     , w3     , w4     , w5     )

  do iel = 1, ncel
    delta = w7(iel)
    deltaf = xfil2*delta
    aij    = -deux*deltaf**2*w9(iel)*w1(iel)
    bij    =  w3(iel)
    xmij(iel,ii) = aij - bij
  enddo

enddo

!     Ici Aij contient alpha_ij, Bij contient beta_ij tilde
!        et XMIJ contient M_ij

!===============================================================================
! 4.  CALCUL DE LA CONSTANTE DE SMAGORINSKY DYNAMIQUE
!===============================================================================

! FILTRAGE DE LA VITESSE ET DE SON CARRE


! U**2
do iel = 1,ncel
  w9(iel) = rtp(iel,iu)*rtp(iel,iu)
enddo
call cfiltr  &
!==========
 ( w9     , w1     , w7     , w8     )

! V**2
do iel = 1,ncel
  w9(iel) = rtp(iel,iv)*rtp(iel,iv)
enddo
call cfiltr &
!==========
 ( w9     , w2     , w7     , w8     )

! W**2
do iel = 1,ncel
  w9(iel) = rtp(iel,iw)*rtp(iel,iw)
enddo
call cfiltr &
!==========
 ( w9     , w3     , w7     , w8     )

! UV
do iel = 1,ncel
  w9(iel) = rtp(iel,iu)*rtp(iel,iv)
enddo
call cfiltr &
!==========
 ( w9     , w4     , w7     , w8     )

! UW
do iel = 1,ncel
  w9(iel) = rtp(iel,iu)*rtp(iel,iw)
enddo
call cfiltr &
!==========
 ( w9     , w5     , w7     , w8     )

! VW
do iel = 1,ncel
  w9(iel) = rtp(iel,iv)*rtp(iel,iw)
enddo
call cfiltr &
!==========
 ( w9     , w6     , w7     , w8     )

! U
call cfiltr &
!==========
 ( rtp(1,iu)    , w7     , w8     , w9     )

! V
call cfiltr &
!==========
 ( rtp(1,iv)    , w8     , w9     , smagor )

! W
call cfiltr &
!==========
 ( rtp(1,iw)    , w9     , smagor , w10    )

do iel = 1, ncel

! --- Calcul de Lij
  xl11 = w1(iel) - w7(iel) * w7(iel)
  xl22 = w2(iel) - w8(iel) * w8(iel)
  xl33 = w3(iel) - w9(iel) * w9(iel)
  xl12 = w4(iel) - w7(iel) * w8(iel)
  xl13 = w5(iel) - w7(iel) * w9(iel)
  xl23 = w6(iel) - w8(iel) * w9(iel)

  xm11 = xmij(iel,1)
  xm22 = xmij(iel,2)
  xm33 = xmij(iel,3)
  xm12 = xmij(iel,4)
  xm13 = xmij(iel,5)
  xm23 = xmij(iel,6)
! ---Calcul de Mij :: Lij
  w1(iel) = xm11 * xl11 + 2.d0* xm12 * xl12 + 2.d0* xm13 * xl13 + &
                                xm22 * xl22 + 2.d0* xm23 * xl23 + &
                                                    xm33 * xl33
! ---Calcul de Mij :: Mij
  w2(iel) = xm11 * xm11 + 2.d0* xm12 * xm12 + 2.d0* xm13 * xm13 + &
                                xm22 * xm22 + 2.d0* xm23 * xm23 + &
                                                    xm33 * xm33

enddo

if (irangp.ge.0.or.iperio.eq.1) then
  call synsca(w1)
  !==========
  call synsca(w2)
  !==========
endif

!     Par defaut on fait une moyenne locale du numerateur et du
!     denominateur, puis seulement on fait le rapport.
!     L'utilisateur peut faire autrement dans USSMAG

call cfiltr                                                       &
!==========
 ( w1     , w3     , w5     , w6     )

call cfiltr                                                       &
!==========
 ( w2     , w4     , w5     , w6     )

do iel = 1, ncel
  if(abs(w4(iel)).le.epzero) then
    smagor(iel) = xsmgmx**2
  else
    smagor(iel) = w3(iel)/w4(iel)
  endif
enddo

call ussmag                                                       &
!==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   ckupdc , smacel ,                                              &
   smagor , w1     , w2     )

iclipc = 0
do iel = 1, ncel
  if(smagor(iel).ge.xsmgmx**2) then
    smagor(iel) = xsmgmx**2
    iclipc = iclipc + 1
  elseif(smagor(iel).le.-xsmgmx**2) then
    smagor(iel) = -xsmgmx**2
    iclipc = iclipc + 1
  endif
enddo

!===============================================================================
! 3.  CALCUL DE LA VISCOSITE (DYNAMIQUE)
!===============================================================================

! On clippe en (mu + mu_t)>0 dans phyvar

do iel = 1, ncel
  coef = smagor(iel)
  delta  = xfil * (xa*volume(iel))**xb
  propce(iel,ipcvst) = propce(iel,ipcrom)                         &
       * coef * delta**2 * propce(iel,ipcvst)
enddo

!     Quelques impressions
if(iwarni(iu).ge.1) then

  smagma = -1.0d12
  smagmn =  1.0d12
  smagmy =  0.d0
  do iel = 1, ncel
    smagma = max(smagma,smagor(iel))
    smagmn = min(smagmn,smagor(iel))
    smagmy = smagmy + smagor(iel)*volume(iel)
  enddo
  if(irangp.ge.0) then
    call parmax(smagma)
    !==========
    call parmin(smagmn)
    !==========
    call parsom(smagmy)
    !==========
    call parcpt(iclipc)
    !==========
  endif
  smagmy = smagmy / voltot
  write(nfecra,1000) iclipc
  write(nfecra,2001)
  write(nfecra,2002) smagma, smagmn, smagmy
  write(nfecra,2003)

endif

! Free memory
deallocate(w1, w2, w3)
deallocate(w4, w5, w6)
deallocate(w7, w8, w9)
deallocate(w10)
deallocate(xmij)

!----
! FORMAT
!----

#if defined(_CS_LANG_FR)

 1000 format(                                                           &
' Nb Clipping Constante Smagorinsky par valeurs maximales ',I10,/)
 2001 format(                                                           &
' --- Informations sur la constante de Smagorinsky^2          ',/,&
' ----------------------------------                          ',/,&
' Valeur moy  Valeur min  Valeur max                          ',/,&
' ----------------------------------                          '  )
 2002 format(                                                           &
 e12.4    ,      e12.4,      e12.4                               )
 2003 format(                                                           &
' ----------------------------------                          ',/)

#else

 1000 format(                                                           &
' Nb of clipping of the Smagorinsky constant by max values',I10,/)
 2001 format(                                                           &
' --- Informations on the squared Smagorinsky constant'        ,/,&
' --------------------------------'                            ,/,&
' Mean value  Min value  Max value'                            ,/,&
' --------------------------------'                              )
 2002 format(                                                           &
 e12.4    ,      e12.4,      e12.4                               )
 2003 format(                                                           &
' --------------------------------'                            ,/)

#endif

!----
! FIN
!----

return
end subroutine
