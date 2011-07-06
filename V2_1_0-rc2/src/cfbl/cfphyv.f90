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

subroutine cfphyv &
!================

 ( nvar   , nscal  ,                                              &
   ibrom  , izfppp ,                                              &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  )

!===============================================================================
! FONCTION :
! --------

! ROUTINE PHYSIQUE PARTICULIERE : COMPRESSIBLE SANS CHOC

! Calcul des proprietes physiques variables


! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! ibrom            ! te ! <-- ! indicateur de remplissage de romb              !
!        !    !     !                                                !
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
use entsor
use ppppar
use ppthch
use ppincl
use mesh

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          ibrom
integer          izfppp(nfabor)

double precision dt(ncelet), rtp(ncelet,*), rtpa(ncelet,*)
double precision propce(ncelet,*)
double precision propfa(nfac,*), propfb(nfabor,*)
double precision coefa(nfabor,*), coefb(nfabor,*)

! Local variables

integer          iel
integer          ifac
integer          iirom , iiromb

integer          ipass
data             ipass /0/
save             ipass

!===============================================================================
!===============================================================================
! 1. INITIALISATIONS A CONSERVER
!===============================================================================

! --- Initialisation memoire



!===============================================================================
! 2. ON DONNE LA MAIN A L'UTILISATEUR
!===============================================================================

iuscfp = 1
call uscfpv                                                       &
!==========
 ( nvar   , nscal  ,                                              &
   dt     , rtp    , rtpa   , propce , propfa , propfb ,          &
   coefa  , coefb  )

!     Si IUSCFP = 0, l'utilisateur n'a pas inclus le ss pgm uscfpv dans
!       ses sources. C'est une erreur si Cp, Cv ou Lambda est variable.
!     On se contente de faire le test au premier passage.
if(ipass.eq.0) then
  ipass = ipass + 1
  if((ivisls(itempk).gt.0.or.                            &
       icp.gt.0.or.icv.gt.0).and.iuscfp.eq.0) then
    write(nfecra,1000)                                          &
         ivisls(itempk),icp,icv
    call csexit (1)
    !==========
  endif
endif

!===============================================================================
! 3. MISE A JOUR DE LAMBDA/CV
!===============================================================================

! On a v�rifi� auparavant que CV0 �tait non nul.
! Si CV variable est nul, c'est une erreur utilisateur. On fait
!     un test � tous les passages (pas optimal), sachant que pour
!     le moment, on est en gaz parfait avec CV constant : si quelqu'un
!     essaye du CV variable, ce serait dommage que cela lui explose � la
!     figure pour de mauvaises raisons.
! Si IVISLS(IENERG).EQ.0, on a forcement IVISLS(ITEMPK).EQ.0
!     et ICV.EQ.0, par construction de IVISLS(IENERG) dans
!     le sous-programme cfvarp

if(ivisls(ienerg).gt.0) then

  if(ivisls(itempk).gt.0) then

    do iel = 1, ncel
      propce(iel,ipproc(ivisls(ienerg))) =               &
           propce(iel,ipproc(ivisls(itempk)))
    enddo

  else
    do iel = 1, ncel
      propce(iel,ipproc(ivisls(ienerg))) =               &
           visls0(itempk)
    enddo

  endif

  if(icv.gt.0) then

    do iel = 1, ncel
      if(propce(iel,ipproc(icv)).le.0.d0) then
        write(nfecra,2000)iel,propce(iel,ipproc(icv))
        call csexit (1)
        !==========
      endif
    enddo

    do iel = 1, ncel
      propce(iel,ipproc(ivisls(ienerg))) =               &
           propce(iel,ipproc(ivisls(ienerg)))            &
           / propce(iel,ipproc(icv))
    enddo

  else

    do iel = 1, ncel
      propce(iel,ipproc(ivisls(ienerg))) =               &
           propce(iel,ipproc(ivisls(ienerg)))            &
           / cv0
    enddo

  endif

else

  visls0(ienerg) = visls0(itempk)/cv0

endif

!===============================================================================
! 3. MISE A JOUR DE ROM et ROMB :
!     On ne s'en sert a priori pas, mais la variable existe
!     On a ici des valeurs issues du pas de temps pr�c�dent (y compris
!       pour les conditions aux limites) ou issues de valeurs initiales
!     L'�change p�rio/parall sera fait dans phyvar.
!===============================================================================

iirom  = ipproc(irom  )
iiromb = ipprob(irom  )

do iel = 1, ncel
  propce(iel,iirom)  = rtpa(iel,isca(irho))
enddo

do ifac = 1, nfabor
  iel = ifabor(ifac)
  propfb(ifac,iiromb) =                                         &
       coefa(ifac,iclrtp(isca(irho),icoef))              &
       + coefb(ifac,iclrtp(isca(irho),icoef))            &
       * rtpa(iel,isca(irho))
enddo

!--------
! FORMATS
!--------

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION (MODULE COMPRESSIBLE)  ',/,&
'@    =========                                               ',/,&
'@                                                            ',/,&
'@  Une ou plusieurs des propri�t�s suivantes a �t� d�clar�e  ',/,&
'@    variable (rep�r�e ci-dessous par un indicateur non nul) ',/,&
'@    et une loi doit �tre fournie dans uscfpv.               ',/,&
'@         propri�t�                               indicateur ',/,&
'@     - conductivit� thermique                    ',I10       ,/,&
'@     - capacit� calorifique � pression constante ',I10       ,/,&
'@     - capacit� calorifique � volume constant    ',I10       ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Renseigner uscfpv ou d�clarer les propri�t�s constantes et',/,&
'@    uniformes (uscfx2 pour la conductivit� thermique,       ',/,&
'@    uscfth pour les capacit�s calorifiques).                ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 2000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''EXECUTION (MODULE COMPRESSIBLE)  ',/,&
'@    =========                                               ',/,&
'@                                                            ',/,&
'@  La capacit� calorifique � volume constant pr�sente (au    ',/,&
'@    moins) une valeur n�gative ou nulle :                   ',/,&
'@    cellule ',I10,   '  Cv = ',E18.9                         ,/,&
'@                                                            ',/,&
'@  Le calcul ne sera pas execute.                            ',/,&
'@                                                            ',/,&
'@  Verifier uscfpv.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)


!----
! FIN
!----

return
end subroutine
