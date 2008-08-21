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
 *  Gestion du module de calcul thermique en paroi 1D
 *============================================================================*/


/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C
 *----------------------------------------------------------------------------*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie BFT
 *----------------------------------------------------------------------------*/

#include <bft_mem.h>
#include <bft_error.h>
#include <bft_printf.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' locaux
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_suite.h"


/*----------------------------------------------------------------------------
 *  Fichiers  `include' associ�s au fichier courant
 *----------------------------------------------------------------------------*/

#include "cs_tpar1d.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*============================================================================
 *  Structures locales
 *============================================================================*/

struct par1d
{
  cs_int_t n;       /* Nombre de pts de discr�tisation pour la face coupl�e */
  cs_real_t *z;     /* Coordonn�es des points de discr�tisation             */
  cs_real_t e;      /* Epaisseur associ�e � la face coupl�e                 */
  cs_real_t *t;     /* Temp�rature en chacun des points de discr�tisation   */
};


/*============================================================================
 *  Variables globales statiques
 *============================================================================*/

static struct par1d *cs_glob_par1d = NULL;
static cs_suite_t   *cs_glob_tpar1d_suite = NULL;


/*============================================================================
 *  Prototype de fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Allocation de la structure cs_glob_par1d
 *----------------------------------------------------------------------------*/

static void cs_loc_tpar1d_cree
(
 const cs_int_t         nfpt1d,   /* : <-  : nombre de faces de bord couplees */
 const cs_int_t  *const nppt1d    /* : <-  : nombre de pts de discr�tisation
                                     sur chaque face coupl�e                  */
);

/*----------------------------------------------------------------------------
 * Ouverture du fichier suite associ� � cs_tpar1d
 * Allocation de cs_glob_tpar1d_suite
 *----------------------------------------------------------------------------*/

static void cs_loc_tpar1d_opnsuite
(
 const char      *const nomsui,  /* :  <-  : nom du fichier suite             */
 const cs_int_t  *const lngnom,  /* :  <-  : longueur du nom du fichier       */
 const cs_suite_mode_t  ireawr,  /* :  <-  : 1 pour lecture, 2 pour �criture  */
 const cs_int_t  *const iforma,  /* :  <-  : 0 pour binaire, 1 pour ascii     */
       cs_int_t         ierror   /* :  ->  : 0 pour succes, < 0 pour erreur   */
);


/*============================================================================
 *  Fonctions publiques pour API Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Creation des maillages de chaque face et initialisation de la temperature
 *
 * Interface Fortran :
 *
 * SUBROUTINE  MAIT1D(NFPT1D, NPPT1D, EPPT1D, RGPT1D, TPPT1D)
 * ******************
 *
 * INTEGER          NFPT1D         : <-  : nombre de faces couplees
 * INTEGER          NPPT1D(NFPT1D) : <-  : nombre de points de maillage pour chaque face
 * DOUBLE PRECISION EPPT1D(NFPT1D) : <-  : epaisseur de paroi a chaque face
 * DOUBLE PRECISION RGPT1D(NFPT1D) : <-  : raison geometrique du maillage de chaque face
 * DOUBLE PRECISION TPPT1D(NFPT1D) : <-  : valeur d'initialisation de la temperature sur
 *                                         sur tout le maillage
 *----------------------------------------------------------------------------*/

void CS_PROCF (mait1d,MAIT1D)
(
 cs_int_t *nf,
 cs_int_t *n,
 cs_real_t *e,
 cs_real_t *r,
 cs_real_t *tp
)
{
  cs_int_t i, k;
  cs_real_t m, rr;
  cs_real_t *zz;

  /* Allocation de la structure globale: cs_glob_par1d et du nombre de pts de
     discr�tisation sur chaque face */
  cs_loc_tpar1d_cree (*nf, n);

  /* Initialisation des �paisseurs e de chaque face coupl�e */
  for (i = 0 ; i < *nf ; i++ ) {
    cs_glob_par1d[i].e = e[i];
  }

  for (i = 0 ; i < *nf ; i++ ) {
    /* Initialisation de la Temperature */
    for (k = 0; k<n[i]; k++) {
      (cs_glob_par1d[i].t)[k] = tp[i];
    }

    /* Maillage */
    zz = cs_glob_par1d[i].z;
    rr = r[i];

    /* Regulier */
    if (fabs(rr-1.0) <= 1.0e-6) {
      zz[0] = e[i]/n[i]/2.;
      for (k = 1; k < n[i]; k++) {
        zz[k]=zz[k-1]+e[i]/n[i];
      }
    }

    /* Geometrique */
    else {
      m = e[i]*(1.0-rr)/(1.0-pow(rr,n[i]));
      *zz = m/2.;
      for (k = 1; k< n[i]; k++) {
        zz[k] = zz[k-1]+m/2.;
        m = m*rr;
        zz[k] = zz[k]+m/2.;
      }
    }
  }

}


