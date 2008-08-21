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
 * Functions associated with code coupling.
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_config.h>
#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include <fvm_locator.h>
#include <fvm_nodal.h>
#include <fvm_writer.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"
#include "cs_mesh_connect.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_couplage.h"

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

#if defined(_CS_HAVE_MPI)

/*=============================================================================
 * Local Structure Definitions
 *============================================================================*/

struct _cs_couplage_t {

  fvm_locator_t   *localis_cel;  /* Localisateur associ� aux cellules */
  fvm_locator_t   *localis_fbr;  /* Localisateur associ� aux faces de bord */

  cs_int_t         nbr_cel_sup;  /* Nombre de cellules support associ�es */
  cs_int_t         nbr_fbr_sup;  /* Nombre de faces de bord support associ�es */
  fvm_nodal_t     *cells_sup;    /* Cellules locales servant de support
                                    d'interpolation � des valeurs distantes */
  fvm_nodal_t     *faces_sup;    /* Faces locales servant de support
                                    d'interpolation � des valeurs distantes */

#if defined(_CS_HAVE_MPI)

  MPI_Comm         comm;         /* Communicateur MPI associ� */

  cs_int_t         nb_rangs_dist;  /* Nombre de processus distants associ�s */
  cs_int_t         rang_deb_dist;  /* Premier rang distant associ� */

#endif

};

/*============================================================================
 *  Variables globales statiques
 *============================================================================*/

/* Tableau des couplages */

static int              cs_glob_nbr_couplages = 0;
static int              cs_glob_nbr_couplages_max = 0;
static cs_couplage_t  **cs_glob_couplages = NULL;

/*============================================================================
 * Prototypes de fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Cr�ation d'un couplage.
 *
 * On autorise les couplages soit avec des groupes de processus totalement
 * distincts du groupe principal (correspondant � cs_glob_base_mpi_comm),
 * soit avec ce m�me groupe.
 *----------------------------------------------------------------------------*/

static cs_couplage_t  * cs_loc_couplage_cree
(
 const cs_int_t   rang_deb            /* --> rang du premier processus coupl� */
);


/*----------------------------------------------------------------------------
 * Destruction d'un couplage
 *----------------------------------------------------------------------------*/

static cs_couplage_t  * cs_loc_couplage_detruit
(
 cs_couplage_t  *couplage             /* <-> pointeur sur structure � lib�rer */
);


/*============================================================================
 * Fonctions Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * R�cup�ration du nombre de cas de couplage
 *
 * Interface Fortran :
 *
 * SUBROUTINE NBCCPL
 * *****************
 *
 * INTEGER          NBRCPL         : <-- : nombre de couplages
 *----------------------------------------------------------------------------*/

void CS_PROCF (nbccpl, NBCCPL)
(
 cs_int_t  *const nbrcpl              /* <-- nombre de couplages              */
)
{
  *nbrcpl = cs_glob_nbr_couplages;
}


/*----------------------------------------------------------------------------
 * Affectation des listes de cellules et de faces de bord associ�es �
 * un couplage, ainsi que d'un ensemble de points.
 *
 * Les cellules et faces de bord locales "support" servent de base de
 * localisation des valeurs aux cellules et faces "coupl�e" distantes.
 * Selon le r�le �metteur et/ou destinataire du processus courant dans le
 * couplage, certains de ces ensembles peuvent �tre vides ou non.
 *
 * Les valeurs aux cellules seront toujours localis�es et interpol�es
 * par rapport au support "cellules" distant. Les valeurs aux faces
 * de bord seront localis�es et interpol�es par rapport au support
 * "faces de bord" s'il existe, et par rapport au support "cellules"
 * sinon. Vu du processeur local, on affecte (g�n�ralement par
 * interpolation) des valeurs � 0 � 2 ensembles de points distants,
 * dont l'un prendra les valeurs bas�es sur les cellules, et l'autre
 * soit sur les cellules, soit sur les faces de bord (selon si l'on
 * a d�fini les faces de bord comme support ou non).
 *
 * Si les tableaux LCESUP et LFBSUP ne sont pas tri�s en entr�e, ils
 * le seront en sortie
 *
 * Interface Fortran :
 *
 * SUBROUTINE DEFCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NCESUP         : --> : nombre de cellules support
 * INTEGER          NFBSUP         : --> : nombre de faces de bord support
 * INTEGER          NCECPL         : --> : nombre de cellules coupl�es
 * INTEGER          NFBCPL         : --> : nombre de faces de bord coupl�es
 * INTEGER          LCESUP(NCESUP) : --> : liste des cellules support
 * INTEGER          LFBSUP(NFBSUP) : --> : liste des faces de bord support
 * INTEGER          LCECPL(NCECPL) : --> : liste des cellules coupl�es
 * INTEGER          LFBCPL(NFBCPL) : --> : liste des faces de bord coupl�es
 *----------------------------------------------------------------------------*/

