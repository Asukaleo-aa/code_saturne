/*============================================================================
 *
 *     This file is part of the Code_Saturne Kernel, element of the
 *     Code_Saturne CFD tool.
 *
 *     Copyright (C) 1998-2011 EDF S.A., France
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

/*============================================================================
 * Sparse Matrix Representation and Operations.
 *============================================================================*/

/*
 * Notes:
 *
 * The aim of these structures and associated functions is multiple:
 *
 * - Provide an "opaque" matrix object for linear solvers, allowing possible
 *   choice of the matrix type based on run-time tuning at code initialization
 *   (depending on matrix size, architecture, and compiler, the most efficient
 *   structure for matrix.vector products may vary).
 *
 * - Provide at least a CSR matrix structure in addition to the "native"
 *   matrix structure, as this may allow us to leverage existing librairies.
 *
 * - Provide a C interface, also so as to be able to interface more easily
 *   with external libraries.
 *
 * The structures used here could easily be extended to block matrixes,
 * using for example the same structure information with 3x3 blocks which
 * could arise from coupled velocity components. This would imply that the
 * corresponding vectors be interlaced (or an interlaced copy be used
 * for recurring operations such as sparse linear system resolution),
 * for better memory locality, and possible loop unrolling.
 */

#if defined(HAVE_CONFIG_H)
#include "cs_config.h"
#endif

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#if defined(__STDC_VERSION__)      /* size_t */
#if (__STDC_VERSION__ == 199901L)
#    include <stddef.h>
#  else
#    include <stdlib.h>
#  endif
#else
#include <stdlib.h>
#endif

#if defined(HAVE_MPI)
#include <mpi.h>
#endif

#if defined (HAVE_MKL)
#include <mkl_spblas.h>
#endif

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>
#include <bft_error.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_defs.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_halo.h"
#include "cs_numbering.h"
#include "cs_prototypes.h"
#include "cs_perio.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_matrix.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro Definitions
 *============================================================================*/

/* Variant for Intel compiler and on Itanium only (optimized by BULL)
   (Use compile flag -DNO_BULL_OPTIM to switch back to general code) */

#if (defined(__INTEL_COMPILER) && defined(__ia64__) && !defined(NO_BULL_OPTIM))

#define IA64_OPTIM
#define IA64_OPTIM_L1_CACHE_SIZE (508)

#endif

/*=============================================================================
 * Local Type Definitions
 *============================================================================*/

/* Formats currently supported:
 *
 *  - Native
 *  - Compressed Sparse Row (CSR)
 *  - Symmetric Compressed Sparse Row (CSR_SYM)
 */

/* Function pointer types */
/*------------------------*/

typedef void
(cs_matrix_set_coeffs_t) (cs_matrix_t      *matrix,
                          cs_bool_t         symmetric,
                          cs_bool_t         interleaved,
                          const cs_real_t  *restrict da,
                          const cs_real_t  *restrict xa);

typedef void
(cs_matrix_release_coeffs_t) (cs_matrix_t  *matrix);

typedef void
(cs_matrix_get_diagonal_t) (const cs_matrix_t  *matrix,
                            cs_real_t          *restrict da);

typedef void
(cs_matrix_vector_product_t) (const cs_matrix_t  *matrix,
                              const cs_real_t    *restrict x,
                              cs_real_t          *restrict y);

typedef void
(cs_matrix_alpha_a_x_p_beta_y_t) (cs_real_t           alpha,
                                  cs_real_t           beta,
                                  const cs_matrix_t  *matrix,
                                  const cs_real_t    *restrict x,
                                  cs_real_t          *restrict y);

typedef void
(cs_matrix_b_vector_product_t) (const cs_matrix_t  *matrix,
                                const cs_real_t    *restrict x,
                                cs_real_t          *restrict y);

typedef void
(cs_matrix_b_alpha_a_x_p_beta_y_t) (cs_real_t           alpha,
                                    cs_real_t           beta,
                                    const cs_matrix_t  *matrix,
                                    const cs_real_t    *restrict x,
                                    cs_real_t          *restrict y);

/*----------------------------------------------------------------------------
 * Local Structure Definitions
 *----------------------------------------------------------------------------*/

/* Native matrix structure representation */
/*----------------------------------------*/

/* Note: the members of this structure are already available through the top
 *       matrix structure, but are replicated here in case of future removal
 *       from the top structure (which would require computation/assignment of
 *       matrix coefficients in another form) */

typedef struct _cs_matrix_struct_native_t {

  cs_int_t           n_cells;       /* Local number of cells */
  cs_int_t           n_cells_ext;   /* Local number of participating cells
                                       (cells + ghost cells sharing a face) */
  cs_int_t           n_faces;       /* Local number of faces
                                       (for extra-diagonal terms */

  /* Pointers to shared arrays */

  const cs_int_t    *face_cell;     /* Face -> cells connectivity (1 to n) */

} cs_matrix_struct_native_t;

/* Native matrix coefficients */
/*----------------------------*/

typedef struct _cs_matrix_coeff_native_t {

  cs_bool_t         symmetric;       /* Symmetry indicator */

  /* Pointers to possibly shared arrays */

  const cs_real_t   *da;            /* Diagonal terms */
  const cs_real_t   *xa;            /* Extra-diagonal terms */

  /* Pointers to private arrays (NULL if shared) */

  cs_real_t         *_da;           /* Diagonal terms */
  cs_real_t         *_xa;           /* Extra-diagonal terms */

} cs_matrix_coeff_native_t;

/* CSR (Compressed Sparse Row) matrix structure representation */
/*-------------------------------------------------------------*/

typedef struct _cs_matrix_struct_csr_t {

  cs_int_t          n_rows;           /* Local number of rows */
  cs_int_t          n_cols;           /* Local number of columns
                                         (> n_rows in case of ghost cells) */
  cs_int_t          n_cols_max;       /* Maximum number of nonzero values
                                         on a given row */

  /* Pointers to structure arrays and info (row_index, col_id) */

  cs_bool_t         have_diag;        /* Has non-zero diagonal */
  cs_bool_t         direct_assembly;  /* True if each value corresponds to
                                         a unique face ; false if multiple
                                         faces contribute to the same
                                         value (i.e. we have split faces) */

  cs_int_t         *row_index;        /* Row index (0 to n-1) */
  cs_int_t         *col_id;           /* Column id (0 to n-1) */

  /* Pointers to optional arrays (built if needed) */

  cs_int_t         *diag_index;       /* Diagonal index (0 to n-1) for
                                         direct access to diagonal terms */

} cs_matrix_struct_csr_t;

/* CSR matrix coefficients representation */
/*----------------------------------------*/

typedef struct _cs_matrix_coeff_csr_t {

  int               n_prefetch_rows;  /* Number of rows at a time for which
                                         the x values in y = Ax should be
                                         prefetched (0 if no prefetch) */

  cs_real_t        *val;              /* Matrix coefficients */

  cs_real_t        *x_prefetch;       /* Prefetch array for x in y = Ax */

} cs_matrix_coeff_csr_t;

/* CSR_SYM (Symmetric Compressed Sparse Row) matrix structure representation */
/*---------------------------------------------------------------------------*/

typedef struct _cs_matrix_struct_csr_sym_t {

  cs_int_t          n_rows;           /* Local number of rows */
  cs_int_t          n_cols;           /* Local number of columns
                                         (> n_rows in case of ghost cells) */
  cs_int_t          n_cols_max;       /* Maximum number of nonzero values
                                         on a given row */

  /* Pointers to structure arrays and info (row_index, col_id) */

  cs_bool_t         have_diag;        /* Has non-zero diagonal */
  cs_bool_t         direct_assembly;  /* True if each value corresponds to
                                         a unique face ; false if multiple
                                         faces contribute to the same
                                         value (i.e. we have split faces) */

  cs_int_t         *row_index;        /* Row index (0 to n-1) */
  cs_int_t         *col_id;           /* Column id (0 to n-1) */

} cs_matrix_struct_csr_sym_t;

/* symmetric CSR matrix coefficients representation */
/*--------------------------------------------------*/

typedef struct _cs_matrix_coeff_csr_sym_t {

  cs_real_t        *val;              /* Matrix coefficients */

} cs_matrix_coeff_csr_sym_t;

/* Matrix structure (representation-independent part) */
/*----------------------------------------------------*/

struct _cs_matrix_structure_t {

  cs_matrix_type_t       type;         /* Matrix storage and definition type */

  cs_int_t               n_cells;      /* Local number of cells */
  cs_int_t               n_cells_ext;  /* Local number of participating cells
                                          (cells + ghost cells sharing a face) */
  cs_int_t               n_faces;      /* Local Number of mesh faces
                                          (necessary to affect coefficients) */

  void                  *structure;    /* Matrix structure */

  /* Pointers to shared arrays from mesh structure
     (face->cell connectivity for coefficient assignment,
     local->local cell numbering for future info or renumbering,
     and halo) */

  const cs_int_t        *face_cell;    /* Face -> cells connectivity (1 to n) */
  const fvm_gnum_t      *cell_num;     /* Global cell numbers */
  const cs_halo_t       *halo;         /* Parallel or periodic halo */
  const cs_numbering_t  *numbering;    /* Vectorization or thread-related
                                          numbering information */
};

/* Structure associated with Matrix (representation-independent part) */
/*--------------------------------------------------------------------*/

struct _cs_matrix_t {

  cs_matrix_type_t       type;         /* Matrix storage and definition type */

  cs_int_t               n_cells;      /* Local number of cells */
  cs_int_t               n_cells_ext;  /* Local number of participating cells
                                          (cells + ghost cells sharing a face) */
  cs_int_t               n_faces;      /* Local Number of mesh faces
                                          (necessary to affect coefficients) */

  int                    b_size[4];    /* Block size, including padding:
                                          0: useful block size
                                          1: vector block extents
                                          2: matrix line extents
                                          3: matrix line*column extents */

  /* Pointer to shared structure */

  const void            *structure;    /* Matrix structure */

  /* Pointers to shared arrays from mesh structure
     (face->cell connectivity for coefficient assignment,
     local->local cell numbering for future info or renumbering,
     and halo) */

  const cs_int_t        *face_cell;    /* Face -> cells connectivity (1 to n) */
  const fvm_gnum_t      *cell_num;     /* Global cell numbers */
  const cs_halo_t       *halo;         /* Parallel or periodic halo */
  const cs_numbering_t  *numbering;    /* Vectorization or thread-related
                                          numbering information */

  /* Pointer to private data */

  void                  *coeffs;       /* Matrix coefficients */

  /* Function pointers */

  cs_matrix_set_coeffs_t            *set_coefficients;
  cs_matrix_release_coeffs_t        *release_coefficients;
  cs_matrix_get_diagonal_t          *get_diagonal;

  cs_matrix_vector_product_t        *vector_multiply;
  cs_matrix_alpha_a_x_p_beta_y_t    *alpha_a_x_p_beta_y;

  cs_matrix_b_vector_product_t      *b_vector_multiply;
  cs_matrix_b_alpha_a_x_p_beta_y_t  *b_alpha_a_x_p_beta_y;
};

/*============================================================================
 *  Global variables
 *============================================================================*/

/* Short names for matrix types */

const char  *cs_matrix_type_name[] = {N_("native"),
                                      N_("CSR"),
                                      N_("symmetric CSR")};

/* Full names for matrix types */

const char  *cs_matrix_type_fullname[] = {N_("diagonal + faces"),
                                          N_("Compressed Sparse Row"),
                                          N_("symmetric Compressed Sparse Row")};

#if !defined (HAVE_MKL)
static int _cs_glob_matrix_prefetch_rows = 2048;
#else
static int _cs_glob_matrix_prefetch_rows = 0;
#endif

static char _cs_glob_perio_ignore_error_str[]
  = N_("Matrix product with CS_PERIO_IGNORE rotation mode not yet\n"
       "implemented: in this case, use cs_matrix_vector_multiply_nosync\n"
       "with an external halo synchronization, preceded by a backup and\n"
       "followed by a restoration of the rotation halo.");

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Compute matrix-vector product for one dense block: y[i] = a[i].x[i]
 *
 * Vectors and blocks may be larger than their useful size, to
 * improve data alignment.
 *
 * parameters:
 *   b_id   <-- block id
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *              b_size[2]: matrix line extents
 *              b_size[3]: matrix line*column (block) extents
 *   a      <-- Pointer to block matrixes array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static inline void
_dense_b_ax(fvm_lnum_t        b_id,
            const int         b_size[4],
            const cs_real_t  *restrict a,
            const cs_real_t  *restrict x,
            cs_real_t        *restrict y)
{
  fvm_lnum_t  ii, jj;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, * da)
  #endif

  for (ii = 0; ii < b_size[0]; ii++) {
    y[b_id*b_size[1] + ii] = 0;
    for (jj = 0; jj < b_size[0]; jj++)
      y[b_id*b_size[1] + ii]
        +=   a[b_id*b_size[3] + ii*b_size[2] + jj]
           * x[b_id*b_size[1] + jj];
  }
}

