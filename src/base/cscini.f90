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

subroutine cscini &
!================

 ( idbia0 , idbra0 ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr , ncelbr ,                            &
   nvar   , nscal  , nphas  ,                                     &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod ,          &
   rdevel , rtuser , ra     )

!===============================================================================
! FONCTION :
! --------

!   INITIALISATION DES VARIABLES PRINCIPALES POUR UN COUPLAGE
!     CODE_SATURNE / CODE_SATURNE

!-------------------------------------------------------------------------------
! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
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

include "paramx.h"
include "pointe.h"
include "numvar.h"
include "optcal.h"
include "cstphy.h"
include "cstnum.h"
include "entsor.h"
include "parall.h"
include "period.h"
include "albase.h"
include "cplsat.h"

!===============================================================================

! Arguments

integer          idbia0 , idbra0
integer          ndim   , ncelet , ncel   , nfac   , nfabor
integer          nfml   , nprfml
integer          nnod   , lndfac , lndfbr , ncelbr
integer          nvar   , nscal  , nphas
integer          nideve , nrdeve , nituse , nrtuse

integer          ifacel(2,nfac) , ifabor(nfabor)
integer          ifmfbr(nfabor) , ifmcel(ncelet)
integer          iprfml(nfml,nprfml)
integer          ipnfac(nfac+1), nodfac(lndfac)
integer          ipnfbr(nfabor+1), nodfbr(lndfbr)
integer          idevel(nideve), ituser(nituse), ia(*)

double precision xyzcen(ndim,ncelet)
double precision surfac(ndim,nfac), surfbo(ndim,nfabor)
double precision cdgfac(ndim,nfac), cdgfbo(ndim,nfabor)
double precision xyznod(ndim,nnod)
double precision rdevel(nrdeve), rtuser(nrtuse), ra(*)

! Local variables

integer          idebia , idebra , ifinia , ifinra
integer          iphas
integer          numcpl
integer          imobmx , ialemx , nvcpmx, ifcpmx

!===============================================================================

idebia = idbia0
idebra = idbra0

do numcpl = 1, nbrcpl

  ! L'interpolation face/face doit �tre d�finie pour tous les couplages
  ! de mani�re identique.

  call mxicpl(numcpl, ifaccp, ifcpmx)
  !==========

  ifaccp = ifcpmx

  ! On v�rifie si l'une des instances est en r�solution en rep�re relatif

  call mxicpl(numcpl, icorio, icormx(numcpl))
  !==========

  ! Si l'un des maillages est mobiles,
  ! on doit mettre � jour la localisation.

  call mxicpl(numcpl, imobil, imobmx)
  !==========

  ! De la m�me mani�re, si l'on a une approche ALE sur l'un des
  ! maillages, on doit mettre � jour la localisation.

  call mxicpl(numcpl, iale  , ialemx)
  !==========

  if (ialemx.eq.1.or.imobmx.eq.1) then
    imajcp(numcpl) = 1
  else
    imajcp(numcpl) = 0
  endif

  ! D�termination du nombre de variables coupl�es entre les deux
  ! instances du couplage NUMCPL. Toutes les variables d'une instance
  ! sont coupl�es, SAUF dans le cas de l'ALE o� la vitesse de maillage
  ! ne sera pas coupl�e.
  ! Il faudrait faire quelque en revanche pour les physiques particuli�res.

  if (iale.eq.0) then
    nvarcp(numcpl) = nvar
  else
    nvarcp(numcpl) = nvar - 3
  endif

  ! Nombre total de variable envoy�es: max des variables de chaque
  ! ex�cutable

  call mxicpl(numcpl, nvarcp(numcpl), nvcpmx)
  !==========

  nvarto(numcpl) = nvcpmx

  ! Coh�rence des mod�les de turbulence entre chaque instance de CS ;
  ! pour l'instant, on ne traite que les cas de couplage entre
  ! modeles RANS et laminaires, sauf pour le modele v2f (dans ce cas
  ! il n'y a que du couplage mono-modele)

  do iphas = 1, nphas

    call tbicpl(numcpl, 1, 1, iturb(iphas), iturcp(numcpl,iphas))
    !==========

    if (iturb(iphas).eq.50.and.iturcp(numcpl,iphas).ne.50) then
      write(nfecra,1000) numcpl
      call csexit(1)
      !==========
    elseif (itytur(iphas).eq.4.and.                               &
            iturcp(numcpl,iphas)/10.ne.4) then
      write(nfecra,1001) numcpl
      call csexit(1)
      !==========
    endif

  enddo

enddo

!--------
! FORMAT
!--------

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    LES MODELES DE TURBULENCE POUR LE COUPLAGE ' ,I10        ,/,&
'@    SONT DIFFERENTS ALORS QUE L UN DES MODELES EST LE       ',/,&
'@    V2F. CE CAS DE FIGURE N''EST PAS PRIS                   ',/,&
'@    EN COMPTE POUR LE MOMENT.                               ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1001 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET A L''ENTREE DES DONNEES               ',/,&
'@    =========                                               ',/,&
'@    LE COUPLAGE ', I10, ' EST UN COUPLAGE RANS/LES.         ',/,&
'@    CE CAS DE FIGURE N''EST PAS PRIS EN COMPTE POUR         ',/,&
'@    LE MOMENT.                                              ',/,&
'@                                                            ',/,&
'@  Le calcul ne peut etre execute.                           ',/,&
'@                                                            ',/,&
'@  Verifier usini1.                                          ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)

return
end subroutine
