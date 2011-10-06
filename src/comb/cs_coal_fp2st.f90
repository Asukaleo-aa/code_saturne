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

subroutine cs_coal_fp2st &
!=======================

 ( nvar   , nscal  , ncepdp , ncesmp ,                             &
   iscal  ,                                                        &
   itypfb ,                                                        &
   icepdc , icetsm , itypsm ,                                      &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,           &
   smbrs  , rovsdt )

!===============================================================================
! FONCTION :
! ----------

! ROUTINE PHYSIQUE PARTICULIERE : FLAMME CHARBON PULVERISE
!   TERMES SOURCES DE PRODUCTION ET DE DISSIPATION POUR
!   LA VARIANCE (BILANS EXPLICITE ET IMPLICITE)

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncepdp           ! i  ! <-- ! number of cells with head loss                 !
! ncesmp           ! i  ! <-- ! number of cells with mass source term          !
! itypfb(nfabor)   ! ia ! <-- ! boundary face types                            !
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
! smbrs(ncelet)    ! tr ! --> ! second membre explicite                        !
! rovsdt(ncelet    ! tr ! --> ! partie diagonale implicite                     !
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
use entsor
use optcal
use cstphy
use cstnum
use parall
use period
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use ppcpfu
use cs_coal_incl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp
integer          iscal

integer          itypfb(nfabor)
integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itypsm(ncesmp,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision smbrs(ncelet), rovsdt(ncelet)

! Local variables

integer           iel    , ifac   , ivar   ,ivar0 , ivarsc
integer           icla   , icha   , numcha
integer           inc    , iccocg , nswrgp , imligp , iwarnp
integer           iphydp , itenso , idimte
integer           ipcrom , ipcvst , ipcx2c
integer           ixchcl , ixckcl , ixnpcl , ipcgd1 , ipcgd2
integer           iold

double precision xk     , xe     , rhovst
double precision epsrgp , climgp , extrap
double precision aux
double precision gdev1 , gdev2 , ghet, gsec , ghetco2, gheth2O
double precision fsd   , fdev  , diamdv , gdev

integer           iok1,iok2
double precision , dimension ( : )     , allocatable :: x1,f1f2
double precision , dimension ( : )     , allocatable :: coefa , coefb
double precision, allocatable, dimension(:,:) :: grad

!===============================================================================
! 1. Initialization
!===============================================================================

!===============================================================================
! Deallocation dynamic arrays
!----
allocate(x1(1:ncelet) , f1f2(1:ncelet),                STAT=iok1)
allocate(grad(ncelet,3), STAT=iok1)
if ( iok1 > 0 ) then
  write(nfecra,*) ' Memory allocation error inside: '
  write(nfecra,*) '     cs_coal_fp2st               '
  call csexit(1)
endif
!===============================================================================

! --- La variance n'est pas associé a un scalaire mais a f1+f2
ivarsc = 0
ivar   = isca(iscal)

! --- Numero des grandeurs physiques
ipcrom = ipproc(irom)
ipcvst = ipproc(ivisct)


!===============================================================================
! 2. PRISE EN COMPTE DES TERMES SOURCES DE PRODUCTION PAR LES GRADIENTS
!    ET DE DISSIPATION
!===============================================================================
if ( itytur.eq.2 .or. iturb.eq.50 .or.             &
     itytur.eq.3 .or. iturb.eq.60      ) then
  inc = 1
  iccocg = 1
! A defaut de savoir pour F1M+F2M on prend comme pour F1M(1)
  nswrgp = nswrgr(isca(if1m(1)))
  imligp = imligr(isca(if1m(1)))
  iwarnp = iwarni(isca(if1m(1)))
  epsrgp = epsrgr(isca(if1m(1)))
  climgp = climgr(isca(if1m(1)))
  extrap = extrag(isca(if1m(1)))

! --> calcul de X1

  x1( : ) = 1.d0
  do icla = 1, nclacp
    ixchcl = isca(ixch(icla))
    ixckcl = isca(ixck(icla))
    ixnpcl = isca(inp(icla ))
    do iel = 1, ncel
      x1(iel) =   x1(iel)                                        &
               -( rtp(iel,ixchcl)                                &
                 +rtp(iel,ixckcl)                                &
                 +rtp(iel,ixnpcl)*xmash(icla) )
      if ( ippmod(iccoal) .ge. 1 ) then
        x1(iel) = x1(iel) - rtp(iel,isca(ixwt(icla)))
      endif
    enddo
  enddo

! --> calcul de F=F1+F2
  f1f2( : ) = zero
  do icha = 1, ncharb
    do iel = 1, ncel
      f1f2(iel) =  f1f2(iel)                                     &
                 + rtp(iel,isca(if1m(icha)))                     &
                 + rtp(iel,isca(if2m(icha)))
    enddo
  enddo
  do iel = 1, ncel
    f1f2(iel) = f1f2(iel)/x1(iel)
  enddo

! --> Calcul du gradient de f1f2
!
  allocate(coefa(1:nfabor),coefb(1:nfabor),STAT=iok1)
  if ( iok1 > 0 ) THEN
    write(nfecra,*) ' Memory allocation error inside : '
    write(nfecra,*) '     cs_coal_fp2st                '
    call csexit(1)
  endif
  do ifac = 1, nfabor
    coefa(ifac) = zero
    coefb(ifac) = 1.d0
    if ( itypfb(ifac).eq.ientre ) then
      coefa(ifac) = zero
      coefb(ifac)=  1.d0
    endif
  enddo

! En periodique et parallele, echange avant calcul du gradient

!    Parallele
  if(irangp.ge.0) then
    call parcom(f1f2)
    !==========
  endif

!    Periodique
  if(iperio.eq.1) then
    idimte = 0
    itenso = 0
    call percom                                           &
    !==========
  ( idimte , itenso ,                                     &
    f1f2   , f1f2   , f1f2 ,                              &
    f1f2   , f1f2   , f1f2 ,                              &
    f1f2   , f1f2   , f1f2    )
  endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
  ivar0  = 0
  iphydp = 0
  call grdcel                                                     &
  !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   f1f2   , coefa  , coefb  ,                                     &
   grad   )

  do iel = 1, ncel
    if ( itytur.eq.2 .or. iturb.eq.50 ) then
      xk = rtpa(iel,ik)
      xe = rtpa(iel,iep)
    elseif ( itytur.eq.3 ) then
      xk = 0.5d0*(rtpa(iel,ir11)+rtpa(iel,ir22)+rtpa(iel,ir33))
      xe = rtpa(iel,iep)
    elseif ( iturb.eq.60 ) then
      xk = rtpa(iel,ik)
      xe = cmu*xk*rtpa(iel,iomg)
    endif

    rhovst = propce(iel,ipproc(irom1))*xe/(xk*rvarfl(iscal))*volume(iel)
    rovsdt(iel) = rovsdt(iel) + max(zero,rhovst)
    smbrs(iel) = smbrs(iel)                                          &
                + 2.d0*propce(iel,ipcvst)*volume(iel)/sigmas(iscal)  &
                 *( grad(iel,1)**2.d0 + grad(iel,2)**2.d0                  &
                  + grad(iel,3)**2.d0 )*x1(iel) - rhovst*rtpa(iel,ivar)
!
! Correction : "valeur fatale" pour la variance
!
   smbrs(iel) = smbrs(iel)                                           &
               + rhovst*( (1.d0-1.d0/x1(iel))*rtpa(iel,ivar)         &
                         +(1.d0-x1(iel))*(f1f2(iel)**2.d0) )
!
  enddo

endif

!===============================================================================
! 3. PRISE EN COMPTE DES TERMES SOURCES RELATIF AUX ECHANGES INTERFACIAUX
!==============================================================================
!
! 2 versions disponible
!   iold = 1 ===> c'est l'ancienne version
!   iold = 2 ===> c'est la nouvelle
!
iold = 1
!
if ( iold .eq. 1 ) then
!
  do icla=1,nclacp
    numcha = ichcor(icla)
    ipcx2c = ipproc(ix2(icla))
    ixchcl = isca(ixch(icla))
    ixckcl = isca(ixck(icla))
    ixnpcl = isca(inp(icla ))
    ipcgd1 = ipproc(igmdv1(icla))
    ipcgd2 = ipproc(igmdv2(icla))
    do iel = 1, ncel
      gdev1 = -propce(iel,ipcrom)*propce(iel,ipcgd1)               &
                                 *rtp(iel,ixchcl)
      gdev2 = -propce(iel,ipcrom)*propce(iel,ipcgd2)               &
                                 *rtp(iel,ixchcl)
      gdev  = gdev1 + gdev2
!
      if ( rtp(iel,ixnpcl) .gt. epsicp ) then
        diamdv = diam20(icla)
        fsd  =  1.d0 - (1.d0-f1f2(iel))                            &
               * exp( ( rtp(iel,ixchcl)                            &
                       *(propce(iel,ipcgd1)+propce(iel,ipcgd2))  ) &
                     /( 2.d0*pi*2.77d-4*diamdv                     &
                        *rtp(iel,ixnpcl)*propce(iel,ipcrom) ) )
        fdev = 1.d0
!
! ts explicite
!
        if ( (fsd-f1f2(iel))*(2.d0*fdev-fsd-f1f2(iel)) .gt. epsicp ) then
          smbrs(iel) = smbrs(iel)                                          &
                     + volume(iel)*(gdev1+gdev2)                           &
                      *(fsd-f1f2(iel))*(2.d0*fdev-fsd-f1f2(iel))
        endif
      endif
!
    enddo
  enddo
!
else
!
  do icla=1,nclacp
    numcha = ichcor(icla)
    ipcx2c = ipproc(ix2(icla))
    ixchcl = isca(ixch(icla))
    ixckcl = isca(ixck(icla))
    do iel = 1, ncel
      gdev1 = -propce(iel,ipcrom)*propce(iel,ipproc(igmdv1(icla)))       &
                                 *rtp(iel,ixchcl)
      gdev2 = -propce(iel,ipcrom)*propce(iel,ipproc(igmdv2(icla)))       &
                                 *rtp(iel,ixchcl)
!
      aux  = (gdev1+gdev2)               *(1.d0-f1f2(iel))**2.d0
!
! ts implicite : pour l'instant on implicite de facon simple
!
      if ( abs(f1f2(iel)*(1.d0-f1f2(iel))) .GT. epsicp ) then
        rhovst = aux*rtpa(iel,ivar)/((f1f2(iel)*(1-f1f2(iel)))**2.d0)      &
                    *volume(iel)
      else
        rhovst = 0.d0
      endif
      rovsdt(iel) = rovsdt(iel) + max(zero,rhovst)
! ts explicite
      smbrs(iel) = smbrs(iel)+aux*volume(iel)-rhovst*rtpa(iel,ivar)

    enddo
  enddo
!
endif
!

!--------
! Formats
!--------

!===============================================================================
! Deallocation dynamic arrays
!----
deallocate(x1,f1f2,grad,STAT=iok1)
deallocate(coefa,coefb,STAT=iok2)
!----
if ( iok1 > 0 .or. iok2 > 0) then
  write(nfecra,*) ' Memory deallocation error inside: '
  write(nfecra,*) '     cs_coal_fp2st                 '
  call csexit(1)
endif
!===============================================================================

!----
! End
!----

return
end subroutine
