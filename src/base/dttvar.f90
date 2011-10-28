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

subroutine dttvar &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   iwarnp ,                                                       &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , ckupdc , smacel )

!===============================================================================
! FONCTION :
! ----------

! CALCUL DU PAS DE TEMPS LOCAL
! AFFICHAGE DES NOMBRES DE COURANT + FOURIER MINIMUM, MAXIMUM
! On dispose des types de faces de bord au pas de temps
!   precedent (sauf au premier pas de temps, ou les tableaux
!   ITYPFB et ITRIFB n'ont pas ete renseignes)

! Sous programme utilise dans le cas une seule phase (ou
! si seule la phase 1 pilote le pas de temps)
!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncepdp           ! i  ! <-- ! number of cells with head loss                 !
! ncesmp           ! i  ! <-- ! number of cells with mass source term          !
! iwarnp           ! i  ! <-- ! verbosity                                      !
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
! (ncesmp,nvar)    !    !     !  source de masse                               !
!                  !    !     ! pour ivar=ipr, smacel=flux de masse            !
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
use cstphy
use optcal
use entsor
use parall
use ppppar
use ppthch
use ppincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ncepdp , ncesmp
integer          iwarnp

integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itypsm(ncesmp,nvar)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision ckupdc(ncepdp,6), smacel(ncesmp,nvar)


! Local variables

character*8      cnom

integer          ifac, iel, icfmax, icfmin, idiff0, iconv0, isym
integer          modntl
integer          ipcvis, ipcvst
integer          iflmas, iflmab
integer          icou, ifou , icoucf
integer          inc, iccocg
integer          nswrgp, imligp
integer          ipcrom, ipbrom, iivar
integer          nbrval
integer          ipccou, ipcfou

double precision epsrgp, climgp, extrap
double precision cfmax,cfmin, coufou, w1min, w2min, w3min
double precision unpvdt, rom
double precision xyzmax(3), xyzmin(3)
double precision dtsdtm,dtsdt0

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: dam
double precision, allocatable, dimension(:) :: wcf
double precision, allocatable, dimension(:) :: cofbdt, coefbr
double precision, allocatable, dimension(:,:) :: grad
double precision, allocatable, dimension(:) :: w1, w2, w3

!===============================================================================

!===============================================================================
! 0.  INITIALISATION
!===============================================================================

! Allocate temporary arrays for the time-step resolution
allocate(viscf(nfac), viscb(nfabor))
allocate(dam(ncelet))
allocate(cofbdt(nfabor))

! Allocate other arrays, depending on user options
if (ippmod(icompf).ge.0) then
  allocate(wcf(ncelet))
endif

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet), w3(ncelet))


iflmas  = ipprof(ifluma(iu))
iflmab  = ipprob(ifluma(iu))
ipcvis  = ipproc(iviscl)
ipcvst  = ipproc(ivisct)
ipcrom  = ipproc(irom  )
ipbrom  = ipprob(irom  )
ipccou  = ipproc(icour )
ipcfou  = ipproc(ifour )

if(ntlist.gt.0) then
  modntl = mod(ntcabs,ntlist)
elseif(ntlist.eq.-1.and.ntcabs.eq.ntmabs) then
  modntl = 0
else
  modntl = 1
endif

if (                                                              &
   .not. ( iconv(iu).ge.1.and.                                 &
           (iwarnp.ge.2.or.modntl.eq.0) ) .and.                   &
   .not. ( idiff(iu).ge.1.and.                                 &
           (iwarnp.ge.2.or.modntl.eq.0) ) .and.                   &
   .not. ( ippmod(icompf).ge.0.and.                               &
           (iwarnp.ge.2.or.modntl.eq.0) ) .and.                   &
   .not. ( idtvar.eq.1.or.idtvar.eq.2.or.                         &
           ( (iwarnp.ge.2.or.modntl.eq.0).and.                    &
             (idiff(iu).ge.1.or.iconv(iu).ge.1              &
                               .or.ippmod(icompf).ge.0)  ) )      &
   ) then

  return

