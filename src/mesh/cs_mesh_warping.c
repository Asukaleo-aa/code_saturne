/*============================================================================
 * Cut warped faces in serial or parallel with/without periodicity.
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

#if defined(HAVE_CONFIG_H)
#include "cs_config.h"
#endif

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_defs.h>
#include <fvm_io_num.h>
#include <fvm_order.h>
#include <fvm_triangulate.h>
#include <fvm_nodal.h>
#include <fvm_writer.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_halo.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"
#include "cs_mesh_quality.h"
#include "cs_mesh_connect.h"
#include "cs_post.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_mesh_warping.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Global variables
 *============================================================================*/

static int cs_glob_mesh_warping_post = 0;
static double cs_glob_mesh_warping_threshold = -1.0;

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create the list of faces to cut in order to respect warping criterion.
 *
 * parameters:
 *   n_faces         <-- number of faces
 *   max_warp_angle  <-- criterion above which face is cut
 *   face_warping    <-- face warping angle
 *   p_n_warp_faces  <-> pointer to the number of warped faces
 *   p_warp_face_lst <-> pointer to the warped face list
 *----------------------------------------------------------------------------*/

static void
_select_warped_faces(cs_int_t        n_faces,
                     double          max_warp_angle,
                     double          face_warping[],
                     cs_int_t        *p_n_warp_faces,
                     cs_int_t        *p_warp_face_lst[])
{
  cs_int_t  face_id;

  cs_int_t  n_warp_faces = 0;
  cs_int_t  *warp_face_lst = NULL;

  if (n_faces > 0) {

    for (face_id = 0; face_id < n_faces; face_id++)
      if (face_warping[face_id] >= max_warp_angle)
        n_warp_faces++;

    BFT_MALLOC(warp_face_lst, n_warp_faces, cs_int_t);

    n_warp_faces = 0;

    for (face_id = 0; face_id < n_faces; face_id++)
      if (face_warping[face_id] >= max_warp_angle)
        warp_face_lst[n_warp_faces++] = face_id + 1;

  }

  *p_n_warp_faces = n_warp_faces;
  *p_warp_face_lst = warp_face_lst;
}

#if defined(HAVE_MPI)

/*----------------------------------------------------------------------------
 * Synchronize flag between parallel and periodic faces (using the max value).
 *
 * This version is used in the parallel case.
 *
 * parameters:
 *   mesh      <-- pointer to a mesh structure
 *   face_ifs  <-- parallel and periodic faces interfaces set
 *   face_flag <-> flag to indicate cutting of each face
 *----------------------------------------------------------------------------*/

static void
_sync_i_face_flag_p(cs_mesh_t                  *mesh,
                    const fvm_interface_set_t  *face_ifs,
                    char                        face_flag[])
{
  int i, j;
  fvm_lnum_t k, l;

  fvm_lnum_t  *send_index = NULL, *recv_index = NULL;
  int  *send_flag = NULL, *recv_flag = NULL;

  const int n_perio = mesh->n_init_perio;
  const int n_interfaces = fvm_interface_set_size(face_ifs);

  /* Note: in the case of periodicity, transform 0 of the interface
     is used for non-periodic sections, and by construction,
     for each periodicity i, transform i*2 + 1 is used for the
     direct periodicity and transform i*2 + 2 for its reverse. */

  BFT_MALLOC(send_index, n_interfaces + 1, fvm_lnum_t);
  BFT_MALLOC(recv_index, n_interfaces + 1, fvm_lnum_t);
  send_index[0] = 0;
  recv_index[0] = 0;

  for (i = 0; i < n_interfaces; i++) {
    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t if_size = fvm_interface_size(face_if);
    send_index[i+1] = send_index[i] + if_size;
    recv_index[i+1] = recv_index[i] + if_size;
  }

  BFT_MALLOC(send_flag, send_index[n_interfaces], int);
  BFT_MALLOC(recv_flag, recv_index[n_interfaces], int);

  /* Prepare send buffer (send reverse transformation values) */

  for (i = 0, j = 0; i < n_interfaces; i++) {

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);

    l = send_index[i];

    if (n_perio != 0) {

      const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

      for (k = tr_index[0]; k < tr_index[1]; k++)
        send_flag[l++] = face_flag[loc_num[k]-1];

      for (j = 0; j < n_perio; j++) {
        for (k = tr_index[j*2+2]; k < tr_index[j*2+3]; k++)
          send_flag[l++] = face_flag[loc_num[k]-1];
        for (k = tr_index[j*2+1]; k < tr_index[j*2+2]; k++)
          send_flag[l++] = face_flag[loc_num[k]-1];
      }
    }
    else { /* if (n_perio != 0) */

      const fvm_lnum_t if_size = fvm_interface_size(face_if);

      for (k = 0; k < if_size; k++)
        send_flag[l++] = face_flag[loc_num[k]-1];
    }
  }

  /* Exchange global face numbers */

  {
    MPI_Request  *request = NULL;
    MPI_Status  *status  = NULL;

    int request_count = 0;

    BFT_MALLOC(request, n_interfaces*2, MPI_Request);
    BFT_MALLOC(status, n_interfaces*2, MPI_Status);

    for (i = 0; i < n_interfaces; i++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
      int distant_rank = fvm_interface_rank(face_if);
      MPI_Irecv(recv_flag + recv_index[i],
                recv_index[i+1] - recv_index[i],
                MPI_INT,
                distant_rank,
                distant_rank,
                cs_glob_mpi_comm,
                &(request[request_count++]));
    }

    for (i = 0; i < n_interfaces; i++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
      int distant_rank = fvm_interface_rank(face_if);
      MPI_Isend(send_flag + send_index[i],
                send_index[i+1] - send_index[i],
                MPI_INT,
                distant_rank,
                (int)cs_glob_rank_id,
                cs_glob_mpi_comm,
                &(request[request_count++]));
    }

    MPI_Waitall(request_count, request, status);

    BFT_FREE(request);
    BFT_FREE(status);
  }

  BFT_FREE(send_flag);
  BFT_FREE(send_index);

  /* Update flag */

  for (i = 0, j = 0; i < n_interfaces; i++) {

    l = recv_index[i];
    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const fvm_lnum_t if_size = fvm_interface_size(face_if);

    for (k = 0; k < if_size; k++) {
      fvm_lnum_t face_id = loc_num[k]-1;
      face_flag[face_id] = CS_MAX(face_flag[face_id], recv_flag[l]);
      l++;
    }
  }

  BFT_FREE(recv_flag);
  BFT_FREE(recv_index);
}

