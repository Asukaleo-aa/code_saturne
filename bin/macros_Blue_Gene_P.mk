#============================================================================
#
#                    Code_Saturne version 1.3
#                    ------------------------
#
#
#     This file is part of the Code_Saturne Kernel, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2008 EDF S.A., France
#
#     contact: saturne-support@edf.fr
#
#     The Code_Saturne Kernel is free software; you can redistribute it
#     and/or modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2 of
#     the License, or (at your option) any later version.
#
#     The Code_Saturne Kernel is distributed in the hope that it will be
#     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with the Code_Saturne Kernel; if not, write to the
#     Free Software Foundation, Inc.,
#     51 Franklin St, Fifth Floor,
#     Boston, MA  02110-1301  USA
#
#============================================================================
#
# Macros du Makefile Code_Saturne pour Blue Gene
################################################
#
# Chemins syst�me
#----------------

BGL_SYS  = /bgsys/drivers/ppcfloor/comm

# Macro pour BFT
#---------------

BFT_HOME        =/gpfs/home/saturne/opt/bft-1.0.6/arch/bgp

BFT_INC         =-I$(BFT_HOME)/include
BFT_LDFLAGS     =-L$(BFT_HOME)/lib -lbft

# Macro pour FVM
#---------------

FVM_HOME        =/gpfs/home/saturne/opt/fvm-0.10.0/arch/bgp

FVM_INC         =-I$(FVM_HOME)/include
FVM_LDFLAGS     =-L$(FVM_HOME)/lib -lfvm

# Macro pour MPI
#---------------

# Option MPI
MPI             =1
MPE             =0
MPE_COMM        =0

# Pour MPI BlueGene
MPI_HOME        =
MPI_INC         = -I$(BGL_SYS)/include
MPI_LIB  = 

# Macro pour Sockets
#-------------------

# Option Socket
SOCKET          =0
SOCKET_INC      =
SOCKET_LIB      =

# Macro pour XML
#---------------

# Option XML
XML             =1

XML_HOME = /gpfs/home/saturne/opt/libxml2-2.3.32/arch/bgp

XML_INC  =-I$(XML_HOME)/include/libxml2
XML_LIB  =-L$(XML_HOME)/lib -lxml2

# Macro pour BLAS
#----------------

# Option BLAS
BLAS            =1
ESSL            =1 # librairie ESSL IBM avec extension BLAS
BLAS_INC        =-I/opt/ibmmath/essl/4.3/include
BLAS_CFLAGS     =-D_CS_HAVE_ESSL
BLAS_LDFLAGS    =

# Macro pour gettext
#-------------------

# Option gettext
NLS				=0


# Preprocesseur
#--------------

PREPROC         =
PREPROCFLAGS    =


# Compilateur C natif
#--------------------

CCOMP                  = bgxlc

CCOMPFLAGSDEF          =

#CCOMPFLAGSDEF          = -g -qmaxmem=-1 -qarch=450d -qtune=450
#CCOMPFLAGSDEF          = -g -qmaxmem=-1 -qarch=450d -qtune=450 -qflttrap=enable:overflow:zerodivide -qsigtrap=xl_trcedump
#CCOMPFLAGSDEF          = -g -qmaxmem=-1 -qarch=450d -qtune=450 -qsource -qlist

CCOMPFLAGS             = $(CCOMPFLAGSDEF) -O3
CCOMPFLAGSOPTPART1     = $(CCOMPFLAGSDEF) -O3 -qhot
CCOMPFLAGSOPTPART2     = $(CCOMPFLAGSDEF) -O3 -qhot
CCOMPFLAGSOPTPART3     = $(CCOMPFLAGSDEF) -O3 -qhot
CCOMPFLAGSLO           = $(CCOMPFLAGSDEF) -O0
CCOMPFLAGSDBG          = $(CCOMPFLAGSDEF) -g
CCOMPFLAGSPROF         = -pg
CCOMPFLAGSVERS         = -v


# Compilateur FORTRAN
#--------------------
#  Profiling gprof : -pg -a

FTNCOMP                = bgxlf

