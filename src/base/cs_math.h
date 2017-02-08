#ifndef __CS_MATH_H__
#define __CS_MATH_H__

/*============================================================================
 * Mathematical base functions.
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

#include <assert.h>
#include <math.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#if defined(DEBUG) && !defined(NDEBUG) /* Sanity check */
#include "bft_error.h"
#endif

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definition
 *============================================================================*/

/*============================================================================
 *  Global variables
 *============================================================================*/

/* Numerical constants */

extern const cs_real_t cs_math_zero_threshold;
extern const cs_real_t cs_math_onethird;
extern const cs_real_t cs_math_onesix;
extern const cs_real_t cs_math_onetwelve;
extern const cs_real_t cs_math_epzero;
extern const cs_real_t cs_math_infinite_r;
extern const cs_real_t cs_math_big_r;
extern const cs_real_t cs_math_pi;

/*=============================================================================
 * Inline static function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the square of a real value
 *
 * \param[in]  x  value
 *
 * \return the square of the given value
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_sq(cs_real_t  x)
{
  return x*x;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the (euclidean) length between two points xa and xb in
 *         a cartesian coordinate system of dimension 3
 *
 * \param[in]  xa   first coordinate
 * \param[in]  xb   second coordinate
 *
 * \return the length between two points xa and xb
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_3_length(const cs_real_t  xa[3],
                 const cs_real_t  xb[3])
{
  cs_real_t  v[3];

  v[0] = xb[0] - xa[0];
  v[1] = xb[1] - xa[1];
  v[2] = xb[2] - xa[2];

  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the squared distance between two points xa and xb in
 *         a cartesian coordinate system of dimension 3
 *
 * \param[in]  xa   first coordinate
 * \param[in]  xb   second coordinate
 *
 * \return the square length between two points xa and xb
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_3_distance_squared(const cs_real_t  xa[3],
                           const cs_real_t  xb[3])
{
  cs_real_t v[3] = {xb[0] - xa[0],
                    xb[1] - xa[1],
                    xb[2] - xa[2]};

  return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the dot product of two vectors of 3 real values.
 *
 * \param[in]     u             vector of 3 real values
 * \param[in]     v             vector of 3 real values
 *
 * \return the resulting dot product u.v.
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_3_dot_product(const cs_real_t  u[3],
                      const cs_real_t  v[3])
{
  cs_real_t uv = u[0]*v[0] + u[1]*v[1] + u[2]*v[2];

  return uv;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the euclidean norm of a vector of dimension 3
 *
 * \param[in]  v
 *
 * \return the value of the norm
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_3_norm(const cs_real_t  v[3])
{
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the square norm of a vector of 3 real values.
 *
 * \param[in]     v             vector of 3 real values
 *
 * \return square norm of v.
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_3_square_norm(const cs_real_t v[3])
{
  cs_real_t v2 = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

  return v2;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the product of a matrix of 3x3 real values by a vector of 3
 * real values.
 *
 * \param[in]     m             matrix of 3x3 real values
 * \param[in]     v             vector of 3 real values
 * \param[out]    mv            vector of 3 real values
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_33_3_product(const cs_real_t  m[3][3],
                     const cs_real_t  v[3],
                     cs_real_3_t      mv)
{
  mv[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
  mv[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
  mv[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the product of the transpose of a matrix of 3x3 real
 * values by a vector of 3 real values.
 *
 * \param[in]     m             matrix of 3x3 real values
 * \param[in]     v             vector of 3 real values
 * \param[out]    mv            vector of 3 real values
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_33t_3_product(const cs_real_t  m[3][3],
                      const cs_real_t  v[3],
                      cs_real_3_t      mv)
{
  mv[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2];
  mv[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2];
  mv[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the product of a symmetric matrix of 3x3 real values by
 * a vector of 3 real values.
 * NB: Symmetric matrix are stored as follows (s11, s22, s33, s12, s23, s13)
 *
 * \param[in]     m             matrix of 3x3 real values
 * \param[in]     v             vector of 3 real values
 * \param[out]    mv            vector of 3 real values
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_sym_33_3_product(const cs_real_t  m[6],
                         const cs_real_t  v[3],
                         cs_real_t        mv[restrict 3])
{
  mv[0] = m[0] * v[0] + m[3] * v[1] + m[5] * v[2];
  mv[1] = m[3] * v[0] + m[1] * v[1] + m[4] * v[2];
  mv[2] = m[5] * v[0] + m[4] * v[1] + m[2] * v[2];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the determinant of a 3x3 matrix
 *
 * \param[in]  m    3x3 matrix
 *
 * \return the determinant
 */
