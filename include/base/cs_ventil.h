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

#ifndef __CS_VENTIL_H__
#define __CS_VENTIL_H__

/*============================================================================
 * D�finitions, variables globales, et fonctions associ�es aux ventilateurs
 *============================================================================*/

/* includes syst�me */


/* includes librairie */

#include "cs_base.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* Fausse "}" pour corriger l'auto-indentation d'Emacs */
}
#endif

/*=============================================================================
 * D�finitions de macros
 *============================================================================*/

/*============================================================================
 * D�finitions de types
 *============================================================================*/

/* Structure associ�e � un ventilateur */

typedef struct _cs_ventil_t cs_ventil_t;

/*============================================================================
 *  Prototypes de fonctions publiques pour API Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * R�cup�ration du nombre de ventilateurs
 *
 * Interface Fortran :
 *
 * SUBROUTINE TSTVTL
 * *****************
 *
 * INTEGER          NBRVTL         : --> : nombre de ventilateurs
 *----------------------------------------------------------------------------*/

void CS_PROCF (tstvtl, TSTVTL)
(
 cs_int_t  *const nbrvtl              /* <-- nombre de ventilateurs           */
);


/*----------------------------------------------------------------------------
 * Ajout d'un ventilateur
 *
 * Interface Fortran :
 *
 * SUBROUTINE DEFVTL (XYZVT1, XYZVT2, RVVT  , RPVT  , RMVT  ,
 * *****************
 *                    CCARAC, TAUVT)
 *
 * INTEGER          DIMMOD     : <-- : Dimension du mod�le de ventilateur :
 *                             :     : f_constante ; 2 : profil_force ;
 *                             :     : 3 : profil_force + couple tangentiel
 * INTEGER          DIMVTL     : <-- : Dimension du ventilateur :
 *                             :     : 2 : pseudo-2D (maillage extrud�)
 *                             :     : 3 : 3D (standard)
 * DOUBLE PRECISION XYZVT1(3)  : <-- : Coord. point de l'axe en face amont
 * DOUBLE PRECISION XYZVT2(3)  : <-- : Coord. point de l'axe en face aval
 * DOUBLE PRECISION RVVT       : <-- : Rayon du ventilateur
 * DOUBLE PRECISION RPVT       : <-- : Rayon des pales
 * DOUBLE PRECISION RMVT       : <-- : Rayon du moyeu
 * DOUBLE PRECISION CCARAC(3)  : <-- : Coefficients de degr� 0, 1, et 2
 *                             :     : de la courbe caract�ristique
 * DOUBLE PRECISION TAUVT      : <-- : Couple axial du ventilateur
 *----------------------------------------------------------------------------*/

void CS_PROCF (defvtl, DEFVTL)
(
 const cs_int_t  *const  dimmod,     /* Dimension du mod�le de ventilateur :
                                        1 : f_constante ; 2 : profil_force ;
                                        3 : profil_force + couple tangentiel */
 const cs_int_t  *const  dimvtl,     /* Dimension du ventilateur :
                                        2 : pseudo-2D (maillage extrud�)
                                        3 : 3D (standard) */
 const cs_real_t         xyzvt1[3],  /* Coord. point de l'axe en face amont */
 const cs_real_t         xyzvt2[3],  /* Coord. point de l'axe en face aval */
 const cs_real_t  *const rvvt,       /* Rayon du ventilateur */
 const cs_real_t  *const rpvt,       /* Rayon des pales */
 const cs_real_t  *const rmvt,       /* Rayon du moyeu */
 const cs_real_t         ccarac[3],  /* Coefficients courbe caract�ristique */
 const cs_real_t  *const tauvt       /* Couple axial du ventilateur*/
);


/*----------------------------------------------------------------------------
 * Construction des listes de cellules associ�es aux ventilateurs
 *
 * Interface Fortran :
 *
 * SUBROUTINE INIVTL
 * *****************
 *----------------------------------------------------------------------------*/

void CS_PROCF (inivtl, INIVTL)
(
 void
);


/*----------------------------------------------------------------------------
 * Marquage des ventilateurs ; affecte le num�ro de ventilateur aux cellules
 * appartenant � un ventilateur, 0 sinon
 *
 * Interface Fortran :
 *
 * SUBROUTINE NUMVTL (INDIC)
 * *****************
 *----------------------------------------------------------------------------*/

void CS_PROCF (numvtl, NUMVTL)
(
 cs_int_t  indic[]              /* Num�ro de ventilateur d'appartenance, ou 0 */
);


/*----------------------------------------------------------------------------
 * Calcul des d�bits � travers les ventilateurs
 *
 * Interface Fortran :
 *
 * SUBROUTINE DEBVTL (NBRVTL, FLUMAS, FLUMAB, RHO, RHOFAB, DEBENT, DEBSOR)
 * *****************
 *
 * DOUBLE PRECISION FLUMAS(*)      : <-- : Flux de masse aux faces int�rieures
 * DOUBLE PRECISION FLUMAB(*)      : <-- : Flux de masse aux faces de bord
 * DOUBLE PRECISION RHOFAC(*)      : <-- : Densit� aux faces int�rieures
 * DOUBLE PRECISION RHOFAB(*)      : <-- : Densit� aux faces de bord
 * DOUBLE PRECISION DEBENT(NBRVTL) : --> : D�bit entrant par ventilateur
 * DOUBLE PRECISION DEBSOR(NBRVTL) : --> : D�bit sortant par ventilateur
 *----------------------------------------------------------------------------*/

