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

#ifndef __CS_GUI_VARIABLES_H__
#define __CS_GUI_VARIABLES_H__

/*============================================================================
 * Management of the GUI parameters file: variables
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Type definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Variables and scalars management structure
 *----------------------------------------------------------------------------*/

typedef struct {
  char  *model;           /* predifined physics model                        */
  char  *model_value;     /* predifined physics model value                  */
  char **head;            /* name of the head                                */
  char **type;            /* type of markup: 'variable' or 'scalar'          */
  char **name;            /* variables name and scalars label                */
  char **label;           /* scalars label                                   */
  int   *rtp;             /* variables position in fortran array RTP         */
  int   *rphas;           /* phase number for rtp variables                  */
  int   *pphas;           /* phase number for propce variables               */
  int    nvar;            /* total number of variables and scalars           */
  int    nscaus;          /* number of user scalars                          */
  int    nscapp;          /* number of specific physics scalars              */
  int    nprop;           /* number of properties                            */
  int    nsalpp;          /* number of predifined physics properties         */
  int    nprayc;          /* number of cell's radiative properties           */
  int    nprayb;          /* number of boundary face's radiative properties  */
  int    ntimaver;        /* number of time averages                         */
  char **properties_name; /* label of properties                             */
  int   *properties_ipp;  /* properties position for post-processing         */
  int   *propce;          /* properties position in fortran array PROPCE     */
  char **b_prop_name;     /* label of boundary faces properties              */
  int   *b_prop_ipp;      /* boundary faces prop position for post           */
  int   *propfb;          /* bound faces prop position in fortran array PROPFB */
} cs_var_t;


typedef struct {
  int     _cs_gui_max_vars;
  int     _cs_gui_last_var;
  char  **_cs_gui_var_name;
} cs_label_t;

/*============================================================================
 * Static global variables
 *============================================================================*/

extern cs_var_t    *cs_glob_var;   /* Pointer to main variables structure */
extern cs_label_t  *cs_glob_label; /* Pointer to main label structure */

/*============================================================================
 * Public function prototypes for Fortran API
 *============================================================================*/

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_GUI_VARIABLES_H__ */