/*----------------------------------------------------------------------------*/

static inline cs_real_t
cs_math_33_determinant(const cs_real_t   m[3][3])
{
  const cs_real_t  com0 = m[1][1]*m[2][2] - m[2][1]*m[1][2];
  const cs_real_t  com1 = m[2][1]*m[0][2] - m[0][1]*m[2][2];
  const cs_real_t  com2 = m[0][1]*m[1][2] - m[1][1]*m[0][2];

  return m[0][0]*com0 + m[1][0]*com1 + m[2][0]*com2;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the cross product of two vectors of 3 real values.
 *
 * \param[in]     u             vector of 3 real values
 * \param[in]     v             vector of 3 real values
 * \param[out]    uv            vector of 3 real values
 */
/*----------------------------------------------------------------------------*/

#if defined(__INTEL_COMPILER)
#pragma optimization_level 0 /* Bug with O1 or above with icc 15.0.1 20141023 */
#endif

static inline void
cs_math_3_cross_product(const cs_real_t u[3],
                        const cs_real_t v[3],
                        cs_real_t       uv[restrict 3])
{
  uv[0] = u[1]*v[2] - u[2]*v[1];
  uv[1] = u[2]*v[0] - u[0]*v[2];
  uv[2] = u[0]*v[1] - u[1]*v[0];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Inverse a 3x3 matrix
 *
 * \param[in]  in    matrix to inverse
 * \param[out] out   inversed matrix
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_33_inv(const cs_real_t   in[3][3],
               cs_real_t         out[3][3])
{
  out[0][0] = in[1][1]*in[2][2] - in[2][1]*in[1][2];
  out[0][1] = in[2][1]*in[0][2] - in[0][1]*in[2][2];
  out[0][2] = in[0][1]*in[1][2] - in[1][1]*in[0][2];

  out[1][0] = in[2][0]*in[1][2] - in[1][0]*in[2][2];
  out[1][1] = in[0][0]*in[2][2] - in[2][0]*in[0][2];
  out[1][2] = in[1][0]*in[0][2] - in[0][0]*in[1][2];

  out[2][0] = in[1][0]*in[2][1] - in[2][0]*in[1][1];
  out[2][1] = in[2][0]*in[0][1] - in[0][0]*in[2][1];
  out[2][2] = in[0][0]*in[1][1] - in[1][0]*in[0][1];

  const double  det = in[0][0]*out[0][0]+in[1][0]*out[0][1]+in[2][0]*out[0][2];
  const double  invdet = 1/det;

  out[0][0] *= invdet, out[0][1] *= invdet, out[0][2] *= invdet;
  out[1][0] *= invdet, out[1][1] *= invdet, out[1][2] *= invdet;
  out[2][0] *= invdet, out[2][1] *= invdet, out[2][2] *= invdet;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the inverse of a symmetric matrix using Cramer's rule.
 *
 * \remark Symmetric matrix coefficients are stored as follows:
 *         (s11, s22, s33, s12, s23, s13)
 *
 * \param[in]     s             symmetric matrix
 * \param[out]    sout          sout = 1/s1
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_sym_33_inv_cramer(const cs_real_t s[6],
                          cs_real_t       sout[restrict 6])
{
  double detinv;

  sout[0] = s[1]*s[2] - s[4]*s[4];
  sout[1] = s[0]*s[2] - s[5]*s[5];
  sout[2] = s[0]*s[1] - s[3]*s[3];
  sout[3] = s[4]*s[5] - s[3]*s[2];
  sout[4] = s[3]*s[5] - s[0]*s[4];
  sout[5] = s[3]*s[4] - s[1]*s[5];

  detinv = 1. / (s[0]*sout[0] + s[3]*sout[3] + s[5]*sout[5]);

  sout[0] *= detinv;
  sout[1] *= detinv;
  sout[2] *= detinv;
  sout[3] *= detinv;
  sout[4] *= detinv;
  sout[5] *= detinv;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the product of two symmetric matrices.
 *
 * \remark Symmetric matrix coefficients are stored as follows:
 *         (s11, s22, s33, s12, s23, s13)
 *
 * \param[in]     s1            symmetric matrix
 * \param[in]     s2            symmetric matrix
 * \param[out]    sout          sout = s1 * s2
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_sym_33_product(const cs_real_t s1[6],
                       const cs_real_t s2[6],
                       cs_real_t       sout[restrict 6])
{
  /* S11 */
  sout[0] = s1[0]*s2[0] + s1[3]*s2[3] + s1[5]*s2[5];
  /* S22 */
  sout[1] = s1[3]*s2[3] + s1[1]*s2[1] + s1[4]*s2[4];
  /* S33 */
  sout[2] = s1[5]*s2[5] + s1[4]*s2[4] + s1[2]*s2[2];
  /* S12 = S21 */
  sout[3] = s1[0]*s2[3] + s1[3]*s2[1] + s1[5]*s2[4];
  /* S23 = S32 */
  sout[4] = s1[3]*s2[5] + s1[1]*s2[4] + s1[4]*s2[2];
  /* S13 = S31 */
  sout[5] = s1[0]*s2[5] + s1[3]*s2[4] + s1[5]*s2[2];
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute a 6x6 matrix A, equivalent to a 3x3 matrix s, such as:
 * A*R_6 = R*s^t + s*R
 *
 * \param[in]     s            3x3 matrix
 * \param[out]    sout         6x6 matrix
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_reduce_sym_prod_33_to_66(const cs_real_t s[3][3],
                                 cs_real_t       sout[restrict 6][6])
{
  cs_real_33_t tens2vect;
  cs_real_6_t iindex;
  cs_real_6_t jindex;

  /* ------------------------- */
  tens2vect[0][0] = 0; tens2vect[0][1] = 3; tens2vect[0][2] = 5;
  tens2vect[1][0] = 3; tens2vect[1][1] = 1; tens2vect[1][2] = 4;
  tens2vect[2][0] = 5; tens2vect[2][1] = 4; tens2vect[2][2] = 2;
  /* ------------------------- */
  iindex[0] = 0; iindex[1] = 1; iindex[2] = 2;
  iindex[3] = 0; iindex[4] = 1; iindex[5] = 0;
  /* ------------------------- */
  jindex[0] = 0; jindex[1] = 1; jindex[2] = 2;
  jindex[3] = 1; jindex[4] = 2; jindex[5] = 2;
  /* ------------------------- */

  /* Consider : W = R*s^t + s*R.
   *            W_ij = Sum_{k<3} [s_jk*r_ik + s_ik*r_jk]
   * We look for A such as
   */
  for (int iR = 0; iR < 6; iR++) {
    int ii;
    int jj;

    ii = iindex[iR];
    jj = jindex[iR];
    for (int k = 0; k < 3; k++) {
      int ikR = tens2vect[k][ii];
      int jkR = tens2vect[k][jj];

      sout[ikR][iR] += s[k][jj];

      sout[jkR][iR] += s[k][ii];
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the all eigenvalue of a 3x3 matrix which is symmetric and real
 *        -> Oliver K. Smith "eigenvalues of a symmetric 3x3 matrix",
 *        Communication of the ACM (April 1961)
 *        -> Wikipedia article entitled "Eigenvalue algorithm"
 *
 * \param[in]  m          3x3 matrix
 * \param[out] eig_vals   size 3 vector
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_33_eigen_vals(const cs_real_t   m[3][3],
                     cs_real_t          eig_vals[3])
{
  cs_real_t  e, e1, e2, e3;

#if defined(DEBUG) && !defined(NDEBUG) /* Sanity check */
  e1 = m[0][1]-m[1][0], e2 = m[0][2]-m[2][0], e3 = m[1][2]-m[2][1];
  if (e1*e1 + e2*e2 + e3*e3 > 0.0)
    bft_error(__FILE__, __LINE__, 0,
              " The given 3x3 matrix is not symmetric.\n"
              " Stop computing eigenvalues of a 3x3 matrix since the"
              " algorithm is only dedicated to symmetric matrix.");
#endif

  cs_real_t  p1 = m[0][1]*m[0][1] + m[0][2]*m[0][2] + m[1][2]*m[1][2];

  if (p1 > 0.0) { // m is not diagonal

    cs_real_t  theta;
    cs_real_t  n[3][3];
    cs_real_t  tr = cs_math_onethird*(m[0][0] + m[1][1] + m[2][2]);

    e1 = m[0][0] - tr, e2 = m[1][1] - tr, e3 = m[2][2] - tr;
    cs_real_t  p2 = e1*e1 + e2*e2 + e3*e3 + 2*p1;

    assert(p2 > 0);
    cs_real_t  p = sqrt(p2*cs_math_onesix);
    cs_real_t  ovp = 1./p;

    for (int  i = 0; i < 3; i++) {
      n[i][i] = ovp * (m[i][i] - tr);
      for (int j = i + 1; j < 3; j++) {
        n[i][j] = ovp*m[i][j];
        n[j][i] = n[i][j];
      }
    }

    /* r should be between -1 and 1 but truncation error and bad conditionning
       can lead to slighty under/over-shoot */
    cs_real_t  r = 0.5 * cs_math_33_determinant((const cs_real_t (*)[3])n);

    if (r <= -1)
      theta = cs_math_onethird*cs_math_pi;
    else if (r >= 1)
      theta = 0.;
    else
      theta = cs_math_onethird*acos(r);

    // eigenvalues computed should satisfy e1 < e2 < e3
    e3 = tr + 2*p*cos(theta);
    e1 = tr + 2*p*cos(theta + 2*cs_math_pi*cs_math_onethird);
    e2 = 3*tr - e1 -e3; // since tr(m) = e1 + e2 + e3

  }
  else { // m is diagonal

    e1 = m[0][0], e2 = m[1][1], e3 = m[2][2];

  } /* diagonal or not */

  if (e3 < e2) e = e3, e3 = e2, e2 = e;
  if (e3 < e1) e = e3, e3 = e1, e1 = e2, e2 = e;
  else {
    if (e2 < e1) e = e2, e2 = e1, e1 = e;
  }
  /* Return values */
  eig_vals[0] = e1;
  eig_vals[1] = e2;
  eig_vals[2] = e3;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the product of three symmetric matrices.
 *
 * \remark Symmetric matrix coefficients are stored as follows:
 *         (s11, s22, s33, s12, s23, s13)
 *
 * \param[in]     s1            symmetric matrix
 * \param[in]     s2            symmetric matrix
 * \param[in]     s3            symmetric matrix
 * \param[out]    sout          sout = s1 * s2 * s3
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_math_sym_33_double_product(const cs_real_t  s1[6],
                              const cs_real_t  s2[6],
                              const cs_real_t  s3[6],
                              cs_real_t        sout[restrict 3][3])
{
  cs_real_33_t _sout;

  /* S11 */
  _sout[0][0] = s1[0]*s2[0] + s1[3]*s2[3] + s1[5]*s2[5];
  /* S22 */
  _sout[1][1] = s1[3]*s2[3] + s1[1]*s2[1] + s1[4]*s2[4];
  /* S33 */
  _sout[2][2] = s1[5]*s2[5] + s1[4]*s2[4] + s1[2]*s2[2];
  /* S12  */
  _sout[0][1] = s1[0]*s2[3] + s1[3]*s2[1] + s1[5]*s2[4];
  /* S21  */
  _sout[1][0] = s2[0]*s1[3] + s2[3]*s1[1] + s2[5]*s1[4];
  /* S23  */
  _sout[1][2] = s1[3]*s2[5] + s1[1]*s2[4] + s1[4]*s2[2];
  /* S32  */
  _sout[2][1] = s2[3]*s1[5] + s2[1]*s1[4] + s2[4]*s1[2];
  /* S13  */
  _sout[0][2] = s1[0]*s2[5] + s1[3]*s2[4] + s1[5]*s2[2];
  /* S31  */
  _sout[2][0] = s2[0]*s1[5] + s2[3]*s1[4] + s2[5]*s1[2];

  sout[0][0] = _sout[0][0]*s3[0] + _sout[0][1]*s3[3] + _sout[0][2]*s3[5];
  /* S22 */
  sout[1][1] = _sout[1][0]*s3[3] + _sout[1][1]*s3[1] + _sout[1][2]*s3[4];
  /* S33 */
  sout[2][2] = _sout[2][0]*s3[5] + _sout[2][1]*s3[4] + _sout[2][2]*s3[2];
  /* S12  */
  sout[0][1] = _sout[0][0]*s3[3] + _sout[0][1]*s3[1] + _sout[0][2]*s3[4];
  /* S21  */
  sout[1][0] = s3[0]*_sout[1][0] + s3[3]*_sout[1][1] + s3[5]*_sout[1][2];
  /* S23  */
  sout[1][2] = _sout[1][0]*s3[5] + _sout[1][1]*s3[4] + _sout[1][2]*s3[2];
  /* S32  */
  sout[2][1] = s3[3]*_sout[2][0] + s3[1]*_sout[2][1] + s3[4]*_sout[2][2];
  /* S13  */
  sout[0][2] = _sout[0][0]*s3[5] + _sout[0][1]*s3[4] + _sout[0][2]*s3[2];
  /* S31  */
  sout[2][0] = s3[0]*_sout[2][0] + s3[3]*_sout[2][1] + s3[5]*_sout[2][2];
}

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value related to the machine precision
 */
/*----------------------------------------------------------------------------*/

void
cs_math_set_machine_epsilon(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the value related to the machine precision
 */
/*----------------------------------------------------------------------------*/

double
cs_math_get_machine_epsilon(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the length (euclidien norm) between two points xa and xb in
 *         a cartesian coordinate system of dimension 3
 *
 * \param[in]   xa       coordinate of the first extremity
 * \param[in]   xb       coordinate of the second extremity
 * \param[out]  len      pointer to the length of the vector va -> vb
 * \param[out]  unitv    unitary vector along xa -> xb
 */
/*----------------------------------------------------------------------------*/

void
cs_math_3_length_unitv(const cs_real_t    xa[3],
                       const cs_real_t    xb[3],
                       cs_real_t         *len,
                       cs_real_3_t        unitv);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the eigenvalues of a 3x3 matrix which is symmetric and real
 *         -> Oliver K. Smith "eigenvalues of a symmetric 3x3 matrix",
 *         Communication of the ACM (April 1961)
 *         -> Wikipedia article entitled "Eigenvalue algorithm"
 *
 * \param[in]  m          3x3 matrix
 * \param[out] eig_ratio  max/min
 * \param[out] eig_max    max. eigenvalue
 */
/*----------------------------------------------------------------------------*/

void
cs_math_33_eigen(const cs_real_t     m[3][3],
                 cs_real_t          *eig_ratio,
                 cs_real_t          *eig_max);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the area of the convex_hull generated by 3 points.
 *         This corresponds to the computation of the surface of a triangle
 *
 * \param[in]  xv  coordinates of the first vertex
 * \param[in]  xe  coordinates of the second vertex
 * \param[in]  xf  coordinates of the third vertex
 *
 * \return the surface of a triangle
 */
/*----------------------------------------------------------------------------*/

double
cs_math_surftri(const cs_real_t  xv[3],
                const cs_real_t  xe[3],
                const cs_real_t  xf[3]);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the volume of the convex_hull generated by 4 points.
 *         This is equivalent to the computation of the volume of a tetrahedron
 *
 * \param[in]  xv  coordinates of the first vertex
 * \param[in]  xe  coordinates of the second vertex
 * \param[in]  xf  coordinates of the third vertex
 * \param[in]  xc  coordinates of the fourth vertex
 *
 * \return the volume of the tetrahedron.
 */
/*----------------------------------------------------------------------------*/

double
cs_math_voltet(const cs_real_t   xv[3],
               const cs_real_t   xe[3],
               const cs_real_t   xf[3],
               const cs_real_t   xc[3]);

/*----------------------------------------------------------------------------
 * Compute inverse of an array of dense matrices of same size.
 *
 * parameters:
 *  n_blocks  <--  number of blocks
 *  db_size   <-- matrix size
 *  ad        <--  diagonal part of linear equation matrix
 *  ad_inv    -->  inverse of the diagonal part of linear equation matrix
 *----------------------------------------------------------------------------*/

void
cs_math_fact_lu(cs_lnum_t         n_blocks,
                const int         db_size,
                const cs_real_t  *ad,
                cs_real_t        *ad_inv);

/*----------------------------------------------------------------------------
 *  Compute forward and backward to solve an LU system of size db_size.
 *
 * parameters
 *  mat     <-- P*P*dim matrix
 *  db_size <-- matrix size
 *  x       --> solution
 *  b       --> 1st part of RHS (c - b)
 *  c       --> 2nd part of RHS (c - b)
 *----------------------------------------------------------------------------*/

void
cs_math_fw_and_bw_lu(const cs_real_t  mat[],
                     const int        db_size,
                     cs_real_t        x[],
                     const cs_real_t  b[],
                     const cs_real_t  c[]);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_MATH_H__ */
