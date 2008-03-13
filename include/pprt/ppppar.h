c@a
c@versb
C-----------------------------------------------------------------------
C
CVERS                  Code_Saturne version 1.3
C                      ------------------------
C
C     This file is part of the Code_Saturne Kernel, element of the
C     Code_Saturne CFD tool.
C
C     Copyright (C) 1998-2008 EDF S.A., France
C
C     contact: saturne-support@edf.fr
C
C     The Code_Saturne Kernel is free software; you can redistribute it
C     and/or modify it under the terms of the GNU General Public License
C     as published by the Free Software Foundation; either version 2 of
C     the License, or (at your option) any later version.
C
C     The Code_Saturne Kernel is distributed in the hope that it will be
C     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
C     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
C     GNU General Public License for more details.
C
C     You should have received a copy of the GNU General Public License
C     along with the Code_Saturne Kernel; if not, write to the
C     Free Software Foundation, Inc.,
C     51 Franklin St, Fifth Floor,
C     Boston, MA  02110-1301  USA
C
C-----------------------------------------------------------------------
c@verse
C                              ppppar.h
C
C***********************************************************************
C
C            INCLUDE GENERAL PROPRE A LA PHYSIQUE PARTICULIERE
C                    CONTENANT DES PARAMETRES COMMUNS
C                        (A PLUSIEURS INCLUDES)
C-----------------------------------------------------------------------
C
C --> NB DE ZONES DE BORD MAXIMAL
      INTEGER    NBZPPM
      PARAMETER (NBZPPM=2000)
C --> NUMERO DE ZONE DE BORD MAXIMAL
      INTEGER    NOZPPM
      PARAMETER (NOZPPM=2000)
C
C
C--> POINTEURS VARIABLES COMBUSTION CHARBON PULVERISE cpincl, ppincl
C
C       NCHARM        --> Nombre maximal de charbons
C       NCPCMX        --> Nombre maximal de classes par charbon
C       NCLCPM        --> Nombre total de classes
C
      INTEGER    NCHARM  , NCPCMX   , NCLCPM
      PARAMETER (NCHARM=3, NCPCMX=10, NCLCPM=NCHARM*NCPCMX)
C -->
C FIN
c@z
