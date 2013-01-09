/*============================================================================
 * Selection criteria for cells, boundary and interior faces.
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2013 EDF S.A.

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

/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <bft_mem_usage.h>
#include <bft_mem.h>
#include <bft_error.h>
#include <bft_printf.h>

#include "cs_halo.h"
#include "cs_mesh.h"
#include "cs_selector.h"

#include "fvm_selector.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro Definitions
 *============================================================================*/

#define CS_SELECTOR_STR_LEN 127

/*=============================================================================
 * Local Type Definitions
 *============================================================================*/

/*============================================================================
 *  Global variables
 *============================================================================*/

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*============================================================================
 *  Public function definitions for Fortran API
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Build a list of boundary faces verifying a given selection criteria.
 *----------------------------------------------------------------------------*/

void CS_PROCF(csgfbr, CSGFBR)
(
 const char   *const fstr,      /* <-- Fortran string */
 cs_int_t     *const len,       /* <-- String Length  */
 cs_int_t     *const n_faces,   /* --> number of faces */
 cs_int_t     *const face_list  /* --> face list  */
 CS_ARGF_SUPP_CHAINE
)
{
  char _c_string[CS_SELECTOR_STR_LEN + 1];
  char *c_string = _c_string;
  int i;
  int c_len = *len -1;

  /* Initialization */

  *n_faces = 0;

  /* Copy fstr without last blanks  */
  while(fstr[c_len--] == ' ' &&  c_len >= 0);

  if (c_len < -1)
    return;

  c_len += 2;

  if (c_len > CS_SELECTOR_STR_LEN)
    BFT_MALLOC(c_string, c_len + 1, char);

  for(i = 0; i < c_len; i++)
    c_string[i] = fstr[i];
  c_string[c_len] = '\0';

  /* Get faces with C string */

  cs_selector_get_b_face_list(c_string, n_faces, face_list);

  if (c_string != _c_string)
    BFT_FREE(c_string);
}

/*----------------------------------------------------------------------------
 * Build a list of interior faces verifying a given selection criteria.
 *----------------------------------------------------------------------------*/

void CS_PROCF(csgfac, CSGFAC)
(
 const char   *const fstr,      /* <-- Fortran string */
 cs_int_t     *const len,       /* <-- String Length  */
 cs_int_t     *const n_faces,   /* --> number of faces */
 cs_int_t     *const face_list  /* --> face list  */
 CS_ARGF_SUPP_CHAINE
)
{
  char _c_string[CS_SELECTOR_STR_LEN + 1];
  char *c_string = _c_string;
  int i;
  int c_len = *len -1;

  /* Initialization */

  *n_faces = 0;

  /* Copy fstr without last blanks  */
  while(fstr[c_len--] == ' ' &&  c_len >= 0);

  if (c_len < -1)
    return;

  c_len += 2;

  if (c_len > CS_SELECTOR_STR_LEN)
    BFT_MALLOC(c_string, c_len + 1, char);

  for(i = 0; i < c_len; i++)
    c_string[i] = fstr[i];
  c_string[c_len] = '\0';

  /* Get faces with C string */

  cs_selector_get_i_face_list(c_string, n_faces, face_list);

  if (c_string != _c_string)
    BFT_FREE(c_string);
}

/*----------------------------------------------------------------------------
 * Build a list of cells verifying a given selection criteria.
 *----------------------------------------------------------------------------*/

void CS_PROCF(csgcel, CSGCEL)
(
 const char   *const fstr,      /* <-- Fortran string */
 cs_int_t     *const len,       /* <-- String Length  */
 cs_int_t     *const n_cells,   /* --> number of cells */
 cs_int_t     *const cell_list  /* --> cell list  */
 CS_ARGF_SUPP_CHAINE
)
{
  char _c_string[CS_SELECTOR_STR_LEN + 1];
  char *c_string = _c_string;
  int i;
  int c_len = *len -1;

  /* Initialization */

  *n_cells = 0;

  /* Copy fstr without last blanks  */
  while(fstr[c_len--] == ' ' &&  c_len >= 0);

  if (c_len < -1)
    return;

  c_len += 2;

  if (c_len > CS_SELECTOR_STR_LEN)
    BFT_MALLOC(c_string, c_len + 1, char);

  for(i = 0; i < c_len; i++)
    c_string[i] = fstr[i];
  c_string[c_len] = '\0';

  /* Get cells with C string */

  cs_selector_get_cell_list(c_string, n_cells, cell_list);

  if (c_string != _c_string)
    BFT_FREE(c_string);
}

/*----------------------------------------------------------------------------
 * Build a list of cells verifying a given selection criteria.
 *----------------------------------------------------------------------------*/

void CS_PROCF(csgceb, CSGCEB)
(
 const char   *const fstr,        /* <-- Fortran string */
 cs_int_t     *const len,         /* <-- String Length  */
 cs_int_t     *const n_i_faces,   /* --> number of interior faces */
 cs_int_t     *const n_b_faces,   /* --> number of boundary faces */
 cs_int_t     *const i_face_list, /* --> interior face list  */
 cs_int_t     *const b_face_list  /* --> boundary face list  */
 CS_ARGF_SUPP_CHAINE
)
{
  char _c_string[CS_SELECTOR_STR_LEN + 1];
  char *c_string = _c_string;
  int i;
  int c_len = *len -1;

  /* Initialization */

  *n_i_faces = 0;
  *n_b_faces = 0;

  /* Copy fstr without last blanks  */
  while(fstr[c_len--] == ' ' &&  c_len >= 0);

  if (c_len < -1)
    return;

  c_len += 2;

  if (c_len > CS_SELECTOR_STR_LEN)
    BFT_MALLOC(c_string, c_len + 1, char);

  for(i = 0; i < c_len; i++)
    c_string[i] = fstr[i];
  c_string[c_len] = '\0';

  /* Get cells with C string */

  cs_selector_get_cells_boundary(c_string,
                                 n_i_faces,
                                 n_b_faces,
                                 i_face_list,
                                 b_face_list);

  if (c_string != _c_string)
    BFT_FREE(c_string);
}

/*----------------------------------------------------------------------------
 * Build a list of interior faces belonging to a given periodicity.
 *----------------------------------------------------------------------------*/

void CS_PROCF(getfpe, GETFPE)
(
 cs_int_t     *const perio_num, /* <-- Periodicity number */
 cs_int_t     *const n_faces,   /* --> number of faces */
 cs_int_t     *const face_list  /* --> face list  */
 CS_ARGF_SUPP_CHAINE
)
{
  cs_selector_get_perio_face_list(*perio_num,
                                  n_faces,
                                  face_list);
}

/*----------------------------------------------------------------------------
 * Build a list of families verifying a given selection criteria.
 *----------------------------------------------------------------------------*/

void CS_PROCF(csgfam, CSGFAM)
(
 const char   *const fstr,         /* <-- Fortran string */
 cs_int_t     *const len,          /* <-- String Length  */
 cs_int_t     *const n_families,   /* --> number of families */
 cs_int_t     *const family_list   /* --> family list  */
 CS_ARGF_SUPP_CHAINE
)
{
  char _c_string[CS_SELECTOR_STR_LEN + 1];
  char *c_string = _c_string;
  int i;
  int c_len = *len -1;

  /* Initialization */

  *n_families = 0;

  /* Copy fstr without last blanks  */
  while(fstr[c_len--] == ' ' &&  c_len >= 0);

  if (c_len < -1)
    return;

  c_len += 2;

  if (c_len > CS_SELECTOR_STR_LEN)
    BFT_MALLOC(c_string, c_len + 1, char);

  for(i = 0; i < c_len; i++)
    c_string[i] = fstr[i];
  c_string[c_len] = '\0';

  /* Get faces with C string */

  cs_selector_get_family_list(c_string, n_families, family_list);

  if (c_string != _c_string)
    BFT_FREE(c_string);
}

