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

subroutine distpr &
!================

 ( nvar   , nscal  ,                                              &
   itypfb ,                                                       &
   distpa )

!===============================================================================
! Purpose:
! --------

! Compute distance to wall by solving a 3D diffusion equation.

! Solve
!    div[grad(T)] = -1
!      avec :
!      T(bord)   = 0 en paroi
!      grad(T).n = 0 ailleurs

! The wall distance is then equal to:

!   d ~ -|grad(T)| + [grad(T).grad(T)+2.T]^(1/2)

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! itypfb           ! ia ! <-- ! boundary face types                            !
! distpa(ncelet    ! ra ! --> ! distance to wall                               !
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
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
use ppppar
use parall
use period
use mltgrd
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          itypfb(nfabor)

double precision distpa(ncelet)

! Local variables

integer          ndircp, iconvp, idiffp, isym
integer          ipol  , ireslp, ipp
integer          niterf, icycle, ncymxp, nitmfp
integer          iinvpe
integer          isqrt , iel   , ifac
integer          inc   , iccocg, ivar
integer          isweep, nittot, idtva0
integer          ibsize, iesize, mmprpl, nswrsl
integer          imucpp, idftnp
double precision relaxp, thetap, rnorm, residu, rnoini
double precision dismax, dismin, hint, pimp, qimp

double precision rvoid(1)

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: coefad, coefbd
double precision, allocatable, dimension(:) :: cofafd, cofbfd
double precision, allocatable, dimension(:) :: dam
double precision, allocatable, dimension(:,:) :: xam
double precision, allocatable, dimension(:) :: rtpdp, smbdp, rovsdp
double precision, allocatable, dimension(:,:) :: grad
double precision, allocatable, dimension(:) :: w1, w2, w3
double precision, allocatable, dimension(:) :: w7, w8, w9

!===============================================================================


!===============================================================================
! 1. Initialization
!===============================================================================

! Allocate temporary arrays for the species resolution
allocate(viscf(nfac), viscb(nfabor))
allocate(coefad(nfabor), coefbd(nfabor))
allocate(cofafd(nfabor), cofbfd(nfabor))
allocate(dam(ncelet), xam(nfac,2))
allocate(rtpdp(ncelet), smbdp(ncelet), rovsdp(ncelet))

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet), w3(ncelet))
allocate(w7(ncelet), w8(ncelet), w9(ncelet))

! Initialize variables to avoid compiler warnings

rnoini = 0.d0

nittot = 0

!===============================================================================
! 2. Boundary conditions
!===============================================================================

!     Conditions aux limites pour le scalaire resolu T
!       Dirichlet a 0 en paroi
!       Neumann nul ailleurs
!     On test aussi la pressence d'un Dirichlet

ndircp = 0

do ifac = 1, nfabor
  if(itypfb(ifac).eq.iparoi .or.                            &
     itypfb(ifac).eq.iparug) then

    ! Dirichlet Boundary Condition
    !-----------------------------

    hint = 1.d0/distb(ifac)
    pimp = 0.d0

    call set_dirichlet_scalar &
         !====================
       ( coefad(ifac), cofafd(ifac),             &
         coefbd(ifac), cofbfd(ifac),             &
         pimp        , hint        , rinfin )


    ndircp = 1
  else

    ! Neumann Boundary Conditions
    !----------------------------

    hint = 1.d0/distb(ifac)
    qimp = 0.d0

    call set_neumann_scalar &
         !==================
       ( coefad(ifac), cofafd(ifac),             &
         coefbd(ifac), cofbfd(ifac),             &
         qimp        , hint )

  endif
enddo

!===============================================================================
! 3. Prepare system to solve
!===============================================================================

! -- Diagonal

do iel = 1, ncel
  rovsdp(iel) = 0.d0
enddo

! -- Diffusion at faces

do iel = 1, ncel
  w1(iel) = 1.d0
enddo

call viscfa                                                       &
!==========
 ( imvisf ,                                                       &
   w1     ,                                                       &
   viscf  , viscb  )

iconvp = 0
idiffp = 1
isym   = 1
thetap = 1.d0
imucpp = 0

call matrix &
!==========
 ( ncelet , ncel   , nfac   , nfabor ,                            &
   iconvp , idiffp , ndircp ,                                     &
   isym   , nfecra ,                                              &
   thetap , imucpp ,                                              &
   ifacel , ifabor ,                                              &
   coefbd , cofbfd , rovsdp ,                                     &
   viscf  , viscb  , viscf  , viscb  ,                            &
   rvoid  , dam    , xam    )

