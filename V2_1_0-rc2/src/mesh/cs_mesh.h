/*============================================================================
 *
 *     This file is part of the Code_Saturne Kernel, element of the
 *     Code_Saturne CFD tool.
 *
 *     Copyright (C) 1998-2011 EDF S.A., France
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

#ifndef __CS_MESH_H__
#define __CS_MESH_H__

/*============================================================================
 * Main structure associated to a mesh
 *============================================================================*/

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_defs.h>
#include <fvm_group.h>
#include <fvm_selector.h>
#include <fvm_interface.h>
#include <fvm_periodicity.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_halo.h"
#include "cs_numbering.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/* Mesh structure definition */
/* ------------------------- */

typedef struct {

  /* General features */

  cs_int_t   dim;                  /* Space dimension */
  cs_int_t   domain_num;           /* Local domain number */
  cs_int_t   n_domains;            /* Number of domains */


  /* Local dimensions */

  cs_int_t   n_cells;              /* Number of cells */
  cs_int_t   n_i_faces;            /* Number of internal faces */
  cs_int_t   n_b_faces;            /* Number of border faces */
  cs_int_t   n_vertices;           /* Number of vertices */

  cs_int_t   i_face_vtx_connect_size;  /* Size of the connectivity
                                          internal faces -> vertices */
  cs_int_t   b_face_vtx_connect_size;  /* Size of the connectivity
                                          border faces -> vertices */

  /* Local structures */

  cs_real_t  *vtx_coord;           /* Vertex coordinates */

  cs_int_t   *i_face_cells;        /* Internal faces -> cells connectivity */
  cs_int_t   *b_face_cells;        /* Border faces -> cells connectivity */

  cs_int_t   *i_face_vtx_idx;      /* Internal faces -> vertices index */
  cs_int_t   *i_face_vtx_lst;      /* Interior faces -> vertices connectivity */

  cs_int_t   *b_face_vtx_idx;      /* Boundary faces -> vertices index */
  cs_int_t   *b_face_vtx_lst;      /* Boundary faces -> vertices connectivity */

  /* Global dimension */

  fvm_gnum_t   n_g_cells;          /* Global number of cells */
  fvm_gnum_t   n_g_i_faces;        /* Global number of internal faces */
  fvm_gnum_t   n_g_b_faces;        /* Global number of border faces */
  fvm_gnum_t   n_g_vertices;       /* Global number of vertices */

  /* Global numbering */

  fvm_gnum_t  *global_cell_num;    /* Global cell numbering */
  fvm_gnum_t  *global_i_face_num;  /* Global internal face numbering */
  fvm_gnum_t  *global_b_face_num;  /* Global border face numbering */
  fvm_gnum_t  *global_vtx_num;     /* Global vertex numbering */

  /* Periodictity features */

  int       n_init_perio;          /* Number of initial periodicities */
  int       n_transforms;          /* Number of transformations */

  int       have_rotation_perio;   /* Periodicity rotation indicator */

  fvm_periodicity_t  *periodicity; /* parameters of each periodicity */

  /* Parallelism and/or periodic features */

  cs_halo_type_t  halo_type;       /* Halo type */

  cs_int_t   n_cells_with_ghosts;  /* Total number of cells on the local rank
                                      (n_cells + n_ghost_cells) */
  cs_int_t   n_ghost_cells;        /* Number of "ghost" cells */

  cs_halo_t  *halo;                /* Structure used to manage ghost cells */

  cs_numbering_t  *i_face_numbering; /* Interior face numbering info */
  cs_numbering_t  *b_face_numbering; /* Boundary face numbering info */

  /* Re-computable connectivity features */

  cs_int_t   n_b_cells;             /* Number of boundary cells */
  cs_int_t  *b_cells;               /* Boundary cell list */

  /* Extended neighborhood features */

  cs_int_t  *cell_cells_idx;   /* "cell -> cells" connectivity index for
                                  extended halo. Only defined if extended
                                  neighborhood is built. */
  cs_int_t  *cell_cells_lst;   /* "cell -> cells" connectivity list for
                                  extended halo. Only defined if extended
                                  neighborhood is built. */

  cs_int_t  *gcell_vtx_idx;    /* ghost cells -> vertices index */
  cs_int_t  *gcell_vtx_lst;    /* ghost cells -> vertices list */

  /* Group and family features */

  cs_int_t    n_groups;    /* Number of groups */
  cs_int_t   *group_idx;   /* Starting index in the in group_lst */
  char       *group_lst;   /* List of group names */

  cs_int_t    n_families;          /* Number of families */
  cs_int_t    n_max_family_items;  /* Max. number of items for one family */
  cs_int_t   *family_item;         /* Family items */
  cs_int_t   *cell_family;         /* Cell family */
  cs_int_t   *i_face_family;       /* Interior face family */
  cs_int_t   *b_face_family;       /* Border face family */

  fvm_group_class_set_t *class_defs;

  fvm_selector_t *select_cells;       /* Cell selection object */
  fvm_selector_t *select_i_faces;     /* Internal faces selection object */
  fvm_selector_t *select_b_faces;     /* Border faces selection object */

  /* Status flags */

  fvm_gnum_t n_g_free_faces;          /* Global number of boundary faces
                                         which are in fact isolated */
  int modified;                       /* Modification status */

} cs_mesh_t;

