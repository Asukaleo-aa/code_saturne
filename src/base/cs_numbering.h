#ifndef __CS_NUMBERING_H__
#define __CS_NUMBERING_H__

/*============================================================================
 * Numbering information for vectorization or multithreading
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
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_defs.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/* Renumbering types */

typedef enum {

  CS_NUMBERING_VECTORIZE,  /* Numbered for vectorization */
  CS_NUMBERING_THREADS     /* Numbered for threads */

} cs_numbering_type_t;

/* Renumbering structure */

typedef struct {

  cs_numbering_type_t   type; /* Numbering type */

  int   vector_size;          /* Vector size if vectorized, 1 otherwise */

  int   n_threads;            /* Number of threads */
  int   n_groups;             /* Number of associated groups */

  fvm_lnum_t *group_index;   /* For thread t and group g, the start and end
                                ids for entities in a given group and thread
                                are group_index[t*n_groups*2 + g] and
                                group_index[t*n_groups*2 + g + 1] respectively.
                                (size: n_groups * n_threads * 2) */

} cs_numbering_t;

/*=============================================================================
 * Global variable definitions
 *============================================================================*/

/* Names for numbering types */

extern const char  *cs_numbering_type_name[];

/*============================================================================
 * Public function prototypes for Fortran API
 *============================================================================*/

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create a numbering information structure in case of vectorization.
 *
 * parameters:
 *   vector_size <-- vector size used for this vectorization
 *
 * returns:
 *   pointer to created cs_numbering_t structure
 *---------------------------------------------------------------------------*/

cs_numbering_t *
cs_numbering_create_vectorized(int  vector_size);

/*----------------------------------------------------------------------------
 * Create a numbering information structure in case of threading.
 *
 * parameters:
 *   n_threads   <-- number of threads
 *   n_groups    <-- number of groups
 *   group_index <-- group_index[thread_id*group_id*2 + group_id*2] and
 *                   group_index[thread_id*group_id*2 + group_id*2 +1] define
 *                   the start and end ids for entities in a given group and
 *                   thread; (size: n_groups *2 * n_threads)
 *
 * returns:
 *   pointer to created cs_numbering_t structure
 *---------------------------------------------------------------------------*/

cs_numbering_t *
cs_numbering_create_threaded(int         n_threads,
                             int         n_groups,
                             fvm_lnum_t  group_index[]);

/*----------------------------------------------------------------------------
 * Destroy a numbering information structure.
 *
 * parameters:
 *   numbering <-> pointer to cs_numbering_t structure pointer (or NULL)
 *---------------------------------------------------------------------------*/

void
cs_numbering_destroy(cs_numbering_t  **numbering);

/*----------------------------------------------------------------------------
 * Dump a cs_numbering_t structure.
 *
 * parameters:
 *   numbering <-- pointer to cs_numbering_t structure (or NULL)
 *---------------------------------------------------------------------------*/

void
cs_numbering_dump(const cs_numbering_t  *numbering);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_NUMBERING_H__ */