/*----------------------------------------------------------------------------
 * Compute matrix-vector product for one diagonal block: y[i] += a[ij].x[j]
 *
 * Vectors and blocks may be larger than their useful size, to
 * improve data alignment.
 *
 * parameters:
 *   b_id   <-- id of matrix block
 *   x_id   <-- id of x block
 *   y_id   <-- id of y block
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *   a      <-- Pointer to scalar matrix coefficients (usually extra-diagonal)
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static inline void
_diag_b_y_p_ax(fvm_lnum_t        b_id,
               fvm_lnum_t        x_id,
               fvm_lnum_t        y_id,
               const int         b_size[2],
               const cs_real_t  *restrict a,
               const cs_real_t  *restrict x,
               cs_real_t        *restrict y)
{
  fvm_lnum_t  ii;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, * a)
  #endif

  for (ii = 0; ii < b_size[0]; ii++)
    y[y_id*b_size[1] + ii] += a[b_id] * x[x_id*b_size[1] + ii];
}

/*----------------------------------------------------------------------------
 * Compute matrix-vector product for one dense block:
 * y[i] = alpha.a[i].x[i] + beta.y[i]
 *
 * Vectors and blocks may be larger than their useful size, to
 * improve data alignment.
 *
 * parameters:
 *   b_id   <-- block id
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *              b_size[2]: matrix line extents
 *              b_size[3]: matrix line*column (block) extents
 *   alpha  <-- alpha coefficient
 *   beta   <-- beta coefficient
 *   a      <-- Pointer to block matrixes array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static inline void
_dense_b_aax_p_by(int               b_id,
                  const int         b_size[4],
                  cs_real_t         alpha,
                  cs_real_t         beta,
                  const cs_real_t  *restrict a,
                  const cs_real_t  *restrict x,
                  cs_real_t        *restrict y)
{
  fvm_lnum_t  ii, jj;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, * da)
  #endif

  for (ii = 0; ii < b_size[0]; ii++) {
    y[b_id*b_size[1] + ii] *= beta;
    for (jj = 0; jj < b_size[0]; jj++)
      y[b_id*b_size[1] + ii]
        +=   a[b_id*b_size[3] + ii*b_size[2] + jj]
           * alpha*x[b_id*b_size[1] + jj];
  }
}

/*----------------------------------------------------------------------------
 * Compute matrix-vector product for one diagonal block:
 * y[i] = alpha.a[i].x[i] + beta.y[i]
 *
 * Vectors and blocks may be larger than their useful size, to
 * improve data alignment.
 *
 * parameters:
 *   b_id   <-- id of matrix block
 *   x_id   <-- id of x block
 *   y_id   <-- id of y block
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *   alpha  <-- alpha coefficient
 *   beta   <-- beta coefficient
 *   a      <-- Pointer to scalar matrix coefficients (usually extra-diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static inline void
_diag_b_aax_p_by(fvm_lnum_t        b_id,
                 fvm_lnum_t        x_id,
                 fvm_lnum_t        y_id,
                 const int         b_size[2],
                 cs_real_t         alpha,
                 cs_real_t         beta,
                 const cs_real_t  *restrict a,
                 const cs_real_t  *restrict x,
                 cs_real_t        *restrict y)
{
  fvm_lnum_t  ii;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, * da)
  #endif

  for (ii = 0; ii < b_size[0]; ii++)
    y[y_id*b_size[1] + ii] =   alpha*(a[b_id] * x[x_id*b_size[1] + ii])
                             + beta*y[y_id*b_size[1] + ii];
}

/*----------------------------------------------------------------------------
 * y[i] = da[i].x[i], with da possibly NULL
 *
 * parameters:
 *   da     <-- Pointer to coefficients array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *   n_elts <-- Array size
 *----------------------------------------------------------------------------*/

static inline void
_diag_vec_p_l(const cs_real_t  *restrict da,
              const cs_real_t  *restrict x,
              cs_real_t        *restrict y,
              cs_int_t          n_elts)
{
  cs_int_t  ii;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, *da)
  #endif

  /* Note: also try with BLAS: DNDOT(n_cells, 1, y, 1, 1, da, x, 1, 1) */

  if (da != NULL) {
#pragma omp parallel for
    for (ii = 0; ii < n_elts; ii++)
      y[ii] = da[ii] * x[ii];
  }
  else {
#pragma omp parallel for
    for (ii = 0; ii < n_elts; ii++)
      y[ii] = 0.0;
  }

}

/*----------------------------------------------------------------------------
 * Block version of y[i] = da[i].x[i], with da possibly NULL
 *
 * parameters:
 *   da     <-- Pointer to coefficients array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *   n_elts <-- Array size
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *              b_size[2]: matrix line extents
 *              b_size[3]: matrix line*column (block) extents
 *----------------------------------------------------------------------------*/

static inline void
_b_diag_vec_p_l(const cs_real_t  *restrict da,
                const cs_real_t  *restrict x,
                cs_real_t        *restrict y,
                fvm_lnum_t        n_elts,
                const int         b_size[4])
{
  fvm_lnum_t  ii;

  if (da != NULL) {
#pragma omp parallel for
    for (ii = 0; ii < n_elts; ii++)
      _dense_b_ax(ii, b_size, da, x, y);
  }
  else {
#pragma omp parallel for
    for (ii = 0; ii < n_elts*b_size[1]; ii++)
      y[ii] = 0.0;
  }
}

/*----------------------------------------------------------------------------
 * y[i] = alpha.da[i].x[i] + beta.y[i], with da possibly NULL
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   da     <-- Pointer to coefficients array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *   n_elts <-- Array size
 *----------------------------------------------------------------------------*/

static inline void
_diag_x_p_beta_y(cs_real_t         alpha,
                 cs_real_t         beta,
                 const cs_real_t  *restrict da,
                 const cs_real_t  *restrict x,
                 cs_real_t        *restrict y,
                 cs_int_t          n_elts)
{
  cs_int_t  ii;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, *da)
  #endif

  if (da != NULL) {
#pragma omp parallel for firstprivate(alpha, beta)
    for (ii = 0; ii < n_elts; ii++)
      y[ii] = (alpha * da[ii] * x[ii]) + (beta * y[ii]);
  }
  else {
#pragma omp parallel for firstprivate(beta)
    for (ii = 0; ii < n_elts; ii++)
      y[ii] *= beta;
  }
}

/*----------------------------------------------------------------------------
 * Block version of y[i] = alpha.da[i].x[i] + beta.y[i], with da possibly NULL
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   da     <-- Pointer to coefficients array (usually matrix diagonal)
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *   n_elts <-- Array size
 *   b_size <-- block size, including padding:
 *              b_size[0]: useful block size
 *              b_size[1]: vector block extents
 *              b_size[2]: matrix line extents
 *              b_size[3]: matrix line*column (block) extents
 *----------------------------------------------------------------------------*/

static inline void
_b_diag_x_p_beta_y(cs_real_t         alpha,
                   cs_real_t         beta,
                   const cs_real_t  *restrict da,
                   const cs_real_t  *restrict x,
                   cs_real_t        *restrict y,
                   fvm_lnum_t        n_elts,
                   const int         b_size[4])
{
  fvm_lnum_t ii, jj;

  #if defined(__xlc__) /* Tell IBM compiler not to alias */
  #pragma disjoint(*x, *y, *da)
  #endif

  if (da != NULL) {
#pragma omp parallel for firstprivate(alpha, beta)
    for (ii = 0; ii < n_elts; ii++)
      _dense_b_aax_p_by(ii,
                        b_size,
                        alpha,
                        beta,
                        da,
                        x,
                        y);
  }
  else {
#pragma omp parallel for firstprivate(beta)
    for (ii = 0; ii < n_elts; ii++) {
      for (jj = 0; jj < b_size[0]; jj++)
        y[ii*b_size[1] + jj] *= beta;
    }
  }
}

/*----------------------------------------------------------------------------
 * Set values from y[start_id] to y[end_id] to 0.
 *
 * parameters:
 *   y        --> Resulting vector
 *   start_id <-- start id in array
 *   end_id   <-- end id in array
 *----------------------------------------------------------------------------*/

static inline void
_zero_range(cs_real_t  *restrict y,
            cs_int_t    start_id,
            cs_int_t    end_id)
{
  fvm_lnum_t  ii;

  #pragma omp parallel for
  for (ii = start_id; ii < end_id; ii++)
    y[ii] = 0.0;
}

/*----------------------------------------------------------------------------
 * Set values from y[start_id] to y[end_id] to 0, block version.
 *
 * parameters:
 *   y        --> Resulting vector
 *   start_id <-- start id in array
 *   end_id   <-- end id in array
 *   b_size   <-- block size, including padding:
 *                b_size[0]: useful block size
 *                b_size[1]: vector block extents
 *----------------------------------------------------------------------------*/

static inline void
_b_zero_range(cs_real_t  *restrict y,
              cs_int_t    start_id,
              cs_int_t    end_id,
              const int   b_size[2])
{
  cs_int_t  ii;

  #pragma omp parallel for
  for (ii = start_id*b_size[1]; ii < end_id*b_size[1]; ii++)
    y[ii] = 0.0;
}

/*----------------------------------------------------------------------------
 * Descend binary tree for the ordering of a fvm_gnum (integer) array.
 *
 * parameters:
 *   number    <-> pointer to elements that should be ordered
 *   level     <-- level of the binary tree to descend
 *   n_elts    <-- number of elements in the binary tree to descend
 *----------------------------------------------------------------------------*/

inline static void
_sort_descend_tree(cs_int_t  number[],
                   size_t    level,
                   size_t    n_elts)
{
  size_t lv_cur;
  cs_int_t num_save;

  num_save = number[level];

  while (level <= (n_elts/2)) {

    lv_cur = (2*level) + 1;

    if (lv_cur < n_elts - 1)
      if (number[lv_cur+1] > number[lv_cur]) lv_cur++;

    if (lv_cur >= n_elts) break;

    if (num_save >= number[lv_cur]) break;

    number[level] = number[lv_cur];
    level = lv_cur;

  }

  number[level] = num_save;
}

/*----------------------------------------------------------------------------
 * Order an array of global numbers.
 *
 * parameters:
 *   number   <-> number of arrays to sort
 *   n_elts   <-- number of elements considered
 *----------------------------------------------------------------------------*/

static void
_sort_local(cs_int_t  number[],
            size_t    n_elts)
{
  size_t i, j, inc;
  cs_int_t num_save;

  if (n_elts < 2)
    return;

  /* Use shell sort for short arrays */

  if (n_elts < 20) {

    /* Compute increment */
    for (inc = 1; inc <= n_elts/9; inc = 3*inc+1);

    /* Sort array */
    while (inc > 0) {
      for (i = inc; i < n_elts; i++) {
        num_save = number[i];
        j = i;
        while (j >= inc && number[j-inc] > num_save) {
          number[j] = number[j-inc];
          j -= inc;
        }
        number[j] = num_save;
      }
      inc = inc / 3;
    }

  }

  else {

    /* Create binary tree */

    i = (n_elts / 2);
    do {
      i--;
      _sort_descend_tree(number, i, n_elts);
    } while (i > 0);

    /* Sort binary tree */

    for (i = n_elts - 1 ; i > 0 ; i--) {
      num_save   = number[0];
      number[0] = number[i];
      number[i] = num_save;
      _sort_descend_tree(number, 0, i);
    }
  }
}

/*----------------------------------------------------------------------------
 * Create native matrix structure.
 *
 * Note that the structure created maps to the given existing
 * face -> cell connectivity array, so it must be destroyed before this
 * array (usually the code's main face -> cell structure) is freed.
 *
 * parameters:
 *   n_cells     <-- Local number of participating cells
 *   n_cells_ext <-- Local number of cells + ghost cells sharing a face
 *   n_faces     <-- Local number of faces
 *   face_cell   <-- Face -> cells connectivity (1 to n)
 *
 * returns:
 *   pointer to allocated native matrix structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_struct_native_t *
_create_struct_native(int              n_cells,
                      int              n_cells_ext,
                      int              n_faces,
                      const cs_int_t  *face_cell)
{
  cs_matrix_struct_native_t  *ms;

  /* Allocate and map */

  BFT_MALLOC(ms, 1, cs_matrix_struct_native_t);

  /* Allocate and map */

  ms->n_cells = n_cells;
  ms->n_cells_ext = n_cells_ext;
  ms->n_faces = n_faces;

  ms->face_cell = face_cell;

  return ms;
}

/*----------------------------------------------------------------------------
 * Destroy native matrix structure.
 *
 * parameters:
 *   matrix  <->  Pointer to native matrix structure pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_struct_native(cs_matrix_struct_native_t **matrix)
{
  if (matrix != NULL && *matrix !=NULL) {

    BFT_FREE(*matrix);

  }
}

/*----------------------------------------------------------------------------
 * Create native matrix coefficients.
 *
 * returns:
 *   pointer to allocated native coefficients structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_coeff_native_t *
_create_coeff_native(void)
{
  cs_matrix_coeff_native_t  *mc;

  /* Allocate */

  BFT_MALLOC(mc, 1, cs_matrix_coeff_native_t);

  /* Initialize */

  mc->symmetric = false;

  mc->da = NULL;
  mc->xa = NULL;

  mc->_da = NULL;
  mc->_xa = NULL;

  return mc;
}

/*----------------------------------------------------------------------------
 * Destroy native matrix coefficients.
 *
 * parameters:
 *   coeff  <->  Pointer to native matrix coefficients pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_coeff_native(cs_matrix_coeff_native_t **coeff)
{
  if (coeff != NULL && *coeff !=NULL) {

    cs_matrix_coeff_native_t  *mc = *coeff;

    if (mc->_xa != NULL)
      BFT_FREE(mc->_xa);

    if (mc->_da != NULL)
      BFT_FREE(mc->_da);

    BFT_FREE(*coeff);

  }
}

/*----------------------------------------------------------------------------
 * Set Native matrix coefficients.
 *
 * Depending on current options and initialization, values will be copied
 * or simply mapped.
 *
 * parameters:
 *   matrix           <-- Pointer to matrix structure
 *   symmetric        <-- Indicates if extradiagonal values are symmetric
 *   interleaved      <-- Indicates if matrix coefficients are interleaved
 *   da               <-- Diagonal values
 *   xa               <-- Extradiagonal values
 *----------------------------------------------------------------------------*/

