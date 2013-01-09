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

subroutine lwctss &
!================

 ( nvar   , nscal  , ncepdp , ncesmp ,                            &
   iscal  ,                                                       &
   icepdc , icetsm , itypsm ,                                     &
   izfppp ,                                                       &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  , ckupdc , smacel ,                            &
   smbrs  , rovsdt )

!===============================================================================
! FONCTION :
! ----------

! ROUTINE PHYSIQUE PARTICULIERE : FLAMME PREMELANGE MODELE LWC
!   ON PRECISE LES TERMES SOURCES POUR UN SCALAIRE PP
!   SUR UN PAS DE TEMPS

! ATTENTION : LE TRAITEMENT DES TERMES SOURCES EST DIFFERENT
! ---------   DE CELUI DE USTSSC.F

! ON RESOUT ROVSDT*D(VAR) = SMBRS

! ROVSDT ET SMBRS CONTIENNENT DEJA D'EVENTUELS TERMES SOURCES
!  UTILISATEUR. IL FAUT DONC LES INCREMENTER ET PAS LES
!  ECRASER

! POUR DES QUESTIONS DE STABILITE, ON NE RAJOUTE DANS ROVSDT
!  QUE DES TERMES POSITIFS. IL N'Y A PAS DE CONTRAINTE POUR
!  SMBRS

! DANS LE CAS D'UN TERME SOURCE EN CEXP + CIMP*VAR ON DOIT
! ECRIRE :
!          SMBRS  = SMBRS  + CEXP + CIMP*VAR
!          ROVSDT = ROVSDT + MAX(-CIMP,ZERO)

! ON FOURNIT ICI ROVSDT ET SMBRS (ILS CONTIENNENT RHO*VOLUME)
!    SMBRS en kg variable/s :
!     ex : pour la vitesse            kg m/s2
!          pour les temperatures      kg degres/s
!          pour les enthalpies        Joules/s
!    ROVSDT en kg /s


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
! icepdc(ncelet    ! te ! <-- ! numero des ncepdp cellules avec pdc            !
! icetsm(ncesmp    ! te ! <-- ! numero des cellules a source de masse          !
! itypsm           ! te ! <-- ! type de source de masse pour les               !
! (ncesmp,nvar)    !    !     !  variables (cf. ustsma)                        !
! izfppp           ! te ! --> ! numero de zone de la face de bord              !
! (nfabor)         !    !     !  pour le module phys. part.                    !
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
integer          iscal

integer          icepdc(ncepdp)
integer          icetsm(ncesmp), itypsm(ncesmp,nvar)
integer          izfppp(nfabor)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)
double precision ckupdc(ncepdp,6), smacel(ncesmp,nvar)
double precision smbrs(ncelet), rovsdt(ncelet)

! Local variables

integer          ivar, iel, idirac, ivar0
integer          inc , iccocg
integer          ipcvst
integer          ipcrom, ii

integer          iptscl(ndracm), ipfmal(ndracm)
integer          ipfmel(ndracm), iprhol(ndracm)

double precision sum, epsi
double precision tsgrad, tschim, tsdiss

double precision, allocatable, dimension(:,:) :: gradf, grady
double precision, allocatable, dimension(:) :: w10, w11

!===============================================================================

!===============================================================================
! 1. INITIALISATION
!===============================================================================

epsi   = 1.0d-10

! --- Numero du scalaire a traiter : ISCAL

! --- Numero de la variable associee au scalaire a traiter ISCAL
ivar = isca(iscal)

! ---
ipcrom = ipproc(irom)
ipcvst = ipproc(ivisct)

! --- Numero des grandeurs physiques (voir cs_user_boundary_conditions)
do idirac = 1, ndirac
  iptscl(idirac) = ipproc(itscl(idirac))
  ipfmal(idirac) = ipproc(ifmal(idirac))
  ipfmel(idirac) = ipproc(ifmel(idirac))
  iprhol(idirac) = ipproc(irhol(idirac))
enddo

!===============================================================================
! 2. PRISE EN COMPTE DES TERMES SOURCES
!===============================================================================

if ( ivar.eq.isca(iyfm) ) then

! ---> Terme source pour la fraction massique moyenne de fuel

  do iel = 1, ncel
      sum = zero
      do idirac = 1, ndirac
        sum  = sum + propce(iel,iprhol(idirac))                   &
           *propce(iel,iptscl(idirac))*volume(iel)
      enddo

! terme implicite

      if (rtpa(iel,ivar).gt.epsi) then
        rovsdt(iel) = rovsdt(iel) + max(-sum/rtpa(iel,ivar),zero)
      endif

! terme explicite

       smbrs(iel) =  smbrs(iel) + sum

  enddo

endif

! ---> Terme source pour la variance de la fraction massique moyenne de fuel

if (ivar.eq.isca(iyfp2m)) then

  do iel = 1, ncel
    sum = zero
    do idirac = 1, ndirac
      sum  = sum + (propce(iel,iptscl(idirac))*volume(iel)        &
        *(propce(iel,ipfmal(idirac)) - rtpa(iel,isca(iyfm)))      &
             *propce(iel,iprhol(idirac)))
    enddo
    smbrs(iel) = smbrs(iel) + sum
  enddo

endif

! ---> Terme source pour la covariance

if ( ivar.eq.isca(icoyfp)) then

  ! Allocate a temporary array for gradient computation
  allocate(gradf(ncelet,3), grady(ncelet,3))

  ! Allocate work arrays
  allocate(w10(ncelet), w11(ncelet))

! --- Calcul du gradient de F
!     =======================

  ii = isca(ifm)
  do iel = 1, ncel
    w10(iel) = rtpa(iel,ii)
  enddo

  ! En periodique et parallele, echange avant calcul du gradient
  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(w10)
    !==========
  endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
  ivar0 = 0
  inc = 1
  iccocg = 1

  call grdcel                                                     &
  !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgr(ii) , imligr(ii) ,  &
   iwarni(ii) , nfecra ,                                          &
   epsrgr(ii) , climgr(ii) , extrag(ii) ,                         &
   w10    , coefa(1,iclrtp(ii,icoef))  ,                          &
            coefb(1,iclrtp(ii,icoef))  ,                          &
   gradf  )

! --- Calcul du gradient de Yfuel
!     ===========================

  ii = isca(iyfm)
  do iel = 1, ncel
    w11(iel) = rtpa(iel,ii)
  enddo

  ! En periodique et parallele, echange avant calcul du gradient
  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(w11)
    !==========
  endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
  ivar0 = 0
  inc = 1
  iccocg = 1

  call grdcel                                                     &
  !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgr(ii) , imligr(ii) ,  &
   iwarni(ii) , nfecra ,                                          &
   epsrgr(ii) , climgr(ii) , extrag(ii) ,                         &
   w11    , coefa(1,iclrtp(ii,icoef))  ,                          &
            coefb(1,iclrtp(ii,icoef))  ,                          &
   grady  )


! --- Calcul du terme source
!     ======================


! ---> Calcul de K et Epsilon en fonction du modele de turbulence


! ---- TURBULENCE

  if (itytur.eq.2) then

    do iel = 1, ncel
      w10(iel) = rtpa(iel,ik)
      w11(iel) = rtpa(iel,iep)
    enddo

  elseif (itytur.eq.3) then

    do iel = 1, ncel
      w10(iel) = ( rtpa(iel,ir11)                          &
                  +rtpa(iel,ir22)                          &
                  +rtpa(iel,ir33) ) / 2.d0
      w11(iel) = rtpa(iel,iep)
    enddo

  elseif (iturb.eq.50) then

    do iel = 1, ncel
      w10(iel) = rtpa(iel,ik)
      w11(iel) = rtpa(iel,iep)
    enddo

  elseif (iturb.eq.60) then

    do iel = 1, ncel
      w10(iel) = rtpa(iel,ik)
      w11(iel) = cmu*rtpa(iel,ik)*rtpa(iel,iomg)
    enddo

  endif

  do iel=1,ncel

!  A confirmer :
!   Le terme de dissipation devrait etre implicite
!   Dans le terme de dissipation, il manque une constante Cf
!   Peut-elle etre consideree egale a 1 ?
!   Verifier le signe du terme de production
!-
! terme implicite


    w11(iel) = w11(iel)/(w10(iel)*rvarfl(iscal))                  &
         *volume(iel)*propce(iel,ipcrom)
    rovsdt(iel) = rovsdt(iel) + max(w11(iel),zero)

! terme de gradient

    tsgrad =  (2.0d0                                              &
         * propce(iel,ipcvst)/(sigmas(iscal))                     &
         * (  gradf(iel,1)*grady(iel,1)                           &
            + gradf(iel,2)*grady(iel,2)                           &
            + gradf(iel,3)*grady(iel,3) ))                        &
         * volume(iel)


! terme de dissipation

    tsdiss = -w11(iel) * rtpa(iel,ivar)

! terme de chimique

    tschim = zero
    do idirac = 1, ndirac
      tschim =   tschim                                           &
           + (propce(iel,iptscl(idirac))                          &
           *(propce(iel,ipfmel(idirac))-rtpa(iel,isca(ifm)))      &
           *volume(iel))*propce(iel,iprhol(idirac))
    enddo

! --> Somme des termes

    smbrs(iel) = smbrs(iel) + tschim + tsgrad + tsdiss

  enddo

  ! Free memory
  deallocate(gradf, grady)
  deallocate(w10, w11)

endif

!----
! FIN
!----

return

end subroutine
