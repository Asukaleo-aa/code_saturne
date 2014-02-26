#ifndef __CS_POST_DEFAULT_H__
#define __CS_POST_DEFAULT_H__

/*============================================================================
 * Post-processing management
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2013 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Local type definitions
 *============================================================================*/

/*=============================================================================
 * Global variables
 *============================================================================*/

/*============================================================================
 * Public Fortran function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Output post-processing meshes using associated writers.
 *
 * Fortran interface:
 *
 * subroutine pstgeo
 * *****************
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstgeo, PSTGEO)
(
 void
);

/*----------------------------------------------------------------------------
 * Loop on post-processing meshes to output variables
 *
 * Fortran interface:
 *
 * subroutine pstvar
 * *****************
 *                  ( ntcabs,
 *                    nvar,   nscal,  nvlsta, nvisbr,
 *                    nbpmax, nvp, nvp1, nvep, nivep,
 *                    ttcabs,
 *                    itepa,
 *                    dt,     rtpa,   rtp,    propce, propfa, propfb,
 *                    coefa,  coefb,
 *                    statce, stativ, statfb,
 *                    ettp, ettpa, tepa )
 *
 * integer          ntcabs      : --> : current time step number
 * integer          nvar        : <-- : number of variables
 * integer          nscal       : <-- : number of scalars
 * integer          nvlsta      : <-- : number of statistical variables (lagr)
 * integer          nvisbr      : <-- : number of boundary stat. variables (lagr)
 * integer          nbpmax      : <-- : maximum number of particles allowed
 * integer          nvp         : <-- : number of particle variables
 * integer          nvp1        : <-- : nvp less position, fluid and
 *                              :     : particle velocity
 * integer          nvep        : <-- : number of real particle attributes
 * integer          nivep       : <-- : number of interger particle attributes
 * double precision ttcabs      : <-- : current physical time
 * integer          itepa       : <-- : integer particle attributes
 * double precision dt          : <-- : local time step
 * double precision rtpa        : <-- : cell variables at previous time step
 * double precision rtp         : <-- : cell variables
 * double precision propce      : <-- : cell physical properties
 * double precision propfa      : <-- : interior face physical properties
 * double precision propfb      : <-- : boundary face physical properties
 * double precision coefa       : <-- : boundary conditions array
 * double precision coefb       : <-- : boundary conditions array
 * double precision statce      : <-- : cell statistics (lagrangian)
 * double precision stativ      : <-- : cell variance statistics (lagrangian)
 * double precision statfb      : <-- : boundary face statistics (lagrangian)
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstvar, PSTVAR)
(
 const cs_int_t   *ntcabs,
 const cs_int_t   *nvar,
 const cs_int_t   *nscal,
 const cs_int_t   *nvlsta,
 const cs_int_t   *nvisbr,
 const cs_int_t   *nbpmax,
 const cs_int_t   *nvp,
 const cs_int_t   *nvp1,
 const cs_int_t   *nvep,
 const cs_int_t   *nivep,
 const cs_real_t  *ttcabs,
 const cs_int_t    itepa[],
 const cs_real_t   dt[],
 const cs_real_t   rtpa[],
 const cs_real_t   rtp[],
 const cs_real_t   propce[],
 const cs_real_t   propfa[],
 const cs_real_t   propfb[],
 const cs_real_t   coefa[],
 const cs_real_t   coefb[],
 const cs_real_t   statce[],
 const cs_real_t   stativ[],
 const cs_real_t   statfb[]
);

/*----------------------------------------------------------------------------
 * Define which Lagragian variables should be postprocessed
 *
 * Fortran interface:
 *
 * subroutine lagpvr
 * *****************
 *                  ( ivisv1, ivisv2, ivistp, ivisdm, iviste,
 *                    ivismp, ivisdk, ivisch, ivisck )
 *
 * integer          ivisv1      : <-- : display of variable 'fluid velocity'
 * integer          ivisv2      : <-- : display of variable 'particles velocity'
 * integer          ivistp      : <-- : display of variable 'resident time'
 * integer          ivisdm      : <-- : display of variable 'particle diameter'
 * integer          iviste      : <-- : display of variable 'particle temperature'
 * integer          ivismp      : <-- : display of variable 'particle mass'
 * integer          ivisdk      : <-- : display of variable 'core diameter of part.'
 * integer          ivisch      : <-- : display of variable 'mass of reactive coal'
 * integer          ivisck      : <-- : display of variable 'mass of char'
 *----------------------------------------------------------------------------*/

void CS_PROCF (lagpvr, LAGPVR)
(
 const cs_int_t  *ivisv1,
 const cs_int_t  *ivisv2,
 const cs_int_t  *ivistp,
 const cs_int_t  *ivisdm,
 const cs_int_t  *iviste,
 const cs_int_t  *ivismp,
 const cs_int_t  *ivisdk,
 const cs_int_t  *ivisch,
 const cs_int_t  *ivisck
);

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_POST_DEFAULT_H__ */
