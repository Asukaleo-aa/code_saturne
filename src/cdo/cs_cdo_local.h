#ifndef __CS_CDO_LOCAL_H__
#define __CS_CDO_LOCAL_H__

/*============================================================================
 * Routines to handle low-level routines related to CDO local quantities:
 * - local matrices (stored in dense format),
 * - local quantities related to a cell.
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

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_cdo.h"
#include "cs_cdo_connect.h"
#include "cs_cdo_quantities.h"
#include "cs_cdo_toolbox.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

#define CS_CDO_LOCAL_PV  (1 <<  0) //   1: cache related to vertices
#define CS_CDO_LOCAL_PVQ (1 <<  1) //   2: cache related to vertex quantities
#define CS_CDO_LOCAL_PE  (1 <<  2) //   4: cache related to edges
#define CS_CDO_LOCAL_PEQ (1 <<  3) //   8: cache related to edge quantities
#define CS_CDO_LOCAL_DFQ (1 <<  4) //  16: cache related to dual face quant.
#define CS_CDO_LOCAL_PF  (1 <<  5) //  32: cache related to face
#define CS_CDO_LOCAL_PFQ (1 <<  6) //  64: cache related to face quantities
#define CS_CDO_LOCAL_DEQ (1 <<  7) // 128: cache related to dual edge quant.
#define CS_CDO_LOCAL_EV  (1 <<  8) // 256: cache related to e2v connect.
#define CS_CDO_LOCAL_FE  (1 <<  9) // 512: cache related to f2e connect.
#define CS_CDO_LOCAL_FEQ (1 << 10) //1024: cache related to f2e quantities
#define CS_CDO_LOCAL_EF  (1 << 11) //2048: cache related to e2f connect.
#define CS_CDO_LOCAL_EFQ (1 << 12) //4096: cache related to e2f quantities
#define CS_CDO_LOCAL_HFQ (1 << 13) //8192: cache related to the face pyramid

/*============================================================================
 * Type definitions
 *============================================================================*/

/* Structure which belongs to one thread */
typedef struct {

  /* Temporary buffers */
  short int     *ids;     // local ids
  double        *values;  // local values
  cs_real_3_t   *vectors; // local 3-dimensional vectors

  /* Structures used to build specific terms composing the algebraic system */
  cs_locmat_t   *hdg;   // local hodge matrix for diffusion (may be NULL)
  cs_locmat_t   *loc;   // local square matrix of size n_cell_dofs;
  cs_locmat_t   *aux;   // auxiliary local square matrix of size n_cell_dofs;

  /* Specific members for the weakly enforcement of Dirichlet BCs (diffusion) */
  double         eig_ratio; // ratio of the eigenvalues of the diffusion tensor
  double         eig_max;   // max. value among eigenvalues

  /* Store the cellwise value for the diffusion, time and reaction properties */
  cs_real_33_t   pty_mat; // If not isotropic
  double         pty_val; // If isotropic

} cs_cell_builder_t;

/* Structure used to store a local system (cell-wise for instance) */
typedef struct {

  int            n_dofs;   // Number of Degrees of Freedom (DoFs) in this cell
  cs_locmat_t   *mat;      // cellwise view of the system matrix
  double        *rhs;      // cellwise view of the right-hand side
  double        *source;   // cellwise view of the source term array
  double        *val_n;    /* values of the unkown at the time t_n (the
                              last computed) */

} cs_cell_sys_t;

