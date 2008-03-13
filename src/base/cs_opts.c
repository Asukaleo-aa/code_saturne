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
 * Parsing of program arguments and associated initializations
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_CS_HAVE_MPI)
#include <mpi.h>
#endif

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include <bft_config.h>
#include <bft_mem.h>
#include <bft_printf.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_comm.h"

#include "cs_couplage.h"
#include "cs_syr_coupling.h"

#if defined(_CS_HAVE_XML)
#include "cs_gui.h"
#include "cs_gui_util.h"
#endif

#include "cs_pp_io.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_opts.h"

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#if 0
} /* Fake brace to force Emacs auto-indentation back to column 0 */
#endif
#endif /* __cplusplus */

/*=============================================================================
 * Local Macro Definitions
 *============================================================================*/

/*=============================================================================
 * Local Structure Definitions
 *============================================================================*/

/*============================================================================
 * Static global variables
 *============================================================================*/

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*============================================================================
 * Public function definitions for Fortran API
 *============================================================================*/

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Print command line help
 *
 * parameters:
 *   name  --> name of executable program
 *----------------------------------------------------------------------------*/

static void
_arg_env_help(const char  *name)
{
  FILE *e = stderr;

  if (cs_glob_base_rang >= 1)
    return;

  fprintf (e, _("Utilisation : %s [options]\n"), name);

  fprintf (e, "\nOptions de la ligne de commandes :\n\n");
  fprintf
    (e, _(" -ec, --echo-comm  echo des donn�es issues du Pr�processeur ou\n"
          "                   des communications avec Syrthes ;\n"
          "                   -1 : erreur seulement (d�faut)\n"
          "                    0 : impression des ent�tes des messages\n"
          "                    n : impression des ent�tes des messages ainsi\n"
          "                        que des n premiers et derniers �l�ments\n"));

  fprintf
    (e, _(" -solcom           Noyau autonome avec maillage \"geomet\" au\n"
          "                   format SolCom (obsol�te) ;\n"));
  fprintf
    (e, _(" -iasize           taille du tableau de travail entier IA ;\n"
          "                    n : nombre d'entiers (d�faut : automatique)\n"
          " -rasize           taille du tableau de travail r�el RA ;\n"
          "                    n : nombre de r�els (defaut : automatique)\n"));

#if defined(_CS_HAVE_MPI)
  fprintf
    (e, _(" -p, --parallel    activation du parall�lisme ;\n"
          "                    [i] n : rang MPI global du 1er processus\n"
          "                            noyau (d�faut : 0) et nombre\n"
          "                            de processus noyau\n"));
  fprintf
    (e, _(" --coupl-cs        couplage avec une autre instance du code\n"
          "                     i : rang MPI global du 1er processus\n"
          "                         coupl�\n"));
#endif
  fprintf
    (e, _(" -syrthes          couplage Syrthes aux faces s�lectionn�es\n"
          "                   -2d : le maillage Syrthes associ� est 2D\n"
          "                   -X : axe pr�f�rentiel de projection\n"
          "                        pour un maillage Syrthes 2D \n"
          "                   -Y : axe pr�f�rentiel de projection\n"
          "                        pour un maillage Syrthes 2D \n"
          "                   -Z : axe pr�f�rentiel de projection\n"
          "                        pour un maillage Syrthes 2D (d�faut)\n"));
#if defined(_CS_HAVE_MPI)
  fprintf
    (e, _("                   -proc <num�ro> : rang MPI du processus Syrthes"
          "\n"));
#endif
#if defined(_CS_HAVE_SOCKET)
  fprintf
    (e, _("                   -socket : communication par socket IP\n"
          "                             (tubes nomm�s par d�faut)\n"));
#endif
  fprintf
    (e, _("                   -color <num�ro(s)> : couleurs des faces\n"
          "                                        � s�lectionner\n"
          "                   -group <nom(s)>    : groupes des faces\n"
          "                                        � s�lectionner\n"
          "                   -invsel : inversion de la s�lection\n"));
  fprintf
    (e, _(" -q, --quality     v�rifications\n"
          "                   -1 : pas de tests activ�s (d�faut)\n"
          "                    0 : verification de l'initialisation seule\n"
          "                    1 � 5 : activation de tests �l�mentaires\n"));
  fprintf
    (e, _(" -cwf              <critere> d�coupage des faces gauches\n"
          "                   (cut warped faces)\n"));
  fprintf
    (e, _(" --benchmark       performance des op�rations �l�mentaires\n"
          "                   [--mpitrace] op�rations effectu�es une seule\n"
          "                                fois pour traces MPI l�g�res\n"));
  fprintf
    (e, _(" --log             redirection des sorties de rang -1 ou 0 :\n"
          "                     0 : sortie standard\n"
          "                     1 : sorties dans \"listing\" (d�faut)\n"));
  fprintf
    (e, _(" --logp            redirection des sorties de rang > 0 :\n"
          "                    -1 : sorties supprimees (d�faut)\n"
          "                     0 : aucune redirection (si terminaux\n"
          "                         ind�pendants, type deboggeur)\n"
          "                     1 : sorties dans \"listing_n<rang>\"\n"));

#if defined(_CS_HAVE_XML)
  fprintf
    (e, _(" -param            [nom_du_fichier] fichier de param�tres\n"));
#endif

  fprintf
    (e, _(" -h, --help        appel de l'aide (cet affichage)\n\n"));
}

