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

subroutine lagcel &
!================

 ( lndnod ,                                                       &
   nvar   , nscal  ,                                              &
   nbpmax , nvp    , nvp1   , nvep   , nivep  ,                   &
   ntersl , nvlsta , nvisbr ,                                     &
   itypfb , itrifb , icocel , itycel , ifrlag , itepa  , ibord  , &
   indep  ,                                                       &
   dlgeo  ,                                                       &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
   ettp   , ettpa  , tepa   , parbor , auxl   )

!===============================================================================
! FONCTION :
! ----------

!   SOUS-PROGRAMME DU MODULE LAGRANGIEN :
!   -------------------------------------

!   Trajectographie des particules : le propos de ce sous-programme
!   est de donner le numero de la cellule d'arrive d'une particule
!   connaissant les coordonnees du point de depart, celles
!   du point d'arrive, ainsi que le numero de la cellule de depart.

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! lndnod           ! e  ! <-- ! dim. connectivite cellules->faces              !
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
! itypfb(nfabor)   ! ia ! <-- ! boundary face types                            !
! itrifb(nfabor)   ! te ! --> ! tab d'indirection pour tri des faces           !
! icocel           ! te ! --> ! connectivite cellules -> faces                 !
!   (lndnod)       !    !     !    face de bord si numero negatif              !
! itycel           ! te ! --> ! connectivite cellules -> faces                 !
!   (ncelet+1)     !    !     !                                                !
! ifrlag           ! te ! <-- ! numero de zone de la face de bord              !
!   (nfabor)       !    !     !  pour le module lagrangien                     !
! itepa            ! te ! --> ! info particulaires (entiers)                   !
! (nbpmax,nivep    !    !     !   (cellule de la particule,...)                !
! ibord            ! te ! --> ! si nordre=2, contient le numero de la          !
!   (nbpmax)       !    !     !   face d'interaction part/frontiere            !
! indep            ! te ! --> ! pour chaque particule :                        !
!   (nbpmax)       !    !     !   numero de la cellule de depart               !
! dlgeo            ! tr ! --> ! tableau contenant les donnees geometriques     !
! (nfabor,ngeol)   !    !     ! pour le sous-modele de depot                   !
! dt(ncelet)       ! ra ! <-- ! time step (per cell)                           !
! rtp, rtpa        ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules (instant courant et prec)          !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
! propfa(nfac, *)  ! ra ! <-- ! physical properties at interior face centers   !
! propfb(nfabor, *)! ra ! <-- ! physical properties at boundary face centers   !
! coefa, coefb     ! ra ! <-- ! boundary conditions                            !
!  (nfabor, *)     !    !     !                                                !
! ettp             ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape courante                !
! ettpa            ! tr ! <-- ! tableaux des variables liees                   !
!  (nbpmax,nvp)    !    !     !   aux particules etape precedente              !
! tepa             ! tr ! <-- ! info particulaires (reels)                     !
! (nbpmax,nvep)    !    !     !   (poids statistiques,...)                     !
! parbor(nfabor    ! tr ! <-- ! cumul des statistiques aux frontieres          !
!    nvisbr)       !    !     !                                                !
! auxl(nbpmax,3    ! tr ! --- ! tableau de travail                             !
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
use entsor
use cstphy
use parall
use period
use lagpar
use lagran
use mesh

!===============================================================================

implicit none

! Arguments

integer          lndnod
integer          nvar   , nscal
integer          nbpmax , nvp    , nvp1   , nvep  , nivep
integer          ntersl , nvlsta , nvisbr

integer          itypfb(nfabor) , itrifb(nfabor)
integer          icocel(lndnod) , itycel(ncelet+1)
integer          ifrlag(nfabor) , itepa(nbpmax,nivep)
integer          ibord(nbpmax)
integer          indep(nbpmax)

double precision dt(ncelet) , rtp(ncelet,*) , rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*) , propfb(nfabor,*)
double precision coefa(nfabor,*) , coefb(nfabor,*)
double precision ettp(nbpmax,nvp) , ettpa(nbpmax,nvp)
double precision tepa(nbpmax,nvep)
double precision parbor(nfabor,nvisbr) , auxl(nbpmax,3)
double precision dlgeo(nfabor,ngeol)

! Local variables

integer          iel, ifac, kfac, nbp, icecpt
integer          ii, jj, in, ip
integer          indian, ifaold, ifanew
integer          isuivi, ierror, ierrie
integer          itypfo, iconfo(100)
integer          itepas, iper
integer          il,ltest,ifacb,ifacp,ielnew,izone,isens

double precision pta(3), ptb(3), vect(3), vectn(3)
double precision xp,yp,zp,xq,yq,zq,xpq,ypq,zpq,aa
double precision xk,yk,zk
double precision vpart(6),vvue(6)
double precision det, xn,yn,zn,xpp,ypp,zpp, xnor
double precision xt,yt,zt,xtt,ytt,ztt,xkp,ykp,zkp
double precision xxp,yyp,zzp,kk,deplx,deply,deplz
double precision distp
double precision ist,istt,dept,deptt

integer, allocatable, dimension(:) :: celcr, percr

!===============================================================================
! -1.  MACRO DE DEBUGGAGE DEVELOPPEUR
!===============================================================================

!            ATTENTION INTERVENTION DEVELOPPEUR UNIQUEMENT.

!     Si cette macro est vrai elle permet les impressions listing
!     de l'avance des particules (si IMPLTG = 1), et permet la sortie
!     des fichers Ensight de debuggage en cas de perte de particule
!     en mode sans erreur (si IERRIE = 1).
!     Lorsque cette macro est vrai, il est conseille d'avoir un NBPART
!     petit, sans quoi les temps de calcul seront enormes a cause
!     des ecritures disque du au fichier SCRATCH4.lag.

!     PAR DEFAUT : DEBUG_LAGCEL = 0


#define DEBUG_LAGCEL 0



#if DEBUG_LAGCEL
integer          impltg
integer          ipt    , kpt    , npt    , ipart  , ielold
integer          nquad4 , ntria3 , nsided
#endif

!===============================================================================

!===============================================================================
! 0.  GESTION MEMOIRE
!===============================================================================


!===============================================================================
! 1.  INITIALISATION
!===============================================================================

! Initialize variables to avoid compiler warnings

ifacp  = 0

do ip = 1,nbpmax
  ibord(ip) = 0
enddo

#if DEBUG_LAGCEL
!--> IMPLTG =1 ecriture de la progression des particules dans le listing
!    (utile si  DEBUG_LAGCEL = 1)

! mettre IMPLTG a 1 ici
impltg = 1

#endif


!--> Si IERRIE est non nul alors on se place en mode sans erreur de
!    trajectoire : il y a arret du calcul a la 1ere erreur sur le
!    reperage.

ierrie = 0

!--> La valeur de IERR bascule a 1 s'il y a une erreur et si IERRIE = 1

! ne pas modifier cette initialisation SVP
ierr = 0

nbperr = 0
dnbper = 0.d0


!--> Si on est en instationnaire, RAZ des statistiques aux frontieres

if (iensi3.eq.1) then

  if (isttio.eq.0 .or. (isttio.eq.1 .and. iplas.le.nstbor)) then
    tstatp = 0.d0
    npstf = 0
    do ii = 1,nvisbr
      do ifac = 1,nfabor
        parbor(ifac,ii) = 0.d0
      enddo
    enddo
  endif

  tstatp = tstatp + dtp
  npstf  = npstf  + 1
  npstft = npstft + 1

endif

if (iphyla.eq.2 .and. iencra.eq.1) then
  npencr = 0
  dnpenc = 0.d0
endif


! Traitement de la periodicite

if (iperio.eq.1) then

  ! Allocate temporary arrays
  allocate(celcr(ncelet-ncel), percr(ncelet-ncel))

  do iel = 1,ncelet-ncel
    celcr(iel) = 0
    percr(iel) = -1
  enddo

  call perloc(celcr, percr)
  !==========

endif

!===============================================================================
! 2. Reperage des particules dans le maillage
!===============================================================================

!--> Boucle principale : les particules sont traitees une par une

do ip = 1,nbpart

! utile car IDEPO2
  if (itepa(ip,jisor).gt.0) then

    ifanew = 0
    icecpt = 0

!--> Ouverture du fichier SCRATCH qui permettra l'ecriture des infos
!    en mode DEBUG

#if DEBUG_LAGCEL
    OPEN (IMPLA4,FILE='SCRATCH4.lag',                             &
          STATUS='UNKNOWN',FORM='UNFORMATTED',                    &
          ACCESS='SEQUENTIAL')
    nquad4 = 0
    ntria3 = 0
    nsided = 0
#endif

!--> Etiquette de retour pour le suivi de la particule dans la cellule voisine

 100      continue

#if DEBUG_LAGCEL
    if (impltg.eq.1) write(nfecra,9001) ip, itepa(ip,jisor)
#endif

!--> Attention :

! 1) IEL est cst jusqu'au GOTO 100, ITEPA(IP,JISOR) est modifie
!    jusqu'au GOTO 100

