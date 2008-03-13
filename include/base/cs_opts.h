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

#ifndef __CS_OPTS_H__
#define __CS_OPTS_H__

/*============================================================================
 * Parsing of program arguments and associated initializations
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Code_Saturne base options structure
 *----------------------------------------------------------------------------*/

typedef struct {

  /* Communication with Pre-processor */

  cs_int_t         ifoenv;      /* 0 if not using Preprocessor, 1 otherwise */
  cs_int_t         echo_comm;   /* Communication verbosity */

  /* Total sizes for work arrays */

  cs_int_t         longia;      /* Size of integer work arrays */
  cs_int_t         longra;      /* Size of floating point work arrays */

  /* Redirection of standard output */

  cs_int_t         ilisr0;      /* Redirection for rank 0
                                   (0: not redirected;
                                   1: redirected to "listing" file) */
  cs_int_t         ilisrp;      /* Redirection for ranks > 0
                                   (0: not redirected;
                                   1: redirected to "listing_n*" file;
                                   2: redirected to "/dev/null", suppressed) */

  /* Other options */

  cs_int_t       iverif;        /* Mesh quality verification modes
                                   (-1 for standard mode) */

  int            benchmark;   /* Benchmark mode:
                                 0: not used;
                                 1: timing (CPU + Walltime) mode
                                 2: MPI trace-friendly mode */

  /* Cut of warped faces */

  cs_bool_t      cwf;            /* CS_TRUE if cut is required */
  double         cwf_criterion;  /* Criterion to choose which face to cut */

} cs_opts_t;

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Print logfile header
 *
 * parameters:
 *   argc  --> number of command line arguments
 *   argv  --> array of command line arguments
 *----------------------------------------------------------------------------*/

void
cs_opts_logfile_head(int    argc,
                     char  *argv[]);

/*----------------------------------------------------------------------------
 * First analysis of the command line to determine if we require MPI
 *
 * parameters:
 *   argc  <-> number of command line arguments
 *   argv  <-> array of command line arguments
 *
 * returns:
 *   -1 if MPI is not needed, or rank in MPI_COMM_WORLD of the first
 *   process associated with this instance of Code_Saturne
 *----------------------------------------------------------------------------*/

int
cs_opts_mpi_rank(int    * argc,
                 char  **argv[]);

/*----------------------------------------------------------------------------
 * Define options and call some associated initializations
 * based on command line arguments
 *
 * parameters:
 *   argc  --> number of command line arguments
 *   argv  --> array of command line arguments
 *   opts  <-- options structure
 *----------------------------------------------------------------------------*/

void
cs_opts_define(int         argc,
               char       *argv[],
               cs_opts_t  *opts);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_OPTS_H__ */
