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

#ifndef __CS_POST_H__
#define __CS_POST_H__

/*============================================================================
 * D�finitions, variables globales, et fonctions associ�es au post traitement
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 *  Fichiers `include` librairies BFT et FVM
 *----------------------------------------------------------------------------*/

#include <fvm_nodal.h>
#include <fvm_writer.h>

/*----------------------------------------------------------------------------
 *  Fichiers `include' locaux
 *----------------------------------------------------------------------------*/

#include "cs_base.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================
 * D�finitions d'�numerations
 *============================================================================*/

/* �num�ration pour transmettre le type d'une donn�e */

typedef enum {
  CS_POST_TYPE_cs_int_t,
  CS_POST_TYPE_cs_real_t,
  CS_POST_TYPE_int,
  CS_POST_TYPE_float,
  CS_POST_TYPE_double
} cs_post_type_t;


/*============================================================================
 * D�finition de macros
 *============================================================================*/


/*============================================================================
 * D�claration de structures et types
 *============================================================================*/

/* Pointeur associ� � un "writer" : cet objet correspond au choix d'un
 * nom de cas, de r�pertoire, et de format, ainsi qu'un indicateur pr�cisant
 * si les maillages associ�s doivent d�pendre ou non du temps, et la
 * fr�quence de sortie par d�faut pour les variables associ�es. */

typedef struct _cs_post_writer_t cs_post_writer_t;

/* Pointeur associ� � un maillage de post traitement ; cet objet
 * g�re le lien entre un tel maillage et les "writers" associ�s. */

typedef struct _cs_post_maillage_t cs_post_maillage_t;

/* Pointeur de fonction associ� � un post-traitement particulier ;
 * on enregistre de telles fonctions via la fonction
 * cs_post_ajoute_var_temporelle(), et toutes les fonctions enregistr�es
 * de la sorte sont appell�es automatiquement par PSTVAR. */

typedef void
(cs_post_var_temporelle_t) (cs_int_t     id_instance,
                            cs_int_t     nt_cur_abs,
                            cs_real_t    t_cur_abs);


/*============================================================================
 * Fonctions publiques pour API Fortran
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Cr�ation d'un "writer" � partir des donn�es du Fortran ; cet objet
 * correspond au choix d'un nom de cas, de r�pertoire, et de format, ainsi
 * qu'un indicateur pr�cisant si les maillages associ�s doivent d�pendre ou
 * non du temps, et la fr�quence de sortie par d�faut pour les
 * variables associ�es.
 *
 * Interface Fortran : utiliser PSTCWR (voir cs_post_util.F)
 *
 * SUBROUTINE PSTCWR (NUMGEP, NOMCAS, NOMREP, NOMFMT, OPTFMT,
 * *****************
 *                    LNMCAS, LNMFMT, LNMREP, LOPFMT,
 *                    INDMOD, NTCHR)
 *
 * INTEGER          NUMGEP      : --> : Num�ro du filtre � cr�er (< 0 pour
 *                              :     : filtre standard ou d�veloppeur,
 *                              :     : > 0 pour filtre utilisateur)
 * CHARACTER        NOMCAS      : --> : Nom du cas associ�
 * CHARACTER        NOMREP      : --> : Nom du r�pertoire associ�
 * INTEGER          NOMFMT      : --> : Nom de format associ�
 * INTEGER          OPTFMT      : --> : Options associ�es au format
 * INTEGER          LNMCAS      : --> : Longueur du nom du cas
 * INTEGER          LNMREP      : --> : Longueur du nom du r�pertoire
 * INTEGER          LNMFMT      : --> : Longueur du nom du format
 * INTEGER          LOPFMT      : --> : Longueur des options du format
 * INTEGER          INDMOD      : --> : 0 si fig�, 1 si d�formable,
 *                              :     : 2 si la topologie change
 * INTEGER          NTCHR       : --> : Fr�quence de sortie par d�faut
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstcw1, PSTCW1)
(
 const cs_int_t   *const numwri,  /* --> num�ro du writer � cr�er
                                   *     < 0 pour writer r�serv�,
                                   *     > 0 pour writer utilisateur)         */
 const char       *const nomcas,  /* --> nom du cas associ�                   */
 const char       *const nomrep,  /* --> nom de r�pertoire associ�            */
 const char       *const nomfmt,  /* --> nom de format associ�                */
 const char       *const optfmt,  /* --> options associ�es au format          */
 const cs_int_t   *const lnmcas,  /* --> longueur du nom du cas               */
 const cs_int_t   *const lnmrep,  /* --> longueur du nom du r�pertoire        */
 const cs_int_t   *const lnmfmt,  /* --> longueur du nom du format            */
 const cs_int_t   *const lopfmt,  /* --> longueur des options du format       */
 const cs_int_t   *const indmod,  /* --> 0 si fig�, 1 si d�formable,
                                   *     2 si topologie change                */
 const cs_int_t   *const ntchr    /* --> fr�quence de sortie par d�faut       */
 CS_ARGF_SUPP_CHAINE              /*     (arguments 'longueur' �ventuels,
                                          Fortran, inutilis�s lors de
                                          l'appel mais plac�s par de
                                          nombreux compilateurs)              */
);


