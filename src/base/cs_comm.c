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
 *  Communications avec d'autres codes (Syrthes)
 *============================================================================*/

/* includes syst�me */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_CS_HAVE_MPI)
#include <mpi.h>
#endif

#if defined(_CS_HAVE_MPI) && defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
#include <mpe.h>
#endif

#if defined(_CS_HAVE_SOCKET)
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/* Includes librairie BFT et FVM */

#include <bft_error.h>
#include <bft_file.h>
#include <bft_mem.h>
#include <bft_printf.h>

/* Includes librairie */

#include "cs_base.h"


/*----------------------------------------------------------------------------
 *  Fichiers  `include' associ�s au fichier courant
 *----------------------------------------------------------------------------*/

#include "cs_comm.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*============================================================================
 *  Structures locales
 *============================================================================*/

struct _cs_comm_t {

  char            *nom;          /* Nom du communicateur                      */

  bft_file_t      *fic;          /* Pointeur sur fichier associ�              */
  cs_int_t         rang_proc;    /* Rang processus en communication (MPI)     */
  int              sock;         /* Num�ro de socket                          */

  cs_comm_mode_t   mode;         /* Mode de communication                     */
  cs_comm_type_t   type;         /* Type de codage des donnees                */
  cs_bool_t        swap_endian;  /* Permutation des octets ?                  */
  cs_int_t         echo;         /* Niveau d'impression des donnees           */

};


/*============================================================================
 *  Constantes et Macros
 *============================================================================*/

#define CS_COMM_LNG_NOM_TYPE_ELT         2    /* Longueur du nom de type      */

#define CS_COMM_SOCKET_ENTETE            "CS_comm_socket"

#define CS_COMM_SOCKET_NBR_MAX          8
#define CS_LOC_COMM_LNG_HOSTNAME      256
#define CS_LOC_COMM_LNG_NOM_MAX       256

/*
  Si SSIZE_MAX non d�finie via les "includes" syst�me, on prend la valeur
  minimale requise par POSIX (pour read/write de bas niveau utilis�s
  avec les sockets).
*/

#if !defined(SSIZE_MAX)
#define SSIZE_MAX  32767
#endif


/*============================================================================
 *  Variables globales statiques
 *============================================================================*/

static char  cs_comm_nom_typ_elt_char[] = "c ";  /* Type "cha�ne"  */
static char  cs_comm_nom_typ_elt_int[]  = "i ";  /* Type "entier"  */
static char  cs_comm_nom_typ_elt_real[] = "r8";  /* Type "r�el"    */


#if defined(_CS_HAVE_SOCKET)

static cs_bool_t       cs_glob_comm_little_endian = CS_FALSE;

static char  cs_glob_comm_sock_nom_hote[CS_LOC_COMM_LNG_HOSTNAME + 1];
static int   cs_glob_comm_sock_num_port = -1;

static int             cs_glob_comm_socket = 0;
struct sockaddr_in     cs_glob_comm_addr_sock;

static char  cs_glob_comm_err_socket[]
  = N_("Erreur pour la communication par socket : "
       " %s (noeud %4d)\n");

#endif /* _CS_HAVE_SOCKET */

/* Instrumentation MPE */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
static int cs_glob_mpe_comm_ouvre;
static int cs_glob_mpe_comm_entete;
static int cs_glob_mpe_comm_corps;
#endif

/*============================================================================
 *  Prototypes de fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fonction qui construit le descripteur du fichier d'interface et initialise
 *  ce fichier par l'envoi ou la lecture d'une �ventuelle "cha�ne magique"
 *  servant a v�rifier le bon format des fichiers
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_fic_ouvre
(
       cs_comm_t  *const comm,
 const char       *const  nom,
 const char       *const chaine_magique
);


/*----------------------------------------------------------------------------
 *  Fonction qui ferme le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_fic_ferme
(
 cs_comm_t  *comm
);


#if defined(_CS_HAVE_MPI)

/*----------------------------------------------------------------------------
 *  Fonction qui initialise une communication MPI par l'envoi ou la lecture
 *  d'une �ventuelle "cha�ne magique" servant a v�rifier le bon format des
 *  donn�es
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_ouvre
(
       cs_comm_t         *comm,
 const char       *const  chaine_magique
);


/*----------------------------------------------------------------------------
 *  Fonction qui �change une ent�te de rubrique via MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_entete
(
       cs_int_t   *const num_rub,
       char       *const nom_rub,
       cs_int_t   *const nbr_elt_rub,
       char       *const nom_typ_elt,
 const cs_comm_t  *const comm
);


/*----------------------------------------------------------------------------
 *  Fonction qui �change le corps d'une rubrique via MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_corps
(
       void        *const elt_rub,
 const cs_int_t           nbr_elt_rub,
       cs_type_t          typ_elt,
 const cs_comm_t   *const comm
);


/*----------------------------------------------------------------------------
 *  Fonction qui imprime un message d'erreur en cas de probl�me de
 *  communication MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_msg_err
(
 const cs_comm_t  *const comm,
 const int               error
);

#endif /* (_CS_HAVE_MPI) */


#if defined(_CS_HAVE_SOCKET)

/*----------------------------------------------------------------------------
 *  Fonction qui initialise une connection par "socket"
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_connect
(
 cs_comm_t  *const  comm
);


/*----------------------------------------------------------------------------
 *  Fonction qui assure l'�change de la "chaine magique" via les sockets
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_ouvre
(
       cs_comm_t  *const  comm          ,
 const char       *const  nom_fic       ,
 const char       *const  chaine_magique
);


/*----------------------------------------------------------------------------
 *  Fonction qui ferme la connextion avec le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_ferme
(
 cs_comm_t  *comm
);


/*----------------------------------------------------------------------------
 *  Fonction qui �crit un enregistrement dans le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_ecrit_sock
(
 const cs_comm_t  *const comm,
 const cs_byte_t  *      rec,
 const size_t            nbr,
       cs_type_t         typ_e
);


/*----------------------------------------------------------------------------
 *  Fonction qui lit un enregistrement dans le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_lit_sock
(
 const cs_comm_t  *const comm,
       cs_byte_t  *      rec,
 const size_t            nbr,
       cs_type_t         typ_e
);

#endif /* (_CS_HAVE_SOCKET) */


/*----------------------------------------------------------------------------
 *  Affichage de l'attente d'�change d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_pre
(
 const cs_comm_t  *const comm
);


/*----------------------------------------------------------------------------
 *  Affichage de l'entete d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_entete
(
 const cs_int_t           num_rub,
 const char        *const nom_rub,
 const cs_int_t           nbr_elt,
 const cs_type_t          typ_elt
);


/*----------------------------------------------------------------------------
 *  Affichage (partiel) du contenu d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_donnees
(
 const cs_int_t          echo,
 const cs_int_t          nbr_elt,
 const cs_type_t         typ_elt,
 const void       *const elt_rub
);


/*----------------------------------------------------------------------------
 *  Fonction qui �crit un enregistrement dans le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_ecrit_rec
(
 const cs_comm_t   *const comm,
 const void        *const rec,
 const size_t             nbr,
       cs_type_t          typ
);


/*----------------------------------------------------------------------------
 *  Fonction qui lit un enregistrement dans le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_lit_rec
(
 const cs_comm_t   *const comm,
       void        *const rec ,
 const size_t             nbr,
       cs_type_t          typ
);


/*============================================================================
 *  D�finitions de fonctions publiques
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
)
{
  unsigned    int_endian;

  char       *nom_fic = NULL;
  cs_comm_t  *comm = NULL;


  BFT_MALLOC(comm, 1, cs_comm_t);

  /* Construction du nom du communicateur */

  BFT_MALLOC(comm->nom,
             strlen(nom_emetteur) + strlen("_vers_") + strlen(nom_recepteur) + 1
             + (numero == 0 ? 0 : 4 + 1),
             char);

  sprintf(comm->nom, "%s_vers_%s", nom_emetteur, nom_recepteur);

  if (numero > 0)
    sprintf(comm->nom + strlen(comm->nom), ".%04d", numero);


  /* Initialisation des autres champs */

  comm->mode = mode;
  comm->type = type;
  comm->echo = echo;

  comm->fic  = NULL;

#if defined(_CS_HAVE_MPI)
  comm->rang_proc = rang_proc;