/*----------------------------------------------------------------------------
 * Convert an argument to an integer and check its validity
 *
 * parameters:
 *   arg_id  --> index of argument in argv
 *   argc    --> number of command line arguments
 *   argv    --> array of command line arguments
 *   argerr  <-- error indicator
 *
 * returns:
 *   integer value
 *----------------------------------------------------------------------------*/

static int
_arg_to_int(int    arg_id,
            int    argc,
            char  *argv[],
            int   *argerr)
{
  char  *start = NULL;
  char  *end = NULL;
  int  retval = 0;

  *argerr = 0;

  if (arg_id < argc) {
    start = argv[arg_id];
    end = start + strlen(start);
    retval = strtol(start, &end, 0);
    if (end != start + strlen(start)) *argerr = 1;
  }
  else {
    *argerr = 1;
  }

  return retval;
}

/*----------------------------------------------------------------------------
 * Convert an argument to a double and check its validity
 *
 * parameters:
 *   arg_id  --> index of argument in argv
 *   argc    --> number of command line arguments
 *   argv    --> array of command line arguments
 *   argerr  <-- error indicator
 *
 * returns:
 *   integer value
 *----------------------------------------------------------------------------*/

static double
_arg_to_double(int    arg_id,
               int    argc,
               char  *argv[],
               int   *argerr)
{
  char  *start = NULL;
  char  *end =  NULL;
  double  retval = 0.;

  *argerr = 0;

  if (arg_id < argc) {
    start = argv[arg_id];
    end = start + strlen(start);
    retval = strtod(start, &end);
    if (end != start + strlen(start)) *argerr = 1;
  }
  else {
    *argerr = 1;
  }

  return retval;
}

#if defined(_CS_HAVE_MPI)

/*----------------------------------------------------------------------------
 * First analysis of the command line to determine if Syrthes coupling
 * requires MPI
 *
 * parameters:
 *   arg_id  <-> index of argument in argv
 *   argc    --> number of command line arguments
 *   argv    --> array of command line arguments
 *
 * returns:
 *   -1 if MPI is not needed, or rank of Syrthes process in MPI_COMM_WORLD
 *----------------------------------------------------------------------------*/

static int
_syr_mpi_rank(int   *arg_id,
              int    argc,
              char  *argv[])
{
  /* local variables */

  int  ii;

  const char  *s = NULL;
  cs_bool_t    is_end = CS_FALSE;
  int          syr_rank = -1;
  int          tmperr = -1;

  for (ii = *arg_id; ii < argc && is_end == CS_FALSE; ii++) {
    s = argv[ii];

    if (strcmp(s, "-2d") == 0 || strcmp(s, "-invsel") == 0)
      continue;
    else if (strcmp(s, "-socket") == 0)
      continue;
    else if (strcmp(s, "-proc") == 0) {
      is_end = CS_TRUE;
      syr_rank = _arg_to_int(ii + 1, argc, argv, &tmperr);
      if (tmperr == 0) {
        ii++;
      }
    }
    else {
      /* Check if the current args define face selection options;
         otherwise, we have reached the end of the Syrthes options */

      if (strcmp(s,"-color") == 0 || strcmp(s,"-group") == 0) {
        while (ii + 1 < argc && strncmp(argv[ii + 1], "-", 1))
          ii++;
      }
      else {
        is_end = CS_TRUE;
      }
    }

  } /* End of loop on Syrthes related arguments */

  if (is_end == CS_TRUE)
    *arg_id = ii - 2;

  return syr_rank;
}