/*----------------------------------------------------------------------------
 * Resolution de l'equation 1D pour une face donnee
 *
 * Interface Fortran :
 *
 * SUBROUTINE  TPAR1D
 * ******************
     &  (II, ICLT1D, TBORD, HBORD, TET1D,
     &   HET1D, FET1D, LAMT1D,
     &   RCPT1D, DTPT1D, TPPT1D)
 *
 * INTEGER          II     : <-  : numero de la face traitee
 * INTEGER          ICLT1D : <-  : type de condition a la limite exterieure
 * DOUBLE PRECISION TBORD  : <-  : temperature fluide au bord
 * DOUBLE PRECISION HBORD  : <-  : coefficient d'echange fluide au bord
 * DOUBLE PRECISION TET1D  : <-  : temperature sur le bord exterieur (CL de Dirichlet)
 * DOUBLE PRECISION HET1D  : <-  : coefficient d'echange sur la paroi exterieure
 * DOUBLE PRECISION FET1D  : <-  : flux sur la paroi exterieure (CL de flux)
 * DOUBLE PRECISION LAMT1D : <-  : valeur de la conductivite lambda
 * DOUBLE PRECISION RCPT1D : <-  : valeur du produit rho*Cp
 * DOUBLE PRECISION DTPT1D : <-> : valeur du pas de temps pour la resolution dans le solide
 * DOUBLE PRECISION TPPT1D : <-> : temperature physique a l'interface fluide/solide
 *----------------------------------------------------------------------------*/

void CS_PROCF (tpar1d,TPAR1D)
(
cs_int_t *ii,
cs_int_t *icdcle,
cs_real_t *tf,
cs_real_t *hf,
cs_real_t *te,
cs_real_t *he,
cs_real_t *fe,
cs_real_t *lb,
cs_real_t *rocp,
cs_real_t *dtf,
cs_real_t *tp
)
{
  cs_int_t k;

  cs_real_t a1; /*coefficient d'extrapolation de la temperature1*/
  cs_real_t h2; /*coefficient d'echange thermique sur T(1)*/
  cs_real_t f3; /*flux thermique sur Tfluide*/
  cs_real_t a4; /*coefficient d'extrapolation de la temperature4*/
  cs_real_t h5; /*coefficient d'echange thermique sur T(n)*/
  cs_real_t f6; /*flux thermique sur Text*/

  cs_real_t m;

  cs_real_t *al, *bl, *cl, *dl;
  cs_real_t *zz;
  cs_int_t n;

  n = cs_glob_par1d[*ii].n;

  BFT_MALLOC(al, 4*n, cs_real_t);
  bl = al+n;
  cl = bl+n;
  dl = cl+n;

  zz = cs_glob_par1d[*ii].z;

  /*construction de la matrice tridiagonale*/

  /*Conditions limites cote fluide Conservation du flux*/
  /*flux dans le fluide = flux dans le solide = f3 + h2*T1*/
  a1 = 1./(*hf)+zz[0]/(*lb);
  h2 = -1./a1;
  f3 = -h2*(*tf);

  /*Conditions limites cote exterieur*/
  /*flux dans le fluide = flux dans le solide = f6 + h5*T(n-1)*/


  /*Condition de type dirichlet */
  if (*icdcle == 1) {
    a4 = 1./(*he)+(cs_glob_par1d[*ii].e - zz[n-1])/(*lb);
    h5 = -1./a4;
    f6 = -h5*(*te);
  }
  /*Condition de type Flux impose*/
  else if (*icdcle == 3) {
    h5 = 0.;
    f6 = *fe;
  }

  /*Points internes du maillage*/
  for (k=1; k <= n-1; k++) {
    al[k] = -(*lb)/(zz[k]-zz[k-1]);
  }

  m = 2*zz[0];
  for (k=1; k <= n-2; k++) {
    m = 2*(zz[k]-zz[k-1])-m;
    bl[k] = (*rocp)/(*dtf)*m +(*lb)/(zz[k+1]-zz[k]) +(*lb)/(zz[k]-zz[k-1]);
  }

  for (k=0; k <= n-2; k++) {
    cl[k] =  -(*lb)/(zz[k+1]-zz[k]);
  }

  m = 2*zz[0];
  dl[0] = (*rocp)/(*dtf)*m*(cs_glob_par1d[*ii].t)[0];

  for (k=1; k <= n-1; k++) {
    m = 2*(zz[k]-zz[k-1])-m;
    dl[k] = (*rocp)/(*dtf)*m*(cs_glob_par1d[*ii].t)[k];
  }

  /*Points frontieres*/
  /*On initialise bl[0] et bl[n-1] et on les remplit ensuite, au cas ou 0 = n-1 !!*/
  bl[0] = 0.;
  bl[n-1] = 0.;
  al[0] = 0.;
  bl[0] = bl[0] + (*rocp)/(*dtf)*2*zz[0] + (*lb)/(zz[1]-zz[0]) - h2;
  cl[0] = cl[0] ;
  dl[0] = dl[0] +f3;
  al[n-1] = al[n-1];
  bl[n-1] = bl[n-1] + (*rocp)/(*dtf)*2*(cs_glob_par1d[*ii].e-zz[n-1]) +(*lb)/(zz[n-1]-zz[n-2]) -h5;
  cl[n-1] = 0.;
  dl[n-1] = dl[n-1] +f6;

  /*Resolution du systeme par double balayage*/
  for (k=1; k<=n-1; k++) {
    bl[k] = bl[k] -al[k]*cl[k-1]/bl[k-1];
    dl[k] = dl[k] -al[k]*dl[k-1]/bl[k-1];
  }

  cs_glob_par1d[*ii].t[n-1] = dl[n-1]/bl[n-1];

  for (k=n-2; k>=0; k-- ) {
    cs_glob_par1d[*ii].t[k] = (dl[k] -cl[k]*cs_glob_par1d[*ii].t[k+1] )/bl[k];
    }


  /*Calcul de la nouvelle valeur de tp*/
  *tp = (*hf)+(*lb)/zz[0];
  *tp = 1/(*tp)*((*lb)*cs_glob_par1d[*ii].t[0]/zz[0]+(*hf)*(*tf));

  BFT_FREE(al);

}