/* Structure used to store local information about the boundary conditions */
typedef struct {

  short int   n_bc_faces;    // Number of border faces associated to a cell
  short int  *bf_ids;        // List of face ids in the cell numbering
  cs_flag_t  *face_flag;     // size n_bc_faces

  short int   n_dofs;        // Number of Degrees of Freedom (DoFs) in this cell
  cs_flag_t  *dof_flag;      // size = number of DoFs

  /* Dirichlet BCs */
  short int   n_dirichlet;   // Number of DoFs attached to a Dirichlet BC
  double     *dir_values;    // Values of the Dirichlet BCs (size = n_dofs)

  /* Neumann BCs */
  short int   n_nhmg_neuman; /* Number of DoFs related to a non-homogeneous
                                Neumann boundary (BC) */
  double     *neu_values;    // Values of the Neumnn BCs (size = n_dofs)

  /* Robin BCs */
  short int   n_robin;       // Number of DoFs attached to a Robin BC
  double     *rob_values;    // Values of the Robin BCs (size = 2*n_dofs)

} cs_cell_bc_t;

/* Structure used to get a better memory locality. Map existing structure
   into a more compact one dedicated to a cell.
   Arrays are allocated to n_max_vbyc or to n_max_ebyc.
   Cell-wise numbering is based on the c2e and c2v connectivity.
*/

typedef struct {

  cs_flag_t      flag;    // indicate which quantities have to be defined
  short int     *kbuf;    // buffer storing ids in a compact way

  /* Sizes used to allocate buffers */
  short int      n_max_vbyc;
  short int      n_max_ebyc;
  short int      n_max_fbyc;

  /* Cell information */
  cs_lnum_t      c_id;    // id of related cell
  cs_real_3_t    xc;      // coordinates of the cell center
  double         vol_c;   // volume of the current cell

  /* Vertex information */
  short int    n_vc;    // local number of vertices in a cell
  cs_lnum_t   *v_ids;   // vertex ids on this rank
  double      *xv;      // local vertex coordinates (copy)
  double      *wvc;     // weight |vol_dc(v) cap vol_c|/|vol_c for each cell vtx

  /* Edge information */
  short int    n_ec;    // local number of edges in a cell
  cs_lnum_t   *e_ids;   // edge ids on this rank
  cs_quant_t  *edge;    // local edge quantities (xe, length and unit vector)
  cs_nvec3_t  *dface;   // local dual face quantities (area and unit normal)

  /* Face information */
  short int    n_fc;    // local number of faces in a cell
  cs_lnum_t   *f_ids;   // face ids on this rank
  short int   *f_sgn;   // incidence number between f and c
  double      *hfc;     // height of the pyramid of basis f and apex c
  cs_quant_t  *face;    // local face quantities (xf, area and unit normal)
  cs_nvec3_t  *dedge;   // local dual edge quantities (length and unit vector)

  /* Local e2v connectivity: size 2*n_ec (allocated to 2*n_max_ebyc) */
  short int   *e2v_ids; // cell-wise edge -> vertices connectivity
  short int   *e2v_sgn; // cell-wise edge -> vertices orientation (-1 or +1)

  /* Local f2e connectivity: size = 2*n_max_ebyc */
  short int   *f2e_idx; // size n_fc + 1
  short int   *f2e_ids; // size 2*n_max_ebyc
  double      *tef;     // |tef| area of the triangle of base |e| and apex xf

  /* Local e2f connectivity: size 2*n_ec (allocated to 2*n_max_ebyc) */
  short int   *e2f_ids; // cell-wise edge -> faces connectivity
  cs_nvec3_t  *sefc;    // portion of dual faces (2 triangles by edge)

} cs_cell_mesh_t;

/* Structure used to get a better memory locality. Map existing structure
   into a more compact one dedicated to a face.
   Arrays are allocated to n_max_vbyf (= n_max_ebyf).
   Face-wise numbering is based on the f2e connectivity.
*/

