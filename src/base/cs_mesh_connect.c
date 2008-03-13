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

/*============================================================================
 * Passage d'une connectivit� noyau � une connecitvit� nodale d'une
 * structure principale associ�e � ou extraite d'un maillage
 *============================================================================*/

/* includes syst�me */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C ou BFT
 *----------------------------------------------------------------------------*/

#include <bft_error.h>
#include <bft_mem.h>

/*----------------------------------------------------------------------------
 *  Fichiers `include' locaux
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_mesh.h"

#include <fvm_defs.h>
#include <fvm_nodal.h>
#include <fvm_nodal_from_desc.h>
#include <fvm_nodal_order.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' associ�s au fichier courant
 *----------------------------------------------------------------------------*/

#include "cs_mesh_connect.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*============================================================================
 *  D�finitions d'�numerations
 *============================================================================*/


/*============================================================================
 *  D�finition de macros
 *============================================================================*/


/*============================================================================
 *  Variables globales statiques
 *============================================================================*/


/*============================================================================
 * Prototypes de fonctions priv�es
 *============================================================================*/

/*============================================================================
 *  Fonctions publiques pour API Fortran
 *============================================================================*/


/*============================================================================
 * Fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Extraction de la connectivit� "cellules -> faces" d'un maillage.
 *
 * On consid�re une num�rotation commune des faces internes et des
 * faces de bord, dans laquelle les faces de bord sont d�finies en
 * premier. L'indice commun de la i-�me face de bord est donc �gal � i,
 * et celui de la j-i�me face interne � nbr_fbr + j.
 *
 * Si ind_cel_extr != NULL, alors :
 * --- ind_cel_extr[icel] = indice dans la liste � extraire (0 � n-1)
 *     si icel correspond � une cellule � extraire
 * --- ind_cel_extr[icel] = -1 si la cellule icel est � ignorer
 *----------------------------------------------------------------------------*/

