#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2013 EDF S.A.
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
try:
    import ConfigParser  # Python2
    configparser = ConfigParser
except Exception:
    import configparser  # Python3
import os
import os.path
import sys

import cs_config

from cs_case_domain import *
from cs_case import *

#-------------------------------------------------------------------------------
# Extract a parameters file name from a shell run sript
#-------------------------------------------------------------------------------

def get_param(path):
    """
    Extract a parameters file name from a shell run sript
    """
    f = open(path, 'r')
    lines = f.readlines()
    f.close()

    param = None

    for line in lines:
        if line[0] == '#':
            continue
        i = line.find('#')
        if i > -1:
            line = line[0:i]
        line = line.strip()
        if len(line) == 0:
            continue
        i = line.find('--param')
        if i >= 0:
            i += len('--param')
            line = line[i:].strip()
            if not line:
                continue
            # Find file name, possibly protected by quotes
            # (protection by escape character not handled)
            sep = line[0]
            if sep == '"' or sep == "'":
                param = line.split(sep)[1]
            else:
                param = line.split()[0]

    return param

#===============================================================================
# Main function for code coupling execution
#===============================================================================

def coupling(package,
             domains,
             casedir,
             pset_size):

    use_saturne = False
    use_syrthes = False
    use_neptune = False

    # Use alternate compute (back-end) package if defined

    config = configparser.ConfigParser()
    config.read([package.get_configfile()])

    package_compute = None
    if config.has_option('install', 'compute_versions'):
        compute_versions = config.get('install', 'compute_versions').split(':')
        if compute_versions[0]:
            package_compute = pkg.get_alternate_version(compute_versions[0])

    # Initialize code domains
    sat_domains = []
    syr_domains = []
    nep_domains = []

    if domains == None:
        raise RunCaseError('No domains defined.')

    for d in domains:

        if (d.get('script') == None or d.get('domain') == None):
            msg = 'Check your coupling definition.\n'
            msg += 'script or domain key is missing.'
            raise RunCaseError(msg)

        if (d.get('solver') == 'Code_Saturne' or d.get('solver') == 'Saturne'):

            try:
                runcase = os.path.join(os.getcwd(),
                                       d.get('domain'),
                                       'SCRIPTS',
                                       d.get('script'))
                param = get_param(runcase)

            except Exception:
                err_str = 'Cannot read Code_Saturne script: ' + runcase
                raise RunCaseError(err_str)

            dom = domain(package,
                         package_compute = package_compute,
                         name = d.get('domain'),
                         param = param,
                         n_procs_weight = d.get('n_procs_weight'),
                         n_procs_min = d.get('n_procs_min'),
                         n_procs_max = d.get('n_procs_max'))

            use_saturne = True
            sat_domains.append(dom)

        elif (d.get('solver') == 'SYRTHES'):

            try:
                dom = syrthes_domain(package,
                                     cmd_line = d.get('opt'),
                                     name = d.get('domain'),
                                     param = d.get('script'),
                                     n_procs_weight = d.get('n_procs_weight'),
                                     n_procs_min = d.get('n_procs_min'),
                                     n_procs_max = d.get('n_procs_max'))

            except Exception:
                err_str = 'Cannot create SYRTHES domain. Opt = ' + d.get('opt') + '\n'
                err_str += ' domain = ' + d.get('domain')
                err_str += ' script = ' + d.get('script') + '\n'
                err_str += ' n_procs_weight = ' + str(d.get('n_procs_weight')) + '\n'
                raise RunCaseError(err_str)

            use_syrthes = True
            syr_domains.append(dom)

        elif (d.get('solver') == 'NEPTUNE_CFD'):

            try:
                runcase = os.path.join(os.getcwd(),
                                       d.get('domain'),
                                       'SCRIPTS',
                                       d.get('script'))
                param = get_param(runcase)

            except Exception:
                err_str = 'Cannot read NEPTUNE_CFD script: ' + runcase
                raise RunCaseError(err_str)

            dom = domain(package,
                         package_compute = package_compute,
                         name = d.get('domain'),
                         param = param,
                         n_procs_weight = d.get('n_procs_weight'),
                         n_procs_min = d.get('n_procs_min'),
                         n_procs_max = d.get('n_procs_max'))

            use_neptune = True
            nep_domains.append(dom)

        elif (d.get('solver') == 'Code_Aster' or d.get('solver') == 'Aster'):
            err_str = 'Code_Aster code coupling not handled yet.\n'
            raise RunCaseError(err_str)

        else:
            err_str = 'Unknown code type : ' + d.get('solver') + '.\n'
            raise RunCaseError(err_str)

    # Now handle case for the corresponding calculation domain(s).

    c = case(package,
             package_compute = package_compute,
             case_dir = casedir,
             domains = sat_domains + nep_domains,
             syr_domains = syr_domains)

    msg = ' Coupling execution between: \n'
    if use_saturne == True:
        msg += '   o Code_Saturne [' + str(len(sat_domains)) + ' domain(s)];\n'
    if use_syrthes == True:
        msg += '   o SYRTHES      [' + str(len(syr_domains)) + ' domain(s)];\n'
    if use_neptune == True:
        msg += '   o NEPTUNE_CFD  [' + str(len(nep_domains)) + ' domain(s)];\n'
    sys.stdout.write(msg+'\n')

    return c

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