void CS_PROCF (debvtl, DEBVTL)
(
 cs_real_t  flumas[],           /* <-- Flux de masse aux faces int�rieures    */
 cs_real_t  flumab[],           /* <-- Flux de masse aux faces de bord        */
 cs_real_t  rhofac[],           /* <-- Densit� aux faces int�rieures          */
 cs_real_t  rhofab[],           /* <-- Densit� aux faces de bord              */
 cs_real_t  debent[],           /* <-- D�bit entrant par ventilateur          */
 cs_real_t  debsor[]            /* <-- D�bit sortant par ventilateur          */
);


/*----------------------------------------------------------------------------
 * Calcul de la force induite par les ventilateurs (n�cessite le
 * calcul pr�alable des d�bits � travers chaque ventilateur) ;
 * La force induite est ajout�e au tableau CRVXEP (qui peut contenir
 * d'autres contributions)
 *
 * Interface Fortran :
 *
 * SUBROUTINE TSVVTL (DEBENT, DEBSOR)
 * *****************
 *
 * INTEGER          IDIMTS         : <-- : Dimension associ�e au terme source
 *                                 :     : de vitesse (1 : X ; 2 : Y ; 3 : Z)
 * DOUBLE PRECISION CRVEXP(NCELET) : <-> : Terme source explicite de vitesse
 *----------------------------------------------------------------------------*/

void CS_PROCF (tsvvtl, TSVVTL)
(
 cs_int_t  *idimts,             /* <-- dimension associ�e au
                                 *     terme source de vitesse :
                                 *     0 (X), 1 (Y), ou 2 (Z)                 */
 cs_real_t  crvexp[]            /* <-- Terme source explicite de vitesse      */
);


/*=============================================================================
 * Prototypes de fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 * D�finition d'un ventilateur (qui est ajout� � ceux d�j� d�finis)
 *----------------------------------------------------------------------------*/

void cs_ventil_definit
(
 const cs_int_t   dim_modele,       /* Dimension du mod�le de ventilateur :
                                       1 : f_constante ; 2 : profil_force ;
                                       3 : profil_force + couple tangentiel */
 const cs_int_t   dim_ventil,       /* Dimension du ventilateur :
                                       2 : pseudo-2D (maillage extrud�)
                                       3 : 3D (standard) */
 const cs_real_t  coo_axe_amont[3], /* Coord. point de l'axe en face amont */
 const cs_real_t  coo_axe_aval[3],  /* Coord. point de l'axe en face aval */
 const cs_real_t  ray_ventil,       /* Rayon du ventilateur */
 const cs_real_t  ray_pales,        /* Rayon des pales */
 const cs_real_t  ray_moyeu,        /* Rayon du moyeu */
 const cs_real_t  coeff_carac[3],   /* Coefficients des termes de degr� 0,
                                       1, et 2 de la caract�ristique */
 const cs_real_t  couple_axial      /* Couple axial du ventilateur*/
);


/*----------------------------------------------------------------------------
 * Destruction des structures associ�es aux ventilateurs
 *----------------------------------------------------------------------------*/

void cs_ventil_detruit_tous
(
 void
);


/*----------------------------------------------------------------------------
 * Recherche des cellules appartenant aux diff�rents ventilateurs
 *----------------------------------------------------------------------------*/

void cs_ventil_cree_listes
(
 const cs_mesh_t             *mesh,            /* <-- structure maillage associ�e  */
 const cs_mesh_quantities_t  *mesh_quantitie   /* <-- grandeurs du maillage        */
);


/*----------------------------------------------------------------------------
 * Calcul des debits � travers les ventilateurs
 *----------------------------------------------------------------------------*/

void cs_ventil_calcul_debits
(
 const cs_mesh_t             *mesh,            /* <-- structure maillage        */
 const cs_mesh_quantities_t  *mesh_quantities, /* <-- grandeurs du maillage     */
 const cs_real_t             flux_masse_fac[], /* <-- flux masse faces internes */
 const cs_real_t             flux_masse_fbr[], /* <-- flux masse faces de bord  */
 const cs_real_t             densite_cel[],    /* <-- densit� aux cellules      */
 const cs_real_t             densite_fbr[]     /* <-- densit� aux faces de bord */
);


/*----------------------------------------------------------------------------
 * Calcul de la force induite par les ventilateurs (n�cessite le
 * calcul pr�alable des d�bits � travers chaque ventilateur).
 * La force induite est ajout�e au tableau t_source (qui peut contenir
 * d'autres contributions)
 *----------------------------------------------------------------------------*/

void cs_ventil_calcul_force
(
 const cs_mesh_quantities_t  *mesh_quantities,  /* <-- grandeurs du maillage     */
 const cs_int_t               idim_source,      /* --> dimension associ�e au
                                                 *     terme source de vitesse :
                                                 *     0 (X), 1 (Y), ou 2 (Z)    */
       cs_real_t           t_source[]           /* --> terme source de vitesse   */
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_VENTIL_H__ */
