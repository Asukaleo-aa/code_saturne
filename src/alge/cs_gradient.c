/*============================================================================
 * Gradient reconstruction.
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2012 EDF S.A.

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
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_error.h>
#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_blas.h"
#include "cs_halo.h"
#include "cs_halo_perio.h"
#include "cs_log.h"
#include "cs_mesh.h"
#include "cs_ext_neighborhood.h"
#include "cs_mesh_quantities.h"
#include "cs_prototypes.h"
#include "cs_timer.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_gradient.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local structure definitions
 *============================================================================*/

/* Basic per gradient compuation options and logging */
/*---------------------------------------------------*/

typedef struct _cs_gradient_info_t {

  char                *name;               /* System name */
  cs_gradient_type_t   type;               /* Gradient type */

  unsigned             n_calls;            /* Number of times system solved */

  cs_timer_counter_t   t_tot;              /* Total time used */

} cs_gradient_info_t;

/*============================================================================
 *  Global variables
 *============================================================================*/

static int cs_glob_gradient_n_systems = 0;      /* Current number of systems */
static int cs_glob_gradient_n_max_systems = 0;  /* Max. number of sytems for
                                                   cs_glob_gradient_systems. */

/* System info array */
static cs_gradient_info_t **cs_glob_gradient_systems = NULL;

/* Short names for gradient computation types */

const char *cs_gradient_type_name[] = {N_("Iterative reconstruction"),
                                       N_("Least-squares (standard)"),
                                       N_("Least-squares (extended)"),
                                       N_("Least-squares (partially extended)"),
                                       N_("Least-squares then iterative")};

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Return pointer to new gradient computation info structure.
 *
 * parameters:
 *   name --> system name
 *   type --> resolution method
 *
 * returns:
 *   pointer to newly created linear system info structure
 *----------------------------------------------------------------------------*/

static cs_gradient_info_t *
_gradient_info_create(const char          *name,
                      cs_gradient_type_t   type)
{
  cs_gradient_info_t *new_info = NULL;

  BFT_MALLOC(new_info, 1, cs_gradient_info_t);
  BFT_MALLOC(new_info->name, strlen(name) + 1, char);

  strcpy(new_info->name, name);
  new_info->type = type;

  new_info->n_calls = 0;

  CS_TIMER_COUNTER_INIT(new_info->t_tot);

  return new_info;
}

/*----------------------------------------------------------------------------
 * Destroy gradient computationw info structure.
 *
 * parameters:
 *   this_info <-> pointer to linear system info structure pointer
 *----------------------------------------------------------------------------*/

static void
_gradient_info_destroy(cs_gradient_info_t  **this_info)
{
  if (*this_info != NULL) {
    BFT_FREE((*this_info)->name);
    BFT_FREE(*this_info);
  }
}

/*----------------------------------------------------------------------------
 * Output information regarding gradient computation.
 *
 * parameters:
 *   this_info <-> pointer to linear system info structure
 *----------------------------------------------------------------------------*/

static void
_gradient_info_dump(cs_gradient_info_t *this_info)
{
  int n_calls = this_info->n_calls;

  cs_log_printf(CS_LOG_PERFORMANCE,
                _("\n"
                  "Summary of gradient computations pour \"%s\" (%s):\n\n"
                  "  Number of calls:     %12d\n"
                  "  Total elapsed time:  %12.3f\n"),
                this_info->name, cs_gradient_type_name[this_info->type],
                n_calls, this_info->t_tot.wall_nsec*1e-9);
}

/*----------------------------------------------------------------------------
 * Return pointer to gradient computation info.
 *
 * If this system did not previously exist, it is added to the list of
 * "known" systems.
 *
 * parameters:
 *   name --> system name
 *   type --> resolution method
 *----------------------------------------------------------------------------*/

static cs_gradient_info_t *
_find_or_add_system(const char          *name,
                    cs_gradient_type_t   type)
{
  int ii, start_id, end_id, mid_id;
  int cmp_ret = 1;

  /* Use binary search to find system */

  start_id = 0;
  end_id = cs_glob_gradient_n_systems - 1;
  mid_id = start_id + ((end_id -start_id) / 2);

  while (start_id <= end_id) {
    cmp_ret = strcmp((cs_glob_gradient_systems[mid_id])->name, name);
    if (cmp_ret == 0)
      cmp_ret = (cs_glob_gradient_systems[mid_id])->type - type;
    if (cmp_ret < 0)
      start_id = mid_id + 1;
    else if (cmp_ret > 0)
      end_id = mid_id - 1;
    else
      break;
    mid_id = start_id + ((end_id -start_id) / 2);
  }

  /* If found, return */

  if (cmp_ret == 0)
    return cs_glob_gradient_systems[mid_id];

  /* Reallocate global array if necessary */

  if (cs_glob_gradient_n_systems >= cs_glob_gradient_n_max_systems) {

    if (cs_glob_gradient_n_max_systems == 0)
      cs_glob_gradient_n_max_systems = 10;
    else
      cs_glob_gradient_n_max_systems *= 2;
    BFT_REALLOC(cs_glob_gradient_systems,
                cs_glob_gradient_n_max_systems,
                cs_gradient_info_t*);

  }

  /* Insert in sorted list */

  for (ii = cs_glob_gradient_n_systems; ii > mid_id; ii--)
    cs_glob_gradient_systems[ii] = cs_glob_gradient_systems[ii - 1];

  cs_glob_gradient_systems[mid_id] = _gradient_info_create(name,
                                                           type);
  cs_glob_gradient_n_systems += 1;

  return cs_glob_gradient_systems[mid_id];
}

/*----------------------------------------------------------------------------
 * Compute L2 norm.
 *
 * parameters:
 *   n_elts <-- Local number of elements
 *   x      <-- array of 3-vectors
 *----------------------------------------------------------------------------*/

static double
_l2_norm_1(cs_lnum_t            n_elts,
           cs_real_t  *restrict x)
{
  double s = cs_dot(n_elts, x, x);

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {
    double _s;
    MPI_Allreduce(&s, &_s, 1, MPI_DOUBLE, MPI_SUM, cs_glob_mpi_comm);
    s = _s;
  }

#endif /* defined(HAVE_MPI) */

  return (sqrt(s));
}

/*----------------------------------------------------------------------------
 * Compute triple L2 norm, summing result over axes.
 *
 * The input array is assumed to be interleaved with block of 4 values,
 * of which the first 3 are used.
 *
 * A superblock algorithm is used for better precision.
 *
 * parameters:
 *   n_elts <-- Local number of elements
 *   x      <-- array of 3-vectors
 *----------------------------------------------------------------------------*/

static double
_l2_norm_3(cs_lnum_t              n_elts,
           cs_real_4_t  *restrict x)
{
  const cs_lnum_t block_size = 60;

  cs_lnum_t sid, bid, ii;
  cs_lnum_t start_id, end_id;
  double sdot1, sdot2, sdot3, cdot1, cdot2, cdot3;

  cs_lnum_t n_blocks = n_elts / block_size;
  cs_lnum_t n_sblocks = sqrt(n_blocks);
  cs_lnum_t blocks_in_sblocks = (n_sblocks > 0) ? n_blocks / n_sblocks : 0;

  double s[3];
  double s1 = 0.0, s2 = 0.0, s3 = 0.0;

# pragma omp parallel for reduction(+:s1, s2, s3) private(bid, start_id, end_id, ii, \
                                                          cdot1, cdot2, cdot3, \
                                                          sdot1, sdot2, sdot3)
  for (sid = 0; sid < n_sblocks; sid++) {

    sdot1 = 0.0;
    sdot2 = 0.0;
    sdot3 = 0.0;

    for (bid = 0; bid < blocks_in_sblocks; bid++) {
      start_id = block_size * (blocks_in_sblocks*sid + bid);
      end_id = block_size * (blocks_in_sblocks*sid + bid + 1);
      cdot1 = 0.0;
      cdot2 = 0.0;
      cdot3 = 0.0;
      for (ii = start_id; ii < end_id; ii++) {
        cdot1 += x[ii][0] * x[ii][0];
        cdot2 += x[ii][1] * x[ii][1];
        cdot3 += x[ii][2] * x[ii][2];
      }
      sdot1 += cdot1;
      sdot2 += cdot2;
      sdot3 += cdot3;
    }

    s1 += sdot1;
    s2 += sdot2;
    s3 += sdot3;

  }

  cdot1 = 0.0;
  cdot2 = 0.0;
  cdot3 = 0.0;
  start_id = block_size * n_sblocks*blocks_in_sblocks;
  end_id = n_elts;
  for (ii = start_id; ii < end_id; ii++) {
    cdot1 += x[ii][0] * x[ii][0];
    cdot2 += x[ii][1] * x[ii][1];
    cdot3 += x[ii][2] * x[ii][2];
  }

  s[0] = s1 + cdot1;
  s[1] = s2 + cdot2;
  s[2] = s3 + cdot3;

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1) {
    double _s[3];
    MPI_Allreduce(s, _s, 3, MPI_DOUBLE, MPI_SUM, cs_glob_mpi_comm);
    s[0] = _s[0];
    s[1] = _s[1];
    s[2] = _s[2];
  }

#endif /* defined(HAVE_MPI) */

  return (sqrt(s[0]) + sqrt(s[1]) + sqrt(s[2]));
}

/*----------------------------------------------------------------------------
 * Synchronize halos for scalar gradients.
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   idimtr         <-- 0 if ivar does not match a vector or tensor
 *                        or there is no periodicity of rotation
 *                      1 for velocity, 2 for Reynolds stress
 *   dpdxyz         <-> gradient of pvar (halo prepared for periodicity
 *                      of rotation)
 *----------------------------------------------------------------------------*/

static void
_sync_scalar_gradient_halo(const cs_mesh_t  *m,
                           int               idimtr,
                           cs_real_3_t       dpdxyz[])
{
  if (m->halo != NULL) {
    if (idimtr == 0) {
      cs_halo_sync_var_strided
        (m->halo, CS_HALO_STANDARD, (cs_real_t *)dpdxyz, 3);
      if (m->n_init_perio > 0)
        cs_halo_perio_sync_var_vect
          (m->halo, CS_HALO_STANDARD, (cs_real_t *)dpdxyz, 3);
    }
    else
      cs_halo_sync_components_strided(m->halo,
                                      CS_HALO_STANDARD,
                                      CS_HALO_ROTATION_IGNORE,
                                      (cs_real_t *)dpdxyz,
                                      3);
  }
}

/*----------------------------------------------------------------------------
 * Initialize rotation halos values from non-interleaved copy.
 *
 * parameters:
 *   halo      <-> halo associated with variables to set
 *   sync_mode <-> synchronization mode
 *   dpdx      <-- x component of gradient
 *   dpdy      <-- y component of gradient
 *   dpdz      <-- z component of gradient
 *   dpdxyz    <-> interleaved gradient components
 *----------------------------------------------------------------------------*/

static void
_initialize_rotation_values(const cs_halo_t  *halo,
                            cs_halo_type_t    sync_mode,
                            const cs_real_t   dpdx[],
                            const cs_real_t   dpdy[],
                            const cs_real_t   dpdz[],
                            cs_real_t         dpdxyz[])
{
  int  rank_id, t_id;
  cs_lnum_t  i, shift, start_std, end_std, start_ext, end_ext;

  const cs_lnum_t  n_cells   = halo->n_local_elts;
  const cs_lnum_t  n_transforms = halo->n_transforms;
  const fvm_periodicity_t  *periodicity = halo->periodicity;

  assert(halo != NULL);

  for (t_id = 0; t_id < n_transforms; t_id++) {

    if (   fvm_periodicity_get_type(periodicity, t_id)
        >= FVM_PERIODICITY_ROTATION) {

      shift = 4 * halo->n_c_domains * t_id;

      for (rank_id = 0; rank_id < halo->n_c_domains; rank_id++) {

        start_std = n_cells + halo->perio_lst[shift + 4*rank_id];
        end_std = start_std + halo->perio_lst[shift + 4*rank_id + 1];

        for (i = start_std; i < end_std; i++) {
          dpdxyz[i*3]     = dpdx[i];
          dpdxyz[i*3 + 1] = dpdy[i];
          dpdxyz[i*3 + 2] = dpdz[i];
        }

        if (sync_mode == CS_HALO_EXTENDED) {

          start_ext = halo->perio_lst[shift + 4*rank_id + 2];
          end_ext = start_ext + halo->perio_lst[shift + 4*rank_id + 3];

          for (i = start_ext; i < end_ext; i++) {
            dpdxyz[i*3]     = dpdx[i];
            dpdxyz[i*3 + 1] = dpdy[i];
            dpdxyz[i*3 + 2] = dpdz[i];
          }

        } /* End if extended halo */

      } /* End of loop on ranks */

    } /* End of test on rotation */

  } /* End of loop on transformations */
}

/*----------------------------------------------------------------------------
 * Clip the gradient of a scalar if necessary. This function deals with
 * the standard or extended neighborhood.
 *
 * parameters:
 *   imrgra         <-- type of computation for the gradient
 *   imligp         <-- type of clipping for the computation of the gradient
 *   iwarnp         <-- output level
 *   idimtr         <-- 0 for scalars or without rotational periodicity,
 *                      1 or 2 for vectors or tensors in case of rotational
 *                      periodicity
 *   climgp         <-- clipping coefficient for the computation of the gradient
 *   var            <-- variable
 *   dpdxyz         --> components of the pressure gradient
 *----------------------------------------------------------------------------*/

static void
_scalar_gradient_clipping(int                    imrgra,
                          int                    imligp,
                          int                    verbosity,
                          int                    idimtr,
                          cs_real_t              climgp,
                          cs_real_t              var[],
                          cs_real_3_t  *restrict dpdxyz)
{
  int        g_id, t_id;
  cs_gnum_t  t_n_clip;
  cs_lnum_t  face_id, ii, jj, ll, cidx;
  cs_real_t  dist[3];
  cs_real_t  dvar, dist1, dist2, dpdxf, dpdyf, dpdzf;
  cs_real_t  global_min_factor, global_max_factor, factor1, factor2;
  cs_real_t  t_min_factor, t_max_factor;

  cs_gnum_t  n_clip = 0, n_g_clip = 0;
  cs_real_t  min_factor = 1, max_factor = 0;
  cs_real_t  *restrict buf = NULL, *restrict clip_factor = NULL;
  cs_real_t  *restrict denom = NULL, *restrict denum = NULL;

  cs_halo_type_t halo_type = CS_HALO_STANDARD;

  const cs_mesh_t  *mesh = cs_glob_mesh;
  const int n_i_groups = mesh->i_face_numbering->n_groups;
  const int n_i_threads = mesh->i_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = mesh->i_face_numbering->group_index;
  const cs_lnum_t  n_cells = mesh->n_cells;
  const cs_lnum_t  n_cells_wghosts = mesh->n_cells_with_ghosts;
  const cs_lnum_t  *cell_cells_idx = mesh->cell_cells_idx;
  const cs_lnum_t  *cell_cells_lst = mesh->cell_cells_lst;
  const cs_real_3_t  *restrict cell_cen
    = (const cs_real_3_t *restrict)cs_glob_mesh_quantities->cell_cen;
  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)mesh->i_face_cells;

  const cs_halo_t *halo = mesh->halo;

  if (imligp < 0)
    return;

  if (imrgra == 2 || imrgra ==  3)
    halo_type = CS_HALO_EXTENDED;

  /* Synchronize variable */

  if (halo != NULL) {

    cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, var);

    /* Exchange for the gradients. Not useful for working array */

    if (imligp == 1) {

      if (idimtr > 0)
        cs_halo_sync_components_strided(halo,
                                        halo_type,
                                        CS_HALO_ROTATION_IGNORE,
                                        (cs_real_t *restrict)dpdxyz,
                                        3);
      else {
        cs_halo_sync_var_strided(halo,
                                 halo_type,
                                 (cs_real_t *restrict)dpdxyz,
                                 3);
        cs_halo_perio_sync_var_vect(halo,
                                    halo_type,
                                    (cs_real_t *restrict)dpdxyz,
                                    3);
      }

    } /* End if imligp == 1 */

  } /* End if halo */

  /* Allocate and initialize working buffers */

  if (imligp == 1)
    BFT_MALLOC(buf, 3*n_cells_wghosts, cs_real_t);
  else
    BFT_MALLOC(buf, 2*n_cells_wghosts, cs_real_t);

  denum = buf;
  denom = buf + n_cells_wghosts;

  if (imligp == 1)
    clip_factor = buf + 2*n_cells_wghosts;

# pragma omp parallel for
  for (ii = 0; ii < n_cells_wghosts; ii++) {
    denum[ii] = 0;
    denom[ii] = 0;
  }

  /* First computations:
      denum holds the maximum variation of the gradient
      denom holds the maximum variation of the variable */

  if (imligp == 0) {

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, \
                                      dist, dist1, dist2, dvar)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++)
            dist[ll] = cell_cen[ii][ll] - cell_cen[jj][ll];

          dist1 = CS_ABS(  dist[0]*dpdxyz[ii][0]
                         + dist[1]*dpdxyz[ii][1]
                         + dist[2]*dpdxyz[ii][2]);
          dist2 = CS_ABS(  dist[0]*dpdxyz[jj][0]
                         + dist[1]*dpdxyz[jj][1]
                         + dist[2]*dpdxyz[jj][2]);

          dvar = CS_ABS(var[ii] - var[jj]);

          denum[ii] = CS_MAX(denum[ii], dist1);
          denum[jj] = CS_MAX(denum[jj], dist2);
          denom[ii] = CS_MAX(denom[ii], dvar);
          denom[jj] = CS_MAX(denom[jj], dvar);

        } /* End of loop on faces */

      } /* End of loop on threads */

    } /* End of loop on thread groups */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, jj, ll, dist, dist1, dvar)
      for (ii = 0; ii < n_cells; ii++) {
        for (cidx = cell_cells_idx[ii] - 1;
             cidx < cell_cells_idx[ii+1] - 1;
             cidx++) {

          jj = cell_cells_lst[cidx] - 1;

          for (ll = 0; ll < 3; ll++)
            dist[ll] = cell_cen[ii][ll] - cell_cen[jj][ll];

          dist1 = CS_ABS(  dist[0]*dpdxyz[ii][0]
                         + dist[1]*dpdxyz[ii][1]
                         + dist[2]*dpdxyz[ii][2]);
          dvar = CS_ABS(var[ii] - var[jj]);

          denum[ii] = CS_MAX(denum[ii], dist1);
          denom[ii] = CS_MAX(denom[ii], dvar);

        }
      }

    } /* End for extended halo */

  }
  else if (imligp == 1) {

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, \
                                      dpdxf, dpdyf, dpdzf, dist, dist1, dvar)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++)
            dist[ll] = cell_cen[ii][ll] - cell_cen[jj][ll];

          dpdxf = 0.5 * (dpdxyz[ii][0] + dpdxyz[jj][0]);
          dpdyf = 0.5 * (dpdxyz[ii][1] + dpdxyz[jj][1]);
          dpdzf = 0.5 * (dpdxyz[ii][2] + dpdxyz[jj][2]);

          dist1 = CS_ABS(dist[0]*dpdxf + dist[1]*dpdyf + dist[2]*dpdzf);
          dvar = CS_ABS(var[ii] - var[jj]);

          denum[ii] = CS_MAX(denum[ii], dist1);
          denum[jj] = CS_MAX(denum[jj], dist1);
          denom[ii] = CS_MAX(denom[ii], dvar);
          denom[jj] = CS_MAX(denom[jj], dvar);

        } /* End of loop on faces */

      } /* End of loop on threads */

    } /* End of loop on thread groups */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, jj, ll, dist, \
                                      dpdxf, dpdyf, dpdzf, dist1, dvar)
      for (ii = 0; ii < n_cells; ii++) {
        for (cidx = cell_cells_idx[ii] - 1;
             cidx < cell_cells_idx[ii+1] - 1;
             cidx++) {

          jj = cell_cells_lst[cidx] - 1;

          for (ll = 0; ll < 3; ll++)
            dist[ll] = cell_cen[ii][ll] - cell_cen[jj][ll];

          dpdxf = 0.5 * (dpdxyz[ii][0] + dpdxyz[jj][0]);
          dpdyf = 0.5 * (dpdxyz[ii][1] + dpdxyz[jj][1]);
          dpdzf = 0.5 * (dpdxyz[ii][2] + dpdxyz[jj][2]);

          dist1 = CS_ABS(dist[0]*dpdxf + dist[1]*dpdyf + dist[2]*dpdzf);
          dvar = CS_ABS(var[ii] - var[jj]);

          denum[ii] = CS_MAX(denum[ii], dist1);
          denom[ii] = CS_MAX(denom[ii], dvar);

        }
      }

    } /* End for extended neighborhood */

  } /* End if *imligp == 1 */

  /* Clipping of the gradient if denum/denom > climgp */

  if (imligp == 0) {

    t_min_factor = min_factor;
    t_max_factor = max_factor;

#   pragma omp parallel private(t_min_factor, t_max_factor, factor1, t_n_clip)
    {
      t_n_clip = 0;
      t_min_factor = min_factor; t_max_factor = max_factor;

#     pragma omp for
      for (ii = 0; ii < n_cells; ii++) {

        if (denum[ii] > climgp * denom[ii]) {

          factor1 = climgp * denom[ii]/denum[ii];
          dpdxyz[ii][0] *= factor1;
          dpdxyz[ii][1] *= factor1;
          dpdxyz[ii][2] *= factor1;

          t_min_factor = CS_MIN(factor1, t_min_factor);
          t_max_factor = CS_MAX(factor1, t_max_factor);
          t_n_clip++;

        } /* If clipping */

      } /* End of loop on cells */

#     pragma omp critical
      {
        min_factor = CS_MIN(min_factor, t_min_factor);
        max_factor = CS_MAX(max_factor, t_max_factor);
        n_clip += t_n_clip;
      }
    } /* End of omp parallel construct */

  }
  else if (imligp == 1) {

#   pragma omp parallel for
    for (ii = 0; ii < n_cells_wghosts; ii++)
      clip_factor[ii] = (cs_real_t)DBL_MAX;

    /* Synchronize variable */

    if (halo != NULL) {
      if (idimtr > 0) {
        cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, denom);
        cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, denum);
      }
      else {
        cs_halo_sync_var(halo, halo_type, denom);
        cs_halo_sync_var(halo, halo_type, denum);
      }
    }

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, factor1, factor2, \
                                      min_factor)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          factor1 = 1.0;
          if (denum[ii] > climgp * denom[ii])
            factor1 = climgp * denom[ii]/denum[ii];

          factor2 = 1.0;
          if (denum[jj] > climgp * denom[jj])
            factor2 = climgp * denom[jj]/denum[jj];

          min_factor = CS_MIN(factor1, factor2);

          clip_factor[ii] = CS_MIN(clip_factor[ii], min_factor);
          clip_factor[jj] = CS_MIN(clip_factor[jj], min_factor);

        } /* End of loop on faces */

      } /* End of loop on threads */

    } /* End of loop on thread groups */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, jj, factor1, factor2)
      for (ii = 0; ii < n_cells; ii++) {

        factor1 = 1.0;

        for (cidx = cell_cells_idx[ii] - 1;
             cidx < cell_cells_idx[ii+1] - 1;
             cidx++) {

          jj = cell_cells_lst[cidx] - 1;

          factor2 = 1.0;

          if (denum[jj] > climgp * denom[jj])
            factor2 = climgp * denom[jj]/denum[jj];

          factor1 = CS_MIN(factor1, factor2);

        }

        clip_factor[ii] = CS_MIN(clip_factor[ii], factor1);

      } /* End of loop on cells */

    } /* End for extended neighborhood */

#   pragma omp parallel private(t_min_factor, t_max_factor, t_n_clip, ll)
    {
      t_n_clip = 0;
      t_min_factor = min_factor; t_max_factor = max_factor;

#     pragma omp for
      for (ii = 0; ii < n_cells; ii++) {

        for (ll = 0; ll < 3; ll++)
          dpdxyz[ii][ll] *= clip_factor[ii];

        if (clip_factor[ii] < 0.99) {
          t_max_factor = CS_MAX(t_max_factor, clip_factor[ii]);
          t_min_factor = CS_MIN(t_min_factor, clip_factor[ii]);
          t_n_clip++;
        }

      } /* End of loop on cells */

#     pragma omp critical
      {
        min_factor = CS_MIN(min_factor, t_min_factor);
        max_factor = CS_MAX(max_factor, t_max_factor);
        n_clip += t_n_clip;
      }
    } /* End of omp parallel construct */

  } /* End if imligp == 1 */

  /* Update min/max and n_clip in case of parallelism */

#if defined(HAVE_MPI)

  if (mesh->n_domains > 1) {

    assert(sizeof(cs_real_t) == sizeof(double));

    /* Global Max */

    MPI_Allreduce(&max_factor, &global_max_factor, 1, CS_MPI_REAL,
                  MPI_MAX, cs_glob_mpi_comm);

    max_factor = global_max_factor;

    /* Global min */

    MPI_Allreduce(&min_factor, &global_min_factor, 1, CS_MPI_REAL,
                  MPI_MIN, cs_glob_mpi_comm);

    min_factor = global_min_factor;

    /* Sum number of clippings */

    MPI_Allreduce(&n_clip, &n_g_clip, 1, CS_MPI_GNUM,
                  MPI_SUM, cs_glob_mpi_comm);

    n_clip = n_g_clip;

  } /* If n_domains > 1 */

#endif /* defined(HAVE_MPI) */

  /* Output warning if necessary */

  if (verbosity > 1)
    bft_printf(_(" Gradient limitation in %llu cells\n"
                 "   minimum factor = %14.5e; maximum factor = %14.5e\n"),
               (unsigned long long)n_clip, min_factor, max_factor);

  /* Synchronize dpdxyz */

  if (halo != NULL) {

    if (idimtr > 0) {

      /* If the gradient is not treated as a "true" vector */

      cs_halo_sync_components_strided(halo,
                                      halo_type,
                                      CS_HALO_ROTATION_IGNORE,
                                      (cs_real_t *restrict)dpdxyz,
                                      3);

    }
    else {

      cs_halo_sync_var_strided(halo,
                               halo_type,
                               (cs_real_t *restrict)dpdxyz,
                               3);

      cs_halo_perio_sync_var_vect(halo,
                                  halo_type,
                                  (cs_real_t *restrict)dpdxyz,
                                  3);

    }

  }

  BFT_FREE(buf);
}

/*----------------------------------------------------------------------------
 * Initialize gradient and right-hand side for scalar gradient reconstruction.
 *
 * A non-reconstructed gradient is computed at this stage.
 *
 * Optionally, a volume force generating a hydrostatic pressure component
 * may be accounted for.
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   idimtr         <-- 0 if ivar does not match a vector or tensor
 *                        or there is no periodicity of rotation
 *                      1 for velocity, 2 for Reynolds stress
 *   iphydp         <-- flag for hydrostatic pressure
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   fextx          <-- x component of exterior force generating pressure
 *   fexty          <-- y component of exterior force generating pressure
 *   fextz          <-- z component of exterior force generating pressure
 *   coefap         <-- B.C. coefficients for boundary face normals
 *   coefbp         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *   dpdxyz         <-> gradient of pvar (halo prepared for periodicity
 *                      of rotation)
 *   rhsv           <-> interleaved array for gradient RHS components
 *                      (0, 1, 2) and variable copy (3)
 *----------------------------------------------------------------------------*/

static void
_initialize_scalar_gradient(const cs_mesh_t             *m,
                            cs_mesh_quantities_t        *fvq,
                            int                          idimtr,
                            int                          iphydp,
                            double                       inc,
                            const cs_real_t              fextx[],
                            const cs_real_t              fexty[],
                            const cs_real_t              fextz[],
                            const cs_real_t              coefap[],
                            const cs_real_t              coefbp[],
                            const cs_real_t              pvar[],
                            cs_real_3_t        *restrict dpdxyz,
                            cs_real_4_t        *restrict rhsv)
{
  const int n_cells = m->n_cells;
  const int n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict weight = fvq->weight;
  const cs_real_t *restrict cell_vol = fvq->cell_vol;
  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;
  const cs_real_3_t *restrict i_face_normal
    = (const cs_real_3_t *restrict)fvq->i_face_normal;
  const cs_real_3_t *restrict b_face_normal
    = (const cs_real_3_t *restrict)fvq->b_face_normal;
  const cs_real_3_t *restrict i_face_cog
    = (const cs_real_3_t *restrict)fvq->i_face_cog;
  const cs_real_3_t *restrict b_face_cog
    = (const cs_real_3_t *restrict)fvq->b_face_cog;

  cs_lnum_t  cell_id, face_id, ii, jj;
  int        g_id, t_id;
  cs_real_t  pfac, vol_inv;
  cs_real_4_t  fctb;

  /* Initialize gradient */
  /*---------------------*/

# pragma omp parallel for
  for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
    rhsv[cell_id][0] = 0.0;
    rhsv[cell_id][1] = 0.0;
    rhsv[cell_id][2] = 0.0;
    rhsv[cell_id][3] = pvar[cell_id];
  }

  /* Standard case, without hydrostatic pressure */
  /*---------------------------------------------*/

  if (iphydp == 0) {

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, pfac, fctb)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          pfac  =        weight[face_id]  * rhsv[ii][3]
                  + (1.0-weight[face_id]) * rhsv[jj][3];
          fctb[0] = pfac * i_face_normal[face_id][0];
          fctb[1] = pfac * i_face_normal[face_id][1];
          fctb[2] = pfac * i_face_normal[face_id][2];
          rhsv[ii][0] += fctb[0];
          rhsv[ii][1] += fctb[1];
          rhsv[ii][2] += fctb[2];
          rhsv[jj][0] -= fctb[0];
          rhsv[jj][1] -= fctb[1];
          rhsv[jj][2] -= fctb[2];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, pfac)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          pfac = inc*coefap[face_id] + coefbp[face_id]*rhsv[ii][3];
          rhsv[ii][0] += pfac * b_face_normal[face_id][0];
          rhsv[ii][1] += pfac * b_face_normal[face_id][1];
          rhsv[ii][2] += pfac * b_face_normal[face_id][2];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

  }

  /* Case with hydrostatic pressure */
  /*--------------------------------*/

  else {

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, pfac, fctb)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          pfac
            =   (  weight[face_id]
                 * (  rhsv[ii][3]
                    - (cell_cen[ii][0] - i_face_cog[face_id][0])*fextx[ii]
                    - (cell_cen[ii][1] - i_face_cog[face_id][1])*fexty[ii]
                    - (cell_cen[ii][2] - i_face_cog[face_id][2])*fextz[ii]))
              + ( (1.0 - weight[face_id])
                 * (  rhsv[jj][3]
                    - (cell_cen[jj][0] - i_face_cog[face_id][0])*fextx[jj]
                    - (cell_cen[jj][1] - i_face_cog[face_id][1])*fexty[jj]
                    - (cell_cen[jj][2] - i_face_cog[face_id][2])*fextz[jj]));

          fctb[0] = pfac * i_face_normal[face_id][0];
          fctb[1] = pfac * i_face_normal[face_id][1];
          fctb[2] = pfac * i_face_normal[face_id][2];
          rhsv[ii][0] += fctb[0];
          rhsv[ii][1] += fctb[1];
          rhsv[ii][2] += fctb[2];
          rhsv[jj][0] -= fctb[0];
          rhsv[jj][1] -= fctb[1];
          rhsv[jj][2] -= fctb[2];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, pfac)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          pfac
            =      coefap[face_id] * inc
              + (  coefbp[face_id]
                 * (  rhsv[ii][3]
                    - (cell_cen[ii][0] - b_face_cog[face_id][0])*fextx[ii]
                    - (cell_cen[ii][1] - b_face_cog[face_id][1])*fexty[ii]
                    - (cell_cen[ii][2] - b_face_cog[face_id][2])*fextz[ii]));

          rhsv[ii][0] += pfac * b_face_normal[face_id][0];
          rhsv[ii][1] += pfac * b_face_normal[face_id][1];
          rhsv[ii][2] += pfac * b_face_normal[face_id][2];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

  } /* End of test on hydrostatic pressure */

  /* Compute gradient */
  /*------------------*/