!===============================================================================
! 4. Solve system
!===============================================================================

ipp = 1
nomvar(ipp) = 'DisParoi'
ipol   = 0
ireslp = 0
! No multigrid (NCYMXP,NITMFP arbitrary)
ncymxp = 100
nitmfp = 10
! Periodicity
iinvpe = 0
if(iperio.eq.1) iinvpe = 1
isqrt = 1
ibsize = 1
iesize = 1
nswrsl = nswrsy
110 continue

! Distance to wall is initialized to 0 for reconstruction

do iel = 1, ncelet
  distpa(iel) = 0.d0
  rtpdp(iel)  = 0.d0
enddo

! -- RHS

do iel = 1, ncel
  smbdp(iel)  = volume(iel)
enddo

! -- Reconstruction loop;
!   if NSWRSY = 1, we must solve twice

do isweep = 0, nswrsl

  call prodsc(ncel,isqrt,smbdp,smbdp,rnorm)
  if (iwarny.ge.2) then
     write(nfecra,5000) nomvar(ipp),isweep,rnorm
  endif
  if (isweep.le.1) rnoini = rnorm
  ! Convergence test
  if (rnorm.le.10.d0*epsily*rnoini) goto 100

  do iel = 1, ncelet
    rtpdp(iel) = 0.d0
  enddo

  call invers &
  !==========
 ( nomvar(ipp)     , isym   , ibsize , iesize ,                   &
   ipol   , ireslp , nitmay , imgrpy ,                            &
   ncymxp , nitmfp ,                                              &
   iwarny , nfecra , niterf , icycle , iinvpe ,                   &
   epsily , rnorm  , residu ,                                     &
   dam    , xam    , smbdp  , rtpdp  )

  nittot = nittot + niterf
  do iel = 1, ncel
    distpa(iel) = distpa(iel) + rtpdp(iel)
  enddo

  ! - Synchronization for parallelism

  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(rtpdp)
    !==========
  endif

  if (isweep.lt.nswrsl) then
    inc    = 0
    iccocg = 1
    imucpp = 0
    idftnp = 1 ! no tensorial diffusivity
    ivar = 0
    idtva0 = 0
    relaxp = 1.d0

    call bilsca &
    !==========
 ( nvar   , nscal  ,                                              &
   idtva0 , ivar   , iconvp , idiffp , nswrgy , imligy , ircfly , &
   ischcy , isstpy , inc    , imrgra , iccocg ,                   &
   ipp    , iwarny , imucpp , idftnp ,                            &
   blency , epsrgy , climgy , extray , relaxp , thetap ,          &
   rtpdp  , rtpdp  , coefad , coefbd , coefad , cofbfd ,          &
   viscf  , viscb  , viscf  , viscb  , rvoid  , rvoid  ,          &
   rvoid  , rvoid  ,                                              &
   smbdp  )

  endif
enddo

mmprpl = 0
do iel = 1, ncel
  if (distpa(iel).lt.0.d0) then
    mmprpl = 1
    exit
  endif
enddo

if (irangp.ge.0) call parcmx(mmprpl)

if (mmprpl.eq.1) then
  if (nswrsl.gt.0) then
    nswrsl = 0
    write(nfecra,9000)
    goto 110
  else
    write(nfecra,9001) distpa(iel)
  endif
endif

 100  continue

do iel=1,ncel
  rtpdp(iel)  = distpa(iel)
enddo

!===============================================================================
! 5. Compute distance to wall
!===============================================================================

! Allocate a temporary array for the gradient calculation
allocate(grad(ncelet,3))

! - Synchronization for parallelism and periodicity

if (irangp.ge.0.or.iperio.eq.1) then
  call synsca(rtpdp)
  !==========
endif

! - Compute gradient

inc    = 1
iccocg = 1
ivar   = 0

call grdcel                                                       &
!==========
 ( ivar   , imrgra , inc    , iccocg , nswrgy , imligy ,          &
   iwarny , nfecra , epsrgy , climgy , extray ,                   &
   rtpdp  , coefad , coefbd ,                                     &
   grad   )

