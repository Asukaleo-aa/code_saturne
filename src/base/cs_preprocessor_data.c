/*============================================================================
 *
 *     This file is part of the Code_Saturne Kernel, element of the
 *     Code_Saturne CFD tool.
 *
 *     Copyright (C) 1998-2009 EDF S.A., France
 *
 *     contact: saturne-support@edf.fr
 *
 *     The Code_Saturne Kernel is free software; you can redistribute it
 *     and/or modify it under the terms of the GNU General Public License
 *     as published by the Free Software Foundation; either version 2 of
 *     the License, or (at your option) any later version.
 *
 *     The Code_Saturne Kernel is distributed in the hope that it will be
 *     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with the Code_Saturne Kernel; if not, write to the
 *     Free Software Foundation, Inc.,
 *     51 Franklin St, Fifth Floor,
 *     Boston, MA  02110-1301  USA
 *
 *============================================================================*/

/*============================================================================
 * Manage the exchange of data between Code_Saturne and the pre-processor
 *============================================================================*/


#if defined(HAVE_CONFIG_H)
#include "cs_config.h"
#endif

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(HAVE_MPI)
#include <mpi.h>
#endif

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_error.h>
#include <bft_file.h>
#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_periodicity.h>

#include <fvm_block_to_part.h>
#include <fvm_io_num.h>
#include <fvm_interface.h>
#include <fvm_order.h>
#include <fvm_parall.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_mesh.h"
#include "cs_io.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_preprocessor_data.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Type Definitions
 *============================================================================*/

/* Structure used for building mesh structure */
/* ------------------------------------------ */

typedef struct {

  /* Face-related dimensions */

  fvm_gnum_t  n_g_faces;
  fvm_gnum_t  n_g_face_connect_size;

  /* Temporary mesh data */

  int           read_cell_rank;
  int          *cell_rank;

  fvm_gnum_t   *face_cells;
  fvm_lnum_t   *face_vertices_idx;
  fvm_gnum_t   *face_vertices;
  cs_int_t     *cell_gc_id;
  cs_int_t     *face_gc_id;
  cs_real_t    *vertex_coords;

  /* Periodic features */

  int           n_perio;               /* Number of periodicities */
  int          *periodicity_num;       /* Periodicity numbers */
  fvm_lnum_t   *n_per_face_couples;    /* Nb. face couples per periodicity */
  fvm_gnum_t   *n_g_per_face_couples;  /* Global nb. couples per periodicity */

  fvm_gnum_t  **per_face_couples;      /* Periodic face couples list. */

  /* Block ranges for parallel distribution */

  fvm_block_to_part_info_t   cell_bi;     /* Block info for cell data */
  fvm_block_to_part_info_t   face_bi;     /* Block info for face data */
  fvm_block_to_part_info_t   vertex_bi;   /* Block info for vertex data */
  fvm_block_to_part_info_t  *per_face_bi; /* Block info for parallel face
                                             couples */

} _mesh_reader_t;

/*============================================================================
 *  Global variables
 *============================================================================*/

static cs_bool_t  _use_sfc = true;

static _mesh_reader_t *_cs_glob_mesh_reader = NULL;

/*=============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create an empty mesh reader helper structure.
 *
 * returns:
 *   A pointer to a mesh reader helper structure
 *----------------------------------------------------------------------------*/

static _mesh_reader_t *
_mesh_reader_create(void)
{
  _mesh_reader_t  *mr = NULL;

  BFT_MALLOC(mr, 1, _mesh_reader_t);

  memset(mr, 0, sizeof(_mesh_reader_t));

  mr->n_g_faces = 0;
  mr->n_g_face_connect_size = 0;

  mr->read_cell_rank = 0;

  mr->cell_rank = NULL;
  mr->face_cells = NULL;
  mr->face_vertices_idx = NULL;
  mr->face_vertices = NULL;
  mr->cell_gc_id = NULL;
  mr->face_gc_id = NULL;
  mr->vertex_coords = NULL;

  mr->n_perio = 0;
  mr->periodicity_num = NULL;
  mr->n_per_face_couples = NULL;
  mr->n_g_per_face_couples = NULL;
  mr->per_face_couples = NULL;

  mr->per_face_bi = NULL;

  return mr;
}

/*----------------------------------------------------------------------------
 * Destroy a mesh reader helper structure
 *
 * mr <-> pointer to a mesh reader helper
 *
 * returns:
 *  NULL pointer
 *----------------------------------------------------------------------------*/

static void
_mesh_reader_destroy(_mesh_reader_t  **mr)
{
  _mesh_reader_t  *_mr = *mr;

  BFT_FREE(_mr->face_cells);
  BFT_FREE(_mr->face_vertices_idx);
  BFT_FREE(_mr->face_vertices);
  BFT_FREE(_mr->cell_gc_id);
  BFT_FREE(_mr->face_gc_id);
  BFT_FREE(_mr->vertex_coords);

  if (_mr->n_perio > 0) {
    int i;
    for (i = 0; i < _mr->n_perio; i++)
      BFT_FREE(_mr->per_face_couples[i]);
    BFT_FREE(_mr->per_face_couples);
    BFT_FREE(_mr->n_g_per_face_couples);
    BFT_FREE(_mr->n_per_face_couples);
    BFT_FREE(_mr->periodicity_num);
    BFT_FREE(_mr->per_face_bi);
  }

  BFT_FREE(*mr);
}

/*----------------------------------------------------------------------------
 * Add a periodicity to mesh->periodicities (fvm_periodicity_t *) structure.
 *
 * Parameters:
 *   mesh       <-> mesh
 *   perio_type <-- periodicity type
 *   perio_num  <-- periodicity number (identifier)
 *   matrix     <-- transformation matrix using homogeneous coordinates
 *----------------------------------------------------------------------------*/

static void
_add_periodicity(cs_mesh_t *mesh,
                 cs_int_t   perio_type,
                 cs_int_t   perio_num,
                 cs_real_t  matrix[3][4])
{
  cs_int_t  i, j, tr_id;
  double  _matrix[3][4];

  fvm_periodicity_type_t _perio_type = perio_type;

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 4; j++)
      _matrix[i][j] = matrix[i][j];
  }

  if (_perio_type == FVM_PERIODICITY_TRANSLATION)
    bft_printf(_(" Adding periodicity %d "
                 "(translation [%10.4e, %10.4e, %10.4e]).\n"),
               (int)perio_num, _matrix[0][3], _matrix[1][3], _matrix[2][3]);

  else if (_perio_type == FVM_PERIODICITY_ROTATION)
    bft_printf(_(" Adding periodicity %d (rotation).\n"),
               (int)perio_num);

  tr_id = fvm_periodicity_add_by_matrix(mesh->periodicity,
                                        perio_num,
                                        _perio_type,
                                        matrix);
}

/*----------------------------------------------------------------------------
 * Set block ranges for parallel reads
 *
 * mesh <-- pointer to mesh structure
 * mr   <-> mesh reader helper
 *----------------------------------------------------------------------------*/

static void
_set_block_ranges(cs_mesh_t       *mesh,
                  _mesh_reader_t  *mr)
{
  int i;

  int rank_id = cs_glob_rank_id;
  int n_ranks = cs_glob_n_ranks;

  /* Always build per_face_range in case of periodicity */

  if (mr->n_perio > 0) {
    BFT_MALLOC(mr->per_face_bi, mr->n_perio, fvm_block_to_part_info_t);
    memset(mr->per_face_bi, 0, sizeof(fvm_block_to_part_info_t)*mr->n_perio);
  }

  /* Set block sizes and ranges (useful for parallel mode) */

  mr->cell_bi = fvm_block_to_part_compute_sizes(rank_id,
                                                n_ranks,
                                                0,
                                                0,
                                                mesh->n_g_cells);

  mr->face_bi = fvm_block_to_part_compute_sizes(rank_id,
                                                n_ranks,
                                                0,
                                                0,
                                                mr->n_g_faces);

  mr->vertex_bi = fvm_block_to_part_compute_sizes(rank_id,
                                                  n_ranks,
                                                  0,
                                                  0,
                                                  mesh->n_g_vertices);

  for (i = 0; i < mr->n_perio; i++)
    mr->per_face_bi[i]
      = fvm_block_to_part_compute_sizes(rank_id,
                                        n_ranks,
                                        0,
                                        0,
                                        mr->n_g_per_face_couples[i]);
}

/*----------------------------------------------------------------------------
 * Read cell rank if available
 *
 * mesh <-- pointer to mesh structure
 * mr   <-> mesh reader helper
 * echo <-- echo (verbosity) level
 *----------------------------------------------------------------------------*/

