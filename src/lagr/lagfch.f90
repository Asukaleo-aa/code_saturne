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

subroutine lagfch &
!================

 ( nvar   , nscal  ,                                              &
   nbpmax , nvp    , nvp1   , nvep   , nivep  ,                   &
   ntersl , nvlsta , nvisbr ,                                     &
   itepa  , ibord  ,                                              &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   ettp   , ettpa  , tepa   , statis , stativ ,                   &
   taup   , tlag   , piil   ,                                     &
   tsuf   , tsup   , bx     , tsfext ,                            &
   vagaus , gradpr , gradvf ,                                     &
   romp   , fextla )

!===============================================================================
! FONCTION :
! ----------

!   SOUS-PROGRAMME DU MODULE LAGRANGIEN :
!   -------------------------------------

!    CALCUL DES FORCES DLVO

!       - FORCES DE VAN DER WAALS
!       - FORCES ELECTROSTATIQUES

!    ELLES DOIVENT ETRE CONNUES EN CHAQUE CELLULE
!      ET ETRE HOMOGENES A LA GRAVITE (M/S2)


!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! nbpmax           ! e  ! <-- ! nombre max de particulies autorise             !
! nvp              ! e  ! <-- ! nombre de variables particulaires              !
! nvp1             ! e  ! <-- ! nvp sans position, vfluide, vpart              !
! nvep             ! e  ! <-- ! nombre info particulaires (reels)              !
! nivep            ! e  ! <-- ! nombre info particulaires (entiers)            !
! ntersl           ! e  ! <-- ! nbr termes sources de couplage retour          !
! nvlsta           ! e  ! <-- ! nombre de var statistiques lagrangien          !
! nvisbr           ! e  ! <-- ! nombre de statistiques aux frontieres          !
! itepa            ! te ! <-- ! info particulaires (entiers)                   !
! (nbpmax,nivep    !    !     !   (cellule de la particule,...)                !
! ibord            ! te ! --> ! si nordre=2, contient le numero de la          !
!   (nbpmax)       !    !     !   face d'interaction part/frontiere            !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! ettp             ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape courante                !
! ettpa            ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape precedente              !
! tepa             ! tr ! <-- ! info particulaires (reels)                     !
! (nbpmax,nvep)    !    !     !   (poids statistiques,...)                     !
! statis           ! tr ! <-- ! cumul pour les moyennes des                    !
!(ncelet,nvlsta    !    !     !    statistiques volumiques                     !
! stativ           ! tr ! <-- ! cumul pour les variances des                   !
!(ncelet,          !    !     !    statistiques volumiques                     !
!   nvlsta-1)      !    !     !                                                !
! taup(nbpmax)     ! tr ! <-- ! temps caracteristique dynamique                !
! tlag(nbpmax)     ! tr ! <-- ! temps caracteristique fluide                   !
! piil(nbpmax,3    ! tr ! <-- ! terme dans l'integration des eds up            !
! tsup(nbpmax,3    ! tr ! <-- ! prediction 1er sous-pas pour                   !
!                  !    !     !   la vitesse des particules                    !
! tsuf(nbpmax,3    ! tr ! <-- ! prediction 1er sous-pas pour                   !
!                  !    !     !   la vitesse du fluide vu                      !
! bx(nbpmax,3,2    ! tr ! <-- ! caracteristiques de la turbulence              !
! tsfext(nbpmax    ! tr ! <-- ! infos pour le couplage retour                  !
! vagaus           ! tr ! <-- ! variables aleatoires gaussiennes               !
!(nbpmax,nvgaus    !    !     !                                                !
! gradpr(ncel,3    ! tr ! <-- ! gradient de pression                           !
! gradvf(ncel,3    ! tr ! <-- ! gradient de la vitesse du fluide               !
! romp             ! tr ! --- ! masse volumique des particules                 !
! fextla           ! tr ! --> ! champ de forces exterieur                      !
!(ncelet,3)        !    !     !    utilisateur (m/s2)                          !
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
use cstphy
use optcal
use entsor
use lagpar
use lagran
use ppppar
use ppthch
use ppincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal
integer          nbpmax , nvp    , nvp1   , nvep  , nivep
integer          ntersl , nvlsta , nvisbr

integer          itepa(nbpmax,nivep) , ibord(nbpmax)

double precision dt(ncelet) , rtp(ncelet,*) , rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*) , propfb(nfabor,*)
double precision ettp(nbpmax,nvp) , ettpa(nbpmax,nvp)
double precision tepa(nbpmax,nvep)
double precision statis(ncelet,*),stativ(ncelet,*)
double precision taup(nbpmax) , tlag(nbpmax,3)
double precision piil(nbpmax,3) , bx(nbpmax,3,2)
double precision tsuf(nbpmax,3) , tsup(nbpmax,3)
double precision tsfext(nbpmax)
double precision vagaus(nbpmax,*)
double precision gradpr(ncelet,3) , gradvf(ncelet,9)
double precision romp(nbpmax)
double precision fextla(nbpmax,3)

! Local variables

integer          ip , iel , mode

double precision val , tempf , dnorm
double precision debye, aa

double precision, allocatable, dimension(:) :: dppar
double precision, allocatable, dimension(:) :: dnxpar, dnypar, dnzpar

!===============================================================================

!===============================================================================
! 0.  GESTION MEMOIRE ET INITIALISATION
!===============================================================================


!===============================================================================
! 1. CALCUL DE LA DISTANCE A LA PAROI + NORMAL A LA PAROI
!===============================================================================

! Allocate temporary arrays
allocate(dppar(nbpart))
allocate(dnxpar(nbpart), dnypar(nbpart), dnzpar(nbpart))

do ip = 1,nbpart
  dppar(ip) = 0.d0
  dnxpar(ip) = 0.d0
  dnypar(ip) = 0.d0
  dnzpar(ip) = 0.d0
enddo

call usladp                                                       &
!==========
 ( nvar   , nscal  ,                                              &
   nbpmax , nvp    , nvp1   , nvep   , nivep  ,                   &
   ntersl , nvlsta , nvisbr ,                                     &
   itepa  ,                                                       &
   dt     , rtpa   , propce , propfa , propfb ,                   &
   ettp   , ettpa  , tepa   , statis ,                            &
   taup   , tlag   , piil   ,                                     &
   vagaus , gradpr , gradvf ,                                     &
   romp   ,                                                       &
   dppar  , dnxpar  , dnypar  , dnzpar  )

!===============================================================================
! 2. FORCES DE VAN DER WAALS
!    Pour etre homogene a des m/s2 on divise par la masse
!===============================================================================

do ip = 1,nbpart

! Force = -A/6 dp/2 /D**2

  if ( dppar(ip) .gt. dparmn ) then

    val = (cstham*ettp(ip,jdp)/2.d0)                              &
         /(6.d0*dppar(ip)*dppar(ip))

    dnorm = sqrt( dnxpar(ip)*dnxpar(ip)                 &
                 +dnypar(ip)*dnypar(ip)                 &
                 +dnzpar(ip)*dnzpar(ip) )

! Attention la normale est oriente du fluide vers l'exterieur

    aa = dnorm*ettp(ip,jmp)

    fextla(ip,1) = fextla(ip,1) + val*dnxpar(ip) /aa
    fextla(ip,2) = fextla(ip,2) + val*dnypar(ip) /aa
    fextla(ip,3) = fextla(ip,3) + val*dnzpar(ip) /aa

  endif

enddo

!===============================================================================
! 3. FORCES ELECTROSTATIQUES
!    Pour etre homogene a des m/s2 on divise par la masse
!===============================================================================

do ip = 1,nbpart

  iel = itepa(ip,jisor)

! Calcul de la temperature du fluide en fonction du type
! d'ecoulement

  if ( dppar(ip) .gt. dparmn ) then

    if ( ippmod(icp3pl).ge.0 .or.                                 &
         ippmod(icpl3c).ge.0      ) then

      tempf = propce(iel,ipproc(itemp1)) - tkelvi

    else if ( ippmod(icod3p).ge.0 .or.                            &
              ippmod(icoebu).ge.0 .or.                            &
              ippmod(ielarc).ge.0 .or.                            &
              ippmod(ieljou).ge.0      ) then

      tempf = propce(iel,ipproc(itemp)) - tkelvi

    else if ( iscsth(iscalt).eq.-1 ) then
      tempf = rtp(iel,isca(iscalt))

    else if ( iscsth(iscalt).eq.1 ) then
      tempf = rtp(iel,isca(iscalt)) - tkelvi

    else if ( iscsth(iscalt).eq.2 ) then
      mode = 1
      call usthht (mode, rtp(iel,isca(iscalt)), tempf)
      !==========
    else
      tempf = t0
    endif

! FORCE :

! Longueur de Debye

    if (fion .ne. 0 .and. cstfar .gt. 0.d0) then
      debye  = sqrt( (epseau*epsvid*rr*tempf)                     &
              /(2000.d0*cstfar*cstfar*fion) )
    else
      write(nfecra,9001) fion,cstfar
      call csexit(1)
    endif

    if ( debye .gt. 0.d0 ) then
      debye = sqrt(debye)
    else
      write(nfecra,9002) ip,debye,tempf,fion,epseau,epsvid
      call csexit(1)
    endif

    val = -4.d0*pi*epseau*epsvid*phi1*phi2*(ettp(ip,jdp)/2.d0)    &
            *exp(-dppar(ip)/debye)                          &
            /debye

    dnorm = sqrt( dnxpar(ip)*dnxpar(ip)                 &
                 +dnypar(ip)*dnypar(ip)                 &
                 +dnzpar(ip)*dnzpar(ip) )

! Attention la normale est oriente du fluide vers l'exterieur

    fextla(ip,1)= fextla(ip,1)+val*dnxpar(ip)                &
                 /(dnorm*ettp(ip,jmp))
    fextla(ip,2)= fextla(ip,2)+val*dnypar(ip)                &
                 /(dnorm*ettp(ip,jmp))
    fextla(ip,3)= fextla(ip,3)+val*dnzpar(ip)                &
                 /(dnorm*ettp(ip,jmp))

  endif

enddo

!===============================================================================
! 4. FORCES D'ADHESION
!===============================================================================

if ( dcoup .gt. 0.d0 ) then
  gamasv = cstham/(24.d0*pi*dcoup*dcoup)
else
  write(nfecra,9010) dcoup
  call csexit(1)
endif

do ip = 1,nbpart

! Force = 3*PI*(Dp/2)*Gamma_SV + SIG2*PI*(Dp/2)/Eps0

  if ( dppar(ip) .le. dparmn ) then

    val = 3.d0*pi*(ettp(ip,jdp)/2.d0)*gamasv                      &
        + sigch*sigch*pi*(ettp(ip,jdp)/2.d0)/epsvid

    dnorm = sqrt( dnxpar(ip)*dnxpar(ip)                 &
                 +dnypar(ip)*dnypar(ip)                 &
                 +dnzpar(ip)*dnzpar(ip) )

! Attention la normale est oriente du fluide vers l'exterieur

    aa = dnorm*ettp(ip,jmp)

    fextla(ip,1)= fextla(ip,1)+val*dnxpar(ip) /aa
    fextla(ip,2)= fextla(ip,2)+val*dnypar(ip) /aa
    fextla(ip,3)= fextla(ip,3)+val*dnzpar(ip) /aa

  endif

enddo

! Free memory
deallocate(dppar)
deallocate(dnxpar, dnypar, dnzpar)

!==============================================================================

!--------
! FORMATS
!--------

 9001 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION DU MODULE LAGRANGIEN   ',/,&
'@    =========                                               ',/,&
'@    LA VALEUR DE LA FORCE IONIQUE EST NULLE                 ',/,&
'@ OU LA VALEUR DE LA CONSTANTE DE FARADET EST NEGATIVE       ',/,&
'@                                          OU NULLEE         ',/,&
'@                                                            ',/,&
'@       FORCE IONIQUE                : ',G15.7                ,/,&
'@       CSTE DE FARADET              : ',G15.7                ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les valeurs de FION et de CSTFAR                 ',/,&
'@                              dans la subroutine USLAG1.    ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

 9002 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION DU MODULE LAGRANGIEN   ',/,&
'@    =========                                               ',/,&
'@    LA VALEUR DE L''EPAISSEUR DE LA DOUBLE COUCHE           ',/,&
'@    EST NEGATIVE OU NULLE :                                 ',/,&
'@       NUMERO DE PARTICULE          : ',I10                  ,/,&
'@       EPAISSEUR                    : ',G15.7                ,/,&
'@       TEMPERATURE                  : ',G15.7                ,/,&
'@       FORCE IONIQUE                : ',G15.7                ,/,&
'@       CSTE DIELECTIQUE DU VIDE     : ',G15.7                ,/,&
'@       CSTE DIELECTIQUE DE L''EAU   : ',G15.7                ,/,&
'@       CSTE DE FARADET              : ',G15.7                ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les valeurs des CSTES dans la subroutine USLAG1. ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

 9010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION DU MODULE LAGRANGIEN   ',/,&
'@    =========                                               ',/,&
'@    LA VALEUR DE DISTANCE DE COUPURE EST                    ',/,&
'@    EST NEGATIVE OU NULLE               : ',G15.7            ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier les valeurs des CSTES dans la subroutine USLAG1. ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

!----
! FIN
!----

end subroutine
