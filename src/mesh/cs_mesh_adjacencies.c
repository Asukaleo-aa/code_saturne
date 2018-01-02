/*============================================================================
 * Additional mesh adjacencies.
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2018 EDF S.A.

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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>

#if defined(HAVE_MPI)
#include <mpi.h>
#endif

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "bft_error.h"
#include "bft_mem.h"
#include "bft_printf.h"

#include "cs_halo.h"
#include "cs_log.h"
#include "cs_mesh.h"
#include "cs_sort.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_mesh_adjacencies.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Additional Doxygen documentation
 *============================================================================*/

/*!
  \file cs_mesh_adjacencies.c
        Additional mesh adjacencies.
*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*=============================================================================
 * Local type definitions
 *============================================================================*/

/*============================================================================
 *  Global variables
 *============================================================================*/

static cs_mesh_adjacencies_t  _cs_glob_mesh_adjacencies;

const cs_mesh_adjacencies_t  *cs_glob_mesh_adjacencies = NULL;

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Update cell -> cells connectivity
 *
 * parameters:
 *   ma <-> mesh adjacecies structure to update
 *----------------------------------------------------------------------------*/

static void
_update_cell_cells(cs_mesh_adjacencies_t  *ma)
{
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_lnum_2_t *restrict face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_faces = m->n_i_faces;

  /* Allocate and map */

  BFT_REALLOC(ma->cell_cells_idx, n_cells + 1, cs_lnum_t);
  cs_lnum_t *c2c_idx = ma->cell_cells_idx;

  /* Count number of nonzero elements per row */

  cs_lnum_t  *count;

  BFT_MALLOC(count, n_cells, cs_lnum_t);

  for (cs_lnum_t i = 0; i < n_cells; i++)
    count[i] = 0;

  for (cs_lnum_t face_id = 0; face_id < n_faces; face_id++) {
    cs_lnum_t i = face_cells[face_id][0];
    cs_lnum_t j = face_cells[face_id][1];
    if (i < n_cells)
      count[i] += 1;
    if (j < n_cells)
      count[j] += 1;
  }

  c2c_idx[0] = 0;
  for (cs_lnum_t i = 0; i < n_cells; i++) {
    c2c_idx[i+1] = c2c_idx[i] + count[i];
    count[i] = 0;
  }

  /* Build structure */

  BFT_REALLOC(ma->cell_cells, c2c_idx[n_cells], cs_lnum_t);

  cs_lnum_t *c2c = ma->cell_cells;

  for (cs_lnum_t face_id = 0; face_id < n_faces; face_id++) {
    cs_lnum_t i = face_cells[face_id][0];
    cs_lnum_t j = face_cells[face_id][1];
    if (i < n_cells) {
      c2c[c2c_idx[i] + count[i]] = j;
      count[i] += 1;
    }
    if (j < n_cells) {
      c2c[c2c_idx[j] + count[j]] = i;
      count[j] += 1;
    }
  }

  BFT_FREE(count);

  /* Sort line elements by column id (for better access patterns) */

  ma->single_faces_to_cells = cs_sort_indexed(n_cells, c2c_idx, c2c);

  /* Compact elements if necessary */

  if (ma->single_faces_to_cells == false) {

    cs_lnum_t *tmp_c2c_idx = NULL;

    BFT_MALLOC(tmp_c2c_idx, n_cells+1, cs_lnum_t);
    memcpy(tmp_c2c_idx, c2c_idx, (n_cells+1)*sizeof(cs_lnum_t));

    cs_lnum_t k = 0;

    for (cs_lnum_t i = 0; i < n_cells; i++) {
      cs_lnum_t js = tmp_c2c_idx[i];
      cs_lnum_t je = tmp_c2c_idx[i+1];
      cs_lnum_t c2c_prev = -1;
      c2c_idx[i] = k;
      for (cs_lnum_t j = js; j < je; j++) {
        if (c2c_prev != c2c[j]) {
          c2c[k++] = c2c[j];
          c2c_prev = c2c[j];
        }
      }
    }
    c2c_idx[n_cells] = k;

    assert(c2c_idx[n_cells] < tmp_c2c_idx[n_cells]);

    BFT_FREE(tmp_c2c_idx);
    BFT_REALLOC(c2c, c2c_idx[n_cells], cs_lnum_t);

    ma->cell_cells = c2c;

  }
}

