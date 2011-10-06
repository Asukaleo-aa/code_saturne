#ifndef __FVM_POINT_LOCATION_H__
#define __FVM_POINT_LOCATION_H__

/*============================================================================
 * Locate local points in a nodal representation associated with a mesh
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

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "fvm_defs.h"
#include "fvm_nodal.h"

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force back Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*=============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/*=============================================================================
 * Static global variables
 *============================================================================*/

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Find elements in a given nodal mesh containing points: updates the
 * location[] and distance[] arrays associated with a set of points
 * for points that are in an element of this mesh, or closer to one
 * than to previously encountered elements.
 *
 * parameters:
 *   this_nodal        <-- pointer to nodal mesh representation structure
 *   tolerance         <-- associated tolerance
 *   locate_on_parents <-- location relative to parent element numbers if 1,
 *                         id of element + 1 in concatenated sections of
 *                         same element dimension if 0
 *   n_points          <-- number of points to locate
 *   point_coords      <-- point coordinates
 *   location          <-> number of element containing or closest to each
 *                         point (size: n_points)
 *   distance          <-> distance from point to element indicated by
 *                         location[]: < 0 if unlocated, 0 - 1 if inside,
 *                         and > 1 if outside a volume element, or absolute
 *                         distance to a surface element (size: n_points)
 *----------------------------------------------------------------------------*/

void
fvm_point_location_nodal(const fvm_nodal_t  *this_nodal,
                         double              tolerance,
                         int                 locate_on_parents,
                         fvm_lnum_t          n_points,
                         const fvm_coord_t   point_coords[],
                         fvm_lnum_t          location[],
                         float               distance[]);

/*----------------------------------------------------------------------------
 * Find elements in a given nodal mesh closest to points: updates the
 * location[] and distance[] arrays associated with a set of points
 * for points that are closer to an element of this mesh than to previously
 * encountered elements.
 *
 * This function currently only handles elements of lower dimension than
 * the spatial dimension.
 *
 * parameters:
 *   this_nodal        <-- pointer to nodal mesh representation structure
 *   locate_on_parents <-- location relative to parent element numbers if 1,
 *                         id of element + 1 in concatenated sections of
 *                         same element dimension if 0
 *   n_points          <-- number of points to locate
 *   point_coords      <-- point coordinates
 *   location          <-> number of element containing or closest to each
 *                         point (size: n_points)
 *   distance          <-> distance from point to element indicated by
 *                         location[]: < 0 if unlocated, or absolute
 *                         distance to a surface element (size: n_points)
 *----------------------------------------------------------------------------*/

void
fvm_point_location_closest_nodal(const fvm_nodal_t  *this_nodal,
                                 int                 locate_on_parents,
                                 fvm_lnum_t          n_points,
                                 const fvm_coord_t   point_coords[],
                                 fvm_lnum_t          location[],
                                 float               distance[]);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FVM_POINT_LOCATION_H__ */