# pragma omp parallel for private(vol_inv)
  for (cell_id = 0; cell_id < n_cells; cell_id++) {
    vol_inv = 1.0 / cell_vol[cell_id];
    dpdxyz[cell_id][0] = rhsv[cell_id][0] * vol_inv;
    dpdxyz[cell_id][1] = rhsv[cell_id][1] * vol_inv;
    dpdxyz[cell_id][2] = rhsv[cell_id][2] * vol_inv;
  }

  /* Synchronize halos */

   _sync_scalar_gradient_halo(m, idimtr, dpdxyz);
}

/*----------------------------------------------------------------------------
 * Compute cell gradient using iterative reconstruction for non-orthogonal
 * meshes (nswrgp > 1).
 *
 * Optionally, a volume force generating a hydrostatic pressure component
 * may be accounted for.
 *
 * cocg is computed to account for variable B.C.'s (flux).
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   var_num        <-- variable's number (0 if non-solved variable)
 *   imrgra         <-- gradient computation method
 *   imobil         <-- 1 if using mobile mesh, 0 otherwise
 *   iccocg         <-- flag to recompute cocg (1 or 0)
 *   iale           <-- 1 if using ALE, 0 otherwise
 *   nswrgp         <-- number of sweeps for gradient reconstruction
 *   idimtr         <-- 0 if ivar does not match a vector or tensor
 *                        or there is no periodicity of rotation
 *                      1 for velocity, 2 for Reynolds stress
 *   iphydp         <-- flag for hydrostatic pressure
 *   verbosity      <-- verbosity level
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   epsrgp         <-- relative precision for gradient reconstruction
 *   extrap         <-- gradient extrapolation coefficient
 *   fextx          <-- x component of exterior force generating pressure
 *   fexty          <-- y component of exterior force generating pressure
 *   fextz          <-- z component of exterior force generating pressure
 *   coefap         <-- B.C. coefficients for boundary face normals
 *   coefbp         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *   dpdxyz         <-> gradient of pvar (halo prepared for periodicity
 *                      of rotation)
 *   rhsv           <-> interleaved array for gradient RHS components
 *                      (0, 1, 2) and variable copy (3)
 *----------------------------------------------------------------------------*/

static void
_iterative_scalar_gradient(const cs_mesh_t             *m,
                           cs_mesh_quantities_t        *fvq,
                           int                          var_num,
                           int                          imrgra,
                           int                          iccocg,
                           int                          imobil,
                           int                          iale,
                           int                          nswrgp,
                           int                          idimtr,
                           int                          iphydp,
                           int                          verbosity,
                           double                       inc,
                           double                       epsrgp,
                           double                       extrap,
                           const cs_real_t              fextx[],
                           const cs_real_t              fexty[],
                           const cs_real_t              fextz[],
                           const cs_real_t              coefap[],
                           const cs_real_t              coefbp[],
                           const cs_real_t              pvar[],
                           cs_real_3_t        *restrict dpdxyz,
                           cs_real_4_t        *restrict rhsv)
{
  const int n_cells = m->n_cells;
  const int n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict weight = fvq->weight;
  const cs_real_t *restrict cell_vol = fvq->cell_vol;
  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;
  const cs_real_3_t *restrict i_face_normal
    = (const cs_real_3_t *restrict)fvq->i_face_normal;
  const cs_real_3_t *restrict b_face_normal
    = (const cs_real_3_t *restrict)fvq->b_face_normal;
  const cs_real_3_t *restrict i_face_cog
    = (const cs_real_3_t *restrict)fvq->i_face_cog;
  const cs_real_3_t *restrict b_face_cog
    = (const cs_real_3_t *restrict)fvq->b_face_cog;
  const cs_real_3_t *restrict diipb
    = (const cs_real_3_t *restrict)fvq->diipb;
  const cs_real_3_t *restrict dofij
    = (const cs_real_3_t *restrict)fvq->dofij;

  cs_real_33_t   *restrict cocgb = fvq->cocgb_s_it;
  cs_real_33_t   *restrict cocg = fvq->cocg_s_it;

  cs_lnum_t  cell_id, face_id, ii, jj, ll, mm;
  int        g_id, t_id;
  cs_real_t  rnorm;
  cs_real_t  a11, a12, a13, a21, a22, a23, a31, a32, a33;
  cs_real_t  cocg11, cocg12, cocg13, cocg21, cocg22, cocg23;
  cs_real_t  cocg31, cocg32, cocg33;
  cs_real_t  pfac, pfac0, pfac1, pip, det_inv;
  cs_real_3_t  fexd;
  cs_real_4_t  fctb;

  bool compute_cocg = false;
  int nswmax = nswrgp;
  int n_sweeps = 0;
  cs_real_t residue = 0.;

  const double epzero = 1.e-12;

  /* Computation without reconstruction */
  /*------------------------------------*/

  /* if initialized using least squares, rhsv is already initialized;
     otherwise, compute a gradient without reconstruction */

  if (imrgra == 0)
    _initialize_scalar_gradient(m,
                                fvq,
                                idimtr,
                                iphydp,
                                inc,
                                fextx,
                                fexty,
                                fextz,
                                coefap,
                                coefbp,
                                pvar,
                                dpdxyz,
                                rhsv);

  if (nswrgp <  1) return;

  /* Reconstruct gradients for non-orthogonal meshes */
  /*-------------------------------------------------*/

  /* Semi-implicit resolution on the whole mesh  */

  if (cocg == NULL || iale == 1 || imobil == 1)
    compute_cocg = true;

  if (compute_cocg) {

    if (cocg == NULL) {
      BFT_MALLOC(cocgb, m->n_b_cells, cs_real_33_t);
      BFT_MALLOC(cocg, n_cells_ext, cs_real_33_t);
      fvq->cocgb_s_it = cocgb;
      fvq->cocg_s_it = cocg;
    }

    /* Compute cocg */

#   pragma omp parallel for
    for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
      cocg[cell_id][0][0] = cell_vol[cell_id];
      cocg[cell_id][0][1] = 0.0;
      cocg[cell_id][0][2] = 0.0;
      cocg[cell_id][1][0] = 0.0;
      cocg[cell_id][1][1] = cell_vol[cell_id];
      cocg[cell_id][1][2] = 0.0;
      cocg[cell_id][2][0] = 0.0;
      cocg[cell_id][2][1] = 0.0;
      cocg[cell_id][2][2] = cell_vol[cell_id];
    }

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, fctb)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++) {
            fctb[0] = -dofij[face_id][0] * 0.5 * i_face_normal[face_id][ll];
            fctb[1] = -dofij[face_id][1] * 0.5 * i_face_normal[face_id][ll];
            fctb[2] = -dofij[face_id][2] * 0.5 * i_face_normal[face_id][ll];
            cocg[ii][ll][0] += fctb[0];
            cocg[ii][ll][1] += fctb[1];
            cocg[ii][ll][2] += fctb[2];
            cocg[jj][ll][0] -= fctb[0];
            cocg[jj][ll][1] -= fctb[1];
            cocg[jj][ll][2] -= fctb[2];
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Save partial cocg at interior faces of boundary cells */

#   pragma omp parallel for private(cell_id, ll, mm)
    for (ii = 0; ii < m->n_b_cells; ii++) {
      cell_id = m->b_cells[ii] - 1;
      for (ll = 0; ll < 3; ll++) {
        for (mm = 0; mm < 3; mm++)
          cocgb[ii][ll][mm] = cocg[cell_id][ll][mm];
      }
    }

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, mm)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] -= (  coefbp[face_id] * diipb[face_id][mm]
                                   * b_face_normal[face_id][ll]);
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Invert for all cells. */
    /*-----------------------*/

    /* The cocg term for interior cells only changes if the mesh does */

#   pragma omp parallel for private(cocg11, cocg12, cocg13, cocg21, cocg22, \
                                    cocg23, cocg31, cocg32, cocg33, a11, a12, \
                                    a13, a21, a22, a23, a31, a32, a33, det_inv)
    for (cell_id = 0; cell_id < n_cells; cell_id++) {

      cocg11 = cocg[cell_id][0][0];
      cocg12 = cocg[cell_id][0][1];
      cocg13 = cocg[cell_id][0][2];
      cocg21 = cocg[cell_id][1][0];
      cocg22 = cocg[cell_id][1][1];
      cocg23 = cocg[cell_id][1][2];
      cocg31 = cocg[cell_id][2][0];
      cocg32 = cocg[cell_id][2][1];
      cocg33 = cocg[cell_id][2][2];

      a11 = cocg22*cocg33 - cocg32*cocg23;
      a12 = cocg32*cocg13 - cocg12*cocg33;
      a13 = cocg12*cocg23 - cocg22*cocg13;
      a21 = cocg31*cocg23 - cocg21*cocg33;
      a22 = cocg11*cocg33 - cocg31*cocg13;
      a23 = cocg21*cocg13 - cocg11*cocg23;
      a31 = cocg21*cocg32 - cocg31*cocg22;
      a32 = cocg31*cocg12 - cocg11*cocg32;
      a33 = cocg11*cocg22 - cocg21*cocg12;

      det_inv = 1. / (cocg11*a11 + cocg21*a12 + cocg31*a13);

      cocg[cell_id][0][0] = a11 * det_inv;
      cocg[cell_id][0][1] = a12 * det_inv;
      cocg[cell_id][0][2] = a13 * det_inv;
      cocg[cell_id][1][0] = a21 * det_inv;
      cocg[cell_id][1][1] = a22 * det_inv;
      cocg[cell_id][1][2] = a23 * det_inv;
      cocg[cell_id][2][0] = a31 * det_inv;
      cocg[cell_id][2][1] = a32 * det_inv;
      cocg[cell_id][2][2] = a33 * det_inv;

    }

  } /* End of test on ale, mobile mesh, or call counter */

  /* If cocg must be recomputed later, only do it for boundary cells,
     with saved cocgb */

  if (iccocg == 1 && compute_cocg == false) {

#   pragma omp parallel for private(cell_id, ll, mm)
    for (ii = 0; ii < m->n_b_cells; ii++) {
      cell_id = m->b_cells[ii] - 1;
      for (ll = 0; ll < 3; ll++) {
        for (mm = 0; mm < 3; mm++)
          cocg[cell_id][ll][mm] = cocgb[ii][ll][mm];
      }
    }

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, mm)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] -= (  coefbp[face_id]*diipb[face_id][mm]
                                   * b_face_normal[face_id][ll]);
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