/*----------------------------------------------------------------------------
 * Cr�ation d'un maillage de post traitement ; les listes de cellules ou
 * faces � extraire sont tri�es en sortie, qu'elles le soient d�j� en entr�e
 * ou non.
 *
 * La liste des cellules associ�es n'est n�cessaire que si le nombre
 * de cellules � extraire est strictement sup�rieur � 0 et inf�rieur au
 * nombre de cellules du maillage.
 *
 * Les listes de faces ne sont prises en compte que si le nombre de cellules
 * � extraire est nul ; si le nombre de faces de bord � extraire est �gal au
 * nombre de faces de bord du maillage global, et le nombre de faces internes
 * � extraire est nul, alors on extrait par d�faut le maillage de bord, et la
 * liste des faces de bord associ�es n'est donc pas n�cessaire.
 *
 * Interface Fortran : utiliser PSTCMA (voir cs_post_util.F)
 *
 * SUBROUTINE PSTCM1 (NUMMAI, NOMMAI, LNMMAI,
 * *****************
 *                    NBRCEL, NBRFAC, NBRFBR, LSTCEL, LSTFAC, LSTFBR)
 *
 * INTEGER          NUMMAI      : --> : Num�ro du maillage externe � cr�er
 *                              :     : (< 0 pour maillage standard ou
 *                              :     : d�veloppeur, > 0 pour maillage
 *                              :     : utilisateur)
 * CHARACTER        NOMMAI      : --> : Nom du maillage externe associ�
 * INTEGER          LNMMAI      : --> : Longueur du nom de maillage
 * INTEGER          NBRCEL      : --> : Nombre de cellules associ�es
 * INTEGER          NBRFAC      : --> : Nombre de faces internes associ�es
 * INTEGER          NBRFBR      : --> : Nombre de faces de bord associ�es
 * INTEGER          LSTCEL      : <-> : Liste des cellules associ�es
 * INTEGER          LSTFAC      : <-> : Liste des faces internes associ�es
 * INTEGER          LSTFBR      : <-> : Liste des faces de bord associ�es
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstcm1, PSTCM1)
(
 const cs_int_t   *const nummai,    /* --> num�ro du maillage � cr�er (< 0 pour
                                     *     maillage standard ou d�veloppeur,
                                     *     > 0 pour maillage utilisateur)     */
 const char       *const nommai,    /* --> nom du maillage externe            */
 const cs_int_t   *const lnmmai,    /* --> longueur du nom du maillage        */
 const cs_int_t   *const nbrcel,    /* --> nombre de cellules                 */
 const cs_int_t   *const nbrfac,    /* --> nombre de faces internes           */
 const cs_int_t   *const nbrfbr,    /* --> nombre de faces de bord            */
       cs_int_t          lstcel[],  /* <-> liste des cellules                 */
       cs_int_t          lstfac[],  /* <-> liste des faces internes           */
       cs_int_t          lstfbr[]   /* <-> liste des faces de bord            */
 CS_ARGF_SUPP_CHAINE                /*     (arguments 'longueur' �ventuels,
                                           Fortran, inutilis�s lors de
                                           l'appel mais plac�s par de
                                           nombreux compilateurs)             */
);