/*----------------------------------------------------------------------------
 * Lecture du fichier suite du module thermique 1D en paroi
 *
 * Interface Fortran :
 *
 * SUBROUTINE  LECT1D
 * *********************
     &  (NOMSUI,LNGNOM,IFOVT1,NFPT1D,NFPT1T,NMXT1D,NFABOR,TPPT1D,IFPT1D)
 *
 * CHAR             NOMSUI         : <-  : nom du fichier suite
 * INTEGER          LNGNOM         : <-  : longueur du nom du fichier
 * INTEGER          IFOVT1         : <-  : Indicateur binaire (0) / ascii (1)
 * INTEGER          NFPT1D         : <-  : nombre de faces avec couplage
 * INTEGER          NFPT1T         : <-  : nombre de faces avec couplage, cumule sur
 *                                 :     : tous les processeurs
 * INTEGER          NMXT1D         : <-  : discretisation maximale des faces
 * INTEGER          NFABOR         : <-  : nombre de faces de bord
 * INTEGER          NPPT1D(NFPT1D) : <-  : nombre de points de discretisation des
 *                                         faces couplees
 * INTEGER          IFPT1D(NFPT1D) : <-  : tableau d'indirection des faces
 *                                         couplees
 * DOUBLE PRECISION EPPT1D(NFPT1D) : <-  : epaisseur de paroi des faces couplees
 * DOUBLE PRECISION RGPT1D(NFPT1D) : <-  : raison geometrique associee aux faces couplees
 * DOUBLE PRECISION TPPT1D(NFPT1D) : <-  : valeur d'initialisation de la
 *                                         temperature sur tout le maillage

 *
 *----------------------------------------------------------------------------*/