#   pragma omp parallel for private(cell_id, cocg11, cocg12, cocg13, cocg21, \
                                    cocg22, cocg23, cocg31, cocg32, cocg33, \
                                    a11, a12, a13, a21, a22, a23, a31, a32, \
                                    a33, det_inv)
    for (ii = 0; ii < m->n_b_cells; ii++) {

      cell_id = m->b_cells[ii] - 1;

      cocg11 = cocg[cell_id][0][0];
      cocg12 = cocg[cell_id][0][1];
      cocg13 = cocg[cell_id][0][2];
      cocg21 = cocg[cell_id][1][0];
      cocg22 = cocg[cell_id][1][1];
      cocg23 = cocg[cell_id][1][2];
      cocg31 = cocg[cell_id][2][0];
      cocg32 = cocg[cell_id][2][1];
      cocg33 = cocg[cell_id][2][2];

      a11 = cocg22*cocg33 - cocg32*cocg23;
      a12 = cocg32*cocg13 - cocg12*cocg33;
      a13 = cocg12*cocg23 - cocg22*cocg13;
      a21 = cocg31*cocg23 - cocg21*cocg33;
      a22 = cocg11*cocg33 - cocg31*cocg13;
      a23 = cocg21*cocg13 - cocg11*cocg23;
      a31 = cocg21*cocg32 - cocg31*cocg22;
      a32 = cocg31*cocg12 - cocg11*cocg32;
      a33 = cocg11*cocg22 - cocg21*cocg12;

      det_inv = 1.0/(cocg11*a11 + cocg21*a12 + cocg31*a13);

      cocg[cell_id][0][0] = a11 * det_inv;
      cocg[cell_id][0][1] = a12 * det_inv;
      cocg[cell_id][0][2] = a13 * det_inv;
      cocg[cell_id][1][0] = a21 * det_inv;
      cocg[cell_id][1][1] = a22 * det_inv;
      cocg[cell_id][1][2] = a23 * det_inv;
      cocg[cell_id][2][0] = a31 * det_inv;
      cocg[cell_id][2][1] = a32 * det_inv;
      cocg[cell_id][2][2] = a33 * det_inv;

    }

  } /* End of test on ale, mobile mesh, or call counter */

  /* Compute normalization residue */

  rnorm = _l2_norm_3(n_cells, rhsv);

  if (fvq->max_vol > 1)
    rnorm /= fvq->max_vol;

  if (rnorm <= epzero)
    return;

  /* Vector OijFij is computed in CLDijP */

  /* Start iterations */
  /*------------------*/

  for (n_sweeps = 1; n_sweeps < nswmax; n_sweeps++) {

    /* Compute right hand side */

#   pragma omp parallel for
    for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
      rhsv[cell_id][0] = -dpdxyz[cell_id][0] * cell_vol[cell_id];
      rhsv[cell_id][1] = -dpdxyz[cell_id][1] * cell_vol[cell_id];
      rhsv[cell_id][2] = -dpdxyz[cell_id][2] * cell_vol[cell_id];
    }

    /* Standard case, without hydrostatic pressure */
    /*---------------------------------------------*/

    if (iphydp == 0) {

      /* Contribution from interior faces */

      for (g_id = 0; g_id < n_i_groups; g_id++) {

#       pragma omp parallel for private(face_id, ii, jj, pfac, fctb)
        for (t_id = 0; t_id < n_i_threads; t_id++) {

          for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
               face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
               face_id++) {

            ii = i_face_cells[face_id][0] - 1;
            jj = i_face_cells[face_id][1] - 1;

            pfac  =        weight[face_id]  * rhsv[ii][3]
                    + (1.0-weight[face_id]) * rhsv[jj][3]
                    + ( dofij[face_id][0] * (dpdxyz[ii][0]+dpdxyz[jj][0])
                    +   dofij[face_id][1] * (dpdxyz[ii][1]+dpdxyz[jj][1])
                    +   dofij[face_id][2] * (dpdxyz[ii][2]+dpdxyz[jj][2])) * 0.5;
            fctb[0] = pfac * i_face_normal[face_id][0];
            fctb[1] = pfac * i_face_normal[face_id][1];
            fctb[2] = pfac * i_face_normal[face_id][2];
            rhsv[ii][0] += fctb[0];
            rhsv[ii][1] += fctb[1];
            rhsv[ii][2] += fctb[2];
            rhsv[jj][0] -= fctb[0];
            rhsv[jj][1] -= fctb[1];
            rhsv[jj][2] -= fctb[2];

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

      /* Contribution from boundary faces */

      for (g_id = 0; g_id < n_b_groups; g_id++) {

#       pragma omp parallel for private(face_id, ii, pip, pfac0, pfac1, pfac)
        for (t_id = 0; t_id < n_b_threads; t_id++) {

          for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
               face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
               face_id++) {

            ii = b_face_cells[face_id] - 1;

            pip =   rhsv[ii][3]
                  + diipb[face_id][0] * dpdxyz[ii][0]
                  + diipb[face_id][1] * dpdxyz[ii][1]
                  + diipb[face_id][2] * dpdxyz[ii][2];

            pfac0 =   coefap[face_id] * inc
                    + coefbp[face_id] * pip;

            pfac1 =   rhsv[ii][3]
                    + (b_face_cog[face_id][0]-cell_cen[ii][0]) * dpdxyz[ii][0]
                    + (b_face_cog[face_id][1]-cell_cen[ii][1]) * dpdxyz[ii][1]
                    + (b_face_cog[face_id][2]-cell_cen[ii][2]) * dpdxyz[ii][2];

            pfac =          coefbp[face_id]  *(extrap*pfac1 + (1.0-extrap)*pfac0)
                   + (1.0 - coefbp[face_id]) * pfac0;

            rhsv[ii][0] = rhsv[ii][0] + pfac * b_face_normal[face_id][0];
            rhsv[ii][1] = rhsv[ii][1] + pfac * b_face_normal[face_id][1];
            rhsv[ii][2] = rhsv[ii][2] + pfac * b_face_normal[face_id][2];

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

    }

    /* Case with hydrostatic pressure */
    /*--------------------------------*/

    else {

      /* Contribution from interior faces */

      for (g_id = 0; g_id < n_i_groups; g_id++) {

#       pragma omp parallel for private(face_id, ii, jj, pfac, fexd, fctb)
        for (t_id = 0; t_id < n_i_threads; t_id++) {

          for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
               face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
               face_id++) {

            ii = i_face_cells[face_id][0] - 1;
            jj = i_face_cells[face_id][1] - 1;

            fexd[0] = 0.5 * (fextx[ii] - fextx[jj]);
            fexd[1] = 0.5 * (fexty[ii] - fexty[jj]);
            fexd[2] = 0.5 * (fextz[ii] - fextz[jj]);

            /* Note: changed expression from:
             *   fmean = 0.5 * (fext[ii] + fext[jj])
             *   fii = fext[ii] - fmean
             *   fjj = fext[jj] - fmean
             * to:
             *   fexd = 0.5 * (fext[ii] - fext[jj])
             *   fii =  fexd
             *   fjj = -fexd
             */

            pfac
              =   (  weight[face_id]
                   * (  rhsv[ii][3]
                      - (cell_cen[ii][0]-i_face_cog[face_id][0])*fexd[0]
                      - (cell_cen[ii][1]-i_face_cog[face_id][1])*fexd[1]
                      - (cell_cen[ii][2]-i_face_cog[face_id][2])*fexd[2]))
              +   (  (1.0 - weight[face_id])
                   * (  rhsv[jj][3]
                      + (cell_cen[jj][0]-i_face_cog[face_id][0])*fexd[0]
                      + (cell_cen[jj][1]-i_face_cog[face_id][1])*fexd[1]
                      + (cell_cen[jj][2]-i_face_cog[face_id][2])*fexd[2]))
              +   (  dofij[face_id][0] * (dpdxyz[ii][0]+dpdxyz[jj][0])
                   + dofij[face_id][1] * (dpdxyz[ii][1]+dpdxyz[jj][1])
                   + dofij[face_id][2] * (dpdxyz[ii][2]+dpdxyz[jj][2]))*0.5;

            fctb[0] = pfac * i_face_normal[face_id][0];
            fctb[1] = pfac * i_face_normal[face_id][1];
            fctb[2] = pfac * i_face_normal[face_id][2];

            rhsv[ii][0] += fctb[0];
            rhsv[ii][1] += fctb[1];
            rhsv[ii][2] += fctb[2];
            rhsv[jj][0] -= fctb[0];
            rhsv[jj][1] -= fctb[1];
            rhsv[jj][2] -= fctb[2];

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

      /* Contribution from boundary faces */

      for (g_id = 0; g_id < n_b_groups; g_id++) {

#       pragma omp parallel for private(face_id, ii, pip, pfac0, pfac1, pfac)
        for (t_id = 0; t_id < n_b_threads; t_id++) {

          for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
               face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
               face_id++) {

            ii = b_face_cells[face_id] - 1;

            pip =   rhsv[ii][3]
                  + diipb[face_id][0] * dpdxyz[ii][0]
                  + diipb[face_id][1] * dpdxyz[ii][1]
                  + diipb[face_id][2] * dpdxyz[ii][2];

            pfac0 =      coefap[face_id] * inc
                    +    coefbp[face_id]
                       * (  pip
                          - (  cell_cen[ii][0]
                             - b_face_cog[face_id][0]
                             + diipb[face_id][0]) * fextx[ii]
                          - (  cell_cen[ii][1]
                             - b_face_cog[face_id][1]
                             + diipb[face_id][1]) * fexty[ii]
                          - (  cell_cen[ii][2]
                             - b_face_cog[face_id][2]
                             + diipb[face_id][2]) * fextz[ii]);

            pfac1 =   rhsv[ii][3]
                    + (b_face_cog[face_id][0]-cell_cen[ii][0]) * dpdxyz[ii][0]
                    + (b_face_cog[face_id][1]-cell_cen[ii][1]) * dpdxyz[ii][1]
                    + (b_face_cog[face_id][2]-cell_cen[ii][2]) * dpdxyz[ii][2];

            pfac =          coefbp[face_id]  *(extrap*pfac1 + (1.0-extrap)*pfac0)
                   + (1.0 - coefbp[face_id]) * pfac0;

            rhsv[ii][0] += pfac * b_face_normal[face_id][0];
            rhsv[ii][1] += pfac * b_face_normal[face_id][1];
            rhsv[ii][2] += pfac * b_face_normal[face_id][2];

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

    } /* End of test on hydrostatic pressure */

    /* Increment gradient */
    /*--------------------*/

#   pragma omp parallel for
    for (cell_id = 0; cell_id < n_cells; cell_id++) {
      dpdxyz[cell_id][0] +=   cocg[cell_id][0][0] * rhsv[cell_id][0]
                            + cocg[cell_id][0][1] * rhsv[cell_id][1]
                            + cocg[cell_id][0][2] * rhsv[cell_id][2];
      dpdxyz[cell_id][1] +=   cocg[cell_id][1][0] * rhsv[cell_id][0]
                            + cocg[cell_id][1][1] * rhsv[cell_id][1]
                            + cocg[cell_id][1][2] * rhsv[cell_id][2];
      dpdxyz[cell_id][2] +=   cocg[cell_id][2][0] * rhsv[cell_id][0]
                            + cocg[cell_id][2][1] * rhsv[cell_id][1]
                            + cocg[cell_id][2][2] * rhsv[cell_id][2];
    }

    /* Synchronize halos */

    _sync_scalar_gradient_halo(m, idimtr, dpdxyz);

    /* Convergence test */

    residue = _l2_norm_3(n_cells, rhsv);

    if (fvq->max_vol > 1)
      residue /= fvq->max_vol;

    if (residue < epsrgp*rnorm) {
      if (verbosity > 1)
        bft_printf(_(" %s; variable: %2d; converged in %d sweeps\n"
                     " %*s  normed residual: %11.4e; norm: %11.4e\n"),
                   __func__, var_num, n_sweeps,
                   (int)(strlen(__func__)), " ", residue/rnorm, rnorm);
      break;
    }

  } /* Loop on sweeps */

  if (residue >= epsrgp*rnorm && verbosity > -1) {
    bft_printf(_(" Warning:\n"
                 " --------\n"
                 "   %s; variable: %2d; sweeps: %d\n"
                 "   %*s  normed residual: %11.4e; norm: %11.4e\n"),
                 __func__, var_num, n_sweeps,
               (int)(strlen(__func__)), " ", residue/rnorm, rnorm);
  }
}

/*----------------------------------------------------------------------------
 * Compute cell gradient using least-squares reconstruction for non-orthogonal
 * meshes (nswrgp > 1).
 *
 * Optionally, a volume force generating a hydrostatic pressure component
 * may be accounted for.
 *
 * cocg is computed to account for variable B.C.'s (flux).
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   imrgra         <-- gradient computation method
 *   iccocg         <-- flag to recompute cocg (1 or 0)
 *   imobil         <-- 1 if using mobile mesh, 0 otherwise
 *   iale           <-- 1 if using ALE, 0 otherwise
 *   nswrgp         <-- number of sweeps for gradient reconstruction
 *   idimtr         <-- 0 if ivar does not match a vector or tensor
 *                        or there is no periodicity of rotation
 *                      1 for velocity, 2 for Reynolds stress
 *   iphydp         <-- flag for hydrostatic pressure
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   extrap         <-- gradient extrapolation coefficient
 *   isympa         <-- Array with value 0 on symmetries, 1 elsewhere
 *   fextx          <-- x component of exterior force generating pressure
 *   fexty          <-- y component of exterior force generating pressure
 *   fextz          <-- z component of exterior force generating pressure
 *   coefap         <-- B.C. coefficients for boundary face normals
 *   coefbp         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *   dpdxyz         <-> gradient of pvar (halo prepared for periodicity
 *                      of rotation)
 *----------------------------------------------------------------------------*/

static void
_lsq_scalar_gradient(const cs_mesh_t             *m,
                     cs_mesh_quantities_t        *fvq,
                     int                          imrgra,
                     int                          iccocg,
                     int                          imobil,
                     int                          iale,
                     int                          nswrgp,
                     int                          idimtr,
                     int                          iphydp,
                     cs_real_t                    inc,
                     double                       extrap,
                     const cs_int_t               isympa[],
                     const cs_real_t              fextx[],
                     const cs_real_t              fexty[],
                     const cs_real_t              fextz[],
                     const cs_real_t              coefap[],
                     const cs_real_t              coefbp[],
                     const cs_real_t              pvar[],
                     cs_real_3_t        *restrict dpdxyz,
                     cs_real_4_t        *restrict rhsv)
{
  const int n_cells = m->n_cells;
  const int n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;
  const cs_lnum_t *restrict cell_cells_idx
    = (const cs_lnum_t *restrict)m->cell_cells_idx;
  const cs_lnum_t *restrict cell_cells_lst
    = (const cs_lnum_t *restrict)m->cell_cells_lst;

  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;
  const cs_real_3_t *restrict b_face_normal
    = (const cs_real_3_t *restrict)fvq->b_face_normal;
  const cs_real_t *restrict b_face_surf
    = (const cs_real_t *restrict)fvq->b_face_surf;
  const cs_real_t *restrict b_dist
    = (const cs_real_t *restrict)fvq->b_dist;
  const cs_real_3_t *restrict i_face_cog
    = (const cs_real_3_t *restrict)fvq->i_face_cog;
  const cs_real_3_t *restrict b_face_cog
    = (const cs_real_3_t *restrict)fvq->b_face_cog;
  const cs_real_3_t *restrict diipb
    = (const cs_real_3_t *restrict)fvq->diipb;

  cs_real_33_t   *restrict cocgb = fvq->cocgb_s_lsq;
  cs_real_33_t   *restrict cocg = fvq->cocg_s_lsq;

  cs_lnum_t  cell_id, cidx, face_id, ii, jj, ll, mm;
  int        g_id, t_id;
  cs_real_t  a11, a12, a13, a22, a23, a33;
  cs_real_t  cocg11, cocg12, cocg13, cocg22, cocg23, cocg33;
  cs_real_t  pfac, det_inv, uddij2;
  cs_real_t  extrab, unddij, umcbdd, udbfs;
  cs_real_3_t  dc, dddij, dsij;
  cs_real_4_t  fctb;

  /* Remark:

     for 2D calculations, if we extrapolate the pressure gradient,
     we obtain a non-invertible cocg matrix, because of the third
     direction.

     To avoid this, we multiply extrap by isympa which is zero for
     symmetries : the gradient is thus not extrapolated on those faces. */

  /* Initialize gradient */
  /*---------------------*/

  if (nswrgp <= 1) {

    _initialize_scalar_gradient(m,
                                fvq,
                                idimtr,
                                iphydp,
                                inc,
                                fextx,
                                fexty,
                                fextz,
                                coefap,
                                coefbp,
                                pvar,
                                dpdxyz,
                                rhsv);

    return;

  }

  /* Reconstruct gradients using least squares for non-orthogonal meshes */
  /*---------------------------------------------------------------------*/

  /* Compute cocg and save contribution at boundaries */

  if ((cocg == NULL || iale == 1|| imobil == 1) && iccocg == 1) {

    if (cocg == NULL) {
      BFT_MALLOC(cocgb, m->n_b_cells, cs_real_33_t);
      BFT_MALLOC(cocg, n_cells_ext, cs_real_33_t);
      fvq->cocgb_s_lsq = cocgb;
      fvq->cocg_s_lsq = cocg;
    }

    /* Initialization */

#   pragma omp parallel for private(ll, mm)
    for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
      for (ll = 0; ll < 3; ll++) {
        for (mm = 0; mm < 3; mm++)
          cocg[cell_id][ll][mm] = 0.0;
      }
    }

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, mm, uddij2, dc)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];
          uddij2 = 1. / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] += dc[ll] * dc[mm] * uddij2;
          }
          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[jj][ll][mm] += dc[ll] * dc[mm] * uddij2;
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Contribution from extended neighborhood */

    if (imrgra == 2 || imrgra == 3) {

#     pragma omp parallel for private(cidx, jj, ll, mm, uddij2, dc)
      for (ii = 0; ii < n_cells; ii++) {
        for (cidx = cell_cells_idx[ii]; cidx < cell_cells_idx[ii+1]; cidx++) {

          jj = cell_cells_lst[cidx - 1] - 1;

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];
          uddij2 = 1. / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] += dc[ll] * dc[mm] * uddij2;
          }

        }
      }

    } /* End for extended neighborhood */

    /* Save partial cocg at interior faces of boundary cells */

#   pragma omp parallel for private(cell_id, ll, mm)
    for (ii = 0; ii < m->n_b_cells; ii++) {
      cell_id = m->b_cells[ii] - 1;
      for (ll = 0; ll < 3; ll++) {
        for (mm = 0; mm < 3; mm++)
          cocgb[ii][ll][mm] = cocg[cell_id][ll][mm];
      }
    }

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, mm, \
                                      extrab, umcbdd, udbfs, dddij)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          extrab = 1. - isympa[face_id]*extrap*coefbp[face_id];

          umcbdd = extrab * (1. - coefbp[face_id]) / b_dist[face_id];
          udbfs = extrab / b_face_surf[face_id];

          for (ll = 0; ll < 3; ll++)
            dddij[ll] =   udbfs * b_face_normal[face_id][ll]
                        + umcbdd * diipb[face_id][ll];

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] += dddij[ll]*dddij[mm];
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Invert for all cells. */
    /*-----------------------*/

    /* The cocg term for interior cells only changes if the mesh does */