/*=============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Fill a list of boundary faces verifying a given selection criteria.
 *
 * parameters:
 *   criteria    <-- selection criteria string
 *   n_b_faces   --> number of selected interior faces
 *   b_face_list --> list of selected boundary faces
 *                   (1 to n, preallocated to cs_glob_mesh->n_b_faces)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_b_face_list(const char  *criteria,
                            cs_lnum_t   *n_b_faces,
                            cs_lnum_t    b_face_list[])
{
  int c_id;

  *n_b_faces = 0;

  c_id = fvm_selector_get_list(cs_glob_mesh->select_b_faces,
                               criteria,
                               n_b_faces,
                               b_face_list);

  if (fvm_selector_n_missing(cs_glob_mesh->select_b_faces, c_id) > 0) {
    const char *missing
      = fvm_selector_get_missing(cs_glob_mesh->select_b_faces, c_id, 0);
    cs_base_warn(__FILE__, __LINE__);
    bft_printf(_("The group \"%s\" in the selection criteria:\n"
                 "\"%s\"\n"
                 " does not correspond to any boundary face.\n"),
               missing, criteria);
  }
}

/*----------------------------------------------------------------------------
 * Fill a list of interior faces verifying a given selection criteria.
 *
 * parameters:
 *   criteria    <-- selection criteria string
 *   n_i_faces   --> number of selected interior faces
 *   i_face_list --> list of selected interior faces
 *                   (1 to n, preallocated to cs_glob_mesh->n_i_faces)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_i_face_list(const char  *criteria,
                            cs_lnum_t   *n_i_faces,
                            cs_lnum_t    i_face_list[])
{
  int c_id;

  *n_i_faces = 0;

  c_id = fvm_selector_get_list(cs_glob_mesh->select_i_faces,
                               criteria,
                               n_i_faces,
                               i_face_list);

  if (fvm_selector_n_missing(cs_glob_mesh->select_i_faces, c_id) > 0) {
    const char *missing
      = fvm_selector_get_missing(cs_glob_mesh->select_i_faces, c_id, 0);
    cs_base_warn(__FILE__, __LINE__);
    bft_printf(_("The group \"%s\" in the selection criteria:\n"
                 "\"%s\"\n"
                 " does not correspond to any interior face.\n"),
               missing, criteria);
  }
}

/*----------------------------------------------------------------------------
 * Fill a list of cells verifying a given selection criteria.
 *
 * parameters:
 *   criteria  <-- selection criteria string
 *   n_cells   --> number of selected cells
 *   cell_list --> list of selected cells
 *                 (1 to n, preallocated to cs_glob_mesh->n_cells)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_cell_list(const char  *criteria,
                          cs_lnum_t   *n_cells,
                          cs_lnum_t    cell_list[])
{
  int c_id;

  *n_cells = 0;

  c_id = fvm_selector_get_list(cs_glob_mesh->select_cells,
                               criteria,
                               n_cells,
                               cell_list);

  if (fvm_selector_n_missing(cs_glob_mesh->select_cells, c_id) > 0) {
    const char *missing
      = fvm_selector_get_missing(cs_glob_mesh->select_cells, c_id, 0);
    cs_base_warn(__FILE__, __LINE__);
    bft_printf(_("The group \"%s\" in the selection criteria:\n"
                 "\"%s\"\n"
                 " does not correspond to any cell.\n"),
               missing, criteria);
  }
}

/*----------------------------------------------------------------------------
 * Fill lists of faces at the boundary of a set of cells verifying a given
 * selection criteria.
 *
 * parameters:
 *   criteria    <-- selection criteria string
 *   n_i_faces   --> number of selected interior faces
 *   n_b_faces   --> number of selected interior faces
 *   i_face_list --> list of selected interior faces
 *                   (1 to n, preallocated to cs_glob_mesh->n_i_faces)
 *   b_face_list --> list of selected boundary faces
 *                   (1 to n, preallocated to cs_glob_mesh->n_b_faces)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_cells_boundary(const char  *criteria,
                               cs_lnum_t   *n_i_faces,
                               cs_lnum_t   *n_b_faces,
                               cs_lnum_t    i_face_list[],
                               cs_lnum_t    b_face_list[])
{
  cs_lnum_t ii, n_cells;
  cs_lnum_t *cell_list, *cell_flag;

  const cs_mesh_t *mesh = cs_glob_mesh;

  /* Mark cells inside zone selection */

  BFT_MALLOC(cell_list, mesh->n_cells, cs_lnum_t);
  BFT_MALLOC(cell_flag, mesh->n_cells, cs_lnum_t);

  for (ii = 0; ii < mesh->n_cells; ii++)
    cell_flag[ii] = 0;

  n_cells = 0;

  cs_selector_get_cell_list(criteria, &n_cells, cell_list);

  for (ii = 0; ii < n_cells; ii++)
    cell_flag[cell_list[ii] - 1] = 1;

  BFT_FREE(cell_list);

  if (mesh->halo != NULL)
    cs_halo_sync_num(mesh->halo, CS_HALO_STANDARD, cell_flag);

  /* Now build lists of faces on cell boundaries */

  for (ii = 0; ii < mesh->n_i_faces; ii++) {
    cs_lnum_t c_id_0 = mesh->i_face_cells[ii*2] - 1;
    cs_lnum_t c_id_1 = mesh->i_face_cells[ii*2 + 1] - 1;
    if (cell_flag[c_id_0] != cell_flag[c_id_1]) {
      i_face_list[*n_i_faces] = ii + 1;
      *n_i_faces += 1;
    }
  }

  for (ii = 0; ii < mesh->n_b_faces; ii++) {
    cs_lnum_t c_id = mesh->b_face_cells[ii] - 1;
    if (cell_flag[c_id] == 1) {
      b_face_list[*n_b_faces] = ii + 1;
      *n_b_faces += 1;
    }
  }

  BFT_FREE(cell_flag);
}

