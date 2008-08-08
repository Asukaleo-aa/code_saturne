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
 * Main program
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_config.h>
#include <bft_mem.h>
#include <bft_printf.h>
#include <bft_fp_trap.h>
#include <bft_timer.h>

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_selector.h>

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_benchmark.h"
#include "cs_comm.h"
#include "cs_couplage.h"
#include "cs_mesh.h"
#include "cs_mesh_connect.h"
#include "cs_mesh_quantities.h"
#include "cs_mesh_solcom.h"
#include "cs_mesh_quality.h"
#include "cs_mesh_warping.h"
#include "cs_mesh_coherency.h"
#include "cs_ecs_messages.h"
#include "cs_opts.h"
#include "cs_pp_io.h"
#include "cs_renumber.h"
#include "cs_sles.h"
#include "cs_suite.h"
#include "cs_syr_coupling.h"
#include "cs_post.h"

#if defined(_CS_HAVE_XML)
#include "cs_gui.h"
#include "cs_gui_radiative_transfer.h"
#endif

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*=============================================================================
 * Local Macro definitions
 *============================================================================*/

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * SUBROUTINE CSINIT : sous-programme d'initialisation Fortran listing
 *----------------------------------------------------------------------------*/

extern void CS_PROCF(csinit, CSINIT)
(
 cs_int_t  *ifoenv,    /* Maillage SolCom ou Pr�processeur                    */
 cs_int_t  *iparal,    /* Rang du noyau en cas de parallelisme                */
 cs_int_t  *nparal,    /* Nombre de processus (=1 en sequentiel)              */
 cs_int_t  *ilisr0,    /* Option de sortie du listing principal (rang 0) :    */
                       /*   0 : non redirig� ; 1 : sur fichier 'listing'      */
 cs_int_t  *ilisrp     /* Option de sortie des listings de rang > 0 :         */
                       /*   0 : non redirig� ; 1 : sur fichier 'listing_n*' ; */
                       /*   2 : sur fichier '/dev/null' (suppression)         */
);

/*----------------------------------------------------------------------------
 * SUBROUTINE INITI1 : sous-programme d'initialisation Fortran
 *----------------------------------------------------------------------------*/

extern void CS_PROCF(initi1, INITI1)
(
 cs_int_t  *longia,    /* Longueur du tableau d'entiers IA                    */
 cs_int_t  *longra,    /* Longueur du tableau d'entiers IA                    */
 cs_int_t  *idebia,    /* Premiere position libre dans IA                     */
 cs_int_t  *idebra,    /* Premiere position libre dans RA                     */
 cs_int_t  *iverif     /* Activation des tests �l�mentaires                   */
);

/*----------------------------------------------------------------------------
 * SUBROUTINE CALTRI : sous-programme principal Fortran
 *----------------------------------------------------------------------------*/

extern void CS_PROCF(caltri, CALTRI)
(
 cs_int_t   *longia,   /* Longueur du tableau d'entiers IA                    */
 cs_int_t   *longra,   /* Longueur du tableau d'entiers IA                    */
 cs_int_t   *idebia,   /* Premiere position libre dans IA                     */
 cs_int_t   *idebra,   /* Premiere position libre dans RA                     */
 cs_int_t   *iverif,   /* Activation des tests elementaires                   */
 cs_int_t   *ifacel,   /* �l�ments voisins d'une face interne                 */
 cs_int_t   *ifabor,   /* �l�ment  voisin  d'une face de bord                 */
 cs_int_t   *ifmfbr,   /* Num�ro de famille d'une face de bord                */
 cs_int_t   *ifmcel,   /* Num�ro de famille d'une cellule                     */
 cs_int_t   *iprfml,   /* Propri�t�s d'une famille                            */
 cs_int_t   *ipnfac,   /* Pointeur par sommet dans NODFAC (optionnel)         */
 cs_int_t   *nodfac,   /* Connectivit� faces internes/sommets (optionnelle)   */
 cs_int_t   *ipnfbr,   /* Pointeur par sommet dans NODFBR (optionnel)         */
 cs_int_t   *nodfbr,   /* Connectivit� faces de bord/sommets (optionnelle)    */
 cs_int_t   *ia,       /* Pointeur sur le tableau d'entiers IA                */
 cs_real_t  *xyzcen,   /* Points associ�s aux centres des volumes de contr�le */
 cs_real_t  *surfac,   /* Vecteurs surfaces des faces internes                */
 cs_real_t  *surfbo,   /* Vecteurs surfaces des faces de bord                 */
 cs_real_t  *cdgfac,   /* Centres de gravit� des faces internes               */
 cs_real_t  *cdgfbr,   /* Centres de gravit� des faces de bord                */
 cs_real_t  *xyznod,   /* Coordonn�es des sommets (optionnelle)               */
 cs_real_t  *volume,   /* Volumes des cellules                                */
 cs_real_t  *ra        /* Pointeur sur le tableau de reels RA                 */
);