! 2) IFAOLD contient le numero de la derniere face traversee par la
!    trajectoire de la particule IP, si > 0 face interne,
!    si < 0 face de bord, IFAOLD est cst jusqu'au GOTO 100,
!    IFANEW est modifie jusqu'au GOTO 100

    iel    = itepa(ip,jisor)
    isuivi = -999
    ifaold = ifanew
    indian = 0
    icecpt = icecpt + 1

!         ---> Elimination des particules qui posent problemes
!              (boucles infinies)

    if (icecpt.gt.30) then
      itepa(ip,jisor) = 0
      nbperr = nbperr + 1
      dnbper = dnbper + tepa(ip,jrpoi)
      if (ierrie.eq.0) then
#if DEBUG_LAGCEL
        if (impltg.eq.1) write(nfecra,9103) ip
#endif
        goto 200
      else
        write(nfecra,9103) ip
        ierr = 1
        goto 300
      endif
    endif

!--> Balayage des KFAC faces entourant la cellule IEL,
!    elles sont stockees entre ITYCEL(IEL) et ITYCEL(IEL+1)-1
!    (donc KFAC ne peut pas valoir ITYCEL(IEL+1)...)

!    Remarque : si on veut supprimer le DO WHILE il faut utiliser
!               la ligne suivante
!         DO KFAC = ITYCEL(IEL),ITYCEL(IEL+1)-1

    kfac = itycel(iel)-1

    do while (indian.eq.0)

      kfac = kfac + 1

!--> Cas ou aucune face a INDIAN= -1 ou 1 sur la cellule n'a ete
!    detectee, gestion de l'erreur :

      if (kfac.eq.itycel(iel+1)) then
        if (ierrie.eq.0) then
          itepa(ip,jisor) = 0
          nbperr = nbperr + 1
          dnbper = dnbper + tepa(ip,jrpoi)
#if DEBUG_LAGCEL
          if (impltg.eq.1)  write(nfecra,9102) iel,ip
#endif
          goto 200
        else
          write(nfecra,9102) iel,ip
          ierr = 1
          goto 300
         endif
      endif

      ifac = icocel(kfac)

!--> Boucle sur les faces internes (numero positif dans ICOCEL)
!    resultat : INDIAN =  0 le rayon PQ ne sort pas de la cellule
!    ~~~~~~~~               par cette face
!               INDIAN = -1 meme cellule
!               INDIAN =  1 sortie de la cellule par cette face

!--> Si la face interne a deja ete traitee dans la cellule precedente
!    son numero est dans IFAOLD et on ne la retraite pas une 2eme fois

      if (ifac.gt.0 .and. ifac.ne.ifaold) then

        in = 0
        do nbp = ipnfac(ifac),ipnfac(ifac+1)-1
          in = in + 1
          iconfo(in) = nodfac(nbp)
        enddo
        itypfo = ipnfac(ifac+1) - ipnfac(ifac) + 1
        iconfo(itypfo) = iconfo(1)

#if DEBUG_LAGCEL
        nbp = ipnfac(ifac+1) - ipnfac(ifac)
        write(impla4)                                             &
        nbp,ifac,iel,(nodfac(in),in=ipnfac(ifac),ipnfac(ifac+1)-1)
        if (nbp.eq.4) then
          nquad4 = nquad4 + 1
        else if (nbp.eq.3) then
          ntria3 = ntria3 + 1
        else if (nbp.ge.5) then
          nsided = nsided + 1
        endif
#endif

        call ouestu                                               &
        !==========
   (    nfecra , ndim   , nnod ,                                  &
        ierror ,                                                  &
        ettpa(ip,jxp)  , ettpa(ip,jyp)  , ettpa(ip,jzp)  ,        &
        ettp(ip,jxp)   , ettp(ip,jyp)   , ettp(ip,jzp)   ,        &
        cdgfac(1,ifac)    , cdgfac(2,ifac)    , cdgfac(3,ifac)   ,&
        xyzcen(1,iel)     , xyzcen(2,iel)     , xyzcen(3,iel)    ,&
        itypfo , iconfo , xyznod ,                                &
        indian )

#if DEBUG_LAGCEL
        if (impltg.eq.1) write(nfecra,9004) ip , ifac , indian
#endif

!         ---> Elimination des particules qui posent problemes

        if (ierror.eq.1) then
          ierror = 0
          itepa(ip,jisor) = 0
          nbperr = nbperr + 1
          dnbper = dnbper + tepa(ip,jrpoi)
          if (ierrie.eq.0) then
#if DEBUG_LAGCEL
            if (impltg.eq.1) write(nfecra,9101) ifac,ip
#endif
            goto 200
          else
            write(nfecra,9101) ifac,ip
            ierr = 1
            goto 300
          endif

!--> Si la particule passe dans la cellule voisine

        else if (indian.eq.1) then