void CS_PROCF (defcpl, DEFCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
 const cs_int_t  *const ncesup,       /* --> nombre de cellules support       */
 const cs_int_t  *const nfbsup,       /* --> nombre de faces de bord support  */
 const cs_int_t  *const ncecpl,       /* --> nombre de cellules coupl�es      */
 const cs_int_t  *const nfbcpl,       /* --> nombre de faces de bord coupl�es */
       cs_int_t         lcesup[],     /* <-> liste des cellules support       */
       cs_int_t         lfbsup[],     /* <-> liste des faces de bord support  */
 const cs_int_t         lcecpl[],     /* --> liste des cellules coupl�es      */
 const cs_int_t         lfbcpl[]      /* --> liste des faces de bord coupl�es */
)
{
  /* Variables locales */

  cs_int_t  ind;

  int  indic_glob[2] = {0, 0};
  int  indic_loc[2] = {0, 0};

  cs_couplage_t  *coupl = NULL;
  fvm_nodal_t  *support_fbr = NULL;
  cs_mesh_quantities_t  *mesh_quantities = cs_glob_mesh_quantities;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  /* Suppression des informations de connectivite et localisation en
   * cas de mise � jour du couplage */

  if (coupl->cells_sup != NULL)
    fvm_nodal_destroy(coupl->cells_sup);
  if (coupl->faces_sup != NULL)
    fvm_nodal_destroy(coupl->faces_sup);

  /* Cr�ation des listes locales */

  coupl->nbr_cel_sup = *ncesup;
  coupl->nbr_fbr_sup = *nfbsup;

  /* Cr�ation des structures fvm correspondantes */

  if (*ncesup > 0)
    indic_loc[0] = 1;
  if (*nfbsup > 0)
    indic_loc[1] = 1;

  for (ind = 0 ; ind < 2 ; ind++)
    indic_glob[ind] = indic_loc[ind];

#if defined(_CS_HAVE_MPI)
  if (cs_glob_base_nbr > 1)
    MPI_Allreduce (indic_loc, indic_glob, 2, MPI_INT, MPI_MAX,
                   cs_glob_base_mpi_comm);
#endif

  if (indic_glob[0] > 0)
    coupl->cells_sup = cs_maillage_extrait_cel_nodal(cs_glob_mesh,
                                                     "cellules_couplees",
                                                     *ncesup,
                                                     lcesup);
  if (indic_glob[1] > 0)
    coupl->faces_sup = cs_maillage_extrait_fac_nodal(cs_glob_mesh,
                                                     "faces_bord_couplees",
                                                     0,
                                                     *nfbsup,
                                                     NULL,
                                                     lfbsup);

  /* Initialisation de la localisation des correspondants */

  fvm_locator_set_nodal(coupl->localis_cel,
                        coupl->cells_sup,
                        1,
                        3,
                        *ncecpl,
                        lcecpl,
                        mesh_quantities->cell_cen);

  if (indic_glob[1] > 0)
    support_fbr = coupl->faces_sup;
  else
    support_fbr = coupl->cells_sup;

  fvm_locator_set_nodal(coupl->localis_fbr,
                        support_fbr,
                        1,
                        3,
                        *nfbcpl,
                        lfbcpl,
                        mesh_quantities->b_face_cog);

#if 0
  /* TODO : permettre l'association des maillages fvm au post traitement,
     via une fonction fournissant un pointeur sur les structures fvm
     associ�es, et une autre permettant leur reduction ou suppression */
  {
    fvm_writer_t *w = fvm_writer_init("maillage_coupl",
                                      NULL,
                                      "EnSight Gold",
                                      "binary",
                                      FVM_WRITER_FIXED_MESH);

    fvm_writer_export_nodal(w, coupl->cells_sup);
    fvm_writer_finalize(w);

  }
#endif

  /* R�duction des supports d'interpolation (pourraient �tre supprim�s) */

  if (coupl->cells_sup != NULL)
    fvm_nodal_reduce(coupl->cells_sup, 1);
  if (coupl->faces_sup != NULL)
    fvm_nodal_reduce(coupl->faces_sup, 1);

#if 0 && defined(DEBUG) && !defined(NDEBUG)
  fvm_locator_dump(coupl->localis_cel);
  fvm_locator_dump(coupl->localis_fbr);
#endif

}


/*----------------------------------------------------------------------------
 * R�cup�ration des nombres de cellules et faces de bord support, coupl�es,
 * et non localis�es associ�es � un couplage
 *
 * Interface Fortran :
 *
 * SUBROUTINE NBECPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NCESUP         : <-- : nombre de cellules support
 * INTEGER          NFBSUP         : <-- : nombre de faces de bord support
 * INTEGER          NCECPL         : <-- : nombre de cellules coupl�es
 * INTEGER          NFBCPL         : <-- : nombre de faces de bord coupl�es
 * INTEGER          NCENCP         : <-- : nombre de cellules non coupl�es
 *                                 :     : car non localis�es
 * INTEGER          NFBNCP         : <-- : nombre de faces de bord non
 *                                 :     : coupl�es car non localis�es
 *----------------------------------------------------------------------------*/