/*----------------------------------------------------------------------------
 * Match face cut info using a faces interface set structure.
 *
 * This version is used for distributed parallel mode.
 *
 * parameters:
 *   mesh             <-- pointer to mesh structure
 *   face_ifs         <-- pointer to face interface structure describing
 *                        periodic face couples
 *   face_flag        <-- flag to indicate cutting of each face
 *   old_to_new       <-- old face -> first new face mapping (0 to n-1)
 *   new_face_vtx_idx <-- new face -> vertices index
 *   new_face_vtx_lst <-> new face -> vertices preliminary connectivity
 *----------------------------------------------------------------------------*/

static void
_match_halo_face_cut_p(const cs_mesh_t             *mesh,
                       const fvm_interface_set_t   *face_ifs,
                       const char                   face_flag[],
                       const fvm_lnum_t             old_to_new[],
                       const fvm_lnum_t             new_face_vtx_idx[],
                       fvm_lnum_t                   new_face_vtx_lst[])
{
  int i, j;
  fvm_lnum_t k, l, t_id, v_id;
  fvm_lnum_t face_id = -1, new_face_id = -1;
  fvm_lnum_t n_face_triangles;
  fvm_lnum_t  *send_count = NULL, *recv_count = NULL;
  fvm_lnum_t  *send_index = NULL, *recv_index = NULL;
  int  *send_buf = NULL, *recv_buf = NULL;

  const int n_perio = mesh->n_init_perio;
  const int n_interfaces = fvm_interface_set_size(face_ifs);

  /* Note: in the case of periodicity, transform 0 of the interface
     is used for non-periodic sections, and by construction,
     for each periodicity i, transform i*2 + 1 is used for the
     direct periodicity and transform i*2 + 2 for its reverse. */

  /* Prepare send counts (non-periodic info is sent from lower
     rank to higher rank, periodic info is sent in direct direction) */

  BFT_MALLOC(send_count, n_interfaces, fvm_lnum_t);
  BFT_MALLOC(recv_count, n_interfaces, fvm_lnum_t);
  for (i = 0; i < n_interfaces; i++) {
    send_count[i] = 0;
    recv_count[i] = 0;
  }

  for (i = 0; i < n_interfaces; i++) {

    fvm_lnum_t tr_0_size;

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const int distant_rank = fvm_interface_rank(face_if);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

    /* Non-periodic faces */

    if (n_perio != 0) {
      assert(tr_index[0] == 0);
      tr_0_size = tr_index[1];
    }
    else
      tr_0_size = fvm_interface_size(face_if);

    l = 0;
    for (k = 0; k < tr_0_size; k++) {
      face_id = loc_num[k]-1;
      if (face_flag[face_id] != 0) {
        l +=  (  mesh->i_face_vtx_idx[face_id+1]
               - mesh->i_face_vtx_idx[face_id]) - 2;
      }
    }
    if (distant_rank > cs_glob_rank_id)
      send_count[i] = l;
    else
      recv_count[i] = l;

    /* Periodic faces */

    for (j = 0; j < n_perio; j++) {

      for (k = tr_index[j*2+1]; k < tr_index[j*2+2]; k++) {
        face_id = loc_num[k]-1;
        if (face_flag[face_id] != 0) {
          send_count[i] +=  (  mesh->i_face_vtx_idx[face_id+1]
                             - mesh->i_face_vtx_idx[face_id]) - 2;
        }
      }
      for (k = tr_index[j*2+2]; k < tr_index[j*2+3]; k++) {
        if (face_flag[face_id] != 0) {
          recv_count[i] +=  (  mesh->i_face_vtx_idx[face_id+1]
                             - mesh->i_face_vtx_idx[face_id]) - 2;
        }
      }
    }
  }

  BFT_MALLOC(send_index, n_interfaces + 1, fvm_lnum_t);
  BFT_MALLOC(recv_index, n_interfaces + 1, fvm_lnum_t);
  send_index[0] = 0;
  recv_index[0] = 0;

  for (i = 0; i < n_interfaces; i++) {
    send_index[i+1] = send_index[i] + send_count[i];
    recv_index[i+1] = recv_index[i] + recv_count[i];
  }

  BFT_FREE(send_count);
  BFT_FREE(recv_count);

  /* Prepare send buffer (new face split info) */

  BFT_MALLOC(send_buf, send_index[n_interfaces], int);
  BFT_MALLOC(recv_buf, recv_index[n_interfaces], int);

  for (i = 0; i < n_interfaces; i++) {

    fvm_lnum_t tr_0_size;

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const int distant_rank = fvm_interface_rank(face_if);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

    l = send_index[i];

    /* Non-periodic faces */

    if (n_perio != 0)
      tr_0_size = tr_index[1];
    else
      tr_0_size = fvm_interface_size(face_if);

    if (distant_rank > cs_glob_rank_id) {
      for (k = 0; k < tr_0_size; k++) {
        face_id = loc_num[k]-1;
        if (face_flag[face_id] != 0) {
          n_face_triangles = (  mesh->i_face_vtx_idx[face_id+1]
                              - mesh->i_face_vtx_idx[face_id]) - 2;
          new_face_id = old_to_new[face_id];
          for (t_id = 0; t_id < n_face_triangles; t_id++) {
            fvm_lnum_t start_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            fvm_lnum_t end_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            for (v_id = start_id; v_id < end_id; v_id++)
              send_buf[l++] = new_face_vtx_lst[v_id];
          }
        }
      }
    }

    /* Periodic faces */

    for (j = 0; j < n_perio; j++) {

      for (k = tr_index[j*2+1]; k < tr_index[j*2+2]; k++) {
        face_id = loc_num[k]-1;
        if (face_flag[face_id] != 0) {
          n_face_triangles = (  mesh->i_face_vtx_idx[face_id+1]
                              - mesh->i_face_vtx_idx[face_id]) - 2;
          new_face_id = old_to_new[face_id];
          for (t_id = 0; t_id < n_face_triangles; t_id++) {
            fvm_lnum_t start_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            fvm_lnum_t end_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            for (v_id = start_id; v_id < end_id; v_id++)
              send_buf[l++] = new_face_vtx_lst[v_id];
          }
        }
      }
    }
  }

  /* Exchange face cut information */

  {
    MPI_Request  *request = NULL;
    MPI_Status  *status  = NULL;

    int request_count = 0;

    BFT_MALLOC(request, n_interfaces*2, MPI_Request);
    BFT_MALLOC(status, n_interfaces*2, MPI_Status);

    for (i = 0; i < n_interfaces; i++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
      int distant_rank = fvm_interface_rank(face_if);
      int n_recv = recv_index[i+1] - recv_index[i];
      if (n_recv > 0)
        MPI_Irecv(recv_buf + recv_index[i],
                  n_recv,
                  MPI_INT,
                  distant_rank,
                  distant_rank,
                  cs_glob_mpi_comm,
                  &(request[request_count++]));
    }

    for (i = 0; i < n_interfaces; i++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
      int distant_rank = fvm_interface_rank(face_if);
      int n_send = send_index[i+1] - send_index[i];
      if (n_send > 0)
        MPI_Isend(send_buf + send_index[i],
                  n_send,
                  MPI_INT,
                  distant_rank,
                  cs_glob_rank_id,
                  cs_glob_mpi_comm,
                  &(request[request_count++]));
    }

    MPI_Waitall(request_count, request, status);

    BFT_FREE(request);
    BFT_FREE(status);
  }

  BFT_FREE(send_buf);
  BFT_FREE(send_index);

  /* Update face cut information */

  for (i = 0; i < n_interfaces; i++) {

    fvm_lnum_t tr_0_size;

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, i);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const int distant_rank = fvm_interface_rank(face_if);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

    l = recv_index[i];

    /* Non-periodic faces */

    if (n_perio != 0)
      tr_0_size = tr_index[1];
    else
      tr_0_size = fvm_interface_size(face_if);

    if (distant_rank <= cs_glob_rank_id) {
      for (k = 0; k < tr_0_size; k++) {
        face_id = loc_num[k]-1;
        if (face_flag[face_id] != 0) {
          n_face_triangles = (  mesh->i_face_vtx_idx[face_id+1]
                              - mesh->i_face_vtx_idx[face_id]) - 2;
          new_face_id = old_to_new[face_id];
          for (t_id = 0; t_id < n_face_triangles; t_id++) {
            fvm_lnum_t start_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            fvm_lnum_t end_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            for (v_id = start_id; v_id < end_id; v_id++)
              new_face_vtx_lst[v_id] = recv_buf[l++];
          }
        }
      }
    }

    /* Periodic faces */

    for (j = 0; j < n_perio; j++) {

      for (k = tr_index[j*2+2]; k < tr_index[j*2+3]; k++) {
        face_id = loc_num[k]-1;
        if (face_flag[face_id] != 0) {
          n_face_triangles = (  mesh->i_face_vtx_idx[face_id+1]
                              - mesh->i_face_vtx_idx[face_id]) - 2;
          new_face_id = old_to_new[face_id];
          for (t_id = 0; t_id < n_face_triangles; t_id++) {
            fvm_lnum_t start_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            fvm_lnum_t end_id = new_face_vtx_idx[new_face_id + t_id] - 1;
            for (v_id = start_id; v_id < end_id; v_id++)
              new_face_vtx_lst[v_id] = recv_buf[l++];
          }
        }
      }
    }
  }

  BFT_FREE(recv_buf);
  BFT_FREE(recv_index);
}