/*----------------------------------------------------------------------------
 * Fonction utilisateur pour la modification de la g�om�trie
 *----------------------------------------------------------------------------*/

void CS_PROCF (usmodg, USMODG)
(
 const cs_int_t  *ndim,      /* --> dimension de l'espace                     */
 const cs_int_t  *ncelet,    /* --> nombre de cellules �tendu                 */
 const cs_int_t  *ncel,      /* --> nombre de cellules                        */
 const cs_int_t  *nfac,      /* --> nombre de faces internes                  */
 const cs_int_t  *nfabor,    /* --> nombre de faces de bord                   */
 const cs_int_t  *nfml,      /* --> nombre de familles                        */
 const cs_int_t  *nprfml,    /* --> nombre de proprietes des familles         */
 const cs_int_t  *nnod,      /* --> nombre de sommets                         */
 const cs_int_t  *lndfac,    /* --> longueur de nodfac                        */
 const cs_int_t  *lndfbr,    /* --> longueur de nodfbr                        */
 const cs_int_t   ifacel[],  /* --> connectivit� faces internes / cellules    */
 const cs_int_t   ifabor[],  /* --> connectivit� faces de bord / cellules     */
 const cs_int_t   ifmfbr[],  /* --> liste des familles des faces de bord      */
 const cs_int_t   ifmcel[],  /* --> liste des familles des cellules           */
 const cs_int_t   iprfml[],  /* --> liste des propri�t�s des familles         */
 const cs_int_t   ipnfac[],  /* --> rang dans nodfac 1er sommet faces int.    */
 const cs_int_t   nodfac[],  /* --> num�ro des sommets des faces int�rieures  */
 const cs_int_t   ipnfbr[],  /* --> rang dans nodfbr 1er sommet faces bord    */
 const cs_int_t   nodfbr[],  /* --> num�ro des sommets des faces de bord      */
       cs_real_t  xyznod[]   /* --> coordonn�es des sommets                   */
);

/*----------------------------------------------------------------------------
 * Update dimension of the mesh for FORTRAN common.
 *
 * Interface Fortran :
 *
 * SUBROUTINE MAJGEO (NCELET, NFAC, NFABOR, NFACGB, NFBRGB)
 * *****************
 *
 * INTEGER          NCELET      : --> : New value to assign
 *----------------------------------------------------------------------------*/

extern void CS_PROCF (majgeo, MAJGEO)
(
 const cs_int_t   *const ncelet,  /* --> New number of halo cells             */
 const cs_int_t   *const nfac,    /* --> New number of internal faces         */
 const cs_int_t   *const nfabor,  /* --> New number of border faces           */
 const cs_int_t   *const nfacgb,  /* --> New number of global internal faces  */
 const cs_int_t   *const nfbrgb   /* --> New number of global border faces    */
);

/*============================================================================
 * Prototypes de fonctions priv�es
 *============================================================================*/

/*============================================================================
 * Programme principal
 *============================================================================*/