static void
_set_coeffs_native(cs_matrix_t      *matrix,
                   cs_bool_t         symmetric,
                   cs_bool_t         interleaved,
                   const cs_real_t  *da,
                   const cs_real_t  *xa)
{
  cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  cs_int_t ii;
  mc->symmetric = symmetric;

  /* Map or copy values */

  if (da != NULL) {

    if (mc->_da == NULL)
      mc->da = da;
    else {
      memcpy(mc->_da, da, matrix->b_size[3]*sizeof(cs_real_t) * ms->n_cells);
      mc->da = mc->_da;
    }

  }
  else {
    mc->da = NULL;
  }

  if (xa != NULL) {

    if (interleaved || symmetric == true) {

      if (mc->_xa == NULL)
        mc->xa = xa;
      else {
        size_t xa_n_bytes = sizeof(cs_real_t) * ms->n_faces;
        if (! symmetric)
          xa_n_bytes *= 2;
        memcpy(mc->_xa, xa, xa_n_bytes);
        mc->xa = mc->_xa;
      }
    }

    else { /* !interleaved && symmetric == false */

      assert(matrix->b_size[3] == 1);

      if (mc->_xa == NULL)
        BFT_MALLOC(mc->_xa, 2*ms->n_faces, cs_real_t);

      for (ii = 0; ii < ms->n_faces; ++ii) {
        mc->_xa[2*ii] = xa[ii];
        mc->_xa[2*ii + 1] = xa[ms->n_faces + ii];
      }
      mc->xa = mc->_xa;

    }
  }
}

/*----------------------------------------------------------------------------
 * Release native matrix coefficients.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *----------------------------------------------------------------------------*/

static void
_release_coeffs_native(cs_matrix_t  *matrix)
{
  cs_matrix_coeff_native_t  *mc = matrix->coeffs;

  if (mc !=NULL) {

    /* Unmap values */

    mc->da = NULL;
    mc->xa = NULL;

    /* Possibly allocated mc->_da and mc->_xa arrays are not freed
       here, but simply unmapped from mc->da and mc->xa;
       they are remapped when _set_coeffs_native() is used. */

  }

}

/*----------------------------------------------------------------------------
 * Get diagonal of native matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   da     --> Diagonal (pre-allocated, size: n_cells)
 *----------------------------------------------------------------------------*/

static void
_get_diagonal_native(const cs_matrix_t  *matrix,
                     cs_real_t          *restrict da)
{
  cs_int_t  ii, jj;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  cs_int_t  n_cells = ms->n_cells;
  const int *b_size = matrix->b_size;

  /* Unblocked version */

  if (matrix->b_size[3] == 1) {

    if (mc->da != NULL) {

#pragma omp parallel for
      for (ii = 0; ii < n_cells; ii++)
        da[ii] = mc->da[ii];

    }
    else {

#pragma omp parallel for
      for (ii = 0; ii < n_cells; ii++)
        da[ii] = 0.0;

    }

  }

  /* Blocked version */

  else {

    if (mc->da != NULL) {

#pragma omp parallel for private(jj)
      for (ii = 0; ii < n_cells; ii++) {
        for (jj = 0; jj < b_size[0]; jj++)
          da[ii*b_size[1] + jj] = mc->da[ii*b_size[3] + jj*b_size[2] + jj];
      }
    }
    else {

#pragma omp parallel for
      for (ii = 0; ii < n_cells*b_size[1]; ii++)
        da[ii] = 0.0;

    }
  }
}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with native matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_mat_vec_p_l_native(const cs_matrix_t  *matrix,
                    const cs_real_t    *restrict x,
                    cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, face_id;

  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;

  const cs_real_t  *restrict xa = mc->xa;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _diag_vec_p_l(mc->da, x, y, ms->n_cells);

  _zero_range(y, ms->n_cells, ms->n_cells_ext);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += xa[face_id] * x[jj];
        y[jj] += xa[face_id] * x[ii];
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += xa[2*face_id] * x[jj];
        y[jj] += xa[2*face_id + 1] * x[ii];
      }

    }

  }
}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with native matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_b_mat_vec_p_l_native(const cs_matrix_t  *matrix,
                      const cs_real_t    *restrict x,
                      cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, kk, face_id;

  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;

  const cs_real_t  *restrict xa = mc->xa;
  const int *b_size = matrix->b_size;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _b_diag_vec_p_l(mc->da, x, y, ms->n_cells, b_size);

  _b_zero_range(y, ms->n_cells, ms->n_cells_ext, b_size);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        for (kk = 0; kk < b_size[0]; kk++) {
          y[ii*b_size[1] + kk] += xa[face_id] * x[jj*b_size[1] + kk];
          y[jj*b_size[1] + kk] += xa[face_id] * x[ii*b_size[1] + kk];
        }
      }
    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        for (kk = 0; kk < b_size[0]; kk++) {
          y[ii*b_size[1] + kk] += xa[2*face_id]     * x[jj*b_size[1] + kk];
          y[jj*b_size[1] + kk] += xa[2*face_id + 1] * x[ii*b_size[1] + kk];
        }
      }

    }

  }

}

#if defined(HAVE_OPENMP) /* OpenMP variants */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with native matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_mat_vec_p_l_native_omp(const cs_matrix_t  *matrix,
                        const cs_real_t    *restrict x,
                        cs_real_t          *restrict y)
{
  int g_id, t_id;
  cs_int_t  ii, jj, face_id;

  const int n_threads = matrix->numbering->n_threads;
  const int n_groups = matrix->numbering->n_groups;
  const fvm_lnum_t *group_index = matrix->numbering->group_index;

  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;

  assert(matrix->numbering->type == CS_NUMBERING_THREADS);

  /* Tell IBM compiler not to alias */

  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _diag_vec_p_l(mc->da, x, y, ms->n_cells);

  _zero_range(y, ms->n_cells, ms->n_cells_ext);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            y[ii] += xa[face_id] * x[jj];
            y[jj] += xa[face_id] * x[ii];
          }
        }
      }
    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            y[ii] += xa[2*face_id] * x[jj];
            y[jj] += xa[2*face_id + 1] * x[ii];
          }
        }
      }
    }

  }
}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with native matrix, blocked version
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_b_mat_vec_p_l_native_omp(const cs_matrix_t  *matrix,
                          const cs_real_t    *restrict x,
                          cs_real_t          *restrict y)
{
  int g_id, t_id;
  cs_int_t  ii, jj, kk, face_id;
  const int *b_size = matrix->b_size;

  const int n_threads = matrix->numbering->n_threads;
  const int n_groups = matrix->numbering->n_groups;
  const fvm_lnum_t *group_index = matrix->numbering->group_index;

  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;

  assert(matrix->numbering->type == CS_NUMBERING_THREADS);

  /* Tell IBM compiler not to alias */

  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _b_diag_vec_p_l(mc->da, x, y, ms->n_cells, b_size);

  _b_zero_range(y, ms->n_cells, ms->n_cells_ext, b_size);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            for (kk = 0; kk < b_size[0]; kk++) {
              y[ii*b_size[1] + kk] += xa[face_id] * x[jj*b_size[1] + kk];
              y[jj*b_size[1] + kk] += xa[face_id] * x[ii*b_size[1] + kk];
            }
          }
        }
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            for (kk = 0; kk < b_size[0]; kk++) {
              y[ii*b_size[1] + kk] += xa[2*face_id]     * x[jj*b_size[1] + kk];
              y[jj*b_size[1] + kk] += xa[2*face_id + 1] * x[ii*b_size[1] + kk];
            }
          }
        }
      }

    }

  }
}

#endif /* defined(HAVE_OPENMP) */

#if defined(IA64_OPTIM)  /* Special variant for IA64 */

static void
_mat_vec_p_l_native_ia64(const cs_matrix_t  *matrix,
                         const cs_real_t    *restrict x,
                         cs_real_t          *restrict y)
{
  cs_int_t  ii, ii_prev, kk, face_id, kk_max;
  cs_real_t y_it, y_it_prev;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict da = mc->da;
  const cs_real_t  *restrict xa = mc->xa;

  /* Diagonal part of matrix.vector product */

  /* Note: also try with BLAS: DNDOT(n_cells, 1, y, 1, 1, da, x, 1, 1) */

  if (mc->da != NULL) {
    for (ii = 0; ii < ms->n_cells; ii++)
      y[ii] = da[ii] * x[ii];
  }
  else {
    for (ii = 0; ii < ms->n_cells; y[ii++] = 0.0);
  }

  for (ii = ms->n_cells; ii < ms->n_cells_ext; y[ii++] = 0.0);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    /*
     * 1/ Split y[ii] and y[jj] computation into 2 loops to remove compiler
     *    data dependency assertion between y[ii] and y[jj].
     * 2/ keep index (*face_cel_p) in L1 cache from y[ii] loop to y[jj] loop
     *    and xa in L2 cache.
     * 3/ break high frequency occurence of data dependency from one iteration
     *    to another in y[ii] loop (nonzero matrix value on the same line ii).
     */

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0;
           face_id < ms->n_faces;
           face_id += IA64_OPTIM_L1_CACHE_SIZE) {

        kk_max = CS_MIN((ms->n_faces - face_id),
                        IA64_OPTIM_L1_CACHE_SIZE);

        /* sub-loop to compute y[ii] += xa[face_id] * x[jj] */

        ii = face_cel_p[0] - 1;
        ii_prev = ii;
        y_it_prev = y[ii_prev] + xa[face_id] * x[face_cel_p[1] - 1];

        for (kk = 1; kk < kk_max; ++kk) {
          ii = face_cel_p[2*kk] - 1;
          /* y[ii] += xa[face_id+kk] * x[jj]; */
          if (ii == ii_prev) {
            y_it = y_it_prev;
          }
          else {
            y_it = y[ii];
            y[ii_prev] = y_it_prev;
          }
          ii_prev = ii;
          y_it_prev = y_it + xa[face_id+kk] * x[face_cel_p[2*kk+1] - 1];
        }
        y[ii] = y_it_prev;

        /* sub-loop to compute y[ii] += xa[face_id] * x[jj] */

        for (kk = 0; kk < kk_max; ++kk) {
          y[face_cel_p[2*kk+1] - 1]
            += xa[face_id+kk] * x[face_cel_p[2*kk] - 1];
        }
        face_cel_p += 2 * IA64_OPTIM_L1_CACHE_SIZE;
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0;
           face_id < ms->n_faces;
           face_id+=IA64_OPTIM_L1_CACHE_SIZE) {

        kk_max = CS_MIN((ms->n_faces - face_id),
                        IA64_OPTIM_L1_CACHE_SIZE);

        /* sub-loop to compute y[ii] += xa[2*face_id] * x[jj] */

        ii = face_cel_p[0] - 1;
        ii_prev = ii;
        y_it_prev = y[ii_prev] + xa[2*face_id] * x[face_cel_p[1] - 1];

        for (kk = 1; kk < kk_max; ++kk) {
          ii = face_cel_p[2*kk] - 1;
          /* y[ii] += xa[2*(face_id+i)] * x[jj]; */
          if (ii == ii_prev) {
            y_it = y_it_prev;
          }
          else {
            y_it = y[ii];
            y[ii_prev] = y_it_prev;
          }
          ii_prev = ii;
          y_it_prev = y_it + xa[2*(face_id+kk)] * x[face_cel_p[2*kk+1] - 1];
        }
        y[ii] = y_it_prev;

        /* sub-loop to compute y[ii] += xa[2*face_id + 1] * x[jj] */

        for (kk = 0; kk < kk_max; ++kk) {
          y[face_cel_p[2*kk+1] - 1]
            += xa[2*(face_id+kk) + 1] * x[face_cel_p[2*kk] - 1];
        }
        face_cel_p += 2 * IA64_OPTIM_L1_CACHE_SIZE;
      }

    }
  }
}

#endif /* defined(IA64_OPTIM) */

#if defined(SX) && defined(_SX) /* For vector machines */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with native matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_mat_vec_p_l_native_vector(const cs_matrix_t  *matrix,
                           const cs_real_t    *restrict x,
                           cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, face_id;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;

  assert(matrix->numbering->type == CS_NUMBERING_VECTORIZE);

  /* Diagonal part of matrix.vector product */

  _diag_vec_p_l(mc->da, x, y, ms->n_cells, 1);

  _zero_range(y, ms->n_cells, ms->n_cells_ext, 1);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      #pragma dir nodep
      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += xa[face_id] * x[jj];
        y[jj] += xa[face_id] * x[ii];
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      #pragma dir nodep
      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += xa[2*face_id] * x[jj];
        y[jj] += xa[2*face_id + 1] * x[ii];
      }

    }

  }
}

