#ifndef __CS_EQUATION_COMMON_H__
#define __CS_EQUATION_COMMON_H__

/*============================================================================
 * Routines to handle common equation features for building algebraic system
 * in CDO schemes
 *============================================================================*/

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

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_cdo_bc.h"
#include "cs_cdo_connect.h"
#include "cs_cdo_quantities.h"
#include "cs_cdo_time.h"
#include "cs_matrix.h"
#include "cs_time_step.h"
#include "cs_timer.h"
#include "cs_source_term.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

typedef struct {

  /* Monitoring the efficiency of the algorithm used to manipulate/build
     an equation builder. */
  cs_timer_counter_t               tcb; /* Cumulated elapsed time for building
                                           the current system */
  /* tcb >= tcd + tca + tcr + tcs */
  cs_timer_counter_t               tcd; /* Cumulated elapsed time for building
                                           diffusion terms */
  cs_timer_counter_t               tca; /* Cumulated elapsed time for building
                                           advection terms */
  cs_timer_counter_t               tcr; /* Cumulated elapsed time for building
                                           reaction terms */
  cs_timer_counter_t               tcs; /* Cumulated elapsed time for building
                                           source terms */

  cs_timer_counter_t               tce; /* Cumulated elapsed time for computing
                                           all extra operations (post, balance,
                                           fluxes...) */

} cs_equation_monitor_t;

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate a pointer to a buffer of size at least the 2*n_cells for
 *         managing temporary usage of memory when dealing with equations
 *         Call specific structure allocation related to a numerical scheme
 *         according the scheme flag
 *         The size of the temporary buffer can be bigger according to the
 *         numerical settings
 *         Set also shared pointers from the main domain members
 *
 * \param[in]  connect       pointer to a cs_cdo_connect_t structure
 * \param[in]  quant         pointer to additional mesh quantities struct.
 * \param[in]  time_step     pointer to a time step structure
 * \param[in]  scheme_flag   flag to identify which kind of numerical scheme is
 *                           requested to solve the computational domain
 */
/*----------------------------------------------------------------------------*/

void
cs_equation_allocate_common_structures(const cs_cdo_connect_t     *connect,
                                       const cs_cdo_quantities_t  *quant,
                                       const cs_time_step_t       *time_step,
                                       cs_flag_t                   scheme_flag);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate a pointer to a buffer of size at least the 2*n_cells for
 *         managing temporary usage of memory when dealing with equations
 *         Call specific structure allocation related to a numerical scheme
 *         according the scheme flag
 *         The size of the temporary buffer can be bigger according to the
 *         numerical settings
 *
 * \param[in]  scheme_flag   flag to identify which kind of numerical scheme is
 *                           requested to solve the computational domain
 */
/*----------------------------------------------------------------------------*/

void
cs_equation_free_common_structures(cs_flag_t   scheme_flag);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the values of the Dirichlet BCs when DoFs are scalar_valued
 *          and attached to vertices
 *
 * \param[in]      mesh        pointer to a cs_mesh_t structure
 * \param[in]      bc_param    pointer to a cs_param_bc_t structure
 * \param[in]      dir         pointer to a cs_cdo_bc_list_t structure
 * \param[in, out] cb          pointer to a cs_cell_builder_t structure
 *
 * \return a pointer to a new allocated array storing the dirichlet values
 */
/*----------------------------------------------------------------------------*/

cs_real_t *
cs_equation_compute_dirichlet_sv(const cs_mesh_t          *mesh,
                                 const cs_param_bc_t      *bc_param,
                                 const cs_cdo_bc_list_t   *dir,
                                 cs_cell_builder_t        *cb);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Assemble a cellwise system related to cell vertices into the global
 *         algebraic system
 *
 * \param[in]       csys      cellwise view of the algebraic system
 * \param[in]       rset      pointer to a cs_range_set_t structure on vertices
 * \param[in]       sys_flag  flag associated to the current system builder
 * \param[in, out]  rhs       array storing the right-hand side
 * \param[in, out]  sources   array storing the contribution of source terms
 * \param[in, out]  mav       pointer to a matrix assembler structure
 */
/*----------------------------------------------------------------------------*/

void
cs_equation_assemble_v(const cs_cell_sys_t            *csys,
                       const cs_range_set_t           *rset,
                       cs_flag_t                       sys_flag,
                       cs_real_t                      *rhs,
                       cs_real_t                      *sources,
                       cs_matrix_assembler_values_t   *mav);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve a pointer to the associated cs_matrix_structure_t according
 *         to the space scheme
 *
 * \param[in]  scheme       enum on the discretization scheme used
 *
 * \return  a pointer on a cs_matrix_structure_t *
 */
/*----------------------------------------------------------------------------*/

const cs_matrix_structure_t *
cs_equation_get_matrix_structure(cs_space_scheme_t   scheme);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve a pointer to the associated cs_matrix_assembler_t according
 *         to the space scheme
 *
 * \param[in]  scheme       enum on the discretization scheme used
 *
 * \return  a pointer on a cs_matrix_assembler_t *
 */
/*----------------------------------------------------------------------------*/

const cs_matrix_assembler_t *
cs_equation_get_matrix_assembler(cs_space_scheme_t   scheme);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the connectivity vertex->vertices for the local rank
 *
 * \return  a pointer to a cs_connect_index_t structure
 */
/*----------------------------------------------------------------------------*/

const cs_connect_index_t *
cs_equation_get_v2v_index(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the connectivity face->faces for the local rank
 *
 * \return  a pointer to a cs_connect_index_t structure
 */
/*----------------------------------------------------------------------------*/

const cs_connect_index_t *
cs_equation_get_f2f_index(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve a pointer to a buffer of size at least the 2*n_cells
 *         The size of the temporary buffer can be bigger according to the
 *         numerical settings
 *
 * \return  a pointer to an array of double
 */
/*----------------------------------------------------------------------------*/

cs_real_t *
cs_equation_get_tmpbuf(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the allocation size of the temporary buffer
 *
 * \return  the size of the temporary buffer
 */
/*----------------------------------------------------------------------------*/

size_t
cs_equation_get_tmpbuf_size(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Initialize a monitoring structure
 *
 * \return a cs_equation_monitor_t structure
 */
/*----------------------------------------------------------------------------*/

cs_equation_monitor_t *
cs_equation_init_monitoring(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Print a message in the performance output file related to the
 *          monitoring of equation
 *
 * \param[in]  eqname    pointer to the name of the current equation
 * \param[in]  monitor   monitoring structure
 */
/*----------------------------------------------------------------------------*/

void
cs_equation_write_monitoring(const char                    *eqname,
                             const cs_equation_monitor_t   *monitor);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_EQUATION_COMMON_H__ */
