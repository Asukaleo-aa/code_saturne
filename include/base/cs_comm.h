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

#ifndef __CS_COMM_H__
#define __CS_COMM_H__

/*============================================================================
 *  Communications avec d'autres codes (Syrthes)
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 *  Fichiers `include' locaux
 *----------------------------------------------------------------------------*/

#include "cs_base.h"


/*============================================================================
 *  D�finitions d'�numerations
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Type de message
 *----------------------------------------------------------------------------*/

typedef enum {

  CS_COMM_TYPE_BINAIRE,          /* Messages par fichiers binaires            */
  CS_COMM_TYPE_MPI,              /* Messages MPI                              */
  CS_COMM_TYPE_SOCKET            /* Messages par sockets IP                   */

} cs_comm_type_t;


/*----------------------------------------------------------------------------
 *  Emission ou r�ception de message
 *----------------------------------------------------------------------------*/

typedef enum {

  CS_COMM_MODE_RECEPTION,        /* Communication en r�ception                */
  CS_COMM_MODE_EMISSION          /* Communication en �mission                 */

} cs_comm_mode_t;


/*============================================================================
 *  D�finition de macros
 *============================================================================*/

#define CS_COMM_FIN_FICHIER                                "EOF"
#define CS_COMM_CMD_ARRET                             "cmd:stop"
#define CS_COMM_CMD_ITER_DEB                      "cmd:iter:deb"
#define CS_COMM_CMD_ITER_DEB_FIN              "cmd:iter:deb:fin"

#define CS_COMM_LNG_NOM_RUB            32   /* Longueur du nom d'une rubrique */

/*
 * Communications par socket : on pr�voit pour l'instant 8 codes coupl�s
                               au maximum ; cette valeur peut �tre modifi�e
                               par la variable d'environnement
                               CS_COMM_SOCKET_NBR_MAX
*/


/*============================================================================
 *  D�claration de structures
 *============================================================================*/

/*
  Pointeur associ� � un communicateur. La structure elle-m�me est d�clar�e
  dans le fichier "cs_comm.c", car elle n'est pas n�cessaire ailleurs.
*/

typedef struct _cs_comm_t cs_comm_t;


/*
  Structure de sauvegarde des donn�es d'une ent�te de message, permettant de
  simplifier le passage de ces donn�es � diff�rentes fonctions de traitement.
*/

typedef struct {

  cs_int_t   num_rub;                          /* Num�ro de rubrique associ�e */
  char       nom_rub[CS_COMM_LNG_NOM_RUB + 1]; /* Nom si num_rub = 0          */
  cs_int_t   nbr_elt;                          /* Nombre d'�l�ments           */
  cs_type_t  typ_elt;                          /* Type si nbr_elt > 0         */

} cs_comm_msg_entete_t;


/*=============================================================================
 * D�finitions de variables globales
 *============================================================================*/


/*============================================================================
 *  Prototypes de fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fonction qui initialise une communication
 *----------------------------------------------------------------------------*/

cs_comm_t * cs_comm_initialise
(
 const char          *const nom_emetteur,   /* --> partie "�metteur" du nom   */
 const char          *const nom_recepteur,  /* --> partie "recepteur du nom   */
 const char          *const chaine_magique, /* --> Cha�ne de v�rif. de type   */
 const cs_int_t             numero,         /* --> Compl�te le nom si non nul */
#if defined(_CS_HAVE_MPI)
 const cs_int_t             rang_proc,      /* --> Rang processus en comm
                                                    (< 0 si comm par fichier) */
#endif
 const cs_comm_mode_t       mode,           /* --> �mission ou r�ception      */
 const cs_comm_type_t       type,           /* --> Type de communication      */
 const cs_int_t             echo            /* --> �cho sur sortie principale
                                                    (< 0 si aucun, ent�te si 0,
                                                    n premiers et derniers
                                                    �l�ments si n)            */
);


/*----------------------------------------------------------------------------
 *  Fonction qui termine une communication
 *----------------------------------------------------------------------------*/

cs_comm_t * cs_comm_termine
(
 cs_comm_t *comm
);


/*----------------------------------------------------------------------------
 *  Fonction qui renvoie un pointeur sur le nom d'une communication
 *----------------------------------------------------------------------------*/

const char * cs_comm_ret_nom
(
 const cs_comm_t  *const comm
);


/*----------------------------------------------------------------------------
 *  Envoi d'un message
 *----------------------------------------------------------------------------*/

void cs_comm_envoie_message
(
 const cs_int_t          num_rub,           /* --> Num. rubrique associ�e     */
 const char              nom_rub[CS_COMM_LNG_NOM_RUB], /* --> Si num_rub = 0  */
 const cs_int_t          nbr_elt,           /* --> Nombre d'�l�ments          */
 const cs_type_t         typ_elt,           /* --> Type si nbr_elt > 0        */
       void       *const elt,               /* --> �l�ments si nbr_elt > 0    */
 const cs_comm_t  *const comm
);


/*----------------------------------------------------------------------------
 *  R�ception de l'entete d'un message ; renvoie le nombre d'�l�ments du
 *  corps du message.
 *----------------------------------------------------------------------------*/

cs_int_t cs_comm_recoit_entete
(
       cs_comm_msg_entete_t  *const entete,  /* <-- ent�te du message         */
 const cs_comm_t             *const comm
);


/*----------------------------------------------------------------------------
 *  R�ception du corps d'un message.
 *
 *  Si la zone m�moire destin�e � recevoir les donn�es existe deja, on
 *  fournit un pointeur "elt" sur cette zone ; la fonction renvoie alors
 *  ce m�me pointeur. Sinon (si "elt" est � NULL), la m�moire est allou�e
 *  ici, et la fonction renvoie un pointeur sur cette zone.
 *----------------------------------------------------------------------------*/

void * cs_comm_recoit_corps
(
 const cs_comm_msg_entete_t  *const entete,  /* --> ent�te du message         */
       void                  *const elt,     /* --> Pointeur sur les �l�ments */
 const cs_comm_t             *const comm
);


#if defined(_CS_HAVE_SOCKET)

/*----------------------------------------------------------------------------
 *  Fonction qui ouvre un "socket" IP pour pr�parer ce mode de communication
 *----------------------------------------------------------------------------*/

void cs_comm_init_socket
(
 void
);

/*----------------------------------------------------------------------------
 *  Fonction qui ferme le "socket" IP avec ce mode de communication
 *----------------------------------------------------------------------------*/

void cs_comm_termine_socket
(
 void
);

#endif /* _CS_HAVE_SOCKET */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_COMM_H__ */