#endif /* defined(_CS_HAVE_MPI) */

/*----------------------------------------------------------------------------
 * First analysis of a Syrthes sub-command line before calling
 * cs_coupl_syr_lit_cmd() to process
 *
 * parameters:
 *   arg_id  <-> index of argument in argv
 *   argc    --> number of command line arguments
 *   argv    --> array of command line arguments
 *   argerr  <-- error indicator
 *----------------------------------------------------------------------------*/

static void
_syr_read_args(int   *arg_id,
               int    argc,
               char  *argv[],
               int   *argerr)
{
  cs_int_t ii, ii_save, arg_id_first;

  const char  *s = NULL;
  cs_bool_t is_end = CS_FALSE;

  /* Parameters with defaults */

  cs_bool_t invsel = CS_FALSE;
  cs_int_t  dim = 3;
  cs_int_t  axis_id = 2;
  cs_int_t  n_colors = 0;
  cs_int_t *colors = NULL;
  cs_int_t  n_groups = 0;
  char    **groups = NULL;
  cs_comm_type_t  comm_type = CS_COMM_TYPE_BINAIRE;
#if defined (_CS_HAVE_MPI)
  cs_int_t  syr_proc_rank = -1;
#endif

  const char missing_arg_fmt[]
    = N_("Erreur dans la sp�cification de la ligne de commande.\n\n"
         "L'option \"%s\" n�cessite un argument au moins.");

  arg_id_first = *arg_id;

  /* Loop on options associated to a Syrthes coupling */

  for (ii = *arg_id; ii < argc && is_end == CS_FALSE; ii++) {

    s = argv[ii];

    if (strcmp(s, "-2d") == 0)
      dim = 2;
    else if (strcmp(s, "-X") == 0)
      axis_id = 0;
    else if (strcmp(s, "-Y") == 0)
      axis_id = 1;
    else if (strcmp(s, "-Z") == 0)
      axis_id = 2;
    else if (strcmp(s, "-invsel") == 0)
      invsel = CS_TRUE;
#if defined(_CS_HAVE_SOCKET)
    else if (strcmp(s, "-socket") == 0) {
      comm_type = CS_COMM_TYPE_SOCKET;
      cs_comm_init_socket();
    }
#endif
#if defined (_CS_HAVE_MPI)
    else if (strcmp(s, "-proc") == 0) {
      comm_type = CS_COMM_TYPE_MPI;
      if (ii < argc - 1 && strncmp(argv[ii + 1], "-", 1))
        syr_proc_rank = atoi(argv[++ii]);
    }
#endif
    else if (strcmp(s, "-color") == 0) {
      ii_save = ii;

      while (ii + 1 < argc && strncmp(argv[ii + 1], "-", 1)) {
        ii++;
        BFT_REALLOC(colors, n_colors + 1, cs_int_t);
        colors[n_colors] = atoi(argv[ii]);
        n_colors++;
      }

      /* Check that at least one color has been defined */
      if (ii_save == ii) {
        _arg_env_help(argv[0]);
        bft_error(__FILE__, __LINE__, 0, missing_arg_fmt, s);
      }
    }
    else if (strcmp(s, "-group") == 0) {

      ii_save = ii;

      while (ii + 1 < argc && strncmp(argv[ii + 1], "-", 1)) {
        ii++;
        BFT_REALLOC(groups, n_groups + 1, char*);
        BFT_MALLOC(groups[n_groups], strlen(argv[ii]) + 1, char);
        strcpy(groups[n_groups],argv[ii]);
        n_groups++;
      }

      /* Check that at least one group has been defined */
      if (ii_save == ii) {
        _arg_env_help(argv[0]);
        bft_error(__FILE__, __LINE__, 0, missing_arg_fmt, s);
      }
    }
    else
      is_end = CS_TRUE;

  } /* End of loop on options associated to a Syrthes coupling */

  if (is_end == CS_TRUE)
    *arg_id = ii - 2;
  else
    *arg_id = --ii;

  if (*arg_id <= arg_id_first)
    *argerr = 1;

  if (*argerr == 0)
    cs_syr_coupling_add(dim,
                        axis_id,
                        invsel,
                        n_colors,
                        colors,
                        n_groups,
                        groups,
#if defined (_CS_HAVE_MPI)
                        syr_proc_rank,
#endif
                        comm_type);

  /* Free temporary memory */

  BFT_FREE(colors);
  for (ii = 0; ii < n_groups; ii++)
    BFT_FREE(groups[ii]);
  BFT_FREE(groups);
}