void CS_PROCF (lect1d,LECT1D)
(
 const char       *const nomsui,  /* <- Nom du fichier suite                  */
 const cs_int_t   *const lngnom,  /* <- Longueur du nom                       */
 const cs_int_t   *const ifovt1,  /* <- Indicateur binaire (0) / ascii (1)    */
 const cs_int_t   *const nfpt1d,  /* <- Nbr de  faces avec couplage           */
 const cs_int_t   *const nfpt1t,  /* <- Nbr de  faces avec couplage cumule sur
                                        tous les processeurs                  */
 const cs_int_t   *const nmxt1d,  /* <- Nbr max de pts sur les maillages 1D   */
 const cs_int_t   *const nfabor,  /* <- Nbr de faces de bord                  */
 const cs_int_t   *const nppt1d,  /* <- Nbr de points de discretisation des
                                                faces avec module 1D                  */
 const cs_int_t   *const ifpt1d,  /* -> Tableau d'indirection des faces avec
                                        module 1D                             */
 const cs_real_t  *const eppt1d,  /* <- Epaisseur de paroi des faces          */
 const cs_real_t  *const rgpt1d,  /* <- Raison geometrique associee aux faces */
       cs_real_t  *const tppt1d   /* <- Temp�rature de paroi avec module 1D   */
 CS_ARGF_SUPP_CHAINE              /*     (arguments 'longueur' �ventuels F77, */
                                  /*     inutilis�s lors de l'appel mais      */
                                  /*     plac�s par de nombreux compilateurs) */
)
{
  cs_bool_t           corresp_cel, corresp_fac, corresp_fbr, corresp_som;
  cs_int_t            nbvent;
  cs_int_t            i, j, ifac, indfac, ierror;
  cs_int_t            version;    /* N'est pas utilis� pour l'instant */

  cs_suite_t          *suite;
  cs_suite_mode_t     suite_mode;
  cs_suite_support_t  support;
  cs_type_t           typ_val;


  ierror = CS_SUITE_SUCCES;
  suite_mode = CS_SUITE_MODE_LECTURE;

  /* Ouverture du fichier suite */
  cs_loc_tpar1d_opnsuite(nomsui,
                          lngnom,
                          suite_mode,
                          ifovt1,
                          ierror);

  if (ierror != CS_SUITE_SUCCES )
    bft_error(__FILE__, __LINE__, 0 ,
              _("Abort while opening the 1D-wall thermal module restart file "
                "in read mode.\n"
                "Verify the existence and the name of the restart file: %s\n"),
              *nomsui);


  /* Pointeur vers la structure suite globale */
  suite = cs_glob_tpar1d_suite;

  /* V�rification du support associ� au fichier suite */
  cs_suite_verif_support (suite, &corresp_cel, &corresp_fac,
                           &corresp_fbr, &corresp_som );

  /* On ne s'int�resse qu'aux faces de bord */
  indfac = (corresp_fbr == CS_TRUE ? 1 : 0 );
  if (indfac == 0 )
    bft_error(__FILE__, __LINE__, 0 ,
              _("Abort while reading the 1D-wall thermal module restart file.\n"
                "The number of boundary faces has been modified\n"
                "Verify that the restart file corresponds to "
                "the present study.\n"));


  { /* Lecture de l'en-t�te */
    char       nomrub[] = "version_fichier_suite_module_1d";
    cs_int_t   *tabvar;

    BFT_MALLOC(tabvar, 1, cs_int_t);

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_SCAL;
    typ_val = CS_TYPE_cs_int_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("WARNING: ABORT WHILE READING THE RESTART FILE\n"
                     "********               1D-WALL THERMAL MODULE\n"
                     "       INCORRECT FILE TYPE\n"
                     "\n"
                     "The file %s does not seem to be a restart file\n"
                     "for the 1D-wall thermal module.\n"
                     "The calculation will not be run.\n"
                     "\n"
                     "Verify that the restart file corresponds to a\n"
                     "restart file for the 1D-wall thermal module.\n"),
                   *nomsui);

    version = *tabvar;

    BFT_FREE( tabvar);

  }

  { /* Lecture du nombre de points de discr�tisation et test de coherence avec
       les donnees entrees dans USPT1D.
         Le test sur IFPT1D suppose que les faces ont ete reperees en ordre
       croissant (IFPT1D(II)>IFPT1D(JJ) si II>JJ). C'est normalement
       le cas (definition de IFPT1D dans une boucle sur IFAC).  */
    char       nomrub[] = "nb_pts_discretis";
    cs_int_t   *tabvar;
    cs_int_t   mfpt1d, mfpt1t;
    cs_int_t   iok;

    BFT_MALLOC(tabvar, *nfabor, cs_int_t);

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_int_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("Problem while reading section in the restart file\n"
                     "for the 1D-wall thermal module:\n"
                     "<%s>\n"
                     "The calculation will not be run.\n"), nomrub);

    /* Test de coherence entre NFPT1T relu et celui de USPT1D */
    mfpt1d = 0;
    for ( ifac = 0 ; ifac < *nfabor ; ifac++ ) {
        if ( tabvar[ifac] > 0 ) mfpt1d++;
    }
    mfpt1t = mfpt1d;
    /* si necessaire on somme sur tous les processeurs */
#if defined(_CS_HAVE_MPI)
    if ( cs_glob_base_nbr > 1 )
        MPI_Allreduce (&mfpt1d, &mfpt1t, 1, CS_MPI_INT, MPI_SUM,
                           cs_glob_base_mpi_comm);
