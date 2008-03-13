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

#ifndef __CS_COUPLAGE_H__
#define __CS_COUPLAGE_H__

/*============================================================================
 * D�finitions, variables globales, et fonctions associ�es aux couplages
 * du code avec lui-m�me ou avec des modules reconnus.
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fichiers `include' librairie standard C
 *----------------------------------------------------------------------------*/


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


/*============================================================================
 * D�finition de macros
 *============================================================================*/


/*============================================================================
 * D�claration de structures
 *============================================================================*/

/*
  Pointeur associ� � un couplage. La structure elle-m�me est d�clar�e
  dans le fichier "cs_couplage.c", car elle n'est pas n�cessaire ailleurs.
*/

typedef struct _cs_couplage_t cs_couplage_t;


/*============================================================================
 * Fonctions publiques pour API Fortran
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
);


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
);


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
);


/*----------------------------------------------------------------------------
 * R�cup�ration des listes de cellules et de faces de bord coupl�es
 * (i.e. r�ceptrices) associ�es � un couplage.
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
);


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
);


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
);


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
);


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
);


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
);


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
);


/*============================================================================
 *  Prototypes de fonctions publiques
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
);


/*----------------------------------------------------------------------------
 * Suppression des couplages.
 *----------------------------------------------------------------------------*/

void cs_couplage_detruit_tout
(
 void
);


#endif /* __CS_COUPLAGE_H__ */