/*----------------------------------------------------------------------------
 * Cr�ation d'un alias sur un maillage de post traitement.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTALM (NUMMAI, NUMWRI)
 * *****************
 *
 * INTEGER          NUMMAI      : --> : Num�ro de l'alias � cr�er
 * INTEGER          NUMREF      : --> : Num�ro du maillage externe associ�
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstalm, PSTALM)
(
 const cs_int_t   *nummai,      /* --> num�ro de l'alias � cr�er              */
 const cs_int_t   *numref       /* --> num�ro du maillage associe             */
);


/*----------------------------------------------------------------------------
 * Association d'un "writer" � un maillage pour le post traitement.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTASS (NUMMAI, NUMWRI)
 * *****************
 *
 * INTEGER          NUMMAI      : --> : Num�ro du maillage externe associ�
 * INTEGER          NUMWRI      : --> : Num�ro du "writer"
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstass, PSTASS)
(
 const cs_int_t   *nummai,      /* --> num�ro du maillage externe associ�     */
 const cs_int_t   *numwri       /* --> num�ro du "writer"                     */
);


/*----------------------------------------------------------------------------
 * Mise � jour de l'indicateur "actif" ou "inactif" des "writers" en
 * fonction du pas de temps et de leur fr�quence de sortie par d�faut.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTNTC (NTCABS)
 * *****************
 *
 * INTEGER          NTCABS      : --> : Num�ro du pas de temps
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstntc, PSTNTC)
(
 const cs_int_t   *ntcabs         /* --> num�ro de pas de temps associ�       */
);


/*----------------------------------------------------------------------------
 * Forcer de l'indicateur "actif" ou "inactif" d'un "writers" sp�cifique
 * ou de l'ensemble des "writers" pour le pas de temps en cours.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTNTC (NTCABS, TTCABS)
 * *****************
 *
 * INTEGER          NUMWRI      : --> : Num�ro du writer, ou 0 pour forcer
 *                              :     : simultan�ment tous les writers
 * INTEGER          INDACT      : --> : 0 pour d�sactiver, 1 pour activer
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstact, PSTACT)
(
 const cs_int_t   *numwri,     /* --> num�ro du writer, ou 0 pour forcer
                                *     simultan�ment tous les writers          */
 const cs_int_t   *indact      /* --> 0 pour d�sactiver, 1 pour activer       */
);


/*----------------------------------------------------------------------------
 * Ecriture des maillages de post traitement en fonction des writers
 * associ�s.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTEMA (NTCABS, TTCABS)
 * *****************
 *
 * INTEGER          NTCABS      : --> : Num�ro du pas de temps
 * DOUBLE PRECISION TTCABS      : --> : Temps physique associ�
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstema, PSTEMA)
(
 const cs_int_t   *ntcabs,        /* --> num�ro de pas de temps associ�       */
 const cs_real_t  *ttcabs         /* --> valeur du pas de temps associ�       */
);


