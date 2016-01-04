/*============================================================================
 * Build discrete Hodge operators
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2016 EDF S.A.

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

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>
#include <bft_printf.h>

#include "cs_sort.h"
#include "cs_evaluate.h"
#include "cs_cdo_toolbox.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_hodge.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Additional doxygen documentation
 *============================================================================*/

/*!
  \file cs_hodge.c

  \brief Build discrete Hodge operators

*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*=============================================================================
 * Local Macro definitions and structure definitions
 *============================================================================*/

#define CS_HODGE_DBG 0

/* Main structure used to define a discrete Hodge operator */
struct _hodge_builder_t {

  cs_lnum_t   n_ent;             /* Number of entities */
  int         n_maxloc_ent;      /* Max local number of entities by primal
                                    cells (use for allocation) */

  cs_param_hodge_t  h_info;   /* Set of parameters related to the discrete
                                 Hodge operator to build. */

  cs_real_33_t      ptymat;   /* Tensor related to the material property.
                                 for EpFd, FpEd, EdFp Hodge op.
                                 Set by default to identity */

  cs_real_t         ptyval;   /* Value related to the material property
                                 for VpCd or CpVd hodge op.
                                 Set by default to unity */

  cs_locmat_t      *hloc;    /* Local dense matrix related to a local
                                discrete Hodge op. */

  void             *algoq;   /* Quantities used during the definition of
                                the local discrete Hodge op.
                                This structure is attached to each type
                                of algorithm */

};

/* Geometric quantities related to the construction of local discrete
   Hodge op. when the algo. is either COST (included DGA, SUSHI, Generalized
   Crouzeix-Raviart) and the type is  between edges and faces (primal or dual).
*/

struct _cost_quant_t {

  double     *invsvol;    /* size = n_ent*/
  double     *qmq;        /* symmetric dense matrix of size n_ent */
  double     *T;          /* dense matrix of size n_ent (not symmetric) */

  cs_nvec3_t   *pq;       /* primal geometric quantity (size: n_ent) */
  cs_nvec3_t   *dq;       /* dual geometric quantity (size: n_ent) */

};

/* Quantities related to the construction of a local discrete  Hodge op.
   when the Whitney Barycentric Subdivision algo. is employed.
   Used only for Vp --> Cd hodge (up to now) */

struct _wbs_quant_t {

  /* Link between local and non-local vertex ids */
  short int  *vtag; /* size = n_vertices; default value = -1 (not used)
                       otherwise local id in [0, n_ent] */

  /* Buffers storing the weights for each vertex (size: n_max_vbyc) */
  double   *wf;     /* weights related to each vertex of a given face */
  double   *wc;     /* weights related to each vertex of a given cell */


  int          bufsize;   /* size of the two following buffers */
  short int   *_v_ids;    /* List of couples of local vertex ids
                             for all the edges of a face */
  cs_lnum_t   *v_ids;     /* List of couples of vertex ids
                             for all the edges of a face */

};

/*============================================================================
 * Local variables
 *============================================================================*/

/*============================================================================
 * Private constant variables
 *============================================================================*/

static const double  invdim = 1./3.;

/*! \endcond (end ignore by Doxygen) */

/*============================================================================
 * Private function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Allocate and initialize by default the matrix related to a discrete
 *          Hodge op. based on vertices
 *          Note: values are filled in a second step
 *
 * \param[in]    connect   pointer to a cs_cdo_connect_t structure
 * \param[in]    quant     pointer to a cs_cdo_quantities_t structure
 *
 * \return a pointer to a cs_sla_matrix_t structure
 */
/*----------------------------------------------------------------------------*/

