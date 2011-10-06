#ifndef __CS_GRADIENT_H__
#define __CS_GRADIENT_H__

/*============================================================================
 * Gradient reconstruction.
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2011 EDF S.A.

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
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definition
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Gradient types
 *----------------------------------------------------------------------------*/

typedef enum {

  CS_GRADIENT_ITER,        /* Iterative method */
  CS_GRADIENT_LSQ_STD,     /* Least-square method */
  CS_GRADIENT_LSQ_EXT,     /* Least-square method with extended neighborhood  */
  CS_GRADIENT_LSQ_EXT_RED, /* Least-square method with reduced extended neig. */
  CS_GRADIENT_LSQ_ITER,    /* LSQ followed with iterative */
  CS_GRADIENT_N_TYPES

} cs_gradient_type_t;

/*============================================================================
 *  Global variables
 *============================================================================*/

/* Short names for gradient types */

extern const char *cs_gradient_type_name[];

/*============================================================================
 * Public function prototypes for Fortran API
 *============================================================================*/

void CS_PROCF (cgdcel, CGDCEL)
(
 const cs_int_t   *const ncelet,      /* --> number of extended cells         */
 const cs_int_t   *const ncel,        /* --> number of cells                  */
 const cs_int_t   *const nfac,        /* --> number of internal faces         */
 const cs_int_t   *const nfabor,      /* --> number of boundary faces         */
 const cs_int_t   *const ncelbr,      /* --> number of cells on boundary      */
 const cs_int_t   *const ivar,
 const cs_int_t   *const imrgra,      /* --> gradient computation mode        */
 const cs_int_t   *const inc,         /* --> 0 or 1: increment or not         */
 const cs_int_t   *const iccocg,      /* --> 1 or 0: recompute COCG or not    */
 const cs_int_t   *const nswrgp,      /* --> >1: with reconstruction          */
 const cs_int_t   *const idimte,      /* --> 0, 1, 2: scalar, vector, tensor  */
 const cs_int_t   *const itenso,      /* --> for rotational periodicity       */
 const cs_int_t   *const iphydp,      /* --> use hydrosatatic pressure        */
 const cs_int_t   *const iwarnp,      /* --> verbosity level                  */
 const cs_int_t   *const nfecra,      /* --> standard output unit             */
 const cs_int_t   *const imligp,      /* --> type of clipping                 */
 const cs_real_t  *const epsrgp,      /* --> precision for iterative gradient
                                             calculation                      */
 const cs_real_t  *const extrap,      /* --> extrapolate gradient at boundary */
 const cs_real_t  *const climgp,      /* --> clipping coefficient             */
 const cs_int_t          ifacel[],    /* --> interior face->cell connectivity */
 const cs_int_t          ifabor[],    /* --> boundary face->cell connectivity */
 const cs_int_t          icelbr[],    /* --> list of cells on boundary        */
 const cs_int_t          isympa[],    /* --> indicator for symmetry faces     */
 const cs_real_t         volume[],    /* --> cell volumes                     */
 const cs_real_t         surfac[],    /* --> surfaces of internal faces       */
 const cs_real_t         surfbo[],    /* --> surfaces of boundary faces       */
 const cs_real_t         surfbn[],    /* --> norm of surfbo                   */
 const cs_real_t         pond[],      /* --> interior faces geometric weight  */
 const cs_real_t         dist[],      /* --> interior faces I' to J' distance */
 const cs_real_t         distbr[],    /* --> boundary faces I' to J' distance */
 const cs_real_t         dijpf[],     /* --> interior faces I'J' vector       */
 const cs_real_t         diipb[],     /* --> boundary faces II' vector        */
 const cs_real_t         dofij[],
       cs_real_t         fextx[],     /* --> components of the exterior force */
       cs_real_t         fexty[],     /*     generating the hydrostatic       */
       cs_real_t         fextz[],     /*     pressure                         */
 const cs_real_t         xyzcen[],    /* --> cell centers                     */
 const cs_real_t         cdgfac[],    /* --> interior face centers of gravity */
 const cs_real_t         cdgfbo[],    /* --> boundary face centers of gravity */
 const cs_real_t         coefap[],    /* --> boundary condition term          */
 const cs_real_t         coefbp[],    /* --> boundary condition term          */
       cs_real_t         pvar[],      /* --> gradient's base variable         */
       cs_real_t         cocgb[],     /* <-> contribution to COCG of cells
                                             on boundary's internal faces     */
       cs_real_t         cocg[],      /* <-> contribution to COCG of cells
                                             on boundary's boundary faces     */
       cs_real_t         cocib[],     /* <-> contribution to COCG of cells
                                             on boundary's internal faces     */
       cs_real_t         coci[],      /* <-> contribution to COCG of cells
                                             on boundary's boundary faces     */
       cs_real_t         grad[]       /* <-- gradient                         */
);

void CS_PROCF (cgdvec, CGDVEC)
(
 const cs_int_t   *const ncelet,      /* --> number of extended cells         */
 const cs_int_t   *const ncel,        /* --> number of cells                  */
 const cs_int_t   *const nfac,        /* --> number of internal faces         */
 const cs_int_t   *const nfabor,      /* --> number of boundary faces         */
 const cs_int_t   *const ivar,
 const cs_int_t   *const imrgra,      /* --> gradient computation mode        */
 const cs_int_t   *const inc,         /* --> 0 or 1: increment or not         */
 const cs_int_t   *const nswrgp,      /* --> >1: with reconstruction          */
 const cs_int_t   *const iwarnp,      /* --> verbosity level                  */
 const cs_int_t   *const nfecra,      /* --> standard output unit             */
 const cs_int_t   *const imligp,      /* --> type of clipping                 */
 const cs_real_t  *const epsrgp,      /* --> precision for iterative gradient
                                             calculation                      */
 const cs_real_t  *const extrap,      /* --> extrapolate gradient at boundary */
 const cs_real_t  *const climgp,      /* --> clipping coefficient             */
 const cs_int_t          ifacel[],
 const cs_int_t          ifabor[],
 const cs_int_t          isympa[],    /* --> indicator for symmetry faces     */
 const cs_real_t         volume[],
 const cs_real_t         surfac[],
 const cs_real_t         surfbo[],
 const cs_real_t         surfbn[],
 const cs_real_t         pond[],      /* --> interior faces geometric weight  */
 const cs_real_t         dist[],      /* --> interior faces I' to J' distance */
 const cs_real_t         distbr[],    /* --> boundary faces I' to J' distance */
 const cs_real_t         dijpf[],     /* --> interior faces I'J' vector       */
 const cs_real_t         diipb[],     /* --> boundary faces II' vector        */
 const cs_real_t         dofij[],
 const cs_real_t         xyzcen[],
 const cs_real_t         cdgfac[],
 const cs_real_t         cdgfbo[],
 const cs_real_t         coefau[],    /* --> boundary condition term          */
 const cs_real_t         coefbu[],    /* --> boundary condition term          */
       cs_real_t         pvar[],      /* --> gradient's base variable         */
       cs_real_t         cocg[],      /* <-> Matrix COCG for the variable     */
       cs_real_t         gradv[]      /* <-- gradient of the variable         */
);


/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Initialize gradient computation API.
 *----------------------------------------------------------------------------*/

void
cs_gradient_initialize(void);

/*----------------------------------------------------------------------------
 * Finalize gradient computation API.
 *----------------------------------------------------------------------------*/

void
cs_gradient_finalize(void);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_GRADIENT__ */