typedef struct {

  short int    n_max_vbyf; // = n_max_ebyf

  cs_lnum_t    c_id;    // id of related cell
  cs_real_3_t  xc;      // pointer to the coordinates of the cell center

  /* Face information */
  cs_lnum_t    f_id;    // local mesh face id
  short int    f_sgn;   // incidence number between f and c
  cs_quant_t   face;    // local face quantities (xf, area and unit normal)
  cs_nvec3_t   dedge;   // local dual edge quantities (length and unit vector)

  /* Vertex information */
  short int    n_vf;    // local number of vertices on this face
  cs_lnum_t   *v_ids;   // vertex ids on this rank or in the cellwise numbering
  double      *xv;      // local vertex coordinates (copy)
  double      *wvf;     // weight related to each vertex

  /* Edge information */
  short int    n_ef;    // local number of edges in on this face (= n_vf)
  cs_lnum_t   *e_ids;   // edge ids on this rank or in the cellwise numbering
  cs_quant_t  *edge;    // local edge quantities (xe, length and unit vector)
  double      *tef;     // area of the triangle of base e and apex xf

  /* Local e2v connectivity: size 2*n_ec (allocated to 2*n_max_ebyf) */
  short int   *e2v_ids;  // face-wise edge -> vertices connectivity

} cs_face_mesh_t;

/*============================================================================
 *  Global variables
 *============================================================================*/

extern cs_cell_mesh_t  **cs_cdo_local_cell_meshes;
extern cs_face_mesh_t  **cs_cdo_local_face_meshes;

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate a cs_cell_sys_t structure
 *
 * \param[in]   n_max_ent    max number of entries
 *
 * \return a pointer to a new allocated cs_cell_sys_t structure
 */
/*----------------------------------------------------------------------------*/

cs_cell_sys_t *
cs_cell_sys_create(int    n_max_ent);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free a cs_cell_sys_t structure
 *
 * \param[in, out]  p_ls   pointer of pointer to a cs_cell_sys_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_sys_free(cs_cell_sys_t     **p_ls);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Dump a local system for debugging purpose
 *
 * \param[in]       msg     associated message to print
 * \param[in]       c_id    id related to the cell
 * \param[in]       csys    pointer to a cs_cell_sys_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_sys_dump(const char              msg[],
                 const cs_lnum_t         c_id,
                 const cs_cell_sys_t    *csys);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate a cs_cell_bc_t structure
 *
 * \param[in]   n_max_dofbyc    max. number of entries in a cell
 * \param[in]   n_max_fbyc      max. number of faces in a cell
 *
 * \return a pointer to a new allocated cs_cell_bc_t structure
 */
/*----------------------------------------------------------------------------*/

cs_cell_bc_t *
cs_cell_bc_create(int    n_max_dofbyc,
                  int    n_max_fbyc);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free a cs_cell_bc_t structure
 *
 * \param[in, out]  p_cbc   pointer of pointer to a cs_cell_bc_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_bc_free(cs_cell_bc_t     **p_cbc);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate and initialize a cs_cell_builder_t structure according to
 *         to the type of discretization which is requested.
 *
 * \param[in]  scheme    type of discretization
 * \param[in]  connect   pointer to a cs_cdo_connect_t structure
 *
 * \return a pointer to the new allocated cs_cell_builder_t structure
 */
/*----------------------------------------------------------------------------*/