/*----------------------------------------------------------------------------
 * Boucle sur les maillages de post traitement pour �criture  des variables
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstvar, PSTVAR)
(
 const cs_int_t   *const idbia0,      /* --> num�ro 1�re case libre dans IA   */
 const cs_int_t   *const idbra0,      /* --> num�ro 1�re case libre dans RA   */
 const cs_int_t   *const ndim,        /* --> dimension de l'espace            */
 const cs_int_t   *const ntcabs,      /* --> num�ro de pas de temps courant   */
 const cs_int_t   *const ncelet,      /* --> nombre de cellules �tendu        */
 const cs_int_t   *const ncel,        /* --> nombre de cellules               */
 const cs_int_t   *const nfac,        /* --> nombre de faces internes         */
 const cs_int_t   *const nfabor,      /* --> nombre de faces de bord          */
 const cs_int_t   *const nfml,        /* --> nombre de familles               */
 const cs_int_t   *const nprfml,      /* --> nombre de proprietes des familles*/
 const cs_int_t   *const nnod,        /* --> nombre de noeuds                 */
 const cs_int_t   *const lndfac,      /* --> longueur de nodfac               */
 const cs_int_t   *const lndfbr,      /* --> longueur de nodfbr               */
 const cs_int_t   *const ncelbr,      /* --> nombre de cellules de bord       */
 const cs_int_t   *const nvar,        /* --> nombre de variables              */
 const cs_int_t   *const nscal,       /* --> nombre de scalaires              */
 const cs_int_t   *const nphas,       /* --> nombre de phases                 */
 const cs_int_t   *const nvlsta,      /* --> nombre de variables stat. (lagr) */
 const cs_int_t   *const nvisbr,      /* --> nombre de variables stat. (lagr) */
 const cs_int_t   *const nideve,      /* --> longueur du tableau idevel[]     */
 const cs_int_t   *const nrdeve,      /* --> longueur du tableau rdevel[]     */
 const cs_int_t   *const nituse,      /* --> longueur du tableau ituser[]     */
 const cs_int_t   *const nrtuse,      /* --> longueur du tableau rtuser[]     */
 const cs_int_t          ifacel[],    /* --> liste des faces internes         */
 const cs_int_t          ifabor[],    /* --> liste des faces de bord          */
 const cs_int_t          ifmfbr[],    /* --> liste des familles des faces bord*/
 const cs_int_t          ifmcel[],    /* --> liste des familles des cellules  */
 const cs_int_t          iprfml[],    /* --> liste des proprietes des familles*/
 const cs_int_t          ipnfac[],    /* --> rg ds nodfac 1er sommet faces int*/
 const cs_int_t          nodfac[],    /* --> numero des sommets des faces int.*/
 const cs_int_t          ipnfbr[],    /* --> rg ds nodfbr 1er sommet faces brd*/
 const cs_int_t          nodfbr[],    /* --> num�ro des sommets des faces bord*/
 const cs_int_t          idevel[],    /* --> tab. compl�mentaire d�veloppeur  */
 const cs_int_t          ituser[],    /* --> tab. compl�mentaire utilisateur  */
 const cs_int_t          ia[],        /* --> macro-tableau entier             */
 const cs_real_t  *const ttcabs,      /* --> temps courant absolu             */
 const cs_real_t         xyzcen[],    /* --> c.d.g. des cellules              */
 const cs_real_t         surfac[],    /* --> surfaces des faces internes      */
 const cs_real_t         surfbo[],    /* --> surfaces des faces de bord       */
 const cs_real_t         cdgfac[],    /* --> c.d.g. des faces internes        */
 const cs_real_t         cdgfbo[],    /* --> c.d.g. des faces de bord         */
 const cs_real_t         xyznod[],    /* --> coordonnees des sommets          */
 const cs_real_t         volume[],    /* --> volumes des cellules             */
 const cs_real_t         dt[],        /* --> pas de temps                     */
 const cs_real_t         rtpa[],      /* --> variables aux cellules (pr�c.)   */
 const cs_real_t         rtp[],       /* --> variables aux cellules           */
 const cs_real_t         propce[],    /* --> propri�t�s physiques cellules    */
 const cs_real_t         propfa[],    /* --> propri�t�s physiques aux faces   */
 const cs_real_t         propfb[],    /* --> propri�t�s physiques faces bord  */
 const cs_real_t         coefa[],     /* --> cond. limites aux faces de bord  */
 const cs_real_t         coefb[],     /* --> cond. limites aux faces de bord  */
 const cs_real_t         statce[],    /* --> moyennes statistiques (Lagrangien*/
 const cs_real_t         stativ[],    /* --> variances statistiques (Lagrangie*/
 const cs_real_t         statfb[],    /* --> moyennes statistiques (Lagrangien*/
 const cs_real_t         rdevel[],    /* --> tab. compl�mentaire d�veloppeur  */
 const cs_real_t         rtuser[],    /* --> tab. compl�mentaire utilisateur  */
 const cs_real_t         ra[]         /* --> macro-tableau r�el               */
);