endif

!===============================================================================
! 1.  CONDITION LIMITE POUR MATRDT
!===============================================================================


do ifac = 1, nfabor
  if(propfb(ifac,iflmab).lt.0.d0) then
    cofbdt(ifac) = 0.d0
  else
    cofbdt(ifac) = 1.d0
  endif
enddo

!===============================================================================
! 2.  CALCUL DE LA LIMITATION EN COMPRESSIBLE
!===============================================================================

!     On commence par cela afin de disposer de VISCF VISCB comme
!       tableaux de travail.

  if(ippmod(icompf).ge.0) then

    call cfdttv                                                   &
    !==========
 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   iwarnp ,                                                       &
   icepdc , icetsm , itypsm ,                                     &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  , ckupdc , smacel ,                            &
   wcf    ,                                                       &
!        ---
   viscf  , viscb  , cofbdt )

  endif


!===============================================================================
! 3.  CALCUL DE LA VISCOSITE FACETTES
!===============================================================================


!     On s'en sert dans les divers matrdt suivants

!     "VITESSE" DE DIFFUSION FACETTE

if( idiff(iu).ge. 1 ) then
  do iel = 1, ncel
    w1    (iel) = propce(iel,ipcvis)                              &
                                +idifft(iu)*propce(iel,ipcvst)
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
! 4.  ALGORITHME INSTATIONNAIRE
!===============================================================================

if (idtvar.ge.0) then

!===============================================================================
! 4.1  PAS DE TEMPS VARIABLE A PARTIR DE COURANT ET FOURIER IMPOSES
!===============================================================================

