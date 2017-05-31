/*============================================================================
 * Build an algebraic CDO vertex-based system for unsteady convection diffusion
 * reaction scalar equations with source terms
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2017 EDF S.A.

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>

#include "cs_cdo_advection.h"
#include "cs_cdo_bc.h"
#include "cs_cdo_diffusion.h"
#include "cs_cdo_local.h"
#include "cs_cdo_scheme_geometry.h"
#include "cs_cdo_time.h"
#include "cs_equation_common.h"
#include "cs_hodge.h"
#include "cs_log.h"
#include "cs_math.h"
#include "cs_mesh_location.h"
#include "cs_param.h"
#include "cs_post.h"
#include "cs_quadrature.h"
#include "cs_reco.h"
#include "cs_search.h"
#include "cs_source_term.h"
#include "cs_timer.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_cdovb_scaleq.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro definitions and structure definitions
 *============================================================================*/

#define CS_CDOVB_SCALEQ_DBG  0

/* Redefined the name of functions from cs_math to get shorter names */
#define _dp3  cs_math_3_dot_product

/* Algebraic system for CDO vertex-based discretization */

struct _cs_cdovb_scaleq_t {

  /* Pointer to a cs_equation_param_t structure shared with a cs_equation_t
     structure.  */
  const cs_equation_param_t  *eqp;

  /* System size */
  cs_lnum_t    n_dofs;

  /* Shortcut to know what to build */
  cs_flag_t     msh_flag;     // Information related to cell mesh (volume)
  cs_flag_t     bd_msh_flag;  // Information related to cell mesh (boundary)
  cs_flag_t     st_msh_flag;  // Information related to cell mesh (source term)
  cs_flag_t     sys_flag;     // Information related to the sytem

  /* Metadata related to associated properties */
  bool          diff_pty_uniform;
  bool          time_pty_uniform;
  bool          reac_pty_uniform[CS_CDO_N_MAX_REACTIONS];

  /* Source terms */
  cs_real_t    *source_terms; /* Array storing the value arising from the
                                 contribution of all source terms */
  cs_mask_t    *source_mask;  /* NULL if at least one source term is not
                                 defined for all cells (size = n_cells) */

  /* Pointer to functions which compute the value of the source term */
  cs_source_term_cellwise_t  *compute_source[CS_N_MAX_SOURCE_TERMS];

  /* Boundary conditions:

     face_bc should not change during the simulation.
     The case of a definition of the BCs which changes of type during the
     simulation is possible but not implemented.
     You just have to call the initialization step each time the type of BCs
     is modified to define an updated cs_cdo_bc_t structure.

  */
  cs_cdo_bc_t  *face_bc;  // Details of the BCs on faces

  /* Pointer of function to build the diffusion term */
  cs_hodge_t                      *get_stiffness_matrix;
  cs_cdo_diffusion_enforce_dir_t  *enforce_dirichlet;
  cs_cdo_diffusion_flux_trace_t   *boundary_flux_op;

  /* Pointer of function to build the advection term */
  cs_cdo_advection_t              *get_advection_matrix;
  cs_cdo_advection_bc_t           *add_advection_bc;

  /* Pointer of function to apply the time scheme */
  cs_cdo_time_scheme_t            *apply_time_scheme;

  /* If one needs to build a local hodge op. for time and reaction */
  cs_param_hodge_t                 hdg_mass;
  cs_hodge_t                      *get_mass_matrix;

  /* Monitoring */
  cs_equation_monitor_t           *monitor;

};

/*============================================================================
 * Private variables
 *============================================================================*/

/* Structure to enable a full cellwise strategy during the system building */
static cs_cell_sys_t      **cs_cdovb_cell_sys = NULL;
static cs_cell_bc_t       **cs_cdovb_cell_bc = NULL;
static cs_cell_builder_t  **cs_cdovb_cell_bld = NULL;

/* Pointer to shared structures (owned by a cs_domain_t structure) */
static const cs_cdo_quantities_t  *cs_shared_quant;
static const cs_cdo_connect_t  *cs_shared_connect;
static const cs_time_step_t  *cs_shared_time_step;

/*============================================================================
 * Private function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Retrieve the flag to give for building a cs_cell_mesh_t structure
 *
 * \param[in]      cell_flag   flag related to the current cell
 * \param[in]      v_msh_flag  default mesh flag for the volumic terms
 * \param[in]      b_msh_flag  default mesh flag for the boundary terms
 * \param[in]      s_msh_flag  default mesh flag for the source terms
 *
 * \return the flag to set for the current cell
 */
/*----------------------------------------------------------------------------*/