#else
  comm->rang_proc = -1;
#endif


  /* Test si syst�me "big-endian" ou "little-endian" */

  comm->swap_endian = CS_FALSE;

  int_endian = 0;
  *((char *)(&int_endian)) = '\1';

  if (int_endian == 1)
    comm->swap_endian = CS_TRUE;

#if defined(DEBUG) && !defined(NDEBUG)

  else {
    int_endian = 0;
    *((char *)(&int_endian) + sizeof(unsigned) - 1) = '\1';
    assert(int_endian == 1);
  }

#endif

  /* Info sur la cr�ation de l'interface */

  bft_printf(_("\n  Ouverture de la communication :  %s ..."), comm->nom);
  bft_printf_flush();

#if defined(_CS_HAVE_SOCKET)
  if (comm->type == CS_COMM_TYPE_SOCKET)
    cs_loc_comm_sock_connect(comm);
#endif /* (_CS_HAVE_SOCKET) */


  /* Cr�ation du descripteur de fichier d'interface */
  /*------------------------------------------------*/

  if (comm->type == CS_COMM_TYPE_MPI) {

#if defined(_CS_HAVE_MPI)
    cs_loc_comm_mpi_ouvre(comm, chaine_magique);
#else
    assert(comm->rang_proc < 0);
#endif

  }
  else {

    if (cs_glob_base_nbr == 1) {

      nom_fic = comm->nom;

    }
    else if (cs_glob_base_nbr > 1) {

      BFT_MALLOC(nom_fic,
                 strlen(nom_emetteur) + strlen("_vers_") + strlen(nom_recepteur)
                 + 1 + (cs_glob_base_nbr == 1 ? 0 : 4 + 2) + (numero == 0 ? 0 : 4 + 1),
                 char);

      if (mode == CS_COMM_MODE_EMISSION)
        sprintf(nom_fic, "%s_n%04d_vers_%s",
                nom_emetteur, cs_glob_base_rang + 1, nom_recepteur);
      else if (mode == CS_COMM_MODE_RECEPTION)
        sprintf(nom_fic, "%s_vers_%s_n%04d",
                nom_emetteur, nom_recepteur, cs_glob_base_rang + 1);
      else
        assert(   mode == CS_COMM_MODE_EMISSION
               || mode == CS_COMM_MODE_RECEPTION);

      if (numero > 0)
        sprintf(nom_fic + strlen(nom_fic), ".%04d", numero);

    }

    if (comm->type == CS_COMM_TYPE_BINAIRE)

      cs_loc_comm_fic_ouvre(comm, nom_fic, chaine_magique);

#if defined(_CS_HAVE_SOCKET)
    else if (comm->type == CS_COMM_TYPE_SOCKET)
      cs_loc_comm_sock_ouvre(comm, nom_fic, chaine_magique);
#endif /* (_CS_HAVE_SOCKET) */

    if (cs_glob_base_nbr > 1)
      BFT_FREE(nom_fic);

  }

  /* Info sur le succ�s de la cr�ation de l'interface */

  bft_printf(" [ok]\n");
  bft_printf_flush();

  return comm;

}


/*----------------------------------------------------------------------------
 *  Fonction qui termine une communication
 *----------------------------------------------------------------------------*/

cs_comm_t * cs_comm_termine
(
 cs_comm_t *comm
)
{

  /* Info sur la fermeture du fichier d'interface */

  bft_printf(_("\n  Fermeture de la communication :  %s\n"), comm->nom);
  bft_printf_flush();

  if (comm->type == CS_COMM_TYPE_BINAIRE)
    cs_loc_comm_fic_ferme(comm);

#if defined(_CS_HAVE_SOCKET)

  else if (comm->type == CS_COMM_TYPE_SOCKET)
    cs_loc_comm_sock_ferme(comm);

#endif /* (_CS_HAVE_SOCKET) */

  BFT_FREE(comm->nom);
  BFT_FREE(comm);

  return NULL;

}


/*----------------------------------------------------------------------------
 *  Fonction qui renvoie un pointeur sur le nom d'une communication
 *----------------------------------------------------------------------------*/

const char * cs_comm_ret_nom
(
 const cs_comm_t  *const comm
)
{
  assert(comm != NULL);

  return(comm->nom);
}


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
)
{

  char   nom_rub_ecr[CS_COMM_LNG_NOM_RUB + 1];

  char  *nom_typ_elt;
  char   nom_typ_elt_ecr[CS_COMM_LNG_NOM_TYPE_ELT + 1];


  assert(comm != NULL);
  assert(nbr_elt >= 0);


  /* nom de la rubrique */

  sprintf(nom_rub_ecr,
          "%-*.*s",
          CS_COMM_LNG_NOM_RUB,
          CS_COMM_LNG_NOM_RUB,
          nom_rub);


  /* nom du type d'elements */

  if (nbr_elt != 0) {

    switch(typ_elt) {

    case CS_TYPE_cs_int_t:
      nom_typ_elt = cs_comm_nom_typ_elt_int;
      break;

    case CS_TYPE_cs_real_t:
      nom_typ_elt = cs_comm_nom_typ_elt_real;
      break;

    case CS_TYPE_char:
      nom_typ_elt = cs_comm_nom_typ_elt_char;
      break;

    default:
      assert(   typ_elt == CS_TYPE_cs_int_t
             || typ_elt == CS_TYPE_cs_real_t
             || typ_elt == CS_TYPE_char);

    } /* Fin `swich(typ_elt_e)' */

    sprintf(nom_typ_elt_ecr,
            "%-*.*s",
            CS_COMM_LNG_NOM_TYPE_ELT,
            CS_COMM_LNG_NOM_TYPE_ELT,
            nom_typ_elt);

  }

  if (comm->echo  >= 0)
    cs_loc_comm_echo_pre(comm);


  /* Communication par fichier */
  /*---------------------------*/

  if (comm->type == CS_COMM_TYPE_BINAIRE) {

    /* num�ro de type de la rubrique */

    cs_loc_comm_ecrit_rec(comm,
                          (const void *)(&num_rub),
                          1,
                          CS_TYPE_cs_int_t);

    /* nom de type de la rubrique */

    if (num_rub == 0)
      cs_loc_comm_ecrit_rec(comm,
                            (const void *) nom_rub_ecr,
                            CS_COMM_LNG_NOM_RUB,
                            CS_TYPE_char);

    /* nombre d'�l�ments */

    cs_loc_comm_ecrit_rec(comm,
                          (const void *)(&nbr_elt),
                          1,
                          CS_TYPE_cs_int_t);

    if (nbr_elt != 0) {

      /* nom du type d'�l�ments */

      cs_loc_comm_ecrit_rec(comm,
                            (const void *) nom_typ_elt_ecr,
                            CS_COMM_LNG_NOM_TYPE_ELT,
                            CS_TYPE_char);

      /* valeurs des �l�ments */

      cs_loc_comm_ecrit_rec(comm,
                            (const void *) elt,
                            (size_t) nbr_elt,
                            typ_elt);

    } /* Fin : s'il y a des �l�ments a �crire */

    bft_file_flush(comm->fic);

  }

#if defined(_CS_HAVE_MPI)

  /* Communication par MPI */
  /*-----------------------*/

  else if (comm->type == CS_COMM_TYPE_MPI) {

    cs_int_t  num_rub_ecr     = num_rub;
    cs_int_t  nbr_elt_rub_ecr = nbr_elt;

    cs_loc_comm_mpi_entete(&num_rub_ecr,
                           nom_rub_ecr,
                           &nbr_elt_rub_ecr,
                           nom_typ_elt_ecr,
                           comm);

    if (nbr_elt > 0)
      cs_loc_comm_mpi_corps((void *) elt,
                            nbr_elt,
                            typ_elt,
                            comm);

  }

#endif /* (_CS_HAVE_MPI) */

#if defined(_CS_HAVE_SOCKET)

  /* Communication par socket */
  /*--------------------------*/

  else if (comm->type == CS_COMM_TYPE_SOCKET) {

    /* num�ro de type de la rubrique */

    cs_loc_comm_ecrit_sock(comm,
                           (const void *)(&num_rub),
                           1,
                           CS_TYPE_cs_int_t);

    /* nom de type de la rubrique */

    if (num_rub == 0)
      cs_loc_comm_ecrit_sock(comm,
                             (const void *) nom_rub_ecr,
                             CS_COMM_LNG_NOM_RUB,
                             CS_TYPE_char);

    /* nombre d'�l�ments */

    cs_loc_comm_ecrit_sock(comm,
                           (const void *)(&nbr_elt),
                           1,
                           CS_TYPE_cs_int_t);

    if (nbr_elt != 0) {

      /* nom du type d'�l�ments */

      cs_loc_comm_ecrit_sock(comm,
                             (const void *) nom_typ_elt_ecr,
                             CS_COMM_LNG_NOM_TYPE_ELT,
                             CS_TYPE_char);

      /* valeurs des �l�ments */

      cs_loc_comm_ecrit_sock(comm,
                             (const void *) elt,
                             (size_t) nbr_elt,
                             typ_elt);

    } /* Fin : s'il y a des �l�ments � �crire */

  }

#endif /* (_CS_HAVE_SOCKET) */

  /* Affichage �ventuel */

  if (comm->echo  >= 0)
    cs_loc_comm_echo_entete(num_rub,
                            nom_rub,
                            nbr_elt,
                            typ_elt);

  if (comm->echo > 0)
    cs_loc_comm_echo_donnees(comm->echo,
                             nbr_elt,
                             typ_elt,
                             elt);

}