!     On calcule le pas de temps thermique max (meme en IDTVAR=0, pour affichage)
!     DTTMAX = 1/SQRT(MAX(0+,gradRO.g/RO) -> W3

  if (iptlro.eq.1) then

    ! Allocate a temporary array for the gradient calculation
    allocate(grad(ncelet,3))
    allocate(coefbr(nfabor))

    do ifac = 1, nfabor
      coefbr(ifac) = 0.d0
    enddo

    nswrgp = nswrgr(ipr)
    imligp = imligr(ipr)
    iwarnp = iwarni(ipr)
    epsrgp = epsrgr(ipr)
    climgp = climgr(ipr)
    extrap = 0.d0

    iivar = 0
    inc   = 1
    iccocg = 1

    call grdcel                                                   &
    !==========
 ( iivar  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   propce(1,ipcrom), propfb(1,ipbrom), coefbr ,                   &
   grad   )

    do iel = 1, ncel
      w3(iel) = (grad(iel,1)*gx + grad(iel,2)*gy + grad(iel,3)*gz)&
           /propce(iel,ipcrom)
      w3(iel) = 1.d0/sqrt(max(epzero,w3(iel)))

    enddo

    ! Free memory
    deallocate(grad)
    deallocate(coefbr)

!     On met le nombre de clippings a 0 (il le restera pour IDTVAR=0)
    nclptr = 0

  endif


  if (idtvar.eq.1.or.idtvar.eq.2) then

    icou = 0
    ifou = 0

! 4.1.1 LIMITATION PAR LE COURANT
! =============================

    if ( coumax.gt.0.d0.and.iconv(iu).ge.1 ) then

!     ICOU = 1 marque l'existence d'une limitation par le COURANT
      icou = 1

! ---> CONSTRUCTION DE U/DX           (COURANT         ) =W1

      idiff0 = 0

!     Matrice a priori non symetrique
      isym = 2

      call matrdt &
      !==========
 ( iconv(iu)    , idiff0          , isym   ,                      &
   cofbdt , propfa(1,iflmas), propfb(1,iflmab), viscf  , viscb  , &
   dam    )

      do iel = 1, ncel
        rom = propce(iel,ipcrom)
        w1    (iel) = dam(iel)/(rom*volume(iel))
      enddo

! ---> CALCUL DE W1     = PAS DE TEMPS VARIABLE VERIFIANT
!       LE NOMBRE DE COURANT         MAXIMUM PRESCRIT PAR L'UTILISATEUR

      do iel = 1, ncel
        w1    (iel) = coumax/max( w1    (iel), epzero)
      enddo

! ---> PAS DE TEMPS UNIFORME : ON PREND LE MINIMUM DE LA CONTRAINTE

      if (idtvar.eq.1) then
        w1min = grand
        do iel = 1, ncel
          w1min = min(w1min,w1(iel))
        enddo
        if (irangp.ge.0) then
          call parmin (w1min)
          !==========
        endif
        do iel = 1, ncel
          w1(iel) = w1min
        enddo
      endif

    endif

! 4.1.2 LIMITATION PAR LE FOURIER
! =============================

    if ( foumax.gt.0.d0.and.idiff(iu).ge.1 ) then

!     IFOU = 1 marque l'existence d'une limitation par le FOURIER
      ifou = 1

      iconv0 = 0
!                                   2
! ---> CONSTRUCTION DE      +2.NU/DX  (         FOURIER) =W2

!     Matrice a priori symetrique
      isym = 1

      call matrdt &
      !==========
 ( iconv0          , idiff(iu)    , isym   ,                      &
   cofbdt , propfa(1,iflmas), propfb(1,iflmab), viscf  , viscb  , &
   dam    )

      do iel = 1, ncel
        rom = propce(iel,ipcrom)
        w2    (iel) = dam(iel)/(rom*volume(iel))
      enddo

! ---> CALCUL DE W2     = PAS DE TEMPS VARIABLE VERIFIANT
!       LE NOMBRE DE         FOURIER MAXIMUM PRESCRIT PAR L'UTILISATEUR

      do iel = 1, ncel
        w2    (iel) = foumax/max( w2    (iel), epzero)
      enddo

! ---> PAS DE TEMPS UNIFORME : ON PREND LE MINIMUM DE LA CONTRAINTE

      if (idtvar.eq.1) then
        w2min = grand
        do iel = 1, ncel
          w2min = min(w2min,w2(iel))
        enddo
        if (irangp.ge.0) then
          call parmin (w2min)
          !==========
        endif
        do iel = 1, ncel
          w2(iel) = w2min
        enddo
      endif

    endif

! 4.1.3 LIMITATION POUR L'ALGORITHME COMPRESSIBLE
! =============================================
!     Il est important de conserver WCF intact : on le reutilise
!     plus bas pour l'affichage

    icoucf = 0
    if ( coumax.gt.0.d0.and.ippmod(icompf).ge.0 ) then

      icoucf = 1

! ---> CALCUL DE DAM     = PAS DE TEMPS VARIABLE VERIFIANT
!       LA CONTRAINTE CFL MAXIMUM PRESCRITE PAR L'UTILISATEUR

      do iel = 1, ncel
        dam(iel) = coumax/max( wcf(iel), epzero)
      enddo

! ---> PAS DE TEMPS UNIFORME : ON PREND LE MINIMUM DE LA CONTRAINTE

      if (idtvar.eq.1) then
        w3min = grand
        do iel = 1, ncel
          w3min = min(w3min,dam(iel))
        enddo
        if (irangp.ge.0) then
          call parmin (w3min)
          !==========
        endif
        do iel = 1, ncel
          dam(iel) = w3min
        enddo
      endif

    endif

! 4.1.4 ON PREND LA PLUS CONTRAIGNANTE DES LIMITATIONS
! ==================================================
!    (le minimum des deux si elles existent et
!     celle qui existe s'il n'en existe qu'une)

    if(icou.eq.1.and.ifou.eq.1) then
      do iel = 1, ncel
        w1(iel) = min(w1(iel),w2(iel))
      enddo
    elseif(icou.eq.0.and.ifou.eq.1) then
      do iel = 1, ncel
        w1(iel) = w2(iel)
      enddo
    endif


!     En compressible, on prend obligatoirement
!     en compte la limitation associee � la masse volumique.

    if(icoucf.eq.1) then
      do iel = 1, ncel
        w1(iel) = min(w1(iel),dam(iel))
      enddo
    endif

! 4.1.5 ON CALCULE EFFECTIVEMENT LE PAS DE TEMPS
! ============================================

! --->  MONTEE           PROGRESSIVE DU PAS DE TEMPS
!              DESCENTE  IMMEDIATE   DU PAS DE TEMPS

    do iel = 1, ncel
      if( w1    (iel).ge.dt(iel) ) then
        unpvdt = 1.d0+varrdt
        dt(iel) = min( unpvdt*dt(iel), w1    (iel) )
      else
        dt(iel) =                      w1    (iel)
      endif
    enddo


! 4.1.6 ON LIMITE PAR LE PAS DE TEMPS "THERMIQUE" MAX
! =================================================
!     DTTMAX = W3 = 1/SQRT(MAX(0+,gradRO.g/RO)
!     on limite le pas de temps a DTTMAX

    if (iptlro.eq.1) then


!  On clippe le pas de temps a DTTMAX
!     (affiche dans ecrlis)

      nclptr = 0

      do iel = 1, ncel
        if ( dt(iel).gt.w3(iel) ) then
          nclptr = nclptr +1
          dt(iel) = w3(iel)
        endif
      enddo

      if (irangp.ge.0) then
        call parcpt (nclptr)
        !==========
      endif

! ---> PAS DE TEMPS UNIFORME : on reuniformise le pas de temps

      if (idtvar.eq.1) then
        w3min = grand
        do iel = 1, ncel
          w3min = min(w3min,dt(iel))
        enddo
        if (irangp.ge.0) then
          call parmin (w3min)
          !==========
        endif
        do iel = 1, ncel
          dt(iel) = w3min
        enddo
      endif

    endif

! 4.1.7 ON CLIPPE LE PAS DE TEMPS PAR RAPPORT A DTMIN ET DTMAX
! ==========================================================

    icfmin = 0
    icfmax = 0

    do iel = 1, ncel

      if( dt(iel).gt.dtmax      ) then
        icfmax = icfmax +1
        dt(iel) = dtmax
      endif
      if( dt(iel).lt.dtmin      ) then
        icfmin = icfmin +1
        dt(iel) = dtmin
      endif

    enddo

    if (irangp.ge.0) then
      call parcpt (icfmin)
      !==========
      call parcpt (icfmax)
      !==========
    endif

    iclpmx(ippdt) = icfmax
    iclpmn(ippdt) = icfmin

    if( iwarnp.ge.2) then
      write (nfecra,1003) icfmin,dtmin,icfmax,dtmax
    endif

  endif

!     Rapport DT sur DTmax lie aux effets de densite
!       (affichage dans ecrlis)
  if (iptlro.eq.1) then

    dtsdtm = 0.d0
    do iel = 1, ncel
      dtsdt0 = dt(iel)/w3(iel)
      if ( dtsdt0 .gt. dtsdtm ) then
        dtsdtm = dtsdt0
        icfmax = iel
      endif
    enddo
    xyzmax(1) = xyzcen(1,icfmax)
    xyzmax(2) = xyzcen(2,icfmax)
    xyzmax(3) = xyzcen(3,icfmax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmxl (nbrval, dtsdtm, xyzmax)
      !==========
    endif
    rpdtro(1) = dtsdtm
    rpdtro(2) = xyzmax(1)
    rpdtro(3) = xyzmax(2)
    rpdtro(4) = xyzmax(3)

  endif

!===============================================================================
! 4.2  CALCUL DU NOMBRE DE COURANT POUR AFFICHAGE
!===============================================================================

  if ( iconv(iu).ge.1.and.                                     &
       (iwarnp.ge.2.or.modntl.eq.0) ) then

    idiff0 = 0
    CNOM   =' COURANT'

!     CONSTRUCTION DE U/DX           (COURANT         ) =W1

! MATRICE A PRIORI NON SYMETRIQUE

    isym = 2

    call matrdt &
    !==========
 ( iconv(iu)    , idiff0          , isym   ,                      &
   cofbdt , propfa(1,iflmas), propfb(1,iflmab), viscf  , viscb  , &
   dam    )

    do iel = 1, ncel
      rom = propce(iel,ipcrom)
      w1    (iel) = dam(iel)/(rom*volume(iel))
    enddo

!     CALCUL DU NOMBRE DE COURANT/FOURIER MAXIMUM ET MINIMUM

    cfmax = -grand
    cfmin =  grand
    icfmax= 1
    icfmin= 1

    do iel = 1, ncel

      coufou = w1(iel)*dt(iel)
      propce(iel,ipccou) = coufou

      if( coufou.le.cfmin ) then
        cfmin  = coufou
        icfmin = iel
      endif

      if( coufou.ge.cfmax ) then
        cfmax  = coufou
        icfmax = iel
      endif

    enddo

    xyzmin(1) = xyzcen(1,icfmin)
    xyzmin(2) = xyzcen(2,icfmin)
    xyzmin(3) = xyzcen(3,icfmin)
    xyzmax(1) = xyzcen(1,icfmax)
    xyzmax(2) = xyzcen(2,icfmax)
    xyzmax(3) = xyzcen(3,icfmax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmnl (nbrval, cfmin, xyzmin)
      !==========
      call parmxl (nbrval, cfmax, xyzmax)
      !==========
    endif

    if(iwarnp.ge.2) then
      write(nfecra,1001) cnom,cfmax,xyzmax(1),xyzmax(2),xyzmax(3)
      write(nfecra,1002) cnom,cfmin,xyzmin(1),xyzmin(2),xyzmin(3)
    endif

!       -> pour listing
    ptploc(1,1) = cfmin
    ptploc(1,2) = xyzmin(1)
    ptploc(1,3) = xyzmin(2)
    ptploc(1,4) = xyzmin(3)
    ptploc(2,1) = cfmax
    ptploc(2,2) = xyzmax(1)
    ptploc(2,3) = xyzmax(2)
    ptploc(2,4) = xyzmax(3)

  endif

!===============================================================================
! 4.3  CALCUL DU NOMBRE DE FOURIER POUR AFFICHAGE
!===============================================================================

  if ( idiff(iu).ge.1.and.                                     &
       (iwarnp.ge.2.or.modntl.eq.0) ) then

    iconv0 = 0
    CNOM   =' FOURIER'
!                                   2
!     CONSTRUCTION DE      +2.NU/DX  (         FOURIER) =W1

! MATRICE A PRIORI SYMETRIQUE

    isym = 1

    call matrdt &
    !==========
 ( iconv0          , idiff(iu)    , isym   ,                      &
   cofbdt , propfa(1,iflmas), propfb(1,iflmab), viscf  , viscb  , &
   dam    )

    do iel = 1, ncel
      rom = propce(iel,ipcrom)
      w1    (iel) = dam(iel)/(rom*volume(iel))
    enddo

!     CALCUL DU NOMBRE DE COURANT/FOURIER MAXIMUM ET MINIMUM

    cfmax  = -grand
    cfmin  =  grand
    icfmax = 0
    icfmin = 0

    do iel = 1, ncel

      coufou = w1(iel)*dt(iel)
      propce(iel,ipcfou) = coufou

      if( coufou.le.cfmin ) then
        cfmin  = coufou
        icfmin = iel
      endif

      if( coufou.ge.cfmax ) then
        cfmax  = coufou
        icfmax = iel
      endif

    enddo

    xyzmin(1) = xyzcen(1,icfmin)
    xyzmin(2) = xyzcen(2,icfmin)
    xyzmin(3) = xyzcen(3,icfmin)
    xyzmax(1) = xyzcen(1,icfmax)
    xyzmax(2) = xyzcen(2,icfmax)
    xyzmax(3) = xyzcen(3,icfmax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmnl (nbrval, cfmin, xyzmin)
      !==========
      call parmxl (nbrval, cfmax, xyzmax)
      !==========
    endif

    if(iwarnp.ge.2) then
      write(nfecra,1001) cnom,cfmax,xyzmax(1),xyzmax(2),xyzmax(3)
      write(nfecra,1002) cnom,cfmin,xyzmin(1),xyzmin(2),xyzmin(3)
    endif

!       -> pour listing
    ptploc(3,1) = cfmin
    ptploc(3,2) = xyzmin(1)
    ptploc(3,3) = xyzmin(2)
    ptploc(3,4) = xyzmin(3)
    ptploc(4,1) = cfmax
    ptploc(4,2) = xyzmax(1)
    ptploc(4,3) = xyzmax(2)
    ptploc(4,4) = xyzmax(3)

  endif

!===============================================================================
! 4.4  CALCUL DU NOMBRE DE COURANT/FOURIER POUR AFFICHAGE
!===============================================================================

!     En incompressible uniquement (en compressible, on preferera
!       afficher la contrainte liee a la masse volumique)

  if ( (iwarnp.ge.2.or.modntl.eq.0).and.                          &
      (idiff(iu).ge.1.or.iconv(iu).ge.1)                    &
      .and.(ippmod(icompf).lt.0)               ) then

    CNOM   =' COU/FOU'
!                                   2
!     CONSTRUCTION DE U/DX +2.NU/DX  (COURANT +FOURIER) =W1

! MATRICE A PRIORI NON SYMETRIQUE

    isym = 1
    if (iconv(iu).gt.0) isym = 2

    call matrdt &
    !==========
 ( iconv(iu)    , idiff(iu)    , isym   ,                         &
   cofbdt , propfa(1,iflmas), propfb(1,iflmab), viscf  , viscb  , &
   dam    )

    do iel = 1, ncel
      rom = propce(iel,ipcrom)
      w1    (iel) = dam(iel)/(rom*volume(iel))
    enddo

!     CALCUL DU NOMBRE DE COURANT/FOURIER MAXIMUM ET MINIMUM

    cfmax  = -grand
    cfmin  =  grand
    icfmax = 0
    icfmin = 0

    do iel = 1, ncel

      coufou = w1(iel)*dt(iel)

      if( coufou.le.cfmin ) then
        cfmin  = coufou
        icfmin = iel
      endif

      if( coufou.ge.cfmax ) then
        cfmax  = coufou
        icfmax = iel
      endif

    enddo

    xyzmin(1) = xyzcen(1,icfmin)
    xyzmin(2) = xyzcen(2,icfmin)
    xyzmin(3) = xyzcen(3,icfmin)
    xyzmax(1) = xyzcen(1,icfmax)
    xyzmax(2) = xyzcen(2,icfmax)
    xyzmax(3) = xyzcen(3,icfmax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmnl (nbrval, cfmin, xyzmin)
      !==========
      call parmxl (nbrval, cfmax, xyzmax)
      !==========
    endif

    if(iwarnp.ge.2) then
      write(nfecra,1001) cnom,cfmax,xyzmax(1),xyzmax(2),xyzmax(3)
      write(nfecra,1002) cnom,cfmin,xyzmin(1),xyzmin(2),xyzmin(3)
    endif

!       -> pour listing
    ptploc(5,1) = cfmin
    ptploc(5,2) = xyzmin(1)
    ptploc(5,3) = xyzmin(2)
    ptploc(5,4) = xyzmin(3)
    ptploc(6,1) = cfmax
    ptploc(6,2) = xyzmax(1)
    ptploc(6,3) = xyzmax(2)
    ptploc(6,4) = xyzmax(3)

  endif

!===============================================================================
! 4.5  CALCUL DE LA CONTRAINTE CFL DE LA MASSE VOL. POUR AFFICHAGE
!===============================================================================

! En Compressible uniquement

  if ( (iwarnp.ge.2.or.modntl.eq.0).and.                          &
       (ippmod(icompf).ge.0)                        ) then

    CNOM   =' CFL/MAS'


!     CALCUL DU NOMBRE DE COURANT/FOURIER MAXIMUM ET MINIMUM

    cfmax  = -grand
    cfmin  =  grand
    icfmax = 0
    icfmin = 0

    do iel = 1, ncel

      coufou = wcf(iel)*dt(iel)

      if( coufou.le.cfmin ) then
        cfmin  = coufou
        icfmin = iel
      endif

      if( coufou.ge.cfmax ) then
        cfmax  = coufou
        icfmax = iel
      endif

    enddo

    xyzmin(1) = xyzcen(1,icfmin)
    xyzmin(2) = xyzcen(2,icfmin)
    xyzmin(3) = xyzcen(3,icfmin)
    xyzmax(1) = xyzcen(1,icfmax)
    xyzmax(2) = xyzcen(2,icfmax)
    xyzmax(3) = xyzcen(3,icfmax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmnl (nbrval, cfmin, xyzmin)
      !==========
      call parmxl (nbrval, cfmax, xyzmax)
      !==========
    endif

    if(iwarnp.ge.2) then
      write(nfecra,1001) cnom,cfmax,xyzmax(1),xyzmax(2),xyzmax(3)
      write(nfecra,1002) cnom,cfmin,xyzmin(1),xyzmin(2),xyzmin(3)
    endif

!       -> pour listing
    ptploc(5,1) = cfmin
    ptploc(5,2) = xyzmin(1)
    ptploc(5,3) = xyzmin(2)
    ptploc(5,4) = xyzmin(3)
    ptploc(6,1) = cfmax
    ptploc(6,2) = xyzmax(1)
    ptploc(6,3) = xyzmax(2)
    ptploc(6,4) = xyzmax(3)

  endif

!===============================================================================
! 5.   ALGORITHME STATIONNAIRE
!===============================================================================
else

  isym = 1
  if (iconv(iu).gt.0) isym = 2

  call matrdt &
  !==========
 ( iconv(iu)    , idiff(iu)    , isym,                            &
   coefb(1,iu)  , propfa(1,iflmas), propfb(1,iflmab),             &
                                                viscf  , viscb  , &
   dt     )

  do iel = 1, ncel
    dt(iel) = relaxv(iu)*propce(iel,ipcrom)                    &
         *volume(iel)/max(dt(iel),epzero)
  enddo

endif

! Free memory
deallocate(viscf, viscb)
deallocate(dam)
deallocate(cofbdt)
if (allocated(wcf)) deallocate(wcf)
deallocate(w1, w2, w3)

!--------
! FORMATS
!--------

#if defined(_CS_LANG_FR)

 1001 FORMAT ( /,A8,' MAX= ',E11.4,                                     &
 ' EN ',E11.4,' ',E11.4,' ',E11.4)
 1002 FORMAT (   A8,' MIN= ',E11.4,                                     &
 ' EN ',E11.4,' ',E11.4,' ',E11.4)
 1003 FORMAT ( /,'CLIPPINGS DE DT : ',                                  &
                             I10,' A ',E11.4,', ',I10,' A ',E11.4)

#else

 1001 FORMAT ( /,A8,' MAX= ',E11.4,                                     &
 ' IN ',E11.4,' ',E11.4,' ',E11.4)
 1002 FORMAT (   A8,' MIN= ',E11.4,                                     &
 ' IN ',E11.4,' ',E11.4,' ',E11.4)
 1003 FORMAT ( /,'DT CLIPPING : ',                                      &
                             I10,' A ',E11.4,', ',I10,' A ',E11.4)

#endif

!----
! FIN
!----

return

end subroutine