void cs_maillage_ret_cel_fac
(
 const cs_mesh_t       *const maillage,       /* --> Maillage */
 const cs_int_t               nbr_cel_extr,   /* --> Taille de ind_cel_extr[] */
 const cs_int_t               ind_cel_extr[], /* --> ind_cel_extr[cellule]
                                               *     = indice cellule extraite
                                               *       ou -1 */
 cs_int_t            * *const p_pos_cel_fac,  /* <-- idx cellule -> face */
 cs_int_t            * *const p_val_cel_fac   /* <-- val cellule -> face */
)
{

  cs_int_t    icel, icel1, icel2, ifac, nbr_cel_loc;

  cs_int_t  * cpt_cel_fac = NULL;
  cs_int_t  * pos_cel_fac = NULL;
  cs_int_t  * val_cel_fac = NULL;

  /* Allocation et initialisation de l'indice des positions */

  nbr_cel_loc = maillage->n_cells;
  if (ind_cel_extr != NULL)
    nbr_cel_loc = nbr_cel_extr;

  BFT_MALLOC(pos_cel_fac, nbr_cel_loc + 1, cs_int_t);

  for (icel = 0 ; icel < nbr_cel_loc + 1 ; icel++)
    pos_cel_fac[icel] = 0;

  /* Comptage du nombre de faces par cellule
   * (on affecte le compteur temporaire correspondant � icel �
   * pos_cel_fac[icel + 1] et non pas � pos_cel_fac[icel] pour
   * faciliter l'�tape suivante) */

  /* Remarque : test si icel < maillage->n_cells sur faces internes
     pour ignorer les cellules fant�mes parall�les et/ou p�riodiques */

  for (ifac = 0 ; ifac < maillage->n_b_faces ; ifac++) {
    icel = maillage->b_face_cells[ifac] - 1;
    if (ind_cel_extr != NULL)
      icel = ind_cel_extr[icel];
    if (icel > -1)
      pos_cel_fac[icel + 1] += 1;
  }

  for (ifac = 0 ; ifac < maillage->n_i_faces ; ifac++) {
    icel1 = maillage->i_face_cells[ifac*2    ] - 1;
    icel2 = maillage->i_face_cells[ifac*2 + 1] - 1;
    if (ind_cel_extr != NULL) {
      if (icel1 < maillage->n_cells)
        icel1 = ind_cel_extr[icel1];
      else
        icel1 = -1;
      if (icel2 < maillage->n_cells)
        icel2 = ind_cel_extr[icel2];
      else
        icel2 = -1;
    }
    if (icel1 > -1 && icel1 < maillage->n_cells)
      pos_cel_fac[icel1 + 1] += 1;
    if (icel2 > -1 && icel2 < maillage->n_cells)
      pos_cel_fac[icel2 + 1] += 1;
  }

  /* Construction de l'indice des positions */

  pos_cel_fac[0] = 1;
  for (icel = 0 ; icel < nbr_cel_loc ; icel++)
    pos_cel_fac[icel + 1] = pos_cel_fac[icel] + pos_cel_fac[icel + 1];

  /* Construction du tableau des valeurs */

  BFT_MALLOC(val_cel_fac, pos_cel_fac[nbr_cel_loc] - 1, cs_int_t);
  BFT_MALLOC(cpt_cel_fac, nbr_cel_loc, cs_int_t);

  for (icel = 0 ; icel < nbr_cel_loc ; icel++)
    cpt_cel_fac[icel] = 0;

  for (ifac = 0 ; ifac < maillage->n_b_faces ; ifac++) {
    icel = maillage->b_face_cells[ifac] - 1;
    if (ind_cel_extr != NULL)
      icel = ind_cel_extr[icel];
    if (icel > -1) {
      val_cel_fac[pos_cel_fac[icel] + cpt_cel_fac[icel] - 1] = ifac + 1;
      cpt_cel_fac[icel] += 1;
    }
  }

  for (ifac = 0 ; ifac < maillage->n_i_faces ; ifac++) {
    icel1 = maillage->i_face_cells[ifac*2    ] - 1;
    icel2 = maillage->i_face_cells[ifac*2 + 1] - 1;
    if (ind_cel_extr != NULL) {
      if (icel1 < maillage->n_cells)
        icel1 = ind_cel_extr[icel1];
      else
        icel1 = -1;
      if (icel2 < maillage->n_cells)
        icel2 = ind_cel_extr[icel2];
      else
        icel2 = -1;
    }
    if (icel1 > -1 && icel1 < maillage->n_cells) {
      val_cel_fac[pos_cel_fac[icel1] + cpt_cel_fac[icel1] - 1]
        =   ifac + maillage->n_b_faces + 1;
      cpt_cel_fac[icel1] += 1;
    }
    if (icel2 > -1 && icel2 < maillage->n_cells) {
      val_cel_fac[pos_cel_fac[icel2] + cpt_cel_fac[icel2] - 1]
        = -(ifac + maillage->n_b_faces + 1);
      cpt_cel_fac[icel2] += 1;
    }
  }

  BFT_FREE(cpt_cel_fac);

  /* Valeurs de retour */

  *p_pos_cel_fac = pos_cel_fac;
  *p_val_cel_fac = val_cel_fac;

#if 0 && defined(DEBUG) && !defined(NDEBUG)
 {
   cs_int_t ipos, ival;
   /* Impression des tableaux */
   bft_printf("dbg : cs_maillage_ret_cel_fac\n"
              "nombre de cellules extraites = %d\n", nbr_cel_extr);
   for (ipos = 0 ; ipos < nbr_cel_extr ; ipos++) {
     bft_printf("  cellule %d\n", ipos);
     bft_printf("    pos_cel_fac[%d] = %d\n", ipos, pos_cel_fac[ipos]);
     for (ival = pos_cel_fac[ipos]     - 1;
          ival < pos_cel_fac[ipos + 1] - 1;
          ival++)
       bft_printf("      val_cel_fac[%d] = %d\n", ival, val_cel_fac[ival]);
   }
   bft_printf("  pos_cel_fac[%d] = %d\n", ipos, pos_cel_fac[ipos]);
 }
#endif

}