/*----------------------------------------------------------------------------
 *  R�ception de l'entete d'un message ; renvoie le nombre d'�l�ments du
 *  corps du message.
 *----------------------------------------------------------------------------*/

cs_int_t cs_comm_recoit_entete
(
       cs_comm_msg_entete_t  *const entete,  /* <-- ent�te du message         */
 const cs_comm_t             *const comm
)
{
  char   nom_typ_elt[CS_COMM_LNG_NOM_TYPE_ELT + 1];

  assert(comm  != NULL);

  entete->nbr_elt = 0;

  if (comm->echo >= 0)
    cs_loc_comm_echo_pre(comm);


  /* Communication par fichier */
  /*---------------------------*/

  if (comm->type == CS_COMM_TYPE_BINAIRE) {

    /* num�ro de type de la rubrique */

    cs_loc_comm_lit_rec(comm,
                        (void *) &(entete->num_rub),
                        1,
                        CS_TYPE_cs_int_t);

    /* nom de type de la rubrique */

    if (entete->num_rub == 0)
      cs_loc_comm_lit_rec(comm,
                          (void *) &(entete->nom_rub),
                          CS_COMM_LNG_NOM_RUB,
                          CS_TYPE_char);

    /* nombre d'�l�ments */

    cs_loc_comm_lit_rec(comm,
                        (void *) &(entete->nbr_elt),
                        1,
                        CS_TYPE_cs_int_t);


    if (entete->nbr_elt != 0) {

      /* nom du type d'�l�ments */

      cs_loc_comm_lit_rec(comm,
                          (void *) nom_typ_elt,
                          CS_COMM_LNG_NOM_TYPE_ELT,
                          CS_TYPE_char);

    } /* Fin : s'il y a des elements � lire */

  }

#if defined(_CS_HAVE_MPI)

  /* Communication par MPI */
  /*-----------------------*/

  else if (comm->type == CS_COMM_TYPE_MPI) {

    cs_loc_comm_mpi_entete(&(entete->num_rub),
                           entete->nom_rub,
                           &(entete->nbr_elt),
                           nom_typ_elt,
                           comm);

  }

#endif /* (_CS_HAVE_MPI) */

#if defined(_CS_HAVE_SOCKET)

  /* Communication par socket */
  /*--------------------------*/

  else if (comm->type == CS_COMM_TYPE_SOCKET) {

    /* num�ro de type de la rubrique */

    cs_loc_comm_lit_sock(comm,
                         (void *) &(entete->num_rub),
                         1,
                         CS_TYPE_cs_int_t);

    /* nom de type de la rubrique */

    if (entete->num_rub == 0)
      cs_loc_comm_lit_sock(comm,
                           (void *) &(entete->nom_rub),
                           CS_COMM_LNG_NOM_RUB,
                           CS_TYPE_char);

    /* nombre d'�l�ments */

    cs_loc_comm_lit_sock(comm,
                         (void *) &(entete->nbr_elt),
                         1,
                         CS_TYPE_cs_int_t);


    if (entete->nbr_elt != 0) {

      /* nom du type d'�l�ments */

      cs_loc_comm_lit_sock(comm,
                           (void *) nom_typ_elt,
                           CS_COMM_LNG_NOM_TYPE_ELT,
                           CS_TYPE_char);

    } /* Fin : s'il y a des elements � lire */

  }

#endif /* (_CS_HAVE_SOCKET) */

  entete->nom_rub[CS_COMM_LNG_NOM_RUB] = '\0';

  if (entete->nbr_elt != 0) {

    nom_typ_elt[CS_COMM_LNG_NOM_TYPE_ELT] = '\0';

    if (strcmp(nom_typ_elt, cs_comm_nom_typ_elt_int) == 0)
      entete->typ_elt = CS_TYPE_cs_int_t;

    else if (strcmp(nom_typ_elt, cs_comm_nom_typ_elt_real) == 0)
      entete->typ_elt = CS_TYPE_cs_real_t;

    else if (strcmp(nom_typ_elt, cs_comm_nom_typ_elt_char) == 0)
      entete->typ_elt = CS_TYPE_char;

    else
      assert(   strcmp(nom_typ_elt, cs_comm_nom_typ_elt_int) == 0
             || strcmp(nom_typ_elt, cs_comm_nom_typ_elt_real) == 0
             || strcmp(nom_typ_elt, cs_comm_nom_typ_elt_char) == 0);

  }


  /* Affichage eventuel */

  if (comm->echo >= 0)
    cs_loc_comm_echo_entete(entete->num_rub,
                            entete->nom_rub,
                            entete->nbr_elt,
                            entete->typ_elt);


  /* Transmission du nombre d'elements � lire */

  return entete->nbr_elt;

}


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
)
{

  cs_int_t    ind;
  void      *_elt_rub;


  assert(comm  != NULL);
  assert(entete->nbr_elt >= 0);


  _elt_rub = elt;

  if (_elt_rub == NULL && entete->nbr_elt != 0) {

    switch(entete->typ_elt) {

    case CS_TYPE_cs_int_t:
      {
        cs_int_t  *elt_rub_int;

        BFT_MALLOC(elt_rub_int, entete->nbr_elt, cs_int_t);
        _elt_rub = (void *) elt_rub_int;
      }
      break;

    case CS_TYPE_cs_real_t:
      {
        cs_real_t  *elt_rub_rea;

        BFT_MALLOC(elt_rub_rea, entete->nbr_elt, cs_real_t);
        _elt_rub = (void *)elt_rub_rea;
      }
      break;

    case CS_TYPE_char:
      {
        char  *elt_rub_cha;

        BFT_MALLOC(elt_rub_cha, entete->nbr_elt + 1, char);
        _elt_rub = (void *)elt_rub_cha;
      }
      break;

    default:
      assert(   entete->typ_elt == CS_TYPE_cs_int_t
             || entete->typ_elt == CS_TYPE_cs_real_t
             || entete->typ_elt == CS_TYPE_char);

    }

  }

  /* valeurs des �l�ments */

  if (entete->nbr_elt != 0) {

    /* Communication par fichier */

    if (comm->type == CS_COMM_TYPE_BINAIRE)
      cs_loc_comm_lit_rec(comm,
                          (void *)_elt_rub,
                          (size_t) entete->nbr_elt,
                          entete->typ_elt);

#if defined(_CS_HAVE_MPI)

    /* Communication par MPI */

    else if (comm->type == CS_COMM_TYPE_MPI)
      cs_loc_comm_mpi_corps((void *)_elt_rub,
                            entete->nbr_elt,
                            entete->typ_elt,
                            comm);

#endif /* (_CS_HAVE_MPI) */

#if defined(_CS_HAVE_SOCKET)

    /* Communication par socket */

    else if (comm->type == CS_COMM_TYPE_SOCKET)
      cs_loc_comm_lit_sock(comm,
                           (void *)_elt_rub,
                           (size_t) entete->nbr_elt,
                           entete->typ_elt);

#endif /* (_CS_HAVE_SOCKET) */

    /* V�rifications */

    if (entete->typ_elt == CS_TYPE_char) {
      for (ind = 0 ;
           ind < entete->nbr_elt && ((char *)_elt_rub)[ind] != '\0' ;
           ind++);
      ((char *)_elt_rub)[ind] = '\0';
    }


    /* Affichage �ventuel */

    if (comm->echo > 0)
      cs_loc_comm_echo_donnees(comm->echo,
                               entete->nbr_elt,
                               entete->typ_elt,
                               _elt_rub);


  } /* Fin : s'il y a des �l�ments a lire */

  /* Transmission des valeurs lues */

  return _elt_rub;

}