static inline cs_flag_t
_get_cell_mesh_flag(cs_flag_t       cell_flag,
                    cs_flag_t       v_msh_flag,
                    cs_flag_t       b_msh_flag,
                    cs_flag_t       s_msh_flag)
{
  cs_flag_t  msh_flag = v_msh_flag | s_msh_flag;

  if (cell_flag & CS_FLAG_BOUNDARY)
    msh_flag |= b_msh_flag;

  return msh_flag;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Retrieve the list of vertices attached to a face
 *
 * \param[in]       f       face id in the cell numbering
 * \param[in]       cm      pointer to a cs_cell_mesh_t structure
 * \param[in, out]  n_vf    pointer of pointer to a cellwise view of the mesh
 * \param[in, out]  v_ids   list of vertex ids in the cell numbering
 */
/*----------------------------------------------------------------------------*/

static inline void
_get_f2v(short int                    f,
         const cs_cell_mesh_t        *cm,
         short int                   *n_vf,
         short int                   *v_ids)
{
  /* Reset */
  *n_vf = 0;
  for (short int v = 0; v < cm->n_vc; v++)
    v_ids[v] = -1;

  /* Tag vertices belonging to the current face f */
  for (short int i = cm->f2e_idx[f]; i < cm->f2e_idx[f+1]; i++) {

    const short int  shift_e = 2*cm->f2e_ids[i];
    v_ids[cm->e2v_ids[shift_e]] = 1;
    v_ids[cm->e2v_ids[shift_e+1]] = 1;

  } // Loop on face edges

  for (short int v = 0; v < cm->n_vc; v++) {
    if (v_ids[v] > 0)
      v_ids[*n_vf] = v, *n_vf += 1;
  }

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Initialize the local structure for the current cell
 *
 * \param[in]      cell_flag   flag related to the current cell
 * \param[in]      cm          pointer to a cellwise view of the mesh
 * \param[in]      b           pointer to a cs_cdovb_scaleq_t structure
 * \param[in]      dir_values  Dirichlet values associated to each vertex
 * \param[in]      field_tn    values of the field at the last computed time
 * \param[in, out] csys        pointer to a cellwise view of the system
 * \param[in, out] cbc         pointer to a cellwise view of the BCs
 * \param[in, out] cb          pointer to a cellwise builder
 */
/*----------------------------------------------------------------------------*/

static void
_init_cell_structures(const cs_flag_t            cell_flag,
                      const cs_cell_mesh_t      *cm,
                      const cs_cdovb_scaleq_t   *b,
                      const cs_real_t            dir_values[],
                      const cs_real_t            field_tn[],
                      cs_cell_sys_t             *csys,
                      cs_cell_bc_t              *cbc,
                      cs_cell_builder_t         *cb)
{
  const cs_cdo_connect_t  *connect = cs_shared_connect;
  const cs_cdo_bc_t  *face_bc = b->face_bc;

  /* Cell-wise view of the linear system to build */
  const int  n_vc = cm->n_vc;

  /* Initialize the local system */
  csys->n_dofs = n_vc;
  csys->mat->n_ent = n_vc;
  for (short int v = 0; v < n_vc; v++) {
    csys->mat->ids[v] = cm->v_ids[v];
    csys->val_n[v] = field_tn[cm->v_ids[v]];
    csys->rhs[v] = csys->source[v] = 0.;
  }
  for (short int v = 0; v < n_vc*n_vc; v++) csys->mat->val[v] = 0;

  /* Store the local values attached to Dirichlet values if the current cell
     has at least one border face */
  if (cell_flag & CS_FLAG_BOUNDARY) {

    /* Sanity check */
    assert(cs_test_flag(cm->flag, CS_CDO_LOCAL_EV | CS_CDO_LOCAL_FE));

    /* Reset values */
    cbc->n_bc_faces = 0;
    cbc->n_dirichlet = cbc->n_nhmg_neuman = cbc->n_robin = 0;
    cbc->n_dofs = cm->n_vc;
    for (short int i = 0; i < cm->n_vc; i++) {
      cbc->dof_flag[i] = 0;
      cbc->dir_values[i] = cbc->neu_values[i] = 0;
      cbc->rob_values[2*i] = cbc->rob_values[2*i+1] = 0.;
    }

    /* Identify which face is a boundary face */
    short int  n_vf;
    for (short int f = 0; f < cm->n_fc; f++) {

      const cs_lnum_t  f_id = cm->f_ids[f] - connect->n_faces[2]; // n_i_faces
      if (f_id > -1) { // Border face

        const cs_flag_t  face_flag = face_bc->flag[f_id];

        cbc->face_flag[cbc->n_bc_faces] = face_flag;
        cbc->bf_ids[cbc->n_bc_faces++] = f;

        _get_f2v(f, cm, &n_vf, cb->ids);

        if (face_flag & CS_CDO_BC_HMG_DIRICHLET) {

          for (short int i = 0; i < n_vf; i++)
            cbc->dof_flag[cb->ids[i]] |= CS_CDO_BC_HMG_DIRICHLET;

        }
        else if (face_flag & CS_CDO_BC_DIRICHLET) {

          for (short int i = 0; i < n_vf; i++) {
            short int  v = cb->ids[i];
            cbc->dir_values[v] = dir_values[cm->v_ids[v]];
            cbc->dof_flag[v] |= CS_CDO_BC_DIRICHLET;
          }

        }

      } // Border faces

    } // Loop on cell faces

    /* Update counters */
    for (short int v = 0; v < cm->n_vc; v++) {

      if (cbc->dof_flag[v] & CS_CDO_BC_HMG_DIRICHLET ||
          cbc->dof_flag[v] & CS_CDO_BC_DIRICHLET)
        cbc->n_dirichlet += 1;

      if (cbc->dof_flag[v] & CS_CDO_BC_NEUMANN)
        cbc->n_nhmg_neuman += 1;

      if (cbc->dof_flag[v] & CS_CDO_BC_ROBIN)
        cbc->n_robin += 1;

    } // Loop on cell vertices

#if defined(DEBUG) && !defined(NDEBUG) /* Sanity check */
    for (short int v = 0; v < cm->n_vc; v++) {
      if (cbc->dof_flag[v] & CS_CDO_BC_HMG_DIRICHLET)
        if (fabs(cbc->dir_values[v]) > 10*DBL_MIN)
          bft_error(__FILE__, __LINE__, 0,
                    "Invalid enforcement of Dirichlet BCs on vertices");
    }
#endif

  } /* Border cell */

}

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set shared pointers from the main domain members
 *
 * \param[in]  quant       additional mesh quantities struct.
 * \param[in]  connect     pointer to a cs_cdo_connect_t struct.
 * \param[in]  time_step   pointer to a time step structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_set_shared_pointers(const cs_cdo_quantities_t    *quant,
                                    const cs_cdo_connect_t       *connect,
                                    const cs_time_step_t         *time_step)
{
  /* Assign static const pointers */
  cs_shared_quant = quant;
  cs_shared_connect = connect;
  cs_shared_time_step = time_step;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate work buffer and general structures related to CDO
 *         vertex-based schemes
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_initialize(void)
{
  const cs_cdo_connect_t  *connect = cs_shared_connect;

  /* Structure used to build the final system by a cell-wise process */
  assert(cs_glob_n_threads > 0);  /* Sanity check */

  BFT_MALLOC(cs_cdovb_cell_sys, cs_glob_n_threads, cs_cell_sys_t *);
  BFT_MALLOC(cs_cdovb_cell_bc, cs_glob_n_threads, cs_cell_bc_t *);
  BFT_MALLOC(cs_cdovb_cell_bld, cs_glob_n_threads, cs_cell_builder_t *);

  for (int i = 0; i < cs_glob_n_threads; i++) {
    cs_cdovb_cell_sys[i] = NULL;
    cs_cdovb_cell_bc[i] = NULL;
    cs_cdovb_cell_bld[i] = NULL;
  }

#if defined(HAVE_OPENMP) /* Determine default number of OpenMP threads */
#pragma omp parallel
  {
    int t_id = omp_get_thread_num();
    assert(t_id < cs_glob_n_threads);

    cs_cdovb_cell_sys[t_id] = cs_cell_sys_create(connect->n_max_vbyc);

    cs_cdovb_cell_bc[t_id] = cs_cell_bc_create(connect->n_max_vbyc, // n_dofbyc
                                               connect->n_max_fbyc);

    cs_cdovb_cell_bld[t_id] = cs_cell_builder_create(CS_SPACE_SCHEME_CDOVB,
                                                     connect);
  }
#else
  assert(cs_glob_n_threads == 1);
  cs_cdovb_cell_sys[0] = cs_cell_sys_create(connect->n_max_vbyc);

  cs_cdovb_cell_bc[0] = cs_cell_bc_create(connect->n_max_vbyc, // n_dofbyc
                                          connect->n_max_fbyc);

  cs_cdovb_cell_bld[0] = cs_cell_builder_create(CS_SPACE_SCHEME_CDOVB,
                                                connect);

#endif /* openMP */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free work buffer and general structure related to CDO vertex-based
 *         schemes
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_finalize(void)
{
#if defined(HAVE_OPENMP) /* Determine default number of OpenMP threads */
#pragma omp parallel
  {
    int t_id = omp_get_thread_num();
    cs_cell_sys_free(&(cs_cdovb_cell_sys[t_id]));
    cs_cell_bc_free(&(cs_cdovb_cell_bc[t_id]));
    cs_cell_builder_free(&(cs_cdovb_cell_bld[t_id]));
  }
#else
  assert(cs_glob_n_threads == 1);
  cs_cell_sys_free(&(cs_cdovb_cell_sys[0]));
  cs_cell_bc_free(&(cs_cdovb_cell_bc[0]));
  cs_cell_builder_free(&(cs_cdovb_cell_bld[0]));
#endif /* openMP */

  BFT_FREE(cs_cdovb_cell_sys);
  BFT_FREE(cs_cdovb_cell_bc);
  BFT_FREE(cs_cdovb_cell_bld);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Initialize a cs_cdovb_scaleq_t builder structure
 *
 * \param[in] eqp       pointer to a cs_equation_param_t structure
 * \param[in] mesh      pointer to a cs_mesh_t structure
 *
 * \return a pointer to a new allocated cs_cdovb_scaleq_t structure
 */
/*----------------------------------------------------------------------------*/

void  *
cs_cdovb_scaleq_init(const cs_equation_param_t   *eqp,
                     const cs_mesh_t             *mesh)
{
  CS_UNUSED(mesh);

  /* Sanity checks */
  assert(eqp != NULL);

  if (eqp->space_scheme != CS_SPACE_SCHEME_CDOVB &&
      eqp->var_type != CS_PARAM_VAR_SCAL)
    bft_error(__FILE__, __LINE__, 0,
              " Invalid type of equation.\n"
              " Expected: scalar-valued CDO vertex-based equation.");

  const cs_cdo_connect_t  *connect = cs_shared_connect;
  const cs_lnum_t  n_vertices = connect->n_vertices;
  const cs_lnum_t  n_b_faces = connect->n_faces[1];
  const cs_param_bc_t  *bc_param = eqp->bc;

  cs_cdovb_scaleq_t  *b = NULL;

  BFT_MALLOC(b, 1, cs_cdovb_scaleq_t);

  b->n_dofs = n_vertices;

  /* Shared pointers */
  b->eqp = eqp;

  /* Store a direct access to which term one has to compute
     High-level information on how to build the current system */
  b->sys_flag = 0;
  if (eqp->verbosity > 100)
    b->sys_flag = CS_FLAG_SYS_DEBUG;

  /* Flag to indicate the minimal set of quantities to build in a cell mesh
     According to the situation, additional flags have to be set */
  b->msh_flag = CS_CDO_LOCAL_PV | CS_CDO_LOCAL_PVQ | CS_CDO_LOCAL_PE |
    CS_CDO_LOCAL_EV;

  /* Store additional flags useful for building boundary operator.
     Only activated on boundary cells */
  b->bd_msh_flag = CS_CDO_LOCAL_PF | CS_CDO_LOCAL_FE;

  /* Set members and structures related to the management of the BCs

     Translate user-defined information about BC into a structure well-suited
     for computation. We make the distinction between homogeneous and
     non-homogeneous BCs.
     We also compute also the list of Dirichlet vertices along with their
     related definition.  */
  b->face_bc = cs_cdo_bc_define(bc_param, n_b_faces);

  /* Diffusion part */
  /* -------------- */

  b->diff_pty_uniform = true;
  b->get_stiffness_matrix = NULL;
  b->boundary_flux_op = NULL;
  b->enforce_dirichlet = NULL;

  if (eqp->flag & CS_EQUATION_DIFFUSION) {

    b->sys_flag |= CS_FLAG_SYS_DIFFUSION;
    b->diff_pty_uniform = cs_property_is_uniform(eqp->diffusion_property);

    switch (eqp->diffusion_hodge.algo) {

    case CS_PARAM_HODGE_ALGO_COST:
      b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ;
      b->get_stiffness_matrix = cs_hodge_vb_cost_get_stiffness;
      b->boundary_flux_op = cs_cdovb_diffusion_cost_flux_op;
      break;

    case CS_PARAM_HODGE_ALGO_VORONOI:
      b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ;
      b->get_stiffness_matrix = cs_hodge_vb_voro_get_stiffness;
      b->boundary_flux_op = cs_cdovb_diffusion_cost_flux_op;
      break;

    case CS_PARAM_HODGE_ALGO_WBS:
      b->msh_flag |= CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_PEQ |
        CS_CDO_LOCAL_FEQ | CS_CDO_LOCAL_HFQ;
      b->get_stiffness_matrix = cs_hodge_vb_wbs_get_stiffness;
      b->boundary_flux_op = cs_cdovb_diffusion_wbs_flux_op;
      break;

    default:
      bft_error(__FILE__, __LINE__, 0,
                (" Invalid type of algorithm to build the diffusion term."));

    } // Switch on Hodge algo.

    switch (bc_param->enforcement) {

    case CS_PARAM_BC_ENFORCE_WEAK_PENA:
      b->enforce_dirichlet = cs_cdovb_diffusion_pena_dirichlet;
      break;

    case CS_PARAM_BC_ENFORCE_WEAK_NITSCHE:
      b->bd_msh_flag |= CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_FEQ;
      b->enforce_dirichlet = cs_cdovb_diffusion_weak_dirichlet;
      break;

    case CS_PARAM_BC_ENFORCE_WEAK_SYM:
      b->bd_msh_flag |= CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_FEQ;
      b->enforce_dirichlet = cs_cdovb_diffusion_wsym_dirichlet;
      break;

    default:
      bft_error(__FILE__, __LINE__, 0,
                (" Invalid type of algorithm to enforce Dirichlet BC."));

    }

  } // Has diffusion

  /* Advection part */
  /* -------------- */

  b->get_advection_matrix = NULL;
  b->add_advection_bc = NULL;

  if (eqp->flag & CS_EQUATION_CONVECTION) {

    b->sys_flag |= CS_FLAG_SYS_ADVECTION;

    const cs_param_advection_t  a_info = eqp->advection_info;
    cs_param_def_type_t  adv_deftype =
      cs_advection_field_get_deftype(eqp->advection_field);

    if (adv_deftype == CS_PARAM_DEF_BY_VALUE)
      b->msh_flag |= CS_CDO_LOCAL_DFQ;
    else if (adv_deftype == CS_PARAM_DEF_BY_ARRAY)
      b->msh_flag |= CS_CDO_LOCAL_PEQ;
    else if (adv_deftype == CS_PARAM_DEF_BY_ANALYTIC_FUNCTION)
      b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_EFQ | CS_CDO_LOCAL_PFQ;

    switch (a_info.formulation) {

    case CS_PARAM_ADVECTION_FORM_CONSERV:

      switch (a_info.scheme) {

      case CS_PARAM_ADVECTION_SCHEME_CENTERED:
        b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ;
        b->get_advection_matrix = cs_cdo_advection_get_vb_cencsv;
        break;

      case CS_PARAM_ADVECTION_SCHEME_UPWIND:
      case CS_PARAM_ADVECTION_SCHEME_SAMARSKII:
      case CS_PARAM_ADVECTION_SCHEME_SG:
        if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {
          b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ;
          b->get_advection_matrix = cs_cdo_advection_get_vb_upwcsvdi;
        }
        else {
          b->msh_flag |= CS_CDO_LOCAL_DFQ;
          b->get_advection_matrix = cs_cdo_advection_get_vb_upwcsv;
        }
        break;

      default:
        bft_error(__FILE__, __LINE__, 0,
                  " Invalid advection scheme for vertex-based discretization");
      } // Scheme
      break; // Formulation

    case CS_PARAM_ADVECTION_FORM_NONCONS:

      switch (a_info.scheme) {
      case CS_PARAM_ADVECTION_SCHEME_CENTERED:
        b->get_advection_matrix = cs_cdo_advection_get_vb_cennoc;
        break;

      case CS_PARAM_ADVECTION_SCHEME_UPWIND:
      case CS_PARAM_ADVECTION_SCHEME_SAMARSKII:
      case CS_PARAM_ADVECTION_SCHEME_SG:
        if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {
          b->msh_flag |= CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ;
          b->get_advection_matrix = cs_cdo_advection_get_vb_upwnocdi;
        }
        else {
          b->msh_flag |= CS_CDO_LOCAL_DFQ;
          b->get_advection_matrix = cs_cdo_advection_get_vb_upwnoc;
        }
        break;

      default:
        bft_error(__FILE__, __LINE__, 0,
                  " Invalid advection scheme for vertex-based discretization");
      } // Scheme
      break; // Formulation

    default:
      bft_error(__FILE__, __LINE__, 0,
                " Invalid type of formulation for the advection term");
    }

    /* Boundary conditions for advection */
    const cs_adv_field_t  *adv_field = eqp->advection_field;
    b->bd_msh_flag |= CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_FEQ;
    if (cs_advection_field_is_cellwise(adv_field))
      b->add_advection_bc = cs_cdo_advection_add_vb_bc_cw;
    else
      b->add_advection_bc = cs_cdo_advection_add_vb_bc;

  }
  else {

    if (bc_param->enforcement != CS_PARAM_BC_ENFORCE_WEAK_NITSCHE)
      b->sys_flag |= CS_FLAG_SYS_SYM; // Algebraic system is symmetric

  }

  /* Reaction part */
  /* ------------- */

  if (eqp->n_reaction_terms > CS_CDO_N_MAX_REACTIONS)
    bft_error(__FILE__, __LINE__, 0,
              " Number of reaction terms for an equation is too high.\n"
              " Modify your settings aor contact the developpement team.");

  for (int i = 0; i < CS_CDO_N_MAX_REACTIONS; i++)
    b->reac_pty_uniform[i] = true;

  if (eqp->flag & CS_EQUATION_REACTION) {

    b->sys_flag |= CS_FLAG_SYS_REACTION;

    for (int i = 0; i < eqp->n_reaction_terms; i++)
      b->reac_pty_uniform[i]
        = cs_property_is_uniform(eqp->reaction_properties[i]);

    if (eqp->reaction_hodge.algo == CS_PARAM_HODGE_ALGO_WBS) {
      b->msh_flag |= CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_FEQ |
        CS_CDO_LOCAL_HFQ;
      b->sys_flag |= CS_FLAG_SYS_HLOC_CONF;
    }
    else
      bft_error(__FILE__, __LINE__, 0,
                " Invalid choice of algorithm for the reaction term.");

  } /* Reaction */

  /* Time part */
  /* --------- */

  b->time_pty_uniform = true;
  b->apply_time_scheme = NULL;

  if (eqp->flag & CS_EQUATION_UNSTEADY) {

    b->sys_flag |= CS_FLAG_SYS_TIME;

    b->time_pty_uniform = cs_property_is_uniform(eqp->time_property);

    if (eqp->time_hodge.algo == CS_PARAM_HODGE_ALGO_VORONOI) {
      b->sys_flag |= CS_FLAG_SYS_TIME_DIAG;
    }
    else if (eqp->time_hodge.algo == CS_PARAM_HODGE_ALGO_WBS) {
      if (eqp->time_info.do_lumping) {
        b->sys_flag |= CS_FLAG_SYS_TIME_DIAG;
      }
      else {
        b->msh_flag |= CS_CDO_LOCAL_PVQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_PFQ |
          CS_CDO_LOCAL_FEQ | CS_CDO_LOCAL_HFQ;
        b->sys_flag |= CS_FLAG_SYS_HLOC_CONF;
      }
    }

    b->apply_time_scheme = cs_cdo_time_get_scheme_function(b->sys_flag,
                                                           eqp->time_info);

  } /* Time part */

  /* Source term part */
  /* ---------------- */

  b->st_msh_flag = 0;
  b->source_terms = NULL;
  b->source_mask = NULL;
  for (int i = 0; i < CS_N_MAX_SOURCE_TERMS; i++)
    b->compute_source[i] = NULL;

  if (eqp->n_source_terms > 0) {

    b->sys_flag |= CS_FLAG_SYS_SOURCETERM;

    b->st_msh_flag = cs_source_term_init(CS_SPACE_SCHEME_CDOVB,
                                         eqp->n_source_terms,
                                         eqp->source_terms,
                                         b->compute_source,
                                         &(b->sys_flag),
                                         &(b->source_mask));

    BFT_MALLOC(b->source_terms, b->n_dofs, cs_real_t);
#pragma omp parallel for if (b->n_dofs > CS_THR_MIN)
    for (cs_lnum_t i = 0; i < b->n_dofs; i++)
      b->source_terms[i] = 0;

  } /* There is at least one source term */

  /* Pre-defined a cs_hodge_builder_t structure */
  b->hdg_mass.is_unity = true;
  b->hdg_mass.is_iso   = true;
  b->hdg_mass.inv_pty  = false;
  b->hdg_mass.type = CS_PARAM_HODGE_TYPE_VPCD;
  b->hdg_mass.algo = CS_PARAM_HODGE_ALGO_WBS;
  b->hdg_mass.coef = 1.0; // not useful in this case

  b->get_mass_matrix = cs_hodge_vpcd_wbs_get;

  /* Monitoring */
  b->monitor = cs_equation_init_monitoring();

  return b;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Destroy a cs_cdovb_scaleq_t structure
 *
 * \param[in, out]  builder   pointer to a cs_cdovb_scaleq_t structure
 *
 * \return a NULL pointer
 */
/*----------------------------------------------------------------------------*/

void *
cs_cdovb_scaleq_free(void   *builder)
{
  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t *)builder;

  if (b == NULL)
    return b;

  /* eqp is only shared. Thies structure is freed later. */

  if (b->sys_flag & CS_FLAG_SYS_SOURCETERM) {
    BFT_FREE(b->source_terms);
    BFT_FREE(b->source_mask);
  }

  /* Free BC structure */
  b->face_bc = cs_cdo_bc_free(b->face_bc);

  /* Monitoring structure */
  BFT_FREE(b->monitor);

  /* Last free */
  BFT_FREE(b);

  return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Display information related to the monitoring of the current system
 *
 * \param[in]  eqname    name of the related equation
 * \param[in]  builder   pointer to a cs_cdovb_scaleq_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_monitor(const char   *eqname,
                        const void   *builder)
{
  const cs_cdovb_scaleq_t  *b = (const cs_cdovb_scaleq_t *)builder;

  if (b == NULL)
    return;

  cs_equation_write_monitoring(eqname, b->monitor);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the contributions of source terms (store inside builder)
 *
 * \param[in, out]  builder     pointer to a cs_cdovb_scaleq_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_compute_source(void   *builder)
{
  if (builder == NULL)
    return;

  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t *)builder;

  const cs_equation_param_t  *eqp = b->eqp;
  const cs_cdo_connect_t  *connect = cs_shared_connect;
  const cs_cdo_quantities_t  *quant = cs_shared_quant;

  if (eqp->n_source_terms == 0)
    return;

  cs_timer_t  t0 = cs_timer_time();

  /* Compute the source term cell by cell */
#pragma omp parallel if (quant->n_cells > CS_THR_MIN) default(none)     \
  shared(quant, connect, eqp, b, cs_cdovb_cell_sys, cs_cdovb_cell_bld)
  {
#if defined(HAVE_OPENMP) /* Determine default number of OpenMP threads */
    int  t_id = omp_get_thread_num();
#else
    int  t_id = 0;
#endif

    cs_cell_mesh_t  *cm = cs_cdo_local_get_cell_mesh(t_id);
    cs_cell_sys_t  *csys = cs_cdovb_cell_sys[t_id];
    cs_cell_builder_t  *cb = cs_cdovb_cell_bld[t_id];
    cs_flag_t  msh_flag = b->st_msh_flag;

#if defined(DEBUG) && !defined(NDEBUG)
    cs_cell_mesh_reset(cm);
#endif

    if (b->sys_flag & CS_FLAG_SYS_SOURCETERM) {
      /* Reset source term array */
#pragma omp for CS_CDO_OMP_SCHEDULE
      for (cs_lnum_t i = 0; i < b->n_dofs; i++)
        b->source_terms[i] = 0;
    }

#pragma omp for CS_CDO_OMP_SCHEDULE
    for (cs_lnum_t  c_id = 0; c_id < quant->n_cells; c_id++) {

      /* Set the local mesh structure for the current cell */
      cs_cell_mesh_build(c_id, msh_flag, connect, quant, cm);

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 2
      if (b->sys_flag & CS_FLAG_SYS_DEBUG)
        if (c_id % (eqp->verbosity - 100) == 0) cs_cell_mesh_dump(cm);
#endif

      /* Build the local dense matrix related to this operator
         Store in cb->hdg inside the cs_cell_builder_t structure */
      if (b->sys_flag & CS_FLAG_SYS_SOURCES_HLOC)
        b->get_mass_matrix(b->hdg_mass, cm, cb);

      /* Initialize the local number of DoFs */
      csys->n_dofs = cm->n_vc;

      /* Compute the contribution of all source terms in each cell */
      cs_source_term_compute_cellwise(eqp->n_source_terms,
                                      eqp->source_terms,
                                      cm,
                                      b->sys_flag,
                                      b->source_mask,
                                      b->compute_source,
                                      cb,    // mass matrix is stored in cb->hdg
                                      csys); // Fill csys->source

      /* Assemble the cellwise contribution to the rank contribution */
      for (short int v = 0; v < cm->n_vc; v++)
#       pragma omp atomic
        b->source_terms[cm->v_ids[v]] += csys->source[v];

    } // Loop on cells

  } // OpenMP block

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 2
  if (b->sys_flag & CS_FLAG_SYS_DEBUG)
    cs_dump_array_to_listing("INIT_SOURCE_TERM_VTX", quant->n_vertices,
                             b->source_terms, 8);
#endif

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tcs), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Create the matrix of the current algebraic system.
 *         Allocate and initialize the right-hand side associated to the given
 *         builder structure
 *
 * \param[in, out] builder        pointer to generic builder structure
 * \param[in, out] system_matrix  pointer of pointer to a cs_matrix_t struct.
 * \param[in, out] system_rhs     pointer of pointer to an array of double
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_initialize_system(void            *builder,
                                  cs_matrix_t    **system_matrix,
                                  cs_real_t      **system_rhs)
{
  if (builder == NULL)
    return;
  assert(*system_matrix == NULL && *system_rhs == NULL);

  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t *)builder;
  cs_timer_t  t0 = cs_timer_time();

  /* Create the matrix related to the current algebraic system */
  const cs_matrix_structure_t  *ms =
    cs_equation_get_matrix_structure(CS_SPACE_SCHEME_CDOVB);

  *system_matrix = cs_matrix_create(ms);

  /* Allocate and initialize the related right-hand side */
  BFT_MALLOC(*system_rhs, b->n_dofs, cs_real_t);
#pragma omp parallel for if  (b->n_dofs > CS_THR_MIN)
  for (cs_lnum_t i = 0; i < b->n_dofs; i++) (*system_rhs)[i] = 0.0;

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tcb), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Build the linear system arising from a scalar convection/diffusion
 *         equation with a CDO vertex-based scheme.
 *         One works cellwise and then process to the assembly
 *
 * \param[in]      mesh       pointer to a cs_mesh_t structure
 * \param[in]      field_val  pointer to the current value of the field
 * \param[in]      dt_cur     current value of the time step
 * \param[in, out] builder    pointer to cs_cdovb_scaleq_t structure
 * \param[in, out] rhs        right-hand side to compute
 * \param[in, out] matrix     pointer to cs_matrix_t structure to compute
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_build_system(const cs_mesh_t        *mesh,
                             const cs_real_t        *field_val,
                             double                  dt_cur,
                             void                   *builder,
                             cs_real_t              *rhs,
                             cs_matrix_t            *matrix)
{
  /* Sanity checks */
  assert(rhs != NULL && matrix != NULL);

  const cs_cdo_quantities_t  *quant = cs_shared_quant;
  const cs_cdo_connect_t  *connect = cs_shared_connect;

  cs_timer_t  t0 = cs_timer_time();

  /* Initialize the structure to assemble values */
  cs_matrix_assembler_values_t  *mav =
    cs_matrix_assembler_values_init(matrix, NULL, NULL);

  /* Compute the values of the Dirichlet BC.
     TODO: do the analogy for Neumann BC */
  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t *)builder;
  cs_real_t  *dir_values =
    cs_equation_compute_dirichlet_sv(mesh,
                                     b->eqp->bc,
                                     b->face_bc->dir,
                                     cs_cdovb_cell_bld[0]);

  /* Update rhs with the previous computation of source term if needed */
  if (b->sys_flag & CS_FLAG_SYS_SOURCETERM) {
    if (b->sys_flag & CS_FLAG_SYS_TIME) {
      cs_timer_t  ta = cs_timer_time();

      cs_cdo_time_update_rhs_with_array(b->sys_flag,
                                        b->eqp->time_info,
                                        b->n_dofs,
                                        b->source_terms,
                                        rhs);

      cs_timer_t  tb = cs_timer_time();
      cs_timer_counter_add_diff(&(b->monitor->tcs), &ta, &tb);
    }
  }

#pragma omp parallel if (quant->n_cells > CS_THR_MIN) default(none)     \
  shared(dt_cur, quant, connect, b, rhs, matrix, mav, dir_values, field_val, \
         cs_cdovb_cell_sys, cs_cdovb_cell_bld, cs_cdovb_cell_bc)
  {
#if defined(HAVE_OPENMP) /* Determine default number of OpenMP threads */
    int  t_id = omp_get_thread_num();
#else
    int  t_id = 0;
#endif

    const cs_equation_param_t  *eqp = b->eqp;

    /* Each thread get back its related structures:
       Get the cell-wise view of the mesh and the algebraic system */
    cs_face_mesh_t  *fm = cs_cdo_local_get_face_mesh(t_id);
    cs_cell_mesh_t  *cm = cs_cdo_local_get_cell_mesh(t_id);
    cs_cell_sys_t  *csys = cs_cdovb_cell_sys[t_id];
    cs_cell_bc_t  *cbc = cs_cdovb_cell_bc[t_id];
    cs_cell_builder_t  *cb = cs_cdovb_cell_bld[t_id];

    /* Set inside the OMP section so that each thread has its own value */

    /* Initialization of the values of properties */
    double  time_pty_val = 1.0;
    double  reac_pty_vals[CS_CDO_N_MAX_REACTIONS];
    for (int i = 0; i < CS_CDO_N_MAX_REACTIONS; i++) reac_pty_vals[i] = 1.0;

    /* Initialize members of the builder related to the current system
       Preparatory step for diffusion term */
    if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {

      if (b->diff_pty_uniform) {

        cs_property_get_cell_tensor(0, // cell_id
                                    eqp->diffusion_property,
                                    eqp->diffusion_hodge.inv_pty,
                                    cb->pty_mat);

        if (eqp->diffusion_hodge.is_iso)
          cb->pty_val = cb->pty_mat[0][0];

        if (eqp->bc->enforcement == CS_PARAM_BC_ENFORCE_WEAK_NITSCHE ||
            eqp->bc->enforcement == CS_PARAM_BC_ENFORCE_WEAK_SYM)
          cs_math_33_eigen((const cs_real_t (*)[3])cb->pty_mat,
                           &(cb->eig_ratio),
                           &(cb->eig_max));

      } /* Diffusion property is uniform */

    } /* Diffusion */

    /* Preparatory step for unsteady term */
    if (b->sys_flag & CS_FLAG_SYS_TIME)
      if (b->time_pty_uniform)
        time_pty_val = cs_property_get_cell_value(0, eqp->time_property);

    /* Preparatory step for reaction term */
    if (b->sys_flag & CS_FLAG_SYS_REACTION) {

      for (int r = 0; r < eqp->n_reaction_terms; r++) {
        if (b->reac_pty_uniform[r]) {
          cs_property_t  *r_pty = eqp->reaction_properties[r];
          reac_pty_vals[r] = cs_property_get_cell_value(0, r_pty);
        }
      } // Loop on reaction properties

    } // Reaction properties

    /* --------------------------------------------- */
    /* Main loop on cells to build the linear system */
    /* --------------------------------------------- */

#pragma omp for CS_CDO_OMP_SCHEDULE
    for (cs_lnum_t c_id = 0; c_id < quant->n_cells; c_id++) {

      const cs_flag_t  cell_flag = connect->cell_flag[c_id];
      const cs_flag_t  msh_flag = _get_cell_mesh_flag(cell_flag,
                                                      b->msh_flag,
                                                      b->bd_msh_flag,
                                                      b->st_msh_flag);

      /* Set the local mesh structure for the current cell */
      cs_cell_mesh_build(c_id, msh_flag, connect, quant, cm);

      /* Set the local (i.e. cellwise) structures for the current cell */
      _init_cell_structures(cell_flag, cm, b, dir_values, field_val, // in
                            csys, cbc, cb);                          // out

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 2
      if (b->sys_flag & CS_FLAG_SYS_DEBUG)
        if (c_id % (eqp->verbosity - 100) == 0) cs_cell_mesh_dump(cm);
#endif

      /* DIFFUSION CONTRIBUTION TO THE ALGEBRAIC SYSTEM */
      /* ============================================== */

      if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {

        /* Define the local stiffness matrix */
        if (!(b->diff_pty_uniform)) {

          cs_property_tensor_in_cell(cm,
                                     eqp->diffusion_property,
                                     eqp->diffusion_hodge.inv_pty,
                                     cb->pty_mat);

          if (eqp->diffusion_hodge.is_iso)
            cb->pty_val = cb->pty_mat[0][0];

        }

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 2
        if (!eqp->diffusion_hodge.is_iso) {
          if (c_id % 100 == 0) {
            bft_printf(" [% 6.3e % 6.3e % 6.3e]\n"
                       " [% 6.3e % 6.3e % 6.3e]\n"
                       " [% 6.3e % 6.3e % 6.3e]\n",
                       cb->pty_mat[0][0], cb->pty_mat[0][1], cb->pty_mat[0][2],
                       cb->pty_mat[1][0], cb->pty_mat[1][1], cb->pty_mat[1][2],
                       cb->pty_mat[2][0], cb->pty_mat[2][1], cb->pty_mat[2][2]);
          }
        }
#endif

        // local matrix owned by the cellwise builder (store in cb->loc)
        b->get_stiffness_matrix(eqp->diffusion_hodge, cm, cb);

        // Add the local diffusion operator to the local system
        cs_locmat_add(csys->mat, cb->loc);

        /* Weakly enforced Dirichlet BCs for cells attached to the boundary
           csys is updated inside (matrix and rhs) */
        if (cell_flag & CS_FLAG_BOUNDARY) {

          if (eqp->bc->enforcement == CS_PARAM_BC_ENFORCE_WEAK_NITSCHE ||
              eqp->bc->enforcement == CS_PARAM_BC_ENFORCE_WEAK_SYM)
            cs_math_33_eigen((const cs_real_t (*)[3])cb->pty_mat,
                             &(cb->eig_ratio),
                             &(cb->eig_max));

          b->enforce_dirichlet(eqp->diffusion_hodge,
                               cbc,
                               cm,
                               b->boundary_flux_op,
                               fm, cb, csys);

        }

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 1
        if (b->sys_flag & CS_FLAG_SYS_DEBUG)
          if (c_id % (eqp->verbosity - 100) == 0)
            cs_cell_sys_dump("\n>> Local system after diffusion", c_id, csys);
#endif
      } /* END OF DIFFUSION */

      /* ADVECTION CONTRIBUTION TO THE ALGEBRAIC SYSTEM */
      /* ============================================== */

      if (b->sys_flag & CS_FLAG_SYS_ADVECTION) {

        /* Define the local advection matrix */
        b->get_advection_matrix(eqp, cm, fm, cb);

        cs_locmat_add(csys->mat, cb->loc);

        /* Last treatment for the advection term: Apply boundary conditions
           csys is updated inside (matrix and rhs) */
        if (cell_flag & CS_FLAG_BOUNDARY)
          b->add_advection_bc(cbc, cm, eqp, fm, cb, csys);

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 1
        if (b->sys_flag & CS_FLAG_SYS_DEBUG)
          if (c_id % (eqp->verbosity - 100) == 0)
            cs_cell_sys_dump("\n>> Local system after advection", c_id, csys);
#endif

      } /* END OF ADVECTION */

      if (b->sys_flag & CS_FLAG_SYS_HLOC_CONF) {
        b->get_mass_matrix(b->hdg_mass, cm, cb); // stored in cb->hdg

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 0
        if (b->sys_flag & CS_FLAG_SYS_DEBUG) {
          if (c_id % (eqp->verbosity - 100) == 0) {
            cs_log_printf(CS_LOG_DEFAULT, ">> Local mass matrix");
            cs_locmat_dump(c_id, cb->hdg);
          }
        }
#endif
      }

      /* REACTION CONTRIBUTION TO THE ALGEBRAIC SYSTEM */
      /* ============================================= */

      if (b->sys_flag & CS_FLAG_SYS_REACTION) {

        /* Define the local reaction property */
        double  rpty_val = 0;
        for (int r = 0; r < eqp->n_reaction_terms; r++)
          if (b->reac_pty_uniform[r])
            rpty_val += reac_pty_vals[r];
          else
            rpty_val += cs_property_value_in_cell(cm,
                                                  eqp->reaction_properties[r]);

        /* Update local system matrix with the reaction term
           cb->hdg corresponds to the current mass matrix */
        cs_locmat_mult_add(csys->mat, rpty_val, cb->hdg);

      } /* END OF REACTION */

      /* SOURCE TERM COMPUTATION */
      /* ======================= */

      if (b->sys_flag & CS_FLAG_SYS_SOURCETERM) {

        /* Source term contribution to the algebraic system
           If the equation is steady, the source term has already been computed
           and is added to the right-hand side during its initialization. */
        cs_source_term_compute_cellwise(eqp->n_source_terms,
                                        eqp->source_terms,
                                        cm,
                                        b->sys_flag,
                                        b->source_mask,
                                        b->compute_source,
                                        cb,    // mass matrix is cb->hdg
                                        csys); // Fill csys->source

        if ((b->sys_flag & CS_FLAG_SYS_TIME) == 0) { // Steady-state case
          /* Same strategy as if one applies an implicit scheme */
          for (short int v = 0; v < cm->n_vc; v++)
            csys->rhs[v] += csys->source[v];
        }

      } /* End of term source contribution */

      /* TIME CONTRIBUTION TO THE ALGEBRAIC SYSTEM */
      /* ========================================= */

      if (b->sys_flag & CS_FLAG_SYS_TIME) {

        /* Get the value of the time property */
        double  tpty_val = 1/dt_cur;
        if (b->time_pty_uniform)
          tpty_val *= time_pty_val;
        else
          tpty_val *= cs_property_value_in_cell(cm, eqp->time_property);

        cs_locmat_t  *mass_mat = cb->hdg;
        if (b->sys_flag & CS_FLAG_SYS_TIME_DIAG) {

          assert(cs_test_flag(b->msh_flag, CS_CDO_LOCAL_PVQ));
          /* Switch to cb->loc. Used as a diagonal only */
          mass_mat = cb->loc;

          /* |c|*wvc = |dual_cell(v) cap c| */
          const double  ptyc = tpty_val * cm->vol_c;
          for (short int v = 0; v < cm->n_vc; v++)
            mass_mat->val[v] = ptyc * cm->wvc[v];

        }

        /* Apply the time discretization to the local system.
           Update csys (matrix and rhs) */
        b->apply_time_scheme(eqp->time_info, tpty_val, mass_mat, b->sys_flag,
                             cb, csys);

      } /* END OF TIME CONTRIBUTION */

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 0
      if (b->sys_flag & CS_FLAG_SYS_DEBUG)
        if (c_id % (eqp->verbosity - 100) == 0)
          cs_cell_sys_dump(">> (FINAL) Local system matrix", c_id, csys);
#endif

      /* Assemble the local system to the global system */
      cs_equation_assemble_v(csys, connect->v_rs, b->sys_flag, // in
                             rhs, b->source_terms, mav);       // out

    } // Main loop on cells

  } // OPENMP Block

  cs_matrix_assembler_values_done(mav); // optional

  /* Free temporary buffers and structures */
  BFT_FREE(dir_values);
  cs_matrix_assembler_values_finalize(&mav);

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 2
  if (b->source_terms != NULL && (b->sys_flag & CS_FLAG_SYS_DEBUG))
    cs_dump_array_to_listing("EQ.BUILD >> TS", b->n_dofs, b->source_terms, 8);
#endif

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tcb), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Store solution(s) of the linear system into a field structure
 *         Update extra-field values if required (for hybrid discretization)
 *
 * \param[in]      solu       solution array
 * \param[in]      rhs        rhs associated to this solution array
 * \param[in, out] builder    pointer to cs_cdovb_scaleq_t structure
 * \param[in, out] field_val  pointer to the current value of the field
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_update_field(const cs_real_t     *solu,
                             const cs_real_t     *rhs,
                             void                *builder,
                             cs_real_t           *field_val)
{
  CS_UNUSED(rhs);

  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t  *)builder;
  cs_timer_t  t0 = cs_timer_time();

  /* Set computed solution in field array */
# pragma omp parallel for if (b->n_dofs > CS_THR_MIN)
  for (cs_lnum_t i = 0; i < b->n_dofs; i++)
    field_val[i] = solu[i];

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tce), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the diffusive and convective flux across a list of faces
 *
 * \param[in]       direction  indicate in which direction flux is > 0
 * \param[in]       pdi        pointer to an array of field values
 * \param[in]       ml_id      id related to a cs_mesh_location_t struct.
 * \param[in, out]  builder    pointer to a builder structure
 * \param[in, out]  diff_flux  pointer to the value of the diffusive flux
 * \param[in, out]  conv_flux  pointer to the value of the convective flux
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_compute_flux_across_plane(const cs_real_t     direction[],
                                          const cs_real_t    *pdi,
                                          int                 ml_id,
                                          void               *builder,
                                          double             *diff_flux,
                                          double             *conv_flux)
{
  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t  *)builder;
  cs_mesh_location_type_t  ml_t = cs_mesh_location_get_type(ml_id);

  *diff_flux = 0.;
  *conv_flux = 0.;

  if (pdi == NULL)
    return;

  if (ml_t != CS_MESH_LOCATION_INTERIOR_FACES &&
      ml_t != CS_MESH_LOCATION_BOUNDARY_FACES) {
    cs_base_warn(__FILE__, __LINE__);
    cs_log_printf(CS_LOG_DEFAULT,
                  _(" Mesh location type is incompatible with the computation\n"
                    " of the flux across faces.\n"));
    return;
  }

  const cs_timer_t  t0 = cs_timer_time();
  const cs_equation_param_t  *eqp = b->eqp;
  const cs_lnum_t  *n_elts = cs_mesh_location_get_n_elts(ml_id);
  const cs_lnum_t  *elt_ids = cs_mesh_location_get_elt_list(ml_id);

  if (n_elts[0] > 0 && elt_ids == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _(" Computing the flux across all interior or border faces is not"
                " managed yet."));

  const cs_cdo_connect_t  *connect = cs_shared_connect;
  const cs_sla_matrix_t  *f2c = connect->f2c;
  const cs_cdo_quantities_t  *quant = cs_shared_quant;

  double  pf;
  cs_real_3_t  gc, pty_gc;
  cs_real_33_t  pty_tens;

  if (ml_t == CS_MESH_LOCATION_BOUNDARY_FACES) { // Belongs to only one cell

    const cs_lnum_t  n_i_faces = connect->n_faces[2];
    const cs_lnum_t  *cell_ids = f2c->col_id + f2c->idx[n_i_faces];

    for (cs_lnum_t i = 0; i < n_elts[0]; i++) {

      const cs_lnum_t  bf_id = elt_ids[i];
      const cs_lnum_t  f_id = n_i_faces + bf_id;
      const cs_lnum_t  c_id = cell_ids[bf_id];
      const cs_quant_t  f = quant->face[f_id];
      const short int  sgn = (_dp3(f.unitv, direction) < 0) ? -1 : 1;
      const double  coef = sgn * f.meas;

      if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {

        /* Compute the local diffusive flux */
        cs_reco_grd_cell_from_pv(c_id, connect, quant, pdi, gc);
        cs_property_get_cell_tensor(c_id,
                                    eqp->diffusion_property,
                                    eqp->diffusion_hodge.inv_pty,
                                    pty_tens);
        cs_math_33_3_product((const cs_real_t (*)[3])pty_tens, gc, pty_gc);

        /* Update the diffusive flux */
        *diff_flux += -coef * _dp3(f.unitv, pty_gc);

      }

      if (b->sys_flag & CS_FLAG_SYS_ADVECTION) {

        cs_nvec3_t  adv_c;

        /* Compute the local advective flux */
        cs_advection_field_get_cell_vector(c_id, eqp->advection_field, &adv_c);
        cs_reco_pf_from_pv(f_id, connect, quant, pdi, &pf);

        /* Update the convective flux */
        *conv_flux += coef * adv_c.meas * _dp3(adv_c.unitv, f.unitv) * pf;

      }

    } // Loop on selected border faces

  }
  else if (ml_t == CS_MESH_LOCATION_INTERIOR_FACES) {

    for (cs_lnum_t i = 0; i < n_elts[0]; i++) {

      const cs_lnum_t  f_id = elt_ids[i];
      const cs_quant_t  f = quant->face[f_id];
      const short int  sgn = (_dp3(f.unitv, direction) < 0) ? -1 : 1;

      for (cs_lnum_t j = f2c->idx[f_id]; j < f2c->idx[f_id+1]; j++) {

        const cs_lnum_t  c_id = f2c->col_id[j];

        if (b->sys_flag & CS_FLAG_SYS_DIFFUSION) {

          const double  coef = 0.5 * sgn * f.meas; // mean value at the face

          /* Compute the local diffusive flux */
          cs_reco_grd_cell_from_pv(c_id, connect, quant, pdi, gc);
          cs_property_get_cell_tensor(c_id,
                                      eqp->diffusion_property,
                                      eqp->diffusion_hodge.inv_pty,
                                      pty_tens);
          cs_math_33_3_product((const cs_real_t (*)[3])pty_tens, gc, pty_gc);
          *diff_flux += -coef * _dp3(f.unitv, pty_gc);

        }

        if (b->sys_flag & CS_FLAG_SYS_ADVECTION) {

          cs_nvec3_t  adv_c;

          /* Compute the local advective flux */
          cs_reco_pf_from_pv(f_id, connect, quant, pdi, &pf);

          /* Evaluate the advection field at the face */
          cs_advection_field_get_cell_vector(c_id, eqp->advection_field,
                                             &adv_c);

          /* Update the convective flux (upwinding according to adv_f) */
          const double  dpc = _dp3(adv_c.unitv, f.unitv);

          cs_real_t  fconv_flux = 0;
          if (dpc > 0) {
            if (f2c->sgn[j] > 0) // nf points outward c; adv.nf > 0
              fconv_flux = adv_c.meas * dpc * sgn * f.meas * pf;
          }
          else if (dpc < 0) {
            if (f2c->sgn[j] < 0) // nf points inward c; adv.nf < 0
              fconv_flux = adv_c.meas * dpc * sgn * f.meas * pf;
          }
          else  // centered approach
            fconv_flux = 0.5 * adv_c.meas * dpc * sgn * f.meas * pf;

          *conv_flux += fconv_flux;

        }

      }

    } // Loop on selected interior faces

  } // Set of interior or border faces

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tce), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Cellwise computation of the diffusive flux
 *
 * \param[in]       values      discrete values for the potential
 * \param[in, out]  builder     pointer to builder structure
 * \param[in, out]  location    where the flux is defined
 * \param[in, out]  diff_flux   value of the diffusive flux
  */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_cellwise_diff_flux(const cs_real_t   *values,
                                   void              *builder,
                                   cs_flag_t          location,
                                   cs_real_t         *diff_flux)
{
  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t  *)builder;

  const cs_equation_param_t  *eqp = b->eqp;
  const cs_cdo_quantities_t  *quant = cs_shared_quant;
  const cs_cdo_connect_t  *connect = cs_shared_connect;

  /* Sanity checks */
  assert(diff_flux != NULL);

  if (!cs_test_flag(location, cs_cdo_primal_cell) &&
      !cs_test_flag(location, cs_cdo_dual_face_byc))
    bft_error(__FILE__, __LINE__, 0,
              "Incompatible location.\n"
              " Stop computing a cellwise diffusive flux.");

  if ((b->sys_flag & CS_FLAG_SYS_DIFFUSION) == 0) { // No diffusion

    size_t  size = 0;
    if (cs_test_flag(location, cs_cdo_primal_cell))
      size = 3*quant->n_cells;
    else if (cs_test_flag(location, cs_cdo_dual_face_byc))
      size = connect->c2e->idx[quant->n_cells];

#   pragma omp parallel for if (size > CS_THR_MIN)
    for (size_t i = 0; i < size; i++)
      diff_flux[i] = 0;

    return;

  }

  cs_timer_t  t0 = cs_timer_time();

#pragma omp parallel if (quant->n_cells > CS_THR_MIN) default(none)     \
  shared(quant, connect, location, eqp, b, diff_flux, values, cs_cdovb_cell_bld)
  {
#if defined(HAVE_OPENMP) /* Determine default number of OpenMP threads */
    int  t_id = omp_get_thread_num();
#else
    int  t_id = 0;
#endif
    double  *pot = NULL;

    /* Each thread get back its related structures:
       Get the cellwise view of the mesh and the algebraic system */
    cs_cell_mesh_t  *cm = cs_cdo_local_get_cell_mesh(t_id);
    cs_cell_builder_t  *cb = cs_cdovb_cell_bld[t_id];
    cs_flag_t  msh_flag = 0;
    cs_hodge_t  *get_diffusion_hodge = NULL;
    cs_cdo_cellwise_diffusion_flux_t  *compute_flux = NULL;

#if defined(DEBUG) && !defined(NDEBUG)
    cs_cell_mesh_reset(cm);
#endif

    switch (eqp->diffusion_hodge.algo) {

    case CS_PARAM_HODGE_ALGO_COST:
      BFT_MALLOC(pot, connect->n_max_vbyc, double);

      msh_flag = CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ | CS_CDO_LOCAL_EV |
        CS_CDO_LOCAL_PVQ;

      /* Set function pointers */
      if (cs_test_flag(location, cs_cdo_primal_cell)) {
        compute_flux = cs_cdo_diffusion_vcost_get_pc_flux;
      }
      else if (cs_test_flag(location, cs_cdo_dual_face_byc)) {
        get_diffusion_hodge = cs_hodge_epfd_cost_get;
        compute_flux = cs_cdo_diffusion_vcost_get_dfbyc_flux;
      }

      break;

    case CS_PARAM_HODGE_ALGO_VORONOI:
      BFT_MALLOC(pot, connect->n_max_vbyc, double);

      /* Set function pointers */
      get_diffusion_hodge = cs_hodge_epfd_voro_get;
      if (cs_test_flag(location, cs_cdo_primal_cell))
        compute_flux = cs_cdo_diffusion_vcost_get_pc_flux;
      else if (cs_test_flag(location, cs_cdo_dual_face_byc))
        compute_flux = cs_cdo_diffusion_vcost_get_dfbyc_flux;

      msh_flag = CS_CDO_LOCAL_PEQ | CS_CDO_LOCAL_DFQ | CS_CDO_LOCAL_EV |
        CS_CDO_LOCAL_EFQ | CS_CDO_LOCAL_PVQ;
      break;

    case CS_PARAM_HODGE_ALGO_WBS:
      BFT_MALLOC(pot, connect->n_max_vbyc + 1, double);

      msh_flag = CS_CDO_LOCAL_PV | CS_CDO_LOCAL_PVQ | CS_CDO_LOCAL_PEQ |
        CS_CDO_LOCAL_PFQ | CS_CDO_LOCAL_DEQ | CS_CDO_LOCAL_FEQ |
        CS_CDO_LOCAL_EV;

      /* Set function pointers */
      if (cs_test_flag(location, cs_cdo_primal_cell)) {
        compute_flux = cs_cdo_diffusion_wbs_get_pc_flux;
        msh_flag |= CS_CDO_LOCAL_HFQ;
      }
      else if (cs_test_flag(location, cs_cdo_dual_face_byc)) {
        compute_flux = cs_cdo_diffusion_wbs_get_dfbyc_flux;
        msh_flag |= CS_CDO_LOCAL_DFQ | CS_CDO_LOCAL_EFQ;
      }
      break;

    default:
      bft_error(__FILE__, __LINE__, 0, " Invalid Hodge algorithm");

    } // Switch hodge algo.

    if (b->diff_pty_uniform) {

      cs_property_get_cell_tensor(0, // cell_id
                                  eqp->diffusion_property,
                                  eqp->diffusion_hodge.inv_pty,
                                  cb->pty_mat);

      if (eqp->diffusion_hodge.is_iso)
        cb->pty_val = cb->pty_mat[0][0];

    }

    /* Define the flux by cellwise contributions */
#   pragma omp for CS_CDO_OMP_SCHEDULE
    for (cs_lnum_t c_id = 0; c_id < quant->n_cells; c_id++) {

      /* Set the local mesh structure for the current cell */
      cs_cell_mesh_build(c_id, msh_flag, connect, quant, cm);

#if defined(DEBUG) && !defined(NDEBUG) && CS_CDOVB_SCALEQ_DBG > 1
      if (b->sys_flag & CS_FLAG_SYS_DEBUG)
        if (c_id % (eqp->verbosity - 100) == 0)
          cs_cell_mesh_dump(cm);
#endif

      if (!b->diff_pty_uniform) {

        cs_property_tensor_in_cell(cm,
                                   eqp->diffusion_property,
                                   eqp->diffusion_hodge.inv_pty,
                                   cb->pty_mat);
        if (eqp->diffusion_hodge.is_iso)
          cb->pty_val = cb->pty_mat[0][0];

      }

      /* Build the local dense matrix related to this operator
         (store in cb->hdg) */
      switch (eqp->diffusion_hodge.algo) {

      case CS_PARAM_HODGE_ALGO_COST:
      case CS_PARAM_HODGE_ALGO_VORONOI:

        if (cs_test_flag(location, cs_cdo_dual_face_byc))
          get_diffusion_hodge(eqp->diffusion_hodge, cm, cb);

        /* Define a local buffer keeping the value of the discrete potential
           for the current cell */
        for (short int v = 0; v < cm->n_vc; v++)
          pot[v] = values[cm->v_ids[v]];

        break;

      case CS_PARAM_HODGE_ALGO_WBS:

        /* Define a local buffer keeping the value of the discrete potential
           for the current cell */
        pot[cm->n_vc] = 0.;
        for (short int v = 0; v < cm->n_vc; v++) {
          pot[v] = values[cm->v_ids[v]];
          pot[cm->n_vc] += cm->wvc[v]*pot[v];
        }
        break;

      default:
        bft_error(__FILE__, __LINE__, 0, " Invalid Hodge algorithm");

      } // End of switch

      if (cs_test_flag(location, cs_cdo_primal_cell))
        compute_flux(cm, pot, cb, diff_flux + 3*c_id);
      else if (cs_test_flag(location, cs_cdo_dual_face_byc))
        compute_flux(cm, pot, cb, diff_flux + connect->c2e->idx[c_id]);

    } // Loop on cells

    BFT_FREE(pot);

  } // OMP Section

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tce), &t0, &t1);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Predefined extra-operations related to this equation
 *
 * \param[in]       eqname     name of the equation
 * \param[in]       field      pointer to a field structure
 * \param[in, out]  builder    pointer to builder structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cdovb_scaleq_extra_op(const char            *eqname,
                         const cs_field_t      *field,
                         void                  *builder)
{
  CS_UNUSED(field);

  cs_cdovb_scaleq_t  *b = (cs_cdovb_scaleq_t  *)builder;

  const cs_timer_t  t0 = cs_timer_time();
  const cs_equation_param_t  *eqp = b->eqp;

  if (b->sys_flag & CS_FLAG_SYS_ADVECTION &&
      (eqp->process_flag & CS_EQUATION_POST_UPWIND_COEF)) {

    cs_real_t  *work_c = cs_equation_get_tmpbuf();
    char *postlabel = NULL;
    int  len = strlen(eqname) + 8 + 1;

    BFT_MALLOC(postlabel, len, char);
    sprintf(postlabel, "%s.UpwCoef", eqname);

    /* Compute in each cell an evaluation of upwind weight value */
    cs_cdo_advection_get_upwind_coef_cell(cs_shared_quant,
                                          eqp->advection_info,
                                          work_c);

    cs_post_write_var(CS_POST_MESH_VOLUME,
                      CS_POST_WRITER_ALL_ASSOCIATED,
                      postlabel,
                      1,
                      true,                 // interlace
                      true,                 // true = original mesh
                      CS_POST_TYPE_cs_real_t,
                      work_c,               // values on cells
                      NULL,                 // values at internal faces
                      NULL,                 // values at border faces
                      cs_shared_time_step); // time step management struct.

    BFT_FREE(postlabel);

  } // Post a Peclet attached to cells

  cs_timer_t  t1 = cs_timer_time();
  cs_timer_counter_add_diff(&(b->monitor->tce), &t0, &t1);
}

/*----------------------------------------------------------------------------*/

#undef _dp3

END_C_DECLS
