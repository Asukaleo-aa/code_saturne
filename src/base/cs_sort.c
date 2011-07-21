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

/*============================================================================
 * Functions related to in-place sorting of arrays.
 *============================================================================*/

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_sort.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Local structure definitions
 *============================================================================*/

/*=============================================================================
 * Private function definitions
 *============================================================================*/

/*=============================================================================
 * Public function definitions
 *============================================================================*/

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
              cs_lnum_t  a[])
{
  int i, j, h;

  /* Compute stride */
  for (h = 1; h <= (r-l)/9; h = 3*h+1) ;

  /* Sort array */
  for (; h > 0; h /= 3) {

    for (i = l+h; i < r; i++) {

      cs_lnum_t  v = a[i];

      j = i;
      while ((j >= l+h) && (v < a[j-h])) {
        a[j] = a[j-h];
        j -= h;
      }
      a[j] = v;

    } /* Loop on array elements */

  } /* End of loop on stride */

}

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
                   cs_gnum_t  a[])
{
  int i, j, h;

  /* Compute stride */
  for (h = 1; h <= (r-l)/9; h = 3*h+1) ;

  /* Sort array */
  for (; h > 0; h /= 3) {

    for (i = l+h; i < r; i++) {

      cs_gnum_t  v = a[i];

      j = i;
      while ((j >= l+h) && (v < a[j-h])) {
        a[j] = a[j-h];
        j -= h;
      }
      a[j] = v;

    } /* Loop on array elements */

  } /* End of loop on stride */

}

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
cs_sort_coupled_shell(cs_lnum_t   l,
                      cs_lnum_t   r,
                      cs_lnum_t   a[],
                      cs_lnum_t   b[])
{
  int  i, j, h;
  cs_lnum_t  size = r - l;

  if (size == 0)
    return;

  /* Compute stride */
  for (h = 1; h <= size/9; h = 3*h+1) ;

  /* Sort array */
  for ( ; h > 0; h /= 3) {

    for (i = l+h; i < r; i++) {

      cs_lnum_t  va = a[i];
      cs_lnum_t  vb = b[i];

      j = i;
      while ( (j >= l+h) && (va < a[j-h]) ) {
        a[j] = a[j-h];
        b[j] = b[j-h];
        j -= h;
      }
      a[j] = va;
      b[j] = vb;

    } /* Loop on array elements */

  } /* End of loop on stride */

}

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
cs_sort_coupled_gnum_shell(cs_lnum_t   l,
                           cs_lnum_t   r,
                           cs_gnum_t   a[],
                           cs_gnum_t   b[])
{
  int  i, j, h;
  cs_lnum_t  size = r - l;

  if (size == 0)
    return;

  /* Compute stride */
  for (h = 1; h <= size/9; h = 3*h+1) ;

  /* Sort array */
  for ( ; h > 0; h /= 3) {

    for (i = l+h; i < r; i++) {

      cs_gnum_t  va = a[i];
      cs_gnum_t  vb = b[i];

      j = i;
      while ( (j >= l+h) && (va < a[j-h]) ) {
        a[j] = a[j-h];
        b[j] = b[j-h];
        j -= h;
      }
      a[j] = va;
      b[j] = vb;

    } /* Loop on array elements */

  } /* End of loop on stride */

}

/*----------------------------------------------------------------------------*/

END_C_DECLS