#if defined(_CS_HAVE_SOCKET)

/*----------------------------------------------------------------------------
 *  Fonction qui ouvre un "socket" IP pour pr�parer ce mode de communication
 *----------------------------------------------------------------------------*/

void cs_comm_init_socket
(
 void
)
{
  char       chaine[CS_LOC_COMM_LNG_HOSTNAME + 1];

  int        nbr_connect_max;
  int        num_port;

#if defined(_CS_ARCH_Linux)
  socklen_t long_sock;
#else
  int       long_sock;  /* size_t d'apres standard SUS-v2, mais d'apres
                           man gethostbyname sous Linux, le standard est
                           mauvais, on doit avoir un int (ou socklen_t) */
#endif

  unsigned  int_endian;

  struct sockaddr_in   addr_sock;
  struct hostent      *ent_hote;


  int rang = (cs_glob_base_rang == -1 ? 0 : cs_glob_base_rang);

  /* Initialisations */

  nbr_connect_max = 0;

  if (getenv("CS_COMM_SOCKET_NBR_MAX") != NULL)
    nbr_connect_max = atoi(getenv("CS_COMM_SOCKET_NBR_MAX"));

  if (nbr_connect_max == 0)
    nbr_connect_max = CS_COMM_SOCKET_NBR_MAX;

  /* Test si syst�me "big-endian" (r�f�rence r�seau) ou "little-endian" */

  cs_glob_comm_little_endian = CS_FALSE;

  int_endian = 0;
  *((char *) (&int_endian)) = '\1';

  if (int_endian == 1)
    cs_glob_comm_little_endian = CS_TRUE;

#if defined(DEBUG) && !defined(NDEBUG)
  else {
    int_endian = 0;
    *((char *) (&int_endian) + sizeof(unsigned) - 1) = '\1';
    assert (int_endian == 1);
  }
#endif

  /* Cr�ation du socket serveur */

  cs_glob_comm_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (cs_glob_comm_socket == -1)
    bft_error(__FILE__, __LINE__, errno,
              _("Erreur d'initialisation du support de communication "
                "par socket.\n"));

  /* Pr�paration � l'utilisation */

  long_sock = sizeof(addr_sock);

  memset((char *) &addr_sock, 0, long_sock);

  addr_sock.sin_family = AF_INET;
  addr_sock.sin_addr.s_addr = INADDR_ANY;
  addr_sock.sin_port = 0;

  if (cs_glob_comm_little_endian == CS_TRUE) {
    bft_file_swap_endian(&(addr_sock.sin_addr.s_addr),
                         &(addr_sock.sin_addr.s_addr),
                         sizeof(addr_sock.sin_addr.s_addr),
                         1);
    bft_file_swap_endian(&(addr_sock.sin_port),
                         &(addr_sock.sin_port),
                         sizeof(addr_sock.sin_port),
                         1);
  }

  if (gethostname(chaine, CS_LOC_COMM_LNG_HOSTNAME) < 0)
    bft_error(__FILE__, __LINE__, errno,
              _("Erreur de r�cup�ration du nom de la machine"));
  chaine[CS_LOC_COMM_LNG_HOSTNAME] = '\0';

  ent_hote = gethostbyname(chaine);
  memcpy(ent_hote->h_addr, &addr_sock.sin_addr, ent_hote->h_length);

  if (bind(cs_glob_comm_socket,
           (struct sockaddr *)&addr_sock,
           long_sock) != 0)
    bft_error(__FILE__, __LINE__, errno,
              _("Erreur d'initialisation du support de communication "
                "par socket.\n"));

  if (listen(cs_glob_comm_socket, nbr_connect_max) < 0)
    bft_error(__FILE__, __LINE__, errno,
              _("Erreur d'initialisation du support de communication "
                "par socket.\n"));

  /* R�cup�ration du num�ro de service affect� */

  if (getsockname(cs_glob_comm_socket,
                  (struct sockaddr *)&addr_sock,
                  &long_sock) != 0)
    bft_error(__FILE__, __LINE__, errno,
              _("Erreur d'initialisation du support de communication "
                "par socket.\n"));

  num_port = addr_sock.sin_port;
  if (cs_glob_comm_little_endian == CS_TRUE) {
    bft_file_swap_endian(&(addr_sock.sin_port),
                         &(addr_sock.sin_port),
                         sizeof(addr_sock.sin_port), 1);
    num_port = addr_sock.sin_port;
    bft_file_swap_endian(&(addr_sock.sin_port),
                         &(addr_sock.sin_port),
                         sizeof(addr_sock.sin_port), 1);
  }

  /* Sauvegarde de la structure dans la variable globale associ�e */

  cs_glob_comm_addr_sock = addr_sock;

  /* Ecriture dans l'ordre des processus du nom de l'h�te et du port */

  if (rang == 0) {

    /* Impression du message de transfert des caract�ristiques
       sur le listing pour le rang "0" */

    bft_printf(_("\n  Communication possible sur %s, port %d\n\n"),
               chaine, num_port);
    bft_printf_flush();

  }

  memcpy(cs_glob_comm_sock_nom_hote, chaine, CS_LOC_COMM_LNG_HOSTNAME);
  cs_glob_comm_sock_nom_hote[CS_LOC_COMM_LNG_HOSTNAME] = '\0';
  cs_glob_comm_sock_num_port = num_port;

}

/*----------------------------------------------------------------------------
 *  Fonction qui ferme le "socket" IP avec ce mode de communication
 *----------------------------------------------------------------------------*/

void cs_comm_termine_socket
(
 void
)
{

  if (cs_glob_comm_socket == 0)
    return;

  close(cs_glob_comm_socket);

  bft_printf(_("\nFermeture du socket ...\t [ok]\n"));
  bft_printf_flush();

}

#endif /* _CS_HAVE_SOCKET */


/*============================================================================
 *  D�finitions de fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fonction qui construit le descripteur du fichier d'interface et initialise
 *  ce fichier par l'envoi ou la lecture d'une eventuelle "cha�ne magique"
 *  servant a v�rifier le bon format des fichiers
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_fic_ouvre
(
       cs_comm_t  *const  comm,
 const char       *const  nom,
 const char       *const  chaine_magique
)
{

  bft_file_type_t fic_typ_comm;
  bft_file_mode_t fic_mod_comm;


  /* Pr�paration de l'ouverture du fichier */

  switch(comm->type) {

  case CS_COMM_TYPE_BINAIRE:
    fic_typ_comm = BFT_FILE_TYPE_BINARY;
    break;

  default:
    assert(comm->type == CS_COMM_TYPE_BINAIRE);

  }

  switch(comm->mode) {

  case CS_COMM_MODE_RECEPTION:
    fic_mod_comm = BFT_FILE_MODE_READ;
    break;

  case CS_COMM_MODE_EMISSION:
    fic_mod_comm = BFT_FILE_MODE_WRITE;
    break;

  default:
    assert(   comm->mode == CS_COMM_MODE_RECEPTION
           || comm->mode == CS_COMM_MODE_EMISSION);

  }


  /* Cr�ation du descripteur du fichier d'interface */

  comm->fic = bft_file_open(nom,
                            fic_mod_comm,
                            fic_typ_comm);
  bft_file_set_big_endian(comm->fic);


  /*-----------------------------------------------------*/
  /* �criture ou lecture �ventuelle d'une chaine magique */
  /*-----------------------------------------------------*/

  if (comm->mode == CS_COMM_MODE_RECEPTION) {

    char      *chaine_magique_lue;
    cs_int_t   lng_chaine_magique = strlen(chaine_magique);

    BFT_MALLOC(chaine_magique_lue, lng_chaine_magique + 1, char);

    cs_loc_comm_lit_rec(comm,
                        (void *)(chaine_magique_lue),
                        strlen(chaine_magique),
                        CS_TYPE_char);

    chaine_magique_lue[lng_chaine_magique] = '\0';

    /* Si la chaine magique ne correspond pas, on a une erreur */

    if (strcmp(chaine_magique_lue, chaine_magique) != 0) {

      bft_error(__FILE__, __LINE__, 0,
                _("Erreur � la lecture du fichier de communication : "
                  "\"%s\".\n"
                  "Le format de l'interface n'est pas � la bonne version.\n"
                  "La cha�ne magique rep�re la version du format "
                  "d'interface :\n"
                  "cha�ne magique lue      : \"%s\"\n"
                  "cha�ne magique actuelle : \"%s\"\n"),
                comm->nom, chaine_magique_lue, chaine_magique);

    }

    BFT_FREE(chaine_magique_lue);

  }
  else if (comm->mode == CS_COMM_MODE_EMISSION) {

    cs_loc_comm_ecrit_rec(comm,
                          (const void *)(chaine_magique),
                          strlen(chaine_magique),
                          CS_TYPE_char);

    bft_file_flush(comm->fic);

  }

}