/*----------------------------------------------------------------------------
 * Extraction et conversion en connectivit� nodale externe d'un sous-ensemble
 * des cellules d'un maillage.
 *
 * La liste des cellules � traiter est optionnelle ; elle peut ne pas
 * �tre ordonn�e en entr�e, elle le sera toujours en sortie (les cellules
 * �tant extraites au cours d'un parcours en ordre croissant, la liste
 * est r�ordonn�e pour assurer la coh�rence des liens des cellules extraites
 * vers leurs cellules parentes, construits � partir de cette liste).
 *----------------------------------------------------------------------------*/

fvm_nodal_t  * cs_maillage_extrait_cel_nodal
(
 const cs_mesh_t      *const mesh,          /* --> maillage                   */
 const char           *const nom,           /* --> nom � affecter             */
 const cs_int_t              nbr_liste_cel, /* --> taille de liste_cel[]      */
       cs_int_t              liste_cel[]    /* <-> liste optionnelle des
                                             *     cellules � traiter (1 � n) */
)
{

  cs_int_t    icel ;

  cs_int_t    nbr_cel_extr = 0 ;
  cs_int_t  * ind_cel_extr = NULL ;

  cs_int_t  * pos_cel_fac = NULL;
  cs_int_t  * val_cel_fac = NULL;

  fvm_lnum_t  dec_num_faces[3];
  fvm_lnum_t  *pos_faces_som[2];
  fvm_lnum_t  *val_faces_som[2];
  fvm_lnum_t  *faces_polyedres = NULL;

  fvm_nodal_t  *maillage_ext;

  /* V�rification que le maillage contient bien les connectivit�s
     faces->sommets */

  if (mesh->b_face_vtx_idx == NULL || mesh->i_face_vtx_idx == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("Le maillage principal ne contient pas de connectivit�\n"
                "faces->sommets, indispensable � la reconstruction\n"
                "de la connectivit� nodale (cs_maillage_extrait_cel_nodal)."));

  /* Comptage du nombre de cellules � convertir */

  if (liste_cel != NULL) {

    BFT_MALLOC(ind_cel_extr, mesh->n_cells, cs_int_t);

    /* Initialisation sous forme de marqueur */
    for (icel = 0 ; icel < mesh->n_cells ; icel++)
      ind_cel_extr[icel] = -1;
    for (icel = 0 ; icel < nbr_liste_cel ; icel++) {
      if (liste_cel[icel] <= mesh->n_cells)
        ind_cel_extr[liste_cel[icel] - 1] = 1;
    }

    /* conversion indices marqu�s comme utilis�s en pointeurs (1 � n)
       et reconstruction des valeurs de liste_cel[] de mani�re �
       s'assurer qu'elle soit tri�e */
    nbr_cel_extr = 0;
    for (icel = 0 ; icel < mesh->n_cells ; icel++) {
      if (ind_cel_extr[icel] == 1) {
        liste_cel[nbr_cel_extr] = icel + 1;
        ind_cel_extr[icel] = nbr_cel_extr++;
      }
    }

    assert(nbr_cel_extr <= nbr_liste_cel);

  }
  else {
    nbr_cel_extr = CS_MIN(mesh->n_cells, nbr_liste_cel);
    ind_cel_extr = NULL;
  }

  /* Extraction de la connectivit� "cellules -> faces" */

  cs_maillage_ret_cel_fac(mesh,
                          nbr_cel_extr,
                          ind_cel_extr,
                          &pos_cel_fac,
                          &val_cel_fac);

  if (ind_cel_extr != NULL)
    BFT_FREE(ind_cel_extr);

  /* Construction de la connectivit� nodale */

  dec_num_faces[0] = 0;
  dec_num_faces[1] = mesh->n_b_faces + dec_num_faces[0];
  dec_num_faces[2] = mesh->n_i_faces + dec_num_faces[1];

  pos_faces_som[0] = mesh->b_face_vtx_idx;
  pos_faces_som[1] = mesh->i_face_vtx_idx;
  val_faces_som[0] = mesh->b_face_vtx_lst;
  val_faces_som[1] = mesh->i_face_vtx_lst;

  maillage_ext = fvm_nodal_create(nom, 3);

  fvm_nodal_from_desc_add_cells(maillage_ext,
                                nbr_cel_extr,
                                NULL,
                                2,
                                dec_num_faces,
                                (const fvm_lnum_t **) pos_faces_som,
                                (const fvm_lnum_t **) val_faces_som,
                                pos_cel_fac,
                                val_cel_fac,
                                liste_cel,
                                &faces_polyedres);

  fvm_nodal_set_shared_vertices(maillage_ext,
                                mesh->vtx_coord);

  /* Lib�ration m�moire */

  BFT_FREE(faces_polyedres);

  BFT_FREE(pos_cel_fac);
  BFT_FREE(val_cel_fac);

  /* Tri des cellules par num�ro ou indice global croissant */

  fvm_nodal_order_cells(maillage_ext, mesh->global_cell_num);
  fvm_nodal_init_io_num(maillage_ext, mesh->global_cell_num, 3);

  /* Tri des sommets par num�ro ou indice global croissant */

  fvm_nodal_order_vertices(maillage_ext, mesh->global_vtx_num);
  fvm_nodal_init_io_num(maillage_ext, mesh->global_vtx_num, 0);

  /* On a termin� */

  return maillage_ext;

}


