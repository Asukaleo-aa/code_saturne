!-------------------------------------------------------------------------------

!                      Code_Saturne version 2.0.0-rc1
!                      --------------------------

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

subroutine usdpst &
!=================

 ( idbia0 , idbra0 ,                                              &
   ndim   , ncelet , ncel   , nfac   , nfabor , nfml   , nprfml , &
   nnod   , lndfac , lndfbr ,                                     &
   nideve , nrdeve , nituse , nrtuse ,                            &
   ifacel , ifabor , ifmfbr , ifmcel , iprfml ,                   &
   ipnfac , nodfac , ipnfbr , nodfbr ,                            &
   lstcel , lstfac , lstfbr ,                                     &
   idevel , ituser , ia     ,                                     &
   xyzcen , surfac , surfbo , cdgfac , cdgfbo , xyznod , volume , &
   rdevel , rtuser , ra     )

!===============================================================================
! Purpose:
! -------

!    User subroutine.

! Define additional post-processing writers and meshes.
!
! Post-processing writers allow outputs in different formats or with
! different format options and output frequancy than the default writer.
!
! Post-processing meshes are defined as a subset of the main meshe's
! cells or faces (interior and boundary).

!-------------------------------------------------------------------------------
! Arguments
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
! nprfml           ! i  ! <-- ! number of properties per family (group class)  !
! nnod             ! i  ! <-- ! number of vertices                             !
! lndfac           ! i  ! <-- ! size of nodfac indexed array                   !
! lndfbr           ! i  ! <-- ! size of nodfbr indexed array                   !
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
! lstcel(ncelet)   ! ia ! --- ! work array (list of cells)                     !
! lstfac(nfac)     ! ia ! --- ! work array (list of interior faces)            !
! lstfbr(nfabor)   ! ia ! --- ! work array (list of boundary faces)            !
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
! rdevel(nrdeve)   ! ra ! <-> ! real work array for temporary development      !
! rtuser(nrtuse)   ! ra ! <-> ! user-reserved real work array                  !
! ra(*)            ! ra ! --- ! main real work array                           !
!__________________!____!_____!________________________________________________!

!     Type: i (integer), r (real), s (string), a (array), l (logical),
!           and composite types (ex: ra real array)
!     mode: <-- input, --> output, <-> modifies data, --- work array
!===============================================================================

implicit none

!===============================================================================

!===============================================================================
! Common blocks
!===============================================================================

include "paramx.h"
include "optcal.h"
include "entsor.h"
include "parall.h"
include "period.h"

!===============================================================================

! Arguments

integer          idbia0 , idbra0
integer          ndim   , ncelet , ncel   , nfac   , nfabor
integer          nfml   , nprfml
integer          nnod   , lndfac , lndfbr
integer          nideve , nrdeve , nituse , nrtuse

integer          ifacel(2,nfac) , ifabor(nfabor)
integer          ifmfbr(nfabor) , ifmcel(ncelet)
integer          iprfml(nfml,nprfml)
integer          ipnfac(nfac+1), nodfac(lndfac)
integer          ipnfbr(nfabor+1), nodfbr(lndfbr)
integer          lstcel(ncelet), lstfac(nfac), lstfbr(nfabor)
integer          idevel(nideve), ituser(nituse)
integer          ia(*)

double precision xyzcen(ndim,ncelet)
double precision surfac(ndim,nfac), surfbo(ndim,nfabor)
double precision cdgfac(ndim,nfac), cdgfbo(ndim,nfabor)
double precision xyznod(ndim,nnod), volume(ncelet)
double precision rdevel(nrdeve), rtuser(nrtuse)
double precision ra(*)

! Local variables

integer          indmod, icas, nbcas, ipart, nbpart, ipref, icat
integer          ntchrl

integer          nlcel, nlfac , nlfbr
integer          iel, ifac  , ii
integer          idebia, idebra
integer          icoul , icoul1, icoul2, iel1  , iel2
character*32     nomcas, nomfmt, nommai
character*96     nomrep, optfmt

double precision xfac  , yfac  , zfac

!===============================================================================



nbcas  = 0
nbpart = 0

! "pointeurs" to the first free positions in 'ia' and 'ra'

idebia = idbia0
idebra = idbra0

!===============================================================================
! Create output writers for post-processing
! (one per case and per format, to be adapted by the user)
!===============================================================================

! Number of writers (case in the EnSight sense, study in the MED sense,
!                    or root of a CGNS tree)

nbcas = 1

do icas = 1, nbcas

  ! Miscellaneous initializations

  do ii = 1, len(nomcas)
    nomcas (II:II) = ' '
  enddo
  do ii = 1, len(nomrep)
    nomrep (ii:ii) = ' '
  enddo
  do ii = 1, len(nomfmt)
    nomfmt (ii:ii) = ' '
  enddo
  do ii = 1, len(optfmt)
    optfmt (ii:ii) = ' '
  enddo

  ! User definition:

  ! 'nomcas' and 'nomrep' respectively define the file names prefix and
  ! the corresponding directory path.
  ! If 'nomrep' is a local name of the "xxxx.ensight" or "xxxx.med" form,
  ! the script will automatically retreive the results to the 'RESU'
  ! directory, under a name such as XXXX.ENSIGHT.$DATE or XXXX.MED.$DATE.
  ! If 'nomrep' is of another form, it will have to be defined as a
  ! generic user output dire or directory so as to be copied.

  ! A user may also defined 'nomrep' as an absolute path, outside of the
  ! execution directory, in which case the results are output directly
  ! to that directory, and not managed by the script.

  ! 'nomfmt' allows choosing the output format ("EnSight Gold",
  ! "MED_fichier", or "CGNS").

  ! 'optfmt' allows the addition of a list of comma-separated
  ! format-specific output options:
  ! - EnSight:
  !      "text" ou "binary" (default),
  ! - EnSight, MED, or CGNS:
  !     "discard_polygons" to ignore polygons in output.
  !     "discard_polyhedra" to ignore polyhedra in output.
  ! - EnSight or MED :
  !     "divide_polygons" to divide polygons into triangles
  !     "divide_polyhedra" to divide polyhedra into tetrahedra and pyramids

  ! 'indmod' indicates if the meshes output using this writer will be:
  !     0: fixed,
  !     1: deformables with constant topology constante,
  !     2 : modifyable (may be redefined during the calculation through
  !         the 'usmpst' user subroutine).
  !     10: as indmod = 0, with a vertex displacement field
  !     11: as indmod = 1, with a vertex displacement field
  !     12: as indmod = 2, with a vertex displacement field

  ! 'ntchrl' defines the default output frequency (output at a specific
  ! time may still be forced or inhibited using the 'usnpst' user subroutine).

  if (icas .eq. 1) then

    nomcas = 'chr'
    nomrep = 'tinf21.ensight'
    nomfmt = 'EnSight Gold'
    optfmt = 'binary, discard_polygons'
    indmod = 2
    ntchrl = 5
  endif

  ! Create writer

  call pstcwr (icas  , nomcas, nomrep, nomfmt, optfmt, indmod, ntchrl)
  !==========

enddo

! Define number of additional postprocessing output meshes
!=========================================================

! 'nbpart' is the number of parts which will be generated (in the EnSight
! sense; the MED and CGNS equivalent terms are mesh and base respectively).

! A "part" may be any volume or surface defined through a selection of the
! main meshe's cells of faces.

! Example:
!
! 4 "parts", correspondant respectivey to a mixed "interior faces"
! / "exterior faces" extraction, an extraction containing only
! interior faces, and 2 time-varying mesh pieces.

! We will later add a 5th "part", which is an alias of the second.

nbpart = 2

! Start of loop on user-defined parts
!====================================

do ipart = 1, nbpart

  ! Miscellaneous initializations
  !==============================

  nlcel = 0
  nlfac = 0
  nlfbr = 0
  do iel = 1, ncelet
    lstcel(iel) = 0
  enddo
  do ifac = 1, nfac
    lstfac(ifac) = 0
  enddo
  do ifac = 1, nfabor
    lstfbr(ifac) = 0
  enddo

  do ii = 1, len(nommai)
    nommai(ii:ii) = ' '
  enddo

  ! Mark cells or faces included in the mesh (to be adapted by the user)
  !=====================================================================

  ! Note that this subroutine is called before boundary conditions
  ! are defined.

  ! Part 1:
  !   We select interior faces separating cells with color 2 from cells
  !   with color 3, as well as boundary faces of color 4.

  if (ipart .eq. 1) then

    nommai = 'Cut 1'

!         internal faces

    do ifac = 1, nfac

!         look if the face belongs to the cut

      if (abs(cdgfac(2,ifac)).lt.1.d-4) then
        nlfac = nlfac+1
        lstfac(nlfac)= ifac
      endif
    enddo
!
!   Second cut (part 2) : cells at T < 21 degree
!
!   Example : ncelet is initialised, the choice of cells will be done in usmpst.f90

  elseif(ipart .eq. 2) then
!
        nommai = 'celTinf21'
        nlcel = ncelet
!
  endif

  ! Create post-processing mesh
  !============================

  call pstcma (ipart, nommai, nlcel, nlfac, nlfbr, lstcel, lstfac, lstfbr)
  !==========

  ! Associate extracted mesh and writer (to be adapted by the user)
  !================================================================

  if ( ipart .eq. 1 ) then

    ! Associate post-processing mesh 1 with standard output (icas= -1).
    icas = -1
    call pstass(ipart, icas)

  elseif ( ipart .eq. 2 ) then

    ! Associate post-processing mesh 2 with case created here (icas= 1) .
    icas = 1
    call pstass(ipart, icas)
    !==========

  endif

  ! End of loop on user-defined parts
  !==================================

enddo


return

!===============================================================================
! Formats
!===============================================================================

end subroutine
