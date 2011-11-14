#!/usr/bin/env python

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2011 EDF S.A.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301, USA.

#-------------------------------------------------------------------------------

"""
This module describes the script used to launch SALOME with the CFDSTUDY module.

This module defines the following functions:
- process_cmd_line
"""


#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import os, sys

from optparse import OptionParser
import ConfigParser

import cs_exec_environment
from cs_config import config

#-------------------------------------------------------------------------------
# Processes the passed command line arguments
#-------------------------------------------------------------------------------


def process_cmd_line(argv, pkg):
    """
    Processes the passed command line arguments.
    """

    parser = OptionParser(usage="usage: %prog [options]")

    (options, args) = parser.parse_args(argv)

    return


#-------------------------------------------------------------------------------
# Launch SALOME platform with CFDSTUDY module
#-------------------------------------------------------------------------------

def main(argv, pkg):
    """
    Main function.
    """

    template = """\
. %(salomepre)s;
. %(salomeenv)s;
CFDSTUDY_ROOT_DIR=%(prefix)s;
PYTHONPATH=%(pythondir)s/salome:%(pkgpythondir)s${PYTHONPATH:+:$PYTHONPATH};
export CFDSTUDY_ROOT_DIR PYTHONPATH;
%(runsalome)s --modules=%(modules)s
"""

    cfg = config()

    if cfg.have_salome == "no":
        sys.stderr.write("SALOME is not available in this installation.\n")
        sys.exit(1)

    # Skipped modules (version 6.3.0): YACS,JOBMANAGER,HOMARD,OPENTURNS
    default_modules = "GEOM,SMESH,MED,CFDSTUDY,PARAVIS,VISU"

    cmd = template % {'salomepre': cfg.salome_pre,
                      'salomeenv': cfg.salome_env,
                      'prefix': pkg.prefix,
                      'pythondir': pkg.pythondir,
                      'pkgpythondir': pkg.pkgpythondir,
                      'runsalome': cfg.salome_run,
                      'modules': default_modules}

    process_cmd_line(argv, pkg)

    retcode = cs_exec_environment.run_command(cmd,
                                              stdout=None,
                                              stderr=None)


if __name__ == "__main__":
    main(sys.argv[1:], None)


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