/* Auxiliary and temporary structure used to build mesh */
/* ---------------------------------------------------- */

typedef struct {

  int           n_perio;              /* Number of periodicities */

  fvm_lnum_t   *n_perio_couples;      /* Local number of periodic face
                                         couples for each periodicity */
  fvm_gnum_t  **perio_couples;        /* List of global numbering of
                                         periodic faces. */

} cs_mesh_builder_t;

/*============================================================================
 * Static global variables
 *============================================================================*/

extern cs_mesh_t *cs_glob_mesh; /* Pointer to main mesh structure */

extern cs_mesh_builder_t  *cs_glob_mesh_builder; /* Pointer to builder mesh
                                                    structure */

/*============================================================================
 *  Public function prototypes for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Return the group number corresponding to a given name. If the group exists,
 * the number corresponds to the group rank (starting from 1) in the list of
 * the meshe's groups, multiplied by -1. This numbering is that used in
 * family (group class) description array IPRFML(NFML, NPRFML).
 *
 * If the group of the given name is not found, 9999 is returned.
 *
 * Fortran interface:
 *
 * FUNCTION NUMGRP (NAME, LEN)
 * ***************
 *
 * CHARACTER*       NAME        : <-- : Name of the group
 * INTEGER          LEN         : <-- : Group name length
 *----------------------------------------------------------------------------*/

cs_int_t CS_PROCF (numgrp, NUMGRP)
(
 const char       *name,    /* <-- Group name */
 const cs_int_t   *len      /* <-- Name length */
 CS_ARGF_SUPP_CHAINE        /*     (possible 'length' arguments added
                                   by many Fortran compilers) */
);

/*----------------------------------------------------------------------------
 * Update a scalar array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine synsca(var)
 * *****************
 *
 * var   : <-> : scalar array
 *----------------------------------------------------------------------------*/

void CS_PROCF(synsca, SYNSCA)
(
 cs_real_t  var[]
);

/*----------------------------------------------------------------------------
 * Update a vector array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine synvec(var)
 * *****************
 *
 * var1   : <-> : vector component 1 array
 * var2   : <-> : vector component 2 array
 * var3   : <-> : vector component 3 array
 *----------------------------------------------------------------------------*/

void CS_PROCF(synvec, SYNVEC)
(
 cs_real_t  var1[],
 cs_real_t  var2[],
 cs_real_t  var3[]
);

/*----------------------------------------------------------------------------
 * Update a vector array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine synvin(var)
 * *****************
 *
 * var   : <-> : interleaved vector (of dimension 3)
 *----------------------------------------------------------------------------*/

void CS_PROCF(synvin, SYNVIN)
(
 cs_real_t  var[]
);

/*----------------------------------------------------------------------------
 * Update a diagonal tensor array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine syndia(var)
 * *****************
 *
 * var11   : <-> : diagonal tensor component 11 array
 * var22   : <-> : diagonal tensor component 22 array
 * var33   : <-> : diagonal tensor component 33 array
 *----------------------------------------------------------------------------*/

void CS_PROCF(syndia, SYNDIA)
(
 cs_real_t  var11[],
 cs_real_t  var22[],
 cs_real_t  var33[]
);

/*----------------------------------------------------------------------------
 * Update a diagonal tensor array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine syndin(var)
 * *****************
 *
 * var   : <-> : interleaved diagonal tensor (of dimension 3)
 *----------------------------------------------------------------------------*/

void CS_PROCF(syndin, SYNDIN)
(
 cs_real_t  var[]
);

/*----------------------------------------------------------------------------
 * Update a tensor array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine synten(var)
 * *****************
 *
 * var11   : <-> : tensor component 11 array
 * var12   : <-> : tensor component 12 array
 * var13   : <-> : tensor component 13 array
 * var21   : <-> : tensor component 21 array
 * var22   : <-> : tensor component 22 array
 * var23   : <-> : tensor component 23 array
 * var31   : <-> : tensor component 31 array
 * var32   : <-> : tensor component 32 array
 * var33   : <-> : tensor component 33 array
 *----------------------------------------------------------------------------*/

