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

subroutine lageli &
!================

 ( nbpmax , nvp    , nvp1   , nvep   , nivep  ,                   &
   npars  ,                                                       &
   itepa  ,                                                       &
   dnpars ,                                                       &
   ettp   , ettpa  , tepa   )

!===============================================================================
! FONCTION :
! ----------

!   SOUS-PROGRAMME DU MODULE LAGRANGIEN :
!   -------------------------------------

!   ELIMINATION DES PARTICULES QUI SONT SORTIES DU DOMAIME
!     --> on gere la memoire pour eviter les places libres

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nbpmax           ! e  ! <-- ! nombre max de particulies autorise             !
! nvp              ! e  ! <-- ! nombre de variables particulaires              !
! nvp1             ! e  ! <-- ! nvp sans position, vfluide, vpart              !
! nvep             ! e  ! <-- ! nombre info particulaires (reels)              !
! nivep            ! e  ! <-- ! nombre info particulaires (entiers)            !
! npars            ! e  ! --> ! nombre max de particules sorties               !
!                  !    !     !   eliminees                                    !
! itepa            ! te ! <-- ! info particulaires (entiers)                   !
! (nbpmax,nivep    !    !     !   (cellule de la particule,...)                !
! dnpars           ! e  ! --> ! nombre max de particules sorties               !
!                  !    !     !   eliminees  (poids stat inclus)               !
! ettp             ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape courante                !
! ettpa            ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape precedente              !
! tepa             ! tr ! <-- ! info particulaires (reels)                     !
! (nbpmax,nvep)    !    !     !   (poids statistiques,...)                     !
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
use cstnum
use optcal
use entsor
use lagpar
use lagran

!==============================================================================

implicit none

! Arguments

integer          nbpmax , nvp    , nvp1   , nvep  , nivep
integer          npars
integer          itepa(nbpmax,nivep)

double precision dnpars
double precision ettp(nbpmax,nvp) , ettpa(nbpmax,nvp)
double precision tepa(nbpmax,nvep)

!  VARIABLES LOCALES

integer nbp , npt , i , ivar
double precision dnbp

!===============================================================================

nbp  = nbpart
dnbp = dnbpar

npars  = 0
dnpars = 0.d0

do npt = nbpart,1,-1

 if (nbpart.lt.1) then
   WRITE(NFECRA,*) ' erreur lageli '
 endif

  if (itepa(npt,jisor).eq.0) then

    npars  = npars  + 1
    dnpars = dnpars + tepa(nbp,jrpoi)

!      ---> la particule est sortie du domaine

    if (npt.eq.nbp) then

!        ---> c'est la derniere particule, on la supprime seulement

      nbp  = nbp - 1
      dnbp = dnbp - tepa(npt,jrpoi)

      do i = 1,nliste
        if ( liste(i).eq.npt ) then
          liste(i) = -1
        endif
      enddo

    else

!        ---> la particule NPT est supprime et on met a la place la
!             particule NBP

      dnbp = dnbp - tepa(npt,jrpoi)

      do ivar = 1,nvp
        ettp(npt,ivar) = ettp(nbp,ivar)
      enddo

      do ivar = 1,nvp
        ettpa(npt,ivar) = ettpa(nbp,ivar)
      enddo

      do ivar = 1,nvep
        tepa(npt,ivar) = tepa(nbp,ivar)
      enddo

      do ivar = 1,nivep
        itepa(npt,ivar) = itepa(nbp,ivar)
      enddo

      do i = 1,nliste
        if (liste(i).eq.npt) then
          liste(i) = -1
        endif
      enddo

      do i = 1,nliste
        if (liste(i).eq.nbp) then
          liste(i) = npt
        endif
      enddo

      nbp  = nbp - 1

    endif

  endif

enddo

!     ---> On met NBPART a la bonne valeur

nbpart = nbp
dnbpar = dnbp

!===============================================================================

!====
! FIN
!====

end subroutine