/*----------------------------------------------------------------------------
 * Update cells -> boundary faces connectivity
 *
 * parameters:
 *   ma <-> mesh adjacecies structure to update
 *----------------------------------------------------------------------------*/

static void
_update_cell_b_faces(cs_mesh_adjacencies_t  *ma)
{
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_b_faces = m->n_b_faces;

  /* (re)build cell -> boundary faces index */

  BFT_REALLOC(ma->cell_b_faces_idx, n_cells + 1, cs_lnum_t);
  cs_lnum_t *c2b_idx = ma->cell_b_faces_idx;

  cs_lnum_t *c2b_count;
  BFT_MALLOC(c2b_count, n_cells, cs_lnum_t);

  for (cs_lnum_t i = 0; i < n_cells; i++)
    c2b_count[i] = 0;

  for (cs_lnum_t i = 0; i < n_b_faces; i++)
    c2b_count[b_face_cells[i]] += 1;

  c2b_idx[0] = 0;
  for (cs_lnum_t i = 0; i < n_cells; i++) {
    c2b_idx[i+1] = c2b_idx[i] + c2b_count[i];
    c2b_count[i] = 0;
  }

  /* Rebuild values */

  BFT_REALLOC(ma->cell_b_faces, c2b_idx[n_cells], cs_lnum_t);
  cs_lnum_t *c2b = ma->cell_b_faces;

  for (cs_lnum_t i = 0; i < n_b_faces; i++) {
    cs_lnum_t c_id = b_face_cells[i];
    c2b[c2b_idx[c_id] + c2b_count[c_id]] = i;
    c2b_count[c_id] += 1;
  }

  BFT_FREE(c2b_count);

  /* Sort array */

  cs_sort_indexed(n_cells, c2b_idx, c2b);
}

/*! (DOXYGEN_SHOULD_SKIP_THIS) \endcond */

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Initialize mesh adjacencies helper API.
 */
/*----------------------------------------------------------------------------*/