#endif
    if ( mfpt1t != *nfpt1t )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("WARNING: ABORT WHILE READING THE RESTART FILE\n"
                     "********               1D-WALL THERMAL MODULE\n"
                     "       CURRENT AND PREVIOUS DATA ARE DIFFERENT\n"
                     "\n"
                     "The number of faces with 1D thermal module has\n"
                     "been modified.\n"
                     "PREVIOUS: %d boundary faces (total)\n"
                     "CURRENT:  %d boundary faces (total)\n"
                     "\n"
                     "The calculation will not be run.\n"
                     "\n"
                     "Verify that the restart file corresponds to a\n"
                     "restart file for the 1D-wall thermal module.\n"
                     "Verify uspt1d.\n"), mfpt1t, *nfpt1t );

    /* Test de coherence entre NFPT1D/IFPT1D relus et ceux de USPT1D */
    iok = 0;
    i = 0;
    for ( ifac = 0 ; ifac < *nfabor ; ifac++ ) {
        if ( tabvar[ifac] > 0 ) {
          if ( ifac != ifpt1d[i]-1 ) iok++;
          if ( tabvar[ifac] != nppt1d[i] ) iok++;
          i++;
        }
    }
    if ( iok > 0 )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("WARNING: ABORT WHILE READING THE RESTART FILE\n"
                     "********               1D-WALL THERMAL MODULE\n"
                     "       CURRENT AND PREVIOUS DATA ARE DIFFERENT\n"
                     "\n"
                     "IFPT1D or NPPT1D has been modified with respect\n"
                     "to the restart file on at least on face with\n"
                     "1D thermal module\n"
                     "\n"
                     "The calculation will not be run.\n"
                     "\n"
                     "Verify that the restart file correspond to\n"
                     "the present study"
                     "Verify uspt1d\n"
                     "(refer to the user manual for the specificities\n"
                     "of the test on IFPT1D)") );

    /* Allocation de la structure cs_glob_par1d */

    cs_loc_tpar1d_cree (*nfpt1d, nppt1d);

    BFT_FREE(tabvar);
  }

  { /* Lecture de l'�paisseur en paroi et test de coherence avec USPT1D*/
    char        nomrub[] = "epaisseur_paroi";
    cs_real_t   *tabvar;
    cs_int_t    iok;

    BFT_MALLOC(tabvar, *nfabor, cs_real_t);

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("Problem while reading section in the restart file\n"
                     "for the 1D-wall thermal module:\n"
                     "<%s>\n"
                     "The calculation will not be run.\n"), nomrub);

    /* Test de coherence entre EPPT1D relu et celui de USPT1D */
    iok = 0;
    for ( i = 0 ; i < *nfpt1d ; i++ ) {
      ifac = ifpt1d[i]-1;
        if ( fabs(tabvar[ifac]-eppt1d[i])/eppt1d[i] > 1.e-10 ) iok++;
    }
    if ( iok > 0 )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("WARNING: ABORT WHILE READING THE RESTART FILE\n"
                     "********               1D-WALL THERMAL MODULE\n"
                     "       CURRENT AND PREVIOUS DATA ARE DIFFERENT\n"
                     "\n"
                     "The parameter EPPT1D has been modified with respect\n"
                     "to the restart file on at least on face with\n"
                     "1D thermal module\n"
                     "\n"
                     "The calculation will not be run.\n"
                     "\n"
                     "Verify that the restart file corresponds to\n"
                     "the present study.\n"
                     "Verify uspt1d\n") );

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
            ifac = ifpt1d[i] - 1 ;
            cs_glob_par1d[i].e = tabvar[ifac];
          }

    BFT_FREE(tabvar);
  }

  { /* Lecture de la temp�rature de bord interne */
    char       nomrub[] = "temperature_bord_int";
    cs_real_t  *tabvar;

    BFT_MALLOC(tabvar, *nfabor, cs_real_t);

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("Problem while reading section in the restart file\n"
                     "for the 1D-wall thermal module:\n"
                     "<%s>\n"
                     "The calculation will not be run.\n"), nomrub);

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
            ifac = ifpt1d[i] - 1 ;
            tppt1d[i] = tabvar[ifac];
          }

    BFT_FREE(tabvar);
  }

  { /* Lecture des coordonn�es du maillage 1D */
    char        nomrub[] = "coords_maillages_1d";
    cs_int_t    nptmx;
    cs_int_t    iok;
    cs_real_t   *tabvar;
    cs_real_t   zz1, zz2, rrgpt1;

    nptmx = (*nfabor) * (*nmxt1d);
    BFT_MALLOC(tabvar, nptmx, cs_real_t);

    nbvent  = *nmxt1d;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES )
        bft_error( __FILE__, __LINE__, 0 ,
                   _("Problem while reading section in the restart file\n"
                     "for the 1D-wall thermal module:\n"
                     "<%s>\n"
                     "The calculation will not be run.\n"), nomrub);

    /* Maintenant qu'on a les centres des mailles, on peut tester RGPT1D */
    iok = 0;
    for ( i = 0 ; i < *nfpt1d ; i++ ) {
      ifac = ifpt1d[i]-1;
        if ( nppt1d[i] > 1 ) {
          zz1 = tabvar[0 + (*nmxt1d)*ifac];
          zz2 = tabvar[1 + (*nmxt1d)*ifac];
          rrgpt1 = (zz2-2.*zz1)/zz1;
          if ( fabs(rrgpt1-rgpt1d[i])/rgpt1d[i] > 1.e-10 ) iok++;
        }
    }

    if ( iok > 0 )
      bft_error(__FILE__, __LINE__, 0 ,
                _("WARNING: ABORT WHILE READING THE RESTART FILE\n"
                  "********               1D-WALL THERMAL MODULE\n"
                  "       CURRENT AND OLD DATA ARE DIFFERENT\n"
                  "\n"
                  "The parameter RGPT1D has been modified with respect\n"
                  "to the restart file on at least on face with\n"
                  "1D thermal module\n"
                  "\n"
                  "The calculation will not be run.\n"
                  "\n"
                   "Verify that the restart file correspond to\n"
                  "the present study\n"
                  "Verify uspt1d\n"));

    for (i = 0 ; i < *nfpt1d ; i++) {
      ifac = ifpt1d[i]-1;
      /* On remplit jusqu'au nombre de points de discr�tisation de
         la face coupl�e consid�r�e */
      for ( j = 0 ; j < cs_glob_par1d[i].n ; j++ )
        cs_glob_par1d[i].z[j] = tabvar[j + (*nmxt1d)*ifac];
    }

    BFT_FREE(tabvar);

  }

  { /* Lecture de la temp�rature dans la paroi */
    char        nomrub[] = "temperature_interne";
    cs_int_t    nptmx;
    cs_real_t   *tabvar;

    nptmx = (*nfabor) * (*nmxt1d);
    BFT_MALLOC(tabvar, nptmx, cs_real_t);

    nbvent  = *nmxt1d;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    ierror = cs_suite_lit_rub ( suite,
                                nomrub,
                                support,
                                nbvent,
                                typ_val,
                                tabvar);

    if ( ierror < CS_SUITE_SUCCES ) {
      cs_base_warn(__FILE__,__LINE__);
      bft_printf ( _("Problem while reading the section in the restart file\n"
                     "for the 1D-wall thermal module:\n"
                     "<%s>\n"), nomrub);
    }

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
      ifac = ifpt1d[i] - 1;

      /* On remplit jusqu'au nombre de points de discr�tisation de
         la face coupl�e consid�r�e */
      for ( j = 0 ; j < cs_glob_par1d[i].n ; j++ )
        cs_glob_par1d[i].t[j] = tabvar[j + (*nmxt1d)*ifac];

    }

    BFT_FREE(tabvar);
  }

  /* Fermeture du fichier et lib�ration des structures */
  cs_suite_detruit(cs_glob_tpar1d_suite);
  cs_glob_tpar1d_suite = NULL;

}