void CS_PROCF (nbecpl, NBECPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
       cs_int_t  *const ncesup,       /* <-- nombre de cellules support       */
       cs_int_t  *const nfbsup,       /* <-- nombre de faces de bord support  */
       cs_int_t  *const ncecpl,       /* <-- nombre de cellules coupl�es      */
       cs_int_t  *const nfbcpl,       /* <-- nombre de faces de bord coupl�es */
       cs_int_t  *const ncencp,       /* <-- nombre de cellules non coupl�es
                                       *     car non localis�es               */
       cs_int_t  *const nfbncp        /* <-- nombre de faces de bord non
                                       *     coupl�es car non localis�es      */
)
{
  cs_couplage_t  *coupl = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  *ncesup = coupl->nbr_cel_sup;
  *nfbsup = coupl->nbr_fbr_sup;

  *ncecpl = 0;
  *nfbcpl = 0;

  *ncencp = 0;
  *nfbncp = 0;

  if (coupl->localis_cel != NULL) {
    *ncecpl = fvm_locator_get_n_interior(coupl->localis_cel);
    *ncencp = fvm_locator_get_n_exterior(coupl->localis_cel);
  }

  if (coupl->localis_fbr != NULL) {
    *nfbcpl = fvm_locator_get_n_interior(coupl->localis_fbr);
    *nfbncp = fvm_locator_get_n_exterior(coupl->localis_fbr);
  }

}


/*----------------------------------------------------------------------------
 * R�cup�ration des listes de cellules et de faces de bord coupl�es
 * (i.e. r�ceptrices) associ�es � un couplage
 *
 * Le nombre de cellules et faces de bord, obtenus via NBECPL(), sont
 * fournis � des fins de v�rification de coh�rence des arguments.
 *
 * Interface Fortran :
 *
 * SUBROUTINE LELCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NCECPL         : --> : nombre de cellules coupl�es
 * INTEGER          NFBCPL         : --> : nombre de faces de bord coupl�es
 * INTEGER          LCECPL(*)      : <-- : liste des cellules coupl�es
 * INTEGER          LFBCPL(*)      : <-- : liste des faces de bord coupl�es
 *----------------------------------------------------------------------------*/

void CS_PROCF (lelcpl, LELCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du cas de couplage        */
 const cs_int_t  *const ncecpl,       /* --> nombre de cellules coupl�es      */
 const cs_int_t  *const nfbcpl,       /* --> nombre de faces de bord coupl�es */
       cs_int_t  *const lcecpl,       /* <-- liste des cellules coupl�es      */
       cs_int_t  *const lfbcpl        /* <-- liste des faces de bord coupl�es */
)
{
  cs_int_t  ind;

  cs_int_t  _ncecpl = 0;
  cs_int_t  _nfbcpl = 0;

  cs_couplage_t  *coupl = NULL;

  const cs_int_t  *lst = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  if (coupl->localis_cel != NULL)
    _ncecpl = fvm_locator_get_n_interior(coupl->localis_cel);

  if (coupl->localis_fbr != NULL)
    _nfbcpl = fvm_locator_get_n_interior(coupl->localis_fbr);

  if (*ncecpl != _ncecpl || *nfbcpl != _nfbcpl)
    bft_error(__FILE__, __LINE__, 0,
              _("Coupling %d: inconsistent arguments for LELCPL()\n"
                "NCECPL = %d and NFBCPL = %d are indicated.\n"
                "The values for this coupling should be %d and %d."),
              *numcpl, (int)(*ncecpl), (int)(*nfbcpl),
              (int)_ncecpl, (int)_nfbcpl);

  /* Copie des listes (serait inutile avec un API C pure) */

  if (_ncecpl > 0) {
    lst = fvm_locator_get_interior_list(coupl->localis_cel);
    for (ind = 0 ; ind < _ncecpl ; ind++)
      lcecpl[ind] = lst[ind];
  }

  if (_nfbcpl > 0) {
    lst = fvm_locator_get_interior_list(coupl->localis_fbr);
    for (ind = 0 ; ind < _nfbcpl ; ind++)
      lfbcpl[ind] = lst[ind];
  }
}