int main
(
 int    argc,       /* Nombre d'arguments dans la ligne de commandes */
 char  *argv[]      /* Tableau des arguments de la ligne de commandes */
)
{
  cs_int_t  n_g_i_faces, n_g_b_faces;
  double  t1, t2;
  cs_int_t  idebia, idebra;
  cs_opts_t  opts;

  int  rang_deb = -1;
  int  _verif = -1;
  cs_int_t  *ia = NULL;
  cs_real_t  *ra = NULL;

  /* Premi�re analyse de la ligne de commande pour savoir si l'on a besoin
     de MPI ou non, et initialisation de MPI le cas �ch�ant */

#if defined(_CS_HAVE_MPI)
  rang_deb = cs_opts_mpi_rank(&argc, &argv);
  if (rang_deb > -1)
    cs_base_mpi_init(&argc, &argv, rang_deb);
#endif

  /* initialisation par d�faut */

#if defined(_CS_ARCH_Linux)

  if (getenv("LANG") != NULL)
    setlocale(LC_ALL,"");
  else
    setlocale(LC_ALL, "C");
  setlocale(LC_NUMERIC, "C");

#endif

#if defined(ENABLE_NLS)
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  (void)bft_timer_wtime();

  bft_fp_trap_set();

  /* Initialisation de la gestion m�moire et des signaux */

  cs_base_mem_init();
  cs_base_erreur_init();

  /* interpr�tation des arguments de la ligne de commande */

  cs_opts_define(argc, argv, &opts);

  /* Ouverture des fichiers 'listing' pour les noeuds de rang > 0 */

  CS_PROCF(csinit, CSINIT)(&(opts.ifoenv),
                           &cs_glob_base_rang,
                           &cs_glob_base_nbr,
                           &(opts.ilisr0),
                           &(opts.ilisrp));
  cs_base_bft_printf_set();

  /* Ent�te et rappel des options de la ligne de commande */

  cs_opts_logfile_head(argc, argv);

  /* Infos syst�me */

  cs_base_info_systeme();

  /* Initialisation des structures globales li�es au maillage principal */

  cs_glob_mesh = cs_mesh_create();
  cs_glob_mesh_builder = cs_mesh_builder_create();
  cs_glob_mesh_quantities = cs_mesh_quantities_create();

  /* Initialisation de la lecture des donn�es Pr�processeur */

  if (opts.ifoenv != 0) {

    cs_glob_pp_io = cs_pp_io_initialize("preprocessor_output",
                                        "ECS_1.3",
                                        CS_PP_IO_MODE_READ,
                                        opts.echo_comm);

    /* Initialisation des communications avec Syrthes */

    if (cs_syr_coupling_n_couplings() != 0) {

      cs_int_t coupl_id;
      cs_int_t n_coupl = cs_syr_coupling_n_couplings();

      for (coupl_id = 0; coupl_id < n_coupl; coupl_id++)
        cs_syr_coupling_init_comm(cs_syr_coupling_by_id(coupl_id),
                                  coupl_id + 1,
                                  opts.echo_comm);

    } /* Couplage Syrthes */

  } /* Si ifoenv != 0 */

  /* Allocation de structures internes de l'API F77 pour fichiers suite */

  cs_suite_f77_api_init();

  /* Appel du sous-programme d'initalisation ou de l'aide */

  _verif = opts.iverif;
  if (opts.benchmark > 0 && _verif < 0)
    _verif = 0;

  CS_PROCF(initi1, INITI1)(&(opts.longia),
                           &(opts.longra),
                           &idebia,
                           &idebra,
                           &_verif);

  if (opts.ifoenv == 0) {

    /* Lecture du fichier au format "SolCom" */

    cs_maillage_solcom_lit(cs_glob_mesh,
                           cs_glob_mesh_quantities);

  }
  else {

    /* Lecture des donn�es issues du Pr�processeur */

    cs_ecs_messages_read_data(cs_glob_mesh);

  } /* End if ifoenv != 0 */

  /* Initialisation du post-traitement principal */

  cs_post_init_pcp();

  /* Initialisation li�es � la construction des halos */

  cs_mesh_init_halo(cs_glob_mesh);

  /* Initialisations li�es au parall�lisme */

  cs_mesh_init_parall(cs_glob_mesh);

  /* Renum�rotation en fonction des options du code */

  bft_printf(_("\n Renumerotation du maillage:\n"));
  bft_printf_flush();
  cs_renumber_mesh(cs_glob_mesh,
                   cs_glob_mesh_quantities);

  /* Modification �ventuelle de la g�om�trie */

  CS_PROCF (usmodg, USMODG)(&(cs_glob_mesh->dim),
                            &(cs_glob_mesh->n_cells_with_ghosts),
                            &(cs_glob_mesh->n_cells),
                            &(cs_glob_mesh->n_i_faces),
                            &(cs_glob_mesh->n_b_faces),
                            &(cs_glob_mesh->n_families),
                            &(cs_glob_mesh->n_max_family_items),
                            &(cs_glob_mesh->n_vertices),
                            &(cs_glob_mesh->i_face_vtx_connect_size),
                            &(cs_glob_mesh->b_face_vtx_connect_size),
                            cs_glob_mesh->i_face_cells,
                            cs_glob_mesh->b_face_cells,
                            cs_glob_mesh->b_face_family,
                            cs_glob_mesh->cell_family,
                            cs_glob_mesh->family_item,
                            cs_glob_mesh->i_face_vtx_idx,
                            cs_glob_mesh->i_face_vtx_lst,
                            cs_glob_mesh->b_face_vtx_idx,
                            cs_glob_mesh->b_face_vtx_lst,
                            cs_glob_mesh->vtx_coord);

  /* D�coupage des faces "gauche" si n�cessaire */

  if (opts.cwf == CS_TRUE) {

    t1 = bft_timer_wtime();
    cs_mesh_warping_cut_faces(cs_glob_mesh, opts.cwf_criterion, opts.cwf_post);
    t2 = bft_timer_wtime();

    bft_printf(_("\n D�coupage des faces gauches (%.3g s)\n"), t2-t1);

  }

  /* Mise � jour de certaines dimensions du maillage */

  n_g_i_faces = (cs_int_t)cs_glob_mesh->n_g_i_faces;
  n_g_b_faces = (cs_int_t)cs_glob_mesh->n_g_b_faces;

  CS_PROCF (majgeo, MAJGEO)(&(cs_glob_mesh->n_cells_with_ghosts),
                            &(cs_glob_mesh->n_i_faces),
                            &(cs_glob_mesh->n_b_faces),
                            &n_g_i_faces,
                            &n_g_b_faces);

  /* Destruction du la structure temporaire servant � la construction du
     maillage principal */

  cs_glob_mesh_builder = cs_mesh_builder_destroy(cs_glob_mesh_builder);

  /* Calcul des grandeurs g�om�triques associ�es au maillage */

  bft_printf_flush();

  t1 = bft_timer_wtime();
  cs_mesh_quantities_compute(cs_glob_mesh, cs_glob_mesh_quantities);
  t2 = bft_timer_wtime();

  bft_printf(_("\n Calcul des grandeurs g�om�triques (%.3g s)\n"), t2-t1);

  cs_mesh_info(cs_glob_mesh);

/* Initialisation de la partie selector de la structure maillage */
  cs_mesh_init_selectors();

#if 0
  /* For debugging purposes */
  cs_mesh_dump(cs_glob_mesh);
  cs_mesh_quantities_dump(cs_glob_mesh, cs_glob_mesh_quantities);
#endif

  /* Boucle en temps ou crit�res de qualit� selon options de v�rification */

  if (opts.iverif == 0) {
    bft_printf(_("\n Calcul des crit�res de qualit�\n"));
    cs_mesh_quality(cs_glob_mesh, cs_glob_mesh_quantities);
  }

  if (opts.iverif >= 0)
    cs_mesh_coherency_check();

  if (opts.benchmark > 0) {
    int mpi_trace_mode = (opts.benchmark == 2) ? 1 : 0;
    cs_benchmark(mpi_trace_mode);
  }

  if (opts.iverif != 0 && opts.benchmark <= 0) {

    /* Allocation des tableaux de travail */
    BFT_MALLOC(ia, opts.longia, cs_int_t);
    BFT_MALLOC(ra, opts.longra, cs_real_t);

    /* Initialisation de la r�solution des syst�mes lin�aires */

    cs_sles_initialize();

    /*------------------------------------------------------------------------
     *  appel du sous-programme de gestion de calcul (noyau du code)
     *------------------------------------------------------------------------*/

    CS_PROCF(caltri, CALTRI)(&(opts.longia),
                             &(opts.longra),
                             &idebia,
                             &idebra,
                             &(opts.iverif),
                             cs_glob_mesh->i_face_cells,
                             cs_glob_mesh->b_face_cells,
                             cs_glob_mesh->b_face_family,
                             cs_glob_mesh->cell_family,
                             cs_glob_mesh->family_item,
                             cs_glob_mesh->i_face_vtx_idx,
                             cs_glob_mesh->i_face_vtx_lst,
                             cs_glob_mesh->b_face_vtx_idx,
                             cs_glob_mesh->b_face_vtx_lst,
                             ia,
                             cs_glob_mesh_quantities->cell_cen,
                             cs_glob_mesh_quantities->i_face_normal,
                             cs_glob_mesh_quantities->b_face_normal,
                             cs_glob_mesh_quantities->i_face_cog,
                             cs_glob_mesh_quantities->b_face_cog,
                             cs_glob_mesh->vtx_coord,
                             cs_glob_mesh_quantities->cell_vol,
                             ra);

    /* Fin de la r�solution des syst�mes lin�aires */

    cs_sles_finalize();

    /* les fichiers listing de noeuds > 0 sont ferm�s dans caltri. */

    /* Lib�ration des tableaux de travail */
    BFT_FREE(ia);
    BFT_FREE(ra);

  }

  bft_printf(_("\n Destruction des structures et cl�ture du calcul\n"));
  bft_printf_flush();

  /* Lib�ration de structures internes de l'API F77 pour fichiers suite */

  cs_suite_f77_api_finalize();

  /* Lib�ration de la m�moire �ventuellement affect�e aux couplages */

  cs_syr_coupling_all_destroy();
#if defined(_CS_HAVE_MPI)
  cs_couplage_detruit_tout();
#endif

  /* Lib�ration de la m�moire associ�e aux post-traitements */

  cs_post_detruit();

  /* Lib�ration du maillage principal */

  cs_mesh_quantities_destroy(cs_glob_mesh_quantities);
  cs_mesh_destroy(cs_glob_mesh);

  /* Temps CPU et finalisation de la gestion m�moire */
  cs_base_bilan_temps();
  cs_base_mem_fin();

  /* retour */
  cs_exit(EXIT_SUCCESS);

  /* jamais appel� normalement, mais pour �viter un warning de compilation */
  return 0;

}

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */
