!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2010 EDF S.A., France

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

subroutine elflux &
!================

 ( iappel ,                                                       &
   nvar   , nscal  ,                                              &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  , viscf  , viscb  )

!===============================================================================
! FONCTION :
! ----------

! ROUTINE PHYSIQUE PARTICULIERE POUR LE MODULE ELECTRIQUE

!              CALCULS DES VARIABLES PROPCE
!        ELLES SONT UTILISEES POUR LE CALCUL DES TS

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! iappel           ! e  ! <-- ! numero d'appel                                 !
!                  !    !     ! 1 : j, e, j.e                                  !
!                  !    !     ! 2 : b,    jxb                                  !
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
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
! smacel           ! tr ! <-- ! valeur des variables associee a la             !
! (ncesmp,*   )    !    !     !  source de masse                               !
!                  !    !     !  pour ivar=ipr, smacel=flux de masse           !
! viscf(nfac)      ! tr ! --- ! tableau de travail    faces internes           !
! viscb(nfabor     ! tr ! --- ! tableau de travail    faces de bord            !
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
use entsor
use optcal
use cstphy
use cstnum
use parall
use period
use ppppar
use ppthch
use ppincl
use elincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          iappel
integer          nvar   , nscal


double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision viscf(nfac), viscb(nfabor)

! Local variables

integer          iel
integer          ipcefj, ipcsig, ipcsii
integer          ipcla1, ipcla2, ipcla3
integer          ipcdc1, ipcdc2, ipcdc3
integer          ipcdi1, ipcdi2, ipcdi3
integer          inc   , iccocg, nswrgp, imligp, iwarnp
integer          ivar0 , iclimv
integer          ivar  , modntl

double precision epsrgp, climgp, extrap, vrmin, vrmax, var

double precision, allocatable, dimension(:) :: w1, w2, w3
double precision, allocatable, dimension(:,:) :: grad

!===============================================================================

!===============================================================================
! 0. INITIALISATION
!===============================================================================

! Allocate work arrays
allocate(w1(ncelet), w2(ncelet), w3(ncelet))
allocate(grad(ncelet,3))

! Initialize variables to avoid compiler warnings

ipcsii = 0
ipcdi1 = 0
ipcdi2 = 0
ipcdi3 = 0
ipcla1 = 0
ipcla2 = 0
ipcla3 = 0

! Memoire


! --- Numero des grandeurs physiques
ipcsig = ipproc(ivisls(ipotr))

if(ippmod(ieljou).eq.2 .or. ippmod(ieljou).eq.4) then
  ipcsii = ipproc(ivisls(ipoti))
endif

ipcefj = ipproc(iefjou)
ipcdc1 = ipproc(idjr(1))
ipcdc2 = ipproc(idjr(2))
ipcdc3 = ipproc(idjr(3))

if ( ippmod(ieljou).eq.4 ) then
  ipcdi1 = ipproc(idji(1))
  ipcdi2 = ipproc(idji(2))
  ipcdi3 = ipproc(idji(3))
endif

! --- Necessite d'une impression (impression si MODNTL=0)
if(ntlist.gt.0) then
  modntl = mod(ntcabs,ntlist)
elseif(ntlist.eq.-1.and.ntcabs.eq.ntmabs) then
  modntl = 0
else
  modntl = 1
endif

!===============================================================================
! 1. PREMIER APPEL : J, E => J.E
!===============================================================================

if(iappel.eq.1) then

!===============================================================================
! 1.1 PRISE EN COMPTE DES TERMES SOURCES ET VARIABLES STANDARD ET
!   COMMUNES A TOUTES LES VERSIONS ELECTRIQUES : EFFET JOULE, E, J
!         CAS IELJOU >= 1 ou IELARC >= 1 ou IELION >= 1
!===============================================================================

!    Pour toutes les versions electriques :
!      on doit calculer le terme joule (partie reelle) = j . E
!      produit de la densite de courant par le champ electrique


!   2.1 Calcul du grad (potR)
!  ---------------------------

  ivar = isca(ipotr)
  iclimv = iclrtp(ivar,icoef)

  inc = 1
  iccocg = 1
  nswrgp = nswrgr(ivar)
  imligp = imligr(ivar)
  iwarnp = iwarni(ivar)
  epsrgp = epsrgr(ivar)
  climgp = climgr(ivar)
  extrap = extrag(ivar)

  ! En periodique et parallele, echange avant calcul du gradient
  !     C'est indispensable car on vient de calculer IVAR
  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(rtp(1,ivar))
    !==========
  endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
  ivar0 = 0

  call grdcel                                                     &
  !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtp(1,ivar), coefa(1,iclimv) , coefb(1,iclimv)  ,              &
!       POTR
   grad   )


!   2.2 Calcul du champ electrique E = - grad (potR)
!  -------------------------------------------------

!   2.3 Calcul de la densite de courant j = sig E  :
!  -------------------------------------------------
!                                   PROPCE(IEL,IPCSIG)


  if( ippmod(ieljou).ge.1 .or. ippmod(ielarc).ge.1 ) then
    do iel = 1, ncel
      propce(iel,ipcdc1)= - propce(iel,ipcsig) * grad(iel,1)
      propce(iel,ipcdc2)= - propce(iel,ipcsig) * grad(iel,2)
      propce(iel,ipcdc3)= - propce(iel,ipcsig) * grad(iel,3)
    enddo
  endif

!   2.4 Calcul de l'Effet Joule j . E : PROPCE( .,IPPROC(IEFJOU) )
!  -----------------------------------------------------------------
!                                                           sig E.E
  do iel = 1, ncel
    propce(iel,ipcefj)=                                           &
         propce(iel,ipcsig)*(grad(iel,1)**2+grad(iel,2)**2+grad(iel,3)**2)
  enddo


!   2.5 On imprime les extrema de E et j
!  -------------------------------------

  if(modntl.eq.0) then

    write(nfecra,1000)

!     Grad PotR = -E
    var    = grad(1,1)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var    = grad(iel,1)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Gr_PotRX',VRMIN,VRMAX

    var    = grad(1,2)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var    = grad(iel,2)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Gr_PotRY',VRMIN,VRMAX

    var    = grad(1,3)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var    = grad(iel,3)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Gr_PotRZ',VRMIN,VRMAX

    var    = -propce(1,ipcsig) * grad(1,1)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var = -propce(iel,ipcsig) * grad(iel,1)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Cour_ReX',VRMIN,VRMAX

    var    = -propce(1,ipcsig) * grad(1,2)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var = -propce(iel,ipcsig) * grad(iel,2)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Cour_ReY',VRMIN,VRMAX

    var    = -propce(1,ipcsig) * grad(1,3)
    vrmin = var
    vrmax = var
    do iel = 1, ncel
      var = -propce(iel,ipcsig) * grad(iel,3)
      vrmin = min(vrmin,var)
      vrmax = max(vrmax,var)
    enddo
    if (irangp.ge.0) then
      call parmin (vrmin)
      call parmax (vrmax)
    endif
    WRITE(NFECRA,1010)'Cour_ReZ',VRMIN,VRMAX

  endif



!===============================================================================
! 1.2 PRISE EN COMPTE ET AJOUT DES TERMES SOURCES ET VARIABLES
!    RELATIVES A LA PRISE EN COMPTE DU POTENTIEL COMPLEXE
!                           CAS IELJOU = 2
!===============================================================================

  if(ippmod(ieljou).ge.2 .or. ippmod(ieljou).eq.4) then


!   3.1 Calcul du grad (potI) :
!  ----------------------------

    ivar = isca(ipoti)
    iclimv = iclrtp(ivar,icoef)

    inc = 1
    iccocg = 1
    nswrgp = nswrgr(ivar)
    imligp = imligr(ivar)
    iwarnp = iwarni(ivar)
    epsrgp = epsrgr(ivar)
    climgp = climgr(ivar)
    extrap = extrag(ivar)

    ! En periodique et parallele, echange avant calcul du gradient
    !     C'est indispensable car on vient de calculer IVAR
    if (irangp.ge.0.or.iperio.eq.1) then
      call synsca(rtp(1,ivar))
      !==========
    endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)
    ivar0 = 0

    call grdcel                                                   &
    !==========
  ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,         &
    iwarnp , nfecra , epsrgp , climgp , extrap ,                  &
    rtp(1,ivar), coefa(1,iclimv) , coefb(1,iclimv) ,              &
    grad   )


!   3.2 Calcul du champ electrique Ei = - grad (potI) :
!  -------------------------------------------------

!   3.3 Partie imaginaire de la densite de courant :
!  ------------------------------------------------
!                                   PROPCE(IEL,IPCSIG)

  if ( ippmod(ieljou).eq.4 ) then
    do iel = 1, ncel
      propce(iel,ipcdi1)= -propce(iel,ipcsig)*grad(iel,1)
      propce(iel,ipcdi2)= -propce(iel,ipcsig)*grad(iel,2)
      propce(iel,ipcdi3)= -propce(iel,ipcsig)*grad(iel,3)
    enddo
  endif


!   3.4 Effet Joule total : PROPCE( .,IPPROC(IEFJOU) )
!  ----------------------------------------------------

    do iel = 1, ncel

!             ajout de la partie imaginaire et ...
      propce(iel,ipcefj) = propce(iel,ipcefj)                     &
         + propce(iel,ipcsii)*(grad(iel,1)**2+grad(iel,2)**2+grad(iel,3)**2)
