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

subroutine inivar &
!================

 ( nvar   , nscal  , ncofab ,                                     &
   dt     , rtp    , propce , propfa , propfb ,                   &
   coefa  , coefb  , frcxt  )

!===============================================================================
! FONCTION :
! --------

! INITIALISATION DES VARIABLES DE CALCUL, DU PAS DE TEMPS
! ET DU TABLEAU INDICATEUR DU CALCUL DE LA DISTANCE A LA PAROI
! PAR L'UTILISATEUR (apres relecture eventuelle d'un fichier suite)
!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ncofab           ! e  ! <-- ! nombre de couples coefa/b pour les cl          !
! dt(ncelet)       ! tr ! <-- ! valeur du pas de temps                         !
! rtp              ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules                                    !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa coefb      ! tr ! <-- ! conditions aux limites aux                     !
!  (nfabor,*)      !    !     !    faces de bord                               !
! frcxt(ncelet,3)  ! tr ! <-- ! force exterieure generant la pression          !
!                  !    !     !  hydrostatique                                 !
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
use optcal
use cstphy
use cstnum
use pointe
use entsor
use parall
use period
use ihmpre
use ppppar
use ppthch
use ppincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal  , ncofab


double precision dt(ncelet), rtp(ncelet,*), propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,ncofab), coefb(nfabor,ncofab)
double precision frcxt(ncelet,3)

! Local variables

character*80     chaine
integer          ivar  , iscal , imom
integer          iel
integer          iclip , ipp  , iok   , ii
integer          idtcm , ipcmom, iiptot
integer          ibormo(nbmomx)

double precision valmax, valmin, vfmin , vfmax
double precision vdtmax, vdtmin
double precision xekmin, xepmin, xomgmn, xphmin, xphmax, xalmin, xalmax
double precision xnumin
double precision x11min, x22min, x33min, valmom
double precision vmomax(nbmomx), vmomin(nbmomx)
double precision xxp0, xyp0, xzp0

double precision rvoid(1)

!===============================================================================

!===============================================================================
! 1.  INITIALISATION
!===============================================================================


iok = 0

!===============================================================================
! 2. ON REPASSE LA MAIN A L'UTILISATEUR POUR LA PROGRAMMATION DES
!    INITIALISATIONS QUI LUI SONT PROPRES
!===============================================================================

! Indicateur d'initialisation des scalaires par l'utilisateur
! (mis a 1 si passage dans USINIV ou PPINIV ou dans l'IHM ; a 0 sinon)

iusini = 1

!   - Interface Code_Saturne
!     ======================

if (iihmpr.eq.1) then

  call uiiniv (ncelet, isuite, isca, iscold, rtp)
  !==========

endif

!   - Sous-programme utilisateur
!     ==========================

call usiniv                                                       &
!==========
 ( nvar   , nscal  ,                                              &
   dt     , rtp    , propce , propfa , propfb , coefa  , coefb  )

!     Avec l'interface, il peut y avoir eu initialisation,
!       meme si usiniv n'est pas utilise.
  if (isuite.eq.0 .and. iihmpr.eq.1) then
    iusini = 1
  endif


! ON FAIT DE LA PHYSIQUE PARTICULIERE
!   On pourrait remonter la partie init non utilisateur de ppiniv avant lecamo
!     dans iniva0, mais il faudrait quand meme conserver ici l'appel a
!     ppiniv car il encapsule les appels aux ss pgm utilisateur similaires a
!     usiniv.
if (ippmod(iphpar).ge.1) then

  iusini = 1

  call ppiniv                                                     &
  !==========
 ( nvar   , nscal  ,                                              &
   dt     , rtp    , propce , propfa , propfb , coefa  , coefb  )
endif

! Si l'utilisateur a change Ptot, on change P* en consequence,
! sinon on met Ptot a P0 + rho.g.r
! A priori l'utilisateur remplira les NCEL valeurs ou rien du
!  tout, mais on ne sait jamais ...
! En compressible, Ptot n'est pas defini (correspond directement a RTP(.,IPR)

if  (ippmod(icompf).lt.0) then
  iiptot = ipproc(iprtot)
  xxp0   = xyzp0(1)
  xyp0   = xyzp0(2)
  xzp0   = xyzp0(3)
  do iel = 1, ncel
    if (propce(iel,iiptot).gt.-0.5d0*rinfin) then
      rtp(iel,ipr) = propce(iel,iiptot)                      &
           - ro0*( gx*(xyzcen(1,iel)-xxp0)                   &
           + gy*(xyzcen(2,iel)-xyp0)                   &
           + gz*(xyzcen(3,iel)-xzp0) )                 &
           + pred0 - p0
    else
      propce(iel,iiptot) = rtp(iel,ipr)                      &
           + ro0*( gx*(xyzcen(1,iel)-xxp0)                   &
           + gy*(xyzcen(2,iel)-xyp0)                   &
           + gz*(xyzcen(3,iel)-xzp0) )                 &
           + p0 - pred0
    endif
  enddo
endif



!===============================================================================
! 3.  CLIPPING DES GRANDEURS TURBULENTES (UTILISATEUR OU SUITE)
!     (pour ITYTUR=2, 3, 5 ou 6)
!     Si l'utilisateur est intervenu dans USINIV, PPINIV ou via l'interface
!         et a impose des valeurs "correctes" (au sens k, eps, Rii > 0)
!         on considere qu'il s'agit d'une initialisation admissible,
!         on la clippe pour la rendre coherente avec le clipping du code
!         et on continue le calcul
!     Si l'utilisateur est intervenu dans USINIV, PPINIV ou via l'interface
!         et a impose des valeurs visiblement erronees
!         (k, eps ou Rii < 0), on s'arrete (il s'est sans doute trompe).
!     On adopte le meme traitement en suite de calcul
!       pour assurer un comportement identique en suite entre un calcul
!       ou l'utilisateur modifie une variable avec usiniv (mais pas la
!       turbulence) et un calcul ou l'utilisateur ne modifie pas usiniv.
!     S'il n'y a ni suite ni intervention dans USINIV ou PPINIV ou via l'interface,
!       les grandeurs ont deja ete clippees par defaut, sauf si UREF n'a pas
!       (ou a mal) ete initialise. Dans ce cas on avertit aussi l'utilisateur et on
!       stoppe le calcul.

