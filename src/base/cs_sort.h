/*============================================================================
 *
 *     This file is part of the Code_Saturne Kernel, element of the
 *     Code_Saturne CFD tool.
 *
 *     Copyright (C) 2008-2009 EDF S.A., France
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
 *===========================================================================*/

#ifndef __CS_SORT_H__
#define __CS_SORT_H__

/*============================================================================
 * Functions related to in-place sorting of arrays.
 *===========================================================================*/

/*----------------------------------------------------------------------------
 * FVM library headers
 *---------------------------------------------------------------------------*/

#include <fvm_defs.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *---------------------------------------------------------------------------*/

#include "cs_base.h"

/*---------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Macro definitions
 *===========================================================================*/

/*============================================================================
 * Type definitions
 *===========================================================================*/

/*=============================================================================
 * Static global variables
 *===========================================================================*/

/*=============================================================================
 * Public function prototypes
 *===========================================================================*/

/*----------------------------------------------------------------------------
 * Sort an array "a" between its left bound "l" and its right bound "r"
 * thanks to a shell sort (Knuth algorithm).
 *
 * parameters:
 *   l <-- left bound
 *   r <-- right bound
 *   a <-> array to sort
 *---------------------------------------------------------------------------*/

void
cs_sort_shell(cs_lnum_t  l,
              cs_lnum_t  r,
              cs_lnum_t  a[]);

/*----------------------------------------------------------------------------
 * Sort a global array "a" between its left bound "l" and its right bound "r"
 * thanks to a shell sort (Knuth algorithm).
 *
 * parameters:
 *   l <-- left bound
 *   r <-- right bound
 *   a <-> array to sort
 *---------------------------------------------------------------------------*/

void
cs_sort_gnum_shell(cs_lnum_t  l,
                   cs_lnum_t  r,
                   cs_gnum_t  a[]);

/*----------------------------------------------------------------------------
 * Sort an array "a" and apply the sort to its associated array "b" (local
 * numbering)
 * Sort is realized thanks to a shell sort (Knuth algorithm).
 *
 * parameters:
 *   l     -->   left bound
 *   r     -->   right bound
 *   a     <->   array to sort
 *   b     <->   associated array
 *---------------------------------------------------------------------------*/

void
cs_sort_coupled_shell(cs_lnum_t  l,
                      cs_lnum_t  r,
                      cs_lnum_t  a[],
                      cs_lnum_t  b[]);

/*----------------------------------------------------------------------------
 * Sort an array "a" and apply the sort to its associated array "b" (local
 * numbering)
 * Sort is realized thanks to a shell sort (Knuth algorithm).
 *
 * parameters:
 *   l     -->   left bound
 *   r     -->   right bound
 *   a     <->   array to sort
 *   b     <->   associated array
 *---------------------------------------------------------------------------*/

void
cs_sort_coupled_gnum_shell(cs_lnum_t  l,
                           cs_lnum_t  r,
                           cs_gnum_t  a[],
                           cs_gnum_t  b[]);

/*---------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_SORT_H__ */