/*----------------------------------------------------------------------------
 * R�cup�ration des listes de cellules et de faces de bord non coupl�es
 * (i.e. r�ceptrices mais non localis�es) associ�es � un couplage
 *
 * Le nombre de cellules et faces de bord, obtenus via NBECPL(), sont
 * fournis � des fins de v�rification de coh�rence des arguments.
 *
 * Interface Fortran :
 *
 * SUBROUTINE LENCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NCENCP         : <-- : nombre de cellules non coupl�es
 *                                 :     : car non localis�es
 * INTEGER          NFBNCP         : <-- : nombre de faces de bord non
 *                                 :     : coupl�es car non localis�es
 * INTEGER          LCENCP(*)      : <-- : liste des cellules non coupl�es
 * INTEGER          LFBNCP(*)      : <-- : liste des faces de bord non coupl�es
 *----------------------------------------------------------------------------*/

void CS_PROCF (lencpl, LENCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du cas de couplage        */
 const cs_int_t  *const ncencp,       /* --> nombre de cellules non coupl�es
                                       *     car non localis�es               */
 const cs_int_t  *const nfbncp,       /* --> nombre de faces de bord non
                                       *     coupl�es car non localis�es      */
       cs_int_t  *const lcencp,       /* <-- liste des cellules non coupl�es  */
       cs_int_t  *const lfbncp        /* <-- liste des faces de bord non
                                       *     coupl�es                         */
)
{
  cs_int_t  ind;

  cs_int_t  _ncencp = 0;
  cs_int_t  _nfbncp = 0;
  cs_couplage_t  *coupl = NULL;

  const cs_int_t  *lst = NULL;


  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  if (coupl->localis_cel != NULL)
    _ncencp = fvm_locator_get_n_exterior(coupl->localis_cel);

  if (coupl->localis_fbr != NULL)
    _nfbncp = fvm_locator_get_n_exterior(coupl->localis_fbr);

  if (*ncencp != _ncencp || *nfbncp != _nfbncp)
    bft_error(__FILE__, __LINE__, 0,
              _("Coupling %d: inconsistent arguments for LELNCP()\n"
                "NCENCP = %d and NFBNCP = %d are indicated.\n"
                "The values for this coupling should be %d and %d."),
              *numcpl, (int)(*ncencp), (int)(*nfbncp),
              (int)_ncencp, (int)_nfbncp);

  /* Copie des listes (serait inutile avec un API C pure) */

  if (_ncencp > 0) {
    lst = fvm_locator_get_exterior_list(coupl->localis_cel);
    for (ind = 0 ; ind < _ncencp ; ind++)
      lcencp[ind] = lst[ind];
  }

  if (_nfbncp > 0) {
    lst = fvm_locator_get_exterior_list(coupl->localis_fbr);
    for (ind = 0 ; ind < _nfbncp ; ind++)
      lfbncp[ind] = lst[ind];
  }
}


/*----------------------------------------------------------------------------
 * R�cup�ration du nombre de points distants associ�s � un couplage
 * et localis�s par rapport au domaine local
 *
 * Interface Fortran :
 *
 * SUBROUTINE NPDCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NCEDIS         : <-- : nombre de cellules distantes
 * INTEGER          NFBDIS         : <-- : nombre de faces de bord distantes
 *----------------------------------------------------------------------------*/

void CS_PROCF (npdcpl, NPDCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
       cs_int_t  *const ncedis,       /* <-- nombre de cellules distantes     */
       cs_int_t  *const nfbdis        /* <-- nombre de faces de bord dist.    */
)
{
  /* Variables locales */

  cs_couplage_t  *coupl = NULL;

  /* V�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  /* R�cup�ration du nombre de points */

  *ncedis = 0;
  *nfbdis = 0;

  if (coupl->localis_cel != NULL)
    *ncedis = fvm_locator_get_n_dist_points(coupl->localis_cel);

  if (coupl->localis_fbr != NULL)
    *nfbdis = fvm_locator_get_n_dist_points(coupl->localis_fbr);

}

/*----------------------------------------------------------------------------
 * R�cup�ration des coordonn�es des points distants affect�s � un
 * couplage et une liste de points, ainsi que les num�ros et le
 * type d'�l�ment (cellules ou faces) "contenant" ces points.
 *
 * Le nombre de points distants NBRPTS doit �tre �gal � l'un des arguments
 * NCEDIS ou NFBDIS retourn�s par NPDCPL(), et est fourni ici � des fins
 * de v�rification de coh�rence avec les arguments NUMCPL et ITYSUP.
 *
 * Interface Fortran :
 *
 * SUBROUTINE COOCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NBRPTS         : --> : nombre de points distants
 * INTEGER          ITYDIS         : --> : 1 : acc�s aux points affect�s
 *                                 :     :     aux cellules distantes
 *                                 :     : 2 : acc�s aux points affect�s
 *                                 :     :     aux faces de bord distantes
 * INTEGER          ITYLOC         : <-- : 1 : localisation par rapport
 *                                 :     :     aux cellules locales
 *                                 :     : 2 : localisation par rapport
 *                                 :     :     aux faces de bord locales
 * INTEGER          LOCPTS(*)      : <-- : num�ro du "contenant" associ� �
 *                                 :     :   chaque point
 * DOUBLE PRECISION COOPTS(3,*)    : <-- : coordonn�es des points distants
 *----------------------------------------------------------------------------*/