void CS_PROCF(synten, SYNTEN)
(
 cs_real_t  var11[],
 cs_real_t  var12[],
 cs_real_t  var13[],
 cs_real_t  var21[],
 cs_real_t  var22[],
 cs_real_t  var23[],
 cs_real_t  var31[],
 cs_real_t  var32[],
 cs_real_t  var33[]
);

/*----------------------------------------------------------------------------
 * Update a tensor array in case of parallelism and/or periodicity.
 *
 * Fortran interface:
 *
 * subroutine syntin(var)
 * *****************
 *
 * var   : <-> : interleaved tensor (of dimension 3x3)
 *----------------------------------------------------------------------------*/

void CS_PROCF(syntin, SYNTIN)
(
 cs_real_t  var[]
);

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create an empty mesh structure
 *
 * returns:
 *   pointer to created mesh structure
 *----------------------------------------------------------------------------*/

cs_mesh_t *
cs_mesh_create(void);

/*----------------------------------------------------------------------------
 * Create an empty mesh builder structure
 *
 * returns:
 *   A pointer to a mesh builder structure.
 *----------------------------------------------------------------------------*/

cs_mesh_builder_t *
cs_mesh_builder_create(void);

/*----------------------------------------------------------------------------
 * Destroy a mesh structure
 *
 * mesh       <->  pointer to a mesh structure
 *
 * returns:
 *   NULL pointer
 *----------------------------------------------------------------------------*/