!     Pour resumer :
!      -en   suite  avec des valeurs positives pour k, eps, Rii : on clippe
!      -avec usiniv ou ppiniv ou interface
!                   avec des valeurs positives pour k, eps, Rii : on clippe
!      -non suite sans usiniv ni ppiniv ni interface avec UREF positif :
!                                      grandeurs par defaut (deja clippees)
!      -non suite sans usiniv ni ppiniv ni interface avec UREF negatif : stop
!      -suite ou usiniv ou ppiniv ou interface
!                   avec une valeur negative de k, eps ou Rii : stop
!                   avec une valeur hors de [0;2] pour phi : stop
!         (on souhaite indiquer a l'utilisateur que son fichier suite est
!          bizarre ou que son initialisation est fausse et qu'il a donc
!          fait au moins une erreur qui peut en cacher d'autres)
!===============================================================================

if(iusini.eq.1.or.isuite.eq.1) then

  if(itytur.eq.2 .or. itytur.eq.5) then

    xekmin = rtp(1,ik)
    xepmin = rtp(1,iep)
    do iel = 1, ncel
      xekmin = min(xekmin,rtp(iel,ik) )
      xepmin = min(xepmin,rtp(iel,iep))
    enddo
    if (irangp.ge.0) then
      call parmin (xekmin)
      !==========
      call parmin (xepmin)
      !==========
    endif

    if(xekmin.ge.0.d0.and.xepmin.ge.0.d0) then
      iclip = 1
      call clipke( ncelet , ncel   , nvar   ,          &
      !==========
                   iclip  , iwarni(ik) ,            &
                   propce , rtp    )
    else
      write(nfecra,3020) xekmin,xepmin
      iok = iok + 1
    endif

    !     En v2-f, phi-fbar ou BL-v2/k, on verifie aussi que phi est
    !     compris entre 0 et 2
    if (itytur.eq.5) then

      xphmin = rtp(1,iphi)
      xphmax = rtp(1,iphi)
      do iel = 1, ncel
        xphmin = min(xphmin,rtp(iel,iphi) )
        xphmax = max(xphmax,rtp(iel,iphi) )
      enddo
      if (irangp.ge.0) then
        call parmin (xphmin)
        !==========
        call parmax (xphmax)
        !==========
      endif

      !     Par coherence avec clpv2f, on ne clippe qu'a zero et pas a 2
      !              IF(XPHMIN.LT.0.D0 .OR. XPHMAX.GT.2.D0) THEN
      if(xphmin.lt.0.d0) then
        write(nfecra,3021) xphmin,xphmax
        iok = iok + 1
      endif

      !     En v2-f, BL-v2/k, on verifie aussi que alpha est
      !     compris entre 0 et 1
      if (iturb.eq.51) then
        xalmin = rtp(1,ial)
        xalmax = rtp(1,ial)
        do iel = 1, ncel
          xalmin = min(xalmin,rtp(iel,ial) )
          xalmax = max(xalmax,rtp(iel,ial) )
        enddo
        if (irangp.ge.0) then
          call parmin (xalmin)
          !==========
          call parmax (xalmax)
          !==========
        endif

        if(xalmin.lt.0.d0 .or. xalmax.gt.1.d0) then
          write(nfecra,3022) xalmin,xalmax
          iok = iok + 1
        endif

      endif

    endif

  elseif(itytur.eq.3) then

    x11min = rtp(1,ir11)
    x22min = rtp(1,ir22)
    x33min = rtp(1,ir33)
    xepmin = rtp(1,iep)
    do iel = 1, ncel
      x11min = min(x11min,rtp(iel,ir11))
      x22min = min(x22min,rtp(iel,ir22))
      x33min = min(x33min,rtp(iel,ir33))
      xepmin = min(xepmin,rtp(iel,iep) )
    enddo
    if (irangp.ge.0) then
      call parmin (x11min)
      !==========
      call parmin (x22min)
      !==========
      call parmin (x33min)
      !==========
      call parmin (xepmin)
      !==========
    endif
    if (x11min.ge.0.d0.and.x22min.ge.0.d0.and.                  &
         x33min.ge.0.d0.and.xepmin.ge.0.d0 ) then
      iclip = 1
      call clprij( ncelet , ncel   , nvar   ,          &
      !==========
                   iclip  ,                            &
                   propce , rtp    , rtp    )
    else
      write(nfecra,3030) x11min,x22min,x33min,xepmin
      iok = iok + 1
    endif

  elseif(iturb.eq.60) then

    xekmin = rtp(1,ik )
    xomgmn = rtp(1,iomg)
    do iel = 1, ncel
      xekmin = min(xekmin,rtp(iel,ik ))
      xomgmn = min(xomgmn,rtp(iel,iomg))
    enddo
    if (irangp.ge.0) then
      call parmin (xekmin)
      !==========
      call parmin (xomgmn)
      !==========
    endif

    !     En k-omega on clippe seulement a 0
    if(xekmin.lt.0.d0 .or. xomgmn.lt.0.d0) then
      write(nfecra,3031) xekmin,xomgmn
      iok = iok + 1
    endif

  elseif(iturb.eq.70) then

    xnumin = rtp(1,inusa)
    do iel = 1, ncel
      xnumin = min(xnumin,rtp(iel,inusa))
    enddo
    if (irangp.ge.0) then
      call parmin (xnumin)
      !==========
    endif

    !     En Spalart-Allmaras on clippe seulement a 0
    if(xnumin.lt.0.d0 ) then
      write(nfecra,3032) xnumin
      iok = iok + 1
    endif

  endif

else

  if (iturb.ne.0 .and. iturb.ne.10                &
       .and. itytur.ne.4) then
    if (uref.lt.0.d0) then
      write(nfecra,3039) uref
      iok = iok + 1
    endif
  endif

endif

!===============================================================================
! 4.  CLIPPING DES SCALAIRES (UTILISATEUR OU SUITE)
!     Si l'utilisateur est intervenu dans USINIV ou PPINIV et
!       a impose des valeurs "correctes" (au sens comprises dans des bornes
!         simplifiees a base de 0, scamin, scamax)
!         on considere qu'il s'agit d'une initialisation admissible,
!         on la clippe pour la rendre coherente avec le clipping du code
!         et on continue le calcul
!       si l'utilisateur a impose des valeurs visiblement erronees
!         (au sens comprises dans des bornes simplifiees a base de 0, scamin,
!          scamax), on s'arrete (il s'est sans doute trompe).
!     On adopte le meme traitement en suite de calcul
!       pour assurer un comportement identique en suite entre un calcul
!       ou l'utilisateur modifie une variable avec usiniv (mais pas un
!       scalaire) et un calcul ou l'utilisateur ne modifie pas usiniv.
!     Sinon, les grandeurs ont deja ete clippees apres les init par defaut

!     Pour resumer :
!      -en   suite  avec des valeurs grossierement admissibles : on clippe
!      -avec usiniv ou ppiniv
!                   avec des valeurs grossierement admissibles : on clippe
!      -non suite sans usiniv ni ppiniv :
!                                      grandeurs par defaut (deja clippees)
!      -suite ou usiniv ou ppiniv
!                   avec une valeur grossierement non admissible : stop
!         (on souhaite indiquer a l'utilisateur que son fichier suite est
!          bizarre ou que son initialisation est fausse et qu'il a donc
!          fait au moins une erreur qui peut en cacher d'autres)
!===============================================================================

! On traite tous les scalaires d'abord, car ils peuvent etre necessaires
!     pour clipper les variances

if(nscal.gt.0.and.(iusini.eq.1.or.isuite.eq.1)) then

!     Scalaires non variance

  do ii = 1, nscal
    if(iscavr(ii).le.0.or.iscavr(ii).gt.nscal) then

      if(scamin(ii).le.scamax(ii)) then
        ivar = isca(ii)
        valmax = rtp(1  ,ivar)
        valmin = rtp(1  ,ivar)
        do iel = 1, ncel
          valmax = max(valmax,rtp(iel,ivar))
          valmin = min(valmin,rtp(iel,ivar))
        enddo
        if (irangp.ge.0) then
          call parmax (valmax)
          !==========
          call parmin (valmin)
          !==========
        endif

!     Verification de la coherence pour les clippings
!                                           des scalaires non variance.
        if(valmin.ge.scamin(ii).and.valmax.le.scamax(ii)) then
          iscal = ii
          call clpsca                                             &
          !==========
                ( ncelet , ncel   , nvar   , nscal  , iscal  ,    &
                  propce , rvoid  , rtp    )
        else
          chaine = nomvar(ipprtp(isca(ii)))
          write(nfecra,3040) ii,chaine(1:8),                      &
                             valmin,scamin(ii),valmax,scamax(ii)
          iok = iok + 1
        endif
      endif

    endif
  enddo


!     Variances

  do ii = 1, nscal
    if(iscavr(ii).gt.0.and.iscavr(ii).le.nscal) then

      if(scamin(ii).le.scamax(ii)) then
        ivar = isca(ii)
        valmax = rtp(1  ,ivar)
        valmin = rtp(1  ,ivar)
        do iel = 1, ncel
          valmax = max(valmax,rtp(iel,ivar))
          valmin = min(valmin,rtp(iel,ivar))
        enddo
        if (irangp.ge.0) then
          call parmax (valmax)
          !==========
          call parmin (valmin)
          !==========
        endif

!     Verification de la coherence pour les clippings de variance.
!     Pour iclvfl = 1 on ne verifie que > 0 sinon ca va devenir difficile
!     de faire une initialisation correcte.

        if(iclvfl(ii).eq.0) then
!       On pourrait clipper dans le cas ou VALMIN.GE.0, mais ca
!       n'apporterait rien, par definition
          if(valmin.lt.0.d0) then
            chaine = nomvar(ipprtp(isca(ii)))
            write(nfecra,3050)ii,chaine(1:8),                     &
                              valmin,scamin(ii),valmax,scamax(ii)
            iok = iok + 1
          endif
        elseif(iclvfl(ii).eq.1) then
! Ici on clippe pour etre coherent avec la valeur du scalaire
          if(valmin.ge.0.d0) then
            iscal = ii
            call clpsca                                           &
            !==========
            ( ncelet , ncel   , nvar   , nscal  , iscal  ,        &
              propce , rtp(1,isca(iscavr(ii))) , rtp      )
          else
            chaine = nomvar(ipprtp(isca(ii)))
            write(nfecra,3050)ii,chaine(1:8),                     &
                              valmin,scamin(ii),valmax,scamax(ii)
            iok = iok + 1
          endif
        elseif(iclvfl(ii).eq.2) then
          vfmin = 0.d0
          vfmin = max(scamin(iscal),vfmin)
          vfmax = scamax(iscal)
! On pourrait clipper dans le cas ou VALMIN.GE.VFMIN.AND.VALMAX.LE.VFMAX
!     mais ca n'apporterait rien, par definition
          if(valmin.lt.vfmin.or.valmax.gt.vfmax) then
            chaine = nomvar(ipprtp(isca(ii)))
            write(nfecra,3051)ii,chaine(1:8),                     &
                              valmin,scamin(ii),valmax,scamax(ii),&
                              ii,iclvfl(ii)
            iok = iok + 1
          endif
        endif
      endif

    endif
  enddo

endif


!===============================================================================
! 6.  IMPRESSIONS DE CONTROLE POUR LES INCONNUES, LE PAS DE TEMPS
!        LE CUMUL DES DUREE POUR LES MOYENNES
!===============================================================================

write(nfecra,2000)

!     Inconnues de calcul : on affiche les bornes
do ipp  = 2, nvppmx
  if(itrsvr(ipp ).ge.1) then
    ivar = itrsvr(ipp )
    valmax = -grand
    valmin =  grand
    do iel = 1, ncel
      valmax = max(valmax,rtp(iel,ivar))
      valmin = min(valmin,rtp(iel,ivar))
    enddo
    if (irangp.ge.0) then
      call parmax (valmax)
      !==========
      call parmin (valmin)
      !==========
    endif
    chaine = nomvar(ipp )
    write(nfecra,2010)chaine(1:8),valmin,valmax
  endif
enddo
write(nfecra,2020)

!     Moyennes  : on affiche les bornes
if(nbmomt.gt.0) then
  do imom = 1, nbmomt
    ipcmom = ipproc(icmome(imom))

!       Si on ne (re)initialise pas
    if(imoold(imom).ne.-1) then
      valmax = -grand
      valmin =  grand
!         Si le cumul en temps est variable en espace
      if(idtmom(imom).gt.0) then
        idtcm  = ipproc(icdtmo(idtmom(imom)))
        do iel = 1, ncel
          valmom = propce(iel,ipcmom)/                            &
                     max(propce(iel,idtcm),epzero)
          valmax = max(valmax,valmom)
          valmin = min(valmin,valmom)
        enddo
!         Si le cumul en temps est uniforme
      else
        idtcm  =-idtmom(imom)
        do iel = 1, ncel
          valmom = propce(iel,ipcmom)/                            &
                     max(dtcmom(idtcm),epzero)
          valmax = max(valmax,valmom)
          valmin = min(valmin,valmom)
        enddo
      endif
      if (irangp.ge.0) then
        call parmax (valmax)
        !==========
        call parmin (valmin)
        !==========
      endif
!       Si on  (re)initialise
    else
      valmax = 0.d0
      valmin = 0.d0
    endif

    chaine = nomvar(ipppro(ipcmom))
    write(nfecra,2010)chaine(1:8),valmin,valmax

  enddo
  write(nfecra,2020)
endif

if (idtvar.ge.0) then
!     Pas de temps : on affiche les bornes
!                    si < 0 on s'arrete
  vdtmax = -grand
  vdtmin =  grand
  do iel = 1, ncel
    vdtmax = max(vdtmax,dt    (iel))
    vdtmin = min(vdtmin,dt    (iel))
  enddo
  if (irangp.ge.0) then
    call parmax (vdtmax)
    !==========
    call parmin (vdtmin)
    !==========
  endif
  write(nfecra,2010) nomvar(ippdt), vdtmin, vdtmax
  write(nfecra,2020)

  if (vdtmin.le.zero) then
    write(nfecra,3010) vdtmin
    iok = iok + 1
  endif

endif

!     Cumul du temps associe aux moments : on affiche les bornes
!                                          si < 0 on s'arrete

if(nbmomt.gt.0) then

!     Indicateur de calcul des bornes pour les cumuls non uniformes
  do imom = 1, nbmomt
    if(idtmom(imom).gt.0) then
      ibormo(icdtmo(idtmom(imom))) =  0
      vmomax(icdtmo(idtmom(imom))) = -grand
      vmomin(icdtmo(idtmom(imom))) =  grand
    endif
  enddo

!     Calcul des bornes des cumuls non uniformes
  do imom = 1, nbmomt
    if(idtmom(imom).gt.0) then
      if(ibormo(icdtmo(idtmom(imom))).eq.0) then
        idtcm  = ipproc(icdtmo(idtmom(imom)))
        vdtmax = -grand
        vdtmin =  grand
        do iel = 1, ncel
          vdtmax = max(vdtmax,propce(iel,idtcm))
          vdtmin = min(vdtmin,propce(iel,idtcm))
        enddo
        if (irangp.ge.0) then
          call parmax (vdtmax)
          !==========
          call parmin (vdtmin)
          !==========
        endif
        vmomax(icdtmo(idtmom(imom))) = vdtmax
        vmomin(icdtmo(idtmom(imom))) = vdtmin
        ibormo(icdtmo(idtmom(imom))) = 1
      endif
    endif
  enddo

!     Impression des bornes
  write(nfecra,2030)
  do imom = 1, nbmomt
    if(idtmom(imom).gt.0) then
      write(nfecra,2040) imom,vmomin(icdtmo(idtmom(imom))),       &
                              vmomax(icdtmo(idtmom(imom))),       &
                              'Variable'
    elseif(idtmom(imom).lt.0) then
#if defined(_CS_LANG_FR)
      write(nfecra,2040) imom,dtcmom(-idtmom(imom))       ,       &
                              dtcmom(-idtmom(imom))       ,       &
                              'Uniforme'
#else
      write(nfecra,2040) imom,dtcmom(-idtmom(imom))       ,       &
                              dtcmom(-idtmom(imom))       ,       &
                              'Uniform'
#endif
    endif
  enddo
  write(nfecra,2050)

!     On s'arrete si des cumuls sont negatifs
  do imom = 1, nbmomt
    if(idtmom(imom).gt.0) then
      if(vmomin(icdtmo(idtmom(imom))).lt.zero) then
        write(nfecra,3011) imom,vmomin(icdtmo(idtmom(imom)))
        iok = iok + 1
      endif
    elseif(idtmom(imom).lt.0) then
      if(dtcmom(-idtmom(imom)).lt.zero) then
        write(nfecra,3011) imom,dtcmom(-idtmom(imom))
        iok = iok + 1
      endif
    endif
  enddo

endif

!===============================================================================
! 7.  ARRET GENERAL SI PB
!===============================================================================

if (iok.gt.0) then
  write(nfecra,3090) iok
  call csexit (1)
endif

write(nfecra,3000)

!----
! FORMATS
!----


#if defined(_CS_LANG_FR)

 2000 format(                                                           &
'                                                             ',/,&
' ----------------------------------------------------------- ',/,&
'                                                             ',/,&
'                                                             ',/,&
' ** INITIALISATION DES VARIABLES                             ',/,&
'    ----------------------------                             ',/,&
'                                                             ',/,&
' ---------------------------------                           ',/,&
'  Variable  Valeur min  Valeur max                           ',/,&
' ---------------------------------                           '  )
 2010 format(                                                           &
 2x,     a8,      e12.4,      e12.4                              )
 2020 format(                                                           &
' ---------------------------------                           ',/)
 2030 format(                                                           &
' Duree cumulee :                                             ',/,&
' ------------------------------------------------------------',/,&
'   Moyenne  Valeur min  Valeur max Uniforme/Variable en espac',/,&
' ------------------------------------------------------------'  )
 2040 format(                                                           &
        i10,      e12.4,      e12.4,1x,   a8                     )
 2050 format(                                                           &
' ------------------------------------------------------------',/)

 3000 format(/,/,                                                 &
'-------------------------------------------------------------',/)
 3010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@    PAS DE TEMPS NEGATIF OU NUL                             ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  La valeur minimale du pas de temps DT est ',E14.5          ,/,&
'@  Verifier l''initialisation dans usiniv ou le fichier suite',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut les modifier par usiniv       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3011 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@    CUMUL DE DUREE POUR LES MOYENNES NEGATIVE               ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  La valeur minimale de la duree cumulee pour la moyenne    ',/,&
'@    IMOM = ',I10   ,' est ',E14.5                            ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation dans usiniv ou le fichier suite',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut reinitialiser la moyenne et le',/,&
'@    cumul temporel associe en imposant IMOOLD(IMOM) = -1    ',/,&
'@    dans usini1.                                            ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3020 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     TURBULENCE NEGATIVE OU NULLE                           ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de k       = ',E14.5                      ,/,&
'@   Valeur minimale de epsilon = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    le fichier suite ou bien la valeur de UREF (usini1      ',/,&
'@    et/ou interface).                                       ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface).                                      ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3021 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     VARIABLE PHI DU V2F (PHI_FBAR ou BL-V2/K)              ',/,&
'@     HORS DES BORNES [0;2]                                  ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de phi     = ',E14.5                      ,/,&
'@   Valeur maximale de phi     = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    ou le fichier suite.                                    ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface.                                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3022 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     VARIABLE ALPHA DU V2F (BL-V2/K)                        ',/,&
'@     HORS DES BORNES [0;1]                                  ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de alpha   = ',E14.5                      ,/,&
'@   Valeur maximale de alpha   = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    ou le fichier suite.                                    ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface.                                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3030 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     TURBULENCE NEGATIVE OU NULLE                           ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de R11     = ',E14.5                      ,/,&
'@   Valeur minimale de R22     = ',E14.5                      ,/,&
'@   Valeur minimale de R33     = ',E14.5                      ,/,&
'@   Valeur minimale de epsilon = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    le fichier suite ou bien la valeur de UREF (usini1      ',/,&
'@    et/ou interface).                                       ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface).                                      ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3031 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@    TURBULENCE NEGATIVE OU NULLE                            ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de k       = ',E14.5                      ,/,&
'@   Valeur minimale de omega   = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    le fichier suite ou bien la valeur de UREF (usini1      ',/,&
'@    et/ou interface).                                       ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface.                                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3032 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@    TURBULENCE NEGATIVE OU NULLE                            ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@   Valeur minimale de nu      = ',E14.5                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation (usiniv et/ou interface),      ',/,&
'@    le fichier suite ou bien la valeur de UREF (usini1      ',/,&
'@    et/ou interface).                                       ',/,&
'@  Dans le cas ou les valeurs lues dans le fichier suite     ',/,&
'@    sont incorrectes, on peut les modifier par usiniv ou    ',/,&
'@    par l''interface.                                       ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3039 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@    LA VITESSE DE REFERENCE UREF N''A PAS ETE INITIALISEE   ',/,&
'@    OU A ETE MAL INITIALISEE (VALEUR NEGATIVE).             ',/,&
'@    ELLE VAUT ICI ',E14.5                                    ,/,&
'@                                                            ',/,&
'@  La turbulence n''a pas pu etre initialisee                ',/,&
'@  Corriger la valeur de UREF (usini1 ou interface)ou bien   ',/,&
'@    initialiser directement la turbulence dans la routine   ',/,&
'@    (usiniv ou interface).                                  ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3040 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     GRANDEUR SCALAIRE HORS BORNES                          ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Scalaire numero ',I10,' : ',A8                             ,/,&
'@  Valeur minimale             = ',E14.5                      ,/,&
'@    Clipping demande a SCAMIN = ',E14.5                      ,/,&
'@  Valeur maximale             = ',E14.5                      ,/,&
'@    Clipping demande a SCAMAX = ',E14.5                      ,/,&
'@  Les valeurs extremes ne sont pas coherentes avec les      ',/,&
'@    limites SCAMIN et SCAMAX imposees dans usini1.          ',/,&
'@                                                            ',/,&
'@  Verifier l''initialisation dans usiniv ou le fichier suite',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut les modifier par usiniv       ',/,&
'@  Verifier les valeurs de clipping dans usini1.             ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3050 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     VARIANCE NEGATIVE                                      ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Scalaire numero ',I10,' : ',A8                             ,/,&
'@  Valeur minimale             = ',E14.5                      ,/,&
'@  Le scalaire indique ci-dessus est une variance (ISCAVR est',/,&
'@    postif dans usini1) mais l initialisation imposee       ',/,&
'@    dans usiniv comporte des valeurs negatives.             ',/,&
'@                                                            ',/,&
'@  Verifier l''initialisation dans usiniv ou le fichier suite',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut les modifier par usiniv       ',/,&
'@  Verifier la definition des variances dans usini1.         ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3051 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@     VARIANCE HORS BORNES                                   ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Scalaire numero ',I10,' : ',A8                             ,/,&
'@  Valeur minimale             = ',E14.5                      ,/,&
'@    Clipping demande a SCAMIN = ',E14.5                      ,/,&
'@  Valeur maximale             = ',E14.5                      ,/,&
'@    Clipping demande a SCAMAX = ',E14.5                      ,/,&
'@  Le scalaire indique ci-dessus est une variance (ISCAVR est',/,&
'@    postif dans usini1) mais l initialisation imposee       ',/,&
'@    dans usiniv comporte des valeurs situees hors des bornes',/,&
'@    SCAMIN, SCAMAX ou inferieures a 0 et le mode de clipping',/,&
'@    demande est ICLVFL(',I10,') = ',I10                      ,/,&
'@                                                            ',/,&
'@  Verifier l''initialisation dans usiniv ou le fichier suite',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut les modifier par usiniv       ',/,&
'@  Verifier la definition des variances et le mode de        ',/,&
'@    clipping demande dans usini1.                           ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3090 format(                                                           &
'@                                                            ',/,&
'@                                                            ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''INITIALISATION DES VARIABLES     ',/,&
'@    =========                                               ',/,&
'@                                                            ',/,&
'@    L INITIALISATION DES VARIABLES EST INCOMPLETE OU        ',/,&
'@      INCOHERENTE AVEC LES VALEURS DES PARAMETRES DE CALCUL ',/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute (',I10,' erreurs).          ',/,&
'@                                                            ',/,&
'@  Se reporter aux impressions precedentes pour plus de      ',/,&
'@    renseignements.                                         ',/,&
'@  Attention a l''initialisation du pas de temps             ',/,&
'@                                de la turbulence            ',/,&
'@                                des scalaires et variances  ',/,&
'@                                des moyennes temporelles    ',/,&
'@                                                            ',/,&
'@  Verifier usiniv ou le fichier suite.                      ',/,&
'@    dans le cas ou les valeurs lues dans le fichier suite   ',/,&
'@    sont incorrectes, on peut les modifier par usiniv       ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

#else

 2000 format(                                                           &
'                                                             ',/,&
' ----------------------------------------------------------- ',/,&
'                                                             ',/,&
'                                                             ',/,&
' ** VARIABLES INITIALIZATION                                 ',/,&
'    ------------------------                                 ',/,&
'                                                             ',/,&
' ---------------------------------                           ',/,&
'  Variable  Min. value  Max. value                           ',/,&
' ---------------------------------                           '  )
 2010 format(                                                           &
 2x,     a8,      e12.4,      e12.4                              )
 2020 format(                                                           &
' ---------------------------------                           ',/)
 2030 format(                                                           &
' Time averages (sum over the time-steps)                     ',/,&
' ------------------------------------------------------------',/,&
'   Average  Min. value  Max. value Uniform/Variable in space ',/,&
' ------------------------------------------------------------'  )
 2040 format(                                                           &
        i10,      e12.4,      e12.4,1x,   a8                     )
 2050 format(                                                           &
' ------------------------------------------------------------',/)

 3000 format(/,/,                                                 &
'-------------------------------------------------------------',/)
 3010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@    NEGATIVE OR NULL TIME STEP                              ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@  The minimum value of the time-step dt is ',E14.5           ,/,&
'@  Verify the initialization in usiniv or the restart file   ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, they may be modified with usiniv         ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3011 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@    NEGATIVE CUMULATIVE TIME FOR THE MOMENTS                ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@  The minimum value of the cumulative time for the moment   ',/,&
'@    IMOM = ',I10   ,' est ',E14.5                            ,/,&
'@                                                            ',/,&
'@  Verify the initialization in usiniv or the restart file   ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, the moment and the associated cumulative ',/,&
'@    time may be re-initialized by setting IMOOLD(IMOM) = -1 ',/,&
'@    in usini1.                                              ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3020 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     NEGATIVE OR NULL TURBULENCE                            ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of k       = ',E14.5                        ,/,&
'@   Minimum value of epsilon = ',E14.5                        ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file or the value of UREF (usini1 and/or    ',/,&
'@    interface).                                             ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3021 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     PHI VARIABLE OF V2F (PHI_FBAR or BL-V2/K)              ',/,&
'@     OUT OF BOUNDS [0;2]                                    ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of phi = ',E14.5                            ,/,&
'@   Maximum value of phi = ',E14.5                            ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file.                                       ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3022 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     ALPHA VARIABLE OF V2F (BL-V2/K)                        ',/,&
'@     OUT OF BOUNDS [0;1]                                    ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of alpha = ',E14.5                          ,/,&
'@   Maximum value of alpha = ',E14.5                          ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file.                                       ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3030 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     NEGATIVE OR NULL TURBULENCE                            ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of R11     = ',E14.5                        ,/,&
'@   Minimum value of R22     = ',E14.5                        ,/,&
'@   Minimum value of R33     = ',E14.5                        ,/,&
'@   Minimum value of epsilon = ',E14.5                        ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file or the value of UREF (usini1 and/or    ',/,&
'@    interface).                                             ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3031 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     NEGATIVE OR NULL TURBULENCE                            ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of k       = ',E14.5                        ,/,&
'@   Minimum value of omega   = ',E14.5                        ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file or the value of UREF (usini1 and/or    ',/,&
'@    interface).                                             ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3032 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     NEGATIVE OR NULL TURBULENCE                            ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@   Minimum value of nu      = ',E14.5                        ,/,&
'@                                                            ',/,&
'@  Verify the initialization (usiniv and/or interface),      ',/,&
'@    the restart file or the value of UREF (usini1 and/or    ',/,&
'@    interface).                                             ',/,&
'@  In the case where the values read in the restart file     ',/,&
'@    are incorrect, they may be modified with usiniv or      ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3039 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@    THE REFERENCE VELOCITY UREF HAS NOT BEEN INITIALIZED    ',/,&
'@    OR HAS NOT BEEN CORRECTLY INITIALIZED (NEGATIVE VALUE)  ',/,&
'@    ITS VALUE IS ',E14.5                                     ,/,&
'@                                                            ',/,&
'@  The turbulence cannot be initialized                      ',/,&
'@  Correct the value of UREF (usini1 or interface) or        ',/,&
'@    initialize directly the turbulence with usiniv or       ',/,&
'@    with the interface.                                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3040 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     SCALAR QUANTITIES OUT OF BOUNDS                        ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@  Scalar number ',I10,': ',A8                                ,/,&
'@  Minimum value                = ',E14.5                     ,/,&
'@    Desired clipping at SCAMIN = ',E14.5                     ,/,&
'@  Maximum value                = ',E14.5                     ,/,&
'@    Desired clipping at SCAMAX = ',E14.5                     ,/,&
'@  The bounds are not coherent with the limits SCAMIN and    ',/,&
'@    SCAMAX set in usini1.                                   ',/,&
'@                                                            ',/,&
'@  Verify the initialization in usiniv or the restart file   ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, they may be modified with usiniv         ',/,&
'@  Verify the clipping values in usini1.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3050 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     NEGATIVE VARIANCE                                      ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@  Scalar number ',I10,': ',A8                                ,/,&
'@  Minimum value               = ',E14.5                      ,/,&
'@  This scalar is a variance (ISCAVR is positive in usini1)  ',/,&
'@    but the initialization in usiniv has some negative      ',/,&
'@    values.                                                 ',/,&
'@                                                            ',/,&
'@  Verify the initialization in usiniv or the restart file   ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, they may be modified with usiniv         ',/,&
'@  Verify the variance definition in usini1.                 ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3051 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@     VARIANCE OUT OF BOUNDS                                 ',/,&
'@                                                            ',/,&
'@  The calculation will not be run.                          ',/,&
'@                                                            ',/,&
'@  Scalar number ',I10,': ',A8                                ,/,&
'@  Minimum value                = ',E14.5                     ,/,&
'@    Desired clipping at SCAMIN = ',E14.5                     ,/,&
'@  Maximum value                = ',E14.5                     ,/,&
'@    Desired clipping at SCAMAX = ',E14.5                     ,/,&
'@  This scalar is a variance (ISCAVR is positive in usini1)  ',/,&
'@    but the initialization in usiniv has some values out    ',/,&
'@    of the bounds SCAMIN, SCAMAX or lower than 0 and the    ',/,&
'@    desired clipping mode is ICLVFL(',I10,') = ',I10         ,/,&
'@                                                            ',/,&
'@  Verify the initialization in usiniv or the restart file   ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, they may be modified with usiniv         ',/,&
'@  Verify the variance definition and the clipping mode in   ',/,&
'@    usini1.                                                 ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 3090 format(                                                           &
'@                                                            ',/,&
'@                                                            ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN THE VARIABLES INITIALIZATION          ',/,&
'@    ========                                                ',/,&
'@                                                            ',/,&
'@    THE VARIABLES INITIALIZATION IS INCOMPLETE OR           ',/,&
'@    INCOHERENT WITH THE PARAMETERS VALUE OF THE CALCULATION ',/,&
'@                                                            ',/,&
'@  The calculation will not be run (',I10,' errors).         ',/,&
'@                                                            ',/,&
'@  Refer to the previous warnings for further information.   ',/,&
'@  Pay attention to the initialization of                    ',/,&
'@                                the time-step               ',/,&
'@                                the turbulence              ',/,&
'@                                the scalars and variances   ',/,&
'@                                the time averages           ',/,&
'@                                                            ',/,&
'@  Verify usiniv or the restart file.                        ',/,&
'@    In the case where the values read in the restart file   ',/,&
'@    are incorrect, they may be modified with usiniv         ',/,&
'@  Verify usini1.                                            ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

#endif

!----
! FIN
!----

return
end subroutine