void
cs_mesh_adjacencies_initialize(void)
{
  cs_mesh_adjacencies_t *ma = &_cs_glob_mesh_adjacencies;

  ma->cell_cells_idx = NULL;
  ma->cell_cells = NULL;

  ma->cell_cells_e_idx = NULL;
  ma->cell_cells_e = NULL;

  ma->cell_b_faces_idx = NULL;
  ma->cell_b_faces = NULL;

  cs_glob_mesh_adjacencies = ma;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Finalize mesh adjacencies helper API.
 */
/*----------------------------------------------------------------------------*/

void
cs_mesh_adjacencies_finalize(void)
{
  cs_mesh_adjacencies_t *ma = &_cs_glob_mesh_adjacencies;

  BFT_FREE(ma->cell_cells_idx);
  BFT_FREE(ma->cell_cells);

  BFT_FREE(ma->cell_b_faces_idx);
  BFT_FREE(ma->cell_b_faces);

  cs_glob_mesh_adjacencies = NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Update mesh adjacencies helper API relative to mesh.
 */
/*----------------------------------------------------------------------------*/

void
cs_mesh_adjacencies_update_mesh(void)
{
  cs_mesh_adjacencies_t *ma = &_cs_glob_mesh_adjacencies;

  /* (re)build cell -> cell connectivities */

  _update_cell_cells(ma);

  /* Map shared connectivities */

  cs_mesh_adjacencies_update_cell_cells_e();

  /* (re)build cell -> boundary face connectivities */

  _update_cell_b_faces(ma);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Update extended cell -> cell connectivites in
 *         mesh adjacencies helper API relative to mesh.
 */
/*----------------------------------------------------------------------------*/

void
cs_mesh_adjacencies_update_cell_cells_e(void)
{
  const cs_mesh_t *m = cs_glob_mesh;

  cs_mesh_adjacencies_t *ma = &_cs_glob_mesh_adjacencies;

  ma->cell_cells_e_idx = m->cell_cells_idx;
  ma->cell_cells_e = m->cell_cells_lst;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Create a cs_adjacency_t structure of size n_elts
 *
 * \param[in]  flag       metadata related to the new cs_adjacency to create
 * \param[in]  stride     > 0 if useful otherwise ignored
 * \param[in]  n_elts     number of entries of the indexed list
 *
 * \return  a pointer to a new allocated cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

cs_adjacency_t *
cs_adjacency_create(cs_flag_t    flag,
                    int          stride,
                    cs_lnum_t    n_elts)
{
  /* Sanity checks */
  if ((stride < 1) && (flag & CS_ADJACENCY_STRIDE))
    bft_error(__FILE__, __LINE__, 0,
              " Ask to create a cs_adjacency_t structure with a stride but"
              " an invalid value for the stride is set.\n");
  if (flag & CS_ADJACENCY_SHARED)
    bft_error(__FILE__, __LINE__, 0,
              " The cs_adjacency_t structure to create cannot be shared using"
              " the function %s\n", __func__);

  cs_adjacency_t  *adj = NULL;

  BFT_MALLOC(adj, 1, cs_adjacency_t);

  adj->n_elts = n_elts;
  adj->flag = flag;
  adj->stride = stride;

  adj->idx = NULL;
  adj->ids = NULL;
  adj->sgn = NULL;

  if (stride > 0) {

    adj->flag |= CS_ADJACENCY_STRIDE;
    BFT_MALLOC(adj->ids, stride*n_elts, cs_lnum_t);
    if (flag & CS_ADJACENCY_SIGNED)
      BFT_MALLOC(adj->sgn, stride*n_elts, short int);

  }
  else {

    BFT_MALLOC(adj->idx, n_elts+1, cs_lnum_t);
#   pragma omp parallel for if (n_elts > CS_THR_MIN)
    for (cs_lnum_t i = 0; i < adj->n_elts + 1; i++)  adj->idx[i] = 0;

  }

  return adj;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Create a cs_adjacency_t structure sharing arrays scanned with a
 *          stride
 *
 * \param[in]  n_elts    number of elements
 * \param[in]  stride    value of the stride
 * \param[in]  ids       array of element ids (size = stride * n_elts)
 * \param[in]  sgn       array storing the orientation (may be NULL)
 *
 * \return  a pointer to a new allocated cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

cs_adjacency_t *
cs_adjacency_create_from_s_arrays(cs_lnum_t    n_elts,
                                  int          stride,
                                  cs_lnum_t   *ids,
                                  short int   *sgn)
{
  /* Sanity checks */
  assert(ids != NULL);
  if (stride < 1)
    bft_error(__FILE__, __LINE__, 0,
              " %s: Invalid value for the stride when creating a cs_adjacency_t"
              " structure with a stride.\n", __func__);

  cs_adjacency_t  *adj = NULL;

  BFT_MALLOC(adj, 1, cs_adjacency_t);

  adj->n_elts = n_elts;
  adj->flag = CS_ADJACENCY_SHARED | CS_ADJACENCY_STRIDE;
  adj->stride = stride;

  adj->idx = NULL;
  adj->ids = ids;

  if (sgn != NULL) {
    adj->flag |= CS_ADJACENCY_SIGNED;
    adj->sgn = sgn;
  }

  return adj;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Create a cs_adjacency_t structure sharing arrays scanned with a
 *          stride
 *
 * \param[in]  n_elts    number of elements
 * \param[in]  idx       array of size n_elts + 1
 * \param[in]  ids       array of element ids (size = idx[n_elts])
 * \param[in]  sgn       array storing the orientation (may be NULL)
 *
 * \return  a pointer to a new allocated cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

cs_adjacency_t *
cs_adjacency_create_from_i_arrays(cs_lnum_t     n_elts,
                                  cs_lnum_t    *idx,
                                  cs_lnum_t    *ids,
                                  short int    *sgn)
{
  /* Sanity checks */
  assert(ids != NULL);
  if (idx == NULL)
    bft_error(__FILE__, __LINE__, 0,
              " %s: index is not allocated.\n", __func__);
  if (ids == NULL)
    bft_error(__FILE__, __LINE__, 0,
              " %s: array of element ids is not allocated.\n", __func__);

  cs_adjacency_t  *adj = NULL;

  BFT_MALLOC(adj, 1, cs_adjacency_t);

  adj->n_elts = n_elts;
  adj->flag = CS_ADJACENCY_SHARED;
  adj->stride = -1;

  adj->idx = idx;
  adj->ids = ids;

  if (sgn != NULL) {
    adj->flag |= CS_ADJACENCY_SIGNED;
    adj->sgn = sgn;
  }

  return adj;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Destroy a cs_adjacency_t structure
 *
 * \param[in, out]  p_adj   pointer of pointer to a cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_adjacency_free(cs_adjacency_t   **p_adj)
{
  cs_adjacency_t  *adj = *p_adj;

  if (adj == NULL)
    return;

  bool  is_shared = (adj->flag & CS_ADJACENCY_SHARED) ? true : false;
  if (!is_shared) {

    if (adj->stride < 1)
      BFT_FREE(adj->idx);

    BFT_FREE(adj->ids);
    if (adj->flag & CS_ADJACENCY_SIGNED)
      BFT_FREE(adj->sgn);
  }

  BFT_FREE(adj);
  *p_adj = NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Create a new cs_adjacency_t structure from the composition of
 *          two cs_adjacency_t structures: (1) A -> B and (2) B -> C
 *          The resulting structure describes A -> C. It does not rely on a
 *          stride and has no sgn member.
 *
 * \param[in]  n_c_elts  number of elements in C set
 * \param[in]  a2b       adjacency A -> B
 * \param[in]  b2c       adjacency B -> C
 *
 *\return  a pointer to the cs_adjacency_t structure A -> C
 */
/*----------------------------------------------------------------------------*/

cs_adjacency_t *
cs_adjacency_compose(int                      n_c_elts,
                     const cs_adjacency_t    *a2b,
                     const cs_adjacency_t    *b2c)
{
  cs_lnum_t  *ctag = NULL;
  cs_adjacency_t  *a2c = cs_adjacency_create(0, -1, a2b->n_elts);

  BFT_MALLOC(ctag, n_c_elts, cs_lnum_t);
# pragma omp parallel for if (n_c_elts > CS_THR_MIN)
  for (cs_lnum_t i = 0; i < n_c_elts; i++) ctag[i] = -1;

  /* Build index */
  /* ----------- */

  if (a2b->stride < 1 && b2c->stride < 1) {

    /* The two adjacencies rely on an indexed array */
    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (cs_lnum_t ja = a2b->idx[a_id]; ja < a2b->idx[a_id+1]; ja++) {

        const cs_lnum_t  b_id = a2b->ids[ja];
        for (cs_lnum_t jb = b2c->idx[b_id]; jb < b2c->idx[b_id+1]; jb++) {

          const cs_lnum_t  c_id = b2c->ids[jb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->idx[a_id+1] += 1;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else if (a2b->stride > 0 && b2c->stride < 1) {

    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (int ka = 0; ka < a2b->stride; ka++) {

        const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + ka];
        for (cs_lnum_t jb = b2c->idx[b_id]; jb < b2c->idx[b_id+1]; jb++) {

          const cs_lnum_t  c_id = b2c->ids[jb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->idx[a_id+1] += 1;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else if (a2b->stride < 1 && b2c->stride > 0) {

    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (cs_lnum_t ja = a2b->idx[a_id]; ja < a2b->idx[a_id+1]; ja++) {

        const cs_lnum_t  b_id = a2b->ids[ja];
        for (int kb = 0; kb < b2c->stride; kb++) {

          const cs_lnum_t  c_id = b2c->ids[b2c->stride*b_id + kb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->idx[a_id+1] += 1;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else {

    assert(a2b->stride > 0 && b2c->stride > 0);
    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (int ka = 0; ka < a2b->stride; ka++) {

        const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + ka];
        for (int kb = 0; kb < b2c->stride; kb++) {

          const cs_lnum_t  c_id = b2c->ids[b2c->stride*b_id + kb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->idx[a_id+1] += 1;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }

  for (cs_lnum_t i = 0; i < a2c->n_elts; i++)
    a2c->idx[i+1] += a2c->idx[i];

  BFT_MALLOC(a2c->ids, a2c->idx[a2c->n_elts], cs_lnum_t);

  /* Reset ctag */
# pragma omp parallel for if (n_c_elts > CS_THR_MIN)
  for (cs_lnum_t i = 0; i < n_c_elts; i++) ctag[i] = -1;

  /* Fill ids */
  /* -------- */

  cs_lnum_t  shift = 0;
  if (a2b->stride < 1 && b2c->stride < 1) {

    /* The two adjacencies rely on an indexed array */
    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (cs_lnum_t ja = a2b->idx[a_id]; ja < a2b->idx[a_id+1]; ja++) {

        const cs_lnum_t  b_id = a2b->ids[ja];
        for (cs_lnum_t jb = b2c->idx[b_id]; jb < b2c->idx[b_id+1]; jb++) {

          const cs_lnum_t  c_id = b2c->ids[jb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->ids[shift++] = c_id;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else if (a2b->stride > 0 && b2c->stride < 1) {

    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (int ka = 0; ka < a2b->stride; ka++) {

        const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + ka];
        for (cs_lnum_t jb = b2c->idx[b_id]; jb < b2c->idx[b_id+1]; jb++) {

          const cs_lnum_t  c_id = b2c->ids[jb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->ids[shift++] = c_id;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else if (a2b->stride < 1 && b2c->stride > 0) {

    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (cs_lnum_t ja = a2b->idx[a_id]; ja < a2b->idx[a_id+1]; ja++) {

        const cs_lnum_t  b_id = a2b->ids[ja];
        for (int kb = 0; kb < b2c->stride; kb++) {

          const cs_lnum_t  c_id = b2c->ids[b2c->stride*b_id + kb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->ids[shift++] = c_id;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }
  else {

    assert(a2b->stride > 0 && b2c->stride > 0);
    for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {

      for (int ka = 0; ka < a2b->stride; ka++) {

        const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + ka];
        for (int kb = 0; kb < b2c->stride; kb++) {

          const cs_lnum_t  c_id = b2c->ids[b2c->stride*b_id + kb];
          if (ctag[c_id] != a_id) { /* Not tagged yet */
            ctag[c_id] = a_id;
            a2c->ids[shift++] = c_id;
          }

        } /* End of loop on C elements */
      } /* End of loop on B elements */
    } /* End of loop on A elements */

  }

  assert(shift == a2c->idx[a2c->n_elts]);

  BFT_FREE(ctag);

  return a2c;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Create a new cs_adjacency_t structure from a one corresponding to
 *          A -> B. The resulting structure deals with B -> A.
 *
 *
 * \param[in]  n_b_elts    size of the set of B elements
 * \param[in]  a2b         pointer to the A -> B cs_adjacency_t structure
 *
 * \return  a new pointer to the cs_adjacency_t structure B -> A
 */
/*----------------------------------------------------------------------------*/

cs_adjacency_t *
cs_adjacency_transpose(int                     n_b_elts,
                       const cs_adjacency_t   *a2b)
{
  cs_flag_t  b2a_flag = 0;

  if (a2b->flag & CS_ADJACENCY_SIGNED)
    b2a_flag |=  CS_ADJACENCY_SIGNED;

  cs_adjacency_t  *b2a = cs_adjacency_create(b2a_flag, -1, n_b_elts);

  if (n_b_elts == 0)
    return b2a;

  /* Build idx */
  /* --------- */

  if (a2b->flag & CS_ADJACENCY_STRIDE) {

    for (cs_lnum_t i = 0; i < a2b->n_elts; i++)
      for (int j = 0; j < a2b->stride; j++)
        b2a->idx[a2b->ids[a2b->stride*i + j]+1] += 1;

  }
  else {

    for (cs_lnum_t i = 0; i < a2b->n_elts; i++)
      for (cs_lnum_t j = a2b->idx[i]; j < a2b->idx[i+1]; j++)
        b2a->idx[a2b->ids[j]+1] += 1;

  }

  for (cs_lnum_t i = 0; i < b2a->n_elts; i++)
    b2a->idx[i+1] += b2a->idx[i];

  /* Allocate and initialize temporary buffer */
  int  *count = NULL;
  BFT_MALLOC(count, n_b_elts, int);
# pragma omp parallel for if (n_b_elts > CS_THR_MIN)
  for (cs_lnum_t i = 0; i < n_b_elts; i++) count[i] = 0;

  /* Build ids */
  /* --------- */

  BFT_MALLOC(b2a->ids, b2a->idx[b2a->n_elts], cs_lnum_t);
  if (b2a->flag & CS_ADJACENCY_SIGNED)
    BFT_MALLOC(b2a->sgn, b2a->idx[b2a->n_elts], short int);

  if (a2b->flag & CS_ADJACENCY_STRIDE) {
    if (b2a->flag & CS_ADJACENCY_SIGNED) {

      for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {
        for (int j = 0; j < a2b->stride; j++) {

          const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + j];
          const cs_lnum_t  shift = count[b_id] + b2a->idx[b_id];

          b2a->ids[shift] = a_id;
          b2a->sgn[shift] = a2b->sgn[a2b->stride*a_id + j];
          count[b_id] += 1;

        }
      }

    }
    else { /* Do not build sgn */

      for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {
        for (int j = 0; j < a2b->stride; j++) {

          const cs_lnum_t  b_id = a2b->ids[a2b->stride*a_id + j];

          b2a->ids[count[b_id] + b2a->idx[b_id]] = a_id;
          count[b_id] += 1;

        }
      }

    }
  }
  else {
    if (b2a->flag & CS_ADJACENCY_SIGNED) {

      for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {
        for (cs_lnum_t j = a2b->idx[a_id]; j < a2b->idx[a_id+1]; j++) {

          const cs_lnum_t  b_id = a2b->ids[j];
          const cs_lnum_t  shift = count[b_id] + b2a->idx[b_id];

          b2a->ids[shift] = a_id;
          b2a->sgn[shift] = a2b->sgn[j];
          count[b_id] += 1;

        }
      }

    }
    else { /* Do not build sgn */

      for (cs_lnum_t a_id = 0; a_id < a2b->n_elts; a_id++) {
        for (cs_lnum_t j = a2b->idx[a_id]; j < a2b->idx[a_id+1]; j++) {

          const cs_lnum_t  b_id = a2b->ids[j];

          b2a->ids[count[b_id] + b2a->idx[b_id]] = a_id;
          count[b_id] += 1;

        }
      }

    }
  }

  /* Free temporary buffer */
  BFT_FREE(count);

  return b2a;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Sort each sub-list related to an entry in a cs_adjacency_t
 *          structure
 *
 * \param[in]  adj     pointer to a cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_adjacency_sort(cs_adjacency_t   *adj)
{
  if (adj == NULL)
    return;

  if (adj->flag & CS_ADJACENCY_STRIDE) {
    if (adj->flag & CS_ADJACENCY_SIGNED) {

#     pragma omp parallel for if (adj->n_elts > CS_THR_MIN)
      for (cs_lnum_t i = 0; i < adj->n_elts; i++)
        cs_sort_sicoupled_shell(adj->stride*i, adj->stride*(i+1),
                                adj->ids, adj->sgn);

    }
    else {

#     pragma omp parallel for if (adj->n_elts > CS_THR_MIN)
      for (cs_lnum_t i = 0; i < adj->n_elts; i++)
        cs_sort_shell(adj->stride*i, adj->stride*(i+1), adj->ids);

    }
  }
  else {
    if (adj->flag & CS_ADJACENCY_SIGNED) {

#     pragma omp parallel for if (adj->n_elts > CS_THR_MIN)
      for (cs_lnum_t i = 0; i < adj->n_elts; i++)
        cs_sort_sicoupled_shell(adj->idx[i], adj->idx[i+1], adj->ids, adj->sgn);

    }
    else {

#     pragma omp parallel for if (adj->n_elts > CS_THR_MIN)
      for (cs_lnum_t i = 0; i < adj->n_elts; i++)
        cs_sort_shell(adj->idx[i], adj->idx[i+1], adj->ids);

    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Dump a cs_adjacency_t structure to a file or into the
 *          standard output
 *
 * \param[in]  name    name of the dump file. Can be set to NULL
 * \param[in]  _f      pointer to a FILE structure. Can be set to NULL.
 * \param[in]  adj     pointer to a cs_adjacency_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_adjacency_dump(const char           *name,
                  FILE                 *_f,
                  cs_adjacency_t       *adj)
{
  FILE  *f = _f;
  bool  close_file = false;

  if (f == NULL) {
    if (name == NULL)
      f = stdout;
    else {
      f = fopen(name,"w");
      close_file = true;
    }
  }

  fprintf(f, "\n Dump cs_adjacency_t struct: %p (%s)\n",
          (const void *)adj, name);

  if (adj == NULL) {
    if (close_file) fclose(f);
    return;
  }

  bool  is_shared = (adj->flag & CS_ADJACENCY_SHARED) ? true : false;

  fprintf(f, "  shared:            %6s\n", is_shared ? "true" : "false");
  fprintf(f, "  n_elts:            %6d\n", adj->n_elts);
  fprintf(f, "  stride:            %6d\n", adj->stride);
  fprintf(f, "  ids_size:          %6d\n", adj->idx[adj->n_elts]);

  if (adj->flag & CS_ADJACENCY_STRIDE) {
    if (adj->flag & CS_ADJACENCY_SIGNED) {

      for (cs_lnum_t i = 0; i < adj->n_elts; i++) {
        fprintf(f, "\n[%6d] ", i);
        for (cs_lnum_t j = i*adj->stride; j < adj->stride*(i+1); j++)
          fprintf(f, "%5d (%-d) |", adj->ids[j], adj->sgn[j]);
      }

    }
    else {

      for (cs_lnum_t i = 0; i < adj->n_elts; i++) {
        fprintf(f, "\n[%6d] ", i);
        for (cs_lnum_t j = i*adj->stride; j < adj->stride*(i+1); j++)
          fprintf(f, "%5d |", adj->ids[j]);
      }

    }
  }
  else {
    if (adj->flag & CS_ADJACENCY_SIGNED) {

      for (cs_lnum_t i = 0; i < adj->n_elts; i++) {
        fprintf(f, "\n[%6d] ", i);
        for (cs_lnum_t j = adj->idx[i]; j < adj->idx[i+1]; j++)
          fprintf(f, "%5d (%-d) |", adj->ids[j], adj->sgn[j]);
      }

    }
    else {

      for (cs_lnum_t i = 0; i < adj->n_elts; i++) {
        fprintf(f, "\n[%6d] ", i);
        for (cs_lnum_t j = adj->idx[i]; j < adj->idx[i+1]; j++)
          fprintf(f, "%5d |", adj->ids[j]);
      }

    }
  }

  if (close_file)
    fclose(f);
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
