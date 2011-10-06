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

subroutine clipke &
!================

 ( ncelet , ncel   , nvar   ,                                     &
   iclip  , iwarnk ,                                              &
   propce , rtp    )

!===============================================================================
! FONCTION :
! ----------

! CLIPPING DE K ET EPSILON

!-------------------------------------------------------------------------------
! Arguments
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! e  ! <-- ! nombre de variables                            !
! ncelet           ! i  ! <-- ! number of extended (real + ghost) cells        !
! ncel             ! i  ! <-- ! number of cells                                !
! iclip            ! e  ! <-- ! indicateur = 0 on utilise viscl0               !
!                  !    !     !            sinon on utilise viscl              !
! iwarnk           ! e  ! <-- ! niveau d'impression                            !
! propce           ! tr ! <-- ! tableaux des variables au pdt courant          !
!(ncelet,*         !    !     !                                                !
! rtp              ! tr ! <-- ! tableaux des variables au pdt courant          !
! (ncelet     )    !    !     !                                                !
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
use cstphy
use cstnum
use entsor
use optcal
use parall

!===============================================================================

implicit none

! Arguments

integer          nvar, ncelet, ncel
integer          iclip, iwarnk
double precision propce(ncelet,*)
double precision rtp(ncelet,nvar)

! Local variables

integer          iclpke,iel,iclpk2,iclpe2
integer          ivar,ipp,ii,iivisc,iiromc
double precision xepmin,xepm,xe,xkmin,xkm,xk, vmin, vmax, var
double precision epz2

!===============================================================================

iivisc = ipproc(iviscl)
iiromc = ipproc(irom)


! Une petite valeur pour eviter des valeurs exactement nulles.

epz2 = epzero**2

!===============================================================================
! ---> Stockage Min et Max pour listing
!===============================================================================

do ii = 1, 2
  if(ii.eq.1) then
    ivar = ik
  elseif(ii.eq.2) then
    ivar = iep
  endif
  ipp  = ipprtp(ivar)

  vmin =  grand
  vmax = -grand
  do iel = 1, ncel
    var = rtp(iel,ivar)
    vmin = min(vmin,var)
    vmax = max(vmax,var)
  enddo
  if (irangp.ge.0) then
    call parmax (vmax)
    !==========
    call parmin (vmin)
    !==========
  endif
  varmna(ipp) = vmin
  varmxa(ipp) = vmax

enddo

!===============================================================================
! ---> Detection des valeurs hors norme "physiques"
!       uniquement pour avertissement
!       ou dans le cas ICLKEP = 1
!===============================================================================

if (iwarnk.ge.2.or.iclkep.eq.1) then

  if(iclip.eq.1) then

    xkm = 1296.d0*sqrt(cmu)/almax**2
    xepm = 46656.d0*cmu/almax**4
    iclpke = 0
    do iel=1,ncel
      xk = rtp(iel,ik)
      xe = rtp(iel,iep)
      xkmin = xkm*(propce(iel,iivisc)/propce(iel,iiromc))**2
      xepmin = xepm*(propce(iel,iivisc)/propce(iel,iiromc))**3
      if(xk.le.xkmin.or.xe.le.xepmin) then
        if(iclkep.eq.1) then
          rtp(iel,ik)  = xkmin
          rtp(iel,iep) = xepmin
        endif
        iclpke = iclpke + 1
      endif
    enddo

  elseif(iclip.eq.0) then

    xkmin = 1296.d0*sqrt(cmu)/almax**2*                    &
            (viscl0/ro0)**2
    xepmin = 46656.d0*cmu/almax**4*                        &
            (viscl0/ro0)**3
    iclpke = 0
    do iel=1,ncel
      xk = rtp(iel,ik)
      xe = rtp(iel,iep)
      if(xk.le.xkmin.or.xe.le.xepmin) then
        if(iclkep.eq.1) then
          rtp(iel,ik)  = xkmin
          rtp(iel,iep) = xepmin
        endif
        iclpke = iclpke + 1
      endif
    enddo

  else

    write(nfecra,1000)iclip
    call csexit (1)

  endif

  if (irangp.ge.0) call parcpt (iclpke)
                             !==========

! ---  Impression eventuelle

  if(iwarnk.ge.2) then

    write(nfecra,1010)iclpke

  endif

! ---  Stockage nb de clippings pour listing

  if(iclkep.eq.1) then
    iclpmn(ipprtp(ik)) = iclpke
    iclpmn(ipprtp(iep)) = iclpke
  endif

endif

!===============================================================================
! ---> Clipping "standard" ICLKEP = 0
!===============================================================================

if(iclkep.eq.0) then

  iclpk2 = 0
  iclpe2 = 0
  do iel = 1, ncel
    xk = rtp(iel,ik)
    xe = rtp(iel,iep)
    if (abs(xk).le.epz2) then
      iclpk2 = iclpk2 + 1
      rtp(iel,ik) = max(rtp(iel,ik),epz2)
    elseif(xk.le.0.d0) then
      iclpk2 = iclpk2 + 1
      rtp(iel,ik) = -xk
    endif
    if (abs(xe).le.epz2) then
      iclpe2 = iclpe2 + 1
      rtp(iel,iep) = max(rtp(iel,iep),epz2)
    elseif(xe.le.0.d0) then
      iclpe2 = iclpe2 + 1
      rtp(iel,iep) = -xe
    endif
  enddo

  if (irangp.ge.0) then
    call parcpt (iclpk2)
    !==========
    call parcpt (iclpe2)
    !==========
  endif

! ---  Stockage nb de clippings pour listing

  iclpmn(ipprtp(ik)) = iclpk2
  iclpmn(ipprtp(iep)) = iclpe2

endif


!===============================================================================
! ---> Formats
!===============================================================================

#if defined(_CS_LANG_FR)

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ ATTENTION : ARRET DANS clipke                           ',/,&
'@    =========                                               ',/,&
'@     APPEL DE clipke              AVEC OPTION = ',I10        ,/,&
'@     Phase : ',I10                                           ,/,&
'@                                                            ',/,&
'@  Le calcul ne peut pas etre execute.                       ',/,&
'@                                                            ',/,&
'@  Contacter l''assistance.                                  ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1010 format(                                                           &
 I10,' VALEURS DU K-EPS AU DELA DES ECHELLES BASEES SUR ALMAX')

#else

 1000 format(                                                           &
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/,&
'@ @@ WARNING: ABORT IN clipke                                ',/,&
'@    ========                                                ',/,&
'@     CALL OF clipke               WITH OPTION = ',I10        ,/,&
'@     Phase : ',I10                                           ,/,&
'@                                                            ',/,&
'@  The calulation will not be run.                           ',/,&
'@                                                            ',/,&
'@  Contact the support.                                      ',/,&
'@                                                            ',/,&
'@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@',/,&
'@                                                            ',/)
 1010 format(                                                           &
 I10,' K-EPS VALUES BEYOND THE SCALES BASED ON ALMAX')

#endif

return

end subroutine