/*----------------------------------------------------------------------------
 * Sortie d'un champ de post traitement d�fini sur les cellules ou faces
 * d'un maillage en fonction des "writers" associ�s.
 *
 * Interface Fortran : utiliser PSTEVA (voir cs_post_util.F)
 *
 * SUBROUTINE PSTEVA (NUMMAI, NOMVAR, IDIMT,  IENTLA, IVARPR,
 * *****************
 *                    NTCABS, TTCABS, VARCEL, VARFAC, VARFBR)
 *
 * INTEGER          NUMMAI      : --> : Num�ro du maillage associ�
 * CHARACTER        NOMVAR      : --> : Nom de la variable
 * INTEGER          IDIMT       : --> : 1 pour scalaire, 3 pour vecteur
 * INTEGER          IENTLA      : --> : Si vecteur, 1 si valeurs entrelac�es
 *                              :     : (x1, y1, z1, x2, y2, ..., yn, zn),
 *                              :     : 0 sinon (x1, x2, ...xn, y1, y2, ...)
 * INTEGER          IVARPR      : --> : 1 si variable d�finie sur maillage
 *                              :     : "parent", 2 si variable restreinte
 *                              :     : au maillage post
 * INTEGER          NTCABS      : --> : Num�ro du pas de temps
 * DOUBLE PRECISION TTCABS      : --> : Temps physique associ�
 * DOUBLE PRECISION VARCEL(*)   : --> : Valeurs associ�es aux cellules
 * DOUBLE PRECISION VARFAC(*)   : --> : Valeurs associ�es aux faces internes
 * DOUBLE PRECISION VARFBO(*)   : --> : Valeurs associ�es aux faces de bord
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstev1, PSTEV1)
(
 const cs_int_t   *const nummai,      /* --> num�ro du maillage associ�       */
 const char       *const nomvar,      /* --> nom de la variable               */
 const cs_int_t   *const lnmvar,      /* --> longueur du nom de la variable   */
 const cs_int_t   *const idimt,       /* --> 1 pour scalaire, 3 pour vecteur  */
 const cs_int_t   *const ientla,      /* --> si vecteur, 1 si valeurs
                                       *     entrelac�es, 0 sinon             */
 const cs_int_t   *const ivarpr,      /* --> 1 si variable d�finie sur
                                       *     maillage "parent", 2 si variable
                                       *     restreinte au maillage post      */
 const cs_int_t   *const ntcabs,      /* --> num�ro de pas de temps associ�   */
 const cs_real_t  *const ttcabs,      /* --> valeur du pas de temps associ�   */
 const cs_real_t         varcel[],    /* --> valeurs aux cellules             */
 const cs_real_t         varfac[],    /* --> valeurs aux faces internes       */
 const cs_real_t         varfbr[]     /* --> valeurs aux faces de bord        */
 CS_ARGF_SUPP_CHAINE                  /*     (arguments 'longueur' �ventuels,
                                             Fortran, inutilis�s lors de
                                             l'appel mais plac�s par de
                                             nombreux compilateurs)           */
);


/*----------------------------------------------------------------------------
 * Prise en compte de la renum�rotation des faces et faces de bord
 * dans les liens de "parent�" des maillages post.
 *
 * Cette fonction ne doit �tre appell�e qu'une fois, apr�s la renum�rotation
 * �vuentuelle des faces, pour adapter les maillages post existants.
 * Des nouveaux maillages post seront automatiquement bas�s sur la
 * "bonne" num�rotation, par construction.
 *
 * Interface Fortran :
 *
 * SUBROUTINE PSTRNM
 * *****************
 *----------------------------------------------------------------------------*/

void CS_PROCF (pstrnm, PSTRNM)
(
 void
);


/*============================================================================
 *  Prototypes de fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Cr�ation d'un "writer" ; cet objet correspond au choix d'un nom de cas,
 * de r�pertoire, et de format, ainsi qu'un indicateur pr�cisant si les
 * maillages associ�s doivent d�pendre ou non du temps, et la fr�quence de
 * sortie par d�faut pour les variables associ�es.
 *----------------------------------------------------------------------------*/

void cs_post_ajoute_writer
(
       cs_int_t          id_writer,  /* --> num�ro du writer � cr�er
                                      *     (< 0 pour writer r�serv�,
                                      *      > 0 pour writer utilisateur)     */
 const char       *const nom_cas,    /* --> nom du cas associ�                */
 const char       *const nom_rep,    /* --> nom de r�pertoire associ�         */
 const char       *const nom_fmt,    /* --> nom de format associ�             */
 const char       *const opt_fmt,    /* --> options associ�es au format       */
       cs_int_t          ind_mod,    /* --> 0 si fig�, 1 si d�formable,
                                      *     2 si topologie change, +10 pour
                                      *     ajouter un champ d�placement      */
       cs_int_t          frequence   /* --> fr�quence de sortie par d�faut    */
);