/*----------------------------------------------------------------------------
 * Fill a list of interior faces belonging to a given periodicity.
 *
 * parameters:
 *   perio_num   <-- periodicity number
 *   n_i_faces   --> number of selected interior faces
 *   i_face_list --> list of selected interior faces
 *                   (1 to n, preallocated to cs_glob_mesh->n_i_faces)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_perio_face_list(int         perio_num,
                                cs_lnum_t  *n_i_faces,
                                cs_lnum_t   i_face_list[])
{
  int ii;
  int *face_perio_num = NULL;

  BFT_MALLOC(face_perio_num, cs_glob_mesh->n_i_faces, int);

  cs_mesh_get_face_perio_num(cs_glob_mesh, face_perio_num);

  *n_i_faces = 0;
  for (ii = 0; ii < cs_glob_mesh->n_i_faces; ii++) {
    if (CS_ABS(face_perio_num[ii]) == perio_num) {
      i_face_list[*n_i_faces] = ii+1;
      *n_i_faces += 1;
    }
  }

  BFT_FREE(face_perio_num);
}

/*----------------------------------------------------------------------------
 * Fill a list of families verifying a given selection criteria.
 *
 * parameters:
 *   criteria    <-- selection criteria string
 *   n_families  --> number of selected interior faces
 *   family_list --> list of selected families faces
 *                   (0 to n, preallocated to cs_glob_mesh->n_families + 1)
 *----------------------------------------------------------------------------*/

void
cs_selector_get_family_list(const char  *criteria,
                            cs_lnum_t   *n_families,
                            cs_int_t     family_list[])
{
  int c_id;

  *n_families = 0;

  /* As all selectors were build with the same group class definitions,
     any selector may be used here. */
  c_id = fvm_selector_get_list(cs_glob_mesh->select_cells,
                               criteria,
                               n_families,
                               family_list);

  if (fvm_selector_n_missing(cs_glob_mesh->select_b_faces, c_id) > 0) {
    const char *missing
      = fvm_selector_get_missing(cs_glob_mesh->select_b_faces, c_id, 0);
    cs_base_warn(__FILE__, __LINE__);
    bft_printf(_("The group \"%s\" in the selection criteria:\n"
                 "\"%s\"\n"
                 " is not present in the mesh.\n"),
               missing, criteria);
  }
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
