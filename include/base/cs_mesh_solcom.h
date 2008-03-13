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

#ifndef __CS_MESH_SOLCOM_H__
#define __CS_MESH_SOLCOM_H__

/*============================================================================
 * Fortran interface for reading data with "SolCom" specifications
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definition
 *============================================================================*/

/*============================================================================
 * Static global variables
 *============================================================================*/

/*============================================================================
 *  Public functions definition for API Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Mise � jour des informations de dimensionnement maillage apr�s la lecture
 * de l'ent�te du fichier de maillage en mode IFOENV = 0
 *
 * Interface Fortran :
 *
 * SUBROUTINE DIMGEO (NDIM  , NCELET, NCEL  , NFAC  , NFABOR, NSOM  ,
 * *****************
 *                    LNDFAC, LNDFBR, NFML  , NPRFML,
 *                    NTETRA, NPYRAM, NPRISM, NHEXAE )
 *
 * INTEGER          NDIM        : <-- : Dimension de l'espace (3)
 * INTEGER          NCELET      : <-- : Nombre d'�l�ments halo compris
 * INTEGER          NCEL        : <-- : Nombre d'�l�ments actifs
 * INTEGER          NFAC        : <-- : Nombre de faces internes
 * INTEGER          NFABOR      : <-- : Nombre de faces de bord
 * INTEGER          NSOM        : <-- : Nombre de sommets (optionnel)
 * INTEGER          LNDFAC      : <-- : Longueur de SOMFAC (optionnel)
 * INTEGER          LNDFBR      : <-- : Longueur de SOMFBR (optionnel)
 * INTEGER          NFML        : <-- : Nombre de familles des faces de bord
 * INTEGER          NPRFML      : <-- : Nombre de propri�t�s max par famille
 * INTEGER          NTETRA      : <-- : Nombre de t�tra�dres
 * INTEGER          NPYRAM      : <-- : Nombre de pyramides
 * INTEGER          NPRISM      : <-- : Nombre de prismes
 * INTEGER          NHEXAE      : <-- : Nombre d'hexa�dres
 *----------------------------------------------------------------------------*/

void CS_PROCF (dimgeo, DIMGEO)
(
 const cs_int_t   *const ndim,    /* <-- dimension de l'espace                */
 const cs_int_t   *const ncelet,  /* <-- nombre d'�l�ments halo compris       */
 const cs_int_t   *const ncel,    /* <-- nombre d'�l�ments actifs             */
 const cs_int_t   *const nfac,    /* <-- nombre de faces internes             */
 const cs_int_t   *const nfabor,  /* <-- nombre de faces de bord              */
 const cs_int_t   *const nsom,    /* <-- nombre de sommets (optionnel)        */
 const cs_int_t   *const lndfac,  /* <-- longueur de somfac (optionnel)       */
 const cs_int_t   *const lndfbr,  /* <-- longueur de somfbr (optionnel)       */
 const cs_int_t   *const nfml,    /* <-- nombre de familles des faces de bord */
 const cs_int_t   *const nprfml,  /* <-- nombre de propri�t�s max par famille */
 const cs_int_t   *const ntetra,  /* <-- nombre de t�tra�dres                 */
 const cs_int_t   *const npyram,  /* <-- nombre de pyramides                  */
 const cs_int_t   *const nprism,  /* <-- nombre de prismes                    */
 const cs_int_t   *const nhexae   /* <-- nombre d'hexa�dres                   */
);

/*=============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Lecture d'un maillage au format "SolCom"
 *
 * mesh               <-> maillage associ�
 * mesh_quantities    <-> grandeurs associ�s
 *
 * Retour:
 *----------------------------------------------------------------------------*/

void
cs_maillage_solcom_lit(cs_mesh_t             *const mesh,
                       cs_mesh_quantities_t  *const mesh_quantities);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_MESH_SOLCOM_H__ */
