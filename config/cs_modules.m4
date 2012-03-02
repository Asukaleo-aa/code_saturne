dnl----------------------------------------------------------------------------
dnl   This file is part of the Code_Saturne Kernel, element of the
dnl   Code_Saturne CFD tool.
dnl
dnl   Copyright (C) 2011 EDF S.A., France
dnl
dnl   The Code_Saturne Kernel is free software; you can redistribute it
dnl   and/or modify it under the terms of the GNU General Public License
dnl   as published by the Free Software Foundation; either version 2 of
dnl   the License, or (at your option) any later version.
dnl
dnl   The Code_Saturne Kernel is distributed in the hope that it will be
dnl   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
dnl   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public Licence
dnl   along with the Code_Saturne Preprocessor; if not, write to the
dnl   Free Software Foundation, Inc.,
dnl   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
dnl-----------------------------------------------------------------------------

# CS_AC_TEST_ENV_MODULES
#-----------------------
# checks for environement modules

AC_DEFUN([CS_AC_TEST_ENV_MODULES], [

AC_ARG_WITH(modules,
            [AS_HELP_STRING([--with-modules=LIST],
                            [colon-separated list of environment modules])],
            [with_modules=$withval],
            [with_modules=check])

# Attempt at auto-detection

cs_env_modules="no"

if test "x$with_modules" = "xcheck" ; then

  # Test for environment modules

  if test "x$MODULESHOME" != "x" ; then

    AC_MSG_CHECKING([for environment modules])

    cs_env_modules=""
    try_modules=""
    try_modules_p=""

    outfile=cs_ac_config_modules-tmp

(
    oldIFS=$IFS; IFS=:
    for m in $LOADEDMODULES; do try_modules="$try_modules $m"; done
    IFS=$oldIFS

    module purge

    while test "x$try_modules" != "x$try_modules_p" ;
    do
      try_modules_p=$try_modules
      try_modules=""
      for m in $try_modules_p ; do
        prv_LOADED=$LOADEDMODULES
        module load $m > /dev/null 2>&1
        if test "$prv_LOADED" != "$LOADEDMODULES" ; then
          cs_env_modules="$cs_env_modules $m"
        else
          try_modules="$retry_modules $m"
        fi
      done
    done
    echo "$cs_env_modules" > $outfile
    module list
)

    cs_env_modules=`cat $outfile`
    rm -fr $outfile

  fi

elif test "x$with_modules" != "xno" ; then

  cs_env_modules=""
  oldIFS=$IFS; IFS=:
  for m in $with_modules; do cs_env_modules="$cs_env_modules $m"; done
  IFS=$oldIFS

fi

# Find the modulecmd executable
AC_PATH_PROG([cs_env_modules_cmd], [modulecmd], [], [${MODULESHOME}/bin:$PATH])

AC_SUBST(cs_env_modules)

])dnl