!--> Si la particule IP est dans la cellule II alors le voisin ne
!    peut etre que la cellule JJ, et vice versa, et inversement, et
!    ainsi de suite.

          ifanew = ifac

          ii = ifacel(1,ifac)
          jj = ifacel(2,ifac)

          if (iel.eq.ii) then
            itepa(ip,jisor) = jj
          else if (iel.eq.jj) then
            itepa(ip,jisor) = ii
          endif

! Traitement de la periodicite (debut).
!  Ne marchera pas si on passe en parallele. Le fait d'etre dans le halo
!  n'est pas suffisant pour conclure si la cellule est periodique.

          if (itepa(ip,jisor).gt.ncel) then

!                 Si on est sur un plan de periodicite, on passe
!                 a l'ordre 1 sur les schemas

            ibord(ip) = -1

            itepas = itepa(ip,jisor)
            itepa(ip,jisor) = celcr(itepas-ncel)

!                 On recupere les informations sur la peridodicite

            iper  = percr(itepas-ncel)

! Faire un test si IPER           != -1 pour ne traiter que les cellules periodiques
! Finir l'implementation dans PERLOC

!                 POINT DE DEPART

            pta(1) = ettpa(ip,jxp)
            pta(2) = ettpa(ip,jyp)
            pta(3) = ettpa(ip,jzp)

            call lagper(iper, pta, ptb)
            !==========

            ettpa(ip,jxp) = ptb(1)
            ettpa(ip,jyp) = ptb(2)
            ettpa(ip,jzp) = ptb(3)

!                 POINT D'ARRIVEE

            pta(1) = ettp(ip,jxp)
            pta(2) = ettp(ip,jyp)
            pta(3) = ettp(ip,jzp)

            iper  = percr(itepas-ncel)

            call lagper(iper, pta, ptb)
            !==========

            ettp(ip,jxp) = ptb(1)
            ettp(ip,jyp) = ptb(2)
            ettp(ip,jzp) = ptb(3)

!                 MODIFICATION DES VITESSES PARTICULES

            vect(1) = ettpa(ip,jup)
            vect(2) = ettpa(ip,jvp)
            vect(3) = ettpa(ip,jwp)

            call lagvec(iper, vect, vectn)
            !==========

            ettpa(ip,jup) = vectn(1)
            ettpa(ip,jvp) = vectn(2)
            ettpa(ip,jwp) = vectn(3)

            vect(1) = ettp(ip,jup)
            vect(2) = ettp(ip,jvp)
            vect(3) = ettp(ip,jwp)

            call lagvec(iper, vect, vectn)
            !==========

            ettp(ip,jup) = vectn(1)
            ettp(ip,jvp) = vectn(2)
            ettp(ip,jwp) = vectn(3)

!                 MODIFICATION DES VITESSES FLUIDES VUES

            vect(1) = ettpa(ip,juf)
            vect(2) = ettpa(ip,jvf)
            vect(3) = ettpa(ip,jwf)

            call lagvec(iper, vect, vectn)
            !==========

            ettpa(ip,juf) = vectn(1)
            ettpa(ip,jvf) = vectn(2)
            ettpa(ip,jwf) = vectn(3)

            vect(1) = ettp(ip,juf)
            vect(2) = ettp(ip,jvf)
            vect(3) = ettp(ip,jwf)

            call lagvec(iper, vect, vectn)
            !==========

            ettp(ip,juf) = vectn(1)
            ettp(ip,jvf) = vectn(2)
            ettp(ip,jwf) = vectn(3)

            ifanew = 0

          endif

! Traitement de la periodicite (fin)

! deposition model treatment

          if (idepst.gt.0) then

             if (itepa(ip,jimark).ge.0) then

               !Test on the new cell if it is a boundary cell at the wall

               ltest = 0
               do il = itycel(itepa(ip,jisor)), itycel(itepa(ip,jisor)+1)-1
                 if (icocel(il) .lt. 0) then
                   ifacp = -icocel(il)
                   izone = ifrlag(ifacp)
                   if (iusclb(izone).eq.idepo1 .or.           &
                       iusclb(izone).eq.idepo2 .or.           &
                       iusclb(izone).eq.idepo3 .or.           &
                       iusclb(izone).eq.idepfa .or.           &
                       iusclb(izone).eq.irebol     ) then
                     ltest = ltest+1
                   endif
                 endif
               enddo

               if (ltest.gt.0) then

                 ! Computation of the intersection position between the crossed
                 ! face and the trajectory

                 xp = ettpa(ip,jxp)
                 yp = ettpa(ip,jyp)
                 zp = ettpa(ip,jzp)

                 xq = ettp(ip,jxp)
                 yq = ettp(ip,jyp)
                 zq = ettp(ip,jzp)

                 xpq = xq - xp
                 ypq = yq - yp
                 zpq = zq - zp

                 aa = xpq * surfac(1,ifac)                         &
                     +ypq * surfac(2,ifac)                         &
                     +zpq * surfac(3,ifac)

                 if (abs(aa).lt.1.d-15) then
                   ! Problematic case: we eliminate the particle
                   if (ierrie.eq.0) then
                     itepa(ip,jisor) = 0
                     nbperr = nbperr + 1
                     dnbper = dnbper + tepa(ip,jrpoi)
#if DEBUG_LAGCEL
                     if (impltg.eq.1)  write(nfecra,9102) iel,ip
