!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2009 EDF S.A., France

!     contact: saturne-support@edf.fr

!     The Code_Saturne Kernel is free software; you can redistribute it
!     and/or modify it under the terms of the GNU General Public License
!     as published by the Free Software Foundation; either version 2 of
!     the License, or (at your option) any later version.

!     The Code_Saturne Kernel is distributed in the hope that it will be
!     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
!     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!     GNU General Public License for more details.

!     You should have received a copy of the GNU General Public License
!     along with the Code_Saturne Kernel; if not, write to the
!     Free Software Foundation, Inc.,
!     51 Franklin St, Fifth Floor,
!     Boston, MA  02110-1301  USA

!-------------------------------------------------------------------------------

subroutine cptsvc &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   iscal  , iscala ,                                              &
   itypfb ,                                                       &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
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
! iscal            ! i  ! <-- ! scalar number                                  !
! iscala           ! e  ! <-- ! numero du scalaire associe                     !
! itypfb           ! ia ! <-- ! boundary face types                            !
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
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp
integer          iscal  , iscala

integer          itypfb(nfabor)
integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itypsm(ncesmp,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision smbrs(ncelet), rovsdt(ncelet)

! Local variables

integer          ivar   , ivarsc , ivarut , ivar0
integer          iel    , ifac
integer          ipcrom , ipcvst
integer          ixchcl , ixckcl , ixnpcl , icla   , icha
integer          inc    , iccocg , nswrgp , imligp , iwarnp

double precision x2     , xk     , xe     , rhovst
double precision epsrgp , climgp , extrap

double precision, allocatable, dimension(:) :: coefap, coefbp
double precision, allocatable, dimension(:,:) :: grad
double precision, allocatable, dimension(:) :: w1, w2
double precision, allocatable, dimension(:) :: w7, w8

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================

! Initialize variables to avoid compiler warnings

xe = 0.d0
xk = 0.d0

! Memoire


! --- Numero du scalaire a traiter : ISCAL

! --- Numero de la variable associee au scalaire a traiter ISCAL
ivar   = isca(iscal)

! --- Numero du scalaire eventuel associe dans le cas fluctuation
!         ISCALA et numero de variable de calcul
if (iscala.gt.0) then
  ivarsc = isca(iscala)
else
  ivarsc = 0
endif

! --- Numero des grandeurs physiques
ipcrom = ipproc(irom)
ipcvst = ipproc(ivisct)


!===============================================================================
! 2. PRISE EN COMPTE DES TERMES SOURCES DE PRODUCTION PAR LES GRADIENTS
!    ET DE DISSIPATION
!===============================================================================

if ( itytur.eq.2 .or. itytur.eq.3                   &
     .or. iturb.eq.50 .or. iturb.eq.60) then

  inc = 1
  iccocg = 1
  if (ivarsc.gt.0) then
    ivarut = ivarsc
  else
! A defaut de savoir pour F4M on prend comme pour F3M
    ivarut = isca(if3m)
  endif
  nswrgp = nswrgr(ivarut)
  imligp = imligr(ivarut)
  iwarnp = iwarni(ivarut)
  epsrgp = epsrgr(ivarut)
  climgp = climgr(ivarut)
  extrap = extrag(ivarut)

  ! Allocate work arrays
  allocate(w7(ncelet), w8(ncelet))

! --> Calcul de FIM et X2 dans W7 et W8

  do iel = 1, ncel
    w7(iel) = zero
    w8(iel) = 1.d0
  enddo

! ---- W8 = X1

  do icla = 1, nclacp
    ixchcl = isca(ixch(icla))
    ixckcl = isca(ixck(icla))
    ixnpcl = isca(inp(icla ))
    do iel = 1, ncel
      x2 = rtp(iel,ixchcl)+rtp(iel,ixckcl)                        &
         + rtp(iel,ixnpcl)*xmash(icla)
      w8(iel) = w8(iel) - x2
    enddo
  enddo

! ---- W7 = FJM (kg/kg du melange gazeux)

  if (ivarsc.eq.0) then
    ! Allocate work arrays
    allocate(w1(ncelet), w2(ncelet))
    do iel = 1, ncel
      w1(iel) = zero
      w2(iel) = zero
    enddo
    do icha = 1, ncharb
      do iel = 1, ncel
        w1(iel) =  w1(iel) + rtp(iel,isca(if1m(icha)))
        w2(iel) =  w2(iel) + rtp(iel,isca(if2m(icha)))
      enddo
    enddo
    do iel = 1, ncel
      w7(iel) = 1.d0 -                                            &
              ( (w1(iel) + w2(iel) + rtp(iel,isca(if3m)))         &
                / w8(iel) )
    enddo
    ! Free some work arrays
    deallocate(w1, w2)
  else
    do iel = 1, ncel
      w7(iel) = rtp(iel,ivarsc) / w8(iel)
    enddo
  endif

  ! Allocate temporary arrays
  allocate(coefap(nfabor), coefbp(nfabor))

  do ifac = 1, nfabor
    coefap(ifac) = zero
    coefbp(ifac) = 1.d0
    if ( itypfb(ifac).eq.ientre ) then
      coefap(ifac) = zero
      coefbp(ifac) = zero
      if (ivarsc.eq.0) coefap(ifac) = 1.d0
    endif
  enddo

  ! En periodique et parallele, echange avant calcul du gradient
  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(w7)
    !==========
  endif

  ! Allocate a temporary array for gradient computation
  allocate(grad(ncelet,3))

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
  ivar0  = 0
  call grdcel                                                     &
  !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   w7     , coefap , coefbp ,                                     &
!          FIM      COEFA        COEFB
   grad   )

  ! Free memory
  deallocate(coefap, coefbp)

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

    rhovst = propce(iel,ipcrom)*xe/                               &
             (xk * rvarfl(iscal))*volume(iel)
    rovsdt(iel) = rovsdt(iel) + max(zero,rhovst)
    smbrs(iel) = smbrs(iel) +                                                 &
                2.d0*propce(iel,ipcvst)*volume(iel)/sigmas(iscal)             &
               * (grad(iel,1)**2 + grad(iel,2)**2 + grad(iel,3)**2) * w8(iel) &
                - rhovst*rtpa(iel,ivar)
  enddo

  ! Free memory
  deallocate(grad)
  deallocate(w7, w8)

endif

!--------
! FORMATS
!--------



!----
! FIN
!----

return

end subroutine