void CS_PROCF (coocpl, COOCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
 const cs_int_t  *const nbrpts,       /* --> nombre de points distants        */
 const cs_int_t  *const itydis,       /* --> 1 : acc�s aux points affect�s
                                       *         aux cellules distantes
                                       *     2 : acc�s aux points affect�s
                                       *         aux faces de bord distantes  */
       cs_int_t  *const ityloc,       /* <-- 1 : localisation par rapport
                                       *         aux cellules locales
                                       *     2 : localisation par rapport
                                       *         aux faces de bord locales    */
       cs_int_t  *const locpts,       /* <-- liste des mailles associ�es      */
       cs_real_t *const coopts        /* <-- coord. des points � localiser    */
)
{
  /* Variables locales */

  cs_int_t  ind, icoo;

  cs_int_t  n_pts_dist = 0;
  cs_couplage_t  *coupl = NULL;
  fvm_locator_t  *localis = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  *ityloc = 0;

  if (*itydis == 1) {
    localis = coupl->localis_cel;
    *ityloc = 1;
  }
  else if (*itydis == 2) {
    localis = coupl->localis_fbr;
    if (coupl->nbr_fbr_sup > 0)
      *ityloc = 2;
    else
      *ityloc = 1;
  }

  if (localis != NULL)
    n_pts_dist = fvm_locator_get_n_dist_points(localis);

  if (*nbrpts != n_pts_dist)
    bft_error(__FILE__, __LINE__, 0,
              _("Coupling %d: inconsistent arguments for COOCPL()\n"
                "ITYDIS = %d and NBRPTS = %d are indicated.\n"
                "The value for NBRPTS should be %d."),
              *numcpl, (int)(*itydis), (int)(*nbrpts), (int)n_pts_dist);

  /* Cr�ation des listes locales */

  if (localis != NULL) {

    n_pts_dist = fvm_locator_get_n_dist_points(localis);

    if (n_pts_dist > 0) {

      const fvm_lnum_t   *element;
      const fvm_coord_t  *coord;

      element = fvm_locator_get_dist_locations(localis);
      coord   = fvm_locator_get_dist_coords(localis);

      for (ind = 0 ; ind < n_pts_dist ; ind++) {
        locpts[ind] = element[ind];
        for (icoo = 0 ; icoo < 3 ; icoo++)
          coopts[ind*3 + icoo] = coord[ind*3 + icoo];
      }

    }

  }

}

/*----------------------------------------------------------------------------
 * Echange d'une variable associ�e � un ensemble de points et � un couplage.
 *
 * Interface Fortran :
 *
 * SUBROUTINE VARCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NBRDIS         : --> : Nombre de valeurs � envoyer
 * INTEGER          NBRLOC         : --> : Nombre de valeurs � recevoir
 * INTEGER          ITYVAR         : --> : 1 : variables aux cellules
 *                                 :     : 2 : variables aux faces de bord
 * DOUBLE PRECISION VARDIS(*) )    : --> : variable distante (� envoyer)
 * DOUBLE PRECISION VARLOC(*) )    : <-- : variable locale (� recevoir)
 *----------------------------------------------------------------------------*/

void CS_PROCF (varcpl, VARCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
 const cs_int_t  *const nbrdis,       /* --> nombre de valeurs � envoyer      */
 const cs_int_t  *const nbrloc,       /* --> nombre de valeurs � recevoir     */
 const cs_int_t  *const ityvar,       /* --> 1 : variables aux cellules
                                       *     2 : variables aux faces de bord  */
       cs_real_t *const vardis,       /* --> variable distante (� envoyer)    */
       cs_real_t *const varloc        /* <-- variable locale (� recevoir)     */
)
{
  /* Variables locales */

  cs_int_t  n_val_dist_ref = 0;
  cs_int_t  n_val_loc_ref = 0;
  cs_real_t  *val_dist = NULL;
  cs_real_t  *val_loc = NULL;
  cs_couplage_t  *coupl = NULL;
  fvm_locator_t  *localis = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  if (*ityvar == 1)
    localis = coupl->localis_cel;
  else if (*ityvar == 2)
    localis = coupl->localis_fbr;

  if (localis != NULL) {
    n_val_dist_ref = fvm_locator_get_n_dist_points(localis);
    n_val_loc_ref  = fvm_locator_get_n_interior(localis);
  }

  if (*nbrdis > 0 && *nbrdis != n_val_dist_ref)
    bft_error(__FILE__, __LINE__, 0,
              _("Coupling %d: inconsistent arguments for VARCPL()\n"
                "ITYVAR = %d and NBRDIS = %d are indicated.\n"
                "NBRDIS should be 0 or %d."),
              *numcpl, (int)(*ityvar), (int)(*nbrdis), (int)n_val_dist_ref);

  if (*nbrloc > 0 && *nbrloc != n_val_loc_ref)
    bft_error(__FILE__, __LINE__, 0,
              _("Coupling %d: inconsistent arguments for VARCPL()\n"
                "ITYVAR = %d and NBRLOC = %d are indicated.\n"
                "NBRLOC should be 0 or %d."),
              *numcpl, (int)(*ityvar), (int)(*nbrloc), (int)n_val_loc_ref);

  /* Cr�ation des listes locales */

  if (localis != NULL) {

    if (*nbrdis > 0)
      val_dist = vardis;
    if (*nbrloc > 0)
      val_loc = varloc;

    fvm_locator_exchange_point_var(localis,
                                   val_dist,
                                   val_loc,
                                   NULL,
                                   sizeof(cs_real_t),
                                   1,
                                   0);

  }

}


