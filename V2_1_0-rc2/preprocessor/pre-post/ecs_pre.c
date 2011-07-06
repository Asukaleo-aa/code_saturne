/*============================================================================
 *  Définition de la fonction de base
 *   de remplissage de la structure de maillage à partir des données lues
 *============================================================================*/

/*
  This file is part of the Code_Saturne Preprocessor, element of the
  Code_Saturne CFD tool.

  Copyright (C) 1999-2009 EDF S.A., France

  contact: saturne-support@edf.fr

  The Code_Saturne Preprocessor is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version.

  The Code_Saturne Preprocessor is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the Code_Saturne Preprocessor; if not, write to the
  Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor,
  Boston, MA  02110-1301  USA
*/


/*============================================================================
 *                                 Visibilité
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' visibles du  paquetage global "Utilitaire"
 *----------------------------------------------------------------------------*/

#include "ecs_def.h"
#include "ecs_file.h"
#include "ecs_mem.h"
#include "ecs_timer.h"


/*----------------------------------------------------------------------------
 *  Fichiers `include' visibles des paquetages visibles
 *----------------------------------------------------------------------------*/

#include "ecs_descr.h"
#include "ecs_descr_chaine.h"


#ifdef HAVE_CGNS
#include "ecs_pre_cgns.h"
#endif /* HAVE_CGNS */
#ifdef HAVE_CCM
#include "ecs_pre_ccm.h"
#endif /* HAVE_CCM */
#include "ecs_pre_comet.h"
#include "ecs_pre_ens.h"
#include "ecs_pre_gambit.h"
#include "ecs_pre_gmsh.h"
#include "ecs_pre_ideas.h"
#ifdef HAVE_MED
#include "ecs_pre_med.h"
#endif /* HAVE_MED */
#include "ecs_pre_nopo.h"


/*----------------------------------------------------------------------------
 *  Fichiers `include' visibles du  paquetage courant
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 *  Fichier  `include' du  paquetage courant associe au fichier courant
 *----------------------------------------------------------------------------*/

#include "ecs_pre.h"


/*----------------------------------------------------------------------------
 *  Fichiers `include' privés   du  paquetage courant
 *----------------------------------------------------------------------------*/


/*============================================================================
 *                       Définition des structures
 *============================================================================*/

/* Structures associées aux formats supportés */
/*--------------------------------------------*/

typedef struct {

  char              nom[32];       /* Nom (et version) du format */
  char              extension[8];  /* Extension de fichier */
  char              cle[8];        /* Nom du type pour ligne de commande */
  int               actif;         /* Format disponible ou non (extensions) */
  ecs_pre_format_t  type;          /* Type de format */

} ecs_pre_format_desc_t;

/* Nombre et description des formats supportés */

static const int _ecs_pre_n_formats = 9;

static ecs_pre_format_desc_t _ecs_pre_formats[9] = {

  {
    "CGNS",
    ".cgns",
    "cgns",
#if defined(HAVE_CGNS)
    1,
#else
    0,
#endif
    ECS_PRE_FORMAT_CGNS
  },

  {
    "STAR-CCM+",
    ".ccm",
    "ccm",
#if defined(HAVE_CCM)
    1,
#else
    0,
#endif
    ECS_PRE_FORMAT_CCM
  },

  {
    "pro-STAR/STAR4",
    ".ngeom",
    "ngeom",
    1,
    ECS_PRE_FORMAT_COMET
  },

  {
    N_("EnSight (6 or Gold)"),
    ".case",
    "ensight",
    1,
    ECS_PRE_FORMAT_ENS
  },

  {
    N_("GAMBIT Neutral"),
    ".neu",
    "gambit",
    1,
    ECS_PRE_FORMAT_GAMBIT
  },

  {
    N_("GMSH"),
    ".msh",
    "gmsh",
    1,
    ECS_PRE_FORMAT_GMSH
  },

  {
    N_("I-deas universal"),
    ".unv",
    "unv",
    1,
    ECS_PRE_FORMAT_IDEAS
  },

  {
    "MED",
    ".med",
    "med",
#if defined(HAVE_MED)
    1,
#else
    0,
#endif
    ECS_PRE_FORMAT_MED
  },

  {
    "Simail (NOPO)",
    ".des",
    "des",
    1,
    ECS_PRE_FORMAT_NOPO
  }

};


