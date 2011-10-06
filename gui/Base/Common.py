# -*- coding: utf-8 -*-

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
This module defines global constant.
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import os.path

from optparse import OptionParser
import ConfigParser

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Global Parameters
#-------------------------------------------------------------------------------

# xml_doc_version modifie le 10/12/07
XML_DOC_VERSION = "2.0"

LABEL_LENGTH_MAX = 32

# Test if MEI syntax checking is available
from cs_package import package
cs_check_syntax = package().get_check_syntax()
if not os.path.isfile(cs_check_syntax):
    cs_check_syntax = None

# Test if a batch system is available

config = ConfigParser.ConfigParser()
config.read([package().get_configfile(),
             os.path.expanduser('~/.' + package().configfile)])

cs_batch_type = None
if config.has_option('install', 'batch'):
    cs_batch_type = config.get('install', 'batch')

del(config)


#-------------------------------------------------------------------------------
# End of Common
#-------------------------------------------------------------------------------