/*----------------------------------------------------------------------------
 * Echange de tableaux d'entiers associ�s � un couplage.
 *
 * On suppose que les tableaux � �changer sont de m�me taille et contiennent
 * les m�mes valeurs sur chaque groupe de processus (locaux et distants).
 *
 * Interface Fortran :
 *
 * SUBROUTINE TBICPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NBRDIS         : --> : Nombre de valeurs � envoyer
 * INTEGER          NBRLOC         : --> : Nombre de valeurs � recevoir
 * INTEGER          TABDIS(*) )    : --> : valeurs distantes (� envoyer)
 * INTEGER          TABLOC(*) )    : --> : valeurs locales (� recevoir)
 *----------------------------------------------------------------------------*/

void CS_PROCF (tbicpl, TBICPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
 const cs_int_t  *const nbrdis,       /* --> nombre de valeurs � envoyer      */
 const cs_int_t  *const nbrloc,       /* --> nombre de valeurs � recevoir     */
       cs_int_t  *const vardis,       /* --> variable distante (� envoyer)    */
       cs_int_t  *const varloc        /* <-- variable locale (� recevoir)     */
)
{
#if defined(_CS_HAVE_MPI)

  /* Variables locales */

  cs_int_t  ind;
  MPI_Status  status;

  cs_int_t  nbr = 0;
  cs_bool_t  distant = CS_FALSE;
  cs_couplage_t  *coupl = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  if (coupl->comm != MPI_COMM_NULL) {

    distant = CS_TRUE;

    /* Enchanges entre les t�tes de groupes */

    if (cs_glob_base_rang < 1)
      MPI_Sendrecv(vardis, *nbrdis, CS_MPI_INT, coupl->rang_deb_dist, 0,
                   varloc, *nbrloc, CS_MPI_INT, coupl->rang_deb_dist, 0,
                   coupl->comm, &status);

    /* Synchronisation � l'int�rieur d'un groupe */

    if (cs_glob_base_nbr > 1)
      MPI_Bcast (varloc, *nbrloc, CS_MPI_INT, 0, cs_glob_base_mpi_comm);

  }

#endif /* defined(_CS_HAVE_MPI) */

  if (distant == CS_FALSE) {

    nbr = CS_MIN(*nbrdis, *nbrloc);

    for (ind = 0; ind < nbr; ind++)
      varloc[ind] = vardis[ind];

  }
}

/*----------------------------------------------------------------------------
 * Echange de tableaux de r�els associ�s � un couplage.
 *
 * On suppose que les tableaux � �changer sont de m�me taille et contiennent
 * les m�mes valeurs sur chaque groupe de processus (locaux et distants).
 *
 * Interface Fortran :
 *
 * SUBROUTINE TBRCPL
 * *****************
 *
 * INTEGER          NUMCPL         : --> : num�ro du couplage
 * INTEGER          NBRDIS         : --> : Nombre de valeurs � envoyer
 * INTEGER          NBRLOC         : --> : Nombre de valeurs � recevoir
 * DOUBLE PRECISION TABDIS(*) )    : --> : valeurs distantes (� envoyer)
 * DOUBLE PRECISION TABLOC(*) )    : --> : valeurs locales (� recevoir)
 *----------------------------------------------------------------------------*/