#endif /* defined(HAVE_MPI) */

/*----------------------------------------------------------------------------
 * Synchronize flag between periodic faces (using the max value).
 *
 * This version is used in the serial case.
 *
 * parameters:
 *   mesh      <-- pointer to a mesh structure
 *   face_ifs  <-- parallel and periodic faces interfaces set
 *   face_flag <-> flag to indicate cutting of each face
 *----------------------------------------------------------------------------*/

static void
_sync_i_face_flag_l(cs_mesh_t                  *mesh,
                    const fvm_interface_set_t  *face_ifs,
                    char                        face_flag[])
{
  const int n_perio = mesh->n_init_perio;

  if (n_perio > 0) {

    fvm_lnum_t i;

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, 0);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const fvm_lnum_t *dist_num = fvm_interface_get_distant_num(face_if);
    const fvm_lnum_t if_size = fvm_interface_size(face_if);

    assert(fvm_interface_set_size(face_ifs) == 1);

    for (i = 0; i < if_size; i++) {
      fvm_lnum_t face_id_0 = loc_num[i]-1;
      fvm_lnum_t face_id_1 = dist_num[i]-1;
      if (face_flag[face_id_0] != 0 || face_flag[face_id_1] != 0) {
        face_flag[face_id_0] = 1;
        face_flag[face_id_1] = 1;
      }
    }
  }
}

/*----------------------------------------------------------------------------
 * Match face cut info using a faces interface set structure.
 *
 * This version is used in serial mode.
 *
 * parameters:
 *   mesh             <-- pointer to mesh structure
 *   face_ifs         <-- pointer to face interface structure describing
 *                        periodic face couples
 *   face_flag        <-- flag to indicate cutting of each face
 *   old_to_new       <-- old face -> first new face mapping (0 to n-1)
 *   new_face_vtx_idx <-- new face -> vertices index
 *   new_face_vtx_lst <-> new face -> vertices preliminary connectivity
 *----------------------------------------------------------------------------*/

static void
_match_halo_face_cut_l(const cs_mesh_t             *mesh,
                       const fvm_interface_set_t   *face_ifs,
                       const char                   face_flag[],
                       const fvm_lnum_t             old_to_new[],
                       const fvm_lnum_t             new_face_vtx_idx[],
                       fvm_lnum_t                   new_face_vtx_lst[])
{
  int j;
  fvm_lnum_t k, l, face_id, p_face_id, new_face_id, new_p_face_id;
  fvm_lnum_t t_id, v_id;
  fvm_lnum_t n_face_triangles;

  const int n_perio = mesh->n_init_perio;
  const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, 0);
  const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
  const fvm_lnum_t *dist_num = fvm_interface_get_distant_num(face_if);
  const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

  /* Note: in the case of periodicity, transform 0 of the interface
     is used for non-periodic sections, and by construction,
     for each periodicity i, transform i*2 + 1 is used for the
     direct periodicity and transform i*2 + 2 for its reverse. */

  assert(fvm_interface_set_size(face_ifs) == 1);

  /* Periodic faces */

  for (j = 0; j < n_perio; j++) {

    for (k = tr_index[j*2+1]; k < tr_index[j*2+2]; k++, l++) {
      face_id = loc_num[k]-1;
      p_face_id = dist_num[k]-1;
      if (face_flag[face_id] != 0) {
        assert(face_flag[p_face_id] != 0);
        n_face_triangles = (  mesh->i_face_vtx_idx[face_id+1]
                            - mesh->i_face_vtx_idx[face_id]) - 2;
        new_face_id = old_to_new[face_id];
        new_p_face_id = old_to_new[p_face_id];
        for (t_id = 0; t_id < n_face_triangles; t_id++) {
          fvm_lnum_t start_id = new_face_vtx_idx[new_face_id + t_id] - 1;
          fvm_lnum_t end_id = new_face_vtx_idx[new_face_id + t_id] - 1;
          l = new_face_vtx_idx[new_p_face_id + t_id] - 1;
          for (v_id = start_id; v_id < end_id; v_id++)
            new_face_vtx_lst[l++] = new_face_vtx_lst[v_id];
        }
      }
    }
  }
}

/*----------------------------------------------------------------------------
 * Cut interior faces with parallelism and/or periodicity
 * and update connectivity.
 *
 * parameters:
 *   mesh            <-> pointer to a mesh structure
 *   p_n_cut_faces   <-> in:  number of faces to cut
 *                       out: number of cut faces
 *   p_cut_face_lst  <-> pointer to the cut face list
 *   p_n_sub_elt_lst <-> pointer to the sub-elt count list
 *----------------------------------------------------------------------------*/

