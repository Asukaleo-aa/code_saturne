/*============================================================================
 * Definition of the calculation mesh.
 *
 * Mesh modification function examples.
 *============================================================================*/

/* VERS */

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
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "bft_error.h"
#include "bft_mem.h"
#include "bft_printf.h"

#include "fvm_defs.h"
#include "fvm_selector.h"

#include "cs_base.h"
#include "cs_mesh_boundary.h"
#include "cs_mesh_group.h"
#include "cs_join.h"
#include "cs_join_perio.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"
#include "cs_mesh_bad_cells.h"
#include "cs_mesh_extrude.h"
#include "cs_mesh_smoother.h"
#include "cs_mesh_warping.h"
#include "cs_parall.h"
#include "cs_post.h"
#include "cs_preprocessor_data.h"
#include "cs_selector.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_prototypes.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*----------------------------------------------------------------------------*/
/*!
 * \file cs_user_mesh.c
 *
 * \brief Mesh modification example.
 *
 * See \subpage cs_user_mesh for examples.
 */
/*----------------------------------------------------------------------------*/

/*============================================================================
 * User function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Modify geometry and mesh.
 *
 * \param[in,out] mesh  pointer to a cs_mesh_t structure
*/
/*----------------------------------------------------------------------------*/

void
cs_user_mesh_modify(cs_mesh_t  *mesh)
{
  /* Example: modify vertex coordinates */
  /*------------------------------------*/

  /* Divide coordinates by 1000 (millimetres to metres).
   *
   * Warning:
   *
   *   This is incompatible with pre-processed periodicity,
   *   as the periodicity transformation is not updated.
   *
   *   With periodicity, using a coordinate transformation matrix
   *   in cs_user_mesh_input is preferred. */

  /*! [mesh_modify_coords] */
  {
    cs_lnum_t  vtx_id;
    const double  coo_mult = 1. / 1000.;

    for (vtx_id = 0; vtx_id < mesh->n_vertices; vtx_id++) {
      mesh->vtx_coord[vtx_id*3]     *= coo_mult;
      mesh->vtx_coord[vtx_id*3 + 1] *= coo_mult;
      mesh->vtx_coord[vtx_id*3 + 2] *= coo_mult;
    }

    /* Set mesh modification flag if it should be saved for future re-use. */

    mesh->modified = 1;
  }
  /*! [mesh_modify_coords] */

  /* Extrude mesh at boundary faces of group "outlet".
     We use a regular extrusion here */

  /*! [mesh_modify_extrude_1] */
  {
    int n_layers = 2;
    double thickness = 1.0;
    double expansion_factor = 1.5;

    const char criteria[] = "outlet";

    /* Select boudary faces */

    cs_lnum_t   n_selected_faces = 0;
    cs_lnum_t  *selected_faces = NULL;

    BFT_MALLOC(selected_faces, mesh->n_b_faces, cs_lnum_t);

    cs_selector_get_b_face_list(criteria,
                                &n_selected_faces,
                                selected_faces);

    /* Extrude selected boundary */

    cs_mesh_extrude_constant(mesh,
                             false, /* Maintain groups on interio faces? */
                             n_layers,
                             thickness,
                             expansion_factor,
                             n_selected_faces,
                             selected_faces);

    /* Free temporary memory */

    BFT_FREE(selected_faces);

  }
  /*! [mesh_modify_extrude_1] */

  /* Add groups to given cells */

  /*! [mesh_modify_extrude_1] */

  /* Add a group to cells in a given region */

  /*! [mesh_modify_groups_1] */
  {
    cs_lnum_t   n_selected_cells = 0;
    cs_lnum_t  *selected_cells = NULL;

    const char criteria[] = "box[0.5, 0.5, 0, 1, 1, 0.05]";

    BFT_MALLOC(selected_cells, mesh->n_cells, cs_lnum_t);

    cs_selector_get_cell_list(criteria,
                              &n_selected_cells,
                              selected_cells);

    cs_mesh_group_cells_add(mesh,
                            "source_region",
                            n_selected_cells,
                            selected_cells);

    BFT_FREE(selected_cells);

    /* Mark mesh as modified to save it */

    mesh->modified = 1;
  }
  /*! [mesh_modify_groups_1] */


}

/*----------------------------------------------------------------------------*/

END_C_DECLS
