#ifndef _ECS_FIC_H_
#define _ECS_FIC_H_

/*============================================================================
 *  Prototypes des fonctions
 *   associées aux impressions dans un fichier
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
 *  Fichiers `include' librairie standard C ou BFT
 *----------------------------------------------------------------------------*/

#include <stdio.h>


/*----------------------------------------------------------------------------
 *  Fichiers `include' publics  du  paquetage global "Utilitaire"
 *----------------------------------------------------------------------------*/

#include "ecs_def.h"
#include "ecs_tab.h"


/*-----------------------------------------------------------------------------
 *  Fichiers `include' publics  du  paquetage courant
 *----------------------------------------------------------------------------*/


/*============================================================================
 *                       Définition d'énumerations
 *============================================================================*/


/*============================================================================
 *                         Définitions de macros
 *============================================================================*/


/*============================================================================
 *                       Prototypes de fonctions publiques
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  Fonction d'impression du nom et de la valeur d'un pointeur
 *----------------------------------------------------------------------------*/

void
ecs_fic__imprime_ptr(FILE        *fic_imp,
                     int          profondeur_imp,
                     const char  *nom_ptr,
                     const void  *ptr);

/*----------------------------------------------------------------------------
 *  Fonction d'impression du nom et de la valeur d'une variable
 *----------------------------------------------------------------------------*/

void
ecs_fic__imprime_val(FILE        *fic_imp,
                     int          profondeur_imp_nom,
                     const char  *nom,
                     ecs_type_t   typ_e,
                     const void  *val);

/*----------------------------------------------------------------------------*/

#endif /* _ECS_FIC_H_ */