static void
_cut_warped_i_faces_halo(cs_mesh_t    *mesh,
                         fvm_lnum_t   *p_n_cut_faces,
                         fvm_lnum_t   *p_cut_face_lst[],
                         fvm_lnum_t   *p_n_sub_elt_lst[])
{
  fvm_lnum_t  i, j, face_id, idx_start, idx_end, old_face_idx;
  fvm_lnum_t  n_triangles, face_shift;

  fvm_lnum_t  n_face_vertices = 0, n_max_face_vertices = 0;
  fvm_lnum_t  n_new_faces = 0, n_cut_faces = 0, connect_size = 0;

  fvm_triangulate_state_t  *triangle_state = NULL;
  fvm_lnum_t  *new_face_vtx_idx = NULL, *new_face_vtx_lst = NULL;
  fvm_lnum_t  *new_face_cells = NULL, *new_face_family = NULL;
  fvm_lnum_t  *cut_face_lst = NULL;
  fvm_lnum_t  *new_face_shift = NULL;
  fvm_lnum_t  *n_sub_elt_lst = NULL;
  int         *perio_num = NULL;
  fvm_lnum_t  *n_perio_faces = NULL;
  fvm_gnum_t  **perio_faces = NULL;

  char *cut_flag = NULL;
  fvm_interface_set_t *face_ifs = NULL;

  const int  dim = mesh->dim;
  const fvm_lnum_t  n_init_faces = mesh->n_i_faces;

  assert(dim == 3);

  /* Build face interfaces interface */

  if (mesh->n_init_perio > 0) {
    BFT_MALLOC(perio_num, mesh->n_init_perio, int);
    for (i = 0; i < mesh->n_init_perio; i++)
      perio_num[i] = i+1;

    cs_mesh_get_perio_faces(mesh, &n_perio_faces, &perio_faces);
  }

  face_ifs
    = fvm_interface_set_create(mesh->n_i_faces,
                               NULL,
                               mesh->global_i_face_num,
                               mesh->periodicity,
                               mesh->n_init_perio,
                               perio_num,
                               n_perio_faces,
                               (const fvm_gnum_t **const)perio_faces);

  if (mesh->n_init_perio > 0) {
    for (i = 0; i < mesh->n_init_perio; i++)
      BFT_FREE(perio_faces[i]);
    BFT_FREE(perio_faces);
    BFT_FREE(n_perio_faces);
    BFT_FREE(perio_num);
  }

  BFT_MALLOC(n_sub_elt_lst, n_init_faces, fvm_lnum_t);
  BFT_MALLOC(new_face_shift, n_init_faces, fvm_lnum_t);

  /* Build flag for each face from list of faces to cut */

  BFT_MALLOC(cut_flag, mesh->n_i_faces, char);

  for (face_id = 0; face_id < n_init_faces; face_id++)
    cut_flag[face_id] = 0;

  for (i = 0; i < *p_n_cut_faces; i++)
    cut_flag[(*p_cut_face_lst)[i] - 1] = 1;

  BFT_FREE(*p_cut_face_lst);

  /* Synchronize face warping flag as a precaution against different
     truncation errors on matching faces */

#if defined(HAVE_MPI)
  if (cs_glob_n_ranks > 1)
    _sync_i_face_flag_p(mesh, face_ifs, cut_flag);
#endif

  if (cs_glob_n_ranks == 1)
    _sync_i_face_flag_l(mesh, face_ifs, cut_flag);

  /* First loop: compute sizes */

  for (face_id = 0; face_id < n_init_faces; face_id++) {

    idx_start = mesh->i_face_vtx_idx[face_id] - 1;
    idx_end = mesh->i_face_vtx_idx[face_id + 1] - 1;

    n_face_vertices = idx_end - idx_start;
    n_max_face_vertices = CS_MAX(n_max_face_vertices, n_face_vertices);

    new_face_shift[face_id] = n_new_faces;

    if (cut_flag[face_id] != 0) {
      n_triangles = n_face_vertices - 2;
      connect_size += n_triangles*3;
      n_new_faces += n_triangles;
      n_cut_faces += n_triangles;
      n_sub_elt_lst[face_id] = n_triangles;
    }
    else {
      connect_size += n_face_vertices;
      n_new_faces += 1;
      n_sub_elt_lst[face_id] = 1;
    }

  } /* End of loop on faces */

  *p_n_sub_elt_lst = n_sub_elt_lst;

  /* If no faces are cut, we are sure at this stage that we do
     need to communicate face cutting info on the interfaces for
     this rank, and do not otherwise call collective MPI operations,
     so we may return immediately. */

  if (n_cut_faces == 0) {
    BFT_FREE(cut_flag);
    BFT_FREE(new_face_shift);
    face_ifs = fvm_interface_set_destroy(face_ifs);
    return;
  }

  BFT_MALLOC(new_face_vtx_idx, n_new_faces + 1, fvm_lnum_t);
  BFT_MALLOC(new_face_vtx_lst, connect_size, fvm_lnum_t);
  BFT_MALLOC(new_face_cells, 2*n_new_faces, fvm_lnum_t);
  BFT_MALLOC(new_face_family, n_new_faces, fvm_lnum_t);

  BFT_MALLOC(cut_face_lst, n_cut_faces, fvm_lnum_t);

  triangle_state = fvm_triangulate_state_create(n_max_face_vertices);

  /* Define the new connectivity after triangulation;
     for cut faces, the new connectivity is defined relative to
     the local vertex positions in the parent faces, not to the true
     vertex numbers, so as to be synchronizable across interfaces. */

  new_face_vtx_idx[0] = 1;
  connect_size = 0;
  n_new_faces = 0;
  n_cut_faces = 0;

  for (face_id = 0; face_id < n_init_faces; face_id++) {

    idx_start = mesh->i_face_vtx_idx[face_id] - 1;
    idx_end = mesh->i_face_vtx_idx[face_id + 1] - 1;
    n_face_vertices = idx_end - idx_start;

    if (cut_flag[face_id] != 0) {

      n_triangles = fvm_triangulate_polygon(dim,
                                            n_face_vertices,
                                            mesh->vtx_coord,
                                            NULL,
                                            mesh->i_face_vtx_lst + idx_start,
                                            FVM_TRIANGULATE_ELT_DEF,
                                            new_face_vtx_lst + connect_size,
                                            triangle_state);

      assert(n_triangles == n_face_vertices - 2);

      /* Update face -> vertex connectivity */

      for (i = 0; i < n_triangles; i++) {

        cut_face_lst[n_cut_faces++] = n_new_faces + 1;

        /* Update "face -> cells" connectivity */

        for (j = 0; j < 2; j++)
          new_face_cells[2*n_new_faces + j]
            = mesh->i_face_cells[2*face_id + j];

        /* Update family for each face */

        new_face_family[n_new_faces] = mesh->i_face_family[face_id];

        /* Update "face -> vertices" connectivity index
           (list has alread been defined by fvm_triangulate_polygon) */

        n_new_faces++;
        connect_size += 3;
        new_face_vtx_idx[n_new_faces]
          = new_face_vtx_idx[n_new_faces-1] + 3;

      } /* End of loop on triangles */

    }
    else {

      /* Update "face -> cells" connectivity */

      for (j = 0; j < 2; j++)
        new_face_cells[2*n_new_faces + j] = mesh->i_face_cells[2*face_id + j];

      /* Update family for each face */

      new_face_family[n_new_faces] = mesh->i_face_family[face_id];

      /* Update "face -> vertices" connectivity */

      for (j = 0, i = idx_start; i < idx_end; i++, j++)
        new_face_vtx_lst[connect_size + j] = mesh->i_face_vtx_lst[i];

      n_new_faces++;
      connect_size += n_face_vertices;
      new_face_vtx_idx[n_new_faces]
        = new_face_vtx_idx[n_new_faces-1] + n_face_vertices;

    }

  } /* End of loop on internal faces */

  triangle_state = fvm_triangulate_state_destroy(triangle_state);

  /* Partial mesh update */

  BFT_FREE(mesh->i_face_cells);
  BFT_FREE(mesh->i_face_family);

  mesh->i_face_cells = new_face_cells;
  mesh->i_face_family = new_face_family;

  new_face_cells = NULL;
  new_face_family = NULL;

  /* Now enforce match of local subdivision for parallel and periodic faces */

#if defined(HAVE_MPI)
  if (cs_glob_n_ranks > 1)
    _match_halo_face_cut_p(mesh,
                           face_ifs,
                           cut_flag,
                           new_face_shift,
                           new_face_vtx_idx,
                           new_face_vtx_lst);
#endif

  if (cs_glob_n_ranks == 1)
    _match_halo_face_cut_l(mesh,
                           face_ifs,
                           cut_flag,
                           new_face_shift,
                           new_face_vtx_idx,
                           new_face_vtx_lst);

  face_ifs = fvm_interface_set_destroy(face_ifs);

  /* Final connectivity update: switch from parent face local to
     full mesh vertex numbering for subdivided faces. */

  /* Get mesh numbering from element numbering */

  for (face_id = 0; face_id < n_init_faces; face_id++) {

    if (cut_flag[face_id] != 0) {

      idx_start = mesh->i_face_vtx_idx[face_id] - 1;
      idx_end = mesh->i_face_vtx_idx[face_id + 1] - 1;

      n_face_vertices = idx_end - idx_start;
      n_triangles = n_face_vertices - 2;

      face_shift = new_face_shift[face_id];

      old_face_idx = mesh->i_face_vtx_idx[face_id] - 1;

      for (i = 0; i < n_triangles; i++) {

        for (j = new_face_vtx_idx[face_shift] - 1;
             j < new_face_vtx_idx[face_shift+1] - 1; j++) {

          fvm_lnum_t v_id = new_face_vtx_lst[j] - 1;
          new_face_vtx_lst[j] = mesh->i_face_vtx_lst[old_face_idx + v_id];
        }

        face_shift++;

      } /* End of loop on triangles */

    } /* If the face is cut */

  } /* End of loop on faces */

  BFT_FREE(cut_flag);
  BFT_FREE(new_face_shift);
  BFT_FREE(mesh->i_face_vtx_idx);
  BFT_FREE(mesh->i_face_vtx_lst);

  /* Update mesh and define returned pointers */

  mesh->i_face_vtx_idx = new_face_vtx_idx;
  mesh->i_face_vtx_lst = new_face_vtx_lst;
  mesh->i_face_vtx_connect_size = connect_size;
  mesh->n_i_faces = n_new_faces;

  *p_n_cut_faces = n_cut_faces;
  *p_cut_face_lst = cut_face_lst;
}