#endif /* Vector machine variant */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y with native matrix.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_alpha_a_x_p_beta_y_native(cs_real_t           alpha,
                           cs_real_t           beta,
                           const cs_matrix_t  *matrix,
                           const cs_real_t    *restrict x,
                           cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, face_id;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _diag_x_p_beta_y(alpha, beta, mc->da, x, y, ms->n_cells);

  _zero_range(y, ms->n_cells, ms->n_cells_ext);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += alpha * xa[face_id] * x[jj];
        y[jj] += alpha * xa[face_id] * x[ii];
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += alpha * xa[2*face_id] * x[jj];
        y[jj] += alpha * xa[2*face_id + 1] * x[ii];
      }

    }

  }
}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y with native matrix,
 * blocked version.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_b_alpha_a_x_p_beta_y_native(cs_real_t           alpha,
                             cs_real_t           beta,
                             const cs_matrix_t  *matrix,
                             const cs_real_t    *restrict x,
                             cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, kk, face_id;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;
  const int *b_size = matrix->b_size;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _b_diag_x_p_beta_y(alpha, beta, mc->da, x, y, ms->n_cells, b_size);

  _b_zero_range(y, ms->n_cells, ms->n_cells_ext, b_size);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */
  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        for (kk = 0; kk < b_size[0]; kk++) {
          y[ii*b_size[1] + kk] += alpha*xa[face_id] * x[jj*b_size[1] + kk];
          y[jj*b_size[1] + kk] += alpha*xa[face_id] * x[ii*b_size[1] + kk];
        }

      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        for (kk = 0; kk < b_size[0]; kk++) {
          y[ii*b_size[1] + kk] += alpha*xa[2*face_id]   * x[jj*b_size[1] + kk];
          y[jj*b_size[1] + kk] += alpha*xa[2*face_id+1] * x[ii*b_size[1] + kk];
        }

      }

    }

  }
}

#if defined(HAVE_OPENMP) /* OpenMP variant */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y with native matrix.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_alpha_a_x_p_beta_y_native_omp(cs_real_t           alpha,
                               cs_real_t           beta,
                               const cs_matrix_t  *matrix,
                               const cs_real_t    *restrict x,
                               cs_real_t          *restrict y)
{
  int g_id, t_id;
  cs_int_t  ii, jj, face_id;
  const int n_threads = matrix->numbering->n_threads;
  const int n_groups = matrix->numbering->n_groups;
  const fvm_lnum_t *group_index = matrix->numbering->group_index;

  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;

  const cs_real_t  *restrict xa = mc->xa;

  assert(matrix->numbering->type == CS_NUMBERING_THREADS);

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *xa)
  #endif

  /* Diagonal part of matrix.vector product */

  _diag_x_p_beta_y(alpha, beta, mc->da, x, y, ms->n_cells);

  _zero_range(y, ms->n_cells, ms->n_cells_ext);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            y[ii] += alpha * xa[face_id] * x[jj];
            y[jj] += alpha * xa[face_id] * x[ii];
          }
        }

      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      for (g_id=0; g_id < n_groups; g_id++) {

        #pragma omp parallel for private(face_id, ii, jj)
        for (t_id=0; t_id < n_threads; t_id++) {

          for (face_id = group_index[(t_id*n_groups + g_id)*2];
               face_id < group_index[(t_id*n_groups + g_id)*2 + 1];
               face_id++) {
            ii = face_cel_p[2*face_id] -1;
            jj = face_cel_p[2*face_id + 1] -1;
            y[ii] += alpha * xa[2*face_id] * x[jj];
            y[jj] += alpha * xa[2*face_id + 1] * x[ii];
          }
        }
      }

    }

  } /* if mc-> xa != NULL */
}

#endif

#if defined(SX) && defined(_SX) /* For vector machines */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y with native matrix.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_alpha_a_x_p_beta_y_native_vector(cs_real_t           alpha,
                                  cs_real_t           beta,
                                  const cs_matrix_t  *matrix,
                                  const cs_real_t    *restrict x,
                                  cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, face_id;
  const cs_matrix_struct_native_t  *ms = matrix->structure;
  const cs_matrix_coeff_native_t  *mc = matrix->coeffs;
  const cs_real_t  *restrict xa = mc->xa;
  assert(matrix->numbering->type == CS_NUMBERING_VECTORIZE);

  /* Diagonal part of matrix.vector product */

  _diag_x_p_beta_y(alpha, beta, mc->da, x, y, ms->n_cells);

  _zero_range(y, ms->n_cells, ms->n_cells_ext);

  /* Note: parallel and periodic synchronization could be delayed to here */

  /* non-diagonal terms */

  if (mc->xa != NULL) {

    if (mc->symmetric) {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      #pragma cdir nodep
      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += alpha * xa[face_id] * x[jj];
        y[jj] += alpha * xa[face_id] * x[ii];
      }

    }
    else {

      const cs_int_t *restrict face_cel_p = ms->face_cell;

      #pragma cdir nodep
      for (face_id = 0; face_id < ms->n_faces; face_id++) {
        ii = face_cel_p[2*face_id] -1;
        jj = face_cel_p[2*face_id + 1] -1;
        y[ii] += alpha * xa[2*face_id] * x[jj];
        y[jj] += alpha * xa[2*face_id + 1] * x[ii];
      }

    }

  }
}

#endif /* Vector machine variant */

/*----------------------------------------------------------------------------
 * Create a CSR matrix structure from a native matrix stucture.
 *
 * Note that the structure created maps global cell numbers to the given
 * existing face -> cell connectivity array, so it must be destroyed before
 * this array (usually the code's global cell numbering) is freed.
 *
 * parameters:
 *   have_diag   <-- Indicates if the diagonal is nonzero
 *   n_cells     <-- Local number of participating cells
 *   n_cells_ext <-- Local number of cells + ghost cells sharing a face
 *   n_faces     <-- Local number of faces
 *   cell_num    <-- Global cell numbers (1 to n)
 *   face_cell   <-- Face -> cells connectivity (1 to n)
 *
 * returns:
 *   pointer to allocated CSR matrix structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_struct_csr_t *
_create_struct_csr(cs_bool_t         have_diag,
                   int               n_cells,
                   int               n_cells_ext,
                   int               n_faces,
                   const cs_int_t   *face_cell)
{
  int n_cols_max;
  cs_int_t ii, jj, face_id;
  const cs_int_t *restrict face_cel_p;

  cs_int_t  diag_elts = 1;
  cs_int_t  *ccount = NULL;

  cs_matrix_struct_csr_t  *ms;

  /* Allocate and map */

  BFT_MALLOC(ms, 1, cs_matrix_struct_csr_t);

  ms->n_rows = n_cells;
  ms->n_cols = n_cells_ext;

  ms->direct_assembly = true;
  ms->have_diag = have_diag;

  BFT_MALLOC(ms->row_index, ms->n_rows + 1, cs_int_t);

  ms->diag_index = NULL; /* Diagonal index only built if required */

  /* Count number of nonzero elements per row */

  BFT_MALLOC(ccount, ms->n_cols, cs_int_t);

  if (have_diag == false)
    diag_elts = 0;

  for (ii = 0; ii < ms->n_rows; ii++)  /* count starting with diagonal terms */
    ccount[ii] = diag_elts;

  if (face_cell != NULL) {

    face_cel_p = face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      ccount[ii] += 1;
      ccount[jj] += 1;
    }

  } /* if (face_cell != NULL) */

  n_cols_max = 0;

  ms->row_index[0] = 0;
  for (ii = 0; ii < ms->n_rows; ii++) {
    ms->row_index[ii+1] = ms->row_index[ii] + ccount[ii];
    if (ccount[ii] > n_cols_max)
      n_cols_max = ccount[ii];
    ccount[ii] = diag_elts; /* pre-count for diagonal terms */
  }

  ms->n_cols_max = n_cols_max;

  /* Build structure */

  BFT_MALLOC(ms->col_id, (ms->row_index[ms->n_rows]), cs_int_t);

  if (have_diag == true) {
    for (ii = 0; ii < ms->n_rows; ii++) {    /* diagonal terms */
      ms->col_id[ms->row_index[ii]] = ii;
    }
  }

  if (face_cell != NULL) {                   /* non-diagonal terms */

    face_cel_p = face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      if (ii < ms->n_rows) {
        ms->col_id[ms->row_index[ii] + ccount[ii]] = jj;
        ccount[ii] += 1;
      }
      if (jj < ms->n_rows) {
        ms->col_id[ms->row_index[jj] + ccount[jj]] = ii;
        ccount[jj] += 1;
      }
    }

  } /* if (face_cell != NULL) */

  BFT_FREE(ccount);

  /* Sort line elements by column id (for better access patterns) */

  if (n_cols_max > 1) {

    for (ii = 0; ii < ms->n_rows; ii++) {
      cs_int_t *col_id = ms->col_id + ms->row_index[ii];
      cs_int_t n_cols = ms->row_index[ii+1] - ms->row_index[ii];
      cs_int_t col_id_prev = -1;
      _sort_local(col_id, ms->row_index[ii+1] - ms->row_index[ii]);
      for (jj = 0; jj < n_cols; jj++) {
        if (col_id[jj] == col_id_prev)
          ms->direct_assembly = false;
        col_id_prev = col_id[jj];
      }
    }

  }

  /* Compact elements if necessary */

  if (ms->direct_assembly == false) {

    cs_int_t *tmp_row_index = NULL;
    cs_int_t  kk = 0;

    BFT_MALLOC(tmp_row_index, ms->n_rows+1, cs_int_t);
    memcpy(tmp_row_index, ms->row_index, (ms->n_rows+1)*sizeof(cs_int_t));

    kk = 0;

    for (ii = 0; ii < ms->n_rows; ii++) {
      cs_int_t *col_id = ms->col_id + ms->row_index[ii];
      cs_int_t n_cols = ms->row_index[ii+1] - ms->row_index[ii];
      cs_int_t col_id_prev = -1;
      ms->row_index[ii] = kk;
      for (jj = 0; jj < n_cols; jj++) {
        if (col_id_prev != col_id[jj]) {
          ms->col_id[kk++] = col_id[jj];
          col_id_prev = col_id[jj];
        }
      }
    }
    ms->row_index[ms->n_rows] = kk;

    assert(ms->row_index[ms->n_rows] < tmp_row_index[ms->n_rows]);

    BFT_FREE(tmp_row_index);
    BFT_REALLOC(ms->col_id, (ms->row_index[ms->n_rows]), cs_int_t);

  }

  return ms;
}

/*----------------------------------------------------------------------------
 * Destroy CSR matrix structure.
 *
 * parameters:
 *   matrix  <->  Pointer to CSR matrix structure pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_struct_csr(cs_matrix_struct_csr_t **matrix)
{
  if (matrix != NULL && *matrix !=NULL) {

    cs_matrix_struct_csr_t  *ms = *matrix;

    if (ms->row_index != NULL)
      BFT_FREE(ms->row_index);

    if (ms->col_id != NULL)
      BFT_FREE(ms->col_id);

    if (ms->diag_index != NULL)
      BFT_FREE(ms->diag_index);

    BFT_FREE(ms);

    *matrix = ms;

  }
}

#if 0
/*----------------------------------------------------------------------------
 * Add a diagonal index to a CSR matrix structure, if applicable.
 *
 * parameters:
 *   ms      <->  Matrix structure
 *----------------------------------------------------------------------------*/

static void
_add_diag_index_struct_csr(cs_matrix_struct_csr_t  *ms)
{
  cs_int_t ii;

  if (ms->diag_index != NULL)
    return;

  BFT_MALLOC(ms->diag_index, ms->n_rows, cs_int_t);

  for (ii = 0; ii < ms->n_rows; ii++) {
    cs_int_t kk;
    for (kk = ms->row_index[ii]; kk < ms->row_index[ii+1]; kk++);
    if (ms->col_id[kk] == ii) {
      ms->diag_index[ii] = kk;
      break;
    }
    if (kk == ms->row_index[ii+1]) { /* if some rows have no diagonal value */
      BFT_FREE(ms->diag_index);
      return;
    }
  }

}
#endif

/*----------------------------------------------------------------------------
 * Create CSR matrix coefficients.
 *
 * returns:
 *   pointer to allocated CSR coefficients structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_coeff_csr_t *
_create_coeff_csr(void)
{
  cs_matrix_coeff_csr_t  *mc;

  /* Allocate */

  BFT_MALLOC(mc, 1, cs_matrix_coeff_csr_t);

  /* Initialize */

  mc->n_prefetch_rows = _cs_glob_matrix_prefetch_rows;

  mc->val = NULL;

  mc->x_prefetch = NULL;

  return mc;
}

/*----------------------------------------------------------------------------
 * Destroy CSR matrix coefficients.
 *
 * parameters:
 *   coeff  <->  Pointer to CSR matrix coefficients pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_coeff_csr(cs_matrix_coeff_csr_t **coeff)
{
  if (coeff != NULL && *coeff !=NULL) {

    cs_matrix_coeff_csr_t  *mc = *coeff;

    if (mc->val != NULL)
      BFT_FREE(mc->val);

    if (mc->x_prefetch != NULL)
      BFT_FREE(mc->x_prefetch);

    BFT_FREE(*coeff);

  }
}

/*----------------------------------------------------------------------------
 * Set CSR extradiagonal matrix coefficients for the case where direct
 * assignment is possible (i.e. when there are no multiple contributions
 * to a given coefficient).
 *
 * parameters:
 *   matrix      <-- Pointer to matrix structure
 *   symmetric   <-- Indicates if extradiagonal values are symmetric
 *   interleaved <-- Indicates if matrix coefficients are interleaved
 *   xa          <-- Extradiagonal values
 *----------------------------------------------------------------------------*/

