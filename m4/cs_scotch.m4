dnl--------------------------------------------------------------------------------
dnl
dnl This file is part of Code_Saturne, a general-purpose CFD tool.
dnl
dnl Copyright (C) 1998-2011 EDF S.A.
dnl
dnl This program is free software; you can redistribute it and/or modify it under
dnl the terms of the GNU General Public License as published by the Free Software
dnl Foundation; either version 2 of the License, or (at your option) any later
dnl version.
dnl
dnl This program is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
dnl FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
dnl details.
dnl
dnl You should have received a copy of the GNU General Public License along with
dnl this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
dnl Street, Fifth Floor, Boston, MA 02110-1301, USA.
dnl
dnl--------------------------------------------------------------------------------

# CS_AC_TEST_SCOTCH
#-----------------
# modifies or sets cs_have_scotch, SCOTCH_CPPFLAGS, SCOTCH_LDFLAGS, and SCOTCH_LIBS
# depending on libraries found

AC_DEFUN([CS_AC_TEST_SCOTCH], [

cs_have_ptscotch=no
cs_have_scotch=no

AC_ARG_WITH(scotch,
            [AS_HELP_STRING([--with-scotch=PATH],
                            [specify prefix directory for SCOTCH])],
            [if test "x$withval" = "x"; then
               with_scotch=yes
             fi],
            [with_scotch=check])

AC_ARG_WITH(scotch-include,
            [AS_HELP_STRING([--with-scotch-include=PATH],
                            [specify directory for SCOTCH include files])],
            [if test "x$with_scotch" = "xcheck" -o "x$with_scotch" = "xno"; then
               with_scotch=yes
             fi
             SCOTCH_CPPFLAGS="-I$with_scotch_include"],
            [if test "x$with_scotch" != "xno" ; then
               if test "x$with_scotch" != "xyes" \
	               -a "x$with_scotch" != "xcheck"; then
                 SCOTCH_CPPFLAGS="-I$with_scotch/include"
               fi
             fi])

AC_ARG_WITH(scotch-lib,
            [AS_HELP_STRING([--with-scotch-lib=PATH],
                            [specify directory for SCOTCH library])],
            [if test "x$with_scotch" = "xcheck" -o "x$with_scotch" = "xno"; then
               with_scotch=yes
             fi
             SCOTCH_LDFLAGS="-L$with_scotch_lib"],
            [if test "x$with_scotch" != "xno" -a "x$with_scotch" != "xyes" \
	          -a "x$with_scotch" != "xcheck"; then
               SCOTCH_LDFLAGS="-L$with_scotch/lib"
             fi])


if test "x$with_scotch" != "xno" ; then

  saved_CPPFLAGS="$CPPFLAGS"
  saved_LDFLAGS="$LDFLAGS"
  saved_LIBS="$LIBS"

  # Test for PT-SCOTCH first

  CPPFLAGS="${CPPFLAGS} ${SCOTCH_CPPFLAGS} ${MPI_CPPFLAGS}"
  LDFLAGS="${LDFLAGS} ${SCOTCH_LDFLAGS} ${MPI_LDFLAGS}"
  SCOTCH_LIBS="-lptscotch -lptscotcherr -lm"
  LIBS="${LIBS} ${SCOTCH_LIBS}  ${MPI_LIBS}"

  AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <stdio.h>
#include <stdint.h>
#include <mpi.h>
#include <ptscotch.h>]],
[[ SCOTCH_dgraphInit((void *)0, MPI_COMM_WORLD); ]])],
[cs_have_ptscotch=yes],
[cs_have_ptscotch=no])

  # Test for SCOTCH second

  if test "x$cs_have_ptscotch" = "xno"; then

    CPPFLAGS="${saved_CPPFLAGS} ${SCOTCH_CPPFLAGS}"
    LDFLAGS="${saved_LDFLAGS} ${SCOTCH_LDFLAGS}"
    SCOTCH_LIBS="-lscotch -lscotcherr -lm"
    LIBS="${saved_LIBS} ${SCOTCH_LIBS}"

    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <stdio.h>
#include <stdint.h>
#include <scotch.h>]],
[[ SCOTCH_graphInit((void *)0); ]])],
[cs_have_scotch=yes],
[cs_have_scotch=no])

  fi

  if test "x$cs_have_ptscotch" = "xyes"; then
    AC_DEFINE([HAVE_PTSCOTCH], 1, [use SCOTCH])
    SCOTCH_LIBS="-lptscotch -lm" # libptscotcherr functions in cs_partition    
  elif test "x$cs_have_scotch" = "xyes"; then
    AC_DEFINE([HAVE_SCOTCH], 1, [use SCOTCH])
    SCOTCH_LIBS="-lscotch -lm" # libscotcherr functions in cs_partition    
  else
    SCOTCH_CPPFLAGS=""
    SCOTCH_LDFLAGS=""
    SCOTCH_LIBS=""
  fi

fi

CPPFLAGS="$saved_CPPFLAGS"
LDFLAGS="$saved_LDFLAGS"
LIBS="$saved_LIBS"

unset saved_CPPFLAGS
unset saved_LDFLAGS
unset saved_LIBS

AC_SUBST(cs_have_scotch)
AC_SUBST(SCOTCH_CPPFLAGS)
AC_SUBST(SCOTCH_LDFLAGS)
AC_SUBST(SCOTCH_LIBS)

])dnl

