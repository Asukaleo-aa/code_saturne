/*============================================================================
 *
 *                    Code_Saturne version 1.3
 *                    ------------------------
 *
 *
 *     This file is part of the Code_Saturne Kernel, element of the
 *     Code_Saturne CFD tool.
 *
 *     Copyright (C) 1998-2008 EDF S.A., France
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

#ifndef __CS_MESH_QUANTITIES_H__
#define __CS_MESH_QUANTITIES_H__

/*============================================================================
 * Management of mesh quantities
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_mesh.h"

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definition
 *============================================================================*/

/* Structure associated to mesh quantities management */

typedef struct {

  cs_real_t  *cell_cen;    /* Cell centre coordinates  */
  cs_real_t  *cell_vol;    /* Cell volume */

  cs_real_t  *i_face_normal;  /* Surface normal of internal faces. Norm is the
                              area of the face */
  cs_real_t  *b_face_normal;  /* Surface normal of border faces. Norm is the
                              area of the face */
  cs_real_t  *i_face_cog;   /* Coord. of the centre of gravity for internal faces */
  cs_real_t  *b_face_cog;   /* Coord. of the centre of gravity for border faces */

} cs_mesh_quantities_t ;

/*============================================================================
 * Static global variables
 *============================================================================*/

/* Pointer to mesh quantities structure associated to the main mesh */

extern cs_mesh_quantities_t  *cs_glob_mesh_quantities;

/*============================================================================
 * Public functions definition for API Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Query or modification of the option for computing cell centers.
 *
 * This function returns 1 or 2 according to the selected algorithm.
 *
 * Fortran interface :
 *
 * SUBROUTINE ALGCEN (IOPT)
 * *****************
 *
 * INTEGER          IOPT        : <-> : Choice of the algorithm
 *                                      < 0 : query
 *                                        0 : computation based
 *                                            on faces (default choice)
 *                                        1 : computation based
 *                                            on vertices
 *----------------------------------------------------------------------------*/

void
CS_PROCF (algcen, ALGCEN) (cs_int_t  *const iopt);

/*=============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Query or modification of the option for computing cell centers.
 *
 *  < 0 : query
 *    0 : computation based on faces (default choice)
 *    1 : computation based on vertices
 *
 * algo_choice  -->  choice of algorithm to compute cell centers.
 *
 * returns:
 *  1 or 2 according to the selected algorithm.
 *----------------------------------------------------------------------------*/

int
cs_mesh_quantities_cell_cen_choice(const int algo_choice);

/*----------------------------------------------------------------------------
 * Create a mesh quantities structure.
 *
 * returns:
 *   pointer to created cs_mesh_quantities_t structure
 *----------------------------------------------------------------------------*/

cs_mesh_quantities_t  *
cs_mesh_quantities_create(void);

/*----------------------------------------------------------------------------
 * Destroy a mesh quantities structure
 *
 * parameters:
 *   mesh_quantities --> pointer to a cs_mesh_quantities_t structure
 *
 * returns:
 *  NULL
 *----------------------------------------------------------------------------*/

cs_mesh_quantities_t *
cs_mesh_quantities_destroy(cs_mesh_quantities_t  *mesh_quantities);

/*----------------------------------------------------------------------------
 * Compute mesh quantities
 *
 * parameters:
 *   mesh            --> pointer to a cs_mesh_t structure
 *   mesh_quantities <-> pointer to a cs_mesh_quantities_t structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_quantities_compute(const cs_mesh_t       *mesh,
                           cs_mesh_quantities_t  *mesh_quantities);

/*----------------------------------------------------------------------------
 * Compute internal and border face normal.
 *
 * parameters:
 *   mesh            --> pointer to a cs_mesh_t structure
 *   p_i_face_normal <-> pointer to the internal face normal array
 *   p_b_face_normal <-> pointer to the border face normal array
 *----------------------------------------------------------------------------*/

void
cs_mesh_quantities_face_normal(const cs_mesh_t   *mesh,
                               cs_real_t         *p_i_face_normal[],
                               cs_real_t         *p_b_face_normal[]);

/*----------------------------------------------------------------------------
 * Dump a cs_mesh_quantities_t structure
 *
 * parameters:
 *   mesh            --> pointer to a cs_mesh_t structure
 *   mesh_quantities --> pointer to a cs_mesh_quantities_t structure
 *----------------------------------------------------------------------------*/

void
cs_mesh_quantities_dump(const cs_mesh_t             *mesh,
                        const cs_mesh_quantities_t  *mesh_quantities);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_MESH_QUANTITIES_H__ */