/*============================================================================
 *                              Fonctions privées
 *============================================================================*/


/*============================================================================
 *                             Fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fonction qui affiche la liste les formats supportés
 *----------------------------------------------------------------------------*/

void
ecs_pre__aff_formats(void)
{
  int i;

  /*xxxxxxxxxxxxxxxxxxxxxxxxxxx Instructions xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/

  for (i = 0; i < _ecs_pre_n_formats; i++) {

    if (_ecs_pre_formats[i].actif > 0) {

      printf("   ");

      ecs_print_padded_str(_(_ecs_pre_formats[i].nom), 30);

      printf("%-9s      %-9s\n",
             _ecs_pre_formats[i].extension,
             _ecs_pre_formats[i].cle);

    }
  }
}

/*----------------------------------------------------------------------------
 *  Fonction qui renvoie le type de format de fichier associé à un fichier
 *   et à une clé optionnelle donnés
 *----------------------------------------------------------------------------*/

ecs_pre_format_t
 ecs_pre__type_format(const char  *nom_fic,
                      const char  *mot_cle)
{
  const char *extension_fic;
  size_t      extension_lng;

  int         ifmt_auto = -1;
  int         ifmt_arg  = -1;

  /*xxxxxxxxxxxxxxxxxxxxxxxxxxx Instructions xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/

  extension_fic = strrchr(nom_fic, '.');
  extension_lng = (extension_fic != NULL) ? strlen(extension_fic) : 0;

  /* S'il s'agit de l'extension 'gz', on cherche l'extension précédente */

  if (ecs_file_version_zlib() != NULL) {

    if (extension_fic != NULL) {

      if (!strcmp(extension_fic, ".gz")) {

        for (extension_fic -= 1;
             extension_fic > nom_fic && *extension_fic != '.';
             extension_fic--);

        if (*extension_fic != '.')
          extension_fic = NULL;
        else
          extension_lng = strlen(extension_fic) - 3;

      }
    }
  }

  if (extension_fic != NULL) {

    /* Une extension a été trouvée */

    for (ifmt_auto = 0; ifmt_auto < _ecs_pre_n_formats; ifmt_auto++) {

      if (!strncmp(extension_fic,
                   _ecs_pre_formats[ifmt_auto].extension,
                   extension_lng))

        break;

    }

    if (ifmt_auto >= _ecs_pre_n_formats)
      ifmt_auto = -1;
  }

  /* Si le format est indiqué, vérifier l'argument */

  if (mot_cle != NULL) {

    size_t  mot_cle_lng = strlen(mot_cle);

    for (ifmt_arg = 0; ifmt_arg < _ecs_pre_n_formats; ifmt_arg++) {

      if (!strncmp(mot_cle,
                   _ecs_pre_formats[ifmt_arg].cle,
                   mot_cle_lng))

        break;
    }

    if (ifmt_arg >= _ecs_pre_n_formats)
      ecs_error(__FILE__, __LINE__, 0,
                _("Error in file format specification.\n\n"
                  "Mesh file type \"%s\" is not recognized.\n"
                  "Impossible to determine the format of mesh file\n"
                  "\"%s\"."),
                mot_cle, nom_fic);
  }

  /* Comparaison entre format détecté automatiquement et extension */

  if (ifmt_arg > -1 && ifmt_auto > -1 && ifmt_arg != ifmt_auto)
    printf(_("\n"
             "Warning\n"
             "=======\n"
             "The file extension for \"%s\"\n"
             "does not correspond with the mesh file format defined by\n"
             "keyword \"%s\". The keyword has priority.\n\n"),
           nom_fic, mot_cle);

  if (ifmt_arg < 0 && ifmt_auto > -1)
    ifmt_arg = ifmt_auto;

  if (ifmt_arg < 0)
    ecs_error(__FILE__, __LINE__, 0,
              _("Mesh file \"%s\" does not have\n"
                "a known extension. A keyword is required to determine\n"
                "the format in this case."),
              nom_fic);

  if (_ecs_pre_formats[ifmt_arg].actif == 0)
    ecs_error(__FILE__, __LINE__, 0,
              _("File format \"%s\" support not available in this\n"
                "installation (file \"%s\"). "),
              _ecs_pre_formats[ifmt_arg].nom, nom_fic);

  /* Return file type */

  return _ecs_pre_formats[ifmt_arg].type;
}

/*----------------------------------------------------------------------------
 *  Fonction qui lit les maillages sur fichiers
 *
 *  La fonction renvoie le maillage concaténé
 *----------------------------------------------------------------------------*/

ecs_maillage_t *
ecs_pre__lit_maillage(const char        *nom_fic,
                      ecs_pre_format_t   format,
                      int                num_maillage,
                      bool               cree_grp_cel_section,
                      bool               cree_grp_cel_zone,
                      bool               cree_grp_fac_section,
                      bool               cree_grp_fac_zone)
{
  double           start_time[2], end_time[2];
  ecs_maillage_t  *maillage = NULL;

  /*xxxxxxxxxxxxxxxxxxxxxxxxxxx Instructions xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/

  start_time[0] = ecs_timer_wtime();
  start_time[1] = ecs_timer_cpu_time();

  switch (format) {

#if defined(HAVE_CGNS)

  case ECS_PRE_FORMAT_CGNS:
    maillage = ecs_pre_cgns__lit_maillage(nom_fic,
                                          num_maillage,
                                          cree_grp_cel_section,
                                          cree_grp_cel_zone,
                                          cree_grp_fac_section,
                                          cree_grp_fac_zone);
    break;

#endif /* HAVE_CGNS */

#if defined(HAVE_CCM)
  case ECS_PRE_FORMAT_CCM:
    maillage = ecs_pre_ccm__lit_maillage(nom_fic,
                                         num_maillage);
    break;

#endif /* HAVE_CCM */

  case ECS_PRE_FORMAT_COMET:
    maillage = ecs_pre_comet__lit_maillage(nom_fic);
    break;

  case ECS_PRE_FORMAT_ENS:
    maillage = ecs_pre_ens__lit_maillage(nom_fic,
                                         num_maillage);
    break;

  case ECS_PRE_FORMAT_GAMBIT:
    maillage = ecs_pre_gambit__lit_maillage(nom_fic);
    break;

  case ECS_PRE_FORMAT_GMSH:
    maillage = ecs_pre_gmsh__lit_maillage(nom_fic);
    break;

  case ECS_PRE_FORMAT_IDEAS:
    maillage = ecs_pre_ideas__lit_maillage(nom_fic);
    break;

#if defined(HAVE_MED)

  case ECS_PRE_FORMAT_MED:
    maillage = ecs_pre_med__lit_maillage(nom_fic,
                                         num_maillage);
    break;

#endif /* HAVE_MED */

  case ECS_PRE_FORMAT_NOPO:
    maillage = ecs_pre_nopo__lit_maillage(nom_fic);
    break;

  default:
    ecs_error(__FILE__, __LINE__, 0,
              _("Unknown mesh file format."));

  }

  end_time[0] = ecs_timer_wtime();
  end_time[1] = ecs_timer_cpu_time();

  printf(_("\n  Wall-clock time: %f s; CPU time: %f s\n\n"),
         (double)(end_time[0] - start_time[0]),
         (double)(end_time[1] - start_time[1]));

  /* Suppression des sommets ne participant pas à la connectivité
     et fusion des éléments surfaciques confondus éventuels */

  ecs_maillage__nettoie_nodal(maillage);

  /* Retour du maillage */

  return maillage;
}

/*----------------------------------------------------------------------------*/