!             .    ..division par 2
      propce(iel,ipcefj) = 0.5d0*propce(iel,ipcefj)

    enddo



!   3.5 On imprime les extrema de E et j
!  -------------------------------------

    if(modntl.eq.0) then

!     Grad PotI = -Ei
      var    = grad(1,1)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = grad(iel,1)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Gr_PotIX',VRMIN,VRMAX

      var    = grad(1,2)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = grad(iel,2)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Gr_PotIY',VRMIN,VRMAX

      var    = grad(1,3)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = grad(iel,3)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Gr_PotIZ',VRMIN,VRMAX

!     j=sigma E
      var    = -propce(1,ipcsii) * grad(1,1)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var = -propce(iel,ipcsii) * grad(iel,1)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Cour_ImX',VRMIN,VRMAX

      var    = -propce(1,ipcsii) * grad(1,2)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var = -propce(iel,ipcsii) * grad(iel,2)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Cour_ImY',VRMIN,VRMAX

      var    = -propce(1,ipcsii) * grad(1,3)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var = -propce(iel,ipcsii) * grad(iel,3)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Cour_ImZ',VRMIN,VRMAX

      write(nfecra,1001)

    endif

  endif

endif
!     Fin du test IAPPEL = 1

!===============================================================================
! 2. DEUXIEME APPEL : A, B, JXB
!===============================================================================

if (iappel.eq.2) then

!===============================================================================
! 2.1 PRISE EN COMPTE ET AJOUT DES TERMES SOURCES ET VARIABLES RELATIVES
!    A LA PRISE EN COMPTE DE L'ARC ELECTRIQUE
!                           CAS IELARC = 1 OU 2
!===============================================================================


  if(ippmod(ielarc).ge.1) then
    ipcla1 = ipproc(ilapla(1))
    ipcla2 = ipproc(ilapla(2))
    ipcla3 = ipproc(ilapla(3))
  endif

!   4.1 ARC ELECTRIQUE DIT 3D : IELARC = 2
!  ----------------------------------------
!         ON PASSE PAR LA RESOLUTION D'EQUATIONS DE POISSON SUR LES
!        -----------------------------------------------------------
!           COMPOSANTES DU POTENTIEL VECTEUR
!          ----------------------------------

  if( ippmod(ielarc).ge.2 ) then

! --> Calcul des composantes du champ magnetique B = (W1, W2, W3)
!===================================================


!    Sur Ax

    ivar = isca(ipotva(1))
    iclimv = iclrtp(ivar,icoef)

    inc = 1
    iccocg = 1
    nswrgp = nswrgr(ivar)
    imligp = imligr(ivar)
    iwarnp = iwarni(ivar)
    epsrgp = epsrgr(ivar)
    climgp = climgr(ivar)
    extrap = extrag(ivar)

    ! En periodique et parallele, echange avant calcul du gradient
    !     C'est indispensable car on vient de calculer IVAR
    if (irangp.ge.0.or.iperio.eq.1) then
      call synsca(rtp(1,ivar))
      !==========
    endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)

    ivar0 = 0

    call grdcel                                                   &
    !==========
 ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,          &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   rtp(1,ivar), coefa(1,iclimv) , coefb(1,iclimv)  ,              &
   grad   )

!       B = rot A

    do iel = 1, ncel
      w1(iel)=  zero
      w2(iel)=  grad(iel,3)
      w3(iel)= -grad(iel,2)
    enddo

!    Sur Ay

    ivar = isca(ipotva(2))
    iclimv = iclrtp(ivar,icoef)

    inc = 1
    iccocg = 1
    nswrgp = nswrgr(ivar)
    imligp = imligr(ivar)
    iwarnp = iwarni(ivar)
    epsrgp = epsrgr(ivar)
    climgp = climgr(ivar)
    extrap = extrag(ivar)

    ! En periodique et parallele, echange avant calcul du gradient
    !     C'est indispensable car on vient de calculer IVAR
    if (irangp.ge.0.or.iperio.eq.1) then
      call synsca(rtp(1,ivar))
      !==========
    endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)

    ivar0 = 0

    call grdcel                                                   &
    !==========
  ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,         &
    iwarnp , nfecra , epsrgp , climgp , extrap ,                  &
    rtp(1,ivar), coefa(1,iclimv) , coefb(1,iclimv) ,              &
    grad   )

!       B = rot A

    do iel = 1, ncel
      w1(iel)= w1(iel) - grad(iel,3)
      w2(iel)= w2(iel) + zero
      w3(iel)= w3(iel) + grad(iel,1)
    enddo

