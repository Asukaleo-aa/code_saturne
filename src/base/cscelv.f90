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

subroutine cscelv &
!================

 ( nvar   , nscal  ,                                              &
   ivar   ,                                                       &
   dt     , rtpa   , vela   ,                                     &
   propce , propfa , propfb ,                                     &
   coefa  , coefb  , coefav , coefbv ,                            &
   crvexp , crvimp )

!===============================================================================
! FONCTION :
! --------

! EXCHANGE OF COUPLING VARIABLES BETWEEN TO TIMES OF CODE_SATURNE THANKS TO
! BOUNDARY FACES
!

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ivar             ! i  ! <-- ! variable number                                !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtpa             ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules (instant            prec)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! crvexp(3,ncelet) ! tr ! --> ! tableau de travail pour part explicit          !
! crvimp           ! tr ! --> ! tableau de travail pour part implicit          !
! (3,3,ncelet)     !    !     !                                                !
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
use pointe
use numvar
use entsor
use optcal
use cstphy
use cstnum
use parall
use period
use cplsat
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          ivar


double precision dt(ncelet), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision crvexp(3,ncelet), crvimp(3,3,ncelet)
double precision coefav(3,nfabor)
double precision coefbv(3,3,nfabor)
double precision vela(3,ncelet)

! Local variables

integer          numcpl
integer          ncesup , nfbsup
integer          ncecpl , nfbcpl , ncencp , nfbncp
integer          ncedis , nfbdis
integer          ncecpg , ncedig
integer          ityloc , ityvar
integer          stride

integer, allocatable, dimension(:) :: lcecpl , lfbcpl
integer, allocatable, dimension(:) :: locpts

double precision, allocatable, dimension(:,:) :: coopts , djppts , dofpts
double precision, allocatable, dimension(:) :: pndpts
double precision, allocatable, dimension(:,:) :: rvdis, rvcel

!===============================================================================


do numcpl = 1, nbrcpl

!===============================================================================
! 1.  DEFINITION DE CHAQUE COUPLAGE
!===============================================================================

  call nbecpl                                                     &
  !==========
 ( numcpl ,                                                       &
   ncesup , nfbsup ,                                              &
   ncecpl , nfbcpl , ncencp , nfbncp )

  ! Allocate temporary arrays for coupling information
  allocate(lcecpl(ncecpl))
  allocate(lfbcpl(nfbcpl))

  call lelcpl                                                     &
  !==========
 ( numcpl ,                                                       &
   ncecpl , nfbcpl ,                                              &
   lcecpl , lfbcpl )

  deallocate(lfbcpl)

!===============================================================================
! 2.  PREPARATION DES VARIABLES A ENVOYER SUR LES CELLULES
!===============================================================================

  ityvar = 1

! --- Informations g�om�triques de localisation

  call npdcpl(numcpl, ncedis, nfbdis)
  !==========

  ! Allocate temporary arrays for geometric quantities
  allocate(locpts(ncedis))
  allocate(coopts(3,ncedis), djppts(3,ncedis), dofpts(3,ncedis))
  allocate(pndpts(ncedis))

  ! Allocate temporary arrays for variables exchange
  allocate(rvdis(3, ncedis))
  allocate(rvcel(3, ncecpl))

  call coocpl &
  !==========
( numcpl , ncedis , ityvar , &
  ityloc , locpts , coopts , &
  djppts , dofpts , pndpts )

  if (ityloc.eq.2) then
    write(nfecra,1000)
    call csexit(1)
    !==========
  endif

!       On v�rifie qu'il faut bien �changer quelque chose
!       de mani�re globale (� cause des appels � GRDCEL notamment)
  ncecpg = ncecpl
  ncedig = ncedis
  if (irangp.ge.0) then
    call parcpt(ncecpg)
    !==========
    call parcpt(ncedig)
    !==========
  endif


! --- Transfert des variables proprement dit.

  if (ncedig.gt.0) then

    call cspcev                                                   &
    !==========
  ( nvar   , nscal  ,                                             &
    ncedis , ityloc ,                                             &
    ivar   ,                                                      &
    locpts ,                                                      &
    dt     , rtpa   , vela   ,                                    &
    propce , propfa , propfb ,                                    &
    coefa  , coefb  , coefav , coefbv ,                           &
    coopts , rvdis  )

  endif

  ! Free memory
  deallocate(locpts)
  deallocate(coopts, djppts, dofpts)
  deallocate(pndpts)

!       Cet appel est sym�trique, donc on teste sur NCEDIG et NCECPG
!       (rien a envoyer, rien a recevoir)
  if (ncedig.gt.0.or.ncecpg.gt.0) then

    ! for vectorial exchange
    stride = 3

    call varcpl &
    !==========
  ( numcpl , ncedis , ncecpl , ityvar , stride ,                  &
    rvdis  ,                                                      &
    rvcel  )

  endif

  ! Free memory
  deallocate(rvdis)

!===============================================================================
! 3.  TRADUCTION DU COUPLAGE EN TERME DE TERMES SOURCES
!===============================================================================

  if (ncecpg.gt.0) then

    call cs2tsv                                                   &
    !==========
  ( nvar   , nscal  ,                                             &
    ncecpl ,                                                      &
    ivar   ,                                                      &
    lcecpl ,                                                      &
    dt     , rtpa   , vela   ,                                    &
    propce , propfa , propfb ,                                    &
    coefa  , coefb  ,                                             &
    crvexp , crvimp ,                                             &
!   ------   ------
    rvcel  )

  endif

  ! Free memory
  deallocate(rvcel)
  deallocate(lcecpl)

enddo
!     Fin de la boucle sur les couplages


!--------
! FORMATS
!--------
 1000 format(                                                     &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION :                                             ',/,&
'@    =========                                               ',/,&
'@    LE COUPLAGE VIA LES FACES EN TANT QU''ELEMENTS          ',/,&
'@    SUPPORTS N''EST PAS ENCORE GERE PAR LE NOYAU.           ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
!----
! FIN
!----

return
end subroutine