/*----------------------------------------------------------------------------
 * Cr�ation d'un maillage de post traitement ; les listes de cellules ou
 * faces � extraire sont tri�es en sortie, qu'elles le soient d�j� en entr�e
 * ou non.
 *
 * La liste des cellules associ�es n'est n�cessaire que si le nombre
 * de cellules � extraire est strictement sup�rieur � 0 et inf�rieur au
 * nombre de cellules du maillage.
 *
 * Les listes de faces ne sont prises en compte que si le nombre de cellules
 * � extraire est nul ; si le nombre de faces de bord � extraire est �gal au
 * nombre de faces de bord du maillage global, et le nombre de faces internes
 * � extraire est nul, alors on extrait par d�faut le maillage de bord, et la
 * liste des faces de bord associ�es n'est donc pas n�cessaire.
 *----------------------------------------------------------------------------*/

void cs_post_ajoute_maillage
(
 const cs_int_t          id_maillage,  /* --> num�ro du maillage � cr�er
                                        *     (< 0 pour maillage r�serv�,
                                        *      > 0 pour maillage utilisateur) */
 const char       *const nom_maillage, /* --> nom du maillage externe         */
 const cs_int_t          nbr_cel,      /* --> nombre de cellules              */
 const cs_int_t          nbr_fac,      /* --> nombre de faces internes        */
 const cs_int_t          nbr_fbr,      /* --> nombre de faces de bord         */
       cs_int_t          liste_cel[],  /* <-> liste des cellules              */
       cs_int_t          liste_fac[],  /* <-> liste des faces internes        */
       cs_int_t          liste_fbr[]   /* <-> liste des faces de bord         */
);


/*----------------------------------------------------------------------------
 * Cr�ation d'un maillage de post traitement par association d'un maillage
 * externe existant.
 *
 * Si le maillage externe n'est plus destin� � �tre utilis� par ailleurs,
 * on peut choisir d'en transf�rer la propri�t� au maillage de post traitement,
 * qui g�rera alors son cycle de vie selon ses seuls besoins.
 *
 * Si le maillage externe doit continuer � �tre partag�, on devra veiller
 * � maintenir la coh�rence entre ce maillage et le posttraitement au cours
 * du temps.
 *----------------------------------------------------------------------------*/

void cs_post_ajoute_maillage_existant
(
 cs_int_t            id_maillage,      /* --> num�ro du maillage � cr�er
                                        *     (< 0 pour maillage r�serv�,
                                        *      > 0 pour maillage utilisateur) */
 fvm_nodal_t  *const maillage_ext,     /* --> maillage externe */
 cs_bool_t           transferer        /* --> indique si l'on transf�re la
                                        *     propri�t� du maillage externe
                                              au maillage de post traitement  */
);


/*----------------------------------------------------------------------------
 * Cr�ation d'un alias sur un maillage de post traitement.
 *
 * Un alias permet d'associer un num�ro suppl�mentaire � un maillage de
 * post traitement d�j� d�fini, et donc de lui associer d'autres
 * "writers" qu'au maillage initial ; ceci permet par exemple d'�crire
 * un jeu de variables principales tous les n1 pas de temps dans un
 * jeu de donn�es de post traitement, et de sortir quelques variables
 * sp�cifiques tous les n2 pas de temps dans un autre jeu de donn�es
 * de post traitement, sans n�cessiter de duplication du maillage support.
 *
 * Un alias est donc trait� en tout point comme le maillage principal
 * associ� ; en particulier, si la d�finition de l'un est modifi�, celle
 * de l'autre l'est aussi.
 *
 * Il est impossible d'associer un alias � un autre alias (cela n'aurait
 * pas d'utilit�), mais on peut associer plusieurs alias � un maillage.
 *----------------------------------------------------------------------------*/