void CS_PROCF (tbrcpl, TBRCPL)
(
 const cs_int_t  *const numcpl,       /* --> num�ro du couplage               */
 const cs_int_t  *const nbrdis,       /* --> nombre de valeurs � envoyer      */
 const cs_int_t  *const nbrloc,       /* --> nombre de valeurs � recevoir     */
       cs_real_t *const vardis,       /* --> variable distante (� envoyer)    */
       cs_real_t *const varloc        /* <-- variable locale (� recevoir)     */
)
{
#if defined(_CS_HAVE_MPI)

  /* Variables locales */

  cs_int_t  ind;
  MPI_Status  status;

  cs_int_t  nbr = 0;
  cs_bool_t  distant = CS_FALSE;
  cs_couplage_t  *coupl = NULL;

  /* Initialisations et v�rifications */

  if (*numcpl < 1 || *numcpl > cs_glob_nbr_couplages)
    bft_error(__FILE__, __LINE__, 0,
              _("Impossible coupling number %d; there are %d couplings"),
              *numcpl, cs_glob_nbr_couplages);
  else
    coupl = cs_glob_couplages[*numcpl - 1];

  if (coupl->comm != MPI_COMM_NULL) {

    distant = CS_TRUE;

    /* Enchanges entre les t�tes de groupes */

    if (cs_glob_base_rang < 1)
      MPI_Sendrecv(vardis, *nbrdis, CS_MPI_REAL, coupl->rang_deb_dist, 0,
                   varloc, *nbrloc, CS_MPI_REAL, coupl->rang_deb_dist, 0,
                   coupl->comm, &status);

    /* Synchronisation � l'int�rieur d'un groupe */

    if (cs_glob_base_nbr > 1)
      MPI_Bcast(varloc, *nbrloc, CS_MPI_REAL, 0, cs_glob_base_mpi_comm);

  }

#endif /* defined(_CS_HAVE_MPI) */

  if (distant == CS_FALSE) {

    nbr = CS_MIN(*nbrdis, *nbrloc);

    for (ind = 0; ind < nbr; ind++)
      varloc[ind] = vardis[ind];

  }
}

/*============================================================================
 * Fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Ajout d'un couplage.
 *
 * On autorise les couplages soit avec des groupes de processus totalement
 * distincts du groupe principal (correspondant � cs_glob_base_mpi_comm),
 * soit avec ce m�me groupe.
 *----------------------------------------------------------------------------*/

void  cs_couplage_ajoute
(
 const cs_int_t   rang_deb            /* --> rang du premier processus coupl� */
)
{
    /* variables locales */

  cs_couplage_t  *couplage = NULL;


  /* Cr�ation de la structure associ�e */

  couplage = cs_loc_couplage_cree(rang_deb);


  /* Redimensionnement du tableau global des couplages */

  if (cs_glob_nbr_couplages == cs_glob_nbr_couplages_max) {

    if (cs_glob_nbr_couplages_max == 0)
      cs_glob_nbr_couplages_max = 2;
    else
      cs_glob_nbr_couplages_max *= 2;

    BFT_REALLOC(cs_glob_couplages,
                cs_glob_nbr_couplages_max,
                cs_couplage_t *);

  }

  /* Affectation du couplage nouvellement cr�e � la structure */

  cs_glob_couplages[cs_glob_nbr_couplages] = couplage;

  cs_glob_nbr_couplages += 1;

  return;

}


/*----------------------------------------------------------------------------
 * Suppression des couplages
 *----------------------------------------------------------------------------*/

void cs_couplage_detruit_tout
(
 void
)
{
  cs_int_t  i;

  for (i = 0 ; i < cs_glob_nbr_couplages ; i++)
    cs_loc_couplage_detruit(cs_glob_couplages[i]);

  BFT_FREE(cs_glob_couplages);

  cs_glob_nbr_couplages = 0;
  cs_glob_nbr_couplages_max = 0;
}


/*============================================================================
 * Fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Cr�ation d'un couplage.
 *
 * On autorise les couplages soit avec des groupes de processus totalement
 * distincts du groupe principal (correspondant � cs_glob_base_mpi_comm),
 * soit avec ce m�me groupe.
 *----------------------------------------------------------------------------*/