static void
_set_xa_coeffs_csr_direct(cs_matrix_t      *matrix,
                          cs_bool_t         symmetric,
                          cs_bool_t         interleaved,
                          const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj, face_id;
  cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;

  /* Copy extra-diagonal values */

  assert(matrix->face_cell != NULL);

  if (symmetric == false) {

    const cs_int_t n_faces = matrix->n_faces;
    const cs_int_t *restrict face_cel_p = matrix->face_cell;

    const cs_real_t  *restrict xa1 = xa;
    const cs_real_t  *restrict xa2 = xa + matrix->n_faces;

    if (interleaved == false) {
      for (face_id = 0; face_id < n_faces; face_id++) {
        cs_int_t kk, ll;
        ii = *face_cel_p++ - 1;
        jj = *face_cel_p++ - 1;
        if (ii < ms->n_rows) {
          for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
          mc->val[kk] = xa1[face_id];
        }
        if (jj < ms->n_rows) {
          for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
          mc->val[ll] = xa2[face_id];
        }
      }
    }
    else { /* interleaved == true */
      for (face_id = 0; face_id < n_faces; face_id++) {
        cs_int_t kk, ll;
        ii = *face_cel_p++ - 1;
        jj = *face_cel_p++ - 1;
        if (ii < ms->n_rows) {
          for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
          mc->val[kk] = xa[2*face_id];
        }
        if (jj < ms->n_rows) {
          for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
          mc->val[ll] = xa[2*face_id + 1];
        }
      }
    }

  }
  else { /* if symmetric == true */

    const cs_int_t n_faces = matrix->n_faces;
    const cs_int_t *restrict face_cel_p = matrix->face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      cs_int_t kk, ll;
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      if (ii < ms->n_rows) {
        for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
        mc->val[kk] = xa[face_id];
      }
      if (jj < ms->n_rows) {
        for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
        mc->val[ll] = xa[face_id];
      }

    }

  } /* end of condition on coefficients symmetry */

}

/*----------------------------------------------------------------------------
 * Set CSR extradiagonal matrix coefficients for the case where there are
 * multiple contributions to a given coefficient).
 *
 * The matrix coefficients should have been initialized (i.e. set to 0)
 * some before using this function.
 *
 * parameters:
 *   matrix      <-- Pointer to matrix structure
 *   symmetric   <-- Indicates if extradiagonal values are symmetric
 *   interleaved <-- Indicates if matrix coefficients are interleaved
 *   xa          <-- Extradiagonal values
 *----------------------------------------------------------------------------*/

static void
_set_xa_coeffs_csr_increment(cs_matrix_t      *matrix,
                             cs_bool_t         symmetric,
                             cs_bool_t         interleaved,
                             const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj, face_id;
  cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;

  /* Copy extra-diagonal values */

  assert(matrix->face_cell != NULL);

  if (symmetric == false) {

    const cs_int_t n_faces = matrix->n_faces;
    const cs_int_t *restrict face_cel_p = matrix->face_cell;

    const cs_real_t  *restrict xa1 = xa;
    const cs_real_t  *restrict xa2 = xa + matrix->n_faces;

    if (interleaved == false) {
      for (face_id = 0; face_id < n_faces; face_id++) {
        cs_int_t kk, ll;
        ii = *face_cel_p++ - 1;
        jj = *face_cel_p++ - 1;
        if (ii < ms->n_rows) {
          for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
          mc->val[kk] += xa1[face_id];
        }
        if (jj < ms->n_rows) {
          for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
          mc->val[ll] += xa2[face_id];
        }
      }
    }
    else { /* interleaved == true */
      for (face_id = 0; face_id < n_faces; face_id++) {
        cs_int_t kk, ll;
        ii = *face_cel_p++ - 1;
        jj = *face_cel_p++ - 1;
        if (ii < ms->n_rows) {
          for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
          mc->val[kk] += xa[2*face_id];
        }
        if (jj < ms->n_rows) {
          for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
          mc->val[ll] += xa[2*face_id + 1];
        }
      }
    }
  }
  else { /* if symmetric == true */

    const cs_int_t n_faces = matrix->n_faces;
    const cs_int_t *restrict face_cel_p = matrix->face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      cs_int_t kk, ll;
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      if (ii < ms->n_rows) {
        for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
        mc->val[kk] += xa[face_id];
      }
      if (jj < ms->n_rows) {
        for (ll = ms->row_index[jj]; ms->col_id[ll] != ii; ll++);
        mc->val[ll] += xa[face_id];
      }

    }

  } /* end of condition on coefficients symmetry */

}

/*----------------------------------------------------------------------------
 * Set CSR matrix coefficients.
 *
 * parameters:
 *   matrix           <-> Pointer to matrix structure
 *   symmetric        <-- Indicates if extradiagonal values are symmetric
 *   interleaved      <-- Indicates if matrix coefficients are interleaved
 *   da               <-- Diagonal values (NULL if all zero)
 *   xa               <-- Extradiagonal values (NULL if all zero)
 *----------------------------------------------------------------------------*/

static void
_set_coeffs_csr(cs_matrix_t      *matrix,
                cs_bool_t         symmetric,
                cs_bool_t         interleaved,
                const cs_real_t  *restrict da,
                const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj;
  cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;

  if (mc->val == NULL)
    BFT_MALLOC(mc->val, ms->row_index[ms->n_rows], cs_real_t);

  /* Initialize coefficients to zero if assembly is incremental */

  if (ms->direct_assembly == false) {
    cs_int_t val_size = ms->row_index[ms->n_rows];
    for (ii = 0; ii < val_size; ii++)

      mc->val[ii] = 0.0;
  }

  /* Allocate prefetch buffer */

  if (mc->n_prefetch_rows > 0 && mc->x_prefetch == NULL) {
    size_t prefetch_size = ms->n_cols_max * mc->n_prefetch_rows;
    size_t matrix_size = matrix->n_cells + (2 * matrix->n_faces);
    if (matrix_size > prefetch_size)
      prefetch_size = matrix_size;
    BFT_MALLOC(mc->x_prefetch, prefetch_size, cs_real_t);
  }

  /* Copy diagonal values */

  if (ms->have_diag == true) {

    if (ms->diag_index == NULL) {

      if (da != NULL) {
        for (ii = 0; ii < ms->n_rows; ii++) {
          cs_int_t kk;
          for (kk = ms->row_index[ii]; ms->col_id[kk] != ii; kk++);
          mc->val[kk] = da[ii];
        }
      }
      else {
        for (ii = 0; ii < ms->n_rows; ii++) {
          cs_int_t kk;
          for (kk = ms->row_index[ii]; ms->col_id[kk] != ii; kk++);
          mc->val[kk] = 0.0;
        }
      }

    }
    else { /* If diagonal index is available, direct assignment */

      const cs_int_t *_diag_index = ms->diag_index;

      if (da != NULL) {
        for (ii = 0; ii < ms->n_rows; ii++)
          mc->val[_diag_index[ii]] = da[ii];
      }
      else {
        for (ii = 0; ii < ms->n_rows; ii++)
          mc->val[_diag_index[ii]] = 0.0;
      }

    }

  }

  /* Copy extra-diagonal values */

  if (matrix->face_cell != NULL) {

    if (xa != NULL) {

      if (ms->direct_assembly == true)
        _set_xa_coeffs_csr_direct(matrix, symmetric, interleaved, xa);
      else
        _set_xa_coeffs_csr_increment(matrix, symmetric, interleaved, xa);

    }
    else { /* if (xa == NULL) */

      for (ii = 0; ii < ms->n_rows; ii++) {
        const cs_int_t  *restrict col_id = ms->col_id + ms->row_index[ii];
        cs_real_t  *m_row = mc->val + ms->row_index[ii];
        cs_int_t  n_cols = ms->row_index[ii+1] - ms->row_index[ii];

        for (jj = 0; jj < n_cols; jj++) {
          if (col_id[jj] != ii)
            m_row[jj] = 0.0;
        }

      }

    }

  } /* (matrix->face_cell != NULL) */

}

/*----------------------------------------------------------------------------
 * Release CSR matrix coefficients.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *----------------------------------------------------------------------------*/

static void
_release_coeffs_csr(cs_matrix_t  *matrix)
{
  cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  if (mc !=NULL) {
    if (mc->val != NULL)
      BFT_FREE(mc->val);
  }

}

/*----------------------------------------------------------------------------
 * Get diagonal of CSR matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   da     --> Diagonal (pre-allocated, size: n_rows)
 *----------------------------------------------------------------------------*/

static void
_get_diagonal_csr(const cs_matrix_t  *matrix,
                  cs_real_t          *restrict da)
{
  cs_int_t  ii, jj;
  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  if (ms->have_diag == true) {

    if (ms->diag_index == NULL) {

      for (ii = 0; ii < n_rows; ii++) {

        const cs_int_t  *restrict col_id = ms->col_id + ms->row_index[ii];
        const cs_real_t  *restrict m_row = mc->val + ms->row_index[ii];
        cs_int_t  n_cols = ms->row_index[ii+1] - ms->row_index[ii];

        da[ii] = 0.0;
        for (jj = 0; jj < n_cols; jj++) {
          if (col_id[jj] == ii) {
            da[ii] = m_row[jj];
            break;
          }
        }

      }

    }

    else {

      const cs_int_t *diag_index = ms->diag_index;

      for (ii = 0; ii < n_rows; ii++)
        da[ii] = mc->val[diag_index[ii]];

    }

  }
  else { /* if (have_diag == false) */

    for (ii = 0; ii < n_rows; da[ii++] = 0.0);

  }

}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with CSR matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

#if !defined (HAVE_MKL)

static void
_mat_vec_p_l_csr(const cs_matrix_t  *matrix,
                 const cs_real_t    *restrict x,
                 cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, n_cols;
  cs_real_t  sii;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  /* Full rows for non-symmetric structure */

  #pragma omp parallel for private(ii, col_id, m_row, n_cols, sii)
  for (ii = 0; ii < n_rows; ii++) {

    col_id = ms->col_id + ms->row_index[ii];
    m_row = mc->val + ms->row_index[ii];
    n_cols = ms->row_index[ii+1] - ms->row_index[ii];
    sii = 0.0;

    /* Tell IBM compiler not to alias */
    #if defined(__xlc__)
    #pragma disjoint(*x, *y, *m_row, *col_id)
    #endif

    for (jj = 0; jj < n_cols; jj++)
      sii += (m_row[jj]*x[col_id[jj]]);

    y[ii] = sii;

  }

}

#else /* if defined (HAVE_MKL) */

static void
_mat_vec_p_l_csr(const cs_matrix_t  *matrix,
                 const cs_real_t    *restrict x,
                 cs_real_t          *restrict y)
{
  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  int n_rows = ms->n_rows;
  char transa[] = "n";

  mkl_cspblas_dcsrgemv(transa,
                       &n_rows,
                       mc->val,
                       ms->row_index,
                       ms->col_id,
                       (double *)x,
                       y);
}

#endif /* defined (HAVE_MKL) */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with CSR matrix (prefetch).
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_mat_vec_p_l_csr_pf(const cs_matrix_t  *matrix,
                    const cs_real_t    *restrict x,
                    cs_real_t          *restrict y)
{
  cs_int_t  start_row, ii, jj, n_cols;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  /* Outer loop on prefetch lines */

  for (start_row = 0; start_row < n_rows; start_row += mc->n_prefetch_rows) {

    cs_int_t end_row = start_row + mc->n_prefetch_rows;

    cs_real_t  *restrict prefetch_p = mc->x_prefetch;

    /* Tell IBM compiler not to alias */
    #if defined(__xlc__)
    #pragma disjoint(*prefetch_p, *y, *m_row)
    #pragma disjoint(*prefetch_p, *x, *col_id)
    #endif

    if (end_row > n_rows)
      end_row = n_rows;

    /* Prefetch */

    for (ii = start_row; ii < end_row; ii++) {

      col_id = ms->col_id + ms->row_index[ii];
      n_cols = ms->row_index[ii+1] - ms->row_index[ii];

      for (jj = 0; jj < n_cols; jj++)
        *prefetch_p++ = x[col_id[jj]];

    }

    /* Compute */

    prefetch_p = mc->x_prefetch;

    for (ii = start_row; ii < end_row; ii++) {

      cs_real_t  sii = 0.0;

      m_row = mc->val + ms->row_index[ii];
      n_cols = ms->row_index[ii+1] - ms->row_index[ii];

      for (jj = 0; jj < n_cols; jj++)
        sii += *m_row++ * *prefetch_p++;

      y[ii] = sii;

    }

  }

}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y with CSR matrix.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

#if !defined (HAVE_MKL)

static void
_alpha_a_x_p_beta_y_csr(cs_real_t           alpha,
                        cs_real_t           beta,
                        const cs_matrix_t  *matrix,
                        const cs_real_t    *restrict x,
                        cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, n_cols;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;
  cs_real_t  sii;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  #pragma omp parallel for private(ii, col_id, m_row, n_cols, sii)
  for (ii = 0; ii < n_rows; ii++) {

    col_id = ms->col_id + ms->row_index[ii];
    m_row = mc->val + ms->row_index[ii];
    n_cols = ms->row_index[ii+1] - ms->row_index[ii];
    sii = 0.0;

    /* Tell IBM compiler not to alias */
    #if defined(__xlc__)
    #pragma disjoint(*x, *y, *m_row, *col_id)
    #endif

    for (jj = 0; jj < n_cols; jj++)
      sii += (m_row[jj]*x[col_id[jj]]);

    y[ii] = (alpha * sii) + (beta * y[ii]);

  }

}

#else /* if defined (HAVE_MKL) */

static void
_alpha_a_x_p_beta_y_csr(cs_real_t           alpha,
                        cs_real_t           beta,
                        const cs_matrix_t  *matrix,
                        const cs_real_t    *restrict x,
                        cs_real_t          *restrict y)
{
  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;

  int n_rows = ms->n_rows;
  int n_cols = ms->n_cols;
  double _alpha = alpha;
  double _beta = beta;
  char mathdescra[7] = "G  C  ";
  char transa[] = "n";

  mkl_dcsrmv(transa,
             &n_rows,
             &n_cols,
             &_alpha,
             mathdescra,
             mc->val,
             ms->col_id,
             ms->row_index,
             ms->row_index + 1,
             (double *)x,
             &_beta,
             y);
}

#endif /* defined (HAVE_MKL) */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y
 * with CSR matrix (prefetch).
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

static void
_alpha_a_x_p_beta_y_csr_pf(cs_real_t           alpha,
                           cs_real_t           beta,
                           const cs_matrix_t  *matrix,
                           const cs_real_t    *restrict x,
                           cs_real_t          *restrict y)
{
  cs_int_t  start_row, ii, jj, n_cols;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;

  const cs_matrix_struct_csr_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  /* Outer loop on prefetch lines */

  for (start_row = 0; start_row < n_rows; start_row += mc->n_prefetch_rows) {

    cs_int_t end_row = start_row + mc->n_prefetch_rows;
    cs_real_t  *restrict prefetch_p = mc->x_prefetch;

    /* Tell IBM compiler not to alias */
    #if defined(__xlc__)
    #pragma disjoint(*prefetch_p, *x, *col_id)
    #pragma disjoint(*prefetch_p, *y, *m_row, *col_id)
    #endif

    if (end_row > n_rows)
      end_row = n_rows;

    /* Prefetch */

    for (ii = start_row; ii < end_row; ii++) {

      col_id = ms->col_id + ms->row_index[ii];
      n_cols = ms->row_index[ii+1] - ms->row_index[ii];

      for (jj = 0; jj < n_cols; jj++)
        *prefetch_p++ = x[col_id[jj]];

    }

    /* Compute */

    prefetch_p = mc->x_prefetch;

    for (ii = start_row; ii < end_row; ii++) {

      cs_real_t  sii = 0.0;

      m_row = mc->val + ms->row_index[ii];
      n_cols = ms->row_index[ii+1] - ms->row_index[ii];

      for (jj = 0; jj < n_cols; jj++)
        sii += *m_row++ * *prefetch_p++;

      y[ii] = (alpha * sii) + (beta * y[ii]);

    }

  }

}

/*----------------------------------------------------------------------------
 * Create a symmetric CSR matrix structure from a native matrix stucture.
 *
 * Note that the structure created maps global cell numbers to the given
 * existing face -> cell connectivity array, so it must be destroyed before
 * this array (usually the code's global cell numbering) is freed.
 *
 * parameters:
 *   have_diag   <-- Indicates if the diagonal is nonzero
 *                   (forced to true for symmetric variant)
 *   n_cells     <-- Local number of participating cells
 *   n_cells_ext <-- Local number of cells + ghost cells sharing a face
 *   n_faces     <-- Local number of faces
 *   cell_num    <-- Global cell numbers (1 to n)
 *   face_cell   <-- Face -> cells connectivity (1 to n)
 *
 * returns:
 *   pointer to allocated CSR matrix structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_struct_csr_sym_t *
_create_struct_csr_sym(cs_bool_t         have_diag,
                       int               n_cells,
                       int               n_cells_ext,
                       int               n_faces,
                       const cs_int_t   *face_cell)
{
  int n_cols_max;
  cs_int_t ii, jj, face_id;
  const cs_int_t *restrict face_cel_p;

  cs_int_t  diag_elts = 1;
  cs_int_t  *ccount = NULL;

  cs_matrix_struct_csr_sym_t  *ms;

  /* Allocate and map */

  BFT_MALLOC(ms, 1, cs_matrix_struct_csr_sym_t);

  ms->n_rows = n_cells;
  ms->n_cols = n_cells_ext;

  ms->have_diag = have_diag;
  ms->direct_assembly = true;

  BFT_MALLOC(ms->row_index, ms->n_rows + 1, cs_int_t);
  ms->row_index = ms->row_index;

  /* Count number of nonzero elements per row */

  BFT_MALLOC(ccount, ms->n_cols, cs_int_t);

  if (have_diag == false)
    diag_elts = 0;

  for (ii = 0; ii < ms->n_rows; ii++)  /* count starting with diagonal terms */
    ccount[ii] = diag_elts;

  if (face_cell != NULL) {

    face_cel_p = face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      if (ii < jj)
        ccount[ii] += 1;
      else
        ccount[jj] += 1;
    }

  } /* if (face_cell != NULL) */

  n_cols_max = 0;

  ms->row_index[0] = 0;
  for (ii = 0; ii < ms->n_rows; ii++) {
    ms->row_index[ii+1] = ms->row_index[ii] + ccount[ii];
    if (ccount[ii] > n_cols_max)
      n_cols_max = ccount[ii];
    ccount[ii] = diag_elts; /* pre-count for diagonal terms */
  }

  ms->n_cols_max = n_cols_max;

  /* Build structure */

  BFT_MALLOC(ms->col_id, (ms->row_index[ms->n_rows]), cs_int_t);
  ms->col_id = ms->col_id;

  if (have_diag == true) {
    for (ii = 0; ii < ms->n_rows; ii++) {    /* diagonal terms */
      ms->col_id[ms->row_index[ii]] = ii;
    }
  }

  if (face_cell != NULL) {                   /* non-diagonal terms */

    face_cel_p = face_cell;

    for (face_id = 0; face_id < n_faces; face_id++) {
      ii = *face_cel_p++ - 1;
      jj = *face_cel_p++ - 1;
      if (ii < jj && ii < ms->n_rows) {
        ms->col_id[ms->row_index[ii] + ccount[ii]] = jj;
        ccount[ii] += 1;
      }
      else if (ii > jj && jj < ms->n_rows) {
        ms->col_id[ms->row_index[jj] + ccount[jj]] = ii;
        ccount[jj] += 1;
      }
    }

  }

  /* Compact elements if necessary */

  if (ms->direct_assembly == false) {

    cs_int_t *tmp_row_index = NULL;
    cs_int_t  kk = 0;

    BFT_MALLOC(tmp_row_index, ms->n_rows+1, cs_int_t);
    memcpy(tmp_row_index, ms->row_index, (ms->n_rows+1)*sizeof(cs_int_t));

    kk = 0;

    for (ii = 0; ii < ms->n_rows; ii++) {
      cs_int_t *col_id = ms->col_id + ms->row_index[ii];
      cs_int_t n_cols = ms->row_index[ii+1] - ms->row_index[ii];
      cs_int_t col_id_prev = -1;
      ms->row_index[ii] = kk;
      for (jj = 0; jj < n_cols; jj++) {
        if (col_id_prev != col_id[jj]) {
          ms->col_id[kk++] = col_id[jj];
          col_id_prev = col_id[jj];
        }
      }
    }
    ms->row_index[ms->n_rows] = kk;

    assert(ms->row_index[ms->n_rows] < tmp_row_index[ms->n_rows]);

    BFT_FREE(tmp_row_index);
    BFT_REALLOC(ms->col_id, (ms->row_index[ms->n_rows]), cs_int_t);

  }

  return ms;
}

/*----------------------------------------------------------------------------
 * Destroy symmetric CSR matrix structure.
 *
 * parameters:
 *   matrix  <->  Pointer to CSR matrix structure pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_struct_csr_sym(cs_matrix_struct_csr_sym_t **matrix)
{
  if (matrix != NULL && *matrix !=NULL) {

    cs_matrix_struct_csr_sym_t  *ms = *matrix;

    if (ms->row_index != NULL)
      BFT_FREE(ms->row_index);

    if (ms->col_id != NULL)
      BFT_FREE(ms->col_id);

    BFT_FREE(ms);

    *matrix = ms;

  }
}

/*----------------------------------------------------------------------------
 * Create symmetric CSR matrix coefficients.
 *
 * returns:
 *   pointer to allocated CSR coefficients structure.
 *----------------------------------------------------------------------------*/

static cs_matrix_coeff_csr_sym_t *
_create_coeff_csr_sym(void)
{
  cs_matrix_coeff_csr_sym_t  *mc;

  /* Allocate */

  BFT_MALLOC(mc, 1, cs_matrix_coeff_csr_sym_t);

  /* Initialize */

  mc->val = NULL;

  return mc;
}

/*----------------------------------------------------------------------------
 * Destroy symmetric CSR matrix coefficients.
 *
 * parameters:
 *   coeff  <->  Pointer to CSR matrix coefficients pointer
 *----------------------------------------------------------------------------*/

static void
_destroy_coeff_csr_sym(cs_matrix_coeff_csr_sym_t **coeff)
{
  if (coeff != NULL && *coeff !=NULL) {

    cs_matrix_coeff_csr_sym_t  *mc = *coeff;

    if (mc->val != NULL)
      BFT_FREE(mc->val);

    BFT_FREE(*coeff);

  }
}

/*----------------------------------------------------------------------------
 * Set symmetric CSR extradiagonal matrix coefficients for the case where
 * direct assignment is possible (i.e. when there are no multiple
 * contributions to a given coefficient).
 *
 * parameters:
 *   matrix    <-- Pointer to matrix structure
 *   xa        <-- Extradiagonal values
 *----------------------------------------------------------------------------*/

static void
_set_xa_coeffs_csr_sym_direct(cs_matrix_t      *matrix,
                              const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj, face_id;
  cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_int_t n_faces = matrix->n_faces;
  const cs_int_t *restrict face_cel_p = matrix->face_cell;

  /* Copy extra-diagonal values */

  assert(matrix->face_cell != NULL);

  for (face_id = 0; face_id < n_faces; face_id++) {
    cs_int_t kk;
    ii = *face_cel_p++ - 1;
    jj = *face_cel_p++ - 1;
    if (ii < jj && ii < ms->n_rows) {
      for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
      mc->val[kk] = xa[face_id];
    }
    else if (ii > jj && jj < ms->n_rows) {
      for (kk = ms->row_index[jj]; ms->col_id[kk] != ii; kk++);
      mc->val[kk] = xa[face_id];
    }
  }
}

/*----------------------------------------------------------------------------
 * Set symmetric CSR extradiagonal matrix coefficients for the case where
 * there are multiple contributions to a given coefficient).
 *
 * The matrix coefficients should have been initialized (i.e. set to 0)
 * some before using this function.
 *
 * parameters:
 *   matrix    <-- Pointer to matrix structure
 *   symmetric <-- Indicates if extradiagonal values are symmetric
 *   xa        <-- Extradiagonal values
 *----------------------------------------------------------------------------*/

static void
_set_xa_coeffs_csr_sym_increment(cs_matrix_t      *matrix,
                                 const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj, face_id;
  cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_int_t n_faces = matrix->n_faces;
  const cs_int_t *restrict face_cel_p = matrix->face_cell;

  /* Copy extra-diagonal values */

  assert(matrix->face_cell != NULL);

  for (face_id = 0; face_id < n_faces; face_id++) {
    cs_int_t kk;
    ii = *face_cel_p++ - 1;
    jj = *face_cel_p++ - 1;
    if (ii < jj && ii < ms->n_rows) {
      for (kk = ms->row_index[ii]; ms->col_id[kk] != jj; kk++);
      mc->val[kk] += xa[face_id];
    }
    else if (ii > jj && jj < ms->n_rows) {
      for (kk = ms->row_index[jj]; ms->col_id[kk] != ii; kk++);
      mc->val[kk] += xa[face_id];
    }
  }
}

/*----------------------------------------------------------------------------
 * Set symmetric CSR matrix coefficients.
 *
 * parameters:
 *   matrix           <-> Pointer to matrix structure
 *   symmetric        <-- Indicates if extradiagonal values are symmetric (true)
 *   interleaved      <-- Indicates if matrix coefficients are interleaved
 *   da               <-- Diagonal values (NULL if all zero)
 *   xa               <-- Extradiagonal values (NULL if all zero)
 *----------------------------------------------------------------------------*/

static void
_set_coeffs_csr_sym(cs_matrix_t      *matrix,
                    cs_bool_t         symmetric,
                    cs_bool_t         interleaved,
                    const cs_real_t  *restrict da,
                    const cs_real_t  *restrict xa)
{
  cs_int_t  ii, jj;
  cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;

  if (mc->val == NULL)
    BFT_MALLOC(mc->val, ms->row_index[ms->n_rows], cs_real_t);

  /* Initialize coefficients to zero if assembly is incremental */

  if (ms->direct_assembly == false) {
    cs_int_t val_size = ms->row_index[ms->n_rows];
    for (ii = 0; ii < val_size; ii++)
      mc->val[ii] = 0.0;
  }

  /* Copy diagonal values */

  if (ms->have_diag == true) {

    const cs_int_t *_diag_index = ms->row_index;

    if (da != NULL) {
      for (ii = 0; ii < ms->n_rows; ii++)
        mc->val[_diag_index[ii]] = da[ii];
    }
    else {
      for (ii = 0; ii < ms->n_rows; ii++)
        mc->val[_diag_index[ii]] = 0.0;
    }

  }

  /* Copy extra-diagonal values */

  if (matrix->face_cell != NULL) {

    if (xa != NULL) {

      if (symmetric == false)
        bft_error(__FILE__, __LINE__, 0,
                  _("Assigning non-symmetric matrix coefficients to a matrix\n"
                    "in a symmetric CSR format."));

      if (ms->direct_assembly == true)
        _set_xa_coeffs_csr_sym_direct(matrix, xa);
      else
        _set_xa_coeffs_csr_sym_increment(matrix, xa);

    }
    else { /* if (xa == NULL) */

      for (ii = 0; ii < ms->n_rows; ii++) {
        const cs_int_t  *restrict col_id = ms->col_id + ms->row_index[ii];
        cs_real_t  *m_row = mc->val + ms->row_index[ii];
        cs_int_t  n_cols = ms->row_index[ii+1] - ms->row_index[ii];

        for (jj = 0; jj < n_cols; jj++) {
          if (col_id[jj] != ii)
            m_row[jj] = 0.0;
        }

      }

    }

  } /* (matrix->face_cell != NULL) */

}