do iel = 1, ncel
  w1(iel) = grad(iel,1)**2.d0+grad(iel,2)**2.d0+grad(iel,3)**2.d0
  if(w1(iel)+2.d0*rtpdp(iel).gt.0.d0) then
    distpa(iel) = - sqrt(w1(iel)) + sqrt(w1(iel)+2.d0*rtpdp(iel))
  else
    write(nfecra,8000)iel, xyzcen(1,iel),xyzcen(2,iel),xyzcen(3,iel)
  endif
enddo

! Free memory
deallocate(grad)

!===============================================================================
! 6. Compute bounds and print info
!===============================================================================

dismax = -grand
dismin =  grand

do iel = 1, ncel
  dismin = min(distpa(iel),dismin)
  dismax = max(distpa(iel),dismax)
enddo

if (irangp.ge.0) then
  call parmin(dismin)
  call parmax(dismax)
endif

write(nfecra,1000)dismin, dismax, nittot

! Free memory
deallocate(viscf, viscb)
deallocate(coefad, coefbd)
deallocate(cofafd, cofbfd)
deallocate(dam, xam)
deallocate(rtpdp, smbdp, rovsdp)
deallocate(w1, w2, w3)
deallocate(w7, w8, w9)

!===============================================================================
! 7. Formats
!===============================================================================

#if defined(_CS_LANG_FR)

 1000 format(                                                           &
'                                                             ',/,&
' ** DISTANCE A LA PAROI                                      ',/,&
'    -------------------                                      ',/,&
'                                                             ',/,&
'   Distance min = ',E14.5    ,'  Distance max = ',E14.5       ,/,&
'                                                             ',/,&
'     (Calcul de la distance realise en ',I10   ,' iterations)',/)

 5000 format(1X,A8,' : SWEEP = ',I5,' NORME SECOND MEMBRE = ',E14.6)

 8000   format(                                                         &
'@                                                            ',/,&
'@ @@ ATTENTION : Calcul de la distance a la paroi            ',/,&
'@    =========                                               ',/,&
'@  La variable associee ne converge pas a la cellule ',I10    ,/,&
'@       Coord X      Coord Y      Coord Z                    ',/,&
'@ ',3E13.5                                                    ,/)

 9000   format(                                                         &
'@                                                            ',/,&
'@ @@ ATTENTION : Calcul de la distance a la paroi            ',/,&
'@    =========                                               ',/,&
'@  La solution du laplacien ne respecte pas le principe du   ',/,&
'@  maximum. On recalcule le laplacien sans les               ',/,&
'@  reconstructions.                                          ',/)

 9001   format(                                                         &
'@                                                            ',/,&
'@ @@ ATTENTION : Calcul de la distance a la paroi            ',/,&
'@    =========                                               ',/,&
'@  La solution du laplacien ne respecte pas le principe du   ',/,&
'@  maximum. (lapalcien negatif : ', E14.6,')                 ',/)


#else

 1000 format(                                                           &
'                                                             ',/,&
' ** WALL DISTANCE                                            ',/,&
'    -------------                                            ',/,&
'                                                             ',/,&
'  Min distance = ',E14.5    ,' Max distance = ',E14.5         ,/,&
'                                                             ',/,&
'     (Distance calculation done in ',I10   ,' iterations)'    ,/)

 5000 format(1X,A8,' : SWEEP = ',I5,' RIGHT HAND SIDE NORM = ',E14.6)

 8000   format(                                                         &
'@                                                            ',/,&
'@ @@ WARNING: Wall distance calculation                      ',/,&
'@    ========                                                ',/,&
'@  The associated variable does not converge in cell ',I10    ,/,&
'@       Coord X      Coord Y      Coord Z                    ',/,&
'@ ',3E13.5                                                    ,/)

 9000   format(                                                         &
'@                                                            ',/,&
'@ @@ WARNING: Wall distance calculation                      ',/,&
'@    =========                                               ',/,&
'@  The laplacian solution does not respect the maximum       ',/,&
'@  principle. We recompute the laplacien without             ',/,&
'@  reconstructions.                                          ',/)

 9001   format(                                                         &
'@                                                            ',/,&
'@ @@ WARNING: Wall distance calculation                      ',/,&
'@    =========                                               ',/,&
'@  The laplacian solution does not respect the maximum       ',/,&
'@  principle. (laplacian solution is  negative :', E14.6,')    ',/)

#endif

return
end subroutine