FTNCOMPFLAGSDEF        = -qextname

#FTNCOMPFLAGSDEF        = -g -qmaxmem=-1 -qarch=450d -qtune=450 -qextname
#FTNCOMPFLAGSDEF        = -g -qmaxmem=-1 -qarch=450d -qtune=450 -qextname -qflttrap=enable:overflow:zerodivide -qsigtrap=xl_trcedump
#FTNCOMPFLAGSDEF        = -g -qmaxmem=-1 -qarch=450d -qtune=450 -qextname -qsource -qlist

FTNCOMPFLAGS           = $(FTNCOMPFLAGSDEF) -O3
FTNCOMPFLAGSOPTPART1   = $(FTNCOMPFLAGSDEF) -O3 -qhot
FTNCOMPFLAGSOPTPART2   = $(FTNCOMPFLAGSDEF) -O3 -qhot
FTNCOMPFLAGSOPTPART3   = $(FTNCOMPFLAGSDEF) -O3 -qhot
FTNCOMPFLAGSLO         = $(FTNCOMPFLAGSDEF) -O0
FTNCOMPFLAGSDBG        = $(FTNCOMPFLAGSDEF) -g
FTNCOMPFLAGSPROF       = -pg
FTNCOMPFLAGSVERS       = -v

FTNPREPROCOPT          = -WF,

# Linker
#-------

# Linker

LDEDL           = bgxlf_r
#LDEDL           = bgxlf_r -qflttrap=enable:overflow:zerodivide -qsigtrap=xl_trcedump
LDEDLFLAGS      = -O3
LDEDLFLAGSLO    = -O0
LDEDLFLAGSDBG   = -g
LDEDLFLAGSPROF  = -pg
LDEDLFLAGSVERS  = -v
LDEDLRPATH      =


# Positionnement des variables pour le pre-processeur
#----------------------------------------------------
#
# _POSIX_SOURCE          : utilisation des fonctions standard POSIX

VARDEF          = -D_POSIX_SOURCE


# Librairies a "linker"
#----------------------

# Zlib utilisee par HDF5
ZLIB     = -L/bgsys/local/tools_ibm/lib -lz

# Librairies IBM
SYSLIBS  = -L/bgsys/drivers/ppcfloor/comm/lib -lmpich.cnk -ldcmfcoll.cnk -ldcmf.cnk -L/bgsys/drivers/ppcfloor/runtime/SPI -lSPI.cna -lrt -lpthread
MASS     = -L/opt/ibmcmp/xlmass/bg/4.4/bglib -lmass -lmassv
ESSL     = -L/opt/ibmmath/essl/4.3/lib -lesslbg -lesslsmpbg
TRACE    = /bgsys/local/tools_ibm/lib/libmpitrace.a

# Librairies de base toujours prises en compte

LIBBASIC = $(ZLIB) -Wl,--allow-multiple-definition $(MASS) $(ESSL) $(TRACE) $(SYSLIBS)

# Librairies en mode sans option

LIBOPT   =

# Librairies en mode optimisation reduite

LIBLO    =

# Librairies en mode DEBUG

LIBDBG   =

# Librairie en mode ElectricFence (malloc debugger)

LIBEF    =

# Liste eventuelle des fichiers a compiler avec des options particulieres
#------------------------------------------------------------------------

# Sous la forme :
# LISTE_OPT_PART = fic_1.c fic_2.c \
#                fic_3.F
#
# paquet 70% cpu promav gradrc gradco prodsc
# paquet 10% cpu jacobi prcpol bilsc2 ;
#    option -qhot recommande pour ces sous-programmes
#    pour les autres, on privilegie l'O3, qui est suppose plus fiable
#      mais fait perdre un  peu de temps
#
#  Pour les fortrans, les listes ci-dessous servent a differencier
#	les options d'optimisation
#

LISTE_OPT_PART1 = gradco.F gradrc.F jacobi.F prcpol.F promav.F cs_matrix.c cs_sles.c
LISTE_OPT_PART2 = prodsc.F prods2.F prods3.F cs_blas.c cs_benchmark.c
LISTE_OPT_PART3 =