cs_cell_builder_t *
cs_cell_builder_create(cs_space_scheme_t         scheme,
                       const cs_cdo_connect_t   *connect);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Initialize to invalid values a cs_cell_mesh_t structure
 *
 * \param[in]  cm         pointer to a cs_cell_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_mesh_reset(cs_cell_mesh_t   *cm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Dump a cs_cell_mesh_t structure
 *
 * \param[in]    cm    pointer to a cs_cell_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_mesh_dump(cs_cell_mesh_t     *cm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free a cs_cell_builder_t structure
 *
 * \param[in, out]  p_cb   pointer of pointer to a cs_cell_builder_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_builder_free(cs_cell_builder_t     **p_cb);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate global structures related to a cs_cell_mesh_t and
 *         cs_face_mesh_t structures
 *
 * \param[in]   connect   pointer to a cs_cdo_connect_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cdo_local_initialize(const cs_cdo_connect_t     *connect);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free global structures related to cs_cell_mesh_t and cs_face_mesh_t
 *         structures
 */
/*----------------------------------------------------------------------------*/

void
cs_cdo_local_finalize(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get a pointer to a cs_cell_mesh_t structure corresponding to mesh id
 *
 * \param[in]   mesh_id   id in the array of pointer to cs_cell_mesh_t struct.
 *
 * \return a pointer to a cs_cell_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

cs_cell_mesh_t *
cs_cdo_local_get_cell_mesh(int    mesh_id);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get a pointer to a cs_face_mesh_t structure corresponding to mesh id
 *
 * \param[in]   mesh_id   id in the array of pointer to cs_face_mesh_t struct.
 *
 * \return a pointer to a cs_face_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

cs_face_mesh_t *
cs_cdo_local_get_face_mesh(int    mesh_id);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate and initialize a cs_cell_mesh_t structure
 *
 * \param[in]  connect        pointer to a cs_cdo_connect_t structure
 *
 * \return a pointer to a new allocated cs_cell_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

cs_cell_mesh_t *
cs_cell_mesh_create(const cs_cdo_connect_t   *connect);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free a cs_cell_mesh_t structure
 *
 * \param[in, out]  p_cm   pointer of pointer to a cs_cell_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_mesh_free(cs_cell_mesh_t     **p_cm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define a cs_cell_mesh_t structure for a given cell id. According
 *         to the requested level, some quantities may not be defined;
 *
 * \param[in]       c_id      cell id
 * \param[in]       level     indicate which members are really defined
 * \param[in]       connect   pointer to a cs_cdo_connect_t structure
 * \param[in]       quant     pointer to a cs_cdo_quantities_t structure
 * \param[in, out]  cm        pointer to a cs_cell_mesh_t structure to set
 */
/*----------------------------------------------------------------------------*/

void
cs_cell_mesh_build(cs_lnum_t                    c_id,
                   cs_flag_t                    level,
                   const cs_cdo_connect_t      *connect,
                   const cs_cdo_quantities_t   *quant,
                   cs_cell_mesh_t              *cm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate a cs_face_mesh_t structure
 *
 * \param[in]  n_max_vbyf    max. number of vertices fir a face
 *
 * \return a pointer to a new allocated cs_face_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

cs_face_mesh_t *
cs_face_mesh_create(short int   n_max_vbyf);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free a cs_face_mesh_t structure
 *
 * \param[in, out]  p_fm   pointer of pointer to a cs_face_mesh_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_face_mesh_free(cs_face_mesh_t     **p_fm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define a cs_face_mesh_t structure for a given face/cell id.
 *
 * \param[in]       c_id      cell id
 * \param[in]       f_id      face id in the mesh structure
 * \param[in]       connect   pointer to a cs_cdo_connect_t structure
 * \param[in]       quant     pointer to a cs_cdo_quantities_t structure
 * \param[in, out]  fm        pointer to a cs_face_mesh_t structure to set
 */
/*----------------------------------------------------------------------------*/

void
cs_face_mesh_build(cs_lnum_t                    c_id,
                   cs_lnum_t                    f_id,
                   const cs_cdo_connect_t      *connect,
                   const cs_cdo_quantities_t   *quant,
                   cs_face_mesh_t              *fm);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define a cs_face_mesh_t structure for a given cell from a
 *         cs_cell_mesh_t structure.
 *         v_ids and e_ids are defined in the cell numbering given by cm
 *
 * \param[in]       cm        pointer to the reference cs_cell_mesh_t structure
 * \param[in]       f         face id in the cs_cell_mesh_t structure
 * \param[in, out]  fm        pointer to a cs_face_mesh_t structure to set
 */
/*----------------------------------------------------------------------------*/

void
cs_face_mesh_build_from_cell_mesh(const cs_cell_mesh_t    *cm,
                                  short int                f,
                                  cs_face_mesh_t          *fm);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_CDO_LOCAL_H__ */