#endif
                     goto 200
                   else
                     write(nfecra,9102) iel,ip
                     ierr = 1
                     goto 300
                   endif

                 endif
                 ! end of the problematic case treatement

                 aa = ( surfac(1,ifac) * cdgfac(1,ifac)            &
                       +surfac(2,ifac) * cdgfac(2,ifac)            &
                       +surfac(3,ifac) * cdgfac(3,ifac)            &
                       -surfac(1,ifac) * xp                        &
                       -surfac(2,ifac) * yp                        &
                       -surfac(3,ifac) * zp )                      &
                    / aa

                 xk = xp + xpq * aa
                 yk = yp + ypq * aa
                 zk = zp + zpq * aa

                 ! We locate the particle at the intersection
                 ! and we stop the treatment

                 ielnew = itepa(ip,jisor)

                 distp = sqrt( (xyzcen(1,ielnew)-xk)**2            &
                              +(xyzcen(2,ielnew)-yk)**2            &
                              +(xyzcen(3,ielnew)-zk)**2 )

                 dept  = (xyzcen(1,ielnew)-xk)*dlgeo(ifacp, 8)     &
                        +(xyzcen(2,ielnew)-yk)*dlgeo(ifacp, 9)     &
                        +(xyzcen(3,ielnew)-zk)*dlgeo(ifacp,10)
                 deptt = (xyzcen(1,ielnew)-xk)*dlgeo(ifacp,11)     &
                        +(xyzcen(2,ielnew)-yk)*dlgeo(ifacp,12)     &
                        +(xyzcen(3,ielnew)-zk)*dlgeo(ifacp,13)

                 if (dept.ge.0) then
                   ist = 1.d0
                 else
                   ist =-1.d0
                 endif

                 if (deptt.ge.0) then
                   istt = 1.d0
                 else
                   istt =-1.d0
                 endif

                 ettp(ip,jxp) = xk+1.d-4*distp*dlgeo(ifacp, 8)*ist  &
                                  +1.d-4*distp*dlgeo(ifacp,11)*istt
                 ettp(ip,jyp) = yk+1.d-4*distp*dlgeo(ifacp, 9)*ist  &
                                  +1.d-4*distp*dlgeo(ifacp,12)*istt
                 ettp(ip,jzp) = zk+1.d-4*distp*dlgeo(ifacp,10)*ist  &
                                  +1.d-4*distp*dlgeo(ifacp,13)*istt

                 ! Switch of the normal and tagential velocities between the
                 ! previous and new reference frame

                 ! normal and tangential velocities computation in the previous
                 ! mesh element

                 ifacb = 0
                 do il = itycel(indep(ip)), itycel(indep(ip)+1)-1

                   ifacp = icocel(il)
                   if (ifacp.lt.0) then
                     ifacp = -ifacp
                     izone = ifrlag(ifacp)
                     if (iusclb(izone).eq.idepo1 .or.           &
                         iusclb(izone).eq.idepo2 .or.           &
                         iusclb(izone).eq.idepo3 .or.           &
                         iusclb(izone).eq.idepfa .or.           &
                         iusclb(izone).eq.irebol) then
                       ifacb = ifacp
                     endif
                   endif
                 enddo
                 if (ifacb.eq.0) then
                   write(nfecra,*) ' Erreur lagcel particule ', ip, iel
                   write(nfecra,*) ' On a pas trouve de face de paroi '
                   write(nfecra,*) ' dans la maille precedente '
                   call csexit(1)
                 endif

                 xnor= sqrt( surfbo(1,ifacb)*surfbo(1,ifacb)       &
                            +surfbo(2,ifacb)*surfbo(2,ifacb)       &
                            +surfbo(3,ifacb)*surfbo(3,ifacb) )

                 xn = dlgeo(ifacb,1)
                 yn = dlgeo(ifacb,2)
                 zn = dlgeo(ifacb,3)

                 ! Trajectory projection on the face

                 xxp = xp
                 yyp = yp
                 zzp = zp

                 kk  = (-dlgeo(ifacb,1)*xxp-dlgeo(ifacb,2)*yyp     &
                        -dlgeo(ifacb,3)*zzp-dlgeo(ifacb,4) )       &
                      /( dlgeo(ifacb,1)*dlgeo(ifacb,1)             &
                        +dlgeo(ifacb,2)*dlgeo(ifacb,2)             &
                        +dlgeo(ifacb,3)*dlgeo(ifacb,3) )

                 xpp = kk*xn+xxp
                 ypp = kk*yn+yyp
                 zpp = kk*zn+zzp

                 xxp = xk
                 yyp = yk
                 zzp = zk

                 kk  = (-dlgeo(ifacb,1)*xxp-dlgeo(ifacb,2)*yyp     &
                        -dlgeo(ifacb,3)*zzp-dlgeo(ifacb,4) )       &
                      /( dlgeo(ifacb,1)*dlgeo(ifacb,1)             &
                        +dlgeo(ifacb,2)*dlgeo(ifacb,2)             &
                        +dlgeo(ifacb,3)*dlgeo(ifacb,3) )

                 xkp = kk*xn+xxp
                 ykp = kk*yn+yyp
                 zkp = kk*zn+zzp

                 xnor = sqrt( (xkp-xpp)*(xkp-xpp)                  &
                             +(ykp-ypp)*(ykp-ypp)                  &
                             +(zkp-zpp)*(zkp-zpp) )

                 if (xnor.gt.1.d-15) then

                   xt = (xkp-xpp)/xnor
                   yt = (ykp-ypp)/xnor
                   zt = (zkp-zpp)/xnor

                   xtt = yn*zt-zn*yt
                   ytt = zn*xt-xn*zt
                   ztt = xn*yt-yn*xt
                   xnor=sqrt(xtt*xtt+ytt*ytt+ztt*ztt)
                   xtt = xtt/xnor
                   ytt = ytt/xnor
                   ztt = ztt/xnor

                 else

                   xt = dlgeo(ifacb, 8)
                   yt = dlgeo(ifacb, 9)
                   zt = dlgeo(ifacb,10)

                   xtt = dlgeo(ifacb,11)
                   ytt = dlgeo(ifacb,12)
                   ztt = dlgeo(ifacb,13)

                 endif

                 isens = 1
                 call  lagprj                                        &
                 !===========
               ( isens        ,                               &
                 ettp(ip,jup) , ettp(ip,jvp) , ettp(ip,jwp) , &
                 vpart(1)     , vpart(2)     , vpart(3)     , &
                 xn           , yn           , zn           , &
                 xt           , yt           , zt           , &
                 xtt          , ytt          , ztt  )

                 call  lagprj                                        &
                 !============
               ( isens,                                       &
                 ettp(ip,juf) , ettp(ip,jvf) , ettp(ip,jwf) , &
                 vvue(1)      , vvue(2)      , vvue(3)      , &
                 xn           , yn           , zn           , &
                 xt           , yt           , zt           , &
                 xtt          , ytt          , ztt  )

                 ! normal and tagential velocities computation in the previous
                 ! mesh element

                 ifacb = 0
                 do il = itycel(itepa(ip,jisor)), itycel(itepa(ip,jisor)+1)-1

                   ifacp = icocel(il)
                   if ( ifacp .lt. 0 ) then
                     ifacp = -ifacp
                     izone = ifrlag(ifacp)
                     if (iusclb(izone).eq.idepo1 .or.            &
                         iusclb(izone).eq.idepo2 .or.            &
                         iusclb(izone).eq.idepo3 .or.            &
                         iusclb(izone).eq.idepfa .or.            &
                         iusclb(izone).eq.irebol) then
                       ifacb = ifacp
                     endif
                   endif
                 enddo

                 if (ifacb.eq.0) then
                   write(nfecra,*) ' Erreur lagcel particule ', ip
                   write(nfecra,*) ' On a pas trouve de face de paroi '
                   write(nfecra,*) ' dans la nouvelle maille '
                   call csexit(1)
                 endif

                 xn = dlgeo(ifacb,1)
                 yn = dlgeo(ifacb,2)
                 zn = dlgeo(ifacb,3)

                 ! Trajectory projection on the face

                 det = dlgeo(ifacb,1)*xn+dlgeo(ifacb,2)*yn         &
                                        +dlgeo(ifacb,3)*zn

                 xxp = xp
                 yyp = yp
                 zzp = zp

                 kk  = (-dlgeo(ifacb,1)*xxp-dlgeo(ifacb,2)*yyp     &
                        -dlgeo(ifacb,3)*zzp-dlgeo(ifacb,4) )       &
                      /( dlgeo(ifacb,1)*dlgeo(ifacb,1)             &
                        +dlgeo(ifacb,2)*dlgeo(ifacb,2)             &
                        +dlgeo(ifacb,3)*dlgeo(ifacb,3) )

                 xpp = kk*xn+xxp
                 ypp = kk*yn+yyp
                 zpp = kk*zn+zzp

                 xxp = xk
                 yyp = yk
                 zzp = zk

                 kk  = (-dlgeo(ifacb,1)*xxp-dlgeo(ifacb,2)*yyp     &
                        -dlgeo(ifacb,3)*zzp-dlgeo(ifacb,4) )       &
                      /( dlgeo(ifacb,1)*dlgeo(ifacb,1)             &
                        +dlgeo(ifacb,2)*dlgeo(ifacb,2)             &
                        +dlgeo(ifacb,3)*dlgeo(ifacb,3) )

                 xkp = kk*xn+xxp
                 ykp = kk*yn+yyp
                 zkp = kk*zn+zzp

                 xnor = sqrt( (xkp-xpp)*(xkp-xpp)                  &
                             +(ykp-ypp)*(ykp-ypp)                  &
                             +(zkp-zpp)*(zkp-zpp) )

                 if (xnor.gt.1.d-15) then

                   xt = (xkp-xpp)/xnor
                   yt = (ykp-ypp)/xnor
                   zt = (zkp-zpp)/xnor

                   xtt = yn*zt-zn*yt
                   ytt = zn*xt-xn*zt
                   ztt = xn*yt-yn*xt
                   xnor=sqrt(xtt*xtt+ytt*ytt+ztt*ztt)
                   xtt = xtt/xnor
                   ytt = ytt/xnor
                   ztt = ztt/xnor

                 else

                   xt = dlgeo(ifacb, 8)
                   yt = dlgeo(ifacb, 9)
                   zt = dlgeo(ifacb,10)

                   xtt = dlgeo(ifacb,11)
                   ytt = dlgeo(ifacb,12)
                   ztt = dlgeo(ifacb,13)

                 endif

                 isens = 2
                 call  lagprj                                        &
                 !===========
               ( isens    ,                                   &
                 vpart(4) , vpart(5) , vpart(6) ,             &
                 vpart(1) , vpart(2) , vpart(3) ,             &
                 xn       , yn       , zn       ,             &
                 xt       , yt       , zt       ,             &
                 xtt      , ytt      , ztt  )

                 call  lagprj                                        &
                 !===========
               ( isens   ,                                    &
                 vvue(4) , vvue(5) , vvue(6) ,                &
                 vvue(1) , vvue(2) , vvue(3) ,                &
                 xn      , yn      , zn      ,                &
                 xt      , yt      , zt      ,                &
                 xtt     , ytt     , ztt  )

                 ettp(ip,jup) = vpart(4)
                 ettp(ip,jvp) = vpart(5)
                 ettp(ip,jwp) = vpart(6)

                 ettp(ip,juf) = vvue(4)
                 ettp(ip,jvf) = vvue(5)
                 ettp(ip,jwf) = vvue(6)

               goto 200

             else

               itepa(ip,jimark) = -1
               tepa(ip,jryplu)  = 1000.d0

             endif

           endif

         endif