static cs_sla_matrix_t *
_init_hodge_vertex(const cs_cdo_connect_t     *connect,
                   const cs_cdo_quantities_t  *quant)
{
  int  i, j, shift;

  cs_connect_index_t  *v2v = NULL, *v2c = NULL;

  const cs_connect_index_t  *c2v = connect->c2v;
  const int  n_vertices = quant->n_vertices;

  /* Allocate and initialize the matrix */
  cs_sla_matrix_t  *h_mat = cs_sla_matrix_create(n_vertices,
                                                 n_vertices,
                                                 1,
                                                 CS_SLA_MAT_MSR,
                                                 false);

  /* Initialize index (v2v connectivity) */
  v2c = cs_index_transpose(n_vertices, c2v);
  v2v = cs_index_compose(n_vertices, v2c, c2v);
  cs_index_free(&v2c);

  cs_index_sort(v2v);
  h_mat->flag |= CS_SLA_MATRIX_SORTED;

  /* Update index */
  h_mat->idx[0] = 0;
  for (i = 0; i < n_vertices; i++)
    h_mat->idx[i+1] = h_mat->idx[i] + v2v->idx[i+1]-v2v->idx[i]-1;

  /* Fill column num */
  BFT_MALLOC(h_mat->col_id, h_mat->idx[n_vertices], cs_lnum_t);
  shift = 0;
  for (i = 0; i < n_vertices; i++)
    for (j = v2v->idx[i]; j < v2v->idx[i+1]; j++)
      if (v2v->ids[j] != i)
        h_mat->col_id[shift++] = v2v->ids[j];

  /* Sanity check */
  assert(shift == h_mat->idx[n_vertices]);

  /* Free temporary memory */
  cs_index_free(&v2v);

  /* Allocate and initialize value array */
  BFT_MALLOC(h_mat->val, h_mat->idx[n_vertices], double);
  for (i = 0; i < h_mat->idx[n_vertices]; i++)
    h_mat->val[i] = 0.0;

  for (i = 0; i < n_vertices; i++)
    h_mat->diag[i] = 0.0;

  return h_mat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Allocate and initialize by default the matrix related to a discrete
 *          Hodge op. based on edges
 *          Note: values are filled in a second step
 *
 * \param[in]    connect  pointer to a cs_cdo_connect_t structure
 * \param[in]    quant    pointer to a cs_cdo_quantities_t structure
 *
 * \return a pointer to a cs_sla_matrix_t structure
 */
/*----------------------------------------------------------------------------*/

static cs_sla_matrix_t *
_init_hodge_edge(const cs_cdo_connect_t     *connect,
                 const cs_cdo_quantities_t  *quant)
{
  int  c_id, eid, eid2, i, j, count, shift;

  int  *etag = NULL;
  cs_connect_index_t  *e2f =  NULL, *f2c = NULL, *e2c = NULL, *e2e = NULL;

  const int  n_edges = quant->n_edges;
  const int  n_cells = quant->n_cells;
  const cs_connect_index_t  *c2e = connect->c2e;

  /* Allocate and initialize the matrix */
  cs_sla_matrix_t  *h_mat = cs_sla_matrix_create(n_edges,
                                                 n_edges,
                                                 1,
                                                 CS_SLA_MAT_MSR,
                                                 false);

  /* Build a edge -> cell connectivity */
  e2f = cs_index_map(connect->e2f->n_rows,
                     connect->e2f->idx,
                     connect->e2f->col_id);
  f2c = cs_index_map(connect->f2c->n_rows,
                     connect->f2c->idx,
                     connect->f2c->col_id);

  e2c = cs_index_compose(n_cells, e2f, f2c);

  /* Count nnz in H */
  BFT_MALLOC(etag, n_edges, int);
  for (eid = 0; eid < n_edges; eid++)
    etag[eid] = -1;

  for (eid = 0; eid < n_edges; eid++) {

    count = 0;
    for (i = e2c->idx[eid]; i < e2c->idx[eid+1]; i++) {
      c_id = e2c->ids[i];
      for (j = c2e->idx[c_id]; j < c2e->idx[c_id+1]; j++) {
        eid2 = c2e->ids[j];
        if (eid != eid2 && etag[eid2] != eid) {
          etag[eid2] = eid;
          count++;
        }

      } /* Loop on edges sharing this cell */

    } /* Loop on cells sharing this edge (eid) */

    h_mat->idx[eid+1] = count;

  } /* End of loop on edges */

  /* Update index */
  for (i = 0; i < n_edges; i++)
    h_mat->idx[i+1] = h_mat->idx[i+1] + h_mat->idx[i];

  /* Fill column num */
  BFT_MALLOC(h_mat->col_id, h_mat->idx[n_edges], cs_lnum_t);
  for (i = 0; i < h_mat->idx[n_edges]; i++)
    h_mat->col_id[i] = -1;

  for (eid = 0; eid < n_edges; eid++)
    etag[eid] = -1;

  for (eid = 0; eid < n_edges; eid++) {

    shift = h_mat->idx[eid];
    for (i = e2c->idx[eid]; i < e2c->idx[eid+1]; i++) {

      c_id = e2c->ids[i];
      for (j = c2e->idx[c_id]; j < c2e->idx[c_id+1]; j++) {

        eid2 = c2e->ids[j];
        if (eid != eid2 && etag[eid2] != eid) {
          etag[eid2] = eid;
          h_mat->col_id[shift++] = eid2;
        }

      } /* Loop on edges sharing this cell */

    } /* Loop on cells sharing this edge (eid) */

  } /* End of loop on edges */

  /* Order column entries in increasing order */
  e2e = cs_index_map(h_mat->n_rows, h_mat->idx, h_mat->col_id);
  cs_index_sort(e2e);
  h_mat->flag |= CS_SLA_MATRIX_SORTED;

  /* Partial buffer free */
  BFT_FREE(etag);
  cs_index_free(&e2e); /* Not owner. Only delete the link with Hodge index */
  cs_index_free(&f2c);
  cs_index_free(&e2f);
  cs_index_free(&e2c);

  /* Allocate and initialize value array */
  for (i = 0; i < n_edges; i++)
    h_mat->diag[i] = 0.0;

  BFT_MALLOC(h_mat->val, h_mat->idx[h_mat->n_rows], double);
  for (i = 0; i < h_mat->idx[n_edges]; i++)
    h_mat->val[i] = 0.0;

  return h_mat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Allocate and initialize by default the matrix related to a discrete
 *          Hodge op. based on faces
 *          Note: values are filled in a second step
 *
 * \param[in]    connect   pointer to a cs_cdo_connect_t structure
 * \param[in]    quant     pointer to a cs_cdo_quantities_t structure
 *
 * \return a pointer to a cs_sla_matrix_t structure
 */
/*----------------------------------------------------------------------------*/

static cs_sla_matrix_t *
_init_hodge_face(const cs_cdo_connect_t     *connect,
                 const cs_cdo_quantities_t  *quant)
{
  int  f_id, j, shift;

  cs_connect_index_t  *f2f = NULL, *c2f = NULL, *f2c = NULL;

  const cs_lnum_t  n_faces = quant->n_faces;
  const cs_sla_matrix_t *mc2f = connect->c2f;
  const cs_sla_matrix_t *mf2c = connect->f2c;

  /* Allocate and initialize the matrix */
  cs_sla_matrix_t  *h_mat = cs_sla_matrix_create(n_faces,
                                                 n_faces,
                                                 1,
                                                 CS_SLA_MAT_MSR,
                                                 false);

  /* Build a face -> face connectivity */
  f2c = cs_index_map(mf2c->n_rows, mf2c->idx, mf2c->col_id);
  c2f = cs_index_map(mc2f->n_rows, mc2f->idx, mc2f->col_id);
  f2f = cs_index_compose(n_faces, f2c, c2f);
  cs_index_sort(f2f);
  h_mat->flag |= CS_SLA_MATRIX_SORTED;

  /* Update index: f2f has the diagonal entry. Remove it for the Hodge index */
  h_mat->idx[0] = 0;
  for (f_id = 0; f_id < n_faces; f_id++)
    h_mat->idx[f_id+1] = h_mat->idx[f_id] + f2f->idx[f_id+1]-f2f->idx[f_id]-1;

  /* Fill column num */
  BFT_MALLOC(h_mat->col_id, h_mat->idx[n_faces], cs_lnum_t);
  shift = 0;
  for (f_id = 0; f_id < n_faces; f_id++)
    for (j = f2f->idx[f_id]; j < f2f->idx[f_id+1]; j++)
      if (f2f->ids[j] != f_id)
        h_mat->col_id[shift++] = f2f->ids[j];

  /* Sanity check */
  assert(shift == h_mat->idx[n_faces]);

  /* Free temporary memory */
  cs_index_free(&f2f);
  cs_index_free(&f2c);
  cs_index_free(&c2f);

  /* Allocate and initialize value array */
  for (f_id = 0; f_id < n_faces; f_id++)
    h_mat->diag[f_id] = 0.0;

  BFT_MALLOC(h_mat->val, h_mat->idx[n_faces], double);
  for (f_id = 0; f_id < h_mat->idx[n_faces]; f_id++)
    h_mat->val[f_id] = 0.0;

  return h_mat;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Allocate and initialize a _cost_quant_t structure
 *
 * \param[in]  n_max_ent    max number of entities by primal cell
 *
 * \return  a pointer to a new allocated _cost_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static struct _cost_quant_t *
_init_cost_quant(int    n_max_ent)
{
   struct _cost_quant_t *hq = NULL;

  BFT_MALLOC(hq, 1, struct _cost_quant_t);

  hq->invsvol = NULL;
  hq->qmq = NULL;
  hq->T = NULL;
  hq->pq = NULL;
  hq->dq = NULL;

  if (n_max_ent > 0) {

    int  msize = n_max_ent*n_max_ent;
    int  tot_size = n_max_ent + 2*msize;

    /* Allocate invsvol with the total requested size and then reference
       other pointers from this one */
    BFT_MALLOC(hq->invsvol, tot_size, double);
    for (int i = 0; i < tot_size; i++)
      hq->invsvol[i] = 0;

    hq->qmq = hq->invsvol + n_max_ent;
    hq->T = hq->invsvol + n_max_ent + msize;

    BFT_MALLOC(hq->pq, n_max_ent, cs_nvec3_t);
    BFT_MALLOC(hq->dq, n_max_ent, cs_nvec3_t);

  }

  return hq;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Free a cs_hodge_costq_t structure
 *
 * \param[in]  hq    pointer to the cs_hodge_costq_t struct. to free
 *
 * \return  a NULL pointer
 */
/*----------------------------------------------------------------------------*/

static struct _cost_quant_t *
_free_cost_quant(struct _cost_quant_t  *hq)
{
  if (hq == NULL)
    return hq;

  BFT_FREE(hq->invsvol); /* Free in the same time qmq and T */
  BFT_FREE(hq->pq);
  BFT_FREE(hq->dq);

  BFT_FREE(hq);

  return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute quantities used for defining the entries of the discrete
 *          Hodge for COST algo. and edge/face quantities
 *
 * \param[in]      n_loc_ent  number of local entities
 * \param[in]      ptymat     values of the tensor related to the material pty
 * \param[in]      pq         pointer to the first set of quantities
 * \param[in]      dq         pointer to the second set of quantities
 * \param[in, out] hq         pointer to a _cost_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_compute_cost_quant(int                     n_loc_ent,
                    const cs_real_33_t      ptymat,
                    const cs_nvec3_t       *pq,
                    const cs_nvec3_t       *dq,
                    struct _cost_quant_t   *hq)
{
  int  i, j, ii, ij, ji, jj;
  double  dpq, tmp_val;
  cs_real_3_t  mdq_i;

  /* Compute T and qmq matrices */
  for (i = 0; i < n_loc_ent; i++) {

    /* Compute invsvol related to each entity */
    dpq = pq[i].meas*dq[i].meas * _dp3(dq[i].unitv, pq[i].unitv);
    hq->invsvol[i] = 3./dpq; /* 1/subvol where subvol = 1/d * dpq */

    /* Compute diagonal entries */
    _mv3(ptymat, dq[i].unitv, mdq_i);
    ii = i*n_loc_ent+i;
    hq->qmq[ii] = dq[i].meas * dq[i].meas * _dp3(dq[i].unitv, mdq_i);
    hq->T[ii] = dpq;

    for (j = i+1; j < n_loc_ent; j++) {

      ij = i*n_loc_ent+j, ji = j*n_loc_ent+i, jj = j*n_loc_ent+j;

      /* Compute qmq (symmetric) */
      tmp_val = dq[j].meas * dq[i].meas * _dp3(dq[j].unitv, mdq_i);
      hq->qmq[ji] = hq->qmq[ij] = tmp_val;

      /* Compute T (not symmetric) */
      hq->T[ij] = dq[i].meas*pq[j].meas * _dp3(dq[i].unitv, pq[j].unitv);
      hq->T[ji] = dq[j].meas*pq[i].meas * _dp3(dq[j].unitv, pq[i].unitv);

    } /* Loop on entities (J) */

  } /* Loop on entities (I) */

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a local discrete Hodge operator using the generic COST algo.
 *
 * \param[in]      cid        cell id
 * \param[in]      connect    pointer to a cs_cdo_connect_t struct.
 * \param[in]      quant      pointer to a cs_cdo_quantities_t struct.
 * \param[in, out] hb         pointer to a cs_hodge_builder_t struct.
 * \param[in, out] hq         pointer to a _cost_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_build_using_cost(int                         cid,
                  const cs_cdo_connect_t     *connect,
                  const cs_cdo_quantities_t  *quant,
                  cs_hodge_builder_t         *hb,
                  struct _cost_quant_t       *hq)
{
  int  i, j, k;
  double  invsurf;

  int  n_ent = 0;
  cs_locmat_t  *hloc = hb->hloc;

  const cs_param_hodge_t  h_info = hb->h_info;

  /* Set numbering and geometrical quantities Hodge builder */
  switch (h_info.type) {

  case CS_PARAM_HODGE_TYPE_EPFD:
    {
      const cs_connect_index_t  *c2e = connect->c2e;

      for (i = c2e->idx[cid]; i < c2e->idx[cid+1]; i++) {

        const cs_lnum_t  e_id = c2e->ids[i];
        const cs_dface_t  fd = quant->dface[i];   /* Dual face quantities */
        const cs_quant_t  ep = quant->edge[e_id]; /* Edge quantities */
        const cs_nvec3_t  df0q = fd.sface[0];
        const cs_nvec3_t  df1q = fd.sface[1];

        hloc->ids[n_ent] = e_id;

        /* Primal and dual vector quantities are split into
           a measure and a unit vector in order to achieve a better accuracy */
        hq->pq[n_ent].meas = ep.meas;
        hq->dq[n_ent].meas = df0q.meas + df1q.meas;
        invsurf = 1/hq->dq[n_ent].meas;
        for (k = 0; k < 3; k++) {
          hq->dq[n_ent].unitv[k] = invsurf * fd.vect[k];
          hq->pq[n_ent].unitv[k] = ep.unitv[k];
        }
        n_ent++;

      } /* Loop on cell edges */

    }
    break;

  case CS_PARAM_HODGE_TYPE_FPED:
    {
      const cs_sla_matrix_t  *c2f = connect->c2f;

      for (i = c2f->idx[cid]; i < c2f->idx[cid+1]; i++) {

        const cs_lnum_t  f_id = c2f->col_id[i];
        const cs_nvec3_t  ed = quant->dedge[i]; /* Dual edge quantities */
        const cs_quant_t  fp = quant->face[f_id];  /* Face quantities */

        hloc->ids[n_ent] = f_id;

        /* Primal and dual vector quantities are split into
           a measure and a unit vector in order to achieve a better accuracy */
        hq->dq[n_ent].meas = ed.meas;
        hq->pq[n_ent].meas = fp.meas;
        for (k = 0; k < 3; k++) {
          hq->pq[n_ent].unitv[k] = fp.unitv[k];
          hq->dq[n_ent].unitv[k] = ed.unitv[k];
        }
        n_ent++;

      } /* Loop on cell faces */

    }
    break;

  case CS_PARAM_HODGE_TYPE_EDFP:
    {
      const cs_sla_matrix_t  *c2f = connect->c2f;

      for (i = c2f->idx[cid]; i < c2f->idx[cid+1]; i++) {

        const cs_lnum_t  f_id = c2f->col_id[i];
        const short int  sgn = c2f->sgn[i];
        const cs_nvec3_t  ed = quant->dedge[i];    /* Dual edge quantities */
        const cs_quant_t  fp = quant->face[f_id];  /* Face quantities */

        hloc->ids[n_ent] = f_id;

        /* Primal and dual vector quantities are split into
           a measure and a unit vector in order to achieve a better accuracy */
        hq->dq[n_ent].meas = ed.meas;
        hq->pq[n_ent].meas = fp.meas;
        for (k = 0; k < 3; k++) {
          hq->pq[n_ent].unitv[k] = sgn*fp.unitv[k];
          hq->dq[n_ent].unitv[k] = sgn*ed.unitv[k];
        }
        n_ent++;

      } /* Loop on cell faces */

    }
    break;

  default:
    bft_error(__FILE__, __LINE__, 0,
              _(" This type of discrete Hodge operator is not covered.\n"));

  } /* End of switch */

  /* Sanity checks */
  assert(n_ent < hloc->n_max_ent + 1);
  assert(n_ent == hloc->n_ent);

  /* Compute additional geometrical quantities: invsvol, qmq and T
     Switch arguments between discrete Hodge operator from PRIMAL->DUAL space
     and discrete Hodge operator from DUAL->PRIMAL space */

  /* PRIMAL --> DUAL */
  if (h_info.type == CS_PARAM_HODGE_TYPE_FPED ||
      h_info.type == CS_PARAM_HODGE_TYPE_EPFD)
    _compute_cost_quant(n_ent, (const cs_real_3_t *)hb->ptymat,
                        hq->pq, hq->dq, hq);

  /* DUAL --> PRIMAL */
  else if (h_info.type == CS_PARAM_HODGE_TYPE_EDFP)
    _compute_cost_quant(n_ent, (const cs_real_3_t *)hb->ptymat,
                        hq->dq, hq->pq, hq);

  /* Coefficients related to the value of beta */
  const double  beta = h_info.coef;
  const double  beta2 = beta*beta;
  const double  invcvol = 1 / quant->cell_vol[cid];
  const double  invcvol2 = invcvol*invcvol;
  const double  coef1 = beta*invcvol2;
  const double  coef2 = (1+ 2*beta)*invcvol;
  const double  coef3 = coef2 - 6*beta2*invcvol;
  const double  coef4 = beta2*invcvol;

  /* Add contribution from each sub-volume related to each edge */
  for (k = 0; k < n_ent; k++) { /* Loop over sub-volumes */

    int  kk =  k*n_ent+k;
    double  val_kk = beta * hq->qmq[kk] * hq->invsvol[k];

    for (i = 0; i < n_ent; i++) { /* Loop on cell entities I */

      int  ik = i*n_ent+k;
      double  Tik = hq->T[ik];

      hloc->mat[i*n_ent+i] += Tik*(Tik*val_kk - 2*hq->qmq[ik]);

      for (j = i + 1; j < n_ent; j++) { /* Loop on cell entities J */

        int  jk = j*n_ent+k, ij = i*n_ent+j;
        double  Tjk = hq->T[jk];

        hloc->mat[ij] += Tik*Tjk*val_kk - Tik*hq->qmq[jk] - Tjk*hq->qmq[ik];

      } /* End of loop on J entities */

    } /* End of loop on I entities */

  } /* End of loop on P sub-regions */

  /* Add contribution independent of the sub-region */
  for (i = 0; i < n_ent; i++) {/* Loop on cell entities I */

    int  ii = i*n_ent+i;
    double  miis = hq->qmq[ii]*hq->invsvol[i];

    hloc->mat[ii] = coef1*hloc->mat[ii] + coef3*hq->qmq[ii] + beta2*miis;

    for (j = i + 1; j < n_ent; j++) { /* Loop on cell entities J */

      int  jj = j*n_ent+j, ij = i*n_ent+j, ji = j*n_ent+i;
      double  mjjs = hq->qmq[jj]*hq->invsvol[j];
      double  contrib =  hq->T[ij]*mjjs + hq->T[ji]*miis;

      hloc->mat[ij] = coef1*hloc->mat[ij] + coef2*hq->qmq[ij] - coef4*contrib;
      hloc->mat[ji] = hloc->mat[ij];

    } /* End of loop on J entities */

  } /* End of loop on I entities */

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a structure used to compute a discrete Hodge op. when using
 *          WBS algo.
 *
 * \param[in]   n_ent_max     max number of local entities
 * \param[in]   aux_bufsize   size of the auxiliary buffers
 * \param[in]   n_vertices    number of vertices in this mesh
 *
 * \return a pointer to a _wbs_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static struct _wbs_quant_t *
_init_wbs_quant(int        n_ent_max,
                int        aux_bufsize,
                cs_lnum_t  n_vertices)
{
  cs_lnum_t  i;

  struct _wbs_quant_t  *hq = NULL;

  /* Allocate structure */
  BFT_MALLOC(hq, 1, struct _wbs_quant_t);

  /* Weights */
  BFT_MALLOC(hq->wf, 2*n_ent_max, double);
  for (i = 0; i < 2*n_ent_max; i++)
    hq->wf[i] = 0;
  hq->wc = hq->wf + n_ent_max;

  /* Correspondance between local and non-local vertex ids */
  BFT_MALLOC(hq->vtag, n_vertices, short int);
  for (i = 0; i < n_vertices; i++)
    hq->vtag[i] = -1;

  /* Store local and non local Vertex ids */
  hq->bufsize = aux_bufsize;
  BFT_MALLOC(hq->_v_ids, 2*aux_bufsize, short int);
  BFT_MALLOC(hq->v_ids, 2*aux_bufsize, cs_lnum_t);

  for (i = 0; i < 2*aux_bufsize; i++) {
    hq->v_ids[i] = -1;
    hq->_v_ids[i] = -1;
  }

  return  hq;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Free a structure used to compute a discrete Hodge op. when using
 *          WBS algo.
 *
 * \param[in]   hq     pointer to a _wbs_quant_t structure
 *
 * \return a NULL pointer
 */
/*----------------------------------------------------------------------------*/

static struct _wbs_quant_t *
_free_wbs_quant(struct _wbs_quant_t  *hq)
{
  if (hq == NULL)
    return hq;

  BFT_FREE(hq->wf); /* Deallocate in the same time hvq->wc and hvq->cumul */
  BFT_FREE(hq->vtag);
  BFT_FREE(hq->v_ids);
  BFT_FREE(hq->_v_ids);

  BFT_FREE(hq);

  return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute for each face a weight related to each vertex w_{v,f}
 *         This weight is equal to |dc(v) cap f|/|f| so that the sum of the
 *         weights is equal to 1.
 *         Set also the local and local numbering of the vertices of this face
 *
 * \param[in]      f_id      id of the face
 * \param[in]      connect   pointer to a cs_cdo_connect_t structure
 * \param[in]      quant     pointer to a cs_cdo_quantites_t structure
 * \param[in, out] hq        pointer to a _wbs_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_compute_wbs_face_quant(cs_lnum_t                   f_id,
                        const cs_cdo_connect_t     *connect,
                        const cs_cdo_quantities_t  *quant,
                        struct _wbs_quant_t        *hq)
{
  cs_lnum_t  i, shift = 0;
  double  contrib, len;
  cs_real_3_t  un, cp;

  const short int  *loc_ids = hq->vtag;
  const cs_quant_t  fq = quant->face[f_id];
  const double  f_coef = 0.25/fq.meas;

  /* Reset weights */
  for (i = 0; i < connect->n_max_vbyc; i++) hq->wf[i] = 0;

  /* Compute a weight for each vertex of the current face */
  for (i = connect->f2e->idx[f_id]; i < connect->f2e->idx[f_id+1]; i++) {

    const cs_lnum_t  e_id = connect->f2e->col_id[i];
    const cs_quant_t  eq = quant->edge[e_id];

    const cs_lnum_t  v1_id = connect->e2v->col_id[2*e_id];
    const cs_lnum_t  v2_id = connect->e2v->col_id[2*e_id+1];
    const short int  _v1 = loc_ids[v1_id];
    const short int  _v2 = loc_ids[v2_id];

    _lenunit3(eq.center, fq.center, &len, &un);
    _cp3(un, eq.unitv, &cp);
    contrib = eq.meas * len * _n3(cp) * f_coef;

    hq->wf[_v1] += contrib;
    hq->wf[_v2] += contrib;
    hq->v_ids[2*shift] = v1_id;
    hq->v_ids[2*shift+1] = v2_id;
    hq->_v_ids[2*shift] = _v1;
    hq->_v_ids[2*shift+1] = _v2;

    shift++;

  } /* End of loop on face edges */

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a local discrete Hodge operator using a conforming algo.
 *          based on the barycentric subdivision of a polyhedron.
 *          This construction is cellwise.
 *          Note: the local matrix is stored inside hb->hloc
 *
 * \param[in]      cid        cell id
 * \param[in]      connect    pointer to a cs_cdo_connect_t struct.
 * \param[in]      quant      pointer to a cs_cdo_quantities_t struct.
 * \param[in, out] hb         pointer to a cs_hodge_builder_t struct.
 * \param[in, out] hq         pointer to a _wbs_quant_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_build_using_wbs(int                         cid,
                 const cs_cdo_connect_t     *connect,
                 const cs_cdo_quantities_t  *quant,
                 cs_hodge_builder_t         *hb,
                 struct _wbs_quant_t        *hq)
{
  short int  i, j, k, n_ent;
  double  val, wic, wjc, wif, wjf;
  cs_real_3_t  xc;
  cs_lnum_t  ii, jj, v_id;

  cs_locmat_t  *hl = hb->hloc;

  const double  volc = quant->cell_vol[cid];
  const double  c_coef = 0.1*volc;
  const double  ovcell = 1/volc;
  const cs_real_t  *xyz = quant->vtx_coord;
  const cs_connect_index_t  *c2v = connect->c2v;
  const cs_lnum_t  vshift = c2v->idx[cid];
  const cs_sla_matrix_t  *c2f = connect->c2f;
  const cs_sla_matrix_t  *f2e = connect->f2e;

  /* Sanity check */
  assert(hb->h_info.algo == CS_PARAM_HODGE_ALGO_WBS);
  assert(hb->h_info.type == CS_PARAM_HODGE_TYPE_VPCD);

  /* Local initializations */
  for (n_ent = 0, ii = vshift; ii < c2v->idx[cid+1]; n_ent++, ii++) {
    v_id = c2v->ids[ii];
    hl->ids[n_ent] = v_id;
    hq->vtag[v_id] = n_ent;
    hq->wc[n_ent] = ovcell*quant->dcell_vol[ii];
  }

  /* Sanity checks */
  assert(hl->n_ent == n_ent);
  assert(hl->n_ent <= hl->n_max_ent);

  for (k = 0; k < 3; k++)
    xc[k] = quant->cell_centers[3*cid+k];

  /* Initialize the upper part of the local Hodge matrix */
  for (i = 0; i < n_ent; i++) {

    int  shift_i = i*n_ent;

    /* Diagonal entry */
    wic = hq->wc[i];
    hl->mat[shift_i+i] = c_coef*wic*wic;

    /* Extra-diagonal entries */
    for (j = i+1; j < n_ent; j++) {
      wjc = hq->wc[j];
      hl->mat[shift_i+j] = c_coef*wic*wjc;
    }

  } // Loop on cell vertices

  /* Loop on each pef and add the contribution */
  for (ii = c2f->idx[cid]; ii < c2f->idx[cid+1]; ii++) {

    const cs_lnum_t  f_id = c2f->col_id[ii];
    const cs_quant_t  pfq = quant->face[f_id];

    int  e_shift = 0;

    /* Compute a weight for each vertex of the current face */
    _compute_wbs_face_quant(f_id, connect, quant, hq);

    for (jj = f2e->idx[f_id]; jj < f2e->idx[f_id+1]; jj++, e_shift++) {

      const cs_lnum_t  v1_id = hq->v_ids[2*e_shift];
      const cs_lnum_t  v2_id = hq->v_ids[2*e_shift+1];
      const short int  _v1 = hq->_v_ids[2*e_shift];
      const short int  _v2 = hq->_v_ids[2*e_shift+1];

      /* Sanity check */
      assert(_v1 > -1 && _v2 > -1);

      const cs_real_t  pef_vol = cs_voltet(&(xyz[3*v1_id]),
                                     &(xyz[3*v2_id]),
                                     pfq.center,
                                     xc);
      const cs_real_t  w_vol = 0.05*pef_vol;

      /* Add local contribution */
      for (i = 0; i < n_ent; i++) {

        const int  shift_i = i*n_ent;
        const bool  iyes = (i == _v1 || i == _v2) ? true : false;

        wic = hq->wc[i];
        wif = hq->wf[i];
        val = 2*wif*(wif + wic);
        if (iyes)
          val += 2*(1 + wic + wif);
        val *= w_vol;    /* 1/20 * |tet| (cf. Rapport HI-A7/7561 in 1991) */

        /* Diagonal entry: i=j */
        hl->mat[shift_i+i] += val;

        /* Extra-diagonal entries */
        for (j = i+1; j < n_ent; j++) {

          const bool  jyes = (j == _v1 || j == _v2) ? true : false;

          wjc = hq->wc[j];
          wjf = hq->wf[j];
          val = 2*wif*wjf + wif*wjc + wic*wjf;
          if (iyes)
            val += wjf + wjc;
          if (jyes)
            val += wif + wic;
          if (iyes && jyes)
            val += 1;
          val *= w_vol;

          hl->mat[shift_i+j] += val;

        } // Extra-diag entries

      } // Loop on cell vertices

    } // Loop on face edges

  } // Loop on cell faces

  /* Take into account the value of the associated property */
  if (fabs(hb->ptyval - 1.0) > cs_get_eps_machine()) {
    for (i = 0; i < n_ent; i++) {
      int  shift_i = i*n_ent;
      for (j = i; j < n_ent; j++)
        hl->mat[shift_i + j] *= hb->ptyval;
    }
  }

  /* Local matrix is symmetric by construction. Set the lower part. */
  for (j = 0; j < n_ent; j++) {
    int  shift_j = j*n_ent;
    for (i = j+1; i < n_ent; i++)
      hl->mat[i*n_ent+j] = hl->mat[shift_j+i];
  }

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a local discrete Hodge op. using the Voronoi algo.
 *
 * \param[in]       c_id      cell id
 * \param[in]       connect   pointer to a cs_cdo_connect_t struct.
 * \param[in]       quant     pointer to a cs_cdo_quantities_t struct.
 * \param[in, out]  hb        pointer to a cs_hodge_builder_t struct.
 */
/*----------------------------------------------------------------------------*/

static void
_build_using_voronoi(cs_lnum_t                    c_id,
                     const cs_cdo_connect_t      *connect,
                     const cs_cdo_quantities_t   *quant,
                     cs_hodge_builder_t          *hb)
{
  int  ii;
  cs_lnum_t  i;
  double  contrib;
  cs_real_3_t  mv;

  cs_locmat_t  *hl = hb->hloc;

  const cs_param_hodge_t  h_info = hb->h_info;

  switch (h_info.type) {

  case CS_PARAM_HODGE_TYPE_EPFD:
    {
      const cs_connect_index_t  *c2e = connect->c2e;

      /* Loop on cell edges */
      for (i = c2e->idx[c_id], ii = 0; i < c2e->idx[c_id+1]; i++, ii++) {

        cs_dface_t  dfq = quant->dface[i];
        cs_nvec3_t  df0q = dfq.sface[0], df1q = dfq.sface[1];
        cs_lnum_t  e_id = c2e->ids[i];
        cs_real_t  len = quant->edge[e_id].meas;

        hl->ids[ii] = e_id;

        /* First sub-triangle contribution */
        _mv3((const cs_real_3_t *)hb->ptymat, df0q.unitv, mv);
        contrib = df0q.meas * _dp3(mv, df0q.unitv);
        /* Second sub-triangle contribution */
        _mv3((const cs_real_3_t *)hb->ptymat, df1q.unitv, mv);
        contrib += df1q.meas * _dp3(mv, df1q.unitv);

        /* Only a diagonal term */
        hl->mat[ii*hl->n_ent+ii] = contrib/len;

      } /* End of loop on cell edges */

    } /* EpFd */
  case CS_PARAM_HODGE_TYPE_FPED:
    {
      const cs_sla_matrix_t *c2f = connect->c2f;

      for (i = c2f->idx[c_id], ii = 0; i < c2f->idx[c_id+1]; i++, ii++) {

        cs_nvec3_t  deq = quant->dedge[i];
        cs_lnum_t  f_id = c2f->col_id[i];
        cs_real_t  surf = quant->face[f_id].meas;

        hl->ids[ii] = f_id;
        /* Only a diagonal term */
        _mv3((const cs_real_3_t *)hb->ptymat, deq.unitv, mv);
        hl->mat[ii*hl->n_ent+ii] = deq.meas * _dp3(mv, deq.unitv) / surf;

      } /* End of loop on cell faces */

    } /* FpEd */

  case CS_PARAM_HODGE_TYPE_VPCD:
    {
      const cs_connect_index_t  *c2v = connect->c2v;

      for (i = c2v->idx[c_id], ii = 0; i < c2v->idx[c_id+1]; i++, ii++) {

        hl->ids[ii] = c2v->ids[i];
        /* Only a diagonal term */
        hl->mat[ii*hl->n_ent+ii] = hb->ptyval * quant->dcell_vol[i];

      } // Loop on cell vertices

    } /* VpCd */
  default:
    break;

  } // End of switch

}

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Allocate and initialize a cs_hodge_builder_t structure
 *
 * \param[in]  connect       pointer to a cs_cdo_connect_t struct.
 * \param[in]  h_info        algorithm used to build the discrete Hodge op.
 *
 * \return  a new allocated cs_hodge_builder_t structure
 */
/*----------------------------------------------------------------------------*/

cs_hodge_builder_t *
cs_hodge_builder_init(const cs_cdo_connect_t   *connect,
                      cs_param_hodge_t          h_info)
{
  cs_hodge_builder_t  *hb = NULL;

  BFT_MALLOC(hb, 1, cs_hodge_builder_t);

  switch (h_info.type) {

  case CS_PARAM_HODGE_TYPE_VPCD:
    hb->n_maxloc_ent = connect->n_max_vbyc;
    hb->n_ent = connect->v_info->n_ent;
    break;
  case CS_PARAM_HODGE_TYPE_EPFD:
    hb->n_maxloc_ent = connect->n_max_ebyc;
    hb->n_ent = connect->e_info->n_ent;
    break;
  case CS_PARAM_HODGE_TYPE_FPED:
  case CS_PARAM_HODGE_TYPE_EDFP:
    hb->n_maxloc_ent = connect->n_max_fbyc;
    hb->n_ent = connect->f_info->n_ent;
    break;
  default:
    hb->n_ent = 0;
    hb->n_maxloc_ent = 0;
    break;

  }

  /* Allocate the local dense matrix storing the coefficient of the local
     discrete Hodge op. associated to a cell */
  hb->hloc = cs_locmat_create(hb->n_maxloc_ent);

  /* Allocate the structure used to stored quantities used during the build
     of the local discrete Hodge op. */
  switch (h_info.algo) {

  case CS_PARAM_HODGE_ALGO_COST:
    hb->algoq = _init_cost_quant(hb->n_maxloc_ent);
    break;

  case CS_PARAM_HODGE_ALGO_WBS:
    assert(h_info.type == CS_PARAM_HODGE_TYPE_VPCD);
    hb->algoq = _init_wbs_quant(hb->n_maxloc_ent,
                                2*connect->n_max_ebyc,
                                hb->n_ent);
    break;

  default:
    hb->algoq = NULL;
    break;

  }

  hb->h_info.inv_pty = h_info.inv_pty;
  hb->h_info.type    = h_info.type;
  hb->h_info.algo    = h_info.algo;
  hb->h_info.coef    = h_info.coef;

  /* Initialize by default the property values */
  hb->ptymat[0][0] = 1., hb->ptymat[0][1] = hb->ptymat[0][2] = 0.;
  hb->ptymat[1][1] = 1., hb->ptymat[1][0] = hb->ptymat[1][2] = 0.;
  hb->ptymat[2][2] = 1., hb->ptymat[2][1] = hb->ptymat[2][0] = 0.;
  hb->ptyval = 1.0; /* for VpCd, CpVd */

  return hb;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Free a cs_hodge_builder_t structure
 *
 * \param[in]  hb    pointer to the cs_hodge_builder_t struct. to free
 *
 * \return  a NULL pointer
 */
/*----------------------------------------------------------------------------*/

cs_hodge_builder_t *
cs_hodge_builder_free(cs_hodge_builder_t  *hb)
{
  if (hb == NULL)
    return hb;

  hb->hloc = cs_locmat_free(hb->hloc);

  switch (hb->h_info.algo) {
  case CS_PARAM_HODGE_ALGO_COST:
    hb->algoq = _free_cost_quant((struct _cost_quant_t *)hb->algoq);
    break;

  case CS_PARAM_HODGE_ALGO_WBS:
    hb->algoq = _free_wbs_quant((struct _wbs_quant_t *)hb->algoq);
    break;

  default:
    hb->algoq = NULL;
    break;
  }

  BFT_FREE(hb);

  return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Set the value of the property attached to a hodge builder
 *
 * \param[in, out]  hb       pointer to a cs_hodge_builder_t structure
 * \param[in]       ptyval   value of the property
 */
/*----------------------------------------------------------------------------*/

void
cs_hodge_builder_set_val(cs_hodge_builder_t    *hb,
                         cs_real_t              ptyval)
{
  if (hb == NULL)
    return;

  hb->ptyval = ptyval;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Set the value of the property attached to a hodge builder
 *
 * \param[in, out]  hb       pointer to a cs_hodge_builder_t structure
 * \param[in]       ptymat   values of the tensor related to a property
 */
/*----------------------------------------------------------------------------*/

void
cs_hodge_builder_set_tensor(cs_hodge_builder_t     *hb,
                            const cs_real_33_t      ptymat)
{
  if (hb == NULL)
    return;

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      hb->ptymat[i][j] = ptymat[i][j];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a local discrete Hodge
 *
 * \param[in]      c_id       cell id
 * \param[in]      connect    pointer to a cs_cdo_connect_t struct.
 * \param[in]      quant      pointer to a cs_cdo_quantities_t struct.
 * \param[in, out] hb         pointer to a cs_hodge_builder_t struct.
 *
 * \return a pointer to a cs_locmat_t struct. (local dense matrix)
 */
/*----------------------------------------------------------------------------*/

cs_locmat_t *
cs_hodge_build_local(int                         c_id,
                     const cs_cdo_connect_t     *connect,
                     const cs_cdo_quantities_t  *quant,
                     cs_hodge_builder_t         *hb)
{
  int n_ent;

  /* Sanity check */
  assert(hb != NULL);

  const cs_param_hodge_t  h_info = hb->h_info;

  /* Set n_ent and reset local hodge matrix */
  switch (h_info.type) {

  case CS_PARAM_HODGE_TYPE_VPCD:
    n_ent = connect->c2v->idx[c_id+1] - connect->c2v->idx[c_id];
    break;
  case CS_PARAM_HODGE_TYPE_EPFD:
    n_ent = connect->c2e->idx[c_id+1] - connect->c2e->idx[c_id];
    break;
  case CS_PARAM_HODGE_TYPE_FPED:
  case CS_PARAM_HODGE_TYPE_EDFP:
    n_ent = connect->c2f->idx[c_id+1] - connect->c2f->idx[c_id];
    break;
  case CS_PARAM_HODGE_TYPE_CPVD:
    n_ent = 1;
    break;
  default:
    bft_error(__FILE__, __LINE__, 0,
              " Invalid type of discrete Hodge operator.");
  }

  hb->hloc->n_ent = n_ent;
  for (int i = 0; i < n_ent*n_ent; i++)
    hb->hloc->mat[i] = 0;

  /* Switch according to the requested type of algorithm to use */
  switch (h_info.algo) {

  case CS_PARAM_HODGE_ALGO_COST:
    _build_using_cost(c_id, connect, quant, hb,
                      (struct _cost_quant_t *)hb->algoq);
    break;

  case CS_PARAM_HODGE_ALGO_WBS:
    _build_using_wbs(c_id, connect, quant, hb,
                     (struct _wbs_quant_t *)hb->algoq);
    break;

  case CS_PARAM_HODGE_ALGO_VORONOI:
    _build_using_voronoi(c_id, connect, quant, hb);
    break;

  default:
    break;

  }

#if CS_HODGE_DBG > 2
  cs_locmat_dump(c_id, hb->hloc);
#endif

  return hb->hloc;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Build a discrete Hodge operator
 *
 * \param[in]  connect    pointer to a cs_cdo_connect_t struct.
 * \param[in]  quant      pointer to a cs_cdo_quantities_t struct.
 * \param[in]  pty        pointer to a cs_property_t struct.
 * \param[in]  h_info     pointer to a cs_param_hodge_t struct.
 *
 * \return a pointer to a cs_sla_matrix_t structure
 */
/*----------------------------------------------------------------------------*/

cs_sla_matrix_t *
cs_hodge_compute(const cs_cdo_connect_t      *connect,
                 const cs_cdo_quantities_t   *quant,
                 const cs_property_t         *pty,
                 const cs_param_hodge_t       h_info)
{
  bool  only_diag = (h_info.algo == CS_PARAM_HODGE_ALGO_VORONOI) ? true : false;
  cs_sla_matrix_t  *h_mat = NULL;

  /* Allocate and initialize a cs_hodge_builder_t structure */
  cs_hodge_builder_t  *hb = cs_hodge_builder_init(connect, h_info);

  bool  update_pty = true;
  bool  pty_is_uniform = true;

  if (pty == NULL)
    update_pty = false;
  else
    pty_is_uniform = cs_property_is_uniform(pty);

  switch (h_info.type) {

  case CS_PARAM_HODGE_TYPE_VPCD:
    if (h_info.algo == CS_PARAM_HODGE_ALGO_VORONOI)
      h_mat = cs_sla_matrix_create(quant->n_vertices, quant->n_vertices, 1,
                                   CS_SLA_MAT_MSR, false);
    else
      h_mat = _init_hodge_vertex(connect, quant);
    break;

  case CS_PARAM_HODGE_TYPE_EPFD:
    if (h_info.algo == CS_PARAM_HODGE_ALGO_VORONOI)
      h_mat = cs_sla_matrix_create(quant->n_edges, quant->n_edges, 1,
                                   CS_SLA_MAT_MSR, false);
    else
      h_mat = _init_hodge_edge(connect, quant);
    break;

  case CS_PARAM_HODGE_TYPE_FPED:
  case CS_PARAM_HODGE_TYPE_EDFP:
    if (h_info.algo == CS_PARAM_HODGE_ALGO_VORONOI)
      h_mat = cs_sla_matrix_create(quant->n_faces, quant->n_faces, 1,
                                   CS_SLA_MAT_MSR, false);
    else
      h_mat = _init_hodge_face(connect, quant);
    break;

  default:
    bft_error(__FILE__, __LINE__, 0,
              _(" Invalid type of Hodge operator.\n"));


  } /* End switch */

  /* Fill the matrix related to the discrete Hodge operator.
     Proceed cellwise and then perform an assembly. */
  for (cs_lnum_t c_id = 0; c_id < quant->n_cells; c_id++) {

    /* Update the value of the property if needed */
    if (update_pty) {
      if (h_info.type == CS_PARAM_HODGE_TYPE_VPCD)
        hb->ptyval = cs_property_get_cell_value(c_id, pty);
      else
        cs_property_get_cell_tensor(c_id, pty, h_info.inv_pty, hb->ptymat);

      if (pty_is_uniform)
        update_pty = false;
    }

    /* The local (dense) matrix is stored inside hloc
       n_ent = number of entities related to the current cell */
    cs_hodge_build_local(c_id, connect, quant, hb);

    /* Assemble the local matrix into the system matrix */
    cs_sla_assemble_msr_sym(hb->hloc, h_mat, only_diag);

  } /* End of loop on cells */

  /* Free temporary memory */
  hb = cs_hodge_builder_free(hb);

  return h_mat;
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