/*----------------------------------------------------------------------------
 *  Fonction qui ferme le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_fic_ferme
(
 cs_comm_t  *comm
)
{

  comm->fic = bft_file_free(comm->fic);

}


#if defined(_CS_HAVE_MPI)

/*----------------------------------------------------------------------------
 *  Fonction qui initialise une communication MPI par l'envoi ou la lecture
 *  d'une �ventuelle "cha�ne magique" servant a v�rifier le bon format des
 *  donn�es
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_ouvre
(
       cs_comm_t  *const  comm,
 const char       *const  chaine_magique
)
{

  int ierror, comm_size;

  MPI_Status status;

  char * chaine_magique_comm;

  cs_int_t lng_chaine_magique = strlen(chaine_magique);

  /*-------------------------------------*/
  /* Initialisation de l'instrumentation */
  /*-------------------------------------*/

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
  {
    int rang;

    bft_printf(_("Instrumentation de la communication MPI via MPE\n"));
    bft_printf_flush();

    cs_glob_mpe_comm_ouvre = MPE_Log_get_event_number();
    cs_glob_mpe_comm_entete = MPE_Log_get_event_number();
    cs_glob_mpe_comm_corps = MPE_Log_get_event_number();

    if (cs_glob_base_mpi_comm != MPI_COMM_NULL)
      MPI_Comm_rank(cs_glob_base_mpi_comm, &rang);
    else
      MPI_Comm_rank(MPI_COMM_WORLD, &rang);

    if (rang == 0) {
      MPE_Describe_event(cs_glob_mpe_comm_ouvre,
                         "cs_loc_comm_mpi_ouvre", "white");
      MPE_Describe_event(cs_glob_mpe_comm_entete,
                         "cs_loc_com_mpi_entete", "blue");
      MPE_Describe_event(cs_glob_mpe_comm_corps,
                         "cs_loc_comm_mpi_corps", "orange");
    }
  }
#endif

  /*------------------------------------*/
  /* Initialisation de la communication */
  /*------------------------------------*/

  /* Instructions */

  assert(   comm->mode == CS_COMM_MODE_RECEPTION
         || comm->mode == CS_COMM_MODE_EMISSION);

  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  if (comm->rang_proc >= comm_size)

    bft_error(__FILE__, __LINE__, 0,
              _("Impossible d'�tablir la communication : %s\n"
                "car le rang du processus recherch� (%d)\n"
                "est sup�rieur ou �gal au nombre de processus MPI (%d)."),
              comm->nom, comm->rang_proc, comm_size);


  BFT_MALLOC(chaine_magique_comm, lng_chaine_magique + 1, char);

  /*-----------------------------------------------------*/
  /* �criture ou lecture eventuelle d'une chaine magique */
  /*-----------------------------------------------------*/

  if (comm->mode == CS_COMM_MODE_RECEPTION) {

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_rcv_a, 0, NULL);
#endif

    ierror = MPI_Recv(chaine_magique_comm, lng_chaine_magique, MPI_CHAR,
                      comm->rang_proc,
                      MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_rcv_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
    MPE_Log_event(cs_glob_mpe_comm_ouvre, 0, NULL);
#endif

    if (ierror != MPI_SUCCESS)
      cs_loc_comm_mpi_msg_err(comm, ierror);

    chaine_magique_comm[lng_chaine_magique] = '\0';

    /* Si la chaine magique ne correspond pas, on a une erreur */

    if (strcmp(chaine_magique_comm, chaine_magique) != 0) {

      bft_error(__FILE__, __LINE__, 0,
                _("Erreur pour la communication : \"%s\".\n"
                  "La cha�ne magique indique une mauvaise version du "
                  "format de l'interface.\n"
                  "cha�ne magique lue      : \"%s\"\n"
                  "cha�ne magique attendue : \"%s\""),
                comm->nom, chaine_magique_comm, chaine_magique);

    }

  }
  else if (comm->mode == CS_COMM_MODE_EMISSION) {

    strncpy(chaine_magique_comm, chaine_magique, lng_chaine_magique);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_send_a, 0, NULL);
#endif

    ierror = MPI_Send(chaine_magique_comm, lng_chaine_magique, MPI_CHAR,
                      comm->rang_proc,
                      0, MPI_COMM_WORLD);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_send_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
    MPE_Log_event(cs_glob_mpe_comm_ouvre, 0, NULL);
#endif

    if (ierror != MPI_SUCCESS)
      cs_loc_comm_mpi_msg_err(comm, ierror);

  }

  BFT_FREE(chaine_magique_comm);

}


/*----------------------------------------------------------------------------
 *  Fonction qui �change une ent�te de rubrique via MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_entete
(
       cs_int_t   *const num_rub,
       char       *const nom_rub,
       cs_int_t   *const nbr_elt_rub,
       char       *const nom_typ_elt,
 const cs_comm_t  *const comm
)
{

#undef  CS_COMM_MPI_PACK_SIZE
#define CS_COMM_MPI_PACK_SIZE          CS_COMM_LNG_NOM_RUB \
                                      + CS_COMM_LNG_NOM_TYPE_ELT \
                                      + (sizeof(int) * 2)

  char buffer[CS_COMM_MPI_PACK_SIZE];

  int position, ierror;

  MPI_Status  status;


  /* Instructions */

  assert(comm != NULL);
  assert(*nbr_elt_rub >= 0);
  assert(sizeof(int) == sizeof(cs_int_t));


  /* Communication en r�ception */
  /*----------------------------*/

  if (comm->mode == CS_COMM_MODE_RECEPTION) {

    /* R�ception du message */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_rcv_a, 0, NULL);
#endif

    ierror = MPI_Recv(buffer, CS_COMM_MPI_PACK_SIZE, MPI_PACKED,
                      comm->rang_proc,
                      MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_rcv_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
    MPE_Log_event(cs_glob_mpe_comm_entete, 0, NULL);
#endif

    if (ierror != MPI_SUCCESS)
      cs_loc_comm_mpi_msg_err(comm, ierror);


    /* Extraction des �l�ments du tampon */

    position = 0;
    MPI_Unpack(buffer, CS_COMM_MPI_PACK_SIZE, &position, num_rub,
               1, CS_MPI_INT, MPI_COMM_WORLD);

    if (*num_rub == 0)
      MPI_Unpack(buffer, CS_COMM_MPI_PACK_SIZE, &position, nom_rub,
                 CS_COMM_LNG_NOM_RUB, MPI_CHAR, MPI_COMM_WORLD);

    MPI_Unpack(buffer, CS_COMM_MPI_PACK_SIZE, &position, nbr_elt_rub,
               1, CS_MPI_INT, MPI_COMM_WORLD);

    if (*nbr_elt_rub > 0)
      MPI_Unpack(buffer, CS_COMM_MPI_PACK_SIZE, &position, nom_typ_elt,
                 CS_COMM_LNG_NOM_TYPE_ELT, MPI_CHAR, MPI_COMM_WORLD);

  }


  /* Communication en �mission */
  /*---------------------------*/

  else if (comm->mode == CS_COMM_MODE_EMISSION) {

    /* Assemblage du tampon */

    position = 0;
    MPI_Pack(num_rub, 1, CS_MPI_INT, buffer, CS_COMM_MPI_PACK_SIZE,
             &position, MPI_COMM_WORLD);

    if (*num_rub == 0)
      MPI_Pack(nom_rub, CS_COMM_LNG_NOM_RUB, MPI_CHAR, buffer,
               CS_COMM_MPI_PACK_SIZE, &position, MPI_COMM_WORLD);

    MPI_Pack(nbr_elt_rub, 1, CS_MPI_INT, buffer, CS_COMM_MPI_PACK_SIZE,
             &position, MPI_COMM_WORLD);

    if (*nbr_elt_rub > 0)
      MPI_Pack(nom_typ_elt, CS_COMM_LNG_NOM_TYPE_ELT, MPI_CHAR, buffer,
               CS_COMM_MPI_PACK_SIZE, &position, MPI_COMM_WORLD);

    /* Envoi du message */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_send_a, 0, NULL);