! deposition model treatment End


!--> Retour pour balayage des faces de la cellule suivante

         goto 100

       endif

!--> Balayage des faces de bord (reperees par leur valeur negative
!    dans ICOCEL)

!    resultat : INDIAN =  0 le rayon PQ ne sort pas de la cellule par
!    ~~~~~~~~               cette face
!               INDIAN = -1 meme cellule
!               INDIAN =  1 interaction avec la frontiere

     else if (ifac.lt.0 .and. ifac.ne.ifaold) then

       ifac = -ifac

       in = 0
       do nbp = ipnfbr(ifac),ipnfbr(ifac+1)-1
         in = in + 1
         iconfo(in) = nodfbr(nbp)
       enddo
       itypfo = ipnfbr(ifac+1) - ipnfbr(ifac) + 1
       iconfo(itypfo) = iconfo(1)

#if DEBUG_LAGCEL
       nbp = ipnfbr(ifac+1) - ipnfbr(ifac)
       write(impla4)                                             &
       nbp, -ifac, iel,                                          &
       (nodfbr(in),in=ipnfbr(ifac),ipnfbr(ifac+1)-1)
       if (nbp.eq.4) then
         nquad4 = nquad4 + 1
       else if (nbp.eq.3) then
         ntria3 = ntria3 + 1
       else if (nbp.ge.5) then
         nsided = nsided + 1
       endif
#endif

       call ouestu                                               &
       !==========
  (    nfecra , ndim   , nnod ,                                  &
       ierror ,                                                  &
       ettpa(ip,jxp)  , ettpa(ip,jyp)  , ettpa(ip,jzp)  ,        &
       ettp(ip,jxp)   , ettp(ip,jyp)   , ettp(ip,jzp)   ,        &
       cdgfbo(1,ifac)    , cdgfbo(2,ifac)    , cdgfbo(3,ifac)   ,&
       xyzcen(1,iel)     , xyzcen(2,iel)     , xyzcen(3,iel)    ,&
       itypfo , iconfo , xyznod ,                                &
       indian )

#if DEBUG_LAGCEL
       if (impltg.eq.1) write(nfecra,9002) ip , ifac , indian
#endif

!         ---> Elimination des particules qui posent problemes

       if (ierror.eq.1) then
         ierror = 0
         itepa(ip,jisor) = 0
         nbperr = nbperr + 1
         dnbper = dnbper + tepa(ip,jrpoi)
         if (ierrie.eq.0) then
#if DEBUG_LAGCEL
           if (impltg.eq.1) write(nfecra,9101) ifac,ip
#endif
           goto 200
         else
           write(nfecra,9101) ifac,ip
           ierr = 1
           goto 300
         endif

!--> Si la trajectoire de la particule traverse la face de bord

       else if (indian.eq.1) then
         if (nordre.eq.2) ibord(ip) = ifac

#if DEBUG_LAGCEL
         if (impltg.eq.1) write(nfecra,9003) ifac
#endif

!--> Traitement de l'interaction particule/frontiere

!   1) modification du numero de la cellule de depart
!      (ITEPA(IP,JISOR) = IEL ou 0)

!   2) P devient K, intersection entre la rayon PQ et le plan
!      de la face de bord

!   3) Q est a determiner selon la nature de l'interaction
!      particule/frontiere

!   resultat : ISUIVI = 0 -> on ne suit plus la particule
!   ~~~~~~~~                 dans le maillage apres USLABO
!              ISUIVI = 1 -> la particule continue a etre suivie
!              ISUIVI = -999 valeur initiale aberrante

