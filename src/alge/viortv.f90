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

subroutine viortv &
!================

 ( imvisf ,                                                       &
   w1     ,                                                       &
   viscf  , viscb  )

!===============================================================================
! FONCTION :
! ----------

! CALCUL DE LA VITESSE DE DIFFUSION "ORTHOTROPE"
! VISCF,B = VISCOSITE*SURFACE/DISTANCE, HOMOGENE A UN DEBIT EN KG/S

!         =
! (NX**2*VISC11_MOY_FACE
! +NY**2*VISC22_MOY_FACE+NZ**2*VISC33_MOY_FACE)*SURFACE/DISTANCE

! LA VISCOSITE EST DONNE PAR W1, W2, W3

! RQE : A PRIORI, PAS BESOIN DE TECHNIQUE DE RECONSTRUCTION
!  ( A AMELIORER SI NECESSAIRE )

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! imvisf           ! e  ! <-- ! methode de calcul de la visc face              !
!                  !    !     !  = 0 arithmetique                              !
!                  !    !     !  = 1 harmonique                                !
! w1(3,ncelet)     ! tr ! <-- ! valeurs de la viscosite                        !
! viscf(nfac)      ! tr ! --> ! visc*surface/dist aux faces internes           !
! viscb(nfabor     ! tr ! --> ! visc*surface/dist aux faces de bord            !
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
use optcal, only: iporos
use parall
use period
use mesh

!===============================================================================

implicit none

! Arguments

integer          imvisf


double precision w1(3,ncelet)
double precision viscf(nfac), viscb(nfabor)

! Local variables

integer          ifac, iel, ii, jj, isou
double precision visci(3), viscj(3), surf2(3)
double precision pnd

!===============================================================================

! ---> Periodicity and parallelism treatment

if (irangp.ge.0.or.iperio.eq.1) then
  call syndin(w1)
endif

! Without porosity
if (iporos.eq.0) then

  ! Arithmetic mean
  if (imvisf.eq.0) then

    do ifac = 1, nfac

      ii = ifacel(1,ifac)
      jj = ifacel(2,ifac)

      do isou = 1, 3
        visci(isou) = w1(isou,ii)
        viscj(isou) = w1(isou,jj)
        surf2(isou) = surfac(isou,ifac)**2
      enddo

      viscf(ifac) = 0.5d0*(                                         &
         (visci(1)+viscj(1))*surf2(1)                               &
       + (visci(2)+viscj(2))*surf2(2)                               &
       + (visci(3)+viscj(3))*surf2(3) ) / (surfan(ifac)*dist(ifac))

    enddo

  ! Harmonic mean
  else

    do ifac = 1, nfac

      ii = ifacel(1,ifac)
      jj = ifacel(2,ifac)

      pnd  = pond(ifac)

      do isou = 1, 3
        visci(isou) = w1(isou,ii)
        viscj(isou) = w1(isou,jj)
        surf2(isou) = surfac(isou,ifac)**2
      enddo

      viscf(ifac) = &
        ( visci(1)*viscj(1)*surf2(1)/(pnd*visci(1)+(1.d0-pnd)*viscj(1))  &
        + visci(2)*viscj(2)*surf2(2)/(pnd*visci(2)+(1.d0-pnd)*viscj(2))  &
        + visci(3)*viscj(3)*surf2(3)/(pnd*visci(3)+(1.d0-pnd)*viscj(3))  &
        ) / (surfan(ifac)*dist(ifac))
    enddo

  endif

  do ifac = 1, nfabor

    viscb(ifac) = surfbn(ifac)

  enddo

! With porosity
else

  ! Arithmetic mean
  if (imvisf.eq.0) then

    do ifac = 1, nfac

      ii = ifacel(1,ifac)
      jj = ifacel(2,ifac)

      do isou = 1, 3
        visci(isou) = w1(isou,ii) * porosi(ii)
        viscj(isou) = w1(isou,jj) * porosi(jj)
        surf2(isou) = surfac(isou,ifac)**2
      enddo

      viscf(ifac) = 0.5d0*(                                         &
         (visci(1)+viscj(1))*surf2(1)                               &
       + (visci(2)+viscj(2))*surf2(2)                               &
       + (visci(3)+viscj(3))*surf2(3) ) / (surfan(ifac)*dist(ifac))

    enddo

  ! Harmonic mean
  else

    do ifac = 1, nfac

      ii = ifacel(1,ifac)
      jj = ifacel(2,ifac)

      pnd  = pond(ifac)

      do isou = 1, 3
        visci(isou) = w1(isou,ii) * porosi(ii)
        viscj(isou) = w1(isou,jj) * porosi(jj)
        surf2(isou) = surfac(isou,ifac)**2
      enddo

      viscf(ifac) = &
        ( visci(1)*viscj(1)*surf2(1)/(pnd*visci(1)+(1.d0-pnd)*viscj(1))  &
        + visci(2)*viscj(2)*surf2(2)/(pnd*visci(2)+(1.d0-pnd)*viscj(2))  &
        + visci(3)*viscj(3)*surf2(3)/(pnd*visci(3)+(1.d0-pnd)*viscj(3))  &
        ) / (surfan(ifac)*dist(ifac))
    enddo

  endif

  do ifac = 1, nfabor

    ii = ifabor(ifac)

    viscb(ifac) = surfbn(ifac) * porosi(ii)

  enddo

endif

!----
! End
!----

return

end subroutine