/*----------------------------------------------------------------------------
 * Extraction et conversion en connectivit� nodale externe d'un sous-ensemble
 * des faces d'un maillage.
 *
 * Les listes des faces � traiter sont optionnelles (si aucune des deux
 * n'est fournie, on extrait les faces de bord par d�faut); elle peuvent
 * ne pas �tre ordonn�es en entr�e, elle le seront toujours en sortie
 * (les faces �tant extraites au cours d'un parcours en ordre croissant,
 * la liste est r�ordonn�e pour assurer la coh�rence des liens des faces
 * extraites vers leurs faces parentes, construites � partir de cette liste).
 *----------------------------------------------------------------------------*/

fvm_nodal_t  * cs_maillage_extrait_fac_nodal
(
 const cs_mesh_t      *const mesh,          /* --> maillage                   */
 const char           *const nom,           /* --> nom � affecter             */
 const cs_int_t              nbr_liste_fac, /* --> taille de liste_fac[]      */
 const cs_int_t              nbr_liste_fbr, /* --> taille de liste_fbr[]      */
       cs_int_t              liste_fac[],   /* <-> liste optionnelle des faces
                                             *     internes � traiter (1 � n) */
       cs_int_t              liste_fbr[]    /* <-> liste optionnelle des faces
                                             *     de bord � traiter (1 � n)  */
)
{

  cs_int_t    ifac, i ;

  cs_int_t    nbr_fac_max = 0;
  cs_int_t    nbr_fbr_liste = 0 ;
  cs_int_t    nbr_fac_liste = 0 ;
  cs_int_t    nbr_fac_extr = 0 ;
  cs_int_t  * ind_fac_extr = NULL ;
  cs_int_t  * liste_fac_extr = NULL ;

  fvm_lnum_t  dec_num_faces[3];
  fvm_lnum_t  *pos_faces_som[2];
  fvm_lnum_t  *val_faces_som[2];

  fvm_gnum_t  *num_glob_fac = NULL;

  fvm_nodal_t  *maillage_ext;

  /* V�rification que le maillage contient bien les connectivit�s
     faces->sommets */

  if (mesh->b_face_vtx_idx == NULL || mesh->i_face_vtx_idx == NULL)
    bft_error(__FILE__, __LINE__, 0,
              _("Le maillage principal ne contient pas de connectivit�\n"
                "faces->sommets, indispensable � la reconstruction\n"
                "de la connectivit� nodale (cs_maillage_extrait_cel_nodal)."));

  /* Comptage du nombre de faces � convertir */

  nbr_fac_max = mesh->n_i_faces + mesh->n_b_faces;
  BFT_MALLOC(ind_fac_extr, nbr_fac_max, cs_int_t);

  /* Initialisation sous forme de marqueur */

  for (ifac = 0 ; ifac < nbr_fac_max ; ifac++)
    ind_fac_extr[ifac] = -1;

  if (nbr_liste_fbr == mesh->n_b_faces) {
    for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++)
      ind_fac_extr[ifac] = 1;
  }
  else if (liste_fbr != NULL) {
    for (ifac = 0 ; ifac < nbr_liste_fbr ; ifac++)
      ind_fac_extr[liste_fbr[ifac] - 1] = 1;
  }

  if (nbr_liste_fac == mesh->n_i_faces) {
    for (ifac = 0 ; ifac < mesh->n_i_faces ; ifac++)
      ind_fac_extr[ifac + mesh->n_b_faces] = 1;
  }
  else if (liste_fac != NULL) {
    for (ifac = 0 ; ifac < nbr_liste_fac ; ifac++)
      ind_fac_extr[liste_fac[ifac] - 1 + mesh->n_b_faces] = 1;
  }

  /* conversion indices marqu�s comme utilis�s en pointeurs (1 � n)
     et reconstruction des valeurs de liste_fbr[] et liste_fac[]
     de mani�re � s'assurer qu'elles soient tri�es */

  nbr_fbr_liste = 0;
  nbr_fac_liste = 0;

  if (liste_fbr != NULL) {
    for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++) {
      if (ind_fac_extr[ifac] == 1) {
        liste_fbr[nbr_fbr_liste] = ifac + 1;
        nbr_fbr_liste++;
      }
    }
  }
  else
    nbr_fbr_liste = CS_MIN(nbr_liste_fbr, mesh->n_b_faces);

  if (liste_fac != NULL) {
    for (ifac = 0, i = mesh->n_b_faces ;
         ifac < mesh->n_i_faces ;
         ifac++, i++) {
      if (ind_fac_extr[i] == 1) {
        liste_fac[nbr_fac_liste] = ifac + 1;
        nbr_fac_liste++;
      }
    }
  }
  else
    nbr_fac_liste = CS_MIN(nbr_liste_fac, mesh->n_i_faces);

  BFT_FREE(ind_fac_extr);

  /* Construction d'une liste continue (faces de bord, faces) */

  nbr_fac_extr = nbr_fbr_liste + nbr_fac_liste;

  BFT_MALLOC(liste_fac_extr, nbr_fac_extr, cs_int_t);

  if (liste_fbr != NULL) {
    for (ifac = 0 ; ifac < nbr_fbr_liste ; ifac++)
      liste_fac_extr[ifac] = liste_fbr[ifac];
  }
  else if (liste_fbr == NULL) { /* faces de bord par d�faut si aucune liste */
    for (ifac = 0 ; ifac < nbr_fbr_liste ; ifac++)
      liste_fac_extr[ifac] = ifac + 1;
  }

  if (liste_fac != NULL) {
    for (ifac = 0, i = nbr_fbr_liste ; ifac < nbr_fac_liste ; ifac++, i++)
      liste_fac_extr[i] = liste_fac[ifac] + mesh->n_b_faces;
  }
  else if (liste_fac == NULL) {
    for (ifac = 0, i = nbr_fbr_liste ; ifac < nbr_fac_liste ; ifac++, i++)
      liste_fac_extr[i] = ifac + mesh->n_b_faces + 1;
  }


  /* Construction de la connectivit� nodale */

  dec_num_faces[0] = 0;
  dec_num_faces[1] = mesh->n_b_faces + dec_num_faces[0];
  dec_num_faces[2] = mesh->n_i_faces + dec_num_faces[1];

  pos_faces_som[0] = mesh->b_face_vtx_idx;
  pos_faces_som[1] = mesh->i_face_vtx_idx;
  val_faces_som[0] = mesh->b_face_vtx_lst;
  val_faces_som[1] = mesh->i_face_vtx_lst;

  maillage_ext = fvm_nodal_create(nom, 3);

  fvm_nodal_from_desc_add_faces(maillage_ext,
                                nbr_fac_extr,
                                liste_fac_extr,
                                2,
                                dec_num_faces,
                                (const fvm_lnum_t **) pos_faces_som,
                                (const fvm_lnum_t **) val_faces_som,
                                NULL);

  fvm_nodal_set_shared_vertices(maillage_ext,
                                mesh->vtx_coord);

  BFT_FREE(liste_fac_extr);

  /* En cas de parall�lisme, tri des faces par num�ro ou indice
     global croissant */

  if (cs_glob_base_nbr > 1) {

    BFT_MALLOC(num_glob_fac, nbr_fac_max, fvm_gnum_t);

    if (mesh->init_b_face_num == NULL) {
      for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++)
        num_glob_fac[ifac] = mesh->global_b_face_num[ifac];
    }
    else {
      for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++)
        num_glob_fac[ifac] =
          mesh->global_b_face_num[mesh->init_b_face_num[ifac] - 1];
    }

    assert(mesh->n_g_b_faces + mesh->n_g_i_faces > 0);

    if (mesh->init_i_face_num == NULL) {
      for (ifac = 0, i = mesh->n_b_faces ;
           ifac < mesh->n_i_faces ;
           ifac++, i++)
        num_glob_fac[i] = mesh->global_i_face_num[ifac] + mesh->n_g_b_faces;
    }
    else {
      for (ifac = 0, i = mesh->n_b_faces ;
           ifac < mesh->n_i_faces ;
           ifac++, i++)
        num_glob_fac[i] = mesh->global_i_face_num[mesh->init_i_face_num[ifac] - 1]
                        + mesh->n_g_b_faces;
    }

  }

  /* Sans parall�lisme, on doit tout de m�me tenir compte d'une �ventuelle
     renum�rotation des faces */

  else if (mesh->init_i_face_num != NULL || mesh->init_b_face_num != NULL) {

    BFT_MALLOC(num_glob_fac, nbr_fac_max, fvm_gnum_t);

    if (mesh->init_b_face_num == NULL) {
      for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++)
        num_glob_fac[ifac] = ifac + 1;
    }
    else {
      for (ifac = 0 ; ifac < mesh->n_b_faces ; ifac++)
        num_glob_fac[ifac] = mesh->init_b_face_num[ifac] - 1;
    }

    if (mesh->init_i_face_num == NULL) {
      for (ifac = 0, i = mesh->n_b_faces ;
           ifac < mesh->n_i_faces ;
           ifac++, i++)
        num_glob_fac[i] = mesh->n_b_faces + ifac + 1;
    }
    else {
      for (ifac = 0, i = mesh->n_b_faces ;
           ifac < mesh->n_i_faces ;
           ifac++, i++)
        num_glob_fac[i] = mesh->n_b_faces + mesh->init_i_face_num[ifac];
    }

  }

  fvm_nodal_order_faces(maillage_ext, num_glob_fac);
  fvm_nodal_init_io_num(maillage_ext, num_glob_fac, 2);

  if (num_glob_fac != NULL)
    BFT_FREE(num_glob_fac);

  /* Tri des sommets par num�ro ou indice global croissant */

  fvm_nodal_order_vertices(maillage_ext, mesh->global_vtx_num);
  fvm_nodal_init_io_num(maillage_ext, mesh->global_vtx_num, 0);

  /* On a termin� */

  return maillage_ext;

}


/*============================================================================
 * Fonctions priv�es
 *============================================================================*/

#ifdef __cplusplus
}
#endif /* __cplusplus */
