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

subroutine navsto &
!================

 ( idbia0 , idbra0 ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  , iterns , icvrge ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   isostd ,                                                       &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   tslagr , coefa  , coefb  , frcxt  ,                            &
   trava  , ximpa  , uvwk   ,                                     &
   viscf  , viscb  , viscfi , viscbi ,                            &
   dam    , xam    ,                                              &
   drtp   , trav   , smbr   , rovsdt ,                            &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , w10    , dfrcxt , frchy  , dfrchy , &
   coefu  , esflum , esflub ,                                     &
   rdevel , rtuser , ra     )

!===============================================================================
! FONCTION :
! ----------

! RESOLUTION DES EQUATIONS N-S 1 PHASE INCOMPRESSIBLE OU RO VARIABLE
! SUR UN PAS DE TEMPS (CONVECTION/DIFFUSION - PRESSION /CONTINUITE)

!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! idbia0           ! i  ! <-- ! number of first free position in ia            !
! idbra0           ! i  ! <-- ! number of first free position in ra            !
! ndim             ! i  ! <-- ! spatial dimension                              !
! ncelet           ! i  ! <-- ! number of extended (real + ghost) cells        !
! ncel             ! i  ! <-- ! number of cells                                !
! nfac             ! i  ! <-- ! number of interior faces                       !
! nfabor           ! i  ! <-- ! number of boundary faces                       !
! nfml             ! i  ! <-- ! number of families (group classes)             !
! nprfml           ! e  ! <-- ! nombre de proprietes des familles              !
! nnod             ! i  ! <-- ! number of vertices                             !
! lndfac           ! i  ! <-- ! size of nodfac indexed array                   !
! lndfbr           ! i  ! <-- ! size of nodfbr indexed array                   !
! ncelbr           ! i  ! <-- ! number of cells with faces on boundary         !
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! nphas            ! i  ! <-- ! number of phases                               !
! iterns           ! e  ! <-- ! numero d'iteration sur navsto                  !
! icvrge           ! e  ! <-- ! indicateur de convergence du pnt fix           !
! nideve, nrdeve   ! i  ! <-- ! sizes of idevel and rdevel arrays              !
! nituse, nrtuse   ! i  ! <-- ! sizes of ituser and rtuser arrays              !
! ifacel(2, nfac)  ! ia ! <-- ! interior faces -> cells connectivity           !
! ifabor(nfabor)   ! ia ! <-- ! boundary faces -> cells connectivity           !
! ifmfbr(nfabor)   ! ia ! <-- ! boundary face family numbers                   !
! ifmcel(ncelet)   ! ia ! <-- ! cell family numbers                            !
! iprfml           ! ia ! <-- ! property numbers per family                    !
!  (nfml, nprfml)  !    !     !                                                !
! ipnfac(nfac+1)   ! ia ! <-- ! interior faces -> vertices index (optional)    !
! nodfac(lndfac)   ! ia ! <-- ! interior faces -> vertices list (optional)     !
! ipnfbr(nfabor+1) ! ia ! <-- ! boundary faces -> vertices index (optional)    !
! nodfbr(lndfbr)   ! ia ! <-- ! boundary faces -> vertices list (optional)     !
! isostd           ! te ! <-- ! indicateur de sortie standard                  !
!    (nfabor+1)    !    !     !  +numero de la face de reference               !
! idevel(nideve)   ! ia ! <-> ! integer work array for temporary development   !
! ituser(nituse)   ! ia ! <-> ! user-reserved integer work array               !
! ia(*)            ! ia ! --- ! main integer work array                        !
! xyzcen           ! ra ! <-- ! cell centers                                   !
!  (ndim, ncelet)  !    !     !                                                !
! surfac           ! ra ! <-- ! interior faces surface vectors                 !
!  (ndim, nfac)    !    !     !                                                !
! surfbo           ! ra ! <-- ! boundary faces surface vectors                 !
!  (ndim, nfabor)  !    !     !                                                !
! cdgfac           ! ra ! <-- ! interior faces centers of gravity              !
!  (ndim, nfac)    !    !     !                                                !
! cdgfbo           ! ra ! <-- ! boundary faces centers of gravity              !
!  (ndim, nfabor)  !    !     !                                                !
! xyznod           ! ra ! <-- ! vertex coordinates (optional)                  !
!  (ndim, nnod)    !    !     !                                                !
! volume(ncelet)   ! ra ! <-- ! cell volumes                                   !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at current and previous time steps)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! frcxt(ncelet,    ! tr ! <-- ! force exterieure generant la pression          !
!   3,nphas)       !    !     !  hydrostatique                                 !
! tslagr           ! tr ! <-- ! terme de couplage retour du                    !
!(ncelet,*)        !    !     !     lagrangien                                 !
! trava,ximpa      ! tr ! <-- ! tableau de travail pour couplage               !
!ncelet,3,nphas    !    !     ! vitesse pression par point fixe                !
! uvwk             ! tr ! <-- ! tableau de travail pour couplage u/p           !
!ncelet,3,nphas    !    !     ! sert a stocker la vitesse de                   !
!                  !    !     ! l'iteration precedente                         !
! viscf(nfac)      ! tr ! --- ! visc*surface/dist aux faces internes           !
! viscb(nfabor     ! tr ! --- ! visc*surface/dist aux faces de bord            !
! viscfi(nfac)     ! tr ! --- ! idem viscf pour increments                     !
! viscbi(nfabor    ! tr ! --- ! idem viscb pour increments                     !
! dam(ncelet       ! tr ! --- ! tableau de travail pour matrice                !
! xam(nfac,*)      ! tr ! --- ! tableau de travail pour matrice                !
! drtp(ncelet      ! tr ! --- ! tableau de travail pour increment              !
! trav(ncelet,3    ! tr ! --- ! tableau de travail pour gradient               !
! smbr  (ncelet    ! tr ! --- ! tableau de travail pour sec mem                !
! rovsdt(ncelet    ! tr ! --- ! tableau de travail pour terme instat           !
! w1..10(ncelet    ! tr ! --- ! tableau de travail                             !
! dfrcxt(ncelet    ! tr ! --- ! variation de force exterieure                  !
!   3,nphas)       !    !     !  generant la pression hydrostatique            !
! frchy(ncelet     ! tr ! --- ! tableau de travail                             !
!  ndim  )         !    !     !  pression hydrostatique                        !
! dfrchy(ncelet    ! tr ! --- ! tableau de travail variation de                !
!  ndim  )         !    !     !  pression hydrostatique                        !
! coefu(nfab,3)    ! tr ! --- ! tableau de travail                             !
! esflum(nfac)     ! tr ! --- ! tableau de travail (iestot  )                  !
! esflub(nfabor    ! tr ! --- ! tableau de travail (iestot  )                  !
! rdevel(nrdeve)   ! ra ! <-> ! real work array for temporary development      !
! rtuser(nrtuse)   ! ra ! <-> ! user-reserved real work array                  !
! ra(*)            ! ra ! --- ! main real work array                           !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail

!===============================================================================

implicit none

!===============================================================================
! Common blocks
!===============================================================================

include "dimfbr.h"
include "paramx.h"
include "numvar.h"
include "entsor.h"
include "cstphy.h"
include "cstnum.h"
include "optcal.h"
include "pointe.h"
include "albase.h"
include "period.h"
include "parall.h"
include "ppppar.h"
include "ppthch.h"
include "ppincl.h"
include "cplsat.h"

!===============================================================================

! Arguments

integer          idbia0 , idbra0
integer          ndim   , ncelet , ncel   , nfac   , nfabor
integer          nfml   , nprfml
integer          nnod   , lndfac , lndfbr , ncelbr
integer          nvar   , nscal  , nphas  , iterns , icvrge
integer          nideve , nrdeve , nituse , nrtuse

integer          ifacel(2,nfac) , ifabor(nfabor)
integer          ifmfbr(nfabor) , ifmcel(ncelet)
integer          iprfml(nfml,nprfml)
integer          ipnfac(nfac+1), nodfac(lndfac)
integer          ipnfbr(nfabor+1), nodfbr(lndfbr)
integer          isostd(nfabor+1,nphas)
integer          idevel(nideve), ituser(nituse)
integer          ia(*)

double precision xyzcen(ndim,ncelet)
double precision surfac(ndim,nfac), surfbo(ndim,nfabor)
double precision cdgfac(ndim,nfac), cdgfbo(ndim,nfabor)
double precision xyznod(ndim,nnod), volume(ncelet)
double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(ndimfb,*)
double precision tslagr(ncelet,*)
double precision coefa(ndimfb,*), coefb(ndimfb,*)
double precision frcxt(ncelet,3,nphas)
double precision trava(ncelet,ndim,nphas),ximpa(ncelet,ndim,nphas)
double precision uvwk(ncelet,ndim,nphas)
double precision viscf(nfac), viscb(nfabor)
double precision viscfi(nfac), viscbi(nfabor)
double precision dam(ncelet), xam(nfac,2)
double precision drtp(ncelet), trav(ncelet,3)
double precision smbr(ncelet), rovsdt(ncelet)
double precision w1(ncelet), w2(ncelet), w3(ncelet)
double precision w4(ncelet), w5(ncelet), w6(ncelet)
double precision w7(ncelet), w8(ncelet), w9(ncelet), w10(ncelet)
double precision dfrcxt(ncelet,3,nphas)
double precision frchy(ncelet,ndim), dfrchy(ncelet,ndim)
double precision coefu(nfabor,3)
double precision esflum(nfac), esflub(nfabor)
double precision rdevel(nrdeve), rtuser(nrtuse), ra(*)

! Local variables

integer          idebia, idebra
integer          iccocg, inc, iel, iel1, iel2, ifac, imax, iii
integer          ii    , inod
integer          iphas , iph, isou, ivar, iitsm, igamm1
integer          ipriph, iuiph , iviph , iwiph , iclipr, iclipf
integer          icliup, iclivp, icliwp, init
integer          icluma, iclvma, iclwma
integer          iflmas, iflmab, ipcrom, ipbrom
integer          iflms1, iflmb1, iflmb0, iismph
integer          nswrgp, imligp, iwarnp, imaspe
integer          idimte, itenso
integer          nbrval, iappel, iescop, idtsca
integer          iflint, iflbrd, icocgv, ifinra
integer          ndircp, icpt  , iecrw
integer          numcpl
double precision rnorm , rnorma, rnormi, vitnor
double precision dtsrom, unsrom, surf  , rhom
double precision epsrgp, climgp, extrap, xyzmax(3)
double precision thetap, xdu, xdv, xdw
double precision ro0iph, p0iph, pr0iph, xxp0 , xyp0 , xzp0
double precision rhofac, dtfac, ddepx , ddepy, ddepz
double precision xnrdis
double precision vitbox, vitboy, vitboz

!===============================================================================

!===============================================================================
! 1.  INITIALISATION
!===============================================================================

if(iwarni(iu(1)).ge.1) then
  write(nfecra,1000)
endif

idebia = idbia0
idebra = idbra0

if(nterup.gt.1) then

  do iphas = 1, nphas

    iuiph   = iu (iphas)
    iviph   = iv (iphas)
    iwiph   = iw (iphas)
    ipriph  = ipr(iphas)
    do isou = 1, 3
      if(isou.eq.1) ivar = iuiph
      if(isou.eq.2) ivar = iviph
      if(isou.eq.3) ivar = iwiph
!     La boucle sur NCELET est une securite au cas
!       ou on utiliserait UVWK par erreur a ITERNS = 1
      do iel = 1,ncelet
        uvwk(iel,isou,iphas) = rtp(iel,ivar)
      enddo
    enddo

! Calcul de la norme L2 de la vitesse
    if(iterns.eq.1) then
      xnrmu0(iphas) = 0.d0
      iuiph   = iu (iphas)
      iviph   = iv (iphas)
      iwiph   = iw (iphas)
      do iel = 1, ncel
        xnrmu0(iphas) = xnrmu0(iphas) +(rtpa(iel,iuiph)**2        &
                                      + rtpa(iel,iviph)**2        &
                                      + rtpa(iel,iwiph)**2)       &
                                      * volume(iel)
      enddo
      if(irangp.ge.0) then
        call parsom (xnrmu0(iphas))
        !==========
      endif
! En cas de couplage entre deux instances de Code_Saturne, on calcule
! la norme totale de la vitesse
! Necessaire pour que l'une des instances ne stoppe pas plus tot que les autres
! (il faudrait quand meme verifier les options numeriques, ...)
      do numcpl = 1, nbrcpl
        call tbrcpl ( numcpl, 1, 1, xnrmu0(iphas), xnrdis )
        !==========
        xnrmu0(iphas) = xnrmu0(iphas) + xnrdis
      enddo
      xnrmu0(iphas) = sqrt(xnrmu0(iphas))
    endif

! On assure la periodicite ou le parallelisme de UVWK et la pression
! (cette derniere vaut la pression a l'iteration precedente)
    if(iterns.gt.1) then
      if(irangp.ge.0) then
        call parcom (uvwk(1,1,iphas))
        !==========
        call parcom (uvwk(1,2,iphas))
        !==========
        call parcom (uvwk(1,3,iphas))
        !==========
        call parcom (rtpa(1,ipriph))
        !==========
      endif
      if(iperio.eq.1) then
        idimte = 1
        itenso = 0
        call percom                                               &
        !==========
      ( idimte , itenso ,                                         &
        uvwk(1,1,iphas),uvwk(1,1,iphas),uvwk(1,1,iphas),          &
        uvwk(1,2,iphas),uvwk(1,2,iphas),uvwk(1,2,iphas),          &
        uvwk(1,3,iphas),uvwk(1,3,iphas),uvwk(1,3,iphas))
        idimte = 0
        itenso = 0
        call percom                                               &
        !==========
      ( idimte , itenso ,                                         &
        rtpa(1,ipriph),rtpa(1,ipriph),rtpa(1,ipriph),             &
        rtpa(1,ipriph),rtpa(1,ipriph),rtpa(1,ipriph),             &
        rtpa(1,ipriph),rtpa(1,ipriph),rtpa(1,ipriph))
      endif
    endif

  enddo

endif


!===============================================================================
! 2.  ETAPE DE PREDICTION DES VITESSES
!===============================================================================

do iphas = 1, nphas

  iappel = 1
  iuiph  = iu(iphas)
  iflmas = ipprof(ifluma(iuiph))
  iflmab = ipprob(ifluma(iuiph))
  iph    = iphas

  call preduv                                                     &
  !==========
 ( idebia , idebra , iappel ,                                     &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  , iterns ,                            &
   ncepdc(iphas)   , ncetsm(iphas)   ,                            &
   nideve , nrdeve , nituse , nrtuse , iph    ,                   &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   ia(iicepd(iphas))        , ia(iicesm(iphas))       ,           &
   ia(iitpsm(iphas))        ,                                     &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   propfa(1,iflmas), propfb(1,iflmab),                            &
   tslagr , coefa  , coefb  ,                                     &
   ra(ickupd(iphas))        , ra(ismace(iphas))        ,  frcxt , &
   trava  , ximpa  , uvwk   , dfrcxt , ra(itpuco)      ,  trav  , &
   viscf  , viscb  , viscfi , viscbi ,                            &
   dam    , xam    ,                                              &
   drtp   , smbr   , rovsdt ,                                     &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , w10    , coefu  ,                   &
   rdevel , rtuser , ra     )

enddo


! --- Sortie si pas de pression continuite (on suppose que
!       la pression est unique, donc pas de iphas dans le test),
!       on met a jour les flux de masse, et on sort

if( iprco.le.0 ) then

  do iphas = 1, nphas

    iuiph  = iu(iphas)
    iviph  = iv(iphas)
    iwiph  = iw(iphas)

    icliup = iclrtp(iuiph ,icoef)
    iclivp = iclrtp(iviph ,icoef)
    icliwp = iclrtp(iwiph ,icoef)

    iflmas = ipprof(ifluma(iuiph))
    iflmab = ipprob(ifluma(iuiph))
    ipcrom = ipproc(irom  (iphas))
    ipbrom = ipprob(irom  (iphas))

    init   = 1
    inc    = 1
    iccocg = 1
    iflmb0 = 1
    if (iale.eq.1) iflmb0 = 0
    iismph = iisymp+nfabor*(iphas-1)
    nswrgp = nswrgr(iuiph)
    imligp = imligr(iuiph)
    iwarnp = iwarni(iuiph)
    epsrgp = epsrgr(iuiph)
    climgp = climgr(iuiph)
    extrap = extrag(iuiph)

    iph    = iphas

    imaspe = 1

    call inimas                                                   &
    !==========
 ( idebia , idebra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   iuiph  , iviph  , iwiph  , imaspe , iph    ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr , ia(iismph) ,               &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtp(1,iuiph) , rtp(1,iviph) , rtp(1,iwiph) ,                   &
   coefa(1,icliup), coefa(1,iclivp), coefa(1,icliwp),             &
   coefb(1,icliup), coefb(1,iclivp), coefb(1,icliwp),             &
   propfa(1,iflmas), propfb(1,iflmab) ,                           &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   rdevel , rtuser , ra     )

  enddo

!     En ALE on doit rajouter la composante en vitesse de maillage
  if (iale.eq.1) then

    icluma = iclrtp(iuma ,icoef)
    iclvma = iclrtp(ivma ,icoef)
    iclwma = iclrtp(iwma ,icoef)

!     On change de signe car on veut l'oppose de la vitesse de maillage
!       aux faces
    do iel = 1, ncelet
      rtp(iel,iuma) = -rtp(iel,iuma)
      rtp(iel,ivma) = -rtp(iel,ivma)
      rtp(iel,iwma) = -rtp(iel,iwma)
    enddo
    do ifac = 1, nfabor
      coefa(ifac,icluma) = -coefa(ifac,icluma)
      coefa(ifac,iclvma) = -coefa(ifac,iclvma)
      coefa(ifac,iclwma) = -coefa(ifac,iclwma)
    enddo

!     One temporary array needed for internal faces, in case some internal vertices
!       are moved directly by the user
    iflint = idebra
    ifinra = iflint + nfac

    CALL RASIZE('NAVSTO',IFINRA)
    !==========

    do iphas = 1, nphas

      iflmas = ipprof(ifluma(iu(iphas)))
      iflmab = ipprob(ifluma(iu(iphas)))
      ipcrom = ipproc(irom  (iphas))
      ipbrom = ipprob(irom  (iphas))

      init   = 0
      inc    = 1
      iccocg = 1
      iflmb0 = 1
      nswrgp = nswrgr(iuma )
      imligp = imligr(iuma )
      iwarnp = iwarni(iuma )
      epsrgp = epsrgr(iuma )
      climgp = climgr(iuma )
      extrap = extrag(iuma )

      iph    = iphas

      imaspe = 1

      do ifac = 1, nfac
        ra(iflint+ifac-1) = 0.d0
      enddo

      call inimas                                                 &
      !==========
 ( idebia , ifinra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   iuiph  , iviph  , iwiph  , imaspe , iph    ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr , ia(iismph) ,               &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtp(1,iuma )    , rtp(1,ivma )    , rtp(1,iwma)     ,          &
   coefa(1,icluma), coefa(1,iclvma), coefa(1,iclwma),             &
   coefb(1,icluma), coefb(1,iclvma), coefb(1,iclwma),             &
   ra(iflint)     , propfb(1,iflmab) ,                            &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   rdevel , rtuser , ra     )

    enddo

    do iel = 1, ncelet
      rtp(iel,iuma) = -rtp(iel,iuma)
      rtp(iel,ivma) = -rtp(iel,ivma)
      rtp(iel,iwma) = -rtp(iel,iwma)
    enddo
    do ifac = 1, nfabor
      coefa(ifac,icluma) = -coefa(ifac,icluma)
      coefa(ifac,iclvma) = -coefa(ifac,iclvma)
      coefa(ifac,iclwma) = -coefa(ifac,iclwma)
    enddo

    do ifac = 1, nfac
      iecrw = 0
      ddepx = 0.d0
      ddepy = 0.d0
      ddepz = 0.d0
      icpt  = 0
      do ii = ipnfac(ifac),ipnfac(ifac+1)-1
        inod = nodfac(ii)
        if (ia(iimpal+inod-1).eq.0) iecrw = iecrw + 1
        icpt = icpt + 1
        ddepx = ddepx + ra(idepal       +inod-1)                  &
               +ra(ixyzn0+(inod-1)*ndim  )-xyznod(1,inod)
        ddepy = ddepy + ra(idepal+nnod  +inod-1)                  &
               +ra(ixyzn0+(inod-1)*ndim+1)-xyznod(2,inod)
        ddepz = ddepz + ra(idepal+2*nnod+inod-1)                  &
               +ra(ixyzn0+(inod-1)*ndim+2)-xyznod(3,inod)
      enddo
!     If all the face vertices have imposed displacement, w is evaluated from
!       this displacement
      if (iecrw.eq.0) then
        iel1 = ifacel(1,ifac)
        iel2 = ifacel(2,ifac)
        dtfac = 0.5d0*(dt(iel1) + dt(iel2))
        rhofac = 0.5d0*(propce(iel1,ipcrom) + propce(iel2,ipcrom))
        propfa(ifac,iflmas) = propfa(ifac,iflmas) - rhofac*(      &
                              ddepx*surfac(1,ifac)                &
                             +ddepy*surfac(2,ifac)                &
                             +ddepz*surfac(3,ifac) )/dtfac/icpt
!     Else w is calculated from the cell-centre mesh velocity
      else
        propfa(ifac,iflmas) = propfa(ifac,iflmas)                 &
                            + ra(iflint+ifac-1)
      endif
    enddo
  endif

  ! Ajout de la vitesse du solide dans le flux convectif,
  ! si le maillage est mobile (solide rigide)
  ! En turbomachine, on conna�t exactement la vitesse de maillage � ajouter
  if (imobil.eq.1) then

    do iphas = 1, nphas

      iflmas = ipprof(ifluma(iu(iphas)))
      iflmab = ipprob(ifluma(iu(iphas)))
      ipcrom = ipproc(irom  (iphas))
      ipbrom = ipprob(irom  (iphas))

      do ifac = 1, nfac
        iel1 = ifacel(1,ifac)
        iel2 = ifacel(2,ifac)
        dtfac  = 0.5d0*(dt(iel1) + dt(iel2))
        rhofac = 0.5d0*(propce(iel1,ipcrom) + propce(iel2,ipcrom))
        vitbox = omegay*cdgfac(3,ifac) - omegaz*cdgfac(2,ifac)
        vitboy = omegaz*cdgfac(1,ifac) - omegax*cdgfac(3,ifac)
        vitboz = omegax*cdgfac(2,ifac) - omegay*cdgfac(1,ifac)
        propfa(ifac,iflmas) = propfa(ifac,iflmas) - rhofac*(        &
      vitbox*surfac(1,ifac) + vitboy*surfac(2,ifac) + vitboz*surfac(3,ifac) )
      enddo
      do ifac = 1, nfabor
        iel = ifabor(ifac)
        dtfac  = dt(iel)
        rhofac = propfb(ifac,ipbrom)
        vitbox = omegay*cdgfbo(3,ifac) - omegaz*cdgfbo(2,ifac)
        vitboy = omegaz*cdgfbo(1,ifac) - omegax*cdgfbo(3,ifac)
        vitboz = omegax*cdgfbo(2,ifac) - omegay*cdgfbo(1,ifac)
        propfb(ifac,iflmab) = propfb(ifac,iflmab) - rhofac*(        &
      vitbox*surfbo(1,ifac) + vitboy*surfbo(2,ifac) + vitboz*surfbo(3,ifac) )
      enddo
    enddo

  endif

  return

endif

!===============================================================================
! 3.  ETAPE DE PRESSION/CONTINUITE ( VITESSE/PRESSION )
!===============================================================================

if(iwarni(iu(1)).ge.1) then
  write(nfecra,1200)
endif

! On n'appelle resolp qu'une seule fois, pour LA phase qu'il faut

iphas = 1
iph   = iphas

! --- Pas de temps scalaire ou pas
idtsca = 0
if ((ipucou.eq.1).or.(ncpdct(iphas).gt.0)) idtsca = 1

call resolp                                                       &
!==========
 ( idebia , idebra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   ncepdc(iphas)   , ncetsm(iphas)   ,                            &
   nideve , nrdeve , nituse , nrtuse , iph    ,                   &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   ia(iicepd(iphas))        , ia(iicesm(iphas))       ,           &
   ia(iitpsm(iphas))        , isostd , idtsca ,                   &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
   ra(ickupd(iphas))        , ra(ismace(iphas))        ,          &
   frcxt  , dfrcxt , ra(itpuco)      , trav   ,                   &
   viscf  , viscb  , viscfi , viscbi ,                            &
   dam    , xam    ,                                              &
   drtp   , smbr   , rovsdt , tslagr ,                            &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , frchy  , dfrchy , coefu  , trava ,  &
   rdevel , rtuser , ra     )


! Si on est en polyphasique, il faudra de toutes maniere modifier
!  tout ceci. Pour le moment, on se contente de mettre a jour les
!  flux de masse des phases 2 a N en postulant qu'ils sont tous egaux
!  au flux de masse de la phase 1 ...
! Ca ne sert qu'aux tests bien sur

if(nphas.gt.1) then
  iflms1 = ipprof(ifluma(iu(1    )))
  iflmb1 = ipprob(ifluma(iu(1    )))
  do iphas = 2, nphas
    iflmas = ipprof(ifluma(iu(iphas)))
    iflmab = ipprob(ifluma(iu(iphas)))
    do ifac = 1, nfac
      propfa(ifac,iflmas) = propfa(ifac,iflms1)
    enddo
    do ifac = 1, nfabor
      propfb(ifac,iflmab) = propfb(ifac,iflmb1)
    enddo
  enddo
endif

!===============================================================================
! 4.  REACTUALISATION DU CHAMP DE VITESSE
!===============================================================================


do iphas = 1, nphas

  ipriph = ipr(iphas)
  iuiph  = iu(iphas)
  iviph  = iv(iphas)
  iwiph  = iw(iphas)

  iclipr = iclrtp(ipriph,icoef)
  iclipf = iclrtp(ipriph,icoeff)
  icliup = iclrtp(iuiph ,icoef)
  iclivp = iclrtp(iviph ,icoef)
  icliwp = iclrtp(iwiph ,icoef)

  iflmas = ipprof(ifluma(iuiph))
  iflmab = ipprob(ifluma(iuiph))
  ipcrom = ipproc(irom  (iphas))
  ipbrom = ipprob(irom  (iphas))
  iismph = iisymp+nfabor*(iphas-1)



!       IREVMC = 0 : Methode standard (pas par moindres carres) : on
!                      ajoute un gradient d'increment de pression standard
!                      a la vitesse predite opur obtenir la vitesse corrigee

!       IREVMC = 1 : On applique la methode par moindres carres a
!                      l'ecart entre le flux de masse predit et le flux
!                      de masse actualise,
!                      c'est-a-dire au gradient d'increment de pression
!                    On ajoute la grandeur obtenue aux cellules a la vitesse
!                      predite pour obtenir la vitesse actualisee
!                    Cette methode correspond a IREVMC = 0 avec
!                      gradient par moindres carres IMRGRA=1 dans la
!                      reactualisation des vitesses.

!       IREVMC = 2 : On applique la methode par moindres carres au
!                      flux de masse actualise
!                      pour obtenir la vitesse actualisee
!                    Cette methode correspond a la methode RT0.

!       La methode IREVMC = 2 semble plus "diffusive", mais semble aussi la
!         seule issue pour certains ecoulements atmospheriques de mercure.
!       La methode IREVMC = 1 semble ne pas trop "diffuser", avec un
!         gain du a l'utilisation du gradient moindres carres. Elle
!         se rapproche beaucoup de IREVMC=0.


  if( irevmc(iphas).eq.1 ) then

!     On a besoin de trois tableaux de travail
    iflint = idebra
    iflbrd = iflint + nfac
    icocgv = iflbrd + nfabor
    ifinra = icocgv + ncelet*9

    CALL RASIZE('NAVSTO',IFINRA)
    !==========

!     on ote la partie en u-predit dans le flux de masse final,
!     on projete des faces vers le centre, puis on rajoute u-predit.
    init   = 1
    inc    = 1
    iccocg = 1
    iflmb0 = 1
    if (iale.eq.1) iflmb0 = 0
    nswrgp = nswrgr(iuiph )
    imligp = imligr(iuiph )
    iwarnp = iwarni(iuiph )
    epsrgp = epsrgr(iuiph )
    climgp = climgr(iuiph )
    extrap = extrag(iuiph )

    iph  = iphas

    imaspe = 1

    call inimas                                                   &
    !==========
 ( idebia , ifinra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   iuiph  , iviph  , iwiph  , imaspe , iph    ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr , ia(iismph) ,               &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtp(1,iuiph)    , rtp(1,iviph)    , rtp(1,iwiph)    ,          &
   coefa(1,icliup), coefa(1,iclivp), coefa(1,icliwp),             &
   coefb(1,icliup), coefb(1,iclivp), coefb(1,icliwp),             &
   ra(iflint), ra(iflbrd),                                        &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   rdevel , rtuser , ra     )

    do ifac = 1, nfac
      ra(iflint+ifac-1) = propfa(ifac,iflmas) - ra(iflint+ifac-1)
    enddo
    do ifac = 1, nfabor
      ra(iflbrd+ifac-1) = propfb(ifac,iflmab) - ra(iflbrd+ifac-1)
    enddo

    call recvmc                                                   &
    !==========
 ( idebia , ifinra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), ra(iflint)      , ra(iflbrd)      ,          &
   w1     , w2     , w3     ,                                     &
   w4     , w5     , w6     , ra(icocgv)      ,                   &
   rdevel , rtuser , ra     )

    do iel = 1, ncel
      rtp(iel,iuiph) = rtp(iel,iuiph) + w1(iel)
      rtp(iel,iviph) = rtp(iel,iviph) + w2(iel)
      rtp(iel,iwiph) = rtp(iel,iwiph) + w3(iel)
    enddo

  elseif( irevmc(iphas).eq.2 ) then

!     On calcule la vitesse corrigee directement a partir du flux de masse
!       corrige    .
!     On a besoin de trois tableaux de travail
    icocgv = idebra
    ifinra = icocgv + ncelet*9

    CALL RASIZE('NAVSTO',IFINRA)
    !==========

    call recvmc                                                   &
    !==========
 ( idebia , ifinra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfa(1,iflmas), propfb(1,iflmab),          &
   rtp(1,iuiph), rtp(1,iviph), rtp(1,iwiph),                      &
   w4     , w5     , w6     , ra(icocgv)   ,                      &
   rdevel , rtuser , ra     )


  else

!     On corrige la vitesse predite par le gradient cellule de
!       l'increment de pression

!     GRADIENT DE L'INCREMENT TOTAL DE PRESSION

    if (idtvar.lt.0) then
      do iel = 1, ncel
        drtp(iel) = (rtp(iel,ipriph) -rtpa(iel,ipriph))           &
                   /relaxv(ipriph)
      enddo
    else
      do iel = 1, ncel
        drtp(iel) = rtp(iel,ipriph) -rtpa(iel,ipriph)
      enddo
    endif

! --->    TRAITEMENT DU PARALLELISME

    if(irangp.ge.0) call parcom (drtp)
                              !==========

! ---> TRAITEMENT DE LA PERIODICITE

    if(iperio.eq.1) then
      idimte = 0
      itenso = 0
      call percom                                                 &
      !==========
      ( idimte , itenso ,                                         &
        drtp   , drtp   , drtp  ,                                 &
        drtp   , drtp   , drtp  ,                                 &
        drtp   , drtp   , drtp  )
    endif


    iccocg = 1
    inc = 0
    if (iphydr.eq.1) inc = 1
    nswrgp = nswrgr(ipriph)
    imligp = imligr(ipriph)
    iwarnp = iwarni(ipriph)
    epsrgp = epsrgr(ipriph)
    climgp = climgr(ipriph)
    extrap = extrag(ipriph)

    call grdcel                                                   &
    !==========
 ( idebia , idebra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr , nphas  ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ipriph , imrgra , inc    , iccocg , nswrgp , imligp , iphydr , &
   iwarnp , nfecra , epsrgp , climgp , extrap ,                   &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   dfrcxt(1,1,iphas),dfrcxt(1,2,iphas),dfrcxt(1,3,iphas),         &
   drtp   , coefa(1,iclipf) , coefb(1,iclipr)  ,                  &
   trav(1,1)       , trav(1,2)       , trav(1,3) ,                &
!        ---------         ---------         ---------
   w1     , w2     , w3     ,                                     &
   rdevel , rtuser , ra     )

!     REACTUALISATION DU CHAMP DE VITESSES

    thetap = thetav(ipriph)
    if (iphydr.eq.0) then
      if (idtsca.eq.0) then
        do iel = 1, ncel
          dtsrom = -thetap*dt(iel)/propce(iel,ipcrom)
          rtp(iel,iuiph) = rtp(iel,iuiph)+dtsrom*trav(iel,1)
          rtp(iel,iviph) = rtp(iel,iviph)+dtsrom*trav(iel,2)
          rtp(iel,iwiph) = rtp(iel,iwiph)+dtsrom*trav(iel,3)
        enddo
      else
        do iel = 1, ncel
          unsrom = -thetap/propce(iel,ipcrom)
          iii = itpuco-1+iel
          rtp(iel,iuiph) = rtp(iel,iuiph)                         &
                          +unsrom*ra(iii         )*trav(iel,1)
          rtp(iel,iviph) = rtp(iel,iviph)                         &
                          +unsrom*ra(iii+ncelet  )*trav(iel,2)
          rtp(iel,iwiph) = rtp(iel,iwiph)                         &
                          +unsrom*ra(iii+2*ncelet)*trav(iel,3)
        enddo
      endif
    else
      if (idtsca.eq.0) then
        do iel = 1, ncel
          dtsrom = thetap*dt(iel)/propce(iel,ipcrom)
          rtp(iel,iuiph) = rtp(iel,iuiph)                         &
               +dtsrom*(dfrcxt(iel,1,iphas)-trav(iel,1) )
          rtp(iel,iviph) = rtp(iel,iviph)                         &
               +dtsrom*(dfrcxt(iel,2,iphas)-trav(iel,2) )
          rtp(iel,iwiph) = rtp(iel,iwiph)                         &
               +dtsrom*(dfrcxt(iel,3,iphas)-trav(iel,3) )
        enddo
      else
        do iel = 1, ncel
          unsrom = thetap/propce(iel,ipcrom)
          iii = itpuco-1+iel
          rtp(iel,iuiph) = rtp(iel,iuiph)                         &
               +unsrom*ra(iii         )                           &
               *(dfrcxt(iel,1,iphas)-trav(iel,1) )
          rtp(iel,iviph) = rtp(iel,iviph)                         &
               +unsrom*ra(iii+ncelet  )                           &
               *(dfrcxt(iel,2,iphas)-trav(iel,2) )
          rtp(iel,iwiph) = rtp(iel,iwiph)                         &
               +unsrom*ra(iii+2*ncelet)                           &
               *(dfrcxt(iel,3,iphas)-trav(iel,3) )
        enddo
      endif
!     mise a jour des forces exterieures pour le calcul des gradients
      do iel=1,ncel
        frcxt(iel,1,iphas) = frcxt(iel,1,iphas)                   &
             + dfrcxt(iel,1,iphas)
        frcxt(iel,2,iphas) = frcxt(iel,2,iphas)                   &
             + dfrcxt(iel,2,iphas)
        frcxt(iel,3,iphas) = frcxt(iel,3,iphas)                   &
             + dfrcxt(iel,3,iphas)
      enddo
      if(irangp.ge.0) then
        call parcom (frcxt(1,1,iphas))
        !==========
        call parcom (frcxt(1,2,iphas))
        !==========
        call parcom (frcxt(1,3,iphas))
        !==========
      endif
      if(iperio.eq.1) then
        idimte = 1
        itenso = 0
        call percom                                               &
        !==========
  ( idimte , itenso ,                                             &
    frcxt(1,1,iphas),frcxt(1,1,iphas),frcxt(1,1,iphas),           &
    frcxt(1,2,iphas),frcxt(1,2,iphas),frcxt(1,2,iphas),           &
    frcxt(1,3,iphas),frcxt(1,3,iphas),frcxt(1,3,iphas) )
      endif
!     mise a jour des Dirichlets de pression en sortie dans COEFA
      iclipr = iclrtp(ipriph,icoef)
      iclipf = iclrtp(ipriph,icoeff)
      do ifac = 1,nfabor
        if (isostd(ifac,iphas).eq.1)                              &
             coefa(ifac,iclipr) = coefa(ifac,iclipr)              &
             + coefa(ifac,iclipf)
      enddo
    endif

  endif

enddo


!     Ajout de la vitesse de maillage dans le flux convectif en ALE
if (iale.eq.1) then

  icluma = iclrtp(iuma ,icoef)
  iclvma = iclrtp(ivma ,icoef)
  iclwma = iclrtp(iwma ,icoef)

!     On change de signe car on veut l'oppose de la vitesse de maillage
!       aux faces
  do iel = 1, ncelet
    rtp(iel,iuma) = -rtp(iel,iuma)
    rtp(iel,ivma) = -rtp(iel,ivma)
    rtp(iel,iwma) = -rtp(iel,iwma)
  enddo
  do ifac = 1, nfabor
    coefa(ifac,icluma) = -coefa(ifac,icluma)
    coefa(ifac,iclvma) = -coefa(ifac,iclvma)
    coefa(ifac,iclwma) = -coefa(ifac,iclwma)
  enddo

!     One temporary array needed for internal faces, in case some internal vertices
!       are moved directly by the user
    iflint = idebra
    ifinra = iflint + nfac

    CALL RASIZE('NAVSTO',IFINRA)
    !==========

  do iphas = 1, nphas

    iflmas = ipprof(ifluma(iu(iphas)))
    iflmab = ipprob(ifluma(iu(iphas)))
    ipcrom = ipproc(irom  (iphas))
    ipbrom = ipprob(irom  (iphas))

    init   = 0
    inc    = 1
    iccocg = 1
    iflmb0 = 1
    nswrgp = nswrgr(iuma )
    imligp = imligr(iuma )
    iwarnp = iwarni(iuma )
    epsrgp = epsrgr(iuma )
    climgp = climgr(iuma )
    extrap = extrag(iuma )

    iph    = iphas

    imaspe = 1

    do ifac = 1, nfac
      ra(iflint+ifac-1) = 0.d0
    enddo

    call inimas                                                   &
    !==========
 ( idebia , ifinra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   iuiph  , iviph  , iwiph  , imaspe , iph    ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr , ia(iismph) ,               &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtp(1,iuma )    , rtp(1,ivma )    , rtp(1,iwma)     ,          &
   coefa(1,icluma), coefa(1,iclvma), coefa(1,iclwma),             &
   coefb(1,icluma), coefb(1,iclvma), coefb(1,iclwma),             &
   ra(iflint)     , propfb(1,iflmab) ,                            &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   rdevel , rtuser , ra     )

  enddo

  do iel = 1, ncelet
    rtp(iel,iuma) = -rtp(iel,iuma)
    rtp(iel,ivma) = -rtp(iel,ivma)
    rtp(iel,iwma) = -rtp(iel,iwma)
  enddo
  do ifac = 1, nfabor
    coefa(ifac,icluma) = -coefa(ifac,icluma)
    coefa(ifac,iclvma) = -coefa(ifac,iclvma)
    coefa(ifac,iclwma) = -coefa(ifac,iclwma)
  enddo

  do ifac = 1, nfac
    iecrw = 0
    ddepx = 0.d0
    ddepy = 0.d0
    ddepz = 0.d0
    icpt  = 0
    do ii = ipnfac(ifac),ipnfac(ifac+1)-1
      inod = nodfac(ii)
      if (ia(iimpal+inod-1).eq.0) iecrw = iecrw + 1
      icpt = icpt + 1
      ddepx = ddepx + ra(idepal       +inod-1)                    &
             +ra(ixyzn0+(inod-1)*ndim  )-xyznod(1,inod)
      ddepy = ddepy + ra(idepal+nnod  +inod-1)                    &
             +ra(ixyzn0+(inod-1)*ndim+1)-xyznod(2,inod)
      ddepz = ddepz + ra(idepal+2*nnod+inod-1)                    &
             +ra(ixyzn0+(inod-1)*ndim+2)-xyznod(3,inod)
    enddo
!     If all the face vertices have imposed displacement, w is evaluated from
!       this displacement
    if (iecrw.eq.0) then
      iel1 = ifacel(1,ifac)
      iel2 = ifacel(2,ifac)
      dtfac = 0.5d0*(dt(iel1) + dt(iel2))
      rhofac = 0.5d0*(propce(iel1,ipcrom) + propce(iel2,ipcrom))
      propfa(ifac,iflmas) = propfa(ifac,iflmas) - rhofac*(        &
                            ddepx*surfac(1,ifac)                  &
                           +ddepy*surfac(2,ifac)                  &
                           +ddepz*surfac(3,ifac) )/dtfac/icpt
!     Else w is calculated from the cell-centre mesh velocity
    else
      propfa(ifac,iflmas) = propfa(ifac,iflmas)                   &
                          + ra(iflint+ifac-1)
    endif
  enddo

endif

! Ajout de la vitesse du solide dans le flux convectif,
! si le maillage est mobile (solide rigide)
! En turbomachine, on conna�t exactement la vitesse de maillage � ajouter
if (imobil.eq.1) then

  do iphas = 1, nphas

    iflmas = ipprof(ifluma(iu(iphas)))
    iflmab = ipprob(ifluma(iu(iphas)))
    ipcrom = ipproc(irom  (iphas))
    ipbrom = ipprob(irom  (iphas))

    do ifac = 1, nfac
      iel1 = ifacel(1,ifac)
      iel2 = ifacel(2,ifac)
      dtfac  = 0.5d0*(dt(iel1) + dt(iel2))
      rhofac = 0.5d0*(propce(iel1,ipcrom) + propce(iel2,ipcrom))
      vitbox = omegay*cdgfac(3,ifac) - omegaz*cdgfac(2,ifac)
      vitboy = omegaz*cdgfac(1,ifac) - omegax*cdgfac(3,ifac)
      vitboz = omegax*cdgfac(2,ifac) - omegay*cdgfac(1,ifac)
      propfa(ifac,iflmas) = propfa(ifac,iflmas) - rhofac*(        &
        vitbox*surfac(1,ifac) + vitboy*surfac(2,ifac) + vitboz*surfac(3,ifac) )
    enddo
    do ifac = 1, nfabor
      iel = ifabor(ifac)
      dtfac  = dt(iel)
      rhofac = propfb(ifac,ipbrom)
      vitbox = omegay*cdgfbo(3,ifac) - omegaz*cdgfbo(2,ifac)
      vitboy = omegaz*cdgfbo(1,ifac) - omegax*cdgfbo(3,ifac)
      vitboz = omegax*cdgfbo(2,ifac) - omegay*cdgfbo(1,ifac)
      propfb(ifac,iflmab) = propfb(ifac,iflmab) - rhofac*(        &
        vitbox*surfbo(1,ifac) + vitboy*surfbo(2,ifac) + vitboz*surfbo(3,ifac) )
    enddo
  enddo

endif


!===============================================================================
! 5.  CALCUL D'UN ESTIMATEUR D'ERREUR DE L'ETAPE DE CORRECTION ET TOTAL
!===============================================================================


do iphas = 1, nphas

  if(iescal(iescor,iphas).gt.0.or.iescal(iestot,iphas).gt.0) then

! ---> REPERAGE DES VARIABLES

    ipriph = ipr(iphas)
    iuiph  = iu(iphas)
    iviph  = iv(iphas)
    iwiph  = iw(iphas)

    icliup = iclrtp(iuiph ,icoef)
    iclivp = iclrtp(iviph ,icoef)
    icliwp = iclrtp(iwiph ,icoef)

    ipcrom = ipproc(irom  (iphas))
    ipbrom = ipprob(irom  (iphas))
    iismph = iisymp+nfabor*(iphas-1)



! ---> ECHANGE DES VITESSES ET PRESSION EN PERIODICITE ET PARALLELISME

!    Pour les estimateurs IESCOR et IESTOT, la vitesse doit etre echangee.

!    Pour l'estimateur IESTOT, la pression doit etre echangee aussi.

!    Cela ne remplace pas l'echange du debut de pas de temps
!     a cause de usproj qui vient plus tard et des calculs suite)


! --- Vitesse

    if(irangp.ge.0) then
      call parcom (rtp(1,iuiph ))
      !==========
      call parcom (rtp(1,iviph ))
      !==========
      call parcom (rtp(1,iwiph ))
      !==========
    endif

    if(iperio.eq.1) then
      idimte = 1
      itenso = 0
      call percom                                                 &
      !==========
         ( idimte , itenso ,                                      &
           rtp(1,iuiph), rtp(1,iuiph), rtp(1,iuiph),              &
           rtp(1,iviph), rtp(1,iviph), rtp(1,iviph),              &
           rtp(1,iwiph), rtp(1,iwiph), rtp(1,iwiph))
    endif


!  -- Pression

    if(iescal(iestot,iphas).gt.0) then

      if(irangp.ge.0) then
        call parcom (rtp(1,ipriph))
        !==========
      endif

      if(iperio.eq.1) then
        idimte = 0
        itenso = 0
        call percom                                               &
        !==========
           ( idimte , itenso ,                                    &
           rtp(1,ipriph), rtp(1,ipriph), rtp(1,ipriph),           &
           rtp(1,ipriph), rtp(1,ipriph), rtp(1,ipriph),           &
           rtp(1,ipriph), rtp(1,ipriph), rtp(1,ipriph))
      endif

    endif


! ---> CALCUL DU FLUX DE MASSE DEDUIT DE LA VITESSE REACTUALISEE

    init   = 1
    inc    = 1
    iccocg = 1
    iflmb0 = 1
    if (iale.eq.1) iflmb0 = 0
    nswrgp = nswrgr(iuiph )
    imligp = imligr(iuiph )
    iwarnp = iwarni(iuiph )
    epsrgp = epsrgr(iuiph )
    climgp = climgr(iuiph )
    extrap = extrag(iuiph )

    iph    = iphas

    imaspe = 1

    call inimas                                                   &
    !==========
 ( idebia , idebra ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   iuiph  , iviph  , iwiph  , imaspe , iph    ,                   &
   nideve , nrdeve , nituse , nrtuse ,                            &
   iflmb0 , init   , inc    , imrgra , iccocg , nswrgp , imligp , &
   iwarnp , nfecra ,                                              &
   epsrgp , climgp , extrap ,                                     &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr , ia(iismph) ,               &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   propce(1,ipcrom), propfb(1,ipbrom),                            &
   rtp(1,iuiph)    , rtp(1,iviph)    , rtp(1,iwiph)    ,          &
   coefa(1,icliup), coefa(1,iclivp), coefa(1,icliwp),             &
   coefb(1,icliup), coefb(1,iclivp), coefb(1,icliwp),             &
   esflum , esflub ,                                              &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , coefu  ,                            &
   rdevel , rtuser , ra     )


! ---> CALCUL DE L'ESTIMATEUR CORRECTION : DIVERGENCE DE ROM * U (N + 1)
!                                          - GAMMA

    if(iescal(iescor,iphas).gt.0) then
      init = 1
      call divmas(ncelet,ncel,nfac,nfabor,init,nfecra,            &
                                   ifacel,ifabor,esflum,esflub,w1)

      if (ncetsm(iphas).gt.0) then
        igamm1 = ismace(iphas)+(ipriph-1)*ncetsm(iphas)-1
        do iitsm = 1, ncetsm(iphas)
          iel = ia(iicesm(iphas)+iitsm-1)
          w1(iel) = w1(iel)-volume(iel)*ra(igamm1+iitsm)
        enddo
      endif

      if(iescal(iescor,iphas).eq.2) then
        iescop = ipproc(iestim(iescor,iphas))
        do iel = 1, ncel
          propce(iel,iescop) =  abs(w1(iel))
        enddo
      elseif(iescal(iescor,iphas).eq.1) then
        iescop = ipproc(iestim(iescor,iphas))
        do iel = 1, ncel
          propce(iel,iescop) =  abs(w1(iel)) / volume(iel)
        enddo
      endif
    endif


! ---> CALCUL DE L'ESTIMATEUR TOTAL

    if(iescal(iestot,iphas).gt.0) then

!   INITIALISATION DE TRAV AVEC LE TERME INSTATIONNAIRE

      do iel = 1, ncel
        trav(iel,1) = propce(iel,ipcrom) * volume(iel) *          &
             ( rtpa(iel,iuiph)- rtp(iel,iuiph) )/dt(iel)
        trav(iel,2) = propce(iel,ipcrom) * volume(iel) *          &
             ( rtpa(iel,iviph)- rtp(iel,iviph) )/dt(iel)
        trav(iel,3) = propce(iel,ipcrom) * volume(iel) *          &
             ( rtpa(iel,iwiph)- rtp(iel,iwiph) )/dt(iel)
      enddo

!   APPEL A PREDUV AVEC RTP ET RTP AU LIEU DE RTP ET RTPA
!                  AVEC LE FLUX DE MASSE RECALCULE
      iappel = 2
      iph    = iphas
      call preduv                                                 &
      !==========
 ( idebia , idebra , iappel ,                                     &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  , iterns ,                            &
   ncepdc(iphas)   , ncetsm(iphas)   ,                            &
   nideve , nrdeve , nituse , nrtuse , iph    ,                   &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   ia(iicepd(iphas))        , ia(iicesm(iphas))       ,           &
   ia(iitpsm(iphas))        ,                                     &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   dt     , rtp    , rtp    , propce , propfa , propfb ,          &
   esflum , esflub ,                                              &
   tslagr , coefa  , coefb  ,                                     &
   ra(ickupd(iphas))        , ra(ismace(iphas))        , frcxt  , &
   trava  , ximpa  , uvwk   , dfrcxt , ra(itpuco)      , trav   , &
   viscf  , viscb  , viscfi , viscbi ,                            &
   dam    , xam    ,                                              &
   drtp   , smbr   , rovsdt ,                                     &
   w1     , w2     , w3     , w4     , w5     , w6     ,          &
   w7     , w8     , w9     , w10    , coefu  ,                   &
   rdevel , rtuser , ra     )

    endif

  endif

enddo

!===============================================================================
! 6.  TRAITEMENT DU POINT FIXE SUR LE SYSTEME VITESSE/PRESSION
!===============================================================================

if(nterup.gt.1) then
! TEST DE CONVERGENCE DE L'ALGORITHME ITERATIF
! On initialise ICVRGE a 1 et on le met a 0 si une des phases n'est
! pas convergee

  icvrge = 1

  do iphas = 1,nphas
    xnrmu(iphas) = 0.d0
    do iel = 1,ncel
      xdu = rtp(iel,iuiph) - uvwk(iel,1,iphas)
      xdv = rtp(iel,iviph) - uvwk(iel,2,iphas)
      xdw = rtp(iel,iwiph) - uvwk(iel,3,iphas)
      xnrmu(iphas) = xnrmu(iphas) +(xdu**2 + xdv**2 + xdw**2)     &
                                  * volume(iel)
    enddo
! --->    TRAITEMENT DU PARALLELISME

    if(irangp.ge.0) call parsom (xnrmu(iphas))
                                !==========
! -- >    TRAITEMENT DU COUPLAGE ENTRE DEUX INSTANCES DE CODE_SATURNE
    do numcpl = 1, nbrcpl
      call tbrcpl ( numcpl, 1, 1, xnrmu(iphas), xnrdis )
      !==========
      xnrmu(iphas) = xnrmu(iphas) + xnrdis
    enddo
    xnrmu(iphas) = sqrt(xnrmu(iphas))

! Indicateur de convergence du point fixe
    if(xnrmu(iphas).ge.epsup(iphas)*xnrmu0(iphas)) icvrge = 0

  enddo


endif

! ---> RECALAGE DE LA PRESSION SUR UNE PRESSION A MOYENNE NULLE
!  On recale si on n'a pas de Dirichlet. Or le nombre de Dirichlets
!  calcule dans typecl.F est NDIRCL si IDIRCL=1 et NDIRCL-1 si IDIRCL=0
!  (ISTAT vaut toujours 0 pour la pression)

do iphas = 1, nphas
  ipriph  = ipr(iphas)
  if (idircl(ipr(iphas)).eq.1) then
    ndircp = ndircl(ipr(iphas))
  else
    ndircp = ndircl(ipr(iphas))-1
  endif
  if(ndircp.le.0) then
    call prmoy0                                                   &
    !==========
    ( idebia , idebra ,                                           &
      ncelet , ncel   , nfac   , nfabor ,                         &
      nideve , nrdeve , nituse , nrtuse ,                         &
      iphas  , idevel , ituser , ia     ,                         &
      volume , rtp(1,ipriph) ,                                    &
      rdevel , rtuser , ra     )
  endif

! Calcul de la pression totale IPRTOT : (definie comme propriete )
! En compressible, la pression resolue est deja la pression totale

  if (ippmod(icompf).lt.0) then
    ro0iph = ro0  (iphas)
    p0iph  = p0   (iphas)
    pr0iph = pred0(iphas)
    xxp0   = xyzp0(1,iphas)
    xyp0   = xyzp0(2,iphas)
    xzp0   = xyzp0(3,iphas)
    do iel=1,ncel
      propce(iel,ipproc(iprtot(iphas)))= rtp(iel,ipr(iphas))      &
           + ro0iph*( gx*(xyzcen(1,iel)-xxp0)                     &
                    + gy*(xyzcen(2,iel)-xyp0)                     &
                    + gz*(xyzcen(3,iel)-xzp0) )                   &
           + p0iph - pr0iph
    enddo
  endif


enddo

!===============================================================================
! 7.  IMPRESSIONS
!===============================================================================

do iphas = 1, nphas

  ipriph = ipr(iphas)
  iuiph  = iu(iphas)
  iviph  = iv(iphas)
  iwiph  = iw(iphas)

  iflmas = ipprof(ifluma(iuiph))
  iflmab = ipprob(ifluma(iuiph))
  ipcrom = ipproc(irom  (iphas))
  ipbrom = ipprob(irom  (iphas))

  if (iwarni(iuiph).ge.1) then

    write(nfecra,2000)iphas

    rnorm = -1.d0
    do iel = 1, ncel
      rnorm  = max(rnorm,abs(rtp(iel,ipriph)))
    enddo
    if (irangp.ge.0) call parmax (rnorm)
                               !==========
    write(nfecra,2100)rnorm

    rnorm = -1.d0
    do iel = 1, ncel
      vitnor =                                                    &
       sqrt(rtp(iel,iuiph)**2+rtp(iel,iviph)**2+rtp(iel,iwiph)**2)
      if(vitnor.ge.rnorm) then
        rnorm = vitnor
        imax  = iel
      endif
    enddo

    xyzmax(1) = xyzcen(1,imax)
    xyzmax(2) = xyzcen(2,imax)
    xyzmax(3) = xyzcen(3,imax)

    if (irangp.ge.0) then
      nbrval = 3
      call parmxl (nbrval, rnorm, xyzmax)
      !==========
    endif
                               !==========

    write(nfecra,2200) rnorm,xyzmax(1),xyzmax(2),xyzmax(3)


! Pour la periodicite et le parallelisme, rom est echange dans phyvar


    rnorma = -grand
    rnormi =  grand
    do ifac = 1, nfac
      iel1 = ifacel(1,ifac)
      iel2 = ifacel(2,ifac)
      surf = ra(isrfan-1+ifac)
      rhom = (propce(iel1,ipcrom)+propce(iel2,ipcrom))*0.5d0
      rnorm = propfa(ifac,iflmas)/(surf*rhom)
      rnorma = max(rnorma,rnorm)
      rnormi = min(rnormi,rnorm)
    enddo
    if (irangp.ge.0) then
      call parmax (rnorma)
      !==========
      call parmin (rnormi)
      !==========
    endif
    write(nfecra,2300)rnorma, rnormi

    rnorma = -grand
    rnormi =  grand
    do ifac = 1, nfabor
      rnorm = propfb(ifac,iflmab)/                                &
             (ra(isrfbn-1+ifac)*propfb(ifac,ipbrom))
      rnorma = max(rnorma,rnorm)
      rnormi = min(rnormi,rnorm)
    enddo
    if (irangp.ge.0) then
      call parmax (rnorma)
      !==========
      call parmin (rnormi)
      !==========
    endif
    write(nfecra,2400)rnorma, rnormi

    rnorm = 0.d0
    do ifac = 1, nfabor
      rnorm = rnorm + propfb(ifac,iflmab)
    enddo

    if (irangp.ge.0) call parsom (rnorm)
                               !==========

    write(nfecra,2500)rnorm

    write(nfecra,2001)

    if(nterup.gt.1) then
      if(icvrge.eq.0) then
        write(nfecra,2600) iterns
        write(nfecra,2601) xnrmu(iphas),                          &
                           xnrmu0(iphas), epsup(iphas)
        write(nfecra,2001)
        if(iterns.eq.nterup) then
          write(nfecra,2603)
          write(nfecra,2001)
        endif
      else
        write(nfecra,2602) iterns
        write(nfecra,2601) xnrmu(iphas),                          &
                           xnrmu0(iphas), epsup(iphas)
        write(nfecra,2001)
      endif
    endif

  endif

enddo

!--------
! FORMATS
!--------
#if defined(_CS_LANG_FR)

 1000 format(/,                                                   &
'   ** RESOLUTION POUR LA VITESSE                             ',/,&
'      --------------------------                             ',/)
 1200 format(/,                                                   &
'   ** RESOLUTION POUR LA PRESSION CONTINUITE                 ',/,&
'      --------------------------------------                 ',/)
 2000 format(/,' APRES PRESSION CONTINUITE',/,                    &
'  -- Phase : ',I10                                            ,/,&
'-------------------------------------------------------------'  )
 2100 format(                                                           &
' Pression max.',E12.4   ,' (max. de la valeur absolue)       ',/)
 2200 format(                                                           &
' Vitesse  max.',E12.4   ,' en',3E11.3                         ,/)
 2300 format(                                                           &
' Vitesse  en face interne max.',E12.4   ,' ; min.',E12.4        )
 2400 format(                                                           &
' Vitesse  en face de bord max.',E12.4   ,' ; min.',E12.4        )
 2500 format(                                                           &
' Bilan de masse   au bord   ',E14.6                             )
 2600 format(                                                           &
' Informations Point fixe a l''iteration :',I10                ,/)
 2601 format('norme = ',E12.4,' norme 0 = ',E12.4,' toler  = ',E12.4 ,/)
 2602 format(                                                           &
' Convergence du point fixe a l''iteration ',I10               ,/)
 2603 format(                                                           &
' Non convergence du couplage vitesse pression par point fixe  ' )
 2001 format(                                                           &
'-------------------------------------------------------------',/)

#else

 1000 format(/,                                                   &
'   ** SOLVING VELOCITY'                                       ,/,&
'      ----------------'                                       ,/)
 1200 format(/,                                                   &
'   ** SOLVING CONTINUITY PRESSURE'                            ,/,&
'      ---------------------------'                            ,/)
 2000 format(/,' AFTER CONTINUITY PRESSURE',/,                    &
'  -- Phase : ',I10                                            ,/,&
'-------------------------------------------------------------'  )
 2100 format(                                                           &
' Max. pressure',E12.4   ,' (max. absolute value)'             ,/)
 2200 format(                                                           &
' Max. velocity',E12.4   ,' en',3E11.3                         ,/)
 2300 format(                                                           &
' Max. velocity at interior face',E12.4   ,' ; min.',E12.4       )
 2400 format(                                                           &
' Max. velocity at boundary face',E12.4   ,' ; min.',E12.4       )
 2500 format(                                                           &
' Mass balance  at boundary  ',E14.6                             )
 2600 format(                                                           &
' Fixed point informations at iteration:',I10                  ,/)
 2601 format('norm = ',E12.4,' norm 0 = ',E12.4,' toler  = ',E12.4   ,/)
 2602 format(                                                           &
' Fixed point convergence at iteration ',I10                   ,/)
 2603 format(                                                           &
' Non convergence of fixed point for velocity pressure coupling' )
 2001 format(                                                           &
'-------------------------------------------------------------',/)

#endif

!----
! FIN
!----

return

end subroutine