#endif

    ierror = MPI_Send(buffer, position, MPI_PACKED, comm->rang_proc,
                      0, MPI_COMM_WORLD);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
    MPE_Log_event(cs_glob_mpe_send_b, 0, NULL);
    MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
    MPE_Log_event(cs_glob_mpe_comm_entete, 0, NULL);
#endif

    if (ierror != MPI_SUCCESS)
      cs_loc_comm_mpi_msg_err(comm, ierror);

  }

  else

    assert(   comm->mode == CS_COMM_MODE_RECEPTION
           || comm->mode == CS_COMM_MODE_EMISSION);

}


/*----------------------------------------------------------------------------
 *  Fonction qui �change le corps d'une rubrique via MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_corps
(
       void        *const elt_rub,
 const cs_int_t           nbr_elt_rub,
       cs_type_t          typ_elt,
 const cs_comm_t   *const comm
)
{

  int ierror;
  int nbr_elt = nbr_elt_rub;

  MPI_Status  status;


  /* Instructions */

  assert(comm != NULL);
  assert(nbr_elt_rub >= 0);


  /* Communication en r�ception */
  /*----------------------------*/

  if (comm->mode == CS_COMM_MODE_RECEPTION) {


    switch (typ_elt) {

    case CS_TYPE_cs_int_t:             /* Tableau d'entiers */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_rcv_a, 0, NULL);
#endif

      ierror = MPI_Recv(elt_rub, nbr_elt, CS_MPI_INT,
                        comm->rang_proc,
                        MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_rcv_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif
      break;

    case CS_TYPE_cs_real_t:            /* Tableau de r�els double pr�cision */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_rcv_a, 0, NULL);
#endif

      ierror = MPI_Recv(elt_rub, nbr_elt, CS_MPI_REAL,
                        comm->rang_proc,
                        MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_rcv_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif

      break;

    case CS_TYPE_char:                 /* Tableau de caract�res */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_rcv_a, 0, NULL);
#endif

      ierror = MPI_Recv(elt_rub, nbr_elt, MPI_CHAR,
                        comm->rang_proc,
                        MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_rcv_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif

      break;

    default:

      assert (   typ_elt == CS_TYPE_char
              || typ_elt == CS_TYPE_cs_int_t
              || typ_elt == CS_TYPE_cs_real_t);

    }

  }


  /* Communication en �mission */
  /*---------------------------*/

  else if (comm->mode == CS_COMM_MODE_EMISSION) {


    switch (typ_elt) {

    case CS_TYPE_cs_int_t:             /* Tableau d'entiers */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_send_a, 0, NULL);
#endif

      ierror = MPI_Send(elt_rub, nbr_elt, CS_MPI_INT,
                        comm->rang_proc,
                        0, MPI_COMM_WORLD);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_send_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif

      break;

    case CS_TYPE_cs_real_t:            /* Tableau de r�els double pr�cision */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_send_a, 0, NULL);
#endif

      ierror = MPI_Send(elt_rub, nbr_elt, CS_MPI_REAL,
                        comm->rang_proc,
                        0, MPI_COMM_WORLD);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_send_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif

      break;

    case CS_TYPE_char:                 /* Tableau de caract�res */

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_compute_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_send_a, 0, NULL);
#endif

      ierror = MPI_Send(elt_rub, nbr_elt, MPI_CHAR,
                        comm->rang_proc,
                        0, MPI_COMM_WORLD);

#if defined(_CS_HAVE_MPE) && defined(_CS_COMM_PROFILING)
      MPE_Log_event(cs_glob_mpe_send_b, 0, NULL);
      MPE_Log_event(cs_glob_mpe_compute_a, 0, NULL);
      MPE_Log_event(cs_glob_mpe_comm_corps, 0, NULL);
#endif

      break;

    default:

      assert(   typ_elt == CS_TYPE_char
             || typ_elt == CS_TYPE_cs_int_t
             || typ_elt == CS_TYPE_cs_real_t);

    }


  }

  else

    assert(   comm->mode == CS_COMM_MODE_RECEPTION
           || comm->mode == CS_COMM_MODE_EMISSION);


  if (ierror != MPI_SUCCESS)
    cs_loc_comm_mpi_msg_err(comm, ierror);

}


/*----------------------------------------------------------------------------
 *  Fonction qui imprime un message d'erreur en cas de probl�me de
 *  communication MPI
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_mpi_msg_err
(
 const cs_comm_t  *const comm,
 const int               error
)
{

  char buffer[MPI_MAX_ERROR_STRING];
  int  buffer_len;

  MPI_Error_string(error, buffer, &buffer_len);

  bft_error(__FILE__, __LINE__, 0,
            _("Erreur MPI pour la communication :  %s\n"
              "Type d'erreur : %s"), comm->nom, buffer);

}


#endif /* (_CS_HAVE_MPI) */


#if defined(_CS_HAVE_SOCKET)

/*----------------------------------------------------------------------------
 *  Fonction qui initialise une connection par "socket"
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_connect
(
 cs_comm_t         *comm
)
{
  int ind;

#if defined(_CS_ARCH_Linux)
  socklen_t long_sock;
#else
  int       long_sock;  /* size_t d'apres standard SUS-v2, mais d'apres
                           man gethostbyname sous Linux, le standard est
                           mauvais, on doit avoir un int (ou socklen_t) */
#endif

  char   str_taille[6] = "     ";
  char  *host_names = NULL;
  int   *tab_num_port = NULL;

#if defined (_CS_HAVE_MPI)
  int ierror = MPI_SUCCESS;
#endif
  int rang = (cs_glob_base_rang == -1 ? 0 : cs_glob_base_rang);

  const int lng_hostname = CS_LOC_COMM_LNG_HOSTNAME + 1;


  /* Connexion au socket "serveur" */

  long_sock = sizeof(cs_glob_comm_addr_sock);

  if (rang == 0)
    comm->sock = accept(cs_glob_comm_socket,
                        (struct sockaddr *)&cs_glob_comm_addr_sock,
                        &long_sock);

  /* R�cup�re le nom de la machine h�te et de son num�ro de port sur
     le rang 0 */

  if (cs_glob_base_nbr > 1) {

    BFT_MALLOC(host_names,
               lng_hostname * cs_glob_base_nbr,
               char);

    BFT_MALLOC(tab_num_port, cs_glob_base_nbr, int);

#if defined(_CS_HAVE_MPI)
    ierror = MPI_Gather(cs_glob_comm_sock_nom_hote, lng_hostname, MPI_CHAR,
                        host_names, lng_hostname, MPI_CHAR, 0,
                        cs_glob_base_mpi_comm);

    if (ierror < 0)
      bft_error(__FILE__, __LINE__, 0,
                _("Erreur lors de l'envoi via MPI du nom de l'h�te "
                  "en initialisant les sockets.\n"));

    /* Envoie du num�ro de port */

    ierror = MPI_Gather(&cs_glob_comm_sock_num_port, 1, MPI_INT,
                        tab_num_port, 1, MPI_INT, 0, cs_glob_base_mpi_comm);

    if (ierror < 0)
      bft_error(__FILE__, __LINE__, 0,
                _("Erreur lors de l'envoi via MPI du num�ro du port "
                  "en initialisant les sockets.\n"));

    if (rang != 0)
      comm->sock = accept(cs_glob_comm_socket,
                          (struct sockaddr *)&cs_glob_comm_addr_sock,
                          &long_sock);

#else
    bft_error(__FILE__, __LINE__, 0,
              _("Besoin de MPI lors de l'initialisation des sockets.\n"));
#endif

    /* envoie depuis le rang 0 par les sockets des noms des machines
       h�te et des num�ros de port */

    if (rang == 0) {

      /* Envoi de la taille max. du nom de l'h�te */

      sprintf(str_taille, "%3d", lng_hostname);

      if (write(comm->sock, str_taille, 4) < 4)
        bft_error(__FILE__, __LINE__, errno,
                  _("Erreur de communication par socket\n"));

      for (ind = 1; ind < cs_glob_base_nbr; ind++) {

        /* Envoi du nom de la machine h�te */

        if (write(comm->sock, &(host_names[lng_hostname*ind]), lng_hostname)
            < lng_hostname)
          bft_error(__FILE__, __LINE__, errno,
                    _("Erreur de communication par socket\n"));

        /* Envoi du num�ro de port */

        sprintf(str_taille, "%5d", tab_num_port[ind]);

        if (write(comm->sock, str_taille, 6) < 6)
          bft_error(__FILE__, __LINE__, errno,
                    _("Erreur de communication par socket\n"));

      }

    } /* Fin de si rang == 0 */

    BFT_FREE(host_names);
    BFT_FREE(tab_num_port);

  } /* Fin de si cs_glob_base_nbr > 1 */

}