#   pragma omp parallel for private(cocg11, cocg12, cocg13, cocg22, \
                                    cocg23, cocg33, a11, a12, \
                                    a13, a22, a23, a33, det_inv)
    for (cell_id = 0; cell_id < n_cells; cell_id++) {

      cocg11 = cocg[cell_id][0][0];
      cocg12 = cocg[cell_id][0][1];
      cocg13 = cocg[cell_id][0][2];
      cocg22 = cocg[cell_id][1][1];
      cocg23 = cocg[cell_id][1][2];
      cocg33 = cocg[cell_id][2][2];

      a11 = cocg22*cocg33 - cocg23*cocg23;
      a12 = cocg23*cocg13 - cocg12*cocg33;
      a13 = cocg12*cocg23 - cocg22*cocg13;
      a22 = cocg11*cocg33 - cocg13*cocg13;
      a23 = cocg12*cocg13 - cocg11*cocg23;
      a33 = cocg11*cocg22 - cocg12*cocg12;

      det_inv = 1. / (cocg11*a11 + cocg12*a12 + cocg13*a13);

      cocg[cell_id][0][0] = a11 * det_inv;
      cocg[cell_id][0][1] = a12 * det_inv;
      cocg[cell_id][0][2] = a13 * det_inv;
      cocg[cell_id][1][0] = a12 * det_inv;
      cocg[cell_id][1][1] = a22 * det_inv;
      cocg[cell_id][1][2] = a23 * det_inv;
      cocg[cell_id][2][0] = a13 * det_inv;
      cocg[cell_id][2][1] = a23 * det_inv;
      cocg[cell_id][2][2] = a33 * det_inv;

    }

  }
  else if (iccocg == 1) {

  /* Recompute cocg at boundaries, using saved cocgb */

#   pragma omp parallel for private(cell_id, ll, mm)
    for (ii = 0; ii < m->n_b_cells; ii++) {
      cell_id = m->b_cells[ii] - 1;
      for (ll = 0; ll < 3; ll++) {
        for (mm = 0; mm < 3; mm++)
          cocg[cell_id][ll][mm] = cocgb[ii][ll][mm];
      }
    }

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, mm, \
                                      extrab, umcbdd, udbfs, dddij)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          extrab = 1. - isympa[face_id]*extrap*coefbp[face_id];

          umcbdd = extrab * (1. - coefbp[face_id]) / b_dist[face_id];
          udbfs = extrab / b_face_surf[face_id];

          for (ll = 0; ll < 3; ll++)
            dddij[ll] =   udbfs * b_face_normal[face_id][ll]
                        + umcbdd * diipb[face_id][ll];

          for (ll = 0; ll < 3; ll++) {
            for (mm = 0; mm < 3; mm++)
              cocg[ii][ll][mm] += dddij[ll]*dddij[mm];
          }

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

#   pragma omp parallel for private(cell_id, cocg11, cocg12, cocg13, cocg22, \
                                    cocg23, cocg33, a11, a12, a13, a22, \
                                    a23, a33, det_inv)
    for (ii = 0; ii < m->n_b_cells; ii++) {

      cell_id = m->b_cells[ii] - 1;

      cocg11 = cocg[cell_id][0][0];
      cocg12 = cocg[cell_id][0][1];
      cocg13 = cocg[cell_id][0][2];
      cocg22 = cocg[cell_id][1][1];
      cocg23 = cocg[cell_id][1][2];
      cocg33 = cocg[cell_id][2][2];

      a11 = cocg22*cocg33 - cocg23*cocg23;
      a12 = cocg23*cocg13 - cocg12*cocg33;
      a13 = cocg12*cocg23 - cocg22*cocg13;
      a22 = cocg11*cocg33 - cocg13*cocg13;
      a23 = cocg12*cocg13 - cocg11*cocg23;
      a33 = cocg11*cocg22 - cocg12*cocg12;

      det_inv = 1. / (cocg11*a11 + cocg12*a12 + cocg13*a13);

      cocg[cell_id][0][0] = a11 * det_inv;
      cocg[cell_id][0][1] = a12 * det_inv;
      cocg[cell_id][0][2] = a13 * det_inv;
      cocg[cell_id][1][0] = a12 * det_inv;
      cocg[cell_id][1][1] = a22 * det_inv;
      cocg[cell_id][1][2] = a23 * det_inv;
      cocg[cell_id][2][0] = a13 * det_inv;
      cocg[cell_id][2][1] = a23 * det_inv;
      cocg[cell_id][2][2] = a33 * det_inv;

    }

  } /* End of iccocg == 1 */

  /* Compute Right-Hand Side */
  /*-------------------------*/

# pragma omp parallel for
  for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
    rhsv[cell_id][0] = 0.0;
    rhsv[cell_id][1] = 0.0;
    rhsv[cell_id][2] = 0.0;
    rhsv[cell_id][3] = pvar[cell_id];
  }

  /* Standard case, without hydrostatic pressure */
  /*---------------------------------------------*/

  if (iphydp == 0) {

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, pfac, dc, fctb)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];

          pfac =   (rhsv[jj][3] - rhsv[ii][3])
                 / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++)
            fctb[ll] = dc[ll] * pfac;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += fctb[ll];

          for (ll = 0; ll < 3; ll++)
            rhsv[jj][ll] += fctb[ll];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Contribution from extended neighborhood */

    if (imrgra == 2 || imrgra == 3) {

#     pragma omp parallel for private(cidx, jj, ll, dc, fctb, pfac)
      for (ii = 0; ii < n_cells; ii++) {
        for (cidx = cell_cells_idx[ii]; cidx < cell_cells_idx[ii+1]; cidx++) {

          jj = cell_cells_lst[cidx - 1] - 1;

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];

          pfac =   (rhsv[jj][3] - rhsv[ii][3])
                 / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++)
            fctb[ll] = dc[ll] * pfac;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += fctb[ll];

        }
      }

    } /* End for extended neighborhood */

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, extrab, \
                                      unddij, udbfs, umcbdd, pfac, dsij)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          extrab = pow((1. - isympa[face_id]*extrap*coefbp[face_id]), 2.0);
          unddij = 1. / b_dist[face_id];
          udbfs = 1. / b_face_surf[face_id];
          umcbdd = (1. - coefbp[face_id]) * unddij;

          for (ll = 0; ll < 3; ll++)
            dsij[ll] =   udbfs * b_face_normal[face_id][ll]
                       + umcbdd*diipb[face_id][ll];

          pfac =   (coefap[face_id]*inc + (coefbp[face_id] -1.)*rhsv[ii][3])
                 * unddij * extrab;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += dsij[ll] * pfac;

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

  }

  /* Case with hydrostatic pressure */
  /*--------------------------------*/

  else {  /* if iphydp != 0 */

    /* Contribution from interior faces */

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, jj, ll, dc, pfac, fctb)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          ii = i_face_cells[face_id][0] - 1;
          jj = i_face_cells[face_id][1] - 1;

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];

          pfac =   (  rhsv[jj][3] - rhsv[ii][3]
                    + (cell_cen[ii][0] - i_face_cog[face_id][0]) * fextx[ii]
                    + (cell_cen[ii][1] - i_face_cog[face_id][1]) * fexty[ii]
                    + (cell_cen[ii][2] - i_face_cog[face_id][2]) * fextz[ii]
                    - (cell_cen[jj][0] - i_face_cog[face_id][0]) * fextx[jj]
                    - (cell_cen[jj][1] - i_face_cog[face_id][1]) * fexty[jj]
                    - (cell_cen[jj][2] - i_face_cog[face_id][2]) * fextz[jj])
                  / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++)
            fctb[ll] = dc[ll] * pfac;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += fctb[ll];

          for (ll = 0; ll < 3; ll++)
            rhsv[jj][ll] += fctb[ll];

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

    /* Contribution from extended neighborhood;
       We assume that the middle of the segment joining cell centers
       may replace the center of gravity of a fictitious face. */

    if (imrgra == 2 || imrgra == 3) {

#     pragma omp parallel for private(cidx, jj, ll, dc, fctb, pfac)
      for (ii = 0; ii < n_cells; ii++) {
        for (cidx = cell_cells_idx[ii]; cidx < cell_cells_idx[ii+1]; cidx++) {

          jj = cell_cells_lst[cidx - 1] - 1;

          /* Note: replaced the expressions:
           *  a) ptmid = 0.5 * (cell_cen[jj] - cell_cen[ii])
           *  b)   (cell_cen[ii] - ptmid) * fext[ii]
           *  c) - (cell_cen[jj] - ptmid) * fext[jj]
           * with:
           *  a) dc = cell_cen[jj] - cell_cen[ii]
           *  b) - 0.5 * dc * fext[ii]
           *  c) - 0.5 * dc * fext[jj]
           */

          for (ll = 0; ll < 3; ll++)
            dc[ll] = cell_cen[jj][ll] - cell_cen[ii][ll];

          pfac =   (  rhsv[jj][3] - rhsv[ii][3]
                    - 0.5 * dc[0] * fextx[ii]
                    - 0.5 * dc[1] * fexty[ii]
                    - 0.5 * dc[2] * fextz[ii]
                    - 0.5 * dc[0] * fextx[jj]
                    - 0.5 * dc[1] * fexty[jj]
                    - 0.5 * dc[2] * fextz[jj])
                  / (dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

          for (ll = 0; ll < 3; ll++)
            fctb[ll] = dc[ll] * pfac;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += fctb[ll];

        }
      }

    } /* End for extended neighborhood */

    /* Contribution from boundary faces */

    for (g_id = 0; g_id < n_b_groups; g_id++) {

#     pragma omp parallel for private(face_id, ii, ll, extrab, \
                                      unddij, udbfs, umcbdd, pfac, dsij)
      for (t_id = 0; t_id < n_b_threads; t_id++) {

        for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
             face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
             face_id++) {

          ii = b_face_cells[face_id] - 1;

          extrab = pow((1. - isympa[face_id]*extrap*coefbp[face_id]), 2.0);
          unddij = 1. / b_dist[face_id];
          udbfs = 1. / b_face_surf[face_id];
          umcbdd = (1. - coefbp[face_id]) * unddij;

          for (ll = 0; ll < 3; ll++)
            dsij[ll] =   udbfs * b_face_normal[face_id][ll]
                       + umcbdd*diipb[face_id][ll];

          pfac
            =   (coefap[face_id]*inc
                 + (  (coefbp[face_id] -1.)
                    * (  rhsv[ii][3]
                       + (b_face_cog[face_id][0] - cell_cen[ii][0]) * fextx[ii]
                       + (b_face_cog[face_id][1] - cell_cen[ii][1]) * fexty[ii]
                       + (b_face_cog[face_id][2] - cell_cen[ii][2]) * fextz[ii])))
              * unddij * extrab;

          for (ll = 0; ll < 3; ll++)
            rhsv[ii][ll] += dsij[ll] * pfac;

        } /* loop on faces */

      } /* loop on threads */

    } /* loop on thread groups */

  } /* End of test on hydrostatic pressure */

  /* Compute gradient */
  /*------------------*/

  if (iphydp == 1) {

#   pragma omp parallel for
    for (cell_id = 0; cell_id < n_cells; cell_id++) {
      dpdxyz[cell_id][0] =   cocg[cell_id][0][0] *rhsv[cell_id][0]
                           + cocg[cell_id][0][1] *rhsv[cell_id][1]
                           + cocg[cell_id][0][2] *rhsv[cell_id][2]
                           + fextx[cell_id];
      dpdxyz[cell_id][1] =   cocg[cell_id][1][0] *rhsv[cell_id][0]
                           + cocg[cell_id][1][1] *rhsv[cell_id][1]
                           + cocg[cell_id][1][2] *rhsv[cell_id][2]
                           + fexty[cell_id];
      dpdxyz[cell_id][2] =   cocg[cell_id][2][0] *rhsv[cell_id][0]
                           + cocg[cell_id][2][1] *rhsv[cell_id][1]
                           + cocg[cell_id][2][2] *rhsv[cell_id][2]
                           + fextz[cell_id];
    }

  }
  else {

#   pragma omp parallel for
    for (cell_id = 0; cell_id < n_cells; cell_id++) {
      dpdxyz[cell_id][0] =   cocg[cell_id][0][0] *rhsv[cell_id][0]
                           + cocg[cell_id][0][1] *rhsv[cell_id][1]
                           + cocg[cell_id][0][2] *rhsv[cell_id][2];
      dpdxyz[cell_id][1] =   cocg[cell_id][1][0] *rhsv[cell_id][0]
                           + cocg[cell_id][1][1] *rhsv[cell_id][1]
                           + cocg[cell_id][1][2] *rhsv[cell_id][2];
      dpdxyz[cell_id][2] =   cocg[cell_id][2][0] *rhsv[cell_id][0]
                           + cocg[cell_id][2][1] *rhsv[cell_id][1]
                           + cocg[cell_id][2][2] *rhsv[cell_id][2];
    }

  }

  /* Synchronize halos */

  _sync_scalar_gradient_halo(m, idimtr, dpdxyz);
}

/*----------------------------------------------------------------------------
 * Clip the gradient of a vector if necessary. This function deals with the
 * standard or extended neighborhood.
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   halo_type      <-- halo type (extended or not)
 *   clipping_type  <-- type of clipping for the computation of the gradient
 *   verbosity      <-- output level
 *   climgp         <-- clipping coefficient for the computation of the gradient
 *   pvar           <-- variable
 *   gradv          <-> gradient of pvar
 *   pvar           <-- variable
 *----------------------------------------------------------------------------*/

static void
_vector_gradient_clipping(const cs_mesh_t              *m,
                          const cs_mesh_quantities_t   *fvq,
                          cs_halo_type_t                halo_type,
                          int                           clipping_type,
                          int                           verbosity,
                          cs_real_t                     climgp,
                          const cs_real_3_t   *restrict pvar,
                          cs_real_33_t        *restrict gradv)
{
  int        g_id, t_id;
  cs_gnum_t  t_n_clip;
  cs_lnum_t  cell_id, cell_id1, cell_id2, cidx, face_id, i, j;
  cs_real_3_t dist, grad_dist1, grad_dist2;
  cs_real_t  dvar_sq, dist_sq1, dist_sq2;
  cs_real_t  global_min_factor, global_max_factor, factor1, factor2;
  cs_real_t  t_max_factor, t_min_factor;

  cs_gnum_t  n_clip = 0, n_g_clip =0;
  cs_real_t  min_factor = 1;
  cs_real_t  max_factor = 0;
  cs_real_t  clipp_coef_sq = climgp*climgp;
  cs_real_t  *restrict buf = NULL, *restrict clip_factor = NULL;
  cs_real_t  *restrict denom = NULL, *restrict denum = NULL;

  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict cell_cells_idx
    = (const cs_lnum_t *restrict)m->cell_cells_idx;
  const cs_lnum_t *restrict cell_cells_lst
    = (const cs_lnum_t *restrict)m->cell_cells_lst;

  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;

  const cs_halo_t *halo = m->halo;

  if (clipping_type < 0)
    return;

  /* The gradient and the variable must be already synchronized */

  /* Allocate and initialize working buffers */

  if (clipping_type == 1)
    BFT_MALLOC(buf, 3*n_cells_ext, cs_real_t);
  else
    BFT_MALLOC(buf, 2*n_cells_ext, cs_real_t);

  denum = buf;
  denom = buf + n_cells_ext;

  if (clipping_type == 1)
    clip_factor = buf + 2*n_cells_ext;

  /* Initialization */

# pragma omp parallel for
  for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
    denum[cell_id] = 0;
    denom[cell_id] = 0;
    if (clipping_type == 1)
      clip_factor[cell_id] = (cs_real_t)DBL_MAX;
  }

  /* Remark:
     denum: holds the maximum l2 norm of the variation of the gradient squared
     denom: holds the maximum l2 norm of the variation of the variable squared */

  /* First clipping Algorithm: based on the cell gradient */
  /*------------------------------------------------------*/

  if (clipping_type == 0) {

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, cell_id1, cell_id2, i, \
                                      dist, grad_dist1, grad_dist2, \
                                      dist_sq1, dist_sq2, dvar_sq)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          cell_id1 = i_face_cells[face_id][0] - 1;
          cell_id2 = i_face_cells[face_id][1] - 1;

          for (i = 0; i < 3; i++)
            dist[i] = cell_cen[cell_id1][i] - cell_cen[cell_id2][i];

          for (i = 0; i < 3; i++) {

            grad_dist1[i] =   gradv[cell_id1][0][i] * dist[0]
                            + gradv[cell_id1][1][i] * dist[1]
                            + gradv[cell_id1][2][i] * dist[2];

            grad_dist2[i] =   gradv[cell_id2][0][i] * dist[0]
                            + gradv[cell_id2][1][i] * dist[1]
                            + gradv[cell_id2][2][i] * dist[2];

          }

          dist_sq1 =   grad_dist1[0]*grad_dist1[0]
                     + grad_dist1[1]*grad_dist1[1]
                     + grad_dist1[2]*grad_dist1[2];

          dist_sq2 =   grad_dist2[0]*grad_dist2[0]
                     + grad_dist2[1]*grad_dist2[1]
                     + grad_dist2[2]*grad_dist2[2];

          dvar_sq =     (pvar[cell_id1][0]-pvar[cell_id2][0])
                      * (pvar[cell_id1][0]-pvar[cell_id2][0])
                    +   (pvar[cell_id1][1]-pvar[cell_id2][1])
                      * (pvar[cell_id1][1]-pvar[cell_id2][1])
                    +   (pvar[cell_id1][2]-pvar[cell_id2][2])
                      * (pvar[cell_id1][2]-pvar[cell_id2][2]);

          denum[cell_id1] = CS_MAX(denum[cell_id1], dist_sq1);
          denum[cell_id2] = CS_MAX(denum[cell_id2], dist_sq2);
          denom[cell_id1] = CS_MAX(denom[cell_id1], dvar_sq);
          denom[cell_id2] = CS_MAX(denom[cell_id2], dvar_sq);

        } /* End of loop on faces */

      } /* End of loop on threads */

    } /* End of loop on thread groups */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, cell_id2, i, dist, \
                                      grad_dist1, dist_sq1, dvar_sq)
      for (cell_id1 = 0; cell_id1 < n_cells; cell_id1++) {
        for (cidx = cell_cells_idx[cell_id1];
             cidx < cell_cells_idx[cell_id1+1];
             cidx++) {

          cell_id2 = cell_cells_lst[cidx-1] - 1;

          for (i = 0; i < 3; i++)
            dist[i] = cell_cen[cell_id1][i] - cell_cen[cell_id2][i];

          for (i = 0; i < 3; i++)
            grad_dist1[i] =   gradv[cell_id1][0][i] * dist[0]
                            + gradv[cell_id1][1][i] * dist[1]
                            + gradv[cell_id1][2][i] * dist[2];


          dist_sq1 =   grad_dist1[0]*grad_dist1[0]
                     + grad_dist1[1]*grad_dist1[1]
                     + grad_dist1[2]*grad_dist1[2];

          dvar_sq =     (pvar[cell_id1][0]-pvar[cell_id2][0])
                      * (pvar[cell_id1][0]-pvar[cell_id2][0])
                    +   (pvar[cell_id1][1]-pvar[cell_id2][1])
                      * (pvar[cell_id1][1]-pvar[cell_id2][1])
                    +   (pvar[cell_id1][2]-pvar[cell_id2][2])
                      * (pvar[cell_id1][2]-pvar[cell_id2][2]);

          denum[cell_id1] = CS_MAX(denum[cell_id1], dist_sq1);
          denom[cell_id1] = CS_MAX(denom[cell_id1], dvar_sq);

        }
      }

    } /* End for extended halo */

  }

  /* Second clipping Algorithm: based on the face gradient */
  /*-------------------------------------------------------*/

  else if (clipping_type == 1) {

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, cell_id1, cell_id2, i, \
                                      dist, grad_dist1, dist_sq1, dvar_sq)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          cell_id1 = i_face_cells[face_id][0] - 1;
          cell_id2 = i_face_cells[face_id][1] - 1;

          for (i = 0; i < 3; i++)
            dist[i] = cell_cen[cell_id1][i] - cell_cen[cell_id2][i];

          for (i = 0; i < 3; i++)
            grad_dist1[i]
              = 0.5 * (  (gradv[cell_id1][0][i]+gradv[cell_id2][0][i])*dist[0]
                       + (gradv[cell_id1][1][i]+gradv[cell_id2][1][i])*dist[1]
                       + (gradv[cell_id1][2][i]+gradv[cell_id2][2][i])*dist[2]);

          dist_sq1 =   grad_dist1[0]*grad_dist1[0]
                     + grad_dist1[1]*grad_dist1[1]
                     + grad_dist1[2]*grad_dist1[2];

          dvar_sq =     (pvar[cell_id1][0]-pvar[cell_id2][0])
                      * (pvar[cell_id1][0]-pvar[cell_id2][0])
                    +   (pvar[cell_id1][1]-pvar[cell_id2][1])
                      * (pvar[cell_id1][1]-pvar[cell_id2][1])
                    +   (pvar[cell_id1][2]-pvar[cell_id2][2])
                      * (pvar[cell_id1][2]-pvar[cell_id2][2]);

          denum[cell_id1] = CS_MAX(denum[cell_id1], dist_sq1);
          denum[cell_id2] = CS_MAX(denum[cell_id2], dist_sq1);
          denom[cell_id1] = CS_MAX(denom[cell_id1], dvar_sq);
          denom[cell_id2] = CS_MAX(denom[cell_id2], dvar_sq);

        } /* End of loop on threads */

      } /* End of loop on thread groups */

    } /* End of loop on faces */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, cell_id2, i, dist, \
                                      grad_dist1, dist_sq1, dvar_sq)
      for (cell_id1 = 0; cell_id1 < n_cells; cell_id1++) {
        for (cidx = cell_cells_idx[cell_id1];
             cidx < cell_cells_idx[cell_id1+1];
             cidx++) {

          cell_id2 = cell_cells_lst[cidx-1] - 1;

          for (i = 0; i < 3; i++)
            dist[i] = cell_cen[cell_id1][i] - cell_cen[cell_id2][i];

          for (i = 0; i < 3; i++)
            grad_dist1[i]
              = 0.5 * (  (gradv[cell_id1][0][i]+gradv[cell_id2][0][i])*dist[0]
                       + (gradv[cell_id1][1][i]+gradv[cell_id2][1][i])*dist[1]
                       + (gradv[cell_id1][2][i]+gradv[cell_id2][2][i])*dist[2]);

          dist_sq1 =   grad_dist1[0]*grad_dist1[0]
                     + grad_dist1[1]*grad_dist1[1]
                     + grad_dist1[2]*grad_dist1[2];

          dvar_sq =     (pvar[cell_id1][0]-pvar[cell_id2][0])
                      * (pvar[cell_id1][0]-pvar[cell_id2][0])
                    +   (pvar[cell_id1][1]-pvar[cell_id2][1])
                      * (pvar[cell_id1][1]-pvar[cell_id2][1])
                    +   (pvar[cell_id1][2]-pvar[cell_id2][2])
                      * (pvar[cell_id1][2]-pvar[cell_id2][2]);

          denum[cell_id1] = CS_MAX(denum[cell_id1], dist_sq1);
          denom[cell_id1] = CS_MAX(denom[cell_id1], dvar_sq);

        }
      }

    } /* End for extended neighborhood */

    /* Synchronize variable */

    if (halo != NULL) {
      cs_halo_sync_var(m->halo, halo_type, denom);
      cs_halo_sync_var(m->halo, halo_type, denum);
    }

  } /* End if clipping_type == 1 */

  /* Clipping of the gradient if denum/denom > climgp**2 */

  /* First clipping Algorithm: based on the cell gradient */
  /*------------------------------------------------------*/

  if (clipping_type == 0) {

#   pragma omp parallel private(t_min_factor, t_max_factor, t_n_clip, \
                                factor1, i, j)
    {
      t_n_clip = 0;
      t_min_factor = min_factor; t_max_factor = max_factor;

#     pragma omp for
      for (cell_id = 0; cell_id < n_cells; cell_id++) {

        if (denum[cell_id] > clipp_coef_sq * denom[cell_id]) {

          factor1 = sqrt(clipp_coef_sq * denom[cell_id]/denum[cell_id]);

          for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++)
              gradv[cell_id][i][j] *= factor1;
          }

          t_min_factor = CS_MIN(factor1, t_min_factor);
          t_max_factor = CS_MAX(factor1, t_max_factor);
          t_n_clip++;

        } /* If clipping */

      } /* End of loop on cells */

#     pragma omp critical
      {
        min_factor = CS_MIN(min_factor, t_min_factor);
        max_factor = CS_MAX(max_factor, t_max_factor);
        n_clip += t_n_clip;
      }
    } /* End of omp parallel construct */

  }

  /* Second clipping Algorithm: based on the face gradient */
  /*-------------------------------------------------------*/

  else if (clipping_type == 1) {

    for (g_id = 0; g_id < n_i_groups; g_id++) {

#     pragma omp parallel for private(face_id, cell_id1, cell_id2, \
                                      factor1, factor2, min_factor)
      for (t_id = 0; t_id < n_i_threads; t_id++) {

        for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
             face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
             face_id++) {

          cell_id1 = i_face_cells[face_id][0] - 1;
          cell_id2 = i_face_cells[face_id][1] - 1;

          factor1 = 1.0;
          if (denum[cell_id1] > clipp_coef_sq * denom[cell_id1])
            factor1 = sqrt(clipp_coef_sq * denom[cell_id1]/denum[cell_id1]);

          factor2 = 1.0;
          if (denum[cell_id2] > clipp_coef_sq * denom[cell_id2])
            factor2 = sqrt(clipp_coef_sq * denom[cell_id2]/denum[cell_id2]);

          min_factor = CS_MIN(factor1, factor2);

          clip_factor[cell_id1] = CS_MIN(clip_factor[cell_id1], min_factor);
          clip_factor[cell_id2] = CS_MIN(clip_factor[cell_id2], min_factor);

        } /* End of loop on faces */

      } /* End of loop on threads */

    } /* End of loop on thread groups */

    /* Complement for extended neighborhood */

    if (cell_cells_idx != NULL && halo_type == CS_HALO_EXTENDED) {

#     pragma omp parallel for private(cidx, cell_id2, min_factor, factor2)
      for (cell_id1 = 0; cell_id1 < n_cells; cell_id1++) {

        min_factor = 1.0;

        for (cidx = cell_cells_idx[cell_id1];
             cidx < cell_cells_idx[cell_id1+1];
             cidx++) {

          cell_id2 = cell_cells_lst[cidx-1] - 1;
          factor2 = 1.0;

          if (denum[cell_id2] > clipp_coef_sq * denom[cell_id2])
            factor2 = sqrt(clipp_coef_sq * denom[cell_id2]/denum[cell_id2]);

          min_factor = CS_MIN(min_factor, factor2);

        }

        clip_factor[cell_id1] = CS_MIN(clip_factor[cell_id1], min_factor);

      } /* End of loop on cells */

    } /* End for extended neighborhood */

#   pragma omp parallel private(t_min_factor, t_max_factor, factor1, \
                                t_n_clip, i, j)
    {
      t_n_clip = 0;
      t_min_factor = min_factor; t_max_factor = max_factor;

#     pragma omp for
      for (cell_id = 0; cell_id < n_cells; cell_id++) {

        for (i = 0; i < 3; i++) {
          for (j = 0; j < 3; j++)
            gradv[cell_id][i][j] *= clip_factor[cell_id];
        }

        if (clip_factor[cell_id] < 0.99) {
          t_max_factor = CS_MAX(t_max_factor, clip_factor[cell_id]);
          t_min_factor = CS_MIN(t_min_factor, clip_factor[cell_id]);
          t_n_clip++;
        }

      } /* End of loop on cells */

#     pragma omp critical
      {
        min_factor = CS_MIN(min_factor, t_min_factor);
        max_factor = CS_MAX(max_factor, t_max_factor);
        n_clip += t_n_clip;
      }
    } /* End of omp parallel construct */

  } /* End if clipping_type == 1 */

  /* Update min/max and n_clip in case of parallelism */
  /*--------------------------------------------------*/

#if defined(HAVE_MPI)

  if (m->n_domains > 1) {

    assert(sizeof(cs_real_t) == sizeof(double));

    /* Global Max */

    MPI_Allreduce(&max_factor, &global_max_factor, 1, CS_MPI_REAL,
                  MPI_MAX, cs_glob_mpi_comm);

    max_factor = global_max_factor;

    /* Global min */

    MPI_Allreduce(&min_factor, &global_min_factor, 1, CS_MPI_REAL,
                  MPI_MIN, cs_glob_mpi_comm);

    min_factor = global_min_factor;

    /* Sum number of clippings */

    MPI_Allreduce(&n_clip, &n_g_clip, 1, CS_MPI_GNUM,
                  MPI_SUM, cs_glob_mpi_comm);

    n_clip = n_g_clip;

  } /* If n_domains > 1 */

#endif /* defined(HAVE_MPI) */

  /* Output warning if necessary */

  if (verbosity > 1)
    bft_printf(_(" Gradient of a vector limitation in %llu cells\n"
                 "   minimum factor = %14.5e; maximum factor = %14.5e\n"),
               (unsigned long long)n_clip, min_factor, max_factor);

  /* Synchronize the updated Gradient */

  if (m->halo != NULL) {
    cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)gradv, 9);
    if (cs_glob_mesh->n_init_perio > 0)
      cs_halo_perio_sync_var_tens(m->halo, halo_type, (cs_real_t *)gradv);
  }

  BFT_FREE(buf);
}

/*----------------------------------------------------------------------------
 * Initialize the gradient of a vector for gradient reconstruction.
 *
 * A non-reconstructed gradient is computed at this stage.
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   halo_type      <-- halo type (extended or not)
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   coefav         <-- B.C. coefficients for boundary face normals
 *   coefbv         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *----------------------------------------------------------------------------*/

static void
_initialize_vector_gradient(const cs_mesh_t              *m,
                            const cs_mesh_quantities_t   *fvq,
                            cs_halo_type_t                halo_type,
                            int                           inc,
                            const cs_real_3_t   *restrict coefav,
                            const cs_real_33_t  *restrict coefbv,
                            const cs_real_3_t   *restrict pvar,
                            cs_real_33_t        *restrict gradv)
{
  int g_id, t_id;
  cs_lnum_t  cell_id, face_id, i, j, cell_id1, cell_id2;
  cs_real_t  pfac, pond, dvol, dvol1, dvol2;

  const cs_lnum_t n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict weight = fvq->weight;
  const cs_real_t *restrict cell_vol = fvq->cell_vol;
  const cs_real_3_t *restrict i_face_normal
    = (const cs_real_3_t *restrict)fvq->i_face_normal;
  const cs_real_3_t *restrict b_face_normal
    = (const cs_real_3_t *restrict)fvq->b_face_normal;

  /* By default, handle the gradient as a tensor
     (i.e. we assume it is the gradient of a vector field) */

  if (m->halo != NULL) {
    cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)pvar, 3);
    if (cs_glob_mesh->n_init_perio > 0)
      cs_halo_perio_sync_var_vect(m->halo, halo_type, (cs_real_t *)pvar, 3);
  }

  /* Computation without reconstruction */
  /*------------------------------------*/

  /* Initialization */