/*----------------------------------------------------------------------------
 * Cut faces if necessary and update connectivity without periodicity
 *
 * parameters:
 *   mesh                    <-> pointer to a mesh structure
 *   face_type               <-- internal or border faces
 *   p_n_cut_faces           <-> in:  number of faces to cut
 *                               out: number of cut faces
 *   p_cut_face_lst          <-> pointer to the cut face list
 *   p_n_sub_elt_lst         <-> pointer to the sub-elt count list
 *   p_n_faces               <-> pointer to the number of faces
 *   p_face_num              <-> pointer to the global face numbers
 *   p_face_vtx_connect_size <-> size of the "face -> vertex" connectivity
 *   p_face_cells            <-> "face -> cells" connectivity
 *   p_face_vtx_idx          <-> pointer on "face -> vertices" connect. index
 *   p_face_vtx_lst          <-> pointer on "face -> vertices" connect. list
 *----------------------------------------------------------------------------*/

static void
_cut_warped_faces(cs_mesh_t       *mesh,
                  int              stride,
                  fvm_lnum_t      *p_n_cut_faces,
                  fvm_lnum_t      *p_cut_face_lst[],
                  fvm_lnum_t      *p_n_sub_elt_lst[],
                  fvm_lnum_t      *p_n_faces,
                  fvm_lnum_t      *p_face_vtx_connect_size,
                  fvm_lnum_t      *p_face_cells[],
                  fvm_lnum_t      *p_face_family[],
                  fvm_lnum_t      *p_face_vtx_idx[],
                  fvm_lnum_t      *p_face_vtx_lst[])
{
  fvm_lnum_t  i, j, face_id, idx_start, idx_end, shift;
  fvm_lnum_t  n_triangles;

  fvm_lnum_t  n_face_vertices = 0, n_max_face_vertices = 0;
  fvm_lnum_t  n_new_faces = 0, n_cut_faces = 0, connect_size = 0;

  fvm_triangulate_state_t  *triangle_state = NULL;
  fvm_lnum_t  *new_face_vtx_idx = NULL, *new_face_vtx_lst = NULL;
  fvm_lnum_t  *new_face_cells = NULL, *new_face_family = NULL;
  fvm_lnum_t  *cut_face_lst = NULL;
  fvm_lnum_t  *n_sub_elt_lst = NULL;
  char *cut_flag = NULL;

  const fvm_lnum_t  dim = mesh->dim;
  const fvm_lnum_t  n_init_faces = *p_n_faces;

  assert(stride == 1 || stride ==2);
  assert(dim == 3);

  BFT_MALLOC(n_sub_elt_lst, n_init_faces, fvm_lnum_t);

  /* Build flag for each face from list of faces to cut */

  BFT_MALLOC(cut_flag, n_init_faces, char);

  for (face_id = 0; face_id < n_init_faces; face_id++)
    cut_flag[face_id] = 0;

  for (i = 0; i < *p_n_cut_faces; i++)
    cut_flag[(*p_cut_face_lst)[i] - 1] = 1;

  BFT_FREE(*p_cut_face_lst);

  /* First loop: count */

  for (face_id = 0; face_id < n_init_faces; face_id++) {

    idx_start = (*p_face_vtx_idx)[face_id] - 1;
    idx_end = (*p_face_vtx_idx)[face_id + 1] - 1;

    n_face_vertices = idx_end - idx_start;
    n_max_face_vertices = CS_MAX(n_max_face_vertices, n_face_vertices);

    if (cut_flag[face_id] != 0) {

      n_triangles = n_face_vertices - 2;
      connect_size += n_triangles*3;
      n_new_faces += n_triangles;
      n_cut_faces += n_triangles;
      n_sub_elt_lst[face_id] = n_triangles;

    }
    else {

      connect_size += n_face_vertices;
      n_new_faces += 1;
      n_sub_elt_lst[face_id] = 1;

    }

  } /* End of loop on faces */

  *p_n_sub_elt_lst = n_sub_elt_lst;

  if (n_cut_faces == 0)
    return;

  BFT_MALLOC(new_face_vtx_idx, n_new_faces + 1, fvm_lnum_t);
  BFT_MALLOC(new_face_vtx_lst, connect_size, fvm_lnum_t);
  BFT_MALLOC(new_face_cells, n_new_faces*stride, fvm_lnum_t);
  BFT_MALLOC(new_face_family, n_new_faces, fvm_lnum_t);

  BFT_MALLOC(cut_face_lst, n_cut_faces, fvm_lnum_t);

  triangle_state = fvm_triangulate_state_create(n_max_face_vertices);

  /* Second loop : define the new connectivity after triangulation */

  new_face_vtx_idx[0] = 1;
  connect_size = 0;
  n_new_faces = 0;
  n_cut_faces = 0;

  for (face_id = 0; face_id < n_init_faces; face_id++) {

    idx_start = (*p_face_vtx_idx)[face_id] - 1;
    idx_end = (*p_face_vtx_idx)[face_id + 1] - 1;
    n_face_vertices = idx_end - idx_start;

    if (cut_flag[face_id] != 0) {

      n_triangles = fvm_triangulate_polygon(dim,
                                            n_face_vertices,
                                            mesh->vtx_coord,
                                            NULL,
                                            (*p_face_vtx_lst) + idx_start,
                                            FVM_TRIANGULATE_MESH_DEF,
                                            new_face_vtx_lst + connect_size,
                                            triangle_state);

      assert(n_triangles == n_face_vertices - 2);

      /* Update face -> vertex connectivity */

      shift = 0;

      for (i = 0; i < n_triangles; i++) {

        cut_face_lst[n_cut_faces++] = n_new_faces + 1;

        /* Update "face -> cells" connectivity */

        for (j = 0; j < stride; j++)
          new_face_cells[stride*n_new_faces + j]
            = (*p_face_cells)[stride*face_id + j];

        /* Update family for each face */

        new_face_family[n_new_faces] = (*p_face_family)[face_id];

        /* Update "face -> vertices" connectivity index
           (list has alread been defined by fvm_triangulate_polygon) */

        n_new_faces++;
        connect_size += 3;
        new_face_vtx_idx[n_new_faces] = new_face_vtx_idx[n_new_faces-1] + 3;

      } /* End of loop on triangles */

    }
    else {

      /* Update "face -> cells" connectivity */

      for (j = 0; j < stride; j++)
        new_face_cells[stride*n_new_faces + j] =
          (*p_face_cells)[stride*face_id + j];

      /* Update family for each faces */

      new_face_family[n_new_faces] = (*p_face_family)[face_id];

      /* Update "face -> vertices" connectivity */

      for (j = 0, i = idx_start; i < idx_end; i++, j++)
        new_face_vtx_lst[connect_size + j] = (*p_face_vtx_lst)[i];

      n_new_faces++;
      connect_size += n_face_vertices;
      new_face_vtx_idx[n_new_faces] =
        new_face_vtx_idx[n_new_faces-1] + n_face_vertices;

    }

  } /* End of loop on internal faces */

  triangle_state = fvm_triangulate_state_destroy(triangle_state);

  BFT_FREE(*p_face_vtx_idx);
  BFT_FREE(*p_face_vtx_lst);
  BFT_FREE(*p_face_cells);
  BFT_FREE(*p_face_family);

  /* Define returned pointers */

  *p_face_vtx_idx = new_face_vtx_idx;
  *p_face_vtx_lst = new_face_vtx_lst;
  *p_face_cells = new_face_cells;
  *p_face_family = new_face_family;
  *p_face_vtx_connect_size = connect_size;
  *p_n_faces = n_new_faces;
  *p_n_cut_faces = n_cut_faces;

  *p_cut_face_lst = cut_face_lst;
}