/*----------------------------------------------------------------------------
 *  Fonction qui assure l'�change de la "chaine magique" via les sockets
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_ouvre
(
 cs_comm_t   *const  comm,
 const char  *const  nom_fic,
 const char  *const  chaine_magique
)
{
  char nom_tmp[CS_LOC_COMM_LNG_NOM_MAX + 1];

  int taille;

  int rang = (cs_glob_base_rang == -1 ? 0 : cs_glob_base_rang);

  taille = strlen(CS_COMM_SOCKET_ENTETE);

  if (read(comm->sock, nom_tmp, taille) < taille)
    bft_error(__FILE__, __LINE__, errno,
              _(cs_glob_comm_err_socket), comm->nom,
              rang + 1);

  /* V�rification que la connexion provient du bon type d'application */

  if (strncmp(nom_tmp, CS_COMM_SOCKET_ENTETE, taille != 0))
    bft_error(__FILE__, __LINE__, 0,
              _("Tentative de connexion au port de communication avec\n"
                "un format de message non reconnu\n"));

  /* Taille du nom du fichier en communication */

  if (read(comm->sock, nom_tmp, 4) < 4)
    bft_error(__FILE__, __LINE__, errno,
              _(cs_glob_comm_err_socket), comm->nom, rang + 1);

  nom_tmp[4] = '\0';
  taille = atoi(nom_tmp);

  if (taille <= CS_LOC_COMM_LNG_NOM_MAX) {

    /* Nom du fichier en communication */

    if (read(comm->sock, nom_tmp, taille) < taille)
      bft_error(__FILE__, __LINE__, errno,
                _(cs_glob_comm_err_socket), comm->nom, rang + 1);

    nom_tmp[taille] = '\0';

    /* Le nom correspond-il � celui attendu ? */

    if (strcmp(nom_tmp, nom_fic) != 0)
      bft_error(__FILE__, __LINE__, 0,
                _("Nom du fichier de communication incoh�rent.\n"
                  "Nom re�u: \"%s\"\n"
                  "Nom attendu: \"%s\"\n"),
                nom_tmp, nom_fic);

  }
  else
    bft_error(__FILE__, __LINE__, 0,
              _("La longueur du nom du fichier de communication est "
                "trop importante\n"));

  /*-----------------------------------------------------*/
  /* �criture ou lecture �ventuelle d'une chaine magique */
  /*-----------------------------------------------------*/

  if (comm->mode == CS_COMM_MODE_RECEPTION) {

    char      *chaine_magique_lue;
    cs_int_t   lng_chaine_magique = strlen(chaine_magique);

    BFT_MALLOC(chaine_magique_lue, lng_chaine_magique + 1, char);

    cs_loc_comm_lit_sock(comm,
                         (void *)(chaine_magique_lue),
                         strlen(chaine_magique),
                         CS_TYPE_char);

    chaine_magique_lue[lng_chaine_magique] = '\0';

    /* Si la chaine magique ne correspond pas, on a une erreur */

    if (strcmp(chaine_magique_lue, chaine_magique) != 0) {

      bft_error(__FILE__, __LINE__, 0,
                _("Erreur � l'initialisation de la communication : "
                  "\"%s\".\n"
                  "Le format de l'interface n'est pas � la bonne version.\n"
                  "La cha�ne magique rep�re la version du format "
                  "d'interface :\n"
                  "cha�ne magique lue      : \"%s\"\n"
                  "cha�ne magique actuelle : \"%s\"\n"),
                comm->nom, chaine_magique_lue, chaine_magique);

    }

    BFT_FREE(chaine_magique_lue);

  }
  else if (comm->mode == CS_COMM_MODE_EMISSION) {

    cs_loc_comm_ecrit_sock(comm,
                           (const void *)(chaine_magique),
                           strlen(chaine_magique),
                           CS_TYPE_char);

  }

}


/*----------------------------------------------------------------------------
 *  Fonction qui ferme la connextion avec le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_sock_ferme
(
 cs_comm_t  *comm
)
{
  if (close(comm->sock) != 0)
    bft_error(__FILE__, __LINE__, errno,
              _("Communication %s) :\n"
                "Erreur � la fermeture du socket.\n"),
              comm->nom);

  comm->sock = -1;
}


/*----------------------------------------------------------------------------
 *  Fonction qui �crit un enregistrement dans le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_ecrit_sock
(
 const cs_comm_t  *const comm,
 const cs_byte_t  *      rec,
 const size_t            nbr,
       cs_type_t         type
)
{
  size_t   ind_deb;
  size_t   ind_fin;
  size_t   nbr_loc;
  size_t   nbr_octet;
  size_t   taille;
  ssize_t  ret;

  cs_byte_t   * rec_tmp;

  assert(rec  != NULL);
  assert(comm != NULL);

  /* D�termination du nombre d'octets  � envoyer */

  switch(type) {
  case CS_TYPE_cs_int_t:
    taille = sizeof(cs_int_t);
    break;
  case CS_TYPE_cs_real_t:
    taille = sizeof(cs_real_t);
    break;
  case CS_TYPE_char:
    taille = sizeof(char);
    break;
  default:
    assert(type == CS_TYPE_cs_int_t  ||
           type == CS_TYPE_cs_real_t ||
           type == CS_TYPE_char);
  } /* Fin `switch (type)' */

  nbr_octet = taille * nbr;

  /* Conversion si "little-endian" */

  if (comm->swap_endian == CS_TRUE && taille != 1) {
    BFT_MALLOC(rec_tmp, nbr_octet, cs_byte_t);
    bft_file_swap_endian(rec_tmp, rec, taille, nbr);
  }
  else
    rec_tmp = NULL;

  /* �criture de l'enregistrement dans le socket */
  /*---------------------------------------------*/

  ind_deb = 0;

  while (ind_deb < nbr_octet) {

    ind_fin = CS_MIN(ind_deb + SSIZE_MAX, nbr_octet);

    nbr_loc = ind_fin - ind_deb;

    if (rec_tmp == NULL)
      ret = write(comm->sock, (const void *)(rec + ind_deb), nbr_loc);
    else
      ret = write(comm->sock, (const void *)(rec_tmp + ind_deb), nbr_loc);

    if (ret < 1)
      bft_error(__FILE__, __LINE__, errno,
                _("Communication %s :\n"
                  "Erreur d'envoi de donn�es par socket.\n"),
                comm->nom);

    ind_deb += ret;

  }

  if (rec_tmp != NULL)
    BFT_FREE(rec_tmp);

}