# pragma omp parallel for private(i, j)
  for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
    for (j = 0; j < 3; j++) {
      for (i = 0; i < 3; i++)
        gradv[cell_id][j][i] = 0.0;
    }
  }

  /* Interior faces contribution */

  for (g_id = 0; g_id < n_i_groups; g_id++) {

#   pragma omp parallel for private(face_id, cell_id1, cell_id2, i, j,  \
                                    pond, dvol1, dvol2, pfac)
    for (t_id = 0; t_id < n_i_threads; t_id++) {

      for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
           face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
           face_id++) {

        cell_id1 = i_face_cells[face_id][0] - 1;
        cell_id2 = i_face_cells[face_id][1] - 1;

        pond = weight[face_id];
        dvol1 = 1./cell_vol[cell_id1];
        dvol2 = 1./cell_vol[cell_id2];

        for (i = 0; i < 3; i++) {
          pfac   = pond * pvar[cell_id1][i] + (1.0-pond) * pvar[cell_id2][i];
          for (j = 0; j < 3; j++) {
            gradv[cell_id1][j][i] += pfac * i_face_normal[face_id][j] * dvol1;
            gradv[cell_id2][j][i] -= pfac * i_face_normal[face_id][j] * dvol2;
          }
        }

      } /* End of loop on faces */

    } /* End of loop on threads */

  } /* End of loop on thread groups */

  /* Boundary face treatment */

  for (g_id = 0; g_id < n_b_groups; g_id++) {

#   pragma omp parallel for private(face_id, cell_id, i,j, dvol, pfac)
    for (t_id = 0; t_id < n_b_threads; t_id++) {

      for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
           face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
           face_id++) {

        cell_id = b_face_cells[face_id] - 1;

        dvol = 1./cell_vol[cell_id];

        for (i = 0; i < 3; i++) {
          pfac =   inc*coefav[face_id][i]
                 + coefbv[face_id][0][i] * pvar[cell_id][0]
                 + coefbv[face_id][1][i] * pvar[cell_id][1]
                 + coefbv[face_id][2][i] * pvar[cell_id][2];
          for (j = 0; j < 3; j++)
            gradv[cell_id][j][i] += pfac * b_face_normal[face_id][j]*dvol;
        }

      } /* loop on faces */

    } /* loop on threads */

  } /* loop on thread groups */

  /* Periodicity and parallelism treatment */

  if (m->halo != NULL) {
    cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)gradv, 9);
    if (cs_glob_mesh->n_init_perio > 0)
      cs_halo_perio_sync_var_tens(m->halo, halo_type, (cs_real_t *)gradv);
  }
}

/*----------------------------------------------------------------------------
 * Compute the gradient of a vector with an iterative technique in order to
 * handle non-orthoganalities (nswrgp > 1).
 *
 * We do not take into account any volumic force here.
 *
 * Compute cocg at the first call and if needed.
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   var_num        <-- variable's number (0 if non-solved variable)
 *   halo_type      <-- halo type (extended or not)
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   nswrgp         --> >1: with reconstruction
 *   verbosity      --> verbosity level
 *   epsrgp         --> precision for iterative gradient calculation
 *   coefav         <-- B.C. coefficients for boundary face normals
 *   coefbv         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *   gradv          <-> gradient of pvar
 *----------------------------------------------------------------------------*/

static void
_iterative_vector_gradient(const cs_mesh_t              *m,
                           const cs_mesh_quantities_t   *fvq,
                           int                           var_num,
                           cs_halo_type_t                halo_type,
                           int                           inc,
                           int                           nswrgp,
                           int                           verbosity,
                           double                        epsrgp,
                           const cs_real_3_t   *restrict coefav,
                           const cs_real_33_t  *restrict coefbv,
                           const cs_real_3_t   *restrict pvar,
                           cs_real_33_t        *restrict gradv)
{
  int isweep, g_id, t_id;
  cs_lnum_t  cell_id, face_id, i, j, k, cell_id1, cell_id2;
  cs_real_t  pfac, l2_norm, l2_residual, vecfac, pond;
  cs_real_t  dvol, dvol1, dvol2;

  cs_real_33_t *rhs;

  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict weight = fvq->weight;
  const cs_real_t *restrict cell_vol = fvq->cell_vol;
  const cs_real_3_t *restrict i_face_normal
    = (const cs_real_3_t *restrict)fvq->i_face_normal;
  const cs_real_3_t *restrict b_face_normal
    = (const cs_real_3_t *restrict)fvq->b_face_normal;
  const cs_real_3_t *restrict diipb
    = (const cs_real_3_t *restrict)fvq->diipb;
  const cs_real_3_t *restrict dofij
    = (const cs_real_3_t *restrict)fvq->dofij;
  cs_real_33_t *restrict cocg = fvq->cocg_it;

  BFT_MALLOC(rhs, n_cells_ext, cs_real_33_t);

  const cs_real_t epzero = 1.e-12;

  /* Gradient reconstruction to handle non-orthogonal meshes */
  /*---------------------------------------------------------*/

  /* L2 norm */

  l2_norm = _l2_norm_1(9*n_cells, (cs_real_t *)gradv);
  l2_residual = l2_norm;

  if (l2_norm > epzero) {

    /* Iterative process */
    /*-------------------*/

    for (isweep = 1; isweep < nswrgp && l2_residual > epsrgp*l2_norm; isweep++) {

      /* Computation of the Right Hand Side*/

#     pragma omp parallel for private(i, j)
      for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
        for (j = 0; j < 3; j++) {
          for (i = 0; i < 3; i++)
            rhs[cell_id][j][i] = -gradv[cell_id][j][i];
        }
      }

      /* Interior face treatment */

      for (g_id = 0; g_id < n_i_groups; g_id++) {

#       pragma omp parallel for private(face_id, cell_id1, cell_id2, \
                                        i, j, k, pond, dvol1, dvol2, pfac)
        for (t_id = 0; t_id < n_i_threads; t_id++) {

          for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
               face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
               face_id++) {

            cell_id1 = i_face_cells[face_id][0] - 1;
            cell_id2 = i_face_cells[face_id][1] - 1;
            pond = weight[face_id];

            dvol1 = 1./cell_vol[cell_id1];
            dvol2 = 1./cell_vol[cell_id2];

            for (i = 0; i < 3; i++) {
              pfac = pond*pvar[cell_id1][i] + (1.0-pond)*pvar[cell_id2][i];

              for (k = 0; k < 3; k++)
                pfac += 0.5 * (gradv[cell_id1][k][i] + gradv[cell_id2][k][i])
                            * dofij[face_id][k];

              for (j = 0; j < 3; j++) {
                rhs[cell_id1][j][i] += pfac * i_face_normal[face_id][j] * dvol1;
                rhs[cell_id2][j][i] -= pfac * i_face_normal[face_id][j] * dvol2;
              }
            }

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

      /* Boundary face treatment */

      for (g_id = 0; g_id < n_b_groups; g_id++) {

#       pragma omp parallel for private(face_id, cell_id, dvol, i, j, k, \
                                        pfac, vecfac)
        for (t_id = 0; t_id < n_b_threads; t_id++) {

          for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
               face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
               face_id++) {

            cell_id = b_face_cells[face_id] - 1;
            dvol = 1./cell_vol[cell_id];

            for (i = 0; i < 3; i++) {

              pfac = inc*coefav[face_id][i];

              for (k = 0; k < 3; k++) {
                vecfac =   pvar[cell_id][k]
                         + gradv[cell_id][0][k] * diipb[face_id][0]
                         + gradv[cell_id][1][k] * diipb[face_id][1]
                         + gradv[cell_id][2][k] * diipb[face_id][2];
                pfac += coefbv[face_id][k][i] * vecfac;
              }

              for (j = 0; j < 3; j++)
                rhs[cell_id][j][i] += pfac * b_face_normal[face_id][j] * dvol;

            }

          } /* loop on faces */

        } /* loop on threads */

      } /* loop on thread groups */

      /* Increment of the gradient */

#     pragma omp parallel for private(i, j, k)
      for (cell_id = 0; cell_id < n_cells; cell_id++) {
        for (j = 0; j < 3; j++) {
          for (i = 0; i < 3; i++) {
            for (k = 0; k < 3; k++)
              gradv[cell_id][j][i] += rhs[cell_id][k][i] * cocg[cell_id][k][j];
          }
        }
      }

      /* Periodicity and parallelism treatment */

      if (m->halo != NULL) {
        cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)gradv, 9);
        if (cs_glob_mesh->n_init_perio > 0)
          cs_halo_perio_sync_var_tens(m->halo, halo_type, (cs_real_t *)gradv);
      }

      /* Convergence test (L2 norm) */

      l2_residual = _l2_norm_1(9*n_cells, (cs_real_t *)rhs);

    } /* End of the iterative process */

    /* Printing */

    if (l2_residual < epsrgp*l2_norm) {
      if (verbosity >= 2) {
        bft_printf
          (_(" %s: isweep = %d, residue norm: %e, norm: %e, var_num = %d\n"),
           __func__, isweep, l2_residual/l2_norm, l2_norm, var_num);
      }
    }
    else if (isweep >= nswrgp) {
      if (verbosity >= 0) {
        bft_printf(" %s: isweep = %d, residu norm: %e, norm: %e, var_num = %d\n",
                   __func__, isweep, l2_residual/l2_norm, l2_norm, var_num);
        bft_printf("@ @@ warning: non convergence of grdvec\n");
      }
    }
  }

  BFT_FREE(rhs);
}

/*----------------------------------------------------------------------------
 * Compute cell gradient of a vector using least-squares reconstruction for
 * non-orthogonal meshes (nswrgp > 1).
 *
 * parameters:
 *   m              <-- pointer to associated mesh structure
 *   fvq            <-- pointer to associated finite volume quantities
 *   halo_type      <-- halo type (extended or not)
 *   inc            <-- if 0, solve on increment; 1 otherwise
 *   coefav         <-- B.C. coefficients for boundary face normals
 *   coefbv         <-- B.C. coefficients for boundary face normals
 *   pvar           <-- variable
 *   gradv          <-> gradient of pvar
 *----------------------------------------------------------------------------*/

static void
_lsq_vector_gradient(const cs_mesh_t              *m,
                     const cs_mesh_quantities_t   *fvq,
                     const cs_halo_type_t          halo_type,
                     const cs_int_t                inc,
                     const cs_real_3_t   *restrict coefav,
                     const cs_real_33_t  *restrict coefbv,
                     const cs_real_3_t   *restrict pvar,
                     cs_real_33_t        *restrict gradv)
{
  const int n_cells = m->n_cells;
  const int n_cells_ext = m->n_cells_with_ghosts;
  const int n_i_groups = m->i_face_numbering->n_groups;
  const int n_i_threads = m->i_face_numbering->n_threads;
  const int n_b_groups = m->b_face_numbering->n_groups;
  const int n_b_threads = m->b_face_numbering->n_threads;
  const cs_lnum_t *restrict i_group_index = m->i_face_numbering->group_index;
  const cs_lnum_t *restrict b_group_index = m->b_face_numbering->group_index;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;
  const cs_lnum_t *restrict cell_cells_idx
    = (const cs_lnum_t *restrict)m->cell_cells_idx;
  const cs_lnum_t *restrict cell_cells_lst
    = (const cs_lnum_t *restrict)m->cell_cells_lst;

  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;
  const cs_real_3_t *restrict b_face_cog
    = (const cs_real_3_t *restrict)fvq->b_face_cog;
  cs_real_33_t *restrict cocg = fvq->cocg_lsq;

  cs_lnum_t  cell_id, cidx, face_id, cell_id1, cell_id2, i, j, k;
  int        g_id, t_id;
  cs_real_t  pfac, ddc;
  cs_real_3_t  dc;
  cs_real_4_t  fctb;

  cs_real_33_t *rhs;

  BFT_MALLOC(rhs, n_cells_ext, cs_real_33_t);

  /* By default, handle the gradient as a tensor
     (i.e. we assume it is the gradient of a vector field) */

  if (m->halo != NULL) {
    cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)pvar, 3);
    if (cs_glob_mesh->n_init_perio > 0)
      cs_halo_perio_sync_var_vect(m->halo, halo_type, (cs_real_t *)pvar, 3);
  }

  /* Compute Right-Hand Side */
  /*-------------------------*/

# pragma omp parallel for private(i, j)
  for (cell_id = 0; cell_id < n_cells_ext; cell_id++) {
    for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
        rhs[cell_id][i][j] = 0.0;
  }

  /* Contribution from interior faces */

  for (g_id = 0; g_id < n_i_groups; g_id++) {

#   pragma omp parallel for private(face_id, cell_id1, cell_id2,\
                                    i, j, pfac, dc, fctb, ddc)
    for (t_id = 0; t_id < n_i_threads; t_id++) {

      for (face_id = i_group_index[(t_id*n_i_groups + g_id)*2];
           face_id < i_group_index[(t_id*n_i_groups + g_id)*2 + 1];
           face_id++) {

        cell_id1 = i_face_cells[face_id][0] - 1;
        cell_id2 = i_face_cells[face_id][1] - 1;

        for (i = 0; i < 3; i++)
          dc[i] = cell_cen[cell_id2][i] - cell_cen[cell_id1][i];

        ddc = 1./(dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

        for (i = 0; i < 3; i++) {
          pfac =  (pvar[cell_id2][i] - pvar[cell_id1][i]) * ddc;

          for (j = 0; j < 3; j++) {
            fctb[j] = dc[j] * pfac;
            rhs[cell_id1][j][i] += fctb[j];
            rhs[cell_id2][j][i] += fctb[j];
          }
        }

      } /* loop on faces */

    } /* loop on threads */

  } /* loop on thread groups */

  /* Contribution from extended neighborhood */

  if (halo_type == CS_HALO_EXTENDED) {

#   pragma omp parallel for private(cidx, cell_id2, dc, pfac, ddc, i, j)
    for (cell_id1 = 0; cell_id1 < n_cells; cell_id1++) {
      for (cidx = cell_cells_idx[cell_id1];
           cidx < cell_cells_idx[cell_id1+1];
           cidx++) {

        cell_id2 = cell_cells_lst[cidx - 1] - 1;

        for (i = 0; i < 3; i++)
          dc[i] = cell_cen[cell_id2][i] - cell_cen[cell_id1][i];

        ddc = 1./(dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

        for (i = 0; i < 3; i++) {

          pfac = (pvar[cell_id2][i] - pvar[cell_id1][i]) * ddc;

          for (j = 0; j < 3; j++) {
            rhs[cell_id1][j][i] += dc[j] * pfac;
          }
        }
      }
    }

  } /* End for extended neighborhood */

  /* Contribution from boundary faces */

  for (g_id = 0; g_id < n_b_groups; g_id++) {

#   pragma omp parallel for private(face_id, cell_id1, i, j, pfac, dc, ddc)
    for (t_id = 0; t_id < n_b_threads; t_id++) {

      for (face_id = b_group_index[(t_id*n_b_groups + g_id)*2];
           face_id < b_group_index[(t_id*n_b_groups + g_id)*2 + 1];
           face_id++) {

        cell_id1 = b_face_cells[face_id] - 1;

        for (i = 0; i < 3; i++)
          dc[i] = b_face_cog[face_id][i] - cell_cen[cell_id1][i];

        ddc = 1./(dc[0]*dc[0] + dc[1]*dc[1] + dc[2]*dc[2]);

        for (i = 0; i < 3; i++) {
          pfac = (coefav[face_id][i]*inc
               + ( coefbv[face_id][0][i] * pvar[cell_id1][0]
                 + coefbv[face_id][1][i] * pvar[cell_id1][1]
                 + coefbv[face_id][2][i] * pvar[cell_id1][2]
                 -                         pvar[cell_id1][i])) * ddc;

          for (j = 0; j < 3; j++)
            rhs[cell_id1][j][i] += dc[j] * pfac;
        }

      } /* loop on faces */

    } /* loop on threads */

  } /* loop on thread groups */

  /* Compute gradient */
  /*------------------*/

  for (cell_id = 0; cell_id < n_cells; cell_id++)
    for (j = 0; j < 3; j++)
      for (i = 0; i < 3; i++) {

        gradv[cell_id][j][i] = 0.0;

        for (k = 0; k < 3; k++)
          gradv[cell_id][j][i] += rhs[cell_id][k][i] * cocg[cell_id][k][j];

      }

  /* Periodicity and parallelism treatment */

  if (m->halo != NULL) {
    cs_halo_sync_var_strided(m->halo, halo_type, (cs_real_t *)gradv, 9);
    if (cs_glob_mesh->n_init_perio > 0)
      cs_halo_perio_sync_var_tens(m->halo, halo_type, (cs_real_t *)gradv);
  }

  BFT_FREE(rhs);
}