void cs_post_alias_maillage
(
 const cs_int_t          id_alias,     /* --> num�ro de l'alias � cr�er
                                        *     (< 0 pour alias r�serv�,
                                        *      > 0 pour alias utilisateur)    */
 const cs_int_t          id_maillage   /* --> num�ro du maillage  associ�     */
);


/*----------------------------------------------------------------------------
 * V�rifie l'existence d'un "writer" associ� � un num�ro donn�.
 *----------------------------------------------------------------------------*/

cs_bool_t cs_post_existe_writer
(
 const cs_int_t   numwri        /* --> num�ro du writer associ�               */
);


/*----------------------------------------------------------------------------
 * Get the writer associated to a writer_id.
 *
 * writer_id       -->  id of the writer in cs_glob_post_writers
 *
 * Returns:
 *  a pointer to a fvm_writer_t structure
 *----------------------------------------------------------------------------*/

fvm_writer_t *
cs_post_get_writer(cs_int_t   writer_id);

/*----------------------------------------------------------------------------
 * V�rifie l'existence d'un maillage de post traitement associ� � un
 * num�ro donn�.
 *----------------------------------------------------------------------------*/

cs_bool_t cs_post_existe_maillage
(
 const cs_int_t   nummai        /* --> num�ro du maillage externe associ�     */
);


/*----------------------------------------------------------------------------
 * Modification d'un maillage de post traitement existant.
 *
 * Il s'agit ici de modifier les listes de cellules ou faces du maillage,
 * par exemple pour faire �voluer une coupe en fonction des zones
 * "int�ressantes (il n'est pas n�cessaire de recourir � cette fonction
 * si le maillage se d�forme simplement).
 *----------------------------------------------------------------------------*/

void cs_post_modifie_maillage
(
 const cs_int_t          id_maillage,  /* --> num�ro du writer � cr�er
                                        *     (< 0 pour maillage r�serv�,
                                        *      > 0 pour maillage utilisateur) */
 const cs_int_t          nbr_cel,      /* --> nombre de cellules              */
 const cs_int_t          nbr_fac,      /* --> nombre de faces internes        */
 const cs_int_t          nbr_fbr,      /* --> nombre de faces de bord         */
       cs_int_t          liste_cel[],  /* <-> liste des cellules              */
       cs_int_t          liste_fac[],  /* <-> liste des faces internes        */
       cs_int_t          liste_fbr[]   /* <-> liste des faces de bord         */
);


/*----------------------------------------------------------------------------
 * R�cup�ration du prochain num�ro de maillage standard ou d�veloppeur
 * disponible (bas� sur le plus petit num�ro n�gatif pr�sent -1).
 *----------------------------------------------------------------------------*/

cs_int_t cs_post_ret_num_maillage_libre
(
 void
);


/*----------------------------------------------------------------------------
 * Association d'un "writer" � un maillage pour le post traitement.
 *----------------------------------------------------------------------------*/

void cs_post_associe
(
 const cs_int_t   id_maillage,  /* --> num�ro du maillage externe associ�     */
 const cs_int_t   id_writer     /* --> num�ro du writer                       */
);


/*----------------------------------------------------------------------------
 * Mise � jour de l'indicateur "actif" ou "inactif" des "writers" en
 * fonction du pas de temps et de leur fr�quence de sortie par d�faut.
 *----------------------------------------------------------------------------*/

void cs_post_activer_selon_defaut
(
 const cs_int_t   nt_cur_abs    /* --> num�ro de pas de temps courant         */
);


/*----------------------------------------------------------------------------
 * Forcer de l'indicateur "actif" ou "inactif" d'un "writers" sp�cifique
 * ou de l'ensemble des "writers" pour le pas de temps en cours.
 *----------------------------------------------------------------------------*/

void cs_post_activer_writer
(
 const cs_int_t   id_writer,    /* --> num�ro du writer,ou 0 pour forcer
                                 *     simultan�ment tous les writers         */
 const cs_int_t   activer       /* --> 0 pour d�sactiver, 1 pour activer      */
);


/*----------------------------------------------------------------------------
 * Ecriture des maillages de post traitement en fonction des "writers"
 * associ�s.
 *----------------------------------------------------------------------------*/