/*----------------------------------------------------------------------------
 *  Fonction qui lit un enregistrement dans le socket d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_lit_sock
(
 const cs_comm_t  *const comm,
       cs_byte_t  *      rec,
 const size_t            nbr,
       cs_type_t         type
)
{
  size_t   ind_deb;
  size_t   ind_fin;
  size_t   nbr_loc;
  size_t   nbr_octet;
  size_t   taille;
  ssize_t  ret;


  assert(rec  != NULL);
  assert(comm != NULL);

  /* D�termination du nombre d'octets  � recevoir */

  switch(type) {
  case CS_TYPE_cs_int_t:
    taille = sizeof(cs_int_t);
    break;
  case CS_TYPE_cs_real_t:
    taille = sizeof(cs_real_t);
    break;
  case CS_TYPE_char:
    taille = sizeof(char);
    break;
  default:
    assert(type == CS_TYPE_cs_int_t  ||
           type == CS_TYPE_cs_real_t ||
           type == CS_TYPE_char);
  } /* Fin `switch (type)' */

  nbr_octet = taille * nbr;


  /* Lecture de l'enregistrement dans le socket */
  /*--------------------------------------------*/

  ind_deb = 0;

  while (ind_deb < nbr_octet) {

    ind_fin = CS_MIN(ind_deb + SSIZE_MAX, nbr_octet);

    nbr_loc = ind_fin - ind_deb;

    ret = read(comm->sock, (void *)(rec + ind_deb), nbr_loc);

    if (ret < 1)
      bft_error(__FILE__, __LINE__, errno,
                _("Communication %s :\n"
                  "Erreur de r�ception de donn�es par socket.\n"),
                comm->nom);

    ind_deb += ret;

  }

  if (comm->swap_endian == CS_TRUE)
    bft_file_swap_endian(rec, rec, taille, nbr);

}

#endif /* (_CS_HAVE_SOCKET) */


/*----------------------------------------------------------------------------
 *  Affichage de l'attente d'�change d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_pre
(
 const cs_comm_t  *const comm
)
{
  assert(comm != NULL);

  switch(comm->mode) {

  case CS_COMM_MODE_RECEPTION:
    bft_printf(_("\nMessage re�u sur \"%s\" :\n"), comm->nom);
    break;

  case CS_COMM_MODE_EMISSION:
    bft_printf(_("\nMessage envoy� sur \"%s\" :\n"), comm->nom);
    break;

  default:
    assert(   comm->mode == CS_COMM_MODE_RECEPTION
           || comm->mode == CS_COMM_MODE_EMISSION);
  }

  bft_printf_flush();

}


/*----------------------------------------------------------------------------
 *  Affichage de l'entete d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_entete
(
 const cs_int_t           num_rub,
 const char        *const nom_rub,
 const cs_int_t           nbr_elt,
 const cs_type_t          typ_elt
)
{

  char nom_rub_ecr[CS_COMM_LNG_NOM_RUB + 1];

  /* instructions */

  strncpy(nom_rub_ecr, nom_rub,  CS_COMM_LNG_NOM_RUB);
  nom_rub_ecr[CS_COMM_LNG_NOM_RUB] = '\0';

  bft_printf(_("    num�ro de rubrique    : %d\n"
               "    nom de la rubrique    : \"%s\"\n"
               "    nombre d'�l�ments     : %d\n"),
             num_rub, nom_rub_ecr, nbr_elt);

  if (nbr_elt > 0) {

    char *nom_typ;

    switch(typ_elt) {
    case CS_TYPE_char:
      nom_typ = cs_comm_nom_typ_elt_char;
      break;
    case CS_TYPE_cs_int_t:
      nom_typ = cs_comm_nom_typ_elt_int;
      break;
    case CS_TYPE_cs_real_t:
      nom_typ = cs_comm_nom_typ_elt_real;
      break;
    default:
      assert(   typ_elt == CS_TYPE_char
             || typ_elt == CS_TYPE_cs_int_t
             || typ_elt == CS_TYPE_cs_real_t);
    }

    bft_printf(_("    nom du type d'�l�ment : \"%s\"\n"), nom_typ);

  }

  bft_printf_flush();

}


/*----------------------------------------------------------------------------
 *  Affichage (partiel) du contenu d'un message
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_echo_donnees
(
 const cs_int_t          echo,
 const cs_int_t          nbr_elt,
 const cs_type_t         typ_elt,
 const void       *const elt_rub
)
{
  cs_int_t  echo_deb = 0;
  cs_int_t  echo_fin;
  cs_int_t  ind;

  /* Instructions */

  if (nbr_elt == 0) return;

  if (echo * 2 < nbr_elt) {
    echo_fin = echo;
    bft_printf(_("    %d premiers et derniers �l�ments :\n"), echo);
  }
  else {
    echo_fin = nbr_elt;
    bft_printf(_("    �l�ments :\n"));
  }

  do {

    switch (typ_elt) {

    case CS_TYPE_cs_int_t:
      {
        const cs_int_t *elt_rub_int = (const cs_int_t *) elt_rub;

        for (ind = echo_deb ; ind < echo_fin ; ind++)
          bft_printf("    %10d : %12d\n", ind + 1, *(elt_rub_int + ind));
      }
      break;

    case CS_TYPE_cs_real_t:
      {
        const cs_real_t *elt_rub_real = (const cs_real_t *) elt_rub;

        for (ind = echo_deb ; ind < echo_fin ; ind++)
          bft_printf("    %10d : %12.5e\n", ind + 1, *(elt_rub_real + ind));
      }
      break;

    case CS_TYPE_char:
      {
        const char *elt_rub_char = (const char *) elt_rub;

        for (ind = echo_deb ; ind < echo_fin ; ind++) {
          if (*(elt_rub_char + ind) != '\0')
            bft_printf("    %10d : '%c'\n", ind + 1, *(elt_rub_char + ind));
          else
            bft_printf("    %10d : '\\0'\n", ind + 1);
        }
      }
      break;

    default:

      assert(   typ_elt == CS_TYPE_cs_int_t
             || typ_elt == CS_TYPE_cs_real_t
             || typ_elt == CS_TYPE_char);

    }

    if (echo_fin < nbr_elt) {
      bft_printf("    ..........   ............\n");
      echo_deb = nbr_elt - echo;
      echo_fin = nbr_elt;
    }
    else {
      assert(echo_fin == nbr_elt);
      echo_fin = nbr_elt + 1;
    }

  } while (echo_fin <= nbr_elt);

  bft_printf_flush();

}


/*----------------------------------------------------------------------------
 *  Fonction qui �crit un enregistrement dans le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_ecrit_rec
(
 const cs_comm_t   *const comm,
 const void        *const rec,
 const size_t             nbr,
       cs_type_t          typ
)
{
  size_t  taille = 0;

  assert(comm != NULL);
  assert(rec  != NULL);

  switch(comm->type) {

  case CS_COMM_TYPE_BINAIRE:

    /* �criture de l'enregistrement dans le fichier */

    switch(typ) {
    case CS_TYPE_char:
      taille = sizeof(char);
      break;
    case CS_TYPE_cs_int_t:
      taille = sizeof(cs_int_t);
      break;
    case CS_TYPE_cs_real_t:
      taille = sizeof(cs_real_t);
      break;
    default:
      assert(   typ == CS_TYPE_char
             || typ == CS_TYPE_cs_int_t
             || typ == CS_TYPE_cs_real_t);
    }

    bft_file_write(rec, taille, nbr, comm->fic);
    break;

  default:
    assert(comm->type == CS_COMM_TYPE_BINAIRE);

  }

}


/*----------------------------------------------------------------------------
 *  Fonction qui lit un enregistrement dans le fichier d'interface
 *----------------------------------------------------------------------------*/

static void cs_loc_comm_lit_rec
(
 const cs_comm_t  *const comm,
       void       *const rec,
 const size_t            nbr,
       cs_type_t         typ
)
{
  size_t  taille = 0;

  assert(comm != NULL);
  assert(rec  != NULL);


  switch(comm->type) {

  case CS_COMM_TYPE_BINAIRE:

    /* Lecture de l'enregistrement dans le fichier */

    switch (typ) {
    case CS_TYPE_char:
      taille = sizeof(char);
      break;
    case CS_TYPE_cs_int_t:
      taille = sizeof(cs_int_t);
      break;
    case CS_TYPE_cs_real_t:
      taille = sizeof(cs_real_t);
      break;
    default:
      assert(   typ == CS_TYPE_char
             || typ == CS_TYPE_cs_int_t
             || typ == CS_TYPE_cs_real_t);
    }

    bft_file_read(rec, taille, nbr, comm->fic);
    break;

  default:

    assert(comm->type == CS_COMM_TYPE_BINAIRE);

  }

}


#ifdef __cplusplus
}
#endif /* __cplusplus */