/*----------------------------------------------------------------------------
 * Release symmetric CSR matrix coefficients.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *----------------------------------------------------------------------------*/

static void
_release_coeffs_csr_sym(cs_matrix_t  *matrix)
{
  cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  if (mc !=NULL) {
    if (mc->val != NULL)
      BFT_FREE(mc->val);
  }

}

/*----------------------------------------------------------------------------
 * Get diagonal of symmetric CSR matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   da     --> Diagonal (pre-allocated, size: n_rows)
 *----------------------------------------------------------------------------*/

static void
_get_diagonal_csr_sym(const cs_matrix_t  *matrix,
                      cs_real_t          *restrict da)
{
  cs_int_t  ii;
  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  if (ms->have_diag == true) {

    /* As structure is symmetric, diagonal values appear first,
       so diag_index == row_index */

    const cs_int_t *diag_index = ms->row_index;

    for (ii = 0; ii < n_rows; ii++)
      da[ii] = mc->val[diag_index[ii]];

  }
  else { /* if (have_diag == false) */

    for (ii = 0; ii < n_rows; da[ii++] = 0.0);

  }

}

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = A.x with symmetric CSR matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

#if !defined (HAVE_MKL)

static void
_mat_vec_p_l_csr_sym(const cs_matrix_t   *matrix,
                     const cs_real_t     *restrict x,
                     cs_real_t           *restrict y)
{
  cs_int_t  ii, jj, n_cols;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;

  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  cs_int_t sym_jj_start = 0;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *m_row, *col_id)
  #endif

  /* By construction, the matrix has either a full or an empty
     diagonal structure, so testing this on the first row is enough */

  for (ii = ms->row_index[0]; ii < ms->row_index[1]; ii++) {
    if (ms->col_id[ii] == 0)
      sym_jj_start = 1;
  }

  /* Initialize y */

  for (ii = 0; ii < ms->n_cols; ii++)
    y[ii] = 0.0;

  /* Upper triangular + diagonal part in case of symmetric structure */

  for (ii = 0; ii < n_rows; ii++) {

    cs_real_t  sii = 0.0;

    col_id = ms->col_id + ms->row_index[ii];
    m_row = mc->val + ms->row_index[ii];
    n_cols = ms->row_index[ii+1] - ms->row_index[ii];

    for (jj = 0; jj < n_cols; jj++)
      sii += (m_row[jj]*x[col_id[jj]]);

    y[ii] += sii;

    for (jj = sym_jj_start; jj < n_cols; jj++)
      y[col_id[jj]] += (m_row[jj]*x[ii]);
  }

}

#else /* if defined (HAVE_MKL) */

static void
_mat_vec_p_l_csr_sym(const cs_matrix_t  *matrix,
                     const cs_real_t    *restrict x,
                     cs_real_t          *restrict y)
{
  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  int n_rows = ms->n_rows;
  char uplo[] = "u";

  mkl_cspblas_dcsrsymv(uplo,
                       &n_rows,
                       mc->val,
                       ms->row_index,
                       ms->col_id,
                       (double *)x,
                       y);
}

#endif /* defined (HAVE_MKL) */

/*----------------------------------------------------------------------------
 * Local matrix.vector product y = alpha.A.x + beta.y
 * with symmetric CSR matrix.
 *
 * parameters:
 *   alpha  <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta   <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      <-> Resulting vector
 *----------------------------------------------------------------------------*/

#if !defined (HAVE_MKL)

static void
_alpha_a_x_p_beta_y_csr_sym(cs_real_t           alpha,
                            cs_real_t           beta,
                            const cs_matrix_t  *matrix,
                            const cs_real_t    *restrict x,
                            cs_real_t          *restrict y)
{
  cs_int_t  ii, jj, n_cols;
  cs_int_t  *restrict col_id;
  cs_real_t  *restrict m_row;

  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;
  cs_int_t  n_rows = ms->n_rows;

  /* Tell IBM compiler not to alias */
  #if defined(__xlc__)
  #pragma disjoint(*x, *y, *m_row, *col_id)
  #endif

  for (ii = 0; ii < ms->n_rows; ii++)
    y[ii] *= beta;

  for (ii = ms->n_rows; ii < ms->n_cols; ii++)
    y[ii] = 0.0;

  /* Upper triangular + diagonal part in case of symmetric structure */

  for (ii = 0; ii < n_rows; ii++) {

    cs_real_t  sii = 0.0;

    col_id = ms->col_id + ms->row_index[ii];
    m_row = mc->val + ms->row_index[ii];
    n_cols = ms->row_index[ii+1] - ms->row_index[ii];

    for (jj = 0; jj < n_cols; jj++)
      sii += (m_row[jj]*x[col_id[jj]]);

    y[ii] += alpha * sii;

    for (jj = 1; jj < n_cols; jj++)
      y[col_id[jj]] += alpha * (m_row[jj]*x[ii]);

  }

}

#else /* if defined (HAVE_MKL) */

static void
_alpha_a_x_p_beta_y_csr_sym(cs_real_t           alpha,
                            cs_real_t           beta,
                            const cs_matrix_t  *matrix,
                            const cs_real_t    *restrict x,
                            cs_real_t          *restrict y)
{
  const cs_matrix_struct_csr_sym_t  *ms = matrix->structure;
  const cs_matrix_coeff_csr_sym_t  *mc = matrix->coeffs;

  int n_rows = ms->n_rows;
  int n_cols = ms->n_cols;
  double _alpha = alpha;
  double _beta = beta;
  char transa[] = "n";
  char mathdescra[7] = "TUNC  ";

  mkl_dcsrmv(transa,
             &n_rows,
             &n_cols,
             &_alpha,
             mathdescra,
             mc->val,
             ms->col_id,
             ms->row_index,
             ms->row_index + 1,
             (double *)x,
             &_beta,
             y);
}

#endif /* defined (HAVE_MKL) */

/*============================================================================
 *  Public function definitions for Fortran API
 *============================================================================*/

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Create a matrix Structure.
 *
 * Note that the structure created maps to the given existing
 * cell global number, face -> cell connectivity arrays, and cell halo
 * structure, so it must be destroyed before they are freed
 * (usually along with the code's main face -> cell structure).
 *
 * Note that the resulting matrix structure will contain either a full or
 * an empty main diagonal, and that the extra-diagonal structure is always
 * symmetric (though the coefficients my not be, and we may choose a
 * matrix format that does not exploit ths symmetry). If the face_cell
 * connectivity argument is NULL, the matrix will be purely diagonal.
 *
 * parameters:
 *   type        <-- Type of matrix considered
 *   have_diag   <-- Indicates if the diagonal structure contains nonzeroes
 *   n_cells     <-- Local number of cells
 *   n_cells_ext <-- Local number of cells + ghost cells sharing a face
 *   n_faces     <-- Local number of internal faces
 *   cell_num    <-- Global cell numbers (1 to n)
 *   face_cell   <-- Face -> cells connectivity (1 to n)
 *   halo        <-- Halo structure associated with cells, or NULL
 *   numbering   <-- vectorization or thread-related numbering info, or NULL
 *
 * returns:
 *   pointer to created matrix structure;
 *----------------------------------------------------------------------------*/

cs_matrix_structure_t *
cs_matrix_structure_create(cs_matrix_type_t       type,
                           cs_bool_t              have_diag,
                           cs_int_t               n_cells,
                           cs_int_t               n_cells_ext,
                           cs_int_t               n_faces,
                           const fvm_gnum_t      *cell_num,
                           const cs_int_t        *face_cell,
                           const cs_halo_t       *halo,
                           const cs_numbering_t  *numbering)
{
  cs_matrix_structure_t *ms;

  BFT_MALLOC(ms, 1, cs_matrix_structure_t);

  ms->type = type;

  ms->n_cells = n_cells;
  ms->n_cells_ext = n_cells_ext;
  ms->n_faces = n_faces;

  /* Define Structure */

  switch(ms->type) {
  case CS_MATRIX_NATIVE:
    ms->structure = _create_struct_native(n_cells,
                                          n_cells_ext,
                                          n_faces,
                                          face_cell);
    break;
  case CS_MATRIX_CSR:
    ms->structure = _create_struct_csr(have_diag,
                                       n_cells,
                                       n_cells_ext,
                                       n_faces,
                                       face_cell);
    break;
  case CS_MATRIX_CSR_SYM:
    ms->structure = _create_struct_csr_sym(have_diag,
                                           n_cells,
                                           n_cells_ext,
                                           n_faces,
                                           face_cell);
    break;
  default:
    bft_error(__FILE__, __LINE__, 0,
              _("Handling of matrixes in %s format\n"
                "is not operational yet."),
              _(cs_matrix_type_name[type]));
    break;
  }

  /* Set pointers to structures shared from mesh here */

  ms->face_cell = face_cell;
  ms->cell_num = cell_num;
  ms->halo = halo;
  ms->numbering = numbering;

  return ms;
}

/*----------------------------------------------------------------------------
 * Destroy a matrix structure.
 *
 * parameters:
 *   ms <-> Pointer to matrix structure pointer
 *----------------------------------------------------------------------------*/

void
cs_matrix_structure_destroy(cs_matrix_structure_t  **ms)
{
  if (ms != NULL && *ms != NULL) {

    cs_matrix_structure_t *_ms = *ms;

    switch(_ms->type) {
    case CS_MATRIX_NATIVE:
      {
        cs_matrix_struct_native_t *structure = _ms->structure;
        _destroy_struct_native(&structure);
      }
      break;
    case CS_MATRIX_CSR:
      {
        cs_matrix_struct_csr_t *structure = _ms->structure;
        _destroy_struct_csr(&structure);
      }
      break;
    case CS_MATRIX_CSR_SYM:
      {
        cs_matrix_struct_csr_sym_t *structure = _ms->structure;
        _destroy_struct_csr_sym(&structure);
      }
      break;
    default:
      assert(0);
      break;
    }
    _ms->structure = NULL;

    /* Now free main structure */

    BFT_FREE(*ms);
  }
}

/*----------------------------------------------------------------------------
 * Create a matrix container using a given structure.
 *
 * Note that the matrix container maps to the assigned structure,
 * so it must be destroyed before that structure.
 *
 * parameters:
 *   ms <-- Associated matrix structure
 *
 * returns:
 *   pointer to created matrix structure;
 *----------------------------------------------------------------------------*/

cs_matrix_t *
cs_matrix_create(const cs_matrix_structure_t  *ms)
{
  int i;
  cs_matrix_t *m;

  BFT_MALLOC(m, 1, cs_matrix_t);

  m->type = ms->type;

  /* Map shared structure */

  m->n_cells = ms->n_cells;
  m->n_cells_ext = ms->n_cells_ext;
  m->n_faces = ms->n_faces;

  for (i = 0; i < 4; i++)
    m->b_size[i] = 1;

  m->structure = ms->structure;

  m->face_cell = ms->face_cell;
  m->cell_num = ms->cell_num;
  m->halo = ms->halo;
  m->numbering = ms->numbering;

  /* Define coefficients */

  switch(m->type) {
  case CS_MATRIX_NATIVE:
    m->coeffs = _create_coeff_native();
    break;
  case CS_MATRIX_CSR:
    m->coeffs = _create_coeff_csr();
    break;
  case CS_MATRIX_CSR_SYM:
    m->coeffs = _create_coeff_csr_sym();
    break;
  default:
    bft_error(__FILE__, __LINE__, 0,
              _("Handling of matrixes in %s format\n"
                "is not operational yet."),
              _(cs_matrix_type_name[m->type]));
    break;
  }

  /* Set function pointers here */

  switch(m->type) {

  case CS_MATRIX_NATIVE:

    m->set_coefficients = _set_coeffs_native;
    m->release_coefficients = _release_coeffs_native;
    m->get_diagonal = _get_diagonal_native;
    m->vector_multiply = _mat_vec_p_l_native;
    m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_native;
    m->b_vector_multiply = _b_mat_vec_p_l_native;
    m->b_alpha_a_x_p_beta_y = _b_alpha_a_x_p_beta_y_native;

    /* Optimized variants here */

#if defined(IA64_OPTIM)
    m->vector_multiply = _mat_vec_p_l_native_ia64;
#endif

    if (m->numbering != NULL) {
#if defined(HAVE_OPENMP)
      if (m->numbering->type == CS_NUMBERING_THREADS) {
        m->vector_multiply = _mat_vec_p_l_native_omp;
        m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_native_omp;
        m->b_vector_multiply = _b_mat_vec_p_l_native_omp;
        m->b_alpha_a_x_p_beta_y = _b_alpha_a_x_p_beta_y_native_omp;
      }
#endif
#if defined(SX) && defined(_SX) /* For vector machines */
      if (m->numbering->type == CS_NUMBERING_VECTORIZE) {
        m->vector_multiply = _mat_vec_p_l_native_vector;
        m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_native_vector;
      }
#endif
    }

    break;

  case CS_MATRIX_CSR:
    m->set_coefficients = _set_coeffs_csr;
    m->release_coefficients = _release_coeffs_csr;
    m->get_diagonal = _get_diagonal_csr;
    if (_cs_glob_matrix_prefetch_rows > 0 && cs_glob_n_threads == 1) {
      m->vector_multiply = _mat_vec_p_l_csr_pf;
      m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_csr_pf;
    }
    else {
      m->vector_multiply = _mat_vec_p_l_csr;
      m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_csr;
    }
    break;

  case CS_MATRIX_CSR_SYM:
    m->set_coefficients = _set_coeffs_csr_sym;
    m->release_coefficients = _release_coeffs_csr_sym;
    m->get_diagonal = _get_diagonal_csr_sym;
    m->vector_multiply = _mat_vec_p_l_csr_sym;
    m->alpha_a_x_p_beta_y = _alpha_a_x_p_beta_y_csr_sym;
    break;

  default:
    assert(0);
    m->set_coefficients = NULL;
    m->vector_multiply = NULL;
    m->alpha_a_x_p_beta_y = NULL;
    m->b_vector_multiply = NULL;
    m->b_alpha_a_x_p_beta_y = NULL;

  }

  return m;
}

/*----------------------------------------------------------------------------
 * Destroy a matrix structure.
 *
 * parameters:
 *   matrix <-> Pointer to matrix structure pointer
 *----------------------------------------------------------------------------*/

void
cs_matrix_destroy(cs_matrix_t **matrix)
{
  if (matrix != NULL && *matrix != NULL) {

    cs_matrix_t *m = *matrix;

    switch(m->type) {
    case CS_MATRIX_NATIVE:
      {
        cs_matrix_coeff_native_t *coeffs = m->coeffs;
        _destroy_coeff_native(&coeffs);
      }
      break;
    case CS_MATRIX_CSR:
      {
        cs_matrix_coeff_csr_t *coeffs = m->coeffs;
        _destroy_coeff_csr(&coeffs);
        m->coeffs = NULL;
      }
      break;
    case CS_MATRIX_CSR_SYM:
      {
        cs_matrix_coeff_csr_sym_t *coeffs = m->coeffs;
        _destroy_coeff_csr_sym(&coeffs);
        m->coeffs = NULL;
      }
      break;
    default:
      assert(0);
      break;
    }

    m->coeffs = NULL;

    /* Now free main structure */

    BFT_FREE(*matrix);
  }
}

/*----------------------------------------------------------------------------
 * Return number of columns in matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *----------------------------------------------------------------------------*/

cs_int_t
cs_matrix_get_n_columns(const cs_matrix_t  *matrix)
{
  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));
  return matrix->n_cells_ext;
}

/*----------------------------------------------------------------------------
 * Return number of rows in matrix.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *----------------------------------------------------------------------------*/

cs_int_t
cs_matrix_get_n_rows(const cs_matrix_t  *matrix)
{
  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));
  return matrix->n_cells;
}

/*----------------------------------------------------------------------------
 * Return matrix diagonal block sizes.
 *
 * Block sizes are defined by a array of 4 values:
 *   0: useful block size, 1: vector block extents,
 *   2: matrix line extents,  3: matrix line*column extents
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *
 * returns:
 *   pointer to block sizes
 *----------------------------------------------------------------------------*/

const int *
cs_matrix_get_diag_block_size(const cs_matrix_t  *matrix)
{
  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));
  if (matrix->type != CS_MATRIX_NATIVE)
    bft_error(__FILE__, __LINE__, 0,
              _("Not supported with CSR"));

  return matrix->b_size;
}

/*----------------------------------------------------------------------------
 * Set matrix coefficients.
 *
 * Depending on current options and initialization, values will be copied
 * or simply mapped.
 *
 * Block sizes are defined by an optional array of 4 values:
 *   0: useful block size, 1: vector block extents,
 *   2: matrix line extents,  3: matrix line*column extents
 *
 * parameters:
 *   matrix           <-> Pointer to matrix structure
 *   symmetric        <-- Indicates if matrix coefficients are symmetric
 *   diag_block_size  <-- Block sizes for diagonal, or NULL
 *   da               <-- Diagonal values (NULL if zero)
 *   xa               <-- Extradiagonal values (NULL if zero)
 *----------------------------------------------------------------------------*/

void
cs_matrix_set_coefficients(cs_matrix_t      *matrix,
                           cs_bool_t         symmetric,
                           const int        *diag_block_size,
                           const cs_real_t  *da,
                           const cs_real_t  *xa)
{
  int i;

  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));

  if (diag_block_size == NULL) {
    for (i = 0; i < 4; i++)
      matrix->b_size[i] = 1;
  }
  else {
    for (i = 0; i < 4; i++)
      matrix->b_size[i] = diag_block_size[i];
  }

  if (matrix->set_coefficients != NULL)
    matrix->set_coefficients(matrix, symmetric, true, da, xa);
}

/*----------------------------------------------------------------------------
 * Set matrix coefficients in the non-interleaved case.
 *
 * In the symmetric case, there is no difference with the interleaved case.
 *
 * Depending on current options and initialization, values will be copied
 * or simply mapped.
 *
 * parameters:
 *   matrix    <-> Pointer to matrix structure
 *   symmetric <-- Indicates if matrix coefficients are symmetric
 *   da        <-- Diagonal values (NULL if zero)
 *   xa        <-- Extradiagonal values (NULL if zero)
 *----------------------------------------------------------------------------*/

void
cs_matrix_set_coefficients_ni(cs_matrix_t      *matrix,
                              cs_bool_t         symmetric,
                              const cs_real_t  *da,
                              const cs_real_t  *xa)
{
  int i;

  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));

  for (i = 0; i < 4; i++)
    matrix->b_size[i] = 1;

  if (matrix->set_coefficients != NULL)
    matrix->set_coefficients(matrix, symmetric, false, da, xa);
}

/*----------------------------------------------------------------------------
 * Release matrix coefficients.
 *
 * parameters:
 *   matrix <-> Pointer to matrix structure
 *----------------------------------------------------------------------------*/

void
cs_matrix_release_coefficients(cs_matrix_t  *matrix)

{
  /* Check API state */

  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));

  if (matrix->release_coefficients != NULL)
    matrix->release_coefficients(matrix);
}

/*----------------------------------------------------------------------------
 * Get matrix diagonal values.
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   da     --> Diagonal (pre-allocated, size: n_cells)
 *----------------------------------------------------------------------------*/

void
cs_matrix_get_diagonal(const cs_matrix_t  *matrix,
                       cs_real_t          *restrict da)
{
  /* Check API state */

  if (matrix == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("The matrix is not defined."));

  if (matrix->get_diagonal != NULL)
    matrix->get_diagonal(matrix, da);
}

/*----------------------------------------------------------------------------
 * Matrix.vector product y = A.x
 *
 * This function includes a halo update of x prior to multiplication by A.
 *
 * parameters:
 *   rotation_mode <-- Halo update option for rotational periodicity
 *   matrix        <-- Pointer to matrix structure
 *   x             <-> Multipliying vector values (ghost values updated)
 *   y             --> Resulting vector
 *----------------------------------------------------------------------------*/

void
cs_matrix_vector_multiply(cs_perio_rota_t     rotation_mode,
                          const cs_matrix_t  *matrix,
                          cs_real_t          *restrict x,
                          cs_real_t          *restrict y)
{
  size_t ii;
  size_t n_cells_ext = matrix->n_cells_ext;

  /* Non-blocked version */

  if (matrix->b_size[3] == 1) {

    /* Synchronize for parallelism and periodicity first */

    for (ii = matrix->n_cells; ii < n_cells_ext; y[ii++] = 0.);

    /* Update distant ghost cells */

    if (matrix->halo != NULL) {

      cs_halo_sync_var(matrix->halo, CS_HALO_STANDARD, x);

      /* Synchronize periodic values */

      if (matrix->halo->n_transforms > 0) {
        if (rotation_mode == CS_PERIO_ROTA_IGNORE)
          bft_error(__FILE__, __LINE__, 0, _cs_glob_perio_ignore_error_str);
        cs_perio_sync_var_scal(matrix->halo, CS_HALO_STANDARD, rotation_mode, x);
      }

    }

    /* Now call local matrix.vector product */

    if (matrix->vector_multiply != NULL)
      matrix->vector_multiply(matrix, x, y);

    else if (matrix->alpha_a_x_p_beta_y != NULL)
      matrix->alpha_a_x_p_beta_y(1.0, 0.0, matrix, x, y);

  }

  /* Blocked version */

  else { /* if (matrix->b_size[3] > 1) */

    const int *b_size = matrix->b_size;

    /* Synchronize for parallelism and periodicity first */

    _b_zero_range(y, matrix->n_cells, n_cells_ext, b_size);

    /* Update distant ghost cells */

    if (matrix->halo != NULL) {

      cs_halo_sync_var_strided(matrix->halo,
                               CS_HALO_STANDARD,
                               x,
                               b_size[1]);

      /* Synchronize periodic values */

      if (matrix->halo->n_transforms > 0 && b_size[0] == 3) {
        cs_perio_sync_var_vect(matrix->halo, CS_HALO_STANDARD, x, b_size[1]);
      }

    }

    /* Now call local matrix.vector product */

    if (matrix->b_vector_multiply != NULL)
      matrix->b_vector_multiply(matrix, x, y);

    else if (matrix->b_alpha_a_x_p_beta_y != NULL)
      matrix->b_alpha_a_x_p_beta_y(1.0, 0.0, matrix, x, y);

  }
}

/*----------------------------------------------------------------------------
 * Matrix.vector product y = A.x with no prior halo update of x.
 *
 * This function does not include a halo update of x prior to multiplication
 * by A, so it should be called only when the halo of x is known to already
 * be up to date (in which case we avoid the performance penalty of a
 * redundant update by using this variant of the matrix.vector product).
 *
 * parameters:
 *   matrix <-- Pointer to matrix structure
 *   x      <-- Multipliying vector values
 *   y      --> Resulting vector
 *----------------------------------------------------------------------------*/

void
cs_matrix_vector_multiply_nosync(const cs_matrix_t  *matrix,
                                 const cs_real_t    *x,
                                 cs_real_t          *restrict y)
{
  if (matrix != NULL) {

    /* Non-blocked version */

    if (matrix->b_size[3] == 1) {

      if (matrix->vector_multiply != NULL)
        matrix->vector_multiply(matrix, x, y);

      else if (matrix->alpha_a_x_p_beta_y != NULL)
        matrix->alpha_a_x_p_beta_y(1.0, 0.0, matrix, x, y);

    }

    /* Blocked version */

    else { /* if (matrix->b_size[3] > 1) */

      if (matrix->vector_multiply != NULL)
        matrix->b_vector_multiply(matrix, x, y);

      else if (matrix->alpha_a_x_p_beta_y != NULL)
        matrix->b_alpha_a_x_p_beta_y(1.0, 0.0, matrix, x, y);

    }

  }
}

/*----------------------------------------------------------------------------
 * Matrix.vector product y = alpha.A.x + beta.y
 *
 * This function includes a halo update of x prior to multiplication by A.
 *
 * parameters:
 *   rotation_mode <-- Halo update option for rotational periodicity
 *   alpha         <-- Scalar, alpha in alpha.A.x + beta.y
 *   beta          <-- Scalar, beta in alpha.A.x + beta.y
 *   matrix        <-- Pointer to matrix structure
 *   x             <-- Multipliying vector values (ghost values updated)
 *   y             --> Resulting vector
 *----------------------------------------------------------------------------*/

void
cs_matrix_alpha_a_x_p_beta_y(cs_perio_rota_t     rotation_mode,
                             cs_real_t           alpha,
                             cs_real_t           beta,
                             const cs_matrix_t  *matrix,
                             cs_real_t          *restrict x,
                             cs_real_t          *restrict y)
{
  /* Non-blocked version */

  if (matrix->b_size[3] == 1) {

    if (matrix->halo != NULL){

      cs_halo_sync_var(matrix->halo, CS_HALO_STANDARD, x);

      /* Synchronize periodic values */

      if (matrix->halo->n_transforms > 0) {
        if (rotation_mode == CS_PERIO_ROTA_IGNORE)
          bft_error(__FILE__, __LINE__, 0, _cs_glob_perio_ignore_error_str);
        cs_perio_sync_var_scal(matrix->halo, CS_HALO_STANDARD, rotation_mode, x);
      }
    }

    /* Now call local matrix.vector product */

    if (matrix->alpha_a_x_p_beta_y != NULL)
      matrix->alpha_a_x_p_beta_y(alpha, beta, matrix, x, y);

  }

  /* Blocked version */

  else { /* if (matrix->b_size[3] > 1) */

    const int *b_size = matrix->b_size;

    /* Update distant ghost cells */

    if (matrix->halo != NULL) {

      cs_halo_sync_var_strided(matrix->halo,
                               CS_HALO_STANDARD,
                               x,
                               b_size[1]);

      /* Synchronize periodic values */

      if (matrix->halo->n_transforms > 0 && b_size[0] == 3) {
        cs_perio_sync_var_vect(matrix->halo, CS_HALO_STANDARD, x, b_size[1]);
      }

    }

    /* Now call local matrix.vector product */

    if (matrix->b_alpha_a_x_p_beta_y != NULL)
      matrix->b_alpha_a_x_p_beta_y(alpha, beta, matrix, x, y);

  }

}

/*----------------------------------------------------------------------------*/

END_C_DECLS