/*----------------------------------------------------------------------------
 * Update warped faces global numbers after cutting
 *
 * parameters:
 *   mesh              <-> pointer to a mesh structure
 *   n_faces           <-- number of faces
 *   n_init_faces      <-- initial number of faces
 *   n_cut_faces       <-- number of cut faces
 *   cut_face_lst      <-- pointer to the cut face list
 *   n_sub_elt_lst     <-- sub-elt count list
 *   n_g_faces         <-> global number of faces
 *   p_global_face_num <-> pointer to the global face numbers
 *----------------------------------------------------------------------------*/

static void
_update_cut_faces_num(cs_mesh_t       *mesh,
                      cs_int_t         n_faces,
                      cs_int_t         n_init_faces,
                      fvm_lnum_t       n_sub_elt_lst[],
                      fvm_gnum_t      *n_g_faces,
                      fvm_gnum_t     **p_global_face_num)
{
  size_t  size;

  fvm_io_num_t  *new_io_num = NULL, *previous_io_num = NULL;
  const fvm_gnum_t  *global_num = NULL;

  /* Simply update global number of faces in trivial case */

  *n_g_faces = n_faces;

  if (*p_global_face_num == NULL)
    return;

  /* Faces should not have been reordered */

  if (fvm_order_local_test(NULL, *p_global_face_num, n_init_faces) == false)
    bft_error(__FILE__, __LINE__, 0,
              _("The faces have been renumbered before cutting.\n"
                "This case should not arise, because the mesh entities\n"
                "should be cut before renumbering."));

  /* Update global number of internal faces and its global numbering */

  if (mesh->n_domains > 1) {

    previous_io_num = fvm_io_num_create(NULL,
                                        *p_global_face_num,
                                        n_init_faces,
                                        0);
    new_io_num = fvm_io_num_create_from_sub(previous_io_num,
                                            n_sub_elt_lst);

    previous_io_num = fvm_io_num_destroy(previous_io_num);

    *n_g_faces = fvm_io_num_get_global_count(new_io_num);

    global_num = fvm_io_num_get_global_num(new_io_num);

    BFT_REALLOC(*p_global_face_num, n_faces, fvm_gnum_t);
    size = sizeof(fvm_gnum_t) * n_faces;
    memcpy(*p_global_face_num, global_num, size);

    new_io_num = fvm_io_num_destroy(new_io_num);

  }
}