/*----------------------------------------------------------------------------
 * Ecriture du fichier suite du module thermique 1D en paroi
 *
 * Interface Fortran :
 *
 * SUBROUTINE  ECRT1D
 * *********************
     &  (NOMSUI,LNGNOM,IFOVT1,NFPT1D,NMXT1D,NFABOR,TPPT1D,IFPT1D)
 *
 *
 * CHAR             NOMSUI         : <-  : nom du fichier suite
 * INTEGER          LNGNOM         : <-  : longueur du nom
 * INTEGER          IFOVT1         : <-  : indicateur biunaire / ascii
 * INTEGER          NFPT1D         : <-  : nombre de faces avec couplage
 * INTEGER          NMXT1D         : <-  : discretisation maximale des faces
 * INTEGER          NFABOR         : <-  : nombre de faces de bord
 * DOUBLE PRECISION TPPT1D(NFPT1D) : <-  : valeur d'initialisation de la
 *                                         temperature sur tout le maillage
 * INTEGER          IFPT1D(NFPT1D) : <-  : tableau d'indirection des faces
 *                                         couplees
 *
 *----------------------------------------------------------------------------*/

void CS_PROCF (ecrt1d,ECRT1D)
(
 const char       *const nomsui,  /* <- Nom du fichier suite                  */
 const cs_int_t   *const lngnom,  /* <- Longueur du nom                       */
 const cs_int_t   *const ifovt1,  /* <- Indicateur binaire (0) / ascii (1)    */
 const cs_int_t   *const nfpt1d,  /* <- Nbr de  faces avec couplage           */
 const cs_int_t   *const nmxt1d,  /* <- Nbr max de pts sur les maillages 1D   */
 const cs_int_t   *const nfabor,  /* <- Nbr de faces de bord                  */
 const cs_real_t  *const tppt1d,  /* <- Temp�rature de paroi avec module 1D   */
 const cs_int_t   *const ifpt1d   /* <- Tableau d'indirection des faces avec
                                     module 1D                                */
 CS_ARGF_SUPP_CHAINE              /*     (arguments 'longueur' �ventuels F77, */
                                  /*     inutilis�s lors de l'appel mais      */
                                  /*     plac�s par de nombreux compilateurs) */
)
{
  cs_int_t            nbvent, ierror;
  cs_int_t            i, j, ifac;

  cs_suite_t          *suite;
  cs_suite_support_t  support;
  cs_suite_mode_t     suite_mode;
  cs_type_t           typ_val;


  ierror = CS_SUITE_SUCCES;
  suite_mode = CS_SUITE_MODE_ECRITURE;

  /* Ouverture du fichier suite */
  cs_loc_tpar1d_opnsuite( nomsui,
                          lngnom,
                          suite_mode,
                          ifovt1,
                          ierror);

  if ( ierror != CS_SUITE_SUCCES )
    bft_error( __FILE__, __LINE__, 0 ,
               _("Abort while opening the 1D-wall thermal module restart "
                 "file in write mode.\n"
                 "Verify the existence and the name of the restart file: %s\n"),
               *nomsui);


  /* Pointeur vers la structure suite globale */
  suite = cs_glob_tpar1d_suite;

  { /* Ecriture de l'en-t�te */
    char       nomrub[] = "version_fichier_suite_module_1d";
    cs_int_t   *tabvar;

    BFT_MALLOC(tabvar, 1, cs_int_t);

    *tabvar = 120;

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_SCAL;
    typ_val = CS_TYPE_cs_int_t;

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  { /* Ecriture du nombre de points de discr�tisation */
    char       nomrub[] = "nb_pts_discretis";
    cs_int_t   *tabvar;

    BFT_MALLOC(tabvar, *nfabor, cs_int_t);

    for ( i = 0 ; i < *nfabor ; i++ )
      tabvar[i] = 0;

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_int_t;

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
            ifac = ifpt1d[i] - 1 ;
            tabvar[ifac] = cs_glob_par1d[i].n;
          }

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  { /* Ecriture de l'�paisseur en paroi */
    char        nomrub[] = "epaisseur_paroi";
    cs_real_t   *tabvar;

    BFT_MALLOC(tabvar, *nfabor, cs_real_t);

    for (i = 0 ; i < *nfabor ; i++ )
      tabvar[i] = 0.0;

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
            ifac = ifpt1d[i] - 1 ;
            tabvar[ifac] = cs_glob_par1d[i].e;
          }

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  { /* Ecriture de la temp�rature de bord interne */
    char       nomrub[] = "temperature_bord_int";
    cs_real_t  *tabvar;

    BFT_MALLOC(tabvar, *nfabor, cs_real_t);

    for (i = 0 ; i < *nfabor ; i++ )
      tabvar[i] = 0.0;

    nbvent  = 1;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
            ifac = ifpt1d[i] - 1 ;
            tabvar[ifac] = tppt1d[i];
          }

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  { /* Ecriture des coordonn�es du maillage 1D */
    char        nomrub[] = "coords_maillages_1d";
    cs_int_t    nptmx;
    cs_real_t   *tabvar;

    nptmx = (*nfabor) * (*nmxt1d);
    BFT_MALLOC(tabvar, nptmx, cs_real_t);

    for (i = 0 ; i < nptmx ; i++ )
      tabvar[i] = 0.0;

    nbvent  = *nmxt1d;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
      ifac = ifpt1d[i] - 1;

      /* On remplit jusqu'au nombre de points de discr�tisation de
         la face coupl�e consid�r�e (les cases suivantes jusqu'a nmxt1d
         contiennent deja 0 de par l'initialisation de tabvar */
      for ( j = 0 ; j < cs_glob_par1d[i].n ; j++ )
        tabvar[j + (*nmxt1d)*ifac] = cs_glob_par1d[i].z[j];
    }

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  { /* Ecriture de la temp�rature dans la paroi */
    char        nomrub[] = "temperature_interne";
    cs_int_t    nptmx;
    cs_real_t   *tabvar;

    nptmx = (*nfabor) * (*nmxt1d);
    BFT_MALLOC(tabvar, nptmx, cs_real_t);

    for (i = 0 ; i < nptmx ; i++ )
      tabvar[i] = 0.0;

    nbvent  = *nmxt1d;
    support = CS_SUITE_SUPPORT_FAC_BRD;
    typ_val = CS_TYPE_cs_real_t;

    for ( i = 0 ; i < *nfpt1d ; i++ ) {
      ifac = ifpt1d[i] - 1;

      /* On remplit jusqu'au nombre de points de discr�tisation de
         la face coupl�e consid�r�e (les cases suivantes jusqu'a nmxt1d
         contiennent deja 0 de par l'initialisation de tabvar */
      for ( j = 0 ; j < cs_glob_par1d[i].n ; j++ )
        tabvar[j + (*nmxt1d)*ifac] = cs_glob_par1d[i].t[j];

    }

    cs_suite_ecr_rub ( suite,
                       nomrub,
                       support,
                       nbvent,
                       typ_val,
                       tabvar);

    BFT_FREE(tabvar);
  }

  /* Fermeture du fichier et lib�ration des structures */
  cs_suite_detruit(cs_glob_tpar1d_suite);
  cs_glob_tpar1d_suite = NULL;

}


/*----------------------------------------------------------------------------
 * Liberation de la memoire
 *
 * Interface Fortran :
 *
 * SUBROUTINE  LBRT1D ()
 * ******************
 *
 *----------------------------------------------------------------------------*/

void CS_PROCF (lbrt1d,LBRT1D)(void)
{
  BFT_FREE(cs_glob_par1d->z);
  BFT_FREE(cs_glob_par1d);
}


/*============================================================================
 *  Fonctions priv�es
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Allocation de la structure cs_glob_par1d
 *----------------------------------------------------------------------------*/

static void cs_loc_tpar1d_cree
(
 const cs_int_t         nfpt1d,   /* : <-  : nombre de faces de bord couplees */
 const cs_int_t  *const nppt1d    /* : <-  : nombre de pts de discr�tisation
                                     sur chaque face coupl�e                  */
)
{
  cs_int_t   nb_pts_tot;
  cs_int_t   i;

  /* Allocation de la structure cs_glob_par1d */
  BFT_MALLOC(cs_glob_par1d, nfpt1d, struct par1d);

  /* Initialisation du nombre de pts de discr�tisation dans chaque structure
     Calcul du nbr total de pts de discr�tisation */
  nb_pts_tot = 0;

  for ( i = 0 ; i < nfpt1d ; i++ ) {
    cs_glob_par1d[i].n = nppt1d[i];
    nb_pts_tot += nppt1d[i];
  }

  /* Allocation des tableaux t: Temp�rature en chaque pts de discr�tisation
     et z: Coordonn�e de chaque pts de discr�tisation */

  BFT_MALLOC(cs_glob_par1d->z, 2 * nb_pts_tot, cs_real_t);
  cs_glob_par1d->t = cs_glob_par1d->z + nb_pts_tot;

  for ( i = 1 ; i < nfpt1d ; i++ ) {
    cs_glob_par1d[i].z = cs_glob_par1d[i-1].z + nppt1d[i-1];
    cs_glob_par1d[i].t = cs_glob_par1d[i-1].t + nppt1d[i-1];
  }

}

/*----------------------------------------------------------------------------
 * Ouverture du fichier suite associ� � cs_tpar1d
 * Allocation de cs_glob_tpar1d_suite
 *----------------------------------------------------------------------------*/

static void cs_loc_tpar1d_opnsuite
(
 const char      *const nomsui,  /* :  <-  : nom du fichier suite             */
 const cs_int_t  *const lngnom,  /* :  <-  : longueur du nom du fichier       */
 const cs_suite_mode_t  ireawr,  /* :  <-  : 1 pour lecture, 2 pour �criture  */
 const cs_int_t  *const iforma,  /* :  <-  : 0 pour binaire, 1 pour ascii     */
       cs_int_t         ierror   /* :  ->  : 0 pour succes, < 0 pour erreur   */
)
{
  char            *nombuf;

  cs_suite_type_t  suite_type;


  ierror = CS_SUITE_SUCCES;

  /* Traitement du nom pour l'API C */
  nombuf = cs_base_chaine_f_vers_c_cree( nomsui,
                                         *lngnom);

  /* Option de cr�ation du fichier */
  switch (*iforma) {
  case 0:
    suite_type = CS_SUITE_TYPE_BINAIRE;
    break;
  case 1:
    suite_type = CS_SUITE_TYPE_ASCII;
    break;
  default:
    cs_base_warn (__FILE__, __LINE__);
    bft_printf ( _("The type of the restart file <%s>\n"
                   "must be equal to 0 (binary) or 1 (formatted) and not <%d>\n"
                   "(default is binary)."),
                 nombuf, (int)(*iforma));

    ierror = CS_SUITE_ERR_TYPE_FIC;
  }

  if (ierror == CS_SUITE_SUCCES)
    cs_glob_tpar1d_suite = cs_suite_cree( nombuf,
                                          ireawr,
                                          suite_type);

  /* Lib�ration de m�moire si n�cessaire */
  nombuf = cs_base_chaine_f_vers_c_detruit(nombuf);

}


#ifdef __cplusplus
}
#endif /* __cplusplus */