!   Blindage dans USLABO : ISUIVI ne peut que valoir 0 ou 1 apres.

         call uslabo                                             &
         !==========
 ( lndnod ,                                                       &
   nvar   , nscal  ,                                              &
   nbpmax , nvp    , nvp1   , nvep   , nivep  ,                   &
   ntersl , nvlsta , nvisbr ,                                     &
   ifac   , ip     , isuivi ,                                     &
   itypfb , itrifb , ifrlag , itepa  , indep  ,                   &
   dt     , rtpa   , rtp    , propce , propfa , propfb ,          &
   coefa  , coefb  ,                                              &
   ettp   , ettpa  , tepa   , parbor , ettp(1,jup) ,              &
                                       ettp(1,juf) , auxl   )

!--> Si la particule continue sa route (ex : rebond) apres l'interaction
!    avec la frontiere, il faut continuer a la suivre donc GOTO 100

         if (isuivi.eq.1) then
           ifanew = -ifac
           goto 100
         else if (isuivi.ne.0 .and. isuivi.ne.1) then
           write (nfecra,8010) ifrlag(ifac) , isuivi
           call csexit (1)
           !==========
         endif

       endif

!  fin de IF (IFAC.GT.0 .AND. IFAC.NE.IFAOLD) THEN
     endif

! fin de DO WHILE
   enddo

!--> Fin de la boucle sur les faces entourant la cellule courante

 200      continue

#if DEBUG_LAGCEL
   if (impltg.eq.1) write(nfecra,9005) ip , itepa(ip,jisor)
   CLOSE(IMPLA4, STATUS='DELETE')
#endif

!-->Fin de la boucle principale sur les particules

 endif

enddo

! Free memory
if (iperio.eq.1) then
  deallocate(celcr, percr)
endif

return


!===============================================================================
! 3. Gestion des erreurs et ecriture des fichiers debugX
!===============================================================================

!--> debugX.CASE sont une aide graphique au debuggage, ils contiennent :
!   1) le dernier morceau de la trajectoire de la particule
!      qui a un pepin,
!   2) les faces (avec leur numero global) entourant les volumes
!      qui contiennent le dernier morceau de trajectoire,
!   3) le fichier debug2.geom contient en plus les CDG faces et cellules
!      qui ont pour numero d'element Ensight celui de l'element
!      de maillage (face ou cellule) associe.

!--> On ecrit 4 fichiers :
!   1) debug1.geom et debug1.CASE ecris au format ensight,
!   2) debug2.geom et debug2.CASE ecris au format ensight gold.
!   Les deux fichiers .CASE peuvent etre lus avec Ensight7 indifferemment.

!--> L'ecriture des fichiers debugX est declenchee par defaut
!    par une erreur sur la detection de la trajectoire (IERR=1).

!   ATTENTION : seules les faces de forme triangles et quadrangles sont
!   =========   reconnues par le format ensight et donc enregistrees
!               dans debug1.geom           !
!               Le format ensight gold reconnait toutes formes de faces
!               et doit etre utilise par defaut...


 300  continue

#if DEBUG_LAGCEL

if (ierr.eq.1) then


!...FORMAT ENSIGHT


      write(nfecra,9050)
      OPEN (IMPLA1,FILE='debug1.geom',                            &
            STATUS='UNKNOWN',FORM='FORMATTED',                    &
            ACCESS='SEQUENTIAL')

      WRITE(IMPLA1,'(A)') 'geometrie debuggage'
      WRITE(IMPLA1,'(A)') 'au format ensight6'
      WRITE(IMPLA1,'(A)') 'node id given'
      WRITE(IMPLA1,'(A)') 'element id given'
      WRITE(IMPLA1,'(A)') 'coordinates'
      WRITE(IMPLA1,'(I8)') 4*NQUAD4 + 3*NTRIA3 + 2

      kpt = 0
      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
        if (nbp.lt.5) then
          do in = 1,nbp
            kpt = max(kpt,iconfo(in))
            WRITE(IMPLA1,'(I8,3E12.5)') ICONFO(IN),               &
                                        xyznod(1,iconfo(in)),     &
                                        xyznod(2,iconfo(in)),     &
                                        xyznod(3,iconfo(in))
          enddo
        endif
      enddo

      WRITE(IMPLA1,'(I8,3E12.5)') KPT + 1,                        &
                                  ettpa(ip,jxp),                  &
                                  ettpa(ip,jyp),                  &
                                  ettpa(ip,jzp)

      WRITE(IMPLA1,'(I8,3E12.5)') KPT + 2,                        &
                                  ettp(ip,jxp),                   &
                                  ettp(ip,jyp),                   &
                                  ettp(ip,jzp)

      WRITE(IMPLA1,'(A)')    'part 1'
      WRITE(IMPLA1,'(A,I9)') 'detail particule ',IP
      WRITE(IMPLA1,'(A)')    'bar2'
      npt = 1
      WRITE(IMPLA1,'(I8)')   NPT
      WRITE(IMPLA1,'(3I8)')  NPT , KPT + 1 , KPT + 2

      WRITE(IMPLA1,'(A)') 'part 2'
      WRITE(IMPLA1,'(A)') 'faces'

      if (ntria3.gt.0) then
        WRITE(IMPLA1,'(A)')  'tria3'
        WRITE(IMPLA1,'(I8)') NTRIA3
      endif

      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
        if (ifac.lt.0) ifac = -ifac
        if (nbp.eq.3) then
          WRITE(IMPLA1,'(4I8)')                                   &
          ifac , ( iconfo(in) , in=1,nbp )
        endif
      enddo

      if (nquad4.gt.0) then
        WRITE(IMPLA1,'(A)')  'quad4'
        WRITE(IMPLA1,'(I8)') NQUAD4
      endif

      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
        if (ifac.lt.0) ifac = -ifac
        if (nbp.eq.4) then
          WRITE(IMPLA1,'(5I8)')                                   &
          ifac , ( iconfo(in) , in=1,nbp )
        endif
      enddo

      close(impla1)


      write(nfecra,9055)
      OPEN (IMPLA1,FILE='debug1.CASE',                            &
            STATUS='UNKNOWN',FORM='FORMATTED',                    &
            ACCESS='SEQUENTIAL')
        WRITE(IMPLA1,'(A)') 'FORMAT'
        WRITE(IMPLA1,'(A)') 'type:     ensight'
        WRITE(IMPLA1,'(A)') 'GEOMETRY'
        WRITE(IMPLA1,'(A)') 'model:    debug1.geom'
      close(impla1)


!...FORMAT ENSIGHT GOLD

!-> Ouverture

      write(nfecra,9060)
      OPEN (IMPLA1,FILE='debug2.geom',                            &
            STATUS='UNKNOWN',FORM='FORMATTED',                    &
            ACCESS='SEQUENTIAL')

!-> Entete

      WRITE(IMPLA1,'(A)') 'geometrie debuggage'
      WRITE(IMPLA1,'(A)') 'au format ensight gold'
      WRITE(IMPLA1,'(A)') 'node id given'
      WRITE(IMPLA1,'(A)') 'element id given'

!-> Id des points du segment de trajectoire

      kpt = 0
      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
        do in = 1,nbp
          kpt = max(kpt,iconfo(in))
        enddo
      enddo

!-> Part de la trajectoire

      ipart = 1
      WRITE(IMPLA1,'(A)')     'part'
      WRITE(IMPLA1,'(I10)')   IPART
      WRITE(IMPLA1,'(A,I9)')  'detail particule ',IP
      WRITE(IMPLA1,'(A)')     'coordinates'
      npt = 2
      WRITE(IMPLA1,'(I10)') NPT
      WRITE(IMPLA1,'(I10)') KPT + 1
      WRITE(IMPLA1,'(I10)') KPT + 2
      do in   = 0,2
        WRITE(IMPLA1,'(E12.5)') ETTPA(IP,JXP+IN)
        WRITE(IMPLA1,'(E12.5)') ETTP(IP,JXP+IN)
      enddo

      WRITE(IMPLA1,'(A)')    'bar2'
      npt = 1
      kpt = 2
      WRITE(IMPLA1,'(I10)')  NPT
      WRITE(IMPLA1,'(I10)')  IP
      WRITE(IMPLA1,'(2I10)') NPT,KPT

!-> Part des triangles

      if (ntria3.gt.0) then
        ipart = ipart+1
        WRITE(IMPLA1,'(A)')    'part'
        WRITE(IMPLA1,'(I10)')  IPART
        WRITE(IMPLA1,'(A)')    'faces triangles'
        WRITE(IMPLA1,'(A)')    'coordinates'
        WRITE(IMPLA1,'(I10)')  NTRIA3*3

        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.eq.3) then
            do in = 1,nbp
              WRITE(IMPLA1,'(I10)') ICONFO(IN)
            enddo
          endif
        enddo
        do in   = 1,3
          rewind(impla4)
          do ipt = 1, nquad4 + ntria3 + nsided
            read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
            if (nbp.eq.3) then
              do ii = 1,nbp
                WRITE(IMPLA1,'(E12.5)') XYZNOD(IN,ICONFO(II))
              enddo
            endif
          enddo
        enddo

        WRITE(IMPLA1,'(A)')  'tria3'
        WRITE(IMPLA1,'(I10)') NTRIA3
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (ifac.lt.0) ifac = -ifac
          IF (NBP.EQ.3) WRITE(IMPLA1,'(I10)') IFAC
        enddo
        kpt = 0
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.eq.3) then
            WRITE(IMPLA1,'(3I10)') KPT+1,KPT+2,KPT+3
            kpt = kpt+3
          endif
        enddo
      endif

!-> Part des quadrangles

      if (nquad4.gt.0) then
        ipart = ipart+1
        WRITE(IMPLA1,'(A)')    'part'
        WRITE(IMPLA1,'(I10)')  IPART
        WRITE(IMPLA1,'(A)')    'faces quadrangles'
        WRITE(IMPLA1,'(A)')    'coordinates'
        WRITE(IMPLA1,'(I10)')  NQUAD4*4

        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.eq.4) then
            do in = 1,nbp
              WRITE(IMPLA1,'(I10)') ICONFO(IN)
            enddo
          endif
        enddo
        do in   = 1,3
          rewind(impla4)
          do ipt = 1, nquad4 + ntria3 + nsided
            read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
            if (nbp.eq.4) then
              do jj = 1,nbp
                WRITE(IMPLA1,'(E12.5)') XYZNOD(IN,ICONFO(JJ))
              enddo
            endif
          enddo
        enddo

        WRITE(IMPLA1,'(A)')  'quad4'
        WRITE(IMPLA1,'(I10)') NQUAD4
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (ifac.lt.0) ifac = -ifac
          IF (NBP.EQ.4) WRITE(IMPLA1,'(I10)') IFAC
        enddo
        kpt = 0
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.eq.4) then
            WRITE(IMPLA1,'(4I10)') KPT+1,KPT+2,KPT+3,KPT+4
            kpt = kpt+4
          endif
        enddo
      endif

!-> Part des Polygones

      if (nsided.gt.0) then
        ipart = ipart+1
        WRITE(IMPLA1,'(A)')    'part'
        WRITE(IMPLA1,'(I10)')  IPART
        WRITE(IMPLA1,'(A)')    'faces polygones'
        WRITE(IMPLA1,'(A)')    'coordinates'

        kpt = 0
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.ge.5) kpt = kpt + nbp
        enddo
        WRITE(IMPLA1,'(I10)')  KPT

        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.ge.5) then
            do in = 1,nbp
              WRITE(IMPLA1,'(I10)') ICONFO(IN)
            enddo
          endif
        enddo
        do in   = 1,3
          rewind(impla4)
          do ipt = 1, nquad4 + ntria3 + nsided
            read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
            if (nbp.ge.5) then
              do jj = 1,nbp
                WRITE(IMPLA1,'(E12.5)') XYZNOD(IN,ICONFO(JJ))
              enddo
            endif
          enddo
        enddo

        WRITE(IMPLA1,'(A)')  'nsided'
        WRITE(IMPLA1,'(I10)') NSIDED
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (ifac.lt.0) ifac = -ifac
          IF (NBP.GE.5) WRITE(IMPLA1,'(I10)') IFAC
        enddo
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          IF (NBP.GE.5) WRITE(IMPLA1,'(I10)') NBP
        enddo
        kpt = 0
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(in),in = 1,nbp)
          if (nbp.ge.5) then
            WRITE(IMPLA1,'(100I10)') (KPT+II,II = 1,NBP)
            kpt = kpt+nbp
          endif
        enddo
      endif

!-> Part des points supplementaires (CDG faces et CDG cellules)


      ipart = ipart + 1
      WRITE(IMPLA1,'(A)')   'part'
      WRITE(IMPLA1,'(I10)') IPART
      WRITE(IMPLA1,'(A)')   'CDG faces et CDG cellules'
      WRITE(IMPLA1,'(A)')   'coordinates'

      kpt    = 0
      ielold = 0
      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
        if (iel.ne.ielold) then
          ielold  = iel
          kpt     = kpt + 1
        endif
      enddo
      WRITE(IMPLA1,'(I10)') NQUAD4 + NTRIA3 + NSIDED + KPT

      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
        if (ifac.gt.0) then
          WRITE(IMPLA1,'(I10)') IFAC
        else if (ifac.lt.0) then
          WRITE(IMPLA1,'(I10)') -IFAC
        endif
      enddo

      ielold  = 0
      rewind(impla4)
      do ipt = 1, nquad4 + ntria3 + nsided
        read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
        if (iel.ne.ielold) then
          ielold = iel
          WRITE(IMPLA1,'(I10)') IEL
        endif
      enddo

      do ip = 1,3
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
          if (ifac.gt.0) then
            WRITE(IMPLA1,'(E12.5)') CDGFAC(IP,IFAC)
          else if (ifac.lt.0) then
            WRITE(IMPLA1,'(E12.5)') CDGFBO(IP,-IFAC)
          endif
        enddo
        ielold  = 0
        rewind(impla4)
        do ipt = 1, nquad4 + ntria3 + nsided
          read(impla4) nbp, ifac, iel, (iconfo(ii),ii = 1,nbp)
          if (iel.ne.ielold) then
            ielold = iel
            WRITE(IMPLA1,'(E12.5)') XYZCEN(IP,IEL)
          endif
        enddo
      enddo