/*============================================================================
 * Public function definitions for Fortran API
 *============================================================================*/

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Print logfile header
 *
 * parameters:
 *   argc  --> number of command line arguments
 *   argv  --> array of command line arguments
 *----------------------------------------------------------------------------*/

void
cs_opts_logfile_head(int    argc,
                     char  *argv[])
{
  char str[81];
  int ii;
  char date_str[] = __DATE__;
  char time_str[] = __TIME__;
  const char mon_name[12][4]
    = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  struct tm time_cnv;

  /* Define MPI Information */

#if defined(MPI_VERSION) && defined(MPI_SUBVERSION)
#if defined(OPEN_MPI)
  const char mpi_lib[] = "Open MPI";
#elif defined(MPICH2)
  const char mpi_lib[] = "MPICH2";
#elif defined(LAM_MPI)
  const char mpi_lib[] = "LAM/MPI";
#elif defined(MPICH_NAME)
  const char mpi_lib[] = "MPICH";
#elif defined(HP_MPI)
  const char mpi_lib[] = "HP-MPI";
#elif defined(MPI_VERSION) && defined(MPI_SUBVERSION)
  const char *mpi_lib = NULL;
#endif
#endif /* defined(MPI_VERSION) && defined(MPI_SUBVERSION) */

  /* Determine compilation date */

  for (ii = 0; ii < 12; ii++) {
    if (strncmp(date_str, mon_name[ii], 3) == 0) {
      time_cnv.tm_mon = ii ;
      break;
    }
  }

  sscanf(date_str + 3, "%d", &(time_cnv.tm_mday)) ;
  sscanf(date_str + 6, "%d", &(time_cnv.tm_year)) ;

  time_cnv.tm_year -= 1900 ;

  sscanf(time_str    , "%d", &(time_cnv.tm_hour)) ;
  sscanf(time_str + 3, "%d", &(time_cnv.tm_min)) ;
  sscanf(time_str + 6, "%d", &(time_cnv.tm_sec)) ;

  time_cnv.tm_isdst = -1 ;

  /* Re-compute and internationalize build date */

  mktime(&time_cnv) ;
  strftime(str, 80, "%c", &time_cnv) ;

  /* Now print info */

  bft_printf(_("commande : \n"));

  for (ii = 0 ; ii < argc ; ii++)
    bft_printf(" %s", argv[ii]);

  bft_printf(_("\n"));
  bft_printf(_("\n************************************"
                   "***************************\n\n"));
  bft_printf(_("                                  (R)\n"
                   "                      Code_Saturne\n\n"
                   "                      Version 1.3.2\n\n"));

  bft_printf(_("\n  Copyright (C) 1998-2008 EDF S.A., France\n\n"));


  bft_printf(_("  build %s\n"), str);

#if defined(MPI_VERSION) && defined(MPI_SUBVERSION)
  if (mpi_lib != NULL)
    bft_printf(_("  MPI version %d.%d (%s)\n\n"),
               MPI_VERSION, MPI_SUBVERSION, mpi_lib);
  else
    bft_printf(_("  MPI version %d.%d\n\n"),
               MPI_VERSION, MPI_SUBVERSION);
#endif

  bft_printf(_("\n"));
  bft_printf(_("  The Code_Saturne CFD tool  is free software;\n"
               "  you can redistribute it and/or modify it under the terms\n"
               "  of the GNU General Public License as published by the\n"
               "  Free Software Foundation; either version 2 of the License,\n"
               "  or (at your option) any later version.\n\n"));

  bft_printf(_("  The Code_Saturne CFD tool is distributed in the hope that\n"
               "  it will be useful, but WITHOUT ANY WARRANTY; without even\n"
               "  the implied warranty of MERCHANTABILITY or FITNESS FOR A\n"
               "  PARTICULAR PURPOSE.  See the GNU General Public License\n"
               "  for more details.\n"));

  bft_printf(_("\n************************************"
               "***************************\n\n"));
}