!    Sur Az

    ivar = isca(ipotva(3))
    iclimv = iclrtp(ivar,icoef)

    inc = 1
    iccocg = 1
    nswrgp = nswrgr(ivar)
    imligp = imligr(ivar)
    iwarnp = iwarni(ivar)
    epsrgp = epsrgr(ivar)
    climgp = climgr(ivar)
    extrap = extrag(ivar)

    ! En periodique et parallele, echange avant calcul du gradient
    !     C'est indispensable car on vient de calculer IVAR
    if (irangp.ge.0.or.iperio.eq.1) then
      call synsca(rtp(1,ivar))
      !==========
    endif

!  IVAR0 = 0 (indique pour la periodicite de rotation que la variable
!     n'est pas la vitesse ni Rij)

    ivar0 = 0

    call grdcel                                                   &
    !==========
  ( ivar0  , imrgra , inc    , iccocg , nswrgp , imligp ,         &
    iwarnp , nfecra , epsrgp , climgp , extrap ,                  &
    rtp(1,ivar), coefa(1,iclimv) , coefb(1,iclimv) ,              &
    grad   )

!       B = rot A

    do iel = 1, ncel
      w1(iel)= w1(iel) + grad(iel,2)
      w2(iel)= w2(iel) - grad(iel,1)
      w3(iel)= w3(iel) + zero
    enddo

!   4.2 CAS D'ARC AXISYMETRIQUE : IELARC = 1
!  ------------------------------------------
!         ON PEUT UTILISER LE THEOREME D'AMPERE
!        ---------------------------------------

  else if(ippmod(ielarc).eq.1) then

!      Calcul du Champ magnetique calcule :
!===========================================

!       B = / j d S

!       Cette version n'a pas ete developpe pour l'instant :
!         - elle ne fonctionne que dans le cas d'un arc axisymetrique
!         - elle demande donc un reperage des plans perpendiculaires
!           a l'arc
!         - elle necessite la connaissance des coordonnees de chaque
!           point du maillage

    write(nfecra,2000)
    call csexit (1)

  endif

!   4.3 ARC ELECTRIQUE : CALCUL DES FORCES DE LAPLACE j X B
!  ---------------------------------------------------------
!         POUR TOUS LES CAS D'ARCS ELECTRIQUES
!        --------------------------------------

  if( ippmod(ielarc) .ge. 1 ) then
    do iel = 1, ncel
      propce(iel,ipcla1)= propce(iel,ipcdc2) * w3(iel)            &
                         -propce(iel,ipcdc3) * w2(iel)
      propce(iel,ipcla2)= propce(iel,ipcdc3) * w1(iel)            &
                         -propce(iel,ipcdc1) * w3(iel)
      propce(iel,ipcla3)= propce(iel,ipcdc1) * w2(iel)            &
                         -propce(iel,ipcdc2) * w1(iel)
    enddo
  endif


!   4.4 Impression de B en arc electrique
!  --------------------------------------

  if( ippmod(ielarc) .ge. 2 ) then

    if(modntl.eq.0) then

      write(nfecra,1000)

!     B=rot A
      var    = w1(1)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = w1(iel)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Ch_MagX ',VRMIN,VRMAX

      var    = w2(1)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = w2(iel)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Ch_MagY ',VRMIN,VRMAX

      var    = w3(1)
      vrmin = var
      vrmax = var
      do iel = 1, ncel
        var    = w3(iel)
        vrmin = min(vrmin,var)
        vrmax = max(vrmax,var)
      enddo
      if (irangp.ge.0) then
        call parmin (vrmin)
        call parmax (vrmax)
      endif
      WRITE(NFECRA,1010)'Ch_MagZ ',VRMIN,VRMAX

      write(nfecra,1001)

    endif

  endif

endif
!     Fin du test IAPPEL = 2

! Free memory
deallocate(w1, w2, w3)
deallocate(grad)

!--------
! FORMATS
!--------


 1000 format(/,                                                   &
'-----------------------------------------                    ',/,&
'   Variable         Minimum       Maximum                    ',/,&
'-----------------------------------------                    '  )
 1010 format(                                                           &
'v  ',A8,'    ',E12.5,'  ',E12.5                                 )
 1001 format(                                                           &
'-----------------------------------------                    '  )

 2000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION DANS ELTSSC            ',/,&
'@    =========                                               ',/,&
'@     LA VERSION ARC ELECTRIQUE AVEC THEOREME D''AMPERE      ',/,&
'@       (IELARC = 1)    N''EST PAS DISPONIBLE                ',/,&
'@                                                            ',/,&
'@            VEUILLEZ UTILISER LA VERSION DITE 3D            ',/,&
'@                      IELARC = 2                            ',/,&
'@   AVEC EQUATION DE TRANSPORT SUR LE POTENTIEL VECTEUR A    ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)


!----
! FIN
!----

return
end subroutine