/*----------------------------------------------------------------------------
 * Post-process the warped faces before cutting.
 *
 * parameters:
 *   n_i_warp_faces  <-- number of internal warped faces
 *   n_b_warp_faces  <-- number of border warped faces
 *   i_warp_face_lst <-- internal warped face list
 *   b_warp_face_lst <-- border warped face list
 *   i_face_warping  <-- face warping angle for internal faces
 *   b_face_warping  <-- face warping angle for internal faces
 *----------------------------------------------------------------------------*/

static void
_post_before_cutting(cs_int_t        n_i_warp_faces,
                     cs_int_t        n_b_warp_faces,
                     cs_int_t        i_warp_face_lst[],
                     cs_int_t        b_warp_face_lst[],
                     double          i_face_warping[],
                     double          b_face_warping[])
{
  fvm_lnum_t  parent_num_shift[2];

  int  n_parent_lists = 2;
  fvm_nodal_t  *fvm_mesh = NULL;
  fvm_writer_t  *writer = NULL;

  const cs_int_t  writer_id = -1; /* default writer */
  const void  *var_ptr[2] = {NULL, NULL};

  parent_num_shift[0] = 0;
  parent_num_shift[1] = cs_glob_mesh->n_b_faces;

  if (cs_post_writer_exists(writer_id) == false)
    return;

  assert(sizeof(double) == sizeof(cs_real_t));

  fvm_mesh = cs_mesh_connect_faces_to_nodal(cs_glob_mesh,
                                            _("Warped faces to cut"),
                                            false,
                                            n_i_warp_faces,
                                            n_b_warp_faces,
                                            i_warp_face_lst,
                                            b_warp_face_lst);

  writer = cs_post_get_writer(writer_id);

  fvm_writer_set_mesh_time(writer, -1, 0.0);

  /* Write a mesh from the selected faces */

  fvm_writer_export_nodal(writer, fvm_mesh);

  /* Write the warping field */

  var_ptr[0] = ((const char *)b_face_warping);
  var_ptr[1] = ((const char *)i_face_warping);

  fvm_writer_export_field(writer,
                          fvm_mesh,
                          _("Face warping"),
                          FVM_WRITER_PER_ELEMENT,
                          1,
                          FVM_INTERLACE,
                          n_parent_lists,
                          parent_num_shift,
                          FVM_DOUBLE,
                          (int)-1,
                          (double)0.0,
                          (const void **)var_ptr);

  fvm_mesh = fvm_nodal_destroy(fvm_mesh);
}

/*----------------------------------------------------------------------------
 * Post-process the warped faces after cutting.
 *
 * parameters:
 *   n_i_cut_faces  <-- number of internal faces generated by cutting
 *   n_b_cut_faces  <-- number of border faces generated by cutting
 *   i_cut_face_lst <-- face warping angle for internal faces
 *   b_cut_face_lst <-- face warping angle for internal faces
 *----------------------------------------------------------------------------*/

static void
_post_after_cutting(cs_int_t       n_i_cut_faces,
                    cs_int_t       n_b_cut_faces,
                    cs_int_t       i_cut_face_lst[],
                    cs_int_t       b_cut_face_lst[])
{
  fvm_nodal_t  *fvm_mesh = NULL;
  fvm_writer_t  *writer = NULL;

  const cs_int_t  writer_id = -1; /* default writer */

  if (cs_post_writer_exists(writer_id) == false)
    return;

  fvm_mesh = cs_mesh_connect_faces_to_nodal(cs_glob_mesh,
                                            _("Warped faces after cutting"),
                                            false,
                                            n_i_cut_faces,
                                            n_b_cut_faces,
                                            i_cut_face_lst,
                                            b_cut_face_lst);

  writer = cs_post_get_writer(writer_id);

  fvm_writer_set_mesh_time(writer, -1, 0.0);

  /* Write a mesh from the selected faces */

  fvm_writer_export_nodal(writer, fvm_mesh);

  fvm_mesh = fvm_nodal_destroy(fvm_mesh);
}

/*============================================================================
 * Public function definitions for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Set the threshold to cut warped faces.
 *
 * Fortran interface :
 *
 * subroutine setcwf (cwfthr)
 * *****************
 *
 * integer          cwfpst      : <-> : if 1, activate postprocessing when
 *                                      cutting warped faces (default 0)
 * double precision cwfthr      : <-> : threshold angle (in degrees) if
 *                                      positive, do not cut warped faces
 *                                      if negative (default -1)
 *----------------------------------------------------------------------------*/

void
CS_PROCF (setcwf, SETCWF) (const cs_int_t   *cwfpst,
                           const cs_real_t  *cwfthr)
{
  cs_mesh_warping_set_defaults((double)(*cwfthr),
                               (int)(*cwfpst));
}

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Cut warped faces.
 *
 * Update border face connectivity and associated mesh quantities.
 *
 * parameters:
 *   mesh           <-> pointer to mesh structure.
 *   max_warp_angle <-- criterion to know which face to cut
 *   post_flag      <-- 1 if we have to post-process cut faces, 0 otherwise
 *----------------------------------------------------------------------------*/