/*============================================================================
 * Public function definitions for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Compute cell gradient of scalar field or component of vector or
 * tensor field.
 *----------------------------------------------------------------------------*/

void CS_PROCF (cgdcel, CGDCEL)
(
 const cs_int_t   *const ivar,        /* <-- variable number                  */
 const cs_int_t   *const imrgra,      /* <-- gradient computation mode        */
 const cs_int_t   *const inc,         /* <-- 0 or 1: increment or not         */
 const cs_int_t   *const iccocg,      /* <-- 1 or 0: recompute COCG or not    */
 const cs_int_t   *const imobil,      /* <-- 1 for mobile mesh, 0 otherwise   */
 const cs_int_t   *const iale,        /* <-- 1 for ALE, 0 otherwise           */
 const cs_int_t   *const nswrgp,      /* <-- >1: with reconstruction          */
 const cs_int_t   *const idimtr,      /* <-- 0, 1, 2: scalar, vector, tensor
                                             in case of rotation              */
 const cs_int_t   *const iphydp,      /* <-- use hydrosatatic pressure        */
 const cs_int_t   *const iwarnp,      /* <-- verbosity level                  */
 const cs_int_t   *const imligp,      /* <-- type of clipping                 */
 const cs_real_t  *const epsrgp,      /* <-- precision for iterative gradient
                                             calculation                      */
 const cs_real_t  *const extrap,      /* <-- extrapolate gradient at boundary */
 const cs_real_t  *const climgp,      /* <-- clipping coefficient             */
 const cs_int_t          isympa[],    /* <-- indicator for symmetry faces     */
       cs_real_t         fextx[],     /* <-- components of the exterior force */
       cs_real_t         fexty[],     /*     generating the hydrostatic       */
       cs_real_t         fextz[],     /*     pressure                         */
 const cs_real_t         coefap[],    /* <-- boundary condition term          */
 const cs_real_t         coefbp[],    /* <-- boundary condition term          */
       cs_real_t         pvar[],      /* <-- gradient's base variable         */
       cs_real_t         grad[]       /* <-> gradient                         */
)
{
  cs_lnum_t ii;

  const cs_mesh_t  *mesh = cs_glob_mesh;
  const cs_halo_t  *halo = mesh->halo;

  cs_halo_type_t halo_type = CS_HALO_STANDARD;

  cs_gradient_info_t *gradient_info = NULL;
  cs_timer_t t0, t1;

  cs_real_4_t  *restrict rhsv;
  cs_real_t    *restrict dpdxyz;

  cs_lnum_t n_cells_ext = mesh->n_cells_with_ghosts;

  bool update_stats = true;
  cs_gradient_type_t gradient_type = CS_GRADIENT_N_TYPES;

  /* Allocate work arrays */

  BFT_MALLOC(dpdxyz, n_cells_ext*3, cs_real_t);
  BFT_MALLOC(rhsv, n_cells_ext, cs_real_4_t);

  /* Choose gradient type */

  switch (*imrgra) {
  case 0: gradient_type = CS_GRADIENT_ITER; break;
  case 1: gradient_type = CS_GRADIENT_LSQ_STD; break;
  case 2: gradient_type = CS_GRADIENT_LSQ_EXT; break;
  case 3: gradient_type = CS_GRADIENT_LSQ_EXT_RED; break;
  case 4: gradient_type = CS_GRADIENT_LSQ_ITER; break;
  default: break;
  }

  if (update_stats == true) {
    char var_name[32];
    snprintf(var_name, 31, "Var. %2d", *ivar); var_name[31] = '\0';
    t0 = cs_timer_time();
    gradient_info = _find_or_add_system(var_name, gradient_type);
  }

  if (*imrgra == 2 || *imrgra ==  3)
    halo_type = CS_HALO_EXTENDED;

  /* Synchronize variable */

  if (halo != NULL) {

    if (*idimtr > 0) {
      cs_real_t  *restrict dpdx = grad;
      cs_real_t  *restrict dpdy = grad + n_cells_ext;
      cs_real_t  *restrict dpdz = grad + n_cells_ext*2;
      cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, pvar);
      _initialize_rotation_values(halo,
                                  halo_type,
                                  dpdx,
                                  dpdy,
                                  dpdz,
                                  dpdxyz);
    }
    else
      cs_halo_sync_var(halo, halo_type, pvar);

    /* TODO: check if fext* components are all up to date, in which
     *       case we need no special treatment for *idimtr > 0 */

    if (*iphydp != 0) {

      if (*idimtr > 0){
        cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, fextx);
        cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, fexty);
        cs_halo_sync_component(halo, halo_type, CS_HALO_ROTATION_IGNORE, fextz);
      }
      else {
        cs_halo_sync_var(halo, halo_type, fextx);
        cs_halo_sync_var(halo, halo_type, fexty);
        cs_halo_sync_var(halo, halo_type, fextz);
        cs_halo_perio_sync_var_vect_ni(halo, halo_type,
                                       fextx, fexty, fextz);
      }
    }

  }

  /* Compute gradient */

  if (*imrgra == 0)
    _iterative_scalar_gradient(cs_glob_mesh,
                               cs_glob_mesh_quantities,
                               *ivar,
                               *imrgra,
                               *iccocg,
                               *imobil,
                               *iale,
                               *nswrgp,
                               *idimtr,
                               *iphydp,
                               *iwarnp,
                               *inc,
                               *epsrgp,
                               *extrap,
                               fextx,
                               fexty,
                               fextz,
                               coefap,
                               coefbp,
                               pvar,
                               (cs_real_3_t *)dpdxyz,
                               rhsv);

  else if (*imrgra == 1 || *imrgra == 2 || *imrgra == 3) {

    _lsq_scalar_gradient(cs_glob_mesh,
                         cs_glob_mesh_quantities,
                         *imrgra,
                         *iccocg,
                         *imobil,
                         *iale,
                         *nswrgp,
                         *idimtr,
                         *iphydp,
                         *inc,
                         *extrap,
                         isympa,
                         fextx,
                         fexty,
                         fextz,
                         coefap,
                         coefbp,
                         pvar,
                         (cs_real_3_t *)dpdxyz,
                         rhsv);

  }
  else if (*imrgra == 4) {

    const cs_int_t  _imlini = 1;
    const cs_real_t _climin = 1.5;

    _lsq_scalar_gradient(cs_glob_mesh,
                         cs_glob_mesh_quantities,
                         *imrgra,
                         *iccocg,
                         *imobil,
                         *iale,
                         *nswrgp,
                         *idimtr,
                         *iphydp,
                         *inc,
                         *extrap,
                         isympa,
                         fextx,
                         fexty,
                         fextz,
                         coefap,
                         coefbp,
                         pvar,
                         (cs_real_3_t *)dpdxyz,
                         rhsv);

    _scalar_gradient_clipping(*imrgra, _imlini, *iwarnp, *idimtr, _climin,
                              pvar, (cs_real_3_t *)dpdxyz);

    _iterative_scalar_gradient(cs_glob_mesh,
                               cs_glob_mesh_quantities,
                               *ivar,
                               *imrgra,
                               *iccocg,
                               *imobil,
                               *iale,
                               *nswrgp,
                               *idimtr,
                               *iphydp,
                               *iwarnp,
                               *inc,
                               *epsrgp,
                               *extrap,
                               fextx,
                               fexty,
                               fextz,
                               coefap,
                               coefbp,
                               pvar,
                               (cs_real_3_t *)dpdxyz,
                               rhsv);

  }

  _scalar_gradient_clipping(*imrgra, *imligp, *iwarnp, *idimtr, *climgp,
                            pvar, (cs_real_3_t *)dpdxyz);

  /* Copy gradient to component arrays */

# pragma omp parallel for
  for (ii = 0; ii < n_cells_ext; ii++) {
    grad[ii]                 = dpdxyz[ii*3];
    grad[ii + n_cells_ext]   = dpdxyz[ii*3 + 1];
    grad[ii + n_cells_ext*2] = dpdxyz[ii*3 + 2];
  }

  if (update_stats == true) {
    gradient_info->n_calls += 1;
    t1 = cs_timer_time();
    cs_timer_counter_add_diff(&(gradient_info->t_tot), &t0, &t1);
  }

  BFT_FREE(rhsv);
  BFT_FREE(dpdxyz);
}

/*----------------------------------------------------------------------------
 * Compute cell gradient of vector field.
 *----------------------------------------------------------------------------*/

void CS_PROCF (cgdvec, CGDVEC)
(
 const cs_int_t         *const ivar,
 const cs_int_t         *const imrgra,  /* <-- gradient computation mode      */
 const cs_int_t         *const inc,     /* <-- 0 or 1: increment or not       */
 const cs_int_t         *const nswrgp,  /* <-- >1: with reconstruction        */
 const cs_int_t         *const iwarnp,  /* <-- verbosity level                */
 const cs_int_t         *const imligp,  /* <-- type of clipping               */
 const cs_real_t        *const epsrgp,  /* <-- precision for iterative
                                               gradient calculation           */
 const cs_real_t        *const climgp,  /* <-- clipping coefficient           */
 const cs_real_3_t   *restrict coefav,  /* <-- boundary condition term        */
 const cs_real_33_t  *restrict coefbv,  /* <-- boundary condition term        */
 const cs_real_3_t   *restrict pvar,    /* <-- gradient's base variable       */
       cs_real_33_t  *restrict gradv    /* <-> gradient of the variable       */
)
{
  const cs_mesh_t  *mesh = cs_glob_mesh;
  const cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;

  cs_halo_type_t halo_type = CS_HALO_STANDARD;

  cs_gradient_info_t *gradient_info = NULL;
  cs_timer_t t0, t1;

  bool update_stats = true;
  cs_gradient_type_t gradient_type = CS_GRADIENT_N_TYPES;

  switch (*imrgra) {
  case 0: gradient_type = CS_GRADIENT_ITER; break;
  case 1: gradient_type = CS_GRADIENT_LSQ_STD; break;
  case 2: gradient_type = CS_GRADIENT_LSQ_EXT; break;
  case 3: gradient_type = CS_GRADIENT_LSQ_EXT_RED; break;
  case 4: gradient_type = CS_GRADIENT_LSQ_ITER; break;
  default: break;
  }

  if (update_stats == true) {
    char var_name[32];
    snprintf(var_name, 31, "Var. %2d", *ivar); var_name[31] = '\0';
    t0 = cs_timer_time();
    gradient_info = _find_or_add_system(var_name, gradient_type);
  }

  if (*imrgra == 2 || *imrgra ==  3)
    halo_type = CS_HALO_EXTENDED;

  /* Compute gradient */

  if (*imrgra == 0) {

    _initialize_vector_gradient(mesh,
                                fvq,
                                halo_type,
                                *inc,
                                coefav,
                                coefbv,
                                pvar,
                                gradv);

    /* If reconstructions are required */

    if (*nswrgp > 1)
      _iterative_vector_gradient(mesh,
                                 fvq,
                                 *ivar,
                                 halo_type,
                                 *inc,
                                 *nswrgp,
                                 *iwarnp,
                                 *epsrgp,
                                 coefav,
                                 coefbv,
                                 pvar,
                                 gradv);

  }
  else if (*imrgra == 1 || *imrgra == 2 || *imrgra == 3) {

    /* If NO reconstruction are required */

    if (*nswrgp <= 1)
      _initialize_vector_gradient(mesh,
                                  fvq,
                                  halo_type,
                                  *inc,
                                  coefav,
                                  coefbv,
                                  pvar,
                                  gradv);

    /* Reconstruction with Least square method */

    else
      _lsq_vector_gradient(mesh,
                           fvq,
                           halo_type,
                           *inc,
                           coefav,
                           coefbv,
                           pvar,
                           gradv);

  }
  else if (*imrgra == 4) {

    /* Clipping algorithm and clipping factor */

    const cs_int_t  _imlini = 1;
    const cs_real_t _climin = 1.5;

    /* Initialization by the Least square method */

    _lsq_vector_gradient(mesh,
                         fvq,
                         halo_type,
                         *inc,
                         coefav,
                         coefbv,
                         pvar,
                         gradv);

    _vector_gradient_clipping(mesh,
                              fvq,
                              halo_type,
                              _imlini,
                              *iwarnp,
                              _climin,
                              pvar,
                              gradv);

    _iterative_vector_gradient(mesh,
                               fvq,
                               *ivar,
                               halo_type,
                               *inc,
                               *nswrgp,
                               *iwarnp,
                               *epsrgp,
                               coefav,
                               coefbv,
                               pvar,
                               gradv);

  }

  _vector_gradient_clipping(mesh,
                            fvq,
                            halo_type,
                            *imligp,
                            *iwarnp,
                             *climgp,
                            pvar,
                            gradv);

  if (update_stats == true) {
    gradient_info->n_calls += 1;
    t1 = cs_timer_time();
    cs_timer_counter_add_diff(&(gradient_info->t_tot), &t0, &t1);
  }
}

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Initialize gradient computation API.
 *----------------------------------------------------------------------------*/

void
cs_gradient_initialize(void)
{
  assert(cs_glob_mesh != NULL);
}

/*----------------------------------------------------------------------------
 * Finalize gradient computation API.
 *----------------------------------------------------------------------------*/

void
cs_gradient_finalize(void)
{
  int ii;

  /* Free system info */

  for (ii = 0; ii < cs_glob_gradient_n_systems; ii++) {
    _gradient_info_dump(cs_glob_gradient_systems[ii]);
    _gradient_info_destroy(&(cs_glob_gradient_systems[ii]));
  }

  cs_log_printf(CS_LOG_PERFORMANCE, "\n");
  cs_log_separator(CS_LOG_PERFORMANCE);

  BFT_FREE(cs_glob_gradient_systems);

  cs_glob_gradient_n_systems = 0;
  cs_glob_gradient_n_max_systems = 0;
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