!-> Fermeture du fichier geometrique

      close(impla1)

!-> Ecriture du fichier CASE

      write(nfecra,9065)
      OPEN (IMPLA1,FILE='debug2.CASE',                            &
            STATUS='UNKNOWN',FORM='FORMATTED',                    &
            ACCESS='SEQUENTIAL')
        WRITE(IMPLA1,'(A)') 'FORMAT'
        WRITE(IMPLA1,'(A)') 'type:     ensight gold'
        WRITE(IMPLA1,'(A)') 'GEOMETRY'
        WRITE(IMPLA1,'(A)') 'model:    debug2.geom'
      close(impla1)

endif

close(impla4)

if (ierr.eq.1) then
  write(nfecra,9100)
  return
endif

write(nfecra,9999) ierrie , ierr
call csexit (1)
!==========

#endif

!--------
! FORMATS
!--------

 8010 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION DU MODULE LAGRANGIEN   ',/,&
'@    =========                                               ',/,&
'@                                                            ',/,&
'@    L''INDICATEUR DE SUIVI DE PARTICULE DANS LE MAILLAGE    ',/,&
'@       APRES INTERACTION AVEC LA FRONTIERE NB = ',I10        ,/,&
'@       A UNE VALEUR NON PERMISE (LAGCEL).                   ',/,&
'@                                                            ',/,&
'@   ISUIVI DEVRAIT ETRE UN ENTIER EGAL A 0 OU 1              ',/,&
'@       IL VAUT ICI ISUIVI = ', I10                           ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier la valeur de ISUIVI dans la subroutine USLABO.   ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

#if DEBUG_LAGCEL
 9001 format(3X,'@@ Particule N°',I6,' >> cellule de depart : ',I7)

 9002 format(3X,'@@ Particule N°',I6,'  > face de bord : ',I7           &
           ,' REPERE = ',I2)

 9003 format(3X,'@@ Interaction frontiere -> face de bord : ',I7)

 9004 format(3X,'@@ Particule N°',I6,'  > face interne : ',I7           &
           ,' REPERE = ',I2)

 9005 format(3X,'@@ Particule N°',I6,' >> cellule d''arrive : ',I7,/)


 9050 format(/3X,'** ECRITURE DU FICHIER debug1.geom ',                 &
        /3X,'   AU FORMAT ENSIGHT6')

 9055 format(/3X,'** ECRITURE DU FICHIER debug1.CASE ',                 &
        /3X,'   AU FORMAT ENSIGHT6')

 9060 format(/3X,'** ECRITURE DU FICHIER debug2.geom ',                 &
        /3X,'   AU FORMAT ENSIGHT GOLD')

 9065 format(/3X,'** ECRITURE DU FICHIER debug2.CASE ',                 &
       /3X,'   AU FORMAT ENSIGHT GOLD',/)

 9100 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS LE REPERAGE D''UNE PARTICULE     ',/,&
'@    =========    (LAGCEL)                                   ',/,&
'@                                                            ',/,&
'@  Les fichiers debug1.CASE et debug2.CASE peuvent fournir   ',/,&
'@    une explication de la cause de cet echec de             ',/,&
'@    l''algorithme.                                          ',/,&
'@  La cause la plus probable est la presence d''une face     ',/,&
'@    non plane dans le maillage (due a un recollement        ',/,&
'@    par exemple).                                           ',/,&
'@                                                            ',/,&
'@  Choisir de preference le fichier debug2.CASE              ',/,&
'@    au format ensight gold, celui-ci contient plus          ',/,&
'@    d''inforamtion que debug1.CASE au format ensight6.      ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute en mode sans erreur.       ',/,&
'@                                                            ',/,&
'@  Contacter l''equipe de developpement.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
#endif

 9101 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS LE REPERAGE D''UNE PARTICULE     ',/,&
'@    =========    (LAGCEL)                                   ',/,&
'@                                                            ',/,&
'@  ECHEC DE REPERAGE SUR LA FACE ',I10                        ,/,&
'@    POUR LA PARTICULE ',I10                                  ,/,&
'@                                                            ',/,&
'@  Explication possible : le reperage echoue lorsque la      ',/,&
'@    position d''arrive de la particule fait partie du       ',/,&
'@    plan de la face.                                        ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute en mode sans erreur.       ',/,&
'@                                                            ',/,&
'@  Contacter l''equipe de developpement.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

 9102 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS LE REPERAGE D''UNE PARTICULE     ',/,&
'@    =========    (LAGCEL)                                   ',/,&
'@                                                            ',/,&
'@  AUCUNE FACE ENTOURANT LA CELLULE ',I10                     ,/,&
'@    N''EST TRAVERSEE PAR LA DROITE PASSANT PAR LES POINTS   ',/,&
'@    DE DEPART ET D''ARRIVEE DE LA PARTICULE ',I10            ,/,&
'@                                                            ',/,&
'@  Explication possible : mauvais traitement d''une          ',/,&
'@    cellule concave (amelioration a venir).                 ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute en mode sans erreur.       ',/,&
'@                                                            ',/,&
'@  Contacter l''equipe de developpement.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

 9103 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS LE REPERAGE D''UNE PARTICULE     ',/,&
'@    =========    (LAGCEL)                                   ',/,&
'@                                                            ',/,&
'@  ECHEC DE REPERAGE POUR LA PARTICULE ',I10                  ,/,&
'@    TROP DE CELLULES PARCOURUES                             ',/,&
'@                                                            ',/,&
'@  Explication possible : mauvais traitement d''une          ',/,&
'@    cellule concave (amelioration a venir).                 ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute en mode sans erreur.       ',/,&
'@                                                            ',/,&
'@  Contacter l''equipe de developpement.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

#if DEBUG_LAGCEL
 9999 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS LE REPERAGE D''UNE PARTICULE     ',/,&
'@    =========    (LAGCEL)                                   ',/,&
'@                                                            ',/,&
'@  LA DETECTION DE LA TRAJECTOIRE DE LA PARTICULE EST SORTIE ',/,&
'@    EN ERREUR (MODE SANS ERREUR ENCLENCHE AVEC              ',/,&
'@    IERRIE = ',I10,'),                                      ',/,&
'@    SANS QUE L''INDICATEUR D''ECRITURE DES FICHIERS DEBUG   ',/,&
'@    AIT ETE ACTIVE (IERR = ',I10,').                        ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Contacter l''equipe de developpement.                     ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
#endif

!----
! FIN
!----

end subroutine