void cs_post_ecrit_maillages
(
 const cs_int_t   nt_cur_abs,         /* --> num�ro de pas de temps courant   */
 const cs_real_t  t_cur_abs           /* --> valeur du temps physique associ� */
);


/*----------------------------------------------------------------------------
 * Sortie d'un champ de post traitement d�fini sur les cellules ou faces
 * d'un maillage en fonction des "writers" associ�s.
 *----------------------------------------------------------------------------*/

void cs_post_ecrit_var
(
       cs_int_t          id_maillage,  /* --> num�ro du maillage post associ� */
 const char             *nom_var,      /* --> nom de la variable              */
       cs_int_t          dim_var,      /* --> 1 pour scalaire, 3 pour vecteur */
       cs_bool_t         entrelace,    /* --> si vecteur, vrai si valeurs
                                        *     entrelac�es, faux sinon         */
       cs_bool_t         var_parent,   /* --> vrai si valeurs d�finies sur
                                        *     maillage "parent", faux si
                                        *     restreintes au maillage post    */
       cs_post_type_t    var_type,     /* --> type de donn�es associ�         */
       cs_int_t          nt_cur_abs,   /* --> num�ro de pas de temps courant  */
       cs_real_t         t_cur_abs,    /* --> valeur du temps physique        */
 const void             *var_cel,      /* --> valeurs aux cellules            */
 const void             *var_fac,      /* --> valeurs aux faces internes      */
 const void             *var_fbr       /* --> valeurs aux faces de bord       */
);


/*----------------------------------------------------------------------------
 * Sortie d'un champ de post traitement d�fini sur les sommets
 * d'un maillage en fonction des "writers" associ�s.
 *----------------------------------------------------------------------------*/

void cs_post_ecrit_var_som
(
       cs_int_t          id_maillage,  /* --> num�ro du maillage post associ� */
 const char             *nom_var,      /* --> nom de la variable              */
       cs_int_t          dim_var,      /* --> 1 pour scalaire, 3 pour vecteur */
       cs_bool_t         entrelace,    /* --> si vecteur, vrai si valeurs
                                        *     entrelac�es, faux sinon         */
       cs_bool_t         var_parent,   /* --> vrai si valeurs d�finies sur
                                        *     maillage "parent", faux si
                                        *     restreintes au maillage post    */
       cs_post_type_t    var_type,     /* --> type de donn�es associ�         */
       cs_int_t          nt_cur_abs,   /* --> num�ro de pas de temps courant  */
       cs_real_t         t_cur_abs,    /* --> valeur du temps physique        */
 const void             *var_som       /* --> valeurs aux sommets             */
);


/*----------------------------------------------------------------------------
 * Prise en compte de la renum�rotation des faces et faces de bord
 * dans les liens de "parent�" des maillages post.
 *
 * Cette fonction ne doit �tre appell�e qu'une fois, apr�s la renum�rotation
 * �vuentuelle des faces, pour adapter les maillages post existants.
 * Des nouveaux maillages post seront automatiquement bas�s sur la
 * "bonne" num�rotation, par construction.
 *----------------------------------------------------------------------------*/

void cs_post_renum_faces
(
 void
);

/*----------------------------------------------------------------------------
 * Destruction des structures associ�es aux post traitements
 *----------------------------------------------------------------------------*/

void cs_post_detruit
(
 void
);


/*----------------------------------------------------------------------------
 * Initialisation du post-traitement principal
 *----------------------------------------------------------------------------*/

void cs_post_init_pcp
(
 void
);


/*----------------------------------------------------------------------------
 * Ajout d'un traitement de variable temporelle � l'appel de PSTVAR.
 *
 * L'identificateur d'instance associ� � la fonction permet d'ajouter
 * une m�me fonction plusieurs fois, avec un identificateur diff�rent
 * permettant � la fonction de s�lectionner un sous-traitement.
 *----------------------------------------------------------------------------*/

void cs_post_ajoute_var_temporelle
(
 cs_post_var_temporelle_t  *fonction,    /* Fonction associ�e                 */
 cs_int_t                   id_instance  /* Indentificateur d'instance
                                            associ� � la fonction             */
 );


/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CS_POST_H__ */