void
cs_mesh_warping_cut_faces(cs_mesh_t    *mesh,
                          double        max_warp_angle,
                          cs_bool_t     post_flag)
{
  cs_int_t  i;

  cs_int_t  n_i_cut_faces = 0, n_b_cut_faces = 0;
  cs_int_t  *i_face_lst = NULL, *b_face_lst = NULL;
  cs_real_t  *i_face_normal = NULL, *b_face_normal = NULL;
  double  *working_array = NULL, *i_face_warping = NULL, *b_face_warping = NULL;
  fvm_lnum_t  *n_i_sub_elt_lst = NULL, *n_b_sub_elt_lst = NULL;
  fvm_gnum_t  n_g_faces_ini = 0;
  fvm_gnum_t  n_g_i_cut_faces = 0, n_g_b_cut_faces = 0;

  const cs_int_t  n_init_i_faces = mesh->n_i_faces;
  const cs_int_t  n_init_b_faces = mesh->n_b_faces;

#if 0   /* DEBUG */
  cs_mesh_dump(mesh);
#endif

  bft_printf(_("\n\n Cutting of warped faces requested\n"
               " ---------------------------------\n\n"
               " Maximum allowed angle (deg): %7.4f\n\n"), max_warp_angle);

  /* Compute face warping */

  BFT_MALLOC(working_array, n_init_i_faces + n_init_b_faces, double);

  for (i = 0; i < n_init_i_faces + n_init_b_faces; i++)
    working_array[i] = 0.;

  i_face_warping = working_array;
  b_face_warping = working_array + n_init_i_faces;

  cs_mesh_quantities_face_normal(mesh,
                                 &i_face_normal,
                                 &b_face_normal);

  cs_mesh_quality_compute_warping(mesh,
                                  i_face_normal,
                                  b_face_normal,
                                  i_face_warping,
                                  b_face_warping);

  BFT_FREE(i_face_normal);
  BFT_FREE(b_face_normal);

  _select_warped_faces(n_init_i_faces,
                       max_warp_angle,
                       i_face_warping,
                       &n_i_cut_faces,
                       &i_face_lst);

  _select_warped_faces(n_init_b_faces,
                       max_warp_angle,
                       b_face_warping,
                       &n_b_cut_faces,
                       &b_face_lst);

  /* Define the global number of faces which need to be cut */

  if (mesh->n_domains > 1) {
#if defined(HAVE_MPI)
    MPI_Allreduce(&n_i_cut_faces, &n_g_i_cut_faces, 1, CS_MPI_INT,
                  MPI_SUM, cs_glob_mpi_comm);

    MPI_Allreduce(&n_b_cut_faces, &n_g_b_cut_faces, 1, CS_MPI_INT,
                  MPI_SUM, cs_glob_mpi_comm);
#endif
  }
  else {
    n_g_i_cut_faces = n_i_cut_faces;
    n_g_b_cut_faces = n_b_cut_faces;
  }

  /* Test if there are faces to cut to continue */

  if (n_g_i_cut_faces == 0 && n_g_b_cut_faces == 0) {

    BFT_FREE(i_face_lst);
    BFT_FREE(b_face_lst);
    BFT_FREE(working_array);

    bft_printf(_("\n No face to cut. Verify the criterion if necessary.\n"));
    return;
  }

  /* Post-processing management */

  if (post_flag == true)
    _post_before_cutting(n_i_cut_faces,
                         n_b_cut_faces,
                         i_face_lst,
                         b_face_lst,
                         i_face_warping,
                         b_face_warping);

  BFT_FREE(working_array);

  /* Internal face treatment */
  /* ----------------------- */

  n_g_faces_ini = mesh->n_g_b_faces;

  if (mesh->halo == NULL)
    _cut_warped_faces(mesh,
                      2,
                      &n_i_cut_faces,
                      &i_face_lst,
                      &n_i_sub_elt_lst,
                      &mesh->n_i_faces,
                      &mesh->i_face_vtx_connect_size,
                      &mesh->i_face_cells,
                      &mesh->i_face_family,
                      &mesh->i_face_vtx_idx,
                      &mesh->i_face_vtx_lst);

  else
    _cut_warped_i_faces_halo(mesh,
                             &n_i_cut_faces,
                             &i_face_lst,
                             &n_i_sub_elt_lst);

  /* Update global number of internal faces and its global numbering */

  _update_cut_faces_num(mesh,
                        mesh->n_i_faces,
                        n_init_i_faces,
                        n_i_sub_elt_lst,
                        &(mesh->n_g_i_faces),
                        &(mesh->global_i_face_num));

  bft_printf(_(" Interior faces:\n\n"
               "   %12llu faces before cutting\n"
               "   %12llu faces after cutting\n\n"),
             (unsigned long long)n_g_faces_ini,
             (unsigned long long)(mesh->n_g_i_faces));

  /* Partial memory free */

  BFT_FREE(n_i_sub_elt_lst);

  /* Border face treatment */
  /* --------------------- */

  n_g_faces_ini = mesh->n_g_b_faces;

  _cut_warped_faces(mesh,
                    1,
                    &n_b_cut_faces,
                    &b_face_lst,
                    &n_b_sub_elt_lst,
                    &mesh->n_b_faces,
                    &mesh->b_face_vtx_connect_size,
                    &mesh->b_face_cells,
                    &mesh->b_face_family,
                    &mesh->b_face_vtx_idx,
                    &mesh->b_face_vtx_lst);

  /* Update global number of border faces and its global numbering */

  _update_cut_faces_num(mesh,
                        mesh->n_b_faces,
                        n_init_b_faces,
                        n_b_sub_elt_lst,
                        &(mesh->n_g_b_faces),
                        &(mesh->global_b_face_num));

  bft_printf(_(" Boundary faces:\n\n"
               "   %12llu faces before cutting\n"
               "   %12llu faces after cutting\n\n"),
             (unsigned long long)n_g_faces_ini,
             (unsigned long long)(mesh->n_g_b_faces));

  /* Partial memory free */

  BFT_FREE(n_b_sub_elt_lst);

  /* post processing of the selected faces */

  if (post_flag == true)
    _post_after_cutting(n_i_cut_faces,
                        n_b_cut_faces,
                        i_face_lst,
                        b_face_lst);

  /* Free memory */

  BFT_FREE(i_face_lst);
  BFT_FREE(b_face_lst);

  /* Set mesh modification flag */

  mesh->modified = 1;
}

/*----------------------------------------------------------------------------
 * Set defaults for cutting of warped faces.
 *
 * parameters:
 *   max_warp_angle <-- maximum warp angle (in degrees) over which faces will
 *                      be cut; negative (-1) if faces should not be cut
 *   postprocess    <-- 1 if postprocessing should be activated when cutting
 *                      warped faces, 0 otherwise
 *----------------------------------------------------------------------------*/

void
cs_mesh_warping_set_defaults(double  max_warp_angle,
                             int     postprocess)
{
  if (max_warp_angle >= 0.0 && max_warp_angle <= 180.0)
    cs_glob_mesh_warping_threshold = max_warp_angle;
  else
    cs_glob_mesh_warping_threshold = -1.0;

  if (postprocess != 0)
    cs_glob_mesh_warping_post = 1;
}

/*----------------------------------------------------------------------------
 * Get defaults for cutting of warped faces.
 *
 * parameters:
 *   max_warp_angle --> if non NULL, returns maximum warp angle (in degrees)
 *                      over which faces will be cut, or -1 if faces should
 *                      not be cut
 *   postprocess    --> if non NULL, returns 1 if postprocessing should be
 *                      activated when cutting warped faces, 0 otherwise
 *----------------------------------------------------------------------------*/

void
cs_mesh_warping_get_defaults(double  *max_warp_angle,
                             int     *postprocess)
{
  if (max_warp_angle != NULL)
    *max_warp_angle = cs_glob_mesh_warping_threshold;

  if (postprocess != NULL)
    *postprocess = cs_glob_mesh_warping_post;
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