/*----------------------------------------------------------------------------
 * First analysis of the command line to determine if we require MPI
 *
 * parameters:
 *   argc  <-> number of command line arguments
 *   argv  <-> array of command line arguments
 *
 * returns:
 *   -1 if MPI is not needed, or rank in MPI_COMM_WORLD of the first
 *   process associated with this instance of Code_Saturne
 *----------------------------------------------------------------------------*/

int
cs_opts_mpi_rank(int    * argc,
                 char  **argv[])
{
  char    *s;
  int     arg_id, argerr;

  int  n_ranks = 1;
  int  root_rank = -1;
  int  syr_rank = -1;
  int  syr_rank_max = -1;

  cs_bool_t  use_mpi = CS_FALSE;

  arg_id = 0, argerr = 0;

#if defined(MPICH_NAME)

  /*
    Using standard MPICH1 1.2.x with the p4 (default) mechanism,
    the information required by MPI_Init() are transferred through
    the commande line, which is then modified by MPI_Init();
    in this case, only rank 0 knows the "user" command line arguments
    at program startup, the other processes obtaining them only upon
    calling  MPI_Init(). In this case, it is thus necessary to initialize
    MPI before parsing the the command line.
  */

  for (arg_id = 0 ; arg_id < *argc ; arg_id++) {

    if (   !strcmp((*argv)[arg_id], "-p4pg")         /* For process 0 */
        || !strcmp((*argv)[arg_id], "-p4rmrank")) {  /* For other processes */

      MPI_Init(argc, argv);
      break;
    }
  }

#endif

  /* Loop on command line arguments */

  arg_id = 0;

  while (++arg_id < *argc) {

    s = (*argv)[arg_id];

    /* Parallel run */

    if (strcmp(s, "-p") == 0 || strcmp(s, "--parallel") == 0) {
      cs_int_t n1 = 0, n2 = 0;
      cs_int_t tmperr = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, *argc, *argv, &argerr);
      n_ranks = n1;
      if (argerr == 0)
        n2 = (cs_int_t) _arg_to_int(arg_id + 1, *argc, *argv, &tmperr);
      if (tmperr == 0) {
        arg_id++;
        if (n2 > 0) {
          root_rank = n1;
          n_ranks = n2;
        }
      }
      else
        n_ranks = n1;
      if (n_ranks > 1)
        use_mpi = CS_TRUE;
    }

    /* Coupling */

    else if (strcmp (s, "--coupl-cs") == 0) {
      cs_int_t n1 = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, *argc, *argv, &argerr);
      if (argerr == 0)
        use_mpi = CS_TRUE;
    }

    /* Syrthes coupling */

    else if (strcmp(s, "-syrthes") == 0) {
      arg_id++;
#if defined(_CS_HAVE_MPI)
      syr_rank = _syr_mpi_rank(&arg_id,
                               *argc,
                               *argv);
#endif


      if (syr_rank > -1) {
        use_mpi = CS_TRUE;
        syr_rank_max = CS_MAX(syr_rank_max, syr_rank);
      }
    }

  } /* End of loop on command line arguments */

  /*
    Return -1 if MPI is not needed, or rank in MPI_COMM_WORLD of
    the first process associated with this instance of Code_Saturne
    if MPI is needed
  */

  if (use_mpi == CS_TRUE) {

    if (syr_rank_max > -1) {
      if (root_rank == -1)
        root_rank = syr_rank_max + 1;
    }
    if (root_rank == -1)
      root_rank = 0;

  }

  return root_rank;
}

/*----------------------------------------------------------------------------
 * Define options and call some associated initializations
 * based on command line arguments
 *
 * parameters:
 *   argc  --> number of command line arguments
 *   argv  --> array of command line arguments
 *   opts  <-- options structure
 *----------------------------------------------------------------------------*/

void
cs_opts_define(int         argc,
               char       *argv[],
               cs_opts_t  *opts)
{
  /* Local variables */

  char  *s;
  int    arg_id = 0, argerr = 0;

  /* Default initialization */

  opts->ifoenv = 1;
  opts->echo_comm = -1;

  opts->longia =  0;
  opts->longra =  0;

  opts->ilisr0 = 1;
  opts->ilisrp = 2;

  opts->iverif = -1;
  opts->benchmark = 0;

  opts->cwf = CS_FALSE;
  opts->cwf_criterion = 0.01;

  /* Parse command line arguments */

  while (++arg_id < argc && argerr == 0) {

    s = argv[arg_id];

    if (strcmp(s, "-solcom") == 0)
      opts->ifoenv = 0;

    else if (strcmp(s, "-ec") == 0 || strcmp(s, "--echo-comm") == 0)
      opts->echo_comm = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);

    else if (strcmp(s, "-iasize") == 0)
      opts->longia = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);
    else if (strcmp(s, "-rasize") == 0)
      opts->longra = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);

#if defined(_CS_HAVE_MPI)

    else if (strcmp(s, "-p") == 0 || strcmp(s, "--parallel") == 0) {
      cs_int_t n1 = 0, n2 = 0;
      cs_int_t tmperr = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);
      if (argerr == 0)
        n2 = (cs_int_t) _arg_to_int(arg_id + 1, argc, argv, &tmperr);
      if (tmperr == 0) {
        arg_id++;
      }
    }

    else if (strcmp(s, "--coupl-cs") == 0) {
      cs_int_t n1 = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);
      if (argerr == 0)
        cs_couplage_ajoute(n1);
    }

#endif /* defined(_CS_HAVE_MPI) */
    else if (strcmp(s, "-syrthes") == 0) {
      arg_id++;
      _syr_read_args(&arg_id, argc, argv, &argerr);
    }

    else if (strcmp(s, "-q") == 0 || strcmp(s, "--quality") == 0) {
      cs_int_t tmperr = 0;
      opts->iverif = (cs_int_t) _arg_to_int(arg_id + 1, argc, argv, &tmperr);
      if (tmperr == 0)
        arg_id++;
    }

    else if (strcmp(s, "--log") == 0) {
      cs_int_t n1 = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);
      if (n1 == 0)
        opts->ilisr0 = 0;
      else if (n1 == 1)
        opts->ilisr0 = 1;
      else
        argerr = 1;
    }

    else if (strcmp(s, "--logp") == 0) {
      cs_int_t n1 = 0;
      n1 = (cs_int_t) _arg_to_int(++arg_id, argc, argv, &argerr);
      if (n1 == -1)
        opts->ilisrp = 2;
      else if (n1 == 0)
        opts->ilisrp = 0;
      else if (n1 == 1)
        opts->ilisrp = 1;
      else
        argerr = 1;
    }

    else if (strcmp(s, "--benchmark") == 0) {
      opts->benchmark = 1;
      if (arg_id + 1 < argc) {
        if (strcmp(argv[arg_id + 1], "--mpitrace") == 0) {
          opts->benchmark = 2;
          arg_id++;
        }
      }
    }

#if defined(_CS_HAVE_XML)
    else if (strcmp(s, "-param") == 0) {
      s = argv[++arg_id];
      argerr = cs_gui_file_loading(s);
    }
#endif

    else if (strcmp(s, "-cwf") == 0) {
      opts->cwf = CS_TRUE;
      if (arg_id + 1 < argc) {
        if (*(argv[arg_id+1]) != '-') {
          opts->cwf_criterion = _arg_to_double(arg_id + 1, argc, argv, &argerr);
          if (argerr == 0)
            arg_id++;
        }
      }
    }

    else if (strcmp(s, "-h") == 0 || strcmp(s, "--help") == 0)
      argerr = 2;
    else
      argerr = 1;

  } /* End parsing command line */

  /* End initialization (sanity check) */
  if (opts->echo_comm < -1) argerr = 1;
  if (opts->longia <  0 || opts->longra < 0) argerr = 1;

  /* Print help and exit if required or in case of command line error */
  if (argerr != 0) {
    cs_opts_logfile_head(argc, argv);
    _arg_env_help(argv[0]) ;
    if (argerr == 2)
      cs_exit(EXIT_SUCCESS);
    else
      cs_exit(EXIT_FAILURE);
  }
}
