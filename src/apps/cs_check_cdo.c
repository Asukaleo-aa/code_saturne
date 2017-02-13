/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2017 EDF S.A.

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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bft_error.h"
#include "bft_mem.h"
#include "bft_printf.h"

#include "cs_cdo.h"
#include "cs_cdo_advection.h"
#include "cs_cdo_bc.h"
#include "cs_cdo_connect.h"
#include "cs_cdo_diffusion.h"
#include "cs_cdo_local.h"
#include "cs_cdo_quantities.h"
#include "cs_equation_param.h"
#include "cs_hodge.h"
#include "cs_source_term.h"
#include "cs_time_step.h"
#include "cs_timer.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

#define _dp3  cs_math_3_dot_product

/*============================================================================
 * Static global variables
 *============================================================================*/

static FILE  *hexa = NULL;
static FILE  *tetra = NULL;
static cs_cdo_connect_t  *connect = NULL;
static cs_cdo_quantities_t  *quant = NULL;
static cs_time_step_t  *time_step = NULL;

/*============================================================================
 * Private function prototypes
 *============================================================================*/

/* Test functions */
static void
_unity(cs_real_t         time,
       cs_lnum_t         n_pts,
       const cs_real_t  *xyz,
       cs_real_t         retval[])
{
  CS_UNUSED(time);
  CS_UNUSED(xyz);
  for (cs_lnum_t i = 0; i < n_pts; i++) retval[i] = 1.0;
}

static void
_linear_xyz(cs_real_t         time,
            cs_lnum_t         n_pts,
            const cs_real_t  *xyz,
            cs_real_t         retval[])
{
  CS_UNUSED(time);
  for (cs_lnum_t i = 0; i < n_pts; i++)
    retval[i] = xyz[3*i] + xyz[3*i+1] + xyz[3*i+2];
}

static void
_quadratic_x2(cs_real_t         time,
              cs_lnum_t         n_pts,
              const cs_real_t  *xyz,
              cs_real_t         retval[])
{
  CS_UNUSED(time);
  for (cs_lnum_t i = 0; i < n_pts; i++)
    retval[i] = xyz[3*i]*xyz[3*i];
}

static void
_nonpoly(cs_real_t         time,
         cs_lnum_t         n_pts,
         const cs_real_t  *xyz,
         cs_real_t         retval[])
{
  CS_UNUSED(time);
  for (cs_lnum_t i = 0; i < n_pts; i++)
    retval[i] = exp(xyz[3*i]+xyz[3*i+1]+xyz[3*i+1]-1.5);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Define a cs_cell_mesh_t structure for a uniform hexahedral cell
 *          of size a
 *
 * \param[in]    a          length of sides
 * \param[in]    cm         pointer to the cs_cell_mesh_t struct. to build
 */
/*----------------------------------------------------------------------------*/

static void
_define_cm_hexa_unif(double            a,
                     cs_cell_mesh_t   *cm)
{
  short int  _v, _e, _f;
  short int  *ids = NULL, *sgn = NULL;
  cs_quant_t  *q = NULL;

  const double  ah = a/2.;

  cm->c_id = 0;
  /* Set all quantities */
  cm->flag = CS_CDO_LOCAL_PV |CS_CDO_LOCAL_PVQ | CS_CDO_LOCAL_PEQ |
    CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_EV | CS_CDO_LOCAL_FEQ |
    CS_CDO_LOCAL_DFQ | CS_CDO_LOCAL_HFQ | CS_CDO_LOCAL_FE |CS_CDO_LOCAL_EFQ;

  cm->xc[0] = cm->xc[1] = cm->xc[2] = ah;
  cm->vol_c = a*a*a;

  /* VERTICES */
  cm->n_vc = 8;
  for (int i = 0; i < cm->n_vc; i++) {
    cm->v_ids[i] = i;
    cm->wvc[i] = 1./8.;
  }

  /* Coordinates */
  _v = 0; // V0
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = 0;
  _v = 1; // V1
  cm->xv[3*_v] = a, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = 0;
  _v = 2; // V2
  cm->xv[3*_v] = a, cm->xv[3*_v+1] = a, cm->xv[3*_v+2] = 0;
  _v = 3; // V3
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = a, cm->xv[3*_v+2] = 0;
  _v = 4; // V4
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = a;
  _v = 5; // V5
  cm->xv[3*_v] = a, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = a;
  _v = 6; // V6
  cm->xv[3*_v] = a, cm->xv[3*_v+1] = a, cm->xv[3*_v+2] = a;
  _v = 7;
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = a, cm->xv[3*_v+2] = a;

  /* EDGES */
  cm->n_ec = 12;

  // e0
  _e = 0, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 1, sgn[0] = -1;
  q->center[0] = ah, q->center[1] = 0, q->center[2] = 0;
  q->unitv[0] = 1.0, q->unitv[1] = 0.0, q->unitv[2] = 0.0;

  // e1
  _e = 1, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 3, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 1.0, q->center[1] = ah;
  q->unitv[2] = 0.0, q->center[2] = 0;

  // e2
  _e = 2, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 4, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 0.0, q->center[1] = 0;
  q->unitv[2] = 1.0, q->center[2] = ah;

  // e3
  _e = 3, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 1, ids[1] = 2, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = a;
  q->unitv[1] = 1.0, q->center[1] = ah;
  q->unitv[2] = 0.0, q->center[2] = 0;

  // e4
  _e = 4, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 1, ids[1] = 5, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = a;
  q->unitv[1] = 0.0, q->center[1] = 0;
  q->unitv[2] = 1.0, q->center[2] = ah;

  // e5
  _e = 5, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 2, ids[1] = 6, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = a;
  q->unitv[1] = 0.0, q->center[1] = a;
  q->unitv[2] = 1.0, q->center[2] = ah;

  // e6
  _e = 6, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 2, ids[1] = 3, sgn[0] = -1;
  q->unitv[0] = -1.0, q->center[0] = ah;
  q->unitv[1] =  0.0, q->center[1] = a;
  q->unitv[2] =  0.0, q->center[2] = 0;

  // e7
  _e = 7, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 4, ids[1] = 5, sgn[0] = -1;
  q->unitv[0] = 1.0, q->center[0] = ah;
  q->unitv[1] = 0.0, q->center[1] = 0;
  q->unitv[2] = 0.0, q->center[2] = a;

  // e8
  _e = 8; ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 5, ids[1] = 6, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = a;
  q->unitv[1] = 1.0, q->center[1] = ah;
  q->unitv[2] = 0.0, q->center[2] = a;

  // e9
  _e = 9, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 6, ids[1] = 7, sgn[0] = -1;
  q->unitv[0] = -1.0, q->center[0] = ah;
  q->unitv[1] =  0.0, q->center[1] = a;
  q->unitv[2] =  0.0, q->center[2] = a;

  // e10
  _e = 10; ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge +_e;
  ids[0] = 4, ids[1] = 7, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 1.0, q->center[1] = ah;
  q->unitv[2] = 0.0, q->center[2] = a;

  // e11
  _e = 11, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge +_e;
  ids[0] = 3, ids[1] = 7, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 0.0, q->center[1] = a;
  q->unitv[2] = 1.0, q->center[2] = ah;

  for (short int e = 0; e < cm->n_ec; e++) {
    cm->e_ids[e] = e;
    cm->edge[e].meas = a;
    cm->dface[e].meas = ah*ah;
    for (int k = 0; k < 3; k++) cm->dface[e].unitv[k] = cm->edge[e].unitv[k];
  }

  /* FACES */
  cm->n_fc = 6;
  cm->f2e_idx[0] = 0;
  for (short int f = 0; f < cm->n_fc; f++)
    cm->f2e_idx[f+1] = cm->f2e_idx[f] + 4;

  // f0
  _f = 0, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 0, ids[1] = 3, ids[2] = 6, ids[3] = 1;
  q->unitv[0] =  0.0, q->center[0] = ah;
  q->unitv[1] =  0.0, q->center[1] = ah;
  q->unitv[2] = -1.0, q->center[2] = 0;

  // f1
  _f = 1, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 0, ids[1] = 4, ids[2] = 7, ids[3] = 2;
  q->unitv[0] =  0.0, q->center[0] = ah;
  q->unitv[1] = -1.0, q->center[1] = 0;
  q->unitv[2] =  0.0, q->center[2] = ah;

  // f2
  _f = 2, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 3, ids[1] = 5, ids[2] = 8, ids[3] = 4;
  q->unitv[0] =  1.0, q->center[0] = a;
  q->unitv[1] =  0.0, q->center[1] = ah;
  q->unitv[2] =  0.0, q->center[2] = ah;

  // f3
  _f = 3, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 6, ids[1] = 11, ids[2] = 9, ids[3] = 5;
  q->unitv[0] =  0.0, q->center[0] = ah;
  q->unitv[1] =  1.0, q->center[1] = a;
  q->unitv[2] =  0.0, q->center[2] = ah;

  // f4
  _f = 4, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 1, ids[1] = 11, ids[2] = 10, ids[3] = 2;
  q->unitv[0] = -1.0, q->center[0] = 0;
  q->unitv[1] =  0.0, q->center[1] = ah;
  q->unitv[2] =  0.0, q->center[2] = ah;

  // f5
  _f = 5, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 7, ids[1] = 8, ids[2] = 9, ids[3] = 10;
  q->unitv[0] =  0.0, q->center[0] = ah;
  q->unitv[1] =  0.0, q->center[1] = ah;
  q->unitv[2] =  1.0, q->center[2] = a;

  assert(cm->f2e_idx[cm->n_fc] == 24);

  for (short int f = 0; f < cm->n_fc; f++) {
    cm->f_ids[f] = f;
    cm->f_sgn[f] = 1; // all face are outward-oriented
    cm->hfc[f] = ah;
    cm->face[f].meas = a*a;
    cm->dedge[f].meas = ah;
    for (int k = 0; k < 3; k++) cm->dedge[f].unitv[k] = cm->face[f].unitv[k];
  }

  for (int i = 0; i < cm->f2e_idx[cm->n_fc]; i++)
    cm->tef[i] = ah*ah;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Define a cs_cell_mesh_t structure for a tetrahedral cell
 *          of size a
 *
 * \param[in]    a          length of sides
 * \param[in]    cm         pointer to the cs_cell_mesh_t struct. to build
 */
/*----------------------------------------------------------------------------*/

static void
_define_cm_tetra_ref(double            a,
                     cs_cell_mesh_t   *cm)
{
  short int  _v, _e, _f;
  short int  *ids = NULL, *sgn = NULL;
  cs_quant_t  *q = NULL;

  const double  ah = a/2.;
  const double  sq2 = sqrt(2.), invsq2 = 1./sq2;

  cm->c_id = 0;
  /* Set all quantities */
  cm->flag = CS_CDO_LOCAL_PV |CS_CDO_LOCAL_PVQ | CS_CDO_LOCAL_PEQ |
    CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_EV | CS_CDO_LOCAL_FEQ |
    CS_CDO_LOCAL_DFQ | CS_CDO_LOCAL_HFQ | CS_CDO_LOCAL_FE |CS_CDO_LOCAL_EFQ;

  cm->vol_c = cs_math_onesix*a*a*a;
  cm->xc[0] = cm->xc[1] = cm->xc[2] = 0.25*a;

  /* VERTICES */
  cm->n_vc = 4;
  for (int i = 0; i < cm->n_vc; i++) {
    cm->v_ids[i] = i;
    cm->wvc[i] = 0;
  }

  /* Coordinates */
  _v = 0; // V0
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = 0;
  _v = 1; // V1
  cm->xv[3*_v] = a, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = 0;
  _v = 2; // V2
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = a, cm->xv[3*_v+2] = 0;
  _v = 3; // V3
  cm->xv[3*_v] = 0, cm->xv[3*_v+1] = 0, cm->xv[3*_v+2] = a;

  /* EDGES */
  cm->n_ec = 6;
  for (short int e = 0; e < cm->n_ec; e++) cm->e_ids[e] = e;

  // e0
  _e = 0, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 1, sgn[0] = -1;
  q->center[0] = ah, q->center[1] = 0, q->center[2] = 0;
  q->unitv[0] = 1.0, q->unitv[1] = 0.0, q->unitv[2] = 0.0;
  q->meas = a;

  // e1
  _e = 1, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 2, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 1.0, q->center[1] = ah;
  q->unitv[2] = 0.0, q->center[2] = 0;
  q->meas = a;

  // e2
  _e = 2, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 0, ids[1] = 3, sgn[0] = -1;
  q->unitv[0] = 0.0, q->center[0] = 0;
  q->unitv[1] = 0.0, q->center[1] = 0;
  q->unitv[2] = 1.0, q->center[2] = ah;
  q->meas = a;

  // e3
  _e = 3, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 1, ids[1] = 2, sgn[0] = -1;
  q->unitv[0] =-invsq2, q->center[0] = ah;
  q->unitv[1] = invsq2, q->center[1] = ah;
  q->unitv[2] =    0.0, q->center[2] = 0;
  q->meas = a * sq2;

  // e4
  _e = 4, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 1, ids[1] = 3, sgn[0] = -1;
  q->unitv[0] =-invsq2, q->center[0] = ah;
  q->unitv[1] =    0.0, q->center[1] = 0;
  q->unitv[2] = invsq2, q->center[2] = ah;
  q->meas = a * sq2;

  // e5
  _e = 5, ids = cm->e2v_ids + 2*_e; sgn = cm->e2v_sgn + _e, q = cm->edge + _e;
  ids[0] = 2, ids[1] = 3, sgn[0] = -1;
  q->unitv[0] =    0.0, q->center[0] = 0;
  q->unitv[1] =-invsq2, q->center[1] = ah;
  q->unitv[2] = invsq2, q->center[2] = ah;
  q->meas = a * sq2;

  /* FACES */
  cm->n_fc = 4;
  for (short int f = 0; f < cm->n_fc; f++) {
    cm->f_ids[f] = f;
    cm->f_sgn[f] = 1; // all face are outward-oriented
  }

  cm->f2e_idx[0] = 0;
  for (short int f = 0; f < cm->n_fc; f++)
    cm->f2e_idx[f+1] = cm->f2e_idx[f] + 3;

  // f0
  _f = 0, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 0, ids[1] = 3, ids[2] = 1;
  q->unitv[0] =  0.0, q->center[0] = a/3.;
  q->unitv[1] =  0.0, q->center[1] = a/3.;
  q->unitv[2] = -1.0, q->center[2] = 0;
  q->meas = a*ah;

  // f1
  _f = 1, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 0, ids[1] = 4, ids[2] = 2;
  q->unitv[0] =  0.0, q->center[0] = a/3.;
  q->unitv[1] = -1.0, q->center[1] = 0;
  q->unitv[2] =  0.0, q->center[2] = a/3.;
  q->meas = a*ah;

  // f2
  _f = 2, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 1, ids[1] = 5, ids[2] = 2;
  q->unitv[0] = -1.0, q->center[0] = 0;
  q->unitv[1] =  0.0, q->center[1] = a/3.;
  q->unitv[2] =  0.0, q->center[2] = a/3.;
  q->meas = a*ah;

  // f3
  _f = 3, ids = cm->f2e_ids + cm->f2e_idx[_f], q = cm->face + _f;
  ids[0] = 3, ids[1] = 5, ids[2] = 4;
  q->unitv[0] = 1/sqrt(3), q->center[0] = a/3.;
  q->unitv[1] = 1/sqrt(3), q->center[1] = a/3.;
  q->unitv[2] = 1/sqrt(3), q->center[2] = a/3.;
  q->meas = 0.5*sqrt(3)*a*a;

  assert(cm->f2e_idx[cm->n_fc] == 12);

  // Dual faces, wvc ?

  /* Compute additional quantities */
  for (short int i = 0; i < 2*cm->n_ec; i++) cm->e2f_ids[i] = -1;

  for (short int f = 0; f < cm->n_fc; f++) {

    const cs_quant_t  pfq = cm->face[f];

    /* Compute dual edge quantities */
    cs_math_3_length_unitv(cm->xc, pfq.center,
                           &(cm->dedge[f].meas), cm->dedge[f].unitv);

    /* Compute height of the pyramid of basis f */
    cm->hfc[f] = cs_math_3_dot_product(pfq.unitv,
                                       cm->dedge[f].unitv)*cm->dedge[f].meas;
    assert(cm->hfc[f] > 0);

    /* Compute tef */
    for (short int i = cm->f2e_idx[f]; i < cm->f2e_idx[f+1]; i++) {

      cs_nvec3_t  sefc;
      cs_real_3_t  cp_efc, xexf, xexc;

      const short int  e = cm->f2e_ids[i], eshft = 2*e;
      const cs_quant_t  peq = cm->edge[e]; /* Edge quantities */

      cm->tef[i] = cs_compute_area_from_quant(peq, pfq.center);

      /* Compute the vectorial area for the triangle : xc, xf, xe */
      for (int k = 0; k < 3; k++) {
        xexf[k] = pfq.center[k] - peq.center[k];
        xexc[k] = cm->xc[k] - peq.center[k];
      }
      cs_math_3_cross_product(xexf, xexc, cp_efc);
      cs_nvec3(cp_efc, &sefc);

      /* One should have (cp_efc, sefc) > 0 */
      short int  _sgn = 1;
      if (_dp3(sefc.unitv, peq.unitv) < 0) _sgn = -1;

      if (cm->e2f_ids[eshft] == -1) {
        cm->e2f_ids[eshft] = f;
        cm->sefc[eshft].meas = 0.5*sefc.meas;
        for (int k = 0; k < 3; k++)
          cm->sefc[eshft].unitv[k] = _sgn*sefc.unitv[k];
      }
      else {
        assert(cm->e2f_ids[eshft+1] == -1);
        cm->e2f_ids[eshft+1] = f;
        cm->sefc[eshft+1].meas = 0.5*sefc.meas;
        for (int k = 0; k < 3; k++)
          cm->sefc[eshft+1].unitv[k] = _sgn*sefc.unitv[k];
      }

    }

  } // Loop on cell faces

  /* Compute dual face quantities */
  for (short int e = 0; e < cm->n_ec; e++) {

    cs_real_3_t  df;
    const cs_nvec3_t  s1 = cm->sefc[2*e], s2 = cm->sefc[2*e+1];
    for (int k = 0; k < 3; k++)
      df[k] = s1.meas*s1.unitv[k] + s2.meas*s2.unitv[k];
    cs_nvec3(df, &(cm->dface[e]));

  } // Loop on cell edges

  /* Compute dual cell volume */
  for (short int f = 0; f < cm->n_fc; f++) {

    const double  hf_coef = cs_math_onesix * cm->hfc[f];

    for (int i = cm->f2e_idx[f]; i < cm->f2e_idx[f+1]; i++) {

      const short int  e = cm->f2e_ids[i];
      const short int  v1 = cm->e2v_ids[2*e];
      const short int  v2 = cm->e2v_ids[2*e+1];
      const double  half_pef_vol = cm->tef[i]*hf_coef;

      cm->wvc[v1] += half_pef_vol;
      cm->wvc[v2] += half_pef_vol;

    } // Loop on face edges

  } // Loop on cell faces

  const double  invvol = 1/cm->vol_c;
  for (short int v = 0; v < cm->n_vc; v++) cm->wvc[v] *= invvol;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Dump a local discrete Hodge operator
 *
 * \param[in]    fic        pointer to a FILE structure
 * \param[in]    msg        optional message to print
 * \param[in]    lm         pointer to the cs_sla_locmat_t struct.
 */
/*----------------------------------------------------------------------------*/

static void
_locmat_dump(FILE               *fic,
             const char         *msg,
             const cs_locmat_t  *lm)
{
  assert(fic != NULL && lm != NULL);

  if (msg != NULL)
    fprintf(fic, "%s\n", msg);

  /* List sub-entity ids */
  fprintf(fic, "%6s","ID");
  for (int i = 0; i < lm->n_ent; i++) fprintf(fic, " %11d", lm->ids[i]);
  fprintf(fic, "\n");

  for (int i = 0; i < lm->n_ent; i++) {
    fprintf(fic, " %5d", lm->ids[i]);
    for (int j = 0; j < lm->n_ent; j++)
      fprintf(fic, " % 6.4e", lm->val[i*lm->n_ent+j]);
    fprintf(fic, "\n");
  }

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Dump a local discrete Hodge operator
 *
 * \param[in]    fic        pointer to a FILE structure
 * \param[in]    msg        optional message to print
 * \param[in]    csys       pointer to the cs_cell_sys_t  struct.
 */
/*----------------------------------------------------------------------------*/

static void
_locsys_dump(FILE                 *fic,
             const char           *msg,
             const cs_cell_sys_t  *csys)
{
  assert(fic != NULL && csys != NULL);
  const cs_locmat_t  *lm = csys->mat;

  if (msg != NULL)
    fprintf(fic, "%s\n", msg);

  /* List sub-entity ids */
  fprintf(fic, "%6s","ID");
  for (int i = 0; i < lm->n_ent; i++) fprintf(fic, " %11d", lm->ids[i]);
  fprintf(fic, "%11s %11s %11s\n", "RHS", "SOURCE", "VAL_N");
  for (int i = 0; i < lm->n_ent; i++) {
    fprintf(fic, " %5d", lm->ids[i]);
    for (int j = 0; j < lm->n_ent; j++)
      fprintf(fic, " % 6.4e", lm->val[i*lm->n_ent+j]);
    fprintf(fic, " % 6.4e % 6.4e % 6.4e\n",
            csys->rhs[i], csys->source[i], csys->val_n[i]);
  }

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Analyse a Hodge operator for Vertex-based schemes
 *
 * \param[in]    out     output file
 * \param[in]    cm      pointer to a cs_cell_mesh_t structure
 * \param[in]    hdg     pointer to a cs_locmat_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_test_hodge_vb(FILE               *out,
               cs_cell_mesh_t     *cm,
               cs_locmat_t        *hdg)
{
  fprintf(out, "\n");

  for (short int vi = 0; vi < cm->n_vc; vi++) {
    double  row_sum = 0.;
    double  *row_vals = hdg->val + vi*cm->n_vc;
    for (short int vj = 0; vj < cm->n_vc; vj++) row_sum += row_vals[vj];
    fprintf(out, "V%d = % 9.6e |delta=% 9.6e\n",
            vi, row_sum, row_sum - cm->wvc[vi]*cm->vol_c);
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Analyse a stiffness matrix for Vertex-based schemes
 *
 * \param[in]    out    output file
 * \param[in]    cm     pointer to a cs_cell_mesh_t structure
 * \param[in]    s      pointer to a cs_locmat_t structure (stiffness matrix)
 */
/*----------------------------------------------------------------------------*/

static void
_test_stiffness_vb(FILE               *out,
                   cs_cell_mesh_t     *cm,
                   cs_locmat_t        *s)
{
  fprintf(out, "\nCDO.VB;   %10s %10s\n", "ROW_SUM", "LIN_SUM");
  for (short int vi = 0; vi < cm->n_vc; vi++) {
    double  row_sum = 0., linear_sum = 0.;
    double  *row_vals = s->val + vi*cm->n_vc;
    for (short int vj = 0; vj < cm->n_vc; vj++) {
      row_sum += row_vals[vj];
      linear_sum += row_vals[vj]*cm->xv[3*vj];
    }
    fprintf(out, "  V%d = % 9.6e % 9.6e\n", vi, row_sum, linear_sum);
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Test CDO vertex-based schemes
 *
 * \param[in]    out       output file
 * \param[in]    cm        pointer to a cs_cell_mesh_t structure
 * \param[in]    cbc       pointer to a cs_cell_bc_t structure
 * \param[in]    fm        pointer to a cs_face_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_test_cdovb_schemes(FILE             *out,
                    cs_cell_mesh_t   *cm,
                    cs_cell_bc_t     *cbc,
                    cs_face_mesh_t   *fm)
{
  /* Test with VB scheme */
  cs_cell_builder_t  *cb = cs_cell_builder_create(CS_SPACE_SCHEME_CDOVB,
                                                  connect);
  cs_cell_sys_t  *csys = cs_cell_sys_create(connect->n_max_vbyc);

  /* Initialize a cell view of the algebraic system */
  csys->n_dofs = cm->n_vc;
  csys->mat->n_ent = cm->n_vc;
  for (short int v = 0; v < cm->n_vc; v++)
    csys->mat->ids[v] = cm->v_ids[v];

  /* Handle anisotropic diffusion */
  cb->pty_mat[0][0] = 1.0, cb->pty_mat[0][1] = 0.5, cb->pty_mat[0][2] = 0.0;
  cb->pty_mat[1][0] = 0.5, cb->pty_mat[1][1] = 1.0, cb->pty_mat[1][2] = 0.5;
  cb->pty_mat[2][0] = 0.0, cb->pty_mat[2][1] = 0.5, cb->pty_mat[2][2] = 1.0;

  // Useful for a weak enforcement of the BC */
  cs_math_33_eigen((const cs_real_t (*)[3])cb->pty_mat,
                   &(cb->eig_ratio),
                   &(cb->eig_max));

  /* HODGE */
  /* ===== */

  /* WBS Hodge operator */
  cs_param_hodge_t  hwbs_info = {.is_unity = true,
                                 .is_iso = true,
                                 .inv_pty = false,
                                 .type = CS_PARAM_HODGE_TYPE_VPCD,
                                 .algo = CS_PARAM_HODGE_ALGO_WBS,
                                 .coef = 1.0};
  cs_hodge_vpcd_wbs_get(hwbs_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; HDG.VPCD.WBS; PERMEABILITY.ISO",
               cb->hdg);
  _test_hodge_vb(out, cm, cb->hdg);

  cs_hodge_compute_wbs_surfacic(fm, cb->hdg);
  _locmat_dump(out, "\nCDO.VB; HDG.VPCD.WBS.FACE; UNITY", cb->hdg);
  for (int vi = 0; vi < fm->n_vf; vi++) {
    double  row_sum = 0.0;
    double  *hi = cb->hdg->val + vi*fm->n_vf;
    for (int vj = 0; vj < fm->n_vf; vj++) row_sum += hi[vj];
    fprintf(out, "V%d = %6.4e |delta= %6.4e\n",
            vi, row_sum, row_sum - fm->face.meas/fm->n_vf);
  }

  /* Voronoi Hodge operator */
  cs_param_hodge_t  hvor_info = {.is_unity = true,
                                 .is_iso = true,
                                 .inv_pty = false,
                                 .type = CS_PARAM_HODGE_TYPE_VPCD,
                                 .algo = CS_PARAM_HODGE_ALGO_VORONOI,
                                 .coef = 1.0};
  cs_hodge_vpcd_voro_get(hvor_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; HDG.VPCD.VORONOI; PERMEABILITY.ISO",
               cb->hdg);
  _test_hodge_vb(out, cm, cb->hdg);

  /* DIFFUSION */
  /* ========= */

  /* Stiffness matrix arising from a Hodge EpFd built with COST algo. */
  cs_param_hodge_t  hcost_info = {.is_unity = true,
                                  .is_iso = true,
                                  .inv_pty = false,
                                  .type = CS_PARAM_HODGE_TYPE_EPFD,
                                  .algo = CS_PARAM_HODGE_ALGO_COST,
                                  .coef = 1./3.}; //DGA
  cs_hodge_vb_cost_get_stiffness(hcost_info, cm, cb);
  _locmat_dump(out,"\nCDO.VB; STIFFNESS WITH HDG.EPFD.DGA; PERMEABILITY.ISO",
               cb->loc);
  _test_stiffness_vb(out, cm, cb->loc);

  /* Anisotropic case */
  hcost_info.is_unity = false, hcost_info.is_iso = false;
  cs_hodge_vb_cost_get_stiffness(hcost_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; STIFFNESS WITH HDG.EPFD.DGA; PERMEABILITY.ANISO",
               cb->loc);
  _test_stiffness_vb(out, cm, cb->loc);

  /* Enforce Dirichlet BC */
  cs_cdovb_diffusion_pena_dirichlet(hcost_info, cbc, cm,
                                    cs_cdovb_diffusion_cost_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; PENA.DGA.FLX.COST; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  cs_cdovb_diffusion_weak_dirichlet(hcost_info, cbc, cm,
                                    cs_cdovb_diffusion_cost_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; WEAK.DGA.FLX.COST; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;
  cs_cdovb_diffusion_wsym_dirichlet(hcost_info, cbc, cm,
                                    cs_cdovb_diffusion_cost_flux_op,
                                    fm, cb, csys);

  _locsys_dump(out, "\nCDO.VB; WSYM.DGA.FLX.COST; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  /* Stiffness matrix arising from a Hodge EpFd built with VORONOI algo. */
  hvor_info.type = CS_PARAM_HODGE_TYPE_EPFD;
  cs_hodge_vb_voro_get_stiffness(hvor_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; STIFFNESS WITH HDG.EPFD.VORO; PERMEABILITY.ISO",
               cb->loc);
  _test_stiffness_vb(out, cm, cb->loc);

  /* Enforce Dirichlet BC */
  cs_cdovb_diffusion_pena_dirichlet(hvor_info, cbc, cm,
                                    cs_cdovb_diffusion_cost_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; PENA.VORO.FLX.COST; PERMEABILITY.ISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  /* Stiffness matrix arising from a Hodge EpFd built with WBS algo. */
  hwbs_info.type = CS_PARAM_HODGE_TYPE_EPFD;
  cs_hodge_vb_wbs_get_stiffness(hwbs_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; STIFFNESS WITH HDG.EPFD.WBS; PERMEABILITY.ISO",
               cb->loc);
  _test_stiffness_vb(out, cm, cb->loc);

  hwbs_info.is_unity = false, hwbs_info.is_iso = false;
  cs_hodge_vb_wbs_get_stiffness(hwbs_info, cm, cb);
  _locmat_dump(out, "\nCDO.VB; STIFFNESS WITH HDG.EPFD.WBS; PERMEABILITY.ANISO",
               cb->loc);
  _test_stiffness_vb(out, cm, cb->loc);

  /* Enforce Dirichlet BC */
  cs_cdovb_diffusion_pena_dirichlet(hwbs_info, cbc, cm,
                                    cs_cdovb_diffusion_wbs_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; PENA.WBS.FLX.WBS; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  cs_cdovb_diffusion_weak_dirichlet(hwbs_info, cbc, cm,
                                    cs_cdovb_diffusion_wbs_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; WEAK.WBS.FLX.WBS; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  cs_cdovb_diffusion_wsym_dirichlet(hwbs_info, cbc, cm,
                                    cs_cdovb_diffusion_wbs_flux_op,
                                    fm, cb, csys);
  _locsys_dump(out, "\nCDO.VB; WSYM.WBS.FLX.WBS; PERMEABILITY.ANISO",
               csys);
  for (int v = 0; v < cm->n_vc; v++) csys->rhs[v] = 0;
  for (int v = 0; v < cm->n_vc*cm->n_vc; v++) csys->mat->val[v] = 0.;

  /* ADVECTION OPERATOR */
  /* ================== */

  cs_adv_field_t  *beta = cs_advection_field_create("Adv.Field");
  cs_equation_param_t  *eqp = cs_equation_param_create(CS_EQUATION_TYPE_USER,
                                                       CS_PARAM_VAR_SCAL,
                                                       CS_PARAM_BC_HMG_NEUMANN);

  eqp->space_scheme = CS_SPACE_SCHEME_CDOVB;

  /* Numerical settings for the advection scheme */
  eqp->advection_info.formulation = CS_PARAM_ADVECTION_FORM_CONSERV;
  eqp->advection_info.scheme = CS_PARAM_ADVECTION_SCHEME_UPWIND;
  eqp->advection_info.weight_criterion = CS_PARAM_ADVECTION_WEIGHT_XEXC;
  eqp->advection_info.quad_type = CS_QUADRATURE_BARY;

  /* Constant advection field */
  cs_real_3_t  vector_field = {1., 0., 0.};
  cs_advection_field_def_by_value(beta, vector_field);
  eqp->advection_field = beta;

  /* Free memory */
  beta = cs_advection_field_free(beta);
  eqp = cs_equation_param_free(eqp);

  /* ADVECTION: BOUNDARY FLUX OPERATOR */
  /* ================================= */

  /* SOURCE TERM */
  /* =========== */

  const int  n_runs = 1000;
  cs_real_t  st0_values[8], st1_values[8], st2_values[8], st3_values[8];
  cs_source_term_t  *st = NULL;

  /* Evaluate the performance */
  cs_timer_counter_t  tc0, tc1, tc2, tc3;
  CS_TIMER_COUNTER_INIT(tc0); // build system
  CS_TIMER_COUNTER_INIT(tc1); // build system
  CS_TIMER_COUNTER_INIT(tc2); // build system
  CS_TIMER_COUNTER_INIT(tc3); // build system

  BFT_MALLOC(st, 1, cs_source_term_t);

  /* Test with a constant function */
  st->def.analytic = _unity;
  // What followed is useless in the current context
  st->name = NULL;
  st->flag = cs_source_term_set_default_flag(CS_SPACE_SCHEME_CDOVB);
  st->flag |= CS_FLAG_SCALAR;
  st->def_type = CS_PARAM_DEF_BY_ANALYTIC_FUNCTION;
  st->quad_type = CS_QUADRATURE_BARY;
  st->array_desc.location = 0;
  st->array_desc.state = 0;
  st->array = NULL;

  // Loop on runs to evaluate the performance of each quadrature
  for (int r = 0; r < n_runs; r++) {

    /* Reset */
    for (int v = 0; v < cm->n_vc; v++)
      st0_values[v] = st1_values[v] = st2_values[v] = st3_values[v] = 0.0;

    cs_timer_t  t0 = cs_timer_time();
    cs_source_term_dcsd_bary_by_analytic(st, cm, cb, st0_values);
    cs_timer_t  t1 = cs_timer_time();
    cs_source_term_dcsd_q1o1_by_analytic(st, cm, cb, st1_values);
    cs_timer_t  t2 = cs_timer_time();
    cs_source_term_dcsd_q10o2_by_analytic(st, cm, cb, st2_values);
    cs_timer_t  t3 = cs_timer_time();
    cs_source_term_dcsd_q5o3_by_analytic(st, cm, cb, st3_values);
    cs_timer_t  t4 = cs_timer_time();

    cs_timer_counter_add_diff(&(tc0), &t0, &t1);
    cs_timer_counter_add_diff(&(tc1), &t1, &t2);
    cs_timer_counter_add_diff(&(tc2), &t2, &t3);
    cs_timer_counter_add_diff(&(tc3), &t3, &t4);

  }

  fprintf(out, "\nCDO.VB; SOURCE_TERM P0\n");
  fprintf(out, " V %12s %12s %12s %12s\n",
          "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
  for (int i = 0; i < cm->n_vc; i++)
    fprintf(out, "%2d %10.6e %10.6e %10.6e %10.6e\n",
            i, st0_values[i], st1_values[i], st2_values[i], st3_values[i]);

  /* Test with a linear function */
  st->def.analytic = _linear_xyz;

  // Loop on runs to evaluate the performance of each quadrature
  for (int r = 0; r < n_runs; r++) {

    /* Reset */
    for (int v = 0; v < cm->n_vc; v++)
      st0_values[v] = st1_values[v] = st2_values[v] = st3_values[v] = 0.0;

    cs_timer_t  t0 = cs_timer_time();
    cs_source_term_dcsd_bary_by_analytic(st, cm, cb, st0_values);
    cs_timer_t  t1 = cs_timer_time();
    cs_source_term_dcsd_q1o1_by_analytic(st, cm, cb, st1_values);
    cs_timer_t  t2 = cs_timer_time();
    cs_source_term_dcsd_q10o2_by_analytic(st, cm, cb, st2_values);
    cs_timer_t  t3 = cs_timer_time();
    cs_source_term_dcsd_q5o3_by_analytic(st, cm, cb, st3_values);
    cs_timer_t  t4 = cs_timer_time();

    cs_timer_counter_add_diff(&(tc0), &t0, &t1);
    cs_timer_counter_add_diff(&(tc1), &t1, &t2);
    cs_timer_counter_add_diff(&(tc2), &t2, &t3);
    cs_timer_counter_add_diff(&(tc3), &t3, &t4);

  }

  fprintf(out, "\nCDO.VB; SOURCE_TERM P1\n");
  fprintf(out, " V %12s %12s %12s %12s\n",
          "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
  for (int i = 0; i < cm->n_vc; i++)
    fprintf(out, "%2d %10.6e %10.6e %10.6e %10.6e\n",
            i, st0_values[i], st1_values[i], st2_values[i], st3_values[i]);

  /* Test with a quadratic (x*x) function */
  st->def.analytic = _quadratic_x2;

  // Loop on runs to evaluate the performance of each quadrature
  for (int r = 0; r < n_runs; r++) {

    /* Reset */
    for (int v = 0; v < cm->n_vc; v++)
      st0_values[v] = st1_values[v] = st2_values[v] = st3_values[v] = 0.0;

    cs_timer_t  t0 = cs_timer_time();
    cs_source_term_dcsd_bary_by_analytic(st, cm, cb, st0_values);
    cs_timer_t  t1 = cs_timer_time();
    cs_source_term_dcsd_q1o1_by_analytic(st, cm, cb, st1_values);
    cs_timer_t  t2 = cs_timer_time();
    cs_source_term_dcsd_q10o2_by_analytic(st, cm, cb, st2_values);
    cs_timer_t  t3 = cs_timer_time();
    cs_source_term_dcsd_q5o3_by_analytic(st, cm, cb, st3_values);
    cs_timer_t  t4 = cs_timer_time();

    cs_timer_counter_add_diff(&(tc0), &t0, &t1);
    cs_timer_counter_add_diff(&(tc1), &t1, &t2);
    cs_timer_counter_add_diff(&(tc2), &t2, &t3);
    cs_timer_counter_add_diff(&(tc3), &t3, &t4);

  }

  fprintf(out, "\nCDO.VB; SOURCE_TERM P2\n");
  fprintf(out, " V %12s %12s %12s %12s\n",
          "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
  for (int i = 0; i < cm->n_vc; i++)
    fprintf(out, "%2d %10.6e %10.6e %10.6e %10.6e\n",
            i, st0_values[i], st1_values[i], st2_values[i], st3_values[i]);

    /* Test with a non-polynomial function */
  st->def.analytic = _nonpoly;
  cs_real_t  exact_result[8] = {0.0609162, // V (0.0,0.0,0.0)
                                0.100434,  // V (1.0,0.0,0.0)
                                0.165587,  // V (1.0,1.0,0.0)
                                0.100434,  // V (0.0,1.0,0.0)
                                0.100434,  // V (0.0,0.0,1.0)
                                0.165587,  // V (1.0,0.0,1.0)
                                0.273007,  // V (1.0,1.0,1.0)
                                0.165587}; // V (0.0,1.0,1.0)
  // Loop on runs to evaluate the performance of each quadrature
  for (int r = 0; r < n_runs; r++) {

    /* Reset */
    for (int v = 0; v < cm->n_vc; v++)
      st0_values[v] = st1_values[v] = st2_values[v] = st3_values[v] = 0.0;

    cs_timer_t  t0 = cs_timer_time();
    cs_source_term_dcsd_bary_by_analytic(st, cm, cb, st0_values);
    cs_timer_t  t1 = cs_timer_time();
    cs_source_term_dcsd_q1o1_by_analytic(st, cm, cb, st1_values);
    cs_timer_t  t2 = cs_timer_time();
    cs_source_term_dcsd_q10o2_by_analytic(st, cm, cb, st2_values);
    cs_timer_t  t3 = cs_timer_time();
    cs_source_term_dcsd_q5o3_by_analytic(st, cm, cb, st3_values);
    cs_timer_t  t4 = cs_timer_time();

    cs_timer_counter_add_diff(&(tc0), &t0, &t1);
    cs_timer_counter_add_diff(&(tc1), &t1, &t2);
    cs_timer_counter_add_diff(&(tc2), &t2, &t3);
    cs_timer_counter_add_diff(&(tc3), &t3, &t4);

  }

  fprintf(out, "\nCDO.VB; SOURCE_TERM NON-POLY\n");
  fprintf(out, " V %12s %12s %12s %12s\n",
          "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
  for (int i = 0; i < cm->n_vc; i++)
    fprintf(out, "%2d % 10.6e % 10.6e % 10.6e % 10.6e\n",
            i, st0_values[i], st1_values[i], st2_values[i], st3_values[i]);
  if (cm->n_vc == 8) {
    fprintf(out, " V %12s %12s %12s %12s (ERROR)\n",
            "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
    for (int i = 0; i < cm->n_vc; i++)
      fprintf(out, "%2d % 10.6e % 10.6e % 10.6e % 10.6e\n",
              i, st0_values[i]-exact_result[i], st1_values[i]-exact_result[i],
              st2_values[i]-exact_result[i], st3_values[i]-exact_result[i]);
  }

  fprintf(out, "\nCDO.VB; PERFORMANCE OF SOURCE TERMS\n");
  fprintf(out, " %12s %12s %12s %12s\n",
          "DCSD_BARY", "DCSD_Q1O1", "DCSD_Q10O2", "DCSD_Q5O3");
  fprintf(out, " %10.6e %10.6e %10.6e %10.6e\n",
          tc0.wall_nsec*1e-9, tc1.wall_nsec*1e-9,
          tc2.wall_nsec*1e-9, tc3.wall_nsec*1e-9);

  /* Free memory */
  BFT_FREE(st);
  cs_cell_builder_free(&cb);
  cs_cell_sys_free(&csys);
}

/*----------------------------------------------------------------------------*/

int
main(int    argc,
     char  *argv[])
{
  CS_UNUSED(argc);
  CS_UNUSED(argv);

  hexa = fopen("CDO_Test_Hexa.log", "w");
  tetra = fopen("CDO_Test_Tetra.log", "w");

  /* connectivity */
  BFT_MALLOC(connect, 1, cs_cdo_connect_t);
  connect->n_max_vbyc = 8;
  connect->n_max_ebyc = 12;
  connect->n_max_fbyc = 6;
  connect->n_max_vbyf = 4;
  connect->v_max_cell_range = 8;
  connect->e_max_cell_range = 12;

  /* Nothing to do for quant */

  /* Time step */
  BFT_MALLOC(time_step, 1, cs_time_step_t);
  time_step->t_cur = 0.; // Useful when analytic function are called

  cs_source_term_set_shared_pointers(quant, connect, time_step);

  /* Allocate local structures */
  cs_cell_mesh_t  *cm = cs_cell_mesh_create(connect);
  cs_face_mesh_t  *fm = cs_face_mesh_create(connect->n_max_vbyf);

  /* ========= */
  /* TEST HEXA */
  /* ========= */

  _define_cm_hexa_unif(1., cm);

  cs_cell_bc_t  *cbc = cs_cell_bc_create(connect->n_max_vbyc,
                                         connect->n_max_vbyf);

  /* Initialize a cell view of the BC */
  cbc->n_dirichlet = 4;
  cbc->n_nhmg_neuman = cbc->n_robin = 0;
  cbc->n_dofs = cm->n_vc;
  for (short int i = 0; i < cm->n_vc; i++) {
    cbc->dof_flag[i] = 0;
    cbc->dir_values[i] = cbc->neu_values[i] = 0;
    cbc->rob_values[2*i] = cbc->rob_values[2*i+1] = 0.;
  }
  cbc->n_bc_faces = 1;
  cbc->bf_ids[0] = 4; //f_id = 4
  cbc->face_flag[0] = CS_CDO_BC_DIRICHLET;

  cs_face_mesh_build_from_cell_mesh(cm, 4, fm); //f_id = 4

  for (short int v = 0; v < fm->n_vf; v++) {
    cbc->dir_values[fm->v_ids[v]] = 1.0;
    cbc->dof_flag[fm->v_ids[v]] |= CS_CDO_BC_DIRICHLET;
  }

  _test_cdovb_schemes(hexa, cm, cbc, fm);

  /* ========== */
  /* TEST TETRA */
  /* ========== */

  _define_cm_tetra_ref(1., cm);

  /* Initialize a cell view of the BC */
  cbc->n_dirichlet = 3;
  cbc->n_nhmg_neuman = cbc->n_robin = 0;
  cbc->n_dofs = cm->n_vc;
  for (short int i = 0; i < cm->n_vc; i++) {
    cbc->dof_flag[i] = 0;
    cbc->dir_values[i] = cbc->neu_values[i] = 0;
    cbc->rob_values[2*i] = cbc->rob_values[2*i+1] = 0.;
  }
  cbc->n_bc_faces = 1;
  cbc->bf_ids[0] = 2; //f_id = 2
  cbc->face_flag[0] = CS_CDO_BC_DIRICHLET;

  cs_face_mesh_build_from_cell_mesh(cm, 2, fm); //f_id = 2

  for (short int v = 0; v < fm->n_vf; v++) {
    cbc->dir_values[fm->v_ids[v]] = 1.0;
    cbc->dof_flag[fm->v_ids[v]] |= CS_CDO_BC_DIRICHLET;
  }

  _test_cdovb_schemes(tetra, cm, cbc, fm);

  /* Free memory */
  cs_cell_mesh_free(&cm);
  cs_cell_bc_free(&cbc);
  cs_face_mesh_free(&fm);
  BFT_FREE(connect);
  BFT_FREE(time_step);

  fclose(hexa);
  fclose(tetra);

  printf(" --> CDO Tests (Done)\n");
  exit (EXIT_SUCCESS);
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