cs_mesh_t *
cs_mesh_destroy(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Destroy a mesh builder structure
 *
 * mesh_builder <->  pointer to a mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_builder_destroy(cs_mesh_builder_t  **mesh_builder);

/*----------------------------------------------------------------------------
 * Discard free (isolated) faces from a mesh.
 *
 * This should always be done before using the mesh for computation.
 *
 * parameters:
 *   mesh  <->  pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_discard_free_faces(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Renumber vertices.
 *
 * We ensure:
 * If i < j then mesh->global_vtx_num[i] < mesh->global_vtx_num[j]
 * which is not insured by the initial numbering from the pre-processor.
 *
 * parameters:
 *   mesh      <->  pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_order_vertices(cs_mesh_t  *const mesh);

/*----------------------------------------------------------------------------
 * Compute or update mesh structure members the depend on other members,
 * but whose results may be reused, such as global number of elements
 * (cells, vertices, internal and border faces) and sync cell family.
 *
 * parameters:
 *   mesh   <->  pointer to a cs_mesh_t structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_update_auxiliary(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Creation and initialization of halo structures.
 *
 * Treatment of parallel and/or periodic halos for standard and extended
 * ghost cells according to halo type requested by global options.
 *
 * parameters:
 *   mesh  <->  pointer to mesh structure
 *   mb    <->  pointer to mesh builder (in case of periodicity)
 *----------------------------------------------------------------------------*/

void
cs_mesh_init_halo(cs_mesh_t          *mesh,
                  cs_mesh_builder_t  *mb);

/*----------------------------------------------------------------------------
 * Get the global number of ghost cells.
 *
 * parameters:
 *  mesh <--  pointer to a mesh structure
 *
 * returns:
 *  Global number of ghost cells
 *---------------------------------------------------------------------------*/

cs_int_t
cs_mesh_n_g_ghost_cells(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Update a scalar array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var  <->  scalar array
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_scal(cs_real_t  *var);

/*----------------------------------------------------------------------------
 * Update a vector array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var1  <->  vector component 1 array
 *   var2  <->  vector component 2 array
 *   var3  <->  vector component 3 array
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_vect_ni(cs_real_t  *var1,
                         cs_real_t  *var2,
                         cs_real_t  *var3);

/*----------------------------------------------------------------------------
 * Update a vector array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var  <->  interleaved vector (of dimension 3)
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_vect(cs_real_t  *var);

/*----------------------------------------------------------------------------
 * Update a diagonal tensor array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var11  <->  diagonal tensor component 11 array
 *   var22  <->  diagonal tensor component 22 array
 *   var33  <->  diagonal tensor component 33 array
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_diag_ni(cs_real_t  *var11,
                         cs_real_t  *var22,
                         cs_real_t  *var33);

/*----------------------------------------------------------------------------
 * Update a diagonal tensor array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var  <->  diagonal tensor interleaved array
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_diag(cs_real_t  *var);

/*----------------------------------------------------------------------------
 * Update a tensor array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var11  <->  tensor component 11 array
 *   var12  <->  tensor component 12 array
 *   var13  <->  tensor component 13 array
 *   var21  <->  tensor component 21 array
 *   var22  <->  tensor component 22 array
 *   var23  <->  tensor component 23 array
 *   var31  <->  tensor component 31 array
 *   var32  <->  tensor component 32 array
 *   var33  <->  tensor component 33 array
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_tens_ni(cs_real_t  *var11,
                         cs_real_t  *var12,
                         cs_real_t  *var13,
                         cs_real_t  *var21,
                         cs_real_t  *var22,
                         cs_real_t  *var23,
                         cs_real_t  *var31,
                         cs_real_t  *var32,
                         cs_real_t  *var33);

/*----------------------------------------------------------------------------
 * Update a tensor array in case of parallelism and/or periodicity.
 *
 * parameters:
 *   var  <->  interleaved tensor (of dimension 3x3)
 *----------------------------------------------------------------------------*/

void
cs_mesh_sync_var_tens(cs_real_t  *var);

/*----------------------------------------------------------------------------
 * Order family numbers and remove duplicates
 *
 * parameters
 *   mesh <-> pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_clean_families(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Add colors to group class descriptions for group names which are
 * convertible to integers.
 *
 * parameters
 *   mesh <-> pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_build_colors(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Define group classes for a mesh based on its family definitions.
 *
 * parameters:
 *   mesh <-> pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_init_group_classes(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Assign selectors to global mesh.
 *
 * Should be called once the mesh is fully built.
 *----------------------------------------------------------------------------*/

void
cs_mesh_init_selectors(void);

/*----------------------------------------------------------------------------
 * Get global lists of periodic face couples.
 *
 * In parallel, each face couple may appear on only one rank.
 *
 * The caller is responsible for freeing the arrays allocated and returned
 * by this function once they are no onger needed.
 *
 * parameters:
 *   mesh                 <-- pointer to mesh structure
 *   n_perio_face_couples --> global number of periodic couples per
 *                            periodicity (size: mesh->n_init_perio)
 *   perio_face_couples   --> arrays of global periodic couple face numbers,
 *                            for each periodicity
 *----------------------------------------------------------------------------*/

void
cs_mesh_get_perio_faces(const cs_mesh_t    *mesh,
                        fvm_lnum_t        **n_perio_face_couples,
                        fvm_gnum_t       ***perio_face_couples);

/*----------------------------------------------------------------------------
 * Build global cell numbering array extended to ghost cell values.
 *
 * If the blank_perio flag is nonzero, periodic ghost cell numbers
 * are set to zero instead of the value of the matching cell.
 *
 * The caller is responsible for freeing the returned array when it
 * is no longer useful.
 *
 * parameters:
 *   mesh        <-- pointer to mesh structure
 *   blank_perio <-- flag to zeroe periodic cell values
 *----------------------------------------------------------------------------*/

fvm_gnum_t *
cs_mesh_get_cell_gnum(const cs_mesh_t  *mesh,
                      int               blank_perio);

/*----------------------------------------------------------------------------
 * Mark interior faces with the number of their associated periodic
 * transform id.
 *
 * parameters:
 *   mesh      <-- pointer to mesh structure
 *   perio_num --> periodicity number associated with each face, signed for
 *                 direct/reverse transform, 0 for non-periodic faces
 *                 (size: mesh->n_i_faces)
 *----------------------------------------------------------------------------*/

void
cs_mesh_get_face_perio_num(const cs_mesh_t  *mesh,
                           int               perio_num[]);

/*----------------------------------------------------------------------------
 * Print information on a mesh structure.
 *
 * parameters:
 *   mesh  <--  pointer to mesh structure.
 *   name  <--  associated name.
 *----------------------------------------------------------------------------*/

void
cs_mesh_print_info(const cs_mesh_t  *mesh,
                   const char       *name);

/*----------------------------------------------------------------------------
 * Compute global face connectivity size.
 *
 * Faces on simple parallel boundaries are counted only once, but periodic
 * faces are counted twice.
 *
 * parameters:
 *   mesh                   <-- pointer to a cs_mesh_t structure
 *   g_i_face_vertices_size --> global interior face connectivity size, or NULL
 *   g_b_face_vertices_size --> global boundary face connectivity size, or NULL
 *----------------------------------------------------------------------------*/

void
cs_mesh_g_face_vertices_sizes(const cs_mesh_t  *mesh,
                              fvm_gnum_t       *g_i_face_vertices_size,
                              fvm_gnum_t       *g_b_face_vertices_size);

/*----------------------------------------------------------------------------
 * Print statistics about mesh selectors usage to log.
 *
 * parameters:
 *   mesh <-- pointer to a mesh structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_selector_stats(cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------
 * Dump of a mesh structure.
 *
 * parameters:
 *   mesh  <->  pointer to mesh structure.
 *----------------------------------------------------------------------------*/

void
cs_mesh_dump(const cs_mesh_t  *mesh);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_MESH_H__ */