static void
_read_cell_rank(cs_mesh_t       *mesh,
                _mesh_reader_t  *mr,
                long             echo)
{
  char file_name[32]; /* more than enough for "domain_number_<n_ranks>" */
  size_t  i;
  cs_io_sec_header_t  header;

  cs_io_t  *rank_pp_in = NULL;
  fvm_lnum_t   n_ranks = 0;
  fvm_gnum_t   n_elts = 0;
  fvm_gnum_t   n_g_cells = 0;

  const char  *unexpected_msg = N_("Message of type <%s> on <%s>\n"
                                   "unexpected or of incorrect size");

  if (n_ranks == 1)
    return;

#if (_CS_STDC_VERSION < 199901L)
  sprintf(file_name, "domain_number_%d", cs_glob_n_ranks);
#else
  snprintf(file_name, 32, "domain_number_%d", cs_glob_n_ranks);
#endif
  file_name[31] = '\0'; /* Just in case; processor counts would need to be
                           in the exa-range for this to be necessary. */

  /* Test if file exists */

  if (! bft_file_isreg(file_name)) {
    bft_printf(_(" No \"%s\" file available;\n"), file_name);
    if (_use_sfc == false)
      bft_printf(_("   an unoptimized domain partitioning will be used.\n"));
    else
      bft_printf(_("   domain partitioning will use a space-filling curve.\n"));
    return;
  }

  /* Open file */

#if defined(FVM_HAVE_MPI)
  rank_pp_in = cs_io_initialize(file_name,
                                "Domain partitioning, R0",
                                CS_IO_MODE_READ,
                                cs_glob_io_hints,
                                echo,
                                cs_glob_mpi_comm);
#else
  rank_pp_in = cs_io_initialize(file_name,
                                "Domain partitioning, R0",
                                CS_IO_MODE_READ,
                                cs_glob_io_hints,
                                echo);
#endif

  if (echo > 0)
    bft_printf("\n");

  /* Loop on read sections */

  while (rank_pp_in != NULL) {

    /* Receive headers */

    cs_io_read_header(rank_pp_in, &header);

    /* Treatment according to the header name */

    if (strncmp(header.sec_name, "n_cells",
                CS_IO_NAME_LEN) == 0) {

      if (header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name,
                  cs_io_get_name(rank_pp_in));
      else {
        cs_io_set_fvm_gnum(&header, rank_pp_in);
        cs_io_read_global(&header, &n_g_cells, rank_pp_in);
        if (n_g_cells != mesh->n_g_cells)
          bft_error(__FILE__, __LINE__, 0,
                    _("The number of cells reported by file\n"
                      "\"%s\" (%llu)\n"
                      "does not correspond the those of the mesh (%llu)."),
                    cs_io_get_name(rank_pp_in),
                    (unsigned long long)(n_g_cells),
                    (unsigned long long)(mesh->n_g_cells));
      }

    }
    else if (strncmp(header.sec_name, "n_ranks",
                     CS_IO_NAME_LEN) == 0) {

      if (header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name,
                  cs_io_get_name(rank_pp_in));
      else {
        cs_io_set_fvm_lnum(&header, rank_pp_in);
        cs_io_read_global(&header, &n_ranks, rank_pp_in);
        if (n_ranks != cs_glob_n_ranks)
          bft_error(__FILE__, __LINE__, 0,
                    _("Le number of ranks reported by file\n"
                      "\"%s\" (%d) does not\n"
                      "correspond to the current number of ranks (%d)."),
                    cs_io_get_name(rank_pp_in), (int)n_ranks,
                    (int)cs_glob_n_ranks);
      }

    }
    else if (strncmp(header.sec_name, "cell:domain number",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mesh->n_g_cells;
      if (header.n_vals != (fvm_file_off_t)n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name,
                  cs_io_get_name(rank_pp_in));
      else {
        mr->read_cell_rank = 1;
        cs_io_set_fvm_lnum(&header, rank_pp_in);
        if (mr->cell_bi.gnum_range[0] > 0)
          n_elts = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];
        BFT_MALLOC(mr->cell_rank, n_elts, fvm_lnum_t);
        cs_io_read_block(&header,
                         mr->cell_bi.gnum_range[0],
                         mr->cell_bi.gnum_range[1],
                         mr->cell_rank, rank_pp_in);
        for (i = 0; i < n_elts; i++) /* Convert 1 to n to 0 to n-1 */
          mr->cell_rank[i] -= 1;
      }
      cs_io_finalize(&rank_pp_in);
      rank_pp_in = NULL;
    }

    else
      bft_error(__FILE__, __LINE__, 0,
                _("Message of type <%s> on <%s> is unexpected."),
                header.sec_name, cs_io_get_name(rank_pp_in));
  }

  if (rank_pp_in != NULL)
    cs_io_finalize(&rank_pp_in);
}

#if defined(HAVE_MPI)

/*----------------------------------------------------------------------------
 * Mark faces by type (0 for interior, 1 for exterior faces with outwards
 * pointing normal, 2 for exterior faces with inwards pointing normal,
 * 3 for isolated faces) in parallel mode.
 *
 * The mesh structure is also updated with face counts and connectivity sizes.
 *
 * parameters:
 *   mesh              <-> pointer to mesh structure
 *   n_faces           <-- number of local faces
 *   face_ifs          <-- parallel and periodic faces interfaces set
 *   face_cell         <-- local face -> cell connectivity
 *   face_vertices_idx <-- local face -> vertices index
 *   face_type         --> face type marker
 *----------------------------------------------------------------------------*/

static void
_face_type_g(cs_mesh_t                  *mesh,
             fvm_lnum_t                  n_faces,
             const fvm_interface_set_t  *face_ifs,
             const fvm_lnum_t            face_cell[],
             const fvm_lnum_t            face_vertices_idx[],
             char                        face_type[])
{
  fvm_lnum_t i;
  int j;

  const int n_interfaces = fvm_interface_set_size(face_ifs);

  /* Mark base interior faces */

  for (i = 0; i < n_faces; i++) {
    if (face_cell[i*2] > 0 && face_cell[i*2+1] > 0)
      face_type[i] = '\0';
    else if (face_cell[i*2] > 0)
      face_type[i] = '\1';
    else if (face_cell[i*2 + 1] > 0)
      face_type[i] = '\2';
    else
      face_type[i] = '\3';
  }

  /* Also mark parallel and periodic faces as interior */

  for (j = 0; j < n_interfaces; j++) {

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
    fvm_lnum_t face_if_size = fvm_interface_size(face_if);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);

    for (i = 0; i < face_if_size; i++)
      face_type[loc_num[i] - 1] = '\0';

  }

  /* Now count faces of each type */

  mesh->n_i_faces = 0;
  mesh->n_b_faces = 0;
  mesh->i_face_vtx_connect_size = 0;
  mesh->b_face_vtx_connect_size = 0;

  for (i = 0; i < n_faces; i++) {
    fvm_lnum_t n_f_vertices = face_vertices_idx[i+1] - face_vertices_idx[i];
    if (face_type[i] == '\0') {
      mesh->n_i_faces += 1;
      mesh->i_face_vtx_connect_size += n_f_vertices;
    }
    else if (face_type[i] == '\1' || face_type[i] == '\2') {
      mesh->n_b_faces += 1;
      mesh->b_face_vtx_connect_size += n_f_vertices;
    }
  }
}

#endif /* defined(HAVE_MPI) */

/*----------------------------------------------------------------------------
 * Mark faces by type (0 for interior, 1 for exterior faces with outwards
 * pointing normal, 2 for exterior faces with inwards pointing normal,
 * 3 for isolated faces) in serial mode.
 *
 * The mesh structure is also updated with face counts and connectivity sizes.
 *
 * parameters:
 *   mesh               <-> pointer to mesh structure
 *   n_faces            <-- number of local faces
 *   n_periodic_couples <-- number of periodic couples associated with
 *                          each periodic list
 *   periodic_couples   <-- array indicating periodic couples (using
 *                          global numberings) for each list
 *   face_cell          <-- local face -> cell connectivity
 *   face_vertices_idx  <-- local face -> vertices index
 *   face_type          --> face type marker
 *----------------------------------------------------------------------------*/

static void
_face_type_l(cs_mesh_t                  *mesh,
             fvm_lnum_t                  n_faces,
             const fvm_lnum_t            n_periodic_couples[],
             const fvm_gnum_t     *const periodic_couples[],
             const fvm_lnum_t            face_cell[],
             const fvm_lnum_t            face_vertices_idx[],
             char                        face_type[])
{
  fvm_lnum_t i;
  int j;

  /* Mark base interior faces */

  for (i = 0; i < n_faces; i++) {
    if (face_cell[i*2] > 0 && face_cell[i*2+1] > 0)
      face_type[i] = '\0';
    else if (face_cell[i*2] > 0)
      face_type[i] = '\1';
    else if (face_cell[i*2 + 1] > 0)
      face_type[i] = '\2';
    else
      face_type[i] = '\3';
  }

  /* Also mark parallel and periodic faces as interior */

  for (i = 0; i < mesh->n_init_perio; i++) {

    const fvm_gnum_t *p_couples = periodic_couples[i];

    for (j = 0; j < n_periodic_couples[i]; j++) {
      face_type[p_couples[j*2] - 1] = '\0';
      face_type[p_couples[j*2 + 1] - 1] = '\0';
    }

  }

  /* Now count faces of each type */

  mesh->n_i_faces = 0;
  mesh->n_b_faces = 0;
  mesh->i_face_vtx_connect_size = 0;
  mesh->b_face_vtx_connect_size = 0;

  for (i = 0; i < n_faces; i++) {
    fvm_lnum_t n_f_vertices = face_vertices_idx[i+1] - face_vertices_idx[i];
    if (face_type[i] == '\0') {
      mesh->n_i_faces += 1;
      mesh->i_face_vtx_connect_size += n_f_vertices;
    }
    else if (face_type[i] == '\1' || face_type[i] == '\2') {
      mesh->n_b_faces += 1;
      mesh->b_face_vtx_connect_size += n_f_vertices;
    }
  }

  mesh->n_g_i_faces = mesh->n_i_faces;
  mesh->n_g_b_faces = mesh->n_b_faces;
}

/*----------------------------------------------------------------------------
 * Build internal and boundary face -> cell connectivity using a common
 * face -> cell connectivity and a face type marker.
 *
 * The corresponding arrays in the mesh structure are allocated and
 * defined by this function, and should have been previously empty.
 *
 * parameters:
 *   mesh      <-> pointer to mesh structure
 *   n_faces   <-- number of local faces
 *   face_cell <-- local face -> cell connectivity
 *   face_type <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_face_cell(cs_mesh_t         *mesh,
                   fvm_lnum_t         n_faces,
                   const fvm_lnum_t   face_cell[],
                   const char         face_type[])
{
  fvm_lnum_t i;

  size_t n_i_faces = 0;
  size_t n_b_faces = 0;

  /* Allocate arrays */

  BFT_MALLOC(mesh->i_face_cells, mesh->n_i_faces * 2, cs_int_t);
  BFT_MALLOC(mesh->b_face_cells, mesh->n_b_faces, cs_int_t);

  /* Now copy face -> cell connectivity */

  for (i = 0; i < n_faces; i++) {

    if (face_type[i] == '\0') {
      mesh->i_face_cells[n_i_faces*2]     = face_cell[i*2];
      mesh->i_face_cells[n_i_faces*2 + 1] = face_cell[i*2 + 1];
      n_i_faces++;
    }

    else if (face_type[i] == '\1') {
      mesh->b_face_cells[n_b_faces] = face_cell[i*2];
      n_b_faces++;
    }

    else if (face_type[i] == '\2') {
      mesh->b_face_cells[n_b_faces] = face_cell[i*2 + 1];
      n_b_faces++;
    }
  }
}

/*----------------------------------------------------------------------------
 * Build internal and boundary face -> vertices connectivity using a common
 * face -> vertices connectivity and a face type marker.
 *
 * The corresponding arrays in the mesh structure are allocated and
 * defined by this function, and should have been previously empty.
 *
 * parameters:
 *   mesh              <-> pointer to mesh structure
 *   n_faces           <-- number of local faces
 *   face_vertices_idx <-- local face -> vertices index
 *   face_vertices     <-- local face -> vertices connectivity
 *   face_type         <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_face_vertices(cs_mesh_t         *mesh,
                       fvm_lnum_t         n_faces,
                       const fvm_lnum_t   face_vertices_idx[],
                       const fvm_lnum_t   face_vertices[],
                       const char         face_type[])
{
  fvm_lnum_t i;
  size_t j;

  size_t n_i_faces = 0;
  size_t n_b_faces = 0;

  /* Allocate and initialize */

  BFT_MALLOC(mesh->i_face_vtx_idx, mesh->n_i_faces+1, cs_int_t);
  BFT_MALLOC(mesh->i_face_vtx_lst, mesh->i_face_vtx_connect_size, cs_int_t);

  mesh->i_face_vtx_idx[0] = 1;

  BFT_MALLOC(mesh->b_face_vtx_idx, mesh->n_b_faces+1, cs_int_t);
  mesh->b_face_vtx_idx[0] = 1;

  if (mesh->n_b_faces > 0)
    BFT_MALLOC(mesh->b_face_vtx_lst, mesh->b_face_vtx_connect_size, cs_int_t);

  /* Now copy face -> vertices connectivity */

  for (i = 0; i < n_faces; i++) {

    size_t n_f_vertices = face_vertices_idx[i+1] - face_vertices_idx[i];
    const fvm_lnum_t *_face_vtx = face_vertices + face_vertices_idx[i];

    if (face_type[i] == '\0') {
      fvm_lnum_t *_i_face_vtx =   mesh->i_face_vtx_lst
                                + mesh->i_face_vtx_idx[n_i_faces] - 1;
      for (j = 0; j < n_f_vertices; j++)
        _i_face_vtx[j] = _face_vtx[j];
      mesh->i_face_vtx_idx[n_i_faces + 1] =   mesh->i_face_vtx_idx[n_i_faces]
                                            + n_f_vertices;
      n_i_faces++;
    }

    else if (face_type[i] == '\1') {
      fvm_lnum_t *_b_face_vtx =   mesh->b_face_vtx_lst
                                + mesh->b_face_vtx_idx[n_b_faces] - 1;
      for (j = 0; j < n_f_vertices; j++)
        _b_face_vtx[j] = _face_vtx[j];
      mesh->b_face_vtx_idx[n_b_faces + 1] =   mesh->b_face_vtx_idx[n_b_faces]
                                            + n_f_vertices;
      n_b_faces++;
    }

    else if (face_type[i] == '\2') {
      fvm_lnum_t *_b_face_vtx =   mesh->b_face_vtx_lst
                                + mesh->b_face_vtx_idx[n_b_faces] - 1;
      for (j = 0; j < n_f_vertices; j++)
        _b_face_vtx[j] = _face_vtx[n_f_vertices - j - 1];
      mesh->b_face_vtx_idx[n_b_faces + 1] =   mesh->b_face_vtx_idx[n_b_faces]
                                            + n_f_vertices;
      n_b_faces++;
    }

  }
}

#if defined(HAVE_MPI)

/*----------------------------------------------------------------------------
 * Build internal and boundary face -> global numberings using a common
 * face group class id and a face type marker.
 *
 * The corresponding arrays in the mesh structure are allocated and
 * defined by this function, and should have been previously empty.
 *
 * parameters:
 *   mesh            <-> pointer to mesh structure
 *   n_faces         <-- number of local faces
 *   global_face_num <-- global face numbers
 *   face_type       <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_face_gnum(cs_mesh_t         *mesh,
                   fvm_lnum_t         n_faces,
                   const fvm_gnum_t   global_face_num[],
                   const char         face_type[])
{
  fvm_lnum_t i;

  size_t n_i_faces = 0;
  size_t n_b_faces = 0;

  fvm_lnum_t *global_i_face = NULL;
  fvm_lnum_t *global_b_face = NULL;

  fvm_io_num_t *tmp_face_num = NULL;

  /* Allocate arrays (including temporary arrays) */

  BFT_MALLOC(mesh->global_i_face_num, mesh->n_i_faces, fvm_gnum_t);
  BFT_MALLOC(mesh->global_b_face_num, mesh->n_b_faces, fvm_gnum_t);

  BFT_MALLOC(global_i_face, mesh->n_i_faces, fvm_lnum_t);
  BFT_MALLOC(global_b_face, mesh->n_b_faces, fvm_lnum_t);

  /* Now build internal and boundary face lists */

  for (i = 0; i < n_faces; i++) {

    if (face_type[i] == '\0')
      global_i_face[n_i_faces++] = i+1;

    else if (face_type[i] == '\1' || face_type[i] == '\2')
      global_b_face[n_b_faces++] = i+1;

  }

  /* Build an I/O numbering on internal faces to compact the global numbering */

  tmp_face_num = fvm_io_num_create(global_i_face,
                                   global_face_num,
                                   n_i_faces,
                                   0);

  memcpy(mesh->global_i_face_num,
         fvm_io_num_get_global_num(tmp_face_num),
         n_i_faces*sizeof(fvm_gnum_t));

  mesh->n_g_i_faces = fvm_io_num_get_global_count(tmp_face_num);

  assert(fvm_io_num_get_local_count(tmp_face_num) == (fvm_lnum_t)n_i_faces);

  tmp_face_num = fvm_io_num_destroy(tmp_face_num);

  /* Build an I/O numbering on boundary faces to compact the global numbering */

  tmp_face_num = fvm_io_num_create(global_b_face,
                                   global_face_num,
                                   n_b_faces,
                                   0);

  if (n_b_faces > 0)
    memcpy(mesh->global_b_face_num,
           fvm_io_num_get_global_num(tmp_face_num),
           n_b_faces*sizeof(fvm_gnum_t));

  mesh->n_g_b_faces = fvm_io_num_get_global_count(tmp_face_num);

  assert(fvm_io_num_get_local_count(tmp_face_num) == (fvm_lnum_t)n_b_faces);

  tmp_face_num = fvm_io_num_destroy(tmp_face_num);

  /* Free remaining temporary arrays */

  BFT_FREE(global_i_face);
  BFT_FREE(global_b_face);
}

#endif /* defined(HAVE_MPI) */

/*----------------------------------------------------------------------------
 * Build internal and boundary face -> group class id using a common
 * face group class id and a face type marker.
 *
 * The corresponding arrays in the mesh structure are allocated and
 * defined by this function, and should have been previously empty.
 *
 * parameters:
 *   mesh       <-> pointer to mesh structure
 *   n_faces    <-- number of local faces
 *   face_gc_id <-- local face group class id
 *   face_type  <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_face_gc_id(cs_mesh_t        *mesh,
                   fvm_lnum_t         n_faces,
                   const fvm_lnum_t   face_gc_id[],
                   const char         face_type[])
{
  fvm_lnum_t i;

  size_t n_i_faces = 0;
  size_t n_b_faces = 0;

  /* Allocate arrays */

  BFT_MALLOC(mesh->i_face_family, mesh->n_i_faces, cs_int_t);
  BFT_MALLOC(mesh->b_face_family, mesh->n_b_faces, cs_int_t);

  /* Now copy face group class (family) id */

  for (i = 0; i < n_faces; i++) {

    assert(face_gc_id[i] > -1 && face_gc_id[i] <= mesh->n_families);

    if (face_type[i] == '\0')
      mesh->i_face_family[n_i_faces++] = face_gc_id[i];

    else if (face_type[i] == '\1' || face_type[i] == '\2')
      mesh->b_face_family[n_b_faces++] = face_gc_id[i];

  }
}

/*----------------------------------------------------------------------------
 * Re-orient local periodic couples in mesh builder structure.
 * This is probably not necessary, but allows us to build arrays
 * identical to those produced by the preprocessor in version 1.3,
 * so this step may be removed after sufficient testing.
 *
 * parameters:
 *   mesh_builder      <-> pointer to mesh builder structure
 *   n_init_perio      <-- number of initial periodicities
 *   i_face_cell       <-- interior face->cell connectivity
 *----------------------------------------------------------------------------*/

static void
_orient_perio_couples(cs_mesh_builder_t  *mb,
                      int                 n_init_perio,
                      const fvm_lnum_t    i_face_cell[])
{
  fvm_lnum_t i;

  const fvm_lnum_t n_couples = mb->per_face_idx[n_init_perio];

  /* In parallel mode */

  if (mb->per_rank_lst != NULL) {

    const int local_rank = cs_glob_rank_id + 1;

    for (i = 0; i < n_couples; i++) {

      if (mb->per_rank_lst[i] == local_rank) {

        fvm_lnum_t inv_sgn = -1;
        fvm_lnum_t face_num_1 = mb->per_face_lst[i*2];
        fvm_lnum_t face_num_2 = mb->per_face_lst[i*2 + 1];
        if (face_num_1 < 0) {
          inv_sgn = 1;
          face_num_1 = -face_num_1;
        }

        if (i_face_cell[(face_num_1-1)*2] == 0) {
          assert(   i_face_cell[(face_num_1-1)*2 + 1] != 0
                 && i_face_cell[(face_num_2-1)*2] != 0
                 && i_face_cell[(face_num_2-1)*2 + 1] == 0);
          mb->per_face_lst[i*2] = face_num_2 * inv_sgn;
          mb->per_face_lst[i*2 + 1] = face_num_1;
        }
      }
    }
  }

  /* In serial mode */

  else { /* if (mb->per_rank_lst == NULL) */

    for (i = 0; i < n_couples; i++) {

      fvm_lnum_t inv_sgn = -1;
      fvm_lnum_t face_num_1 = mb->per_face_lst[i*2];
      fvm_lnum_t face_num_2 = mb->per_face_lst[i*2 + 1];
      if (face_num_1 < 0) {
        inv_sgn = 1;
        face_num_1 = -face_num_1;
      }

      if (i_face_cell[(face_num_1-1)*2] == 0) {
        mb->per_face_lst[i*2] = face_num_2 * inv_sgn;
        mb->per_face_lst[i*2 + 1] = face_num_1;
      }
    }
  }
}

#if defined(HAVE_MPI)

/*----------------------------------------------------------------------------
 * Extract periodic face connectivity information for mesh builder when
 * running in parallel mode.
 *
 * parameters:
 *   mesh_builder      <-> pointer to mesh builder structure
 *   n_init_perio      <-- number of initial periodicities
 *   n_faces           <-- number of local faces
 *   face_ifs          <-- parallel and periodic faces interfaces set
 *   face_type         <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_periodic_faces_g(cs_mesh_builder_t          *mb,
                          int                         n_init_perio,
                          fvm_lnum_t                  n_faces,
                          const fvm_interface_set_t  *face_ifs,
                          const char                  face_type[])
{
  fvm_lnum_t i;
  int j;

  fvm_lnum_t   i_face_count = 0;
  fvm_lnum_t  *i_face_id = NULL;
  fvm_lnum_t  *per_face_count = NULL;
  fvm_lnum_t  *if_index = NULL;
  fvm_lnum_t  *send_num = NULL, *recv_num = NULL;

  const int n_interfaces = fvm_interface_set_size(face_ifs);
  const fvm_lnum_t tr_index_size = n_init_perio*2 + 2;

  /* Allocate arrays in mesh builder (initializing per_face_idx) */

  BFT_MALLOC(mb->per_face_idx, n_init_perio + 1, cs_int_t);

  for (i = 0; i < n_init_perio + 1; i++)
    mb->per_face_idx[i] = 0;

  for (j = 0; j < n_interfaces; j++) {

    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);
    const int distant_rank = fvm_interface_rank(face_if);

    assert(fvm_interface_get_tr_index_size(face_if) == tr_index_size);

    /* Only count 1 transformation direction when corresponding
       faces are on the same rank (in which case they appear
       once per opposing direction transforms) */

    for (i = 1; i < tr_index_size-1; i++) {
      if ((distant_rank != cs_glob_rank_id) || (i%2 == 1))
        mb->per_face_idx[(i-1)/2 + 1] += tr_index[i+1] - tr_index[i];
    }
  }

  mb->per_face_idx[0] = 0;
  for (i = 1; i < n_init_perio+1; i++)
    mb->per_face_idx[i] += mb->per_face_idx[i-1];

  BFT_MALLOC(mb->per_face_lst, mb->per_face_idx[n_init_perio] * 2, cs_int_t);
  BFT_MALLOC(mb->per_rank_lst, mb->per_face_idx[n_init_perio], cs_int_t);

  /* Build face renumbering */

  BFT_MALLOC(i_face_id, n_faces, fvm_lnum_t);

  for (i = 0; i < n_faces; i++) {
    if (face_type[i] == '\0')
      i_face_id[i] = i_face_count++;
    else
      i_face_id[i] = -1;
  }

  /* Copy periodic interface arrays and renumber them */

  BFT_MALLOC(if_index, n_interfaces + 1, fvm_lnum_t);
  if_index[0] = 0;

  for (j = 0; j < n_interfaces; j++) {
    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);
    if_index[j+1] = if_index[j] + tr_index[tr_index_size - 1] - tr_index[1];
  }

  BFT_MALLOC(send_num, if_index[n_interfaces], fvm_lnum_t);
  BFT_MALLOC(recv_num, if_index[n_interfaces], fvm_lnum_t);

  for (j = 0; j < n_interfaces; j++) {

    fvm_lnum_t k, l;

    const fvm_lnum_t start_id = if_index[j];
    const fvm_lnum_t end_id = if_index[j+1];
    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);
    const fvm_lnum_t *loc_num = fvm_interface_get_local_num(face_if);
    const int distant_rank = fvm_interface_rank(face_if);

    for (k = start_id, l = tr_index[1]; k < end_id; k++, l++)
      send_num[k] = i_face_id[loc_num[l] - 1] + 1;

    if (distant_rank == cs_glob_rank_id) {
      const fvm_lnum_t *dist_num = fvm_interface_get_distant_num(face_if);
      for (k = start_id, l = tr_index[1]; k < end_id; k++, l++)
        recv_num[k] = i_face_id[dist_num[l] - 1] + 1;
    }
  }

  BFT_FREE(i_face_id);

  /* Exchange local face numbers */

  {
    MPI_Request  *request = NULL;
    MPI_Status  *status  = NULL;

    int request_count = 0;

    BFT_MALLOC(request, n_interfaces*2, MPI_Request);
    BFT_MALLOC(status, n_interfaces*2, MPI_Status);

    for (j = 0; j < n_interfaces; j++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
      int distant_rank = fvm_interface_rank(face_if);
      if (distant_rank != cs_glob_rank_id)
        MPI_Irecv(recv_num + if_index[j],
                  if_index[j+1] - if_index[j],
                  FVM_MPI_LNUM,
                  distant_rank,
                  distant_rank,
                  cs_glob_mpi_comm,
                  &(request[request_count++]));
    }

    for (j = 0; j < n_interfaces; j++) {
      const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
      int distant_rank = fvm_interface_rank(face_if);
      if (distant_rank != cs_glob_rank_id)
        MPI_Isend(send_num + if_index[j],
                  if_index[j+1] - if_index[j],
                  FVM_MPI_LNUM,
                  distant_rank,
                  (int)cs_glob_rank_id,
                  cs_glob_mpi_comm,
                  &(request[request_count++]));
    }

    MPI_Waitall(request_count, request, status);

    BFT_FREE(request);
    BFT_FREE(status);
  }

  /* Copy new interface information to mesh builder */

  BFT_MALLOC(per_face_count, n_init_perio, fvm_lnum_t);
  for (i = 0; i < n_init_perio; i++)
    per_face_count[i] = 0;

  for (j = 0; j < n_interfaces; j++) {

    fvm_lnum_t  tr_shift = 0;
    const fvm_interface_t *face_if = fvm_interface_set_get(face_ifs, j);
    const int distant_rank = fvm_interface_rank(face_if);
    const fvm_lnum_t *tr_index = fvm_interface_get_tr_index(face_if);

    for (i = 1; i < tr_index_size - 1; i++) {

      fvm_lnum_t n_elts = tr_index[i+1] - tr_index[i];

      if ((distant_rank != cs_glob_rank_id) || (i%2 == 1)) {

        fvm_lnum_t k, l, send_shift, recv_shift;

        int perio_id = (i-1)/2;
        int perio_sgn = (i%2)*2 - 1; /* 1 for odd, -1 for even */
        fvm_lnum_t n_dir_elts = tr_index[2*perio_id+2] - tr_index[2*perio_id+1];
        fvm_lnum_t n_rev_elts = tr_index[2*perio_id+3] - tr_index[2*perio_id+2];

        send_shift = if_index[j] + tr_shift;
        if (distant_rank != cs_glob_rank_id) {
          if (perio_sgn > 0)
            recv_shift = if_index[j] + n_rev_elts + tr_shift;
          else
            recv_shift = if_index[j] - n_dir_elts + tr_shift;
        }
        else /* if (i%2 == 1) */
          recv_shift = send_shift;

        for (k = 0; k < n_elts; k++) {
          l = mb->per_face_idx[perio_id] + per_face_count[perio_id];
          mb->per_face_lst[l*2]     = send_num[send_shift + k]*perio_sgn;
          mb->per_face_lst[l*2 + 1] = recv_num[recv_shift + k];
          mb->per_rank_lst[l] = distant_rank + 1;
          per_face_count[perio_id] += 1;
        }
      }

      tr_shift += n_elts;

    } /* End of loop on tr_index */

  } /* End of loop on interfaces */

#if 0 && defined(DEBUG) && !defined(NDEBUG)
 {
   cs_int_t  perio_id;

   bft_printf("\n  Dump periodic data received from preprocessor\n");

   for (perio_id = 0; perio_id < n_init_perio; perio_id++) {

     cs_int_t  start_id = mb->per_face_idx[perio_id];
     cs_int_t  end_id = mb->per_face_idx[perio_id+1];
     const cs_int_t  local_rank = (cs_glob_rank_id == -1) ? 0:cs_glob_rank_id;

     bft_printf("\n  Perio id: %4d - Number of elements: %7d "
                "(start: %7d - end: %7d)\n",
                perio_id, end_id-start_id, start_id, end_id);
     bft_printf("   id    | 1st face | 2nd face | associated rank\n");

     for (i = start_id; i < end_id; i++) {
       if (cs_glob_n_ranks > 1)
         bft_printf("%10d | %10d | %10d | %6d\n", i, mb->per_face_lst[2*i],
                    mb->per_face_lst[2*i+1], mb->per_rank_lst[i]-1);
       else
         bft_printf("%10d | %10d | %10d | %6d\n",
                    i, mb->per_face_lst[2*i], mb->per_face_lst[2*i+1],
                    local_rank);
     }
     bft_printf_flush();

   }

 }
#endif

  BFT_FREE(per_face_count);
  BFT_FREE(recv_num);
  BFT_FREE(send_num);
  BFT_FREE(if_index);
}

#endif /* defined(HAVE_MPI) */

/*----------------------------------------------------------------------------
 * Extract periodic face connectivity information for mesh builder when
 * running in serial mode.
 *
 * parameters:
 *   mesh_builder       <-> pointer to mesh builder structure
 *   n_init_perio       <-- number of initial periodicities
 *   n_faces            <-- number of local faces
 *   n_periodic_couples <-- number of periodic couples associated with
 *                          each periodic list
 *   periodic_couples   <-- array indicating periodic couples (using
 *                          global numberings) for each list
 *   face_type          <-- face type marker
 *----------------------------------------------------------------------------*/

static void
_extract_periodic_faces_l(cs_mesh_builder_t        *mb,
                          int                       n_init_perio,
                          fvm_lnum_t                n_faces,
                          const fvm_lnum_t          n_periodic_couples[],
                          const fvm_gnum_t   *const periodic_couples[],
                          const char                face_type[])
{
  int i;
  fvm_lnum_t j;

  fvm_lnum_t   i_face_count = 0;
  fvm_lnum_t  *i_face_id = NULL;

  /* Allocate arrays in mesh builder (initializing per_face_idx) */

  BFT_MALLOC(mb->per_face_idx, n_init_perio + 1, cs_int_t);

  mb->per_face_idx[0] = 0;
  for (i = 0; i < n_init_perio; i++)
    mb->per_face_idx[i+1] = mb->per_face_idx[i] + n_periodic_couples[i];

  BFT_MALLOC(mb->per_face_lst, mb->per_face_idx[n_init_perio] * 2, cs_int_t);

  /* Build face renumbering */

  BFT_MALLOC(i_face_id, n_faces, fvm_lnum_t);

  for (i = 0; i < n_faces; i++) {
    if (face_type[i] == '\0')
      i_face_id[i] = i_face_count++;
    else
      i_face_id[i] = -1;
  }

  /* Copy new interface information to mesh builder */

  for (i = 0; i < n_init_perio; i++) {

    const fvm_gnum_t *p_couples = periodic_couples[i];

    for (j = 0; j < n_periodic_couples[i]; j++) {

      fvm_lnum_t k = mb->per_face_idx[i] + j;

      mb->per_face_lst[k*2]   = i_face_id[p_couples[j*2] - 1] + 1;
      mb->per_face_lst[k*2+1] = i_face_id[p_couples[j*2+1] - 1] + 1;

    }

  }

  BFT_FREE(i_face_id);
}

#if defined(HAVE_MPI)

/*----------------------------------------------------------------------------
 * Compute cell centers using minimal local data.
 *
 * parameters:
 *   n_cells      <-- number of cells
 *   n_faces      <-- number of faces
 *   face_cells   <-- face -> cells connectivity
 *   face_vtx_idx <-- face -> vertices connectivity index
 *   face_vtx     <-- face -> vertices connectivity
 *   vtx_coord    <-- vertex coordinates
 *   cell_center  --> cell centers
 *----------------------------------------------------------------------------*/

static void
_cell_center(fvm_lnum_t        n_cells,
             fvm_lnum_t        n_faces,
             const fvm_lnum_t  face_cells[],
             const fvm_lnum_t  face_vtx_idx[],
             const fvm_lnum_t  face_vtx[],
             const cs_real_t   vtx_coord[],
             fvm_coord_t       cell_center[])
{
  fvm_lnum_t i, j;
  fvm_lnum_t vtx_id, face_id, start_id, end_id;
  fvm_lnum_t n_face_vertices;
  fvm_coord_t ref_normal[3], vtx_cog[3], face_center[3];

  fvm_lnum_t n_max_face_vertices = 0;

  cs_point_t *face_vtx_coord = NULL;
  fvm_coord_t *weight = NULL;

  assert(face_vtx_idx[0] == 0);

  BFT_MALLOC(weight, n_cells, fvm_coord_t);

  for (i = 0; i < n_cells; i++) {
    weight[i] = 0.0;
    for (j = 0; j < 3; j++)
      cell_center[i*3 + j] = 0.0;
  }

  /* Counting and allocation */

  n_max_face_vertices = 0;

  for (face_id = 0; face_id < n_faces; face_id++) {
    n_face_vertices = face_vtx_idx[face_id + 1] - face_vtx_idx[face_id];
    if (n_max_face_vertices <= n_face_vertices)
      n_max_face_vertices = n_face_vertices;
  }

  BFT_MALLOC(face_vtx_coord, n_max_face_vertices, cs_point_t);

  /* Loop on each face */

  for (face_id = 0; face_id < n_faces; face_id++) {

    /* Initialization */

    fvm_lnum_t tri_id;

    fvm_lnum_t cell_id_0 = face_cells[face_id*2] -1;
    fvm_lnum_t cell_id_1 = face_cells[face_id*2 + 1] -1;
    fvm_coord_t face_surface = 0.0;

    n_face_vertices = 0;

    start_id = face_vtx_idx[face_id];
    end_id = face_vtx_idx[face_id + 1];

    /* Define the polygon (P) according to the vertices (Pi) of the face */

    for (vtx_id = start_id; vtx_id < end_id; vtx_id++) {

      fvm_lnum_t shift = 3 * (face_vtx[vtx_id] - 1);
      for (i = 0; i < 3; i++)
        face_vtx_coord[n_face_vertices][i] = vtx_coord[shift + i];
      n_face_vertices++;

    }

    /* Compute the barycentre of the face vertices */

    for (i = 0; i < 3; i++) {
      vtx_cog[i] = 0.0;
      for (vtx_id = 0; vtx_id < n_face_vertices; vtx_id++)
        vtx_cog[i] += face_vtx_coord[vtx_id][i];
      vtx_cog[i] /= n_face_vertices;
    }

    /* Loop on the triangles of the face (defined by an edge of the face
       and its barycentre) */

    for (i = 0; i < 3; i++) {
      ref_normal[i] = 0.;
      face_center[i] = 0.0;
    }

    for (tri_id = 0 ; tri_id < n_face_vertices ; tri_id++) {

      fvm_coord_t tri_surface;
      fvm_coord_t vect1[3], vect2[3], tri_normal[3], tri_center[3];

      fvm_lnum_t id0 = tri_id;
      fvm_lnum_t id1 = (tri_id + 1)%n_face_vertices;

      /* Normal for each triangle */

      for (i = 0; i < 3; i++) {
        vect1[i] = face_vtx_coord[id0][i] - vtx_cog[i];
        vect2[i] = face_vtx_coord[id1][i] - vtx_cog[i];
      }

      tri_normal[0] = vect1[1] * vect2[2] - vect2[1] * vect1[2];
      tri_normal[1] = vect2[0] * vect1[2] - vect1[0] * vect2[2];
      tri_normal[2] = vect1[0] * vect2[1] - vect2[0] * vect1[1];

      if (tri_id == 0) {
        for (i = 0; i < 3; i++)
          ref_normal[i] = tri_normal[i];
      }

      /* Center of gravity for a triangle */

      for (i = 0; i < 3; i++) {
        tri_center[i] = (  vtx_cog[i]
                         + face_vtx_coord[id0][i]
                         + face_vtx_coord[id1][i]) / 3.0;
      }

      tri_surface = sqrt(  tri_normal[0]*tri_normal[0]
                         + tri_normal[1]*tri_normal[1]
                         + tri_normal[2]*tri_normal[2]) * 0.5;

      if ((  tri_normal[0]*ref_normal[0]
           + tri_normal[1]*ref_normal[1]
           + tri_normal[2]*ref_normal[2]) < 0.0)
        tri_surface *= -1.0;

      /* Now compute contribution to face center and surface */

      face_surface += tri_surface;

      for (i = 0; i < 3; i++)
        face_center[i] += tri_surface * tri_center[i];

    } /* End of loop  on triangles of the face */

    for (i = 0; i < 3; i++)
      face_center[i] /= face_surface;

    /* Now contribute to cell centers */

    assert(cell_id_0 > -2 && cell_id_1 > -2);

    if (cell_id_0 > -1) {
      for (i = 0; i < 3; i++)
        cell_center[cell_id_0*3 + i] += face_center[i]*face_surface;
      weight[cell_id_0] += face_surface;
    }

    if (cell_id_1 > -1) {
      for (i = 0; i < 3; i++)
        cell_center[cell_id_1*3 + i] += face_center[i]*face_surface;
      weight[cell_id_1] += face_surface;
    }

  } /* End of loop on faces */

  BFT_FREE(face_vtx_coord);

  for (i = 0; i < n_cells; i++) {
    for (j = 0; j < 3; j++)
      cell_center[i*3 + j] /= weight[i];
  }

  BFT_FREE(weight);
}

/*----------------------------------------------------------------------------
 * Compute cell centers using block data read from file.
 *
 * parameters:
 *   mr          <-- pointer to mesh reader helper structure
 *   cell_center --> cell centers array
 *   comm        <-- associated MPI communicator
 *----------------------------------------------------------------------------*/

static void
_precompute_cell_center(const _mesh_reader_t     *mr,
                        fvm_coord_t               cell_center[],
                        MPI_Comm                  comm)
{
  fvm_lnum_t i;
  int n_ranks = 0;

  fvm_datatype_t gnum_type = (sizeof(fvm_gnum_t) == 8) ? FVM_UINT64 : FVM_UINT32;
  fvm_datatype_t real_type = (sizeof(cs_real_t) == 8) ? FVM_DOUBLE : FVM_FLOAT;

  fvm_lnum_t _n_cells = 0;
  fvm_lnum_t _n_faces = 0;
  fvm_lnum_t _n_vertices = 0;

  fvm_gnum_t *_cell_num = NULL;
  fvm_gnum_t *_face_num = NULL;
  fvm_gnum_t *_vtx_num = NULL;
  fvm_gnum_t *_face_gcells = NULL;
  fvm_gnum_t *_face_gvertices = NULL;

  fvm_lnum_t *_face_cells = NULL;
  fvm_lnum_t *_face_vertices_idx = NULL;
  fvm_lnum_t *_face_vertices = NULL;

  cs_real_t *_vtx_coord = NULL;

  fvm_block_to_part_t *d = NULL;

  /* Initialization */

  MPI_Comm_size(comm, &n_ranks);

  assert((sizeof(fvm_lnum_t) == 4) || (sizeof(fvm_lnum_t) == 8));

  _n_cells = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];

  BFT_MALLOC(_cell_num, _n_cells, fvm_gnum_t);

  for (i = 0; i < _n_cells; i++)
    _cell_num[i] = mr->cell_bi.gnum_range[0] + i;

  if (_n_cells == 0)
    bft_error(__FILE__, __LINE__, 0,
              _("Number of cells on rank %d is zero.\n"
                "(number of cells / number of processes ratio too low)."),
              (int)cs_glob_rank_id);

  /* Distribute faces */
  /*------------------*/

  d = fvm_block_to_part_create_by_adj_s(comm,
                                        mr->face_bi,
                                        mr->cell_bi,
                                        2,
                                        mr->face_cells,
                                        NULL);

  _n_faces = fvm_block_to_part_get_n_part_ents(d);

  BFT_MALLOC(_face_gcells, _n_faces*2, fvm_gnum_t);

  /* Face -> cell connectivity */

  fvm_block_to_part_copy_array(d,
                               gnum_type,
                               2,
                               mr->face_cells,
                               _face_gcells);

  /* Now convert face -> cell connectivity to local cell numbers */

  BFT_MALLOC(_face_cells, _n_faces*2, fvm_lnum_t);

  fvm_block_to_part_global_to_local(_n_faces*2,
                                    1,
                                    _n_cells,
                                    _cell_num,
                                    _face_gcells,
                                    _face_cells);

  BFT_FREE(_cell_num);
  BFT_FREE(_face_gcells);

  /* Face connectivity */

  BFT_MALLOC(_face_vertices_idx, _n_faces + 1, fvm_lnum_t);

  fvm_block_to_part_copy_index(d,
                               mr->face_vertices_idx,
                               _face_vertices_idx);

  BFT_MALLOC(_face_gvertices, _face_vertices_idx[_n_faces], fvm_gnum_t);

  fvm_block_to_part_copy_indexed(d,
                                 gnum_type,
                                 mr->face_vertices_idx,
                                 mr->face_vertices,
                                 _face_vertices_idx,
                                 _face_gvertices);

  _face_num = fvm_block_to_part_transfer_gnum(d);

  fvm_block_to_part_destroy(&d);

  /* Vertices */

  d = fvm_block_to_part_create_adj(comm,
                                   mr->vertex_bi,
                                   _face_vertices_idx[_n_faces],
                                   _face_gvertices);

  _n_vertices = fvm_block_to_part_get_n_part_ents(d);

  BFT_MALLOC(_vtx_coord, _n_vertices*3, cs_real_t);

  fvm_block_to_part_copy_array(d,
                               real_type,
                               3,
                               mr->vertex_coords,
                               _vtx_coord);

  _vtx_num = fvm_block_to_part_transfer_gnum(d);

  fvm_block_to_part_destroy(&d);

  /* Now convert face -> vertex connectivity to local vertex numbers */

  BFT_MALLOC(_face_vertices, _face_vertices_idx[_n_faces], fvm_lnum_t);

  fvm_block_to_part_global_to_local(_face_vertices_idx[_n_faces],
                                    1,
                                    _n_vertices,
                                    _vtx_num,
                                    _face_gvertices,
                                    _face_vertices);

  BFT_FREE(_face_gvertices);

  _cell_center(_n_cells,
               _n_faces,
               _face_cells,
               _face_vertices_idx,
               _face_vertices,
               _vtx_coord,
               cell_center);

  BFT_FREE(_vtx_coord);
  BFT_FREE(_vtx_num);

  BFT_FREE(_face_cells);

  BFT_FREE(_face_vertices_idx);
  BFT_FREE(_face_vertices);

  BFT_FREE(_face_num);
}

/*----------------------------------------------------------------------------
 * Compute cell centers using block data read from file.
 *
 * parameters:
 *   mr          <-_ pointer to mesh reader helper structure
 *   cell_rank   --> cell rank
 *   comm        <-- associated MPI communicator
 *----------------------------------------------------------------------------*/

static void
_cell_rank_by_sfc(const _mesh_reader_t     *mr,
                  int                       cell_rank[],
                  MPI_Comm                  comm)
{
  fvm_lnum_t i;
  fvm_lnum_t n_cells = 0, block_size = 0, rank_step = 0;
  fvm_coord_t *cell_center = NULL;
  fvm_io_num_t *cell_io_num = NULL;
  const fvm_gnum_t *cell_num = NULL;

  n_cells = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];
  block_size = mr->cell_bi.block_size;
  rank_step = mr->cell_bi.rank_step;

  BFT_MALLOC(cell_center, n_cells*3, fvm_coord_t);

  _precompute_cell_center(mr, cell_center, comm);

  cell_io_num = fvm_io_num_create_from_coords(cell_center, 3, n_cells);

  BFT_FREE(cell_center);

  cell_num = fvm_io_num_get_global_num(cell_io_num);

  /* Determine rank based on global numbering with SFC ordering */
  for (i = 0; i < n_cells; i++)
    cell_rank[i] = ((cell_num[i] - 1) / block_size) * rank_step;

  cell_io_num = fvm_io_num_destroy(cell_io_num);
}

/*----------------------------------------------------------------------------
 * Organize data read by blocks in parallel and build most mesh structures.
 *
 * parameters:
 *   mesh         <-> pointer to mesh structure
 *   mesh_builder <-> pointer to mesh builder structure
 *   mr           <-> pointer to mesh reader helper structure
 *   comm         <-- associated MPI communicator
 *----------------------------------------------------------------------------*/

static void
_decompose_data_g(cs_mesh_t          *mesh,
                  cs_mesh_builder_t  *mesh_builder,
                  _mesh_reader_t     *mr,
                  MPI_Comm            comm)
{
  fvm_lnum_t i;
  int n_ranks = 0;

  fvm_datatype_t lnum_type = (sizeof(fvm_lnum_t) == 8) ? FVM_INT64 : FVM_INT32;
  fvm_datatype_t gnum_type = (sizeof(fvm_gnum_t) == 8) ? FVM_UINT64 : FVM_UINT32;
  fvm_datatype_t real_type = (sizeof(cs_real_t) == 8) ? FVM_DOUBLE : FVM_FLOAT;

  int use_cell_rank = 0;

  fvm_lnum_t _n_faces = 0;
  fvm_gnum_t *_face_num = NULL;
  fvm_gnum_t *_face_gcells = NULL;
  fvm_gnum_t *_face_gvertices = NULL;

  fvm_lnum_t *_face_cells = NULL;
  fvm_lnum_t *_face_gc_id = NULL;
  fvm_lnum_t *_face_vertices_idx = NULL;
  fvm_lnum_t *_face_vertices = NULL;

  char *face_type = NULL;
  fvm_interface_set_t *face_ifs = NULL;

  fvm_block_to_part_t *d = NULL;

  /* Initialization */

  MPI_Comm_size(comm, &n_ranks);

  assert((sizeof(fvm_lnum_t) == 4) || (sizeof(fvm_lnum_t) == 8));

  /* Different handling of cells depending on whether decomposition
     data is available or not. */

  if (mr->read_cell_rank != 0)
    use_cell_rank = 1;

  else if (_use_sfc == true && mr->read_cell_rank == 0) {

    fvm_lnum_t _n_cells = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];

    BFT_MALLOC(mr->cell_rank, _n_cells, fvm_lnum_t);

    _cell_rank_by_sfc(mr,  mr->cell_rank, comm);

    use_cell_rank = 1;
  }

  if (use_cell_rank != 0) {

    d = fvm_block_to_part_create_by_rank(comm,
                                         mr->cell_bi,
                                         mr->cell_rank);

    mesh->n_cells = fvm_block_to_part_get_n_part_ents(d);

    BFT_MALLOC(mesh->cell_family, mesh->n_cells, fvm_lnum_t);

    fvm_block_to_part_copy_array(d,
                                 lnum_type,
                                 1,
                                 mr->cell_gc_id,
                                 mesh->cell_family);

    BFT_FREE(mr->cell_gc_id);

    mesh->global_cell_num = fvm_block_to_part_transfer_gnum(d);

    fvm_block_to_part_destroy(&d);

  }
  else {

    mesh->n_cells = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];

    BFT_MALLOC(mesh->global_cell_num, mesh->n_cells, fvm_gnum_t);

    for (i = 0; i < mesh->n_cells; i++)
      mesh->global_cell_num[i] = mr->cell_bi.gnum_range[0] + i;

    mesh->cell_family = mr->cell_gc_id;
    mr->cell_gc_id = NULL;
  }

  if (mesh->n_cells == 0)
    bft_error(__FILE__, __LINE__, 0,
              _("Number of cells on rank %d is zero.\n"
                "(number of cells / number of processes ratio too low)."),
              (int)cs_glob_rank_id);

  /* Distribute faces */
  /*------------------*/

  d = fvm_block_to_part_create_by_adj_s(comm,
                                        mr->face_bi,
                                        mr->cell_bi,
                                        2,
                                        mr->face_cells,
                                        mr->cell_rank);

  BFT_FREE(mr->cell_rank); /* Not needed anymore */

  _n_faces = fvm_block_to_part_get_n_part_ents(d);

  BFT_MALLOC(_face_gcells, _n_faces*2, fvm_gnum_t);

  /* Face -> cell connectivity */

  fvm_block_to_part_copy_array(d,
                               gnum_type,
                               2,
                               mr->face_cells,
                               _face_gcells);

  BFT_FREE(mr->face_cells);

  /* Now convert face -> cell connectivity to local cell numbers */

  BFT_MALLOC(_face_cells, _n_faces*2, fvm_lnum_t);

  fvm_block_to_part_global_to_local(_n_faces*2,
                                    1,
                                    mesh->n_cells,
                                    mesh->global_cell_num,
                                    _face_gcells,
                                    _face_cells);

  BFT_FREE(_face_gcells);

  /* Face family */

  BFT_MALLOC(_face_gc_id, _n_faces, fvm_lnum_t);

  fvm_block_to_part_copy_array(d,
                               lnum_type,
                               1,
                               mr->face_gc_id,
                               _face_gc_id);

  BFT_FREE(mr->face_gc_id);

  /* Face connectivity */

  BFT_MALLOC(_face_vertices_idx, _n_faces + 1, fvm_lnum_t);

  fvm_block_to_part_copy_index(d,
                               mr->face_vertices_idx,
                               _face_vertices_idx);

  BFT_MALLOC(_face_gvertices, _face_vertices_idx[_n_faces], fvm_gnum_t);

  fvm_block_to_part_copy_indexed(d,
                                 gnum_type,
                                 mr->face_vertices_idx,
                                 mr->face_vertices,
                                 _face_vertices_idx,
                                 _face_gvertices);

  BFT_FREE(mr->face_vertices_idx);
  BFT_FREE(mr->face_vertices);

  _face_num = fvm_block_to_part_transfer_gnum(d);

  fvm_block_to_part_destroy(&d);

  /* Vertices */

  d = fvm_block_to_part_create_adj(comm,
                                   mr->vertex_bi,
                                   _face_vertices_idx[_n_faces],
                                   _face_gvertices);

  mesh->n_vertices = fvm_block_to_part_get_n_part_ents(d);

  BFT_MALLOC(mesh->vtx_coord, mesh->n_vertices*3, cs_real_t);

  fvm_block_to_part_copy_array(d,
                               real_type,
                               3,
                               mr->vertex_coords,
                               mesh->vtx_coord);

  BFT_FREE(mr->vertex_coords);

  mesh->global_vtx_num = fvm_block_to_part_transfer_gnum(d);

  fvm_block_to_part_destroy(&d);

  /* Now convert face -> vertex connectivity to local vertex numbers */

  BFT_MALLOC(_face_vertices, _face_vertices_idx[_n_faces], fvm_lnum_t);

  fvm_block_to_part_global_to_local(_face_vertices_idx[_n_faces],
                                    1,
                                    mesh->n_vertices,
                                    mesh->global_vtx_num,
                                    _face_gvertices,
                                    _face_vertices);

  BFT_FREE(_face_gvertices);

  /* In case of periodicity, build an fvm_interface so as to obtain
     periodic face correspondants in local numbering (periodic couples
     need not be defined by the ranks owning one of the 2 members
     for the interface to be built correctly). */

  face_ifs
    = fvm_interface_set_create(_n_faces,
                               NULL,
                               _face_num,
                               mesh->periodicity,
                               mr->n_perio,
                               mr->periodicity_num,
                               mr->n_per_face_couples,
                               (const fvm_gnum_t **const)mr->per_face_couples);

  /* We may now separate interior from boundary faces */

  BFT_MALLOC(face_type, _n_faces, char);

  _face_type_g(mesh,
               _n_faces,
               face_ifs,
               _face_cells,
               _face_vertices_idx,
               face_type);

  _extract_face_cell(mesh, _n_faces, _face_cells, face_type);

  BFT_FREE(_face_cells);

  if (mr->n_perio > 0) {

    _extract_periodic_faces_g(mesh_builder,
                              mesh->n_init_perio,
                              _n_faces,
                              face_ifs,
                              face_type);

    _orient_perio_couples(mesh_builder,
                          mesh->n_init_perio,
                          mesh->i_face_cells);

  }

  face_ifs = fvm_interface_set_destroy(face_ifs);

  _extract_face_vertices(mesh,
                         _n_faces,
                         _face_vertices_idx,
                         _face_vertices,
                         face_type);

  BFT_FREE(_face_vertices_idx);
  BFT_FREE(_face_vertices);

  _extract_face_gnum(mesh,
                     _n_faces,
                     _face_num,
                     face_type);

  BFT_FREE(_face_num);

  _extract_face_gc_id(mesh,
                      _n_faces,
                      _face_gc_id,
                      face_type);

  BFT_FREE(_face_gc_id);

  BFT_FREE(face_type);
}

#endif /* defined(HAVE_MPI) */

/*----------------------------------------------------------------------------
 * Organize data read locally and build most mesh structures
 *
 * parameters:
 *   mesh         <-- pointer to mesh structure
 *   mesh_builder <-- pointer to mesh builder structure
 *   mr           <-> pointer to mesh reader helper structure
 *----------------------------------------------------------------------------*/

static void
_decompose_data_l(cs_mesh_t          *mesh,
                  cs_mesh_builder_t  *mesh_builder,
                  _mesh_reader_t     *mr)
{
  fvm_lnum_t i;

  fvm_lnum_t _n_faces = 0;

  fvm_lnum_t *_face_cells = NULL;
  fvm_lnum_t *_face_vertices_idx = NULL;
  fvm_lnum_t *_face_vertices = NULL;

  char *face_type = NULL;

  /* Initialization */

  assert((sizeof(fvm_lnum_t) == 4) || (sizeof(fvm_lnum_t) == 8));

  mesh->n_cells = mr->cell_bi.gnum_range[1] - 1;

  /* Cell families are already of the correct type,
     so they can simply be moved */

  mesh->cell_family = mr->cell_gc_id;
  mr->cell_gc_id = NULL;

  /* Build faces */
  /*-------------*/

  _n_faces = mr->face_bi.gnum_range[1] - 1;

  /* Now copy face -> cell connectivity to local cell numbers */

  BFT_MALLOC(_face_cells, _n_faces*2, fvm_lnum_t);

  for (i = 0; i < _n_faces; i++) {
    _face_cells[i*2] = mr->face_cells[i*2];
    _face_cells[i*2 + 1] = mr->face_cells[i*2 + 1];
  }

  BFT_FREE(mr->face_cells);

  /* Face connectivity */

  BFT_MALLOC(_face_vertices_idx, _n_faces + 1, fvm_lnum_t);

  for (i = 0; i < _n_faces+1; i++)
    _face_vertices_idx[i] = mr->face_vertices_idx[i];

  BFT_FREE(mr->face_vertices_idx);

  BFT_MALLOC(_face_vertices, _face_vertices_idx[_n_faces], fvm_lnum_t);

  for (i = 0; i < _face_vertices_idx[_n_faces]; i++)
    _face_vertices[i] = mr->face_vertices[i];

  BFT_FREE(mr->face_vertices);

  /* Vertices */

  mesh->n_vertices = mr->vertex_bi.gnum_range[1] - 1;

  mesh->vtx_coord = mr->vertex_coords;
  mr->vertex_coords = NULL;

  /* We may now separate interior from boundary faces */

  BFT_MALLOC(face_type, _n_faces, char);

  _face_type_l(mesh,
               _n_faces,
               mr->n_per_face_couples,
               (const fvm_gnum_t **const)mr->per_face_couples,
               _face_cells,
               _face_vertices_idx,
               face_type);

  _extract_face_cell(mesh, _n_faces, _face_cells, face_type);

  BFT_FREE(_face_cells);

  if (mr->n_perio > 0) {

    _extract_periodic_faces_l(mesh_builder,
                              mesh->n_init_perio,
                              _n_faces,
                              mr->n_per_face_couples,
                              (const fvm_gnum_t **const)mr->per_face_couples,
                              face_type);

    _orient_perio_couples(mesh_builder,
                          mesh->n_init_perio,
                          mesh->i_face_cells);

  }

  _extract_face_vertices(mesh,
                         _n_faces,
                         _face_vertices_idx,
                         _face_vertices,
                         face_type);

  BFT_FREE(_face_vertices_idx);
  BFT_FREE(_face_vertices);

  _extract_face_gc_id(mesh,
                      _n_faces,
                      mr->face_gc_id,
                      face_type);

  BFT_FREE(mr->face_gc_id);

  BFT_FREE(face_type);
}

/*============================================================================
 *  Public functions definition for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Query or modification of the option for domain partitioning when no
 * partitioning file is present.
 *
 * This function returns 1 or 2 according to the selected algorithm.
 *
 * Fortran interface :
 *
 * SUBROUTINE ALGDOM (IOPT)
 * *****************
 *
 * INTEGER          IOPT        : <-> : Choice of the partitioning base
 *                                        0: query
 *                                        1: initial numbering
 *                                        2: space-filling curve (default)
 *----------------------------------------------------------------------------*/

void
CS_PROCF(algdom, ALGDOM)(cs_int_t  *iopt)
{
  *iopt = cs_preprocessor_data_part_choice(*iopt);
}

/*----------------------------------------------------------------------------
 * Receive messages from the pre-processor about the dimensions of mesh
 * parameters
 *
 * FORTRAN Interface:
 *
 * SUBROUTINE LEDEVI(NOMRUB, TYPENT, NBRENT, TABENT)
 * *****************
 *
 * INTEGER          NDIM        : <-- : Spacial dimension (3)
 * INTEGER          NFML        : <-- : Number of families (group classes)
 * INTEGER          NPRFML      : <-- : Number of peroperties per family
 * INTEGER          IPERIO      : <-- : Periodicity inidcator
 * INTEGER          IPEROT      : <-- : Number of rotation periodicities
 *----------------------------------------------------------------------------*/

void
CS_PROCF(ledevi, LEDEVI)(cs_int_t   *ndim,
                         cs_int_t   *nfml,
                         cs_int_t   *nprfml,
                         cs_int_t   *iperio,
                         cs_int_t   *iperot)
{
  cs_int_t  i;
  cs_io_sec_header_t  header;

  fvm_gnum_t n_elts = 0;
  cs_bool_t  dim_read = false;
  cs_bool_t  end_read = false;
  cs_io_t  *pp_in = NULL;
  cs_mesh_t  *mesh = cs_glob_mesh;
  _mesh_reader_t *mr = NULL;

  const char  *unexpected_msg = N_("Message of type <%s> on <%s>\n"
                                   "unexpected or of incorrect size");

  /* Initialize reading of Preprocessor output */

#if defined(FVM_HAVE_MPI)
  cs_glob_pp_io = cs_io_initialize("preprocessor_output",
                                   "Face-based mesh definition, R0",
                                   CS_IO_MODE_READ,
                                   cs_glob_io_hints,
                                   CS_IO_ECHO_OPEN_CLOSE,
                                   cs_glob_mpi_comm);
#else
  cs_glob_pp_io = cs_io_initialize("preprocessor_output",
                                   "Face-based mesh definition, R0",
                                   CS_IO_MODE_READ,
                                   CS_IO_ECHO_OPEN_CLOSE,
                                   -1);
#endif

  pp_in = cs_glob_pp_io;

  /* Initialize parameter values */

  *ndim = 3;
  *nfml = 0;
  *nprfml = 0;

  if (mesh->n_init_perio > 0)
    *iperio = 1;
  if (mesh->have_rotation_perio > 0)
    *iperot = 1;

  /* Periodicities can be added before reading the preprocessor_output and
     defined as a joining, but we don't want the n_init_perio to be set
     before reading the preprocessor_output. It will be redefined later, either
     while reading the preprocessor_output file or in the joining algorithm */

  mesh->n_init_perio = 0;

  mr = _mesh_reader_create();

  _cs_glob_mesh_reader = mr;

  /* Loop on read sections */

  while (end_read == false) {

    /* Receive headers and clen header names */

    cs_io_read_header(pp_in, &header);

    /* Treatment according to the header name */

    if (strncmp(header.sec_name, "EOF", CS_IO_NAME_LEN)
        == 0) {
      cs_io_finalize(&pp_in);
      pp_in = NULL;
    }

    if (strncmp(header.sec_name, "start_block:dimensions",
                CS_IO_NAME_LEN) == 0) {

      if (dim_read == false)
        dim_read = true;
      else
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));

    }
    else if (strncmp(header.sec_name, "end_block:dimensions",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read == true) {
        dim_read = false;
        end_read = true;
      }
      else
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));

    }

    /* Receive dimensions from the pre-processor */

    else if (strncmp(header.sec_name, "ndim",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else
        cs_io_read_global(&header, (void *) &(mesh->dim), pp_in);

    }
    else if (strncmp(header.sec_name, "n_cells",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        cs_io_read_global(&header, &(mesh->n_g_cells), pp_in);
      }

    }
    else if (strncmp(header.sec_name, "n_faces",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        cs_io_read_global(&header, &(mr->n_g_faces), pp_in);
      }

    }
    else if (strncmp(header.sec_name, "n_vertices",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        cs_io_read_global(&header, &(mesh->n_g_vertices), pp_in);
      }

    }
    else if (strncmp(header.sec_name, "face_vertices_size",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        cs_io_read_global(&header, &(mr->n_g_face_connect_size), pp_in);
      }

    }
    else if (strncmp(header.sec_name, "n_group_classes",
                     CS_IO_NAME_LEN) == 0) {
      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else
        cs_io_read_global(&header, (void *) &(mesh->n_families), pp_in);

    }
    else if (strncmp(header.sec_name, "n_group_class_props_max",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else
        cs_io_read_global(&header,
                          (void *) &(mesh->n_max_family_items), pp_in);

    }
    else if (strncmp(header.sec_name, "n_groups",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else
        cs_io_read_global(&header, (void *) &(mesh->n_groups), pp_in);

    }
    else if (strncmp(header.sec_name, "group_name_index",
                     CS_IO_NAME_LEN) == 0) {

      if ((cs_int_t)header.n_vals != mesh->n_groups + 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        BFT_MALLOC(mesh->group_idx, mesh->n_groups + 1, cs_int_t);
        cs_io_read_global(&header, (void *) mesh->group_idx, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "group_name",
                     CS_IO_NAME_LEN) == 0) {

      if (   mesh->group_idx == NULL
          || (cs_int_t)header.n_vals != mesh->group_idx[mesh->n_groups] - 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        BFT_MALLOC(mesh->group_lst, header.n_vals + 1, char);
        cs_io_read_global(&header, (void *) mesh->group_lst, pp_in);
      }

    }
    else if (   strncmp(header.sec_name, "group_class_properties",
                        CS_IO_NAME_LEN) == 0
             || strncmp(header.sec_name, "iprfml",
                        CS_IO_NAME_LEN) == 0) {

      n_elts = mesh->n_families * mesh->n_max_family_items;
      if (dim_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        BFT_MALLOC(mesh->family_item, n_elts, cs_int_t);
        cs_io_read_global(&header, (void *) mesh->family_item, pp_in);
      }

    }

    /* Additional messages for periodicity. Dimensions for periodic ghost
       cells have been received before. Here we allocate parameter list
       for periodicities and coupled face list for halo builder. */

    else if (strncmp(header.sec_name, "n_periodic_directions",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_read_global(&header, (void *) &(mesh->n_init_perio), pp_in);

        assert(mesh->n_init_perio > 0);

        *iperio = 1;
        mesh->periodicity = fvm_periodicity_create(0.001);

        BFT_MALLOC(mr->periodicity_num, mesh->n_init_perio, int);
        BFT_MALLOC(mr->n_per_face_couples, mesh->n_init_perio, fvm_lnum_t);
        BFT_MALLOC(mr->n_g_per_face_couples, mesh->n_init_perio, fvm_gnum_t);
        BFT_MALLOC(mr->per_face_couples, mesh->n_init_perio, fvm_gnum_t *);

        mr->n_perio = mesh->n_init_perio;

        for (i = 0; i < mesh->n_init_perio; i++) {
          mr->periodicity_num[i] = i+1;
          mr->n_per_face_couples[i] = 0;
          mr->n_g_per_face_couples[i] = 0;
          mr->per_face_couples[i] = NULL;
        }
      }

    }
    else if (strncmp(header.sec_name, "n_periodic_rotations",
                     CS_IO_NAME_LEN) == 0) {

      if (dim_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_read_global(&header, (void *) iperot, pp_in);
        if (*iperot > 0)
          mesh->have_rotation_perio = 1;
      }

    }
    else if (strncmp(header.sec_name, "n_periodic_faces",
                     CS_IO_NAME_LEN) == 0) {

      if ((cs_int_t)header.n_vals != mesh->n_init_perio)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        cs_io_read_global(&header, mr->n_g_per_face_couples, pp_in);
        for (i = 0; i < mesh->n_init_perio; i++)
          mr->n_g_per_face_couples[i] /= 2;
      }

    }
    else
      bft_error(__FILE__, __LINE__, 0,
                _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));

  } /* End of test on headers */

  /* Return values */

  *ndim = mesh->dim;
  *nfml = mesh->n_families;
  *nprfml = mesh->n_max_family_items;

  mesh->n_domains = cs_glob_n_ranks;
  mesh->domain_num = cs_glob_rank_id + 1;

  /* Update data in cs_mesh_t structure in serial mode */

  if (cs_glob_n_ranks == 1) {
    mesh->n_cells = mesh->n_g_cells;
    mesh->n_cells_with_ghosts = mesh->n_cells;
    mesh->domain_num = 1;
  }
  else
    mesh->domain_num = cs_glob_rank_id + 1;
}

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Query or modification of the option for domain partitioning when no
 * partitioning file is present.
 *
 *  0 : query
 *  1 : partition based on initial numbering
 *  2 : partition based on space-filling curve (default)
 *
 * choice <-- of partitioning algorithm.
 *
 * returns:
 *   1 or 2 according to the selected algorithm.
 *----------------------------------------------------------------------------*/

int
cs_preprocessor_data_part_choice(int choice)
{
  int retval = 0;

  if (choice < 0 || choice > 2)
    bft_error(__FILE__, __LINE__,0,
              _("The algorithm selection indicator for domain partitioning\n"
                "can take the following values:\n"
                "  1: partition based on initial numbering\n"
                "  2: partition based on space-filling curve\n"
                "and not %d."), choice);

  if (choice == 1)
    _use_sfc = false;
  else if (choice == 2)
    _use_sfc = true;

  if (_use_sfc == true)
    retval = 2;
  else
    retval = 1;

  return retval;
}

/*----------------------------------------------------------------------------
 * Read pre-processor mesh data and finalize input.
 *
 * parameters:
 *   mesh         <-- pointer to mesh structure
 *   mesh_builder <-- pointer to mesh builder structure
 *----------------------------------------------------------------------------*/

void
cs_preprocessor_data_read_mesh(cs_mesh_t          *mesh,
                               cs_mesh_builder_t  *mesh_builder)
{
  cs_int_t  perio_id, perio_type;
  cs_io_sec_header_t  header;

  cs_real_t  perio_matrix[3][4];

  cs_int_t  perio_num = -1;
  fvm_gnum_t n_elts = 0;
  fvm_gnum_t face_vertices_idx_shift = 0;
  cs_bool_t  end_read = false;
  cs_bool_t  data_read = false;
  long  echo = CS_IO_ECHO_OPEN_CLOSE;
  cs_io_t  *pp_in = cs_glob_pp_io;
  _mesh_reader_t  *mr = _cs_glob_mesh_reader;

  const char  *unexpected_msg = N_("Section of type <%s> on <%s>\n"
                                   "inexpected or of incorrect size.");

  echo = cs_io_get_echo(pp_in);

  _set_block_ranges(mesh, mr);

  /* Loop on sections read */

  while (end_read == false) {

    /* Receive header and clean header name */

    cs_io_read_header(pp_in, &header);

    /* Process according to the header name */

    if (strncmp(header.sec_name, "EOF", CS_IO_NAME_LEN)
        == 0) {
      cs_io_finalize(&pp_in);
      pp_in = NULL;
    }

    if (strncmp(header.sec_name, "start_block:data",
                CS_IO_NAME_LEN) == 0) {

      if (data_read == false)
        data_read = true;
      else
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));

    }
    else if (strncmp(header.sec_name, "end_block:data",
                     CS_IO_NAME_LEN) == 0) {

      if (data_read == true) {
        data_read = false;
        end_read = true;
      }
      else
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));

    }

    /* Read data from the pre-processor output file */

    else if (strncmp(header.sec_name, "face_cells",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mr->n_g_faces * 2;
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_gnum(&header, pp_in);
        if (mr->face_bi.gnum_range[0] > 0)
          n_elts = (mr->face_bi.gnum_range[1] - mr->face_bi.gnum_range[0])*2;
        BFT_MALLOC(mr->face_cells, n_elts, fvm_gnum_t);
        assert(header.n_location_vals == 2);
        cs_io_read_block(&header,
                         mr->face_bi.gnum_range[0],
                         mr->face_bi.gnum_range[1],
                         mr->face_cells, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "cell_group_class_id",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mesh->n_g_cells;
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_lnum(&header, pp_in);
        if (mr->cell_bi.gnum_range[0] > 0)
          n_elts = mr->cell_bi.gnum_range[1] - mr->cell_bi.gnum_range[0];
        BFT_MALLOC(mr->cell_gc_id, n_elts, cs_int_t);
        cs_io_read_block(&header,
                         mr->cell_bi.gnum_range[0],
                         mr->cell_bi.gnum_range[1],
                         mr->cell_gc_id, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "face_group_class_id",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mr->n_g_faces;
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        cs_io_set_fvm_lnum(&header, pp_in);
        if (mr->face_bi.gnum_range[0] > 0)
          n_elts = mr->face_bi.gnum_range[1] - mr->face_bi.gnum_range[0];
        BFT_MALLOC(mr->face_gc_id, n_elts, cs_int_t);
        cs_io_read_block(&header,
                         mr->face_bi.gnum_range[0],
                         mr->face_bi.gnum_range[1],
                         mr->face_gc_id, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "face_vertices_index",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mr->n_g_faces + 1;
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        fvm_gnum_t *_g_face_vertices_idx;
        cs_io_set_fvm_gnum(&header, pp_in);
        if (mr->face_bi.gnum_range[0] > 0)
          n_elts = mr->face_bi.gnum_range[1] - mr->face_bi.gnum_range[0] + 1;
        BFT_MALLOC(mr->face_vertices_idx, n_elts, fvm_lnum_t);
        BFT_MALLOC(_g_face_vertices_idx, n_elts, fvm_gnum_t);
        cs_io_read_index_block(&header,
                               mr->face_bi.gnum_range[0],
                               mr->face_bi.gnum_range[1],
                               _g_face_vertices_idx, pp_in);
        if (n_elts > 0) {
          fvm_gnum_t elt_id;
          face_vertices_idx_shift = _g_face_vertices_idx[0];
          for (elt_id = 0; elt_id < n_elts; elt_id++)
            mr->face_vertices_idx[elt_id]
              = _g_face_vertices_idx[elt_id] - face_vertices_idx_shift;
        }
        BFT_FREE(_g_face_vertices_idx);
      }

    }
    else if (strncmp(header.sec_name, "face_vertices",
                     CS_IO_NAME_LEN) == 0) {

      if (   data_read != true
          || header.n_vals != mr->n_g_face_connect_size)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        size_t n_faces = mr->face_bi.gnum_range[1] - mr->face_bi.gnum_range[0];
        cs_io_set_fvm_gnum(&header, pp_in);
        n_elts =   mr->face_vertices_idx[n_faces]
                 - mr->face_vertices_idx[0];
        BFT_MALLOC(mr->face_vertices, n_elts, fvm_gnum_t);
        cs_io_read_block
          (&header,
           mr->face_vertices_idx[0] + face_vertices_idx_shift,
           mr->face_vertices_idx[n_faces] + face_vertices_idx_shift,
           mr->face_vertices, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "vertex_coords",
                     CS_IO_NAME_LEN) == 0) {

      n_elts = mesh->n_g_vertices * 3;
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        if (mr->vertex_bi.gnum_range[0] > 0)
        cs_io_assert_cs_real(&header, pp_in);
          n_elts = (  mr->vertex_bi.gnum_range[1]
                    - mr->vertex_bi.gnum_range[0])*3;
        BFT_MALLOC(mr->vertex_coords, n_elts, cs_real_t);
        assert(header.n_location_vals == 3);
        cs_io_read_block(&header,
                         mr->vertex_bi.gnum_range[0],
                         mr->vertex_bi.gnum_range[1],
                         mr->vertex_coords, pp_in);
      }

    }

    /* Additional buffers for periodicity */

    else if (strncmp(header.sec_name, "periodicity_type_",
                     strlen("periodicity_type_")) == 0) {

      if (data_read != true || header.n_vals != 1)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        perio_num = atoi(header.sec_name + strlen("periodicity_type_"));
        cs_io_read_global(&header, &perio_type, pp_in);
      }

    }
    else if (strncmp(header.sec_name, "periodicity_matrix_",
                     strlen("periodicity_matrix_")) == 0) {

      n_elts = 12; /* 3x4 */
      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {
        assert(   perio_num
               == atoi(header.sec_name + strlen("periodicity_matrix_")));
        cs_io_assert_cs_real(&header, pp_in);
        cs_io_read_global(&header, perio_matrix, pp_in);

        /* Add a periodicity to mesh->periodicities */

        _add_periodicity(mesh,
                         perio_type,
                         perio_num,
                         perio_matrix);

      }

    }
    else if (strncmp(header.sec_name, "periodicity_faces_",
                     strlen("periodicity_faces_")) == 0) {

      perio_id = atoi(header.sec_name
                      + strlen("periodicity_faces_")) - 1;
      n_elts = mr->n_g_per_face_couples[perio_id] * 2;

      if (data_read != true || header.n_vals != n_elts)
        bft_error(__FILE__, __LINE__, 0,
                  _(unexpected_msg), header.sec_name, cs_io_get_name(pp_in));
      else {

        if ((mr->per_face_bi[perio_id]).gnum_range[0] > 0)
          mr->n_per_face_couples[perio_id]
            = (  (mr->per_face_bi[perio_id]).gnum_range[1]
               - (mr->per_face_bi[perio_id]).gnum_range[0]);
        else
          mr->n_per_face_couples[perio_id] = 0;

        cs_io_set_fvm_gnum(&header, pp_in);
        n_elts = mr->n_per_face_couples[perio_id]*2;
        BFT_MALLOC(mr->per_face_couples[perio_id], n_elts, fvm_gnum_t);
        assert(header.n_location_vals == 2);
        cs_io_read_block(&header,
                         (mr->per_face_bi[perio_id]).gnum_range[0],
                         (mr->per_face_bi[perio_id]).gnum_range[1],
                         mr->per_face_couples[perio_id],
                         pp_in);

      }

    }

  } /* End of loop on messages */

  /* Finalize pre-processor input */
  /*------------------------------*/

  if (cs_glob_pp_io != NULL) {
    cs_io_finalize(&cs_glob_pp_io);
    cs_glob_pp_io = NULL;
  }

  /* Read cell rank data if available */

  if (cs_glob_n_ranks > 1)
    _read_cell_rank(mesh, mr, echo);

  /* Now send data to the correct rank */
  /*-----------------------------------*/

#if defined(HAVE_MPI)

  if (cs_glob_n_ranks > 1)
    _decompose_data_g(mesh,
                      mesh_builder,
                      mr,
                      cs_glob_mpi_comm);

#endif

  if (cs_glob_n_ranks == 1)
    _decompose_data_l(mesh, mesh_builder, mr);

  /* Free temporary memory */

  _mesh_reader_destroy(&_cs_glob_mesh_reader);
  mr = _cs_glob_mesh_reader;
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