static cs_couplage_t  * cs_loc_couplage_cree
(
 const cs_int_t   rang_deb            /* --> rang du premier processus coupl� */
)
{
    /* variables locales */

  int  mpi_flag = 0;
  int  nb_rangs_dist = 0;
  int  rang_deb_dist = 0;
  cs_couplage_t  *couplage = NULL;

  const double  tolerance = 0.1;

  /* Cr�ation de la structure associ�e et association d'un communicateur MPI */

  BFT_MALLOC(couplage, 1, cs_couplage_t);

#if defined(_CS_HAVE_MPI)

  MPI_Initialized(&mpi_flag);

  if (mpi_flag == 0)
    couplage->comm = MPI_COMM_NULL;

  else {

    int  nb_rangs_loc, nb_rangs_glob, r_glob, r_loc_min, r_loc_max;

    /* V�rification que les processus coupl�s se chevauchent exactement
       ou pas du tout */

    MPI_Comm_rank(MPI_COMM_WORLD, &r_glob);
    MPI_Allreduce(&r_glob, &r_loc_min, 1, MPI_INT, MPI_MIN, cs_glob_base_mpi_comm);
    MPI_Allreduce(&r_glob, &r_loc_max, 1, MPI_INT, MPI_MAX, cs_glob_base_mpi_comm);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_rangs_glob);

    MPI_Comm_size(cs_glob_base_mpi_comm, &nb_rangs_loc);

    if (rang_deb > r_loc_min && rang_deb <= r_loc_max)
      bft_error(__FILE__, __LINE__, 0,
                _("Coupling definition is impossible: a distant root rank equal to\n"
                  "%d is required, whereas the local group corresponds to\n"
                  "rank %d to %d\n"),
                (int)rang_deb, r_loc_min, r_loc_max);

    else if (rang_deb < 0 || rang_deb >= nb_rangs_glob)
      bft_error(__FILE__, __LINE__, 0,
                _("Coupling definition is impossible: a distant root rank equal to\n"
                  "%d is required, whereas the global ranks (MPI_COMM_WORLD)\n"
                  "range from to 0 to %d\n"),
                (int)rang_deb, nb_rangs_glob - 1);

    /* Cas d'un couplage interne au groupe de processus */

    if (rang_deb == r_loc_min) {
      if (nb_rangs_loc == 1)
        couplage->comm = MPI_COMM_NULL;
      else
        couplage->comm = cs_glob_base_mpi_comm;
      nb_rangs_dist = nb_rangs_loc;
    }

    /* Cas d'un couplage externe au groupe de processus */

    else {

      MPI_Comm  intercomm_tmp;
      int  r_coupl, r_coupl_min;
      int  haut = (rang_deb > r_loc_max) ? 0 : 1;
      const int  cs_couplage_tag = 'C'+'S'+'_'+'C'+'O'+'U'+'P'+'L'+'A'+'G'+'E';

      /* Cr�ation d'un communicateur r�serv� */

      MPI_Intercomm_create(cs_glob_base_mpi_comm, 0, MPI_COMM_WORLD,
                           (int)rang_deb, cs_couplage_tag, &intercomm_tmp);

      MPI_Intercomm_merge(intercomm_tmp, haut, &(couplage->comm));

      MPI_Comm_free(&intercomm_tmp);

      /* Calcul du nombre de rangs distants et du premier rang distant */

      MPI_Comm_size(couplage->comm, &nb_rangs_dist);
      nb_rangs_dist -= nb_rangs_loc;

      /* V�rification du rang dans le nouveau communicateur (ne devrait
         pas �tre n�cessaire avec valeur "haut" bien positionn�e,
         mais semble l'�tre avec Open MPI 1.0.1) */

      MPI_Comm_rank(couplage->comm, &r_coupl);
      MPI_Allreduce(&r_coupl, &r_coupl_min, 1, MPI_INT, MPI_MIN,
                    cs_glob_base_mpi_comm);
      haut = (r_coupl_min == 0) ? 0 : 1;

      /* On en d�duit la postion du premier rang distant dans le
       * nouveau communicateur */

      if (haut == 0)
        rang_deb_dist = nb_rangs_loc;
      else
        rang_deb_dist = 0;

      bft_printf("r %d (%d / %d) : nb_rangs_dist = %d, rang_deb_dist = %d\n",
                 r_glob, haut, r_coupl, nb_rangs_dist, rang_deb_dist);
    }

  }

  couplage->nb_rangs_dist = nb_rangs_dist;
  couplage->rang_deb_dist = rang_deb_dist;

#endif

  /* Cr�ation des structures de localisation */

#if defined(FVM_HAVE_MPI)

  couplage->localis_cel = fvm_locator_create(tolerance,
                                             couplage->comm,
                                             nb_rangs_dist,
                                             rang_deb_dist);

  couplage->localis_fbr = fvm_locator_create(tolerance,
                                             couplage->comm,
                                             nb_rangs_dist,
                                             rang_deb_dist);

#else

  couplage->localis_cel = fvm_locator_create(tolerance);
  couplage->localis_fbr = fvm_locator_create(tolerance);

#endif

  couplage->nbr_cel_sup = 0;
  couplage->nbr_fbr_sup = 0;
  couplage->cells_sup = NULL;
  couplage->faces_sup = NULL;

  return couplage;
}


/*----------------------------------------------------------------------------
 * Destruction d'un couplage
 *----------------------------------------------------------------------------*/

static cs_couplage_t  * cs_loc_couplage_detruit
(
 cs_couplage_t  *couplage             /* <-> pointeur sur structure � lib�rer */
)
{
  fvm_locator_destroy(couplage->localis_cel);
  fvm_locator_destroy(couplage->localis_fbr);

  if (couplage->cells_sup != NULL)
    fvm_nodal_destroy(couplage->cells_sup);
  if (couplage->faces_sup != NULL)
    fvm_nodal_destroy(couplage->faces_sup);

#if defined(_CS_HAVE_MPI)
  if (   couplage->comm != MPI_COMM_WORLD
      && couplage->comm != cs_glob_base_mpi_comm)
    MPI_Comm_free(&(couplage->comm));
#endif

  BFT_FREE(couplage);

  return NULL;
}


#endif /* _CS_HAVE_MPI */

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */
