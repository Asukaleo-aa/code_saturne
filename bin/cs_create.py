#!/usr/bin/env python
#-------------------------------------------------------------------------------
#   This file is part of the Code_Saturne Solver.
#
#   Copyright (C) 2009-2011  EDF
#
#   The Code_Saturne Preprocessor is free software; you can redistribute it
#   and/or modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation; either version 2 of
#   the License, or (at your option) any later version.
#
#   The Code_Saturne Preprocessor is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public Licence
#   along with the Code_Saturne Preprocessor; if not, write to the
#   Free Software Foundation, Inc.,
#   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#-------------------------------------------------------------------------------


"""
This module describes the script used to create a study/case for Code_Saturne.

This module defines the following functions:
- usage
- process_cmd_line
- make_executable
- comments
and the following classes:
- study
- class
"""


#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import os, sys, pwd, shutil, stat
import types, string, re, fnmatch
from optparse import OptionParser
import ConfigParser


#-------------------------------------------------------------------------------
# Process the passed command line arguments
#-------------------------------------------------------------------------------

def process_cmd_line(argv, pkg):
    """
    Process the passed command line arguments.
    """

    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-s", "--study", dest="study_name", type="string",
                      metavar="<study>",
                      help="create a new study")

    parser.add_option("-c", "--case", dest="case_names", type="string",
                      metavar="<case>", action="append",
                      help="create a new case")

    parser.add_option("--copy-from", dest="copy", type="string",
                      metavar="<case>",
                      help="create a case from another one")

    parser.add_option("--nogui", dest="use_gui",
                      action="store_false",
                      help="don't use the GUI")

    parser.add_option("--noref", dest="use_ref",
                      action="store_false",
                      help="don't copy references")

    parser.add_option("-q", "--quiet",
                      action="store_const", const=0, dest="verbose",
                      help="do not output any information")

    parser.add_option("-v", "--verbose",
                      action="store_const", const=2, dest="verbose",
                      help="dump study creation parameters")

    parser.add_option("--syrthes", dest="syr_case_names", type="string",
                      metavar="<syr_cases>", action="append",
                      help="create new SYRTHES case(s).")

    parser.add_option("--aster", dest="ast_case_name", type="string",
                      metavar="<ast_case>",
                      help="create a new Code_Aster case.")

    parser.set_defaults(use_gui=True)
    parser.set_defaults(use_ref=True)
    parser.set_defaults(study_name=os.path.basename(os.getcwd()))
    parser.set_defaults(case_names=[])
    parser.set_defaults(copy=None)
    parser.set_defaults(verbose=1)
    parser.set_defaults(n_sat=1)
    parser.set_defaults(syr_case_names=[])
    parser.set_defaults(ast_case_name=None)

    (options, args) = parser.parse_args(argv)

    if options.case_names == []:
        if len(args) > 0:
            options.case_names = args
        else:
            options.case_names = ["CASE1"]

    return Study(pkg,
                 options.study_name,
                 options.case_names,
                 options.syr_case_names,
                 options.ast_case_name,
                 options.copy,
                 options.use_gui,
                 options.use_ref,
                 options.verbose)


#-------------------------------------------------------------------------------
# Assign executable mode (chmod +x) to a file
#-------------------------------------------------------------------------------

def make_executable(filename):
    """
    Give executable permission to a given file.
    Equivalent to `chmod +x` shell function.
    """

    st   = os.stat(filename)
    mode = st[stat.ST_MODE]
    os.chmod(filename, mode | stat.S_IEXEC)

    return


#-------------------------------------------------------------------------------
# Build lines necessary to import SYRTHES packages
#-------------------------------------------------------------------------------

def syrthes_path_line(pkg):
    """
    Build lines necessary to import SYRTHES packages
    """

    line = None

    config = ConfigParser.ConfigParser()
    config.read([pkg.get_configfile(),
                 os.path.expanduser('~/.' + pkg.configfile)])

    if config.has_option('install', 'syrthes'):
        syr_datapath = os.path.join(config.get('install', 'syrthes'),
                                    os.path.join('share', 'syrthes'))
        line = 'sys.path.insert(1, \'' + syr_datapath + '\')\n'

    return line

#-------------------------------------------------------------------------------
# Comment or uncomment examples in user files
#-------------------------------------------------------------------------------

def comments(filename, use_gui):
    """
    Comment or uncomment examples in user files.
    """

    fd = file(filename, 'r')
    fdt = file(filename+'.tmp','w')

    kwd_beg = re.compile('EXAMPLE_CODE_TO_BE_ADAPTED_BY_THE_USER_START')
    kwd_end = re.compile('EXAMPLE_CODE_TO_BE_ADAPTED_BY_THE_USER_END')


    if use_gui:

        comment_line = 0
        for line in fd:
            if kwd_beg.search(line): comment_line = 1
            if comment_line == 0:
                fdt.write(line)
            else:
                if len(line) > 1:
                    fdt.write('!ex '+line)
                else:
                    fdt.write('!ex'+line)
                if kwd_end.search(line): comment_line = 0

    else:

        for line in fd:
            if not kwd_beg.search(line) and not kwd_end.search(line):
                fdt.write(line)

    fd.close()
    fdt.close()

    shutil.move(filename+'.tmp', filename)

    return


#-------------------------------------------------------------------------------
# Definition of a class for a study
#-------------------------------------------------------------------------------

class Study:

    def __init__(self, package, name, cases, syr_case_names, ast_case_name,
                 copy, use_gui, use_ref, verbose):
        """
        Initialize the structure for a study.
        """

        # Package specific information

        self.package = package

        self.name = name
        self.copy = copy
        if self.copy is not None:
            self.copy = os.path.abspath(self.copy)
        self.use_gui = use_gui
        self.use_ref = use_ref
        self.verbose = verbose

        self.cases = []
        for c in cases:
            self.cases.append(c)
        self.n_sat = len(cases)

        self.syr_case_names = []
        for c in syr_case_names:
            self.syr_case_names.append(c)

        self.ast_case_name = ast_case_name


    def get_syrthes_version(self):
        """
        Get available SYRTHES version.
        """
        syrthes_version = None

        config = ConfigParser.ConfigParser()
        config.read([self.package.get_configfile(),
                     os.path.expanduser('~/.' + self.package.configfile)])
        if config.has_option('install', 'syrthes'):
            syrthes_version = 4
        elif (len(self.package.syrthes_prefix) > 0):
            syrthes_version = 3

        return syrthes_version


    def create(self):
        """
        Create a study.
        """

        if self.name != os.path.basename(os.getcwd()):

            if self.verbose > 0:
                sys.stdout.write("  o Creating study '%s'...\n" % self.name)

            os.mkdir(self.name)
            os.chdir(self.name)
            os.mkdir('MESH')
            os.mkdir('POST')

        # Creating Code_Saturne cases
        repbase = os.getcwd()
        for c in self.cases:
            os.chdir(repbase)
            self.create_case(c)

        # Creating SYRTHES cases
        if len(self.syr_case_names) > 0:
            syrthes_version = self.get_syrthes_version()
            if syrthes_version == 4:
                self.create_syrthes_cases(repbase)
            elif syrthes_version == 3:
                self.create_syrthes3_cases(repbase)
            else:
                sys.stderr.write("Cannot locate SYRTHES installation.")
                sys.exit(1)

        # Creating Code_Aster case
        if self.ast_case_name is not None:
            config = ConfigParser.ConfigParser()
            config.read([self.package.get_configfile(),
                         os.path.expanduser('~/.' + self.package.configfile)])
            if config.has_option('install', 'aster'):
                self.create_aster_case(repbase)

            else:
                sys.stderr.write("Cannot locate Code_Aster installation.")
                sys.exit(1)

        # Creating coupling structure
        if len(self.cases) + len(self.syr_case_names) > 1:
            self.create_coupling(repbase)


    def create_syrthes3_cases(self, repbase):
        """
        Create and initialize SYRTHES 3 case directories.
        """

        syr_home = self.package.syrthes_prefix

        for s in self.syr_case_names:
            c = os.path.join(repbase, s)
            data_syr = os.path.join(c, 'DATA')
            src_syr = os.path.join(c, 'SRC')
            src_ref_syr = os.path.join(syr_home, 'data')
            os.mkdir(c)
            os.mkdir(data_syr)
            os.mkdir(src_syr)
            data_ref_syr = os.path.join(data_syr, 'REFERENCE')
            shutil.copytree(src_ref_syr, data_ref_syr)
            users_ref_syr = os.path.join(src_syr, 'REFERENCE')
            shutil.copytree(os.path.join(syr_home, 'usr'), users_ref_syr)


    def create_syrthes_cases(self, repbase):
        """
        Create and initialize SYRTHES case directories.
        """

        try:
            config = ConfigParser.ConfigParser()
            config.read([self.package.get_configfile(),
                         os.path.expanduser('~/.' + self.package.configfile)])
            syr_datapath = os.path.join(config.get('install', 'syrthes'),
                                        os.path.join('share', 'syrthes'))
            sys.path.insert(0, syr_datapath)
            import syrthes
        except Exception:
            sys.stderr.write("SYRTHES create case: Cannot locate SYRTHES installation.\n")
            sys.exit(1)

        for s in self.syr_case_names:
            os.chdir(repbase)
            retval = syrthes.create_syrcase(s)
            if retval > 0:
                sys.stderr.write("Cannot create SYRTHES case: '%s'\n" % s)
                sys.exit(1)


    def create_aster_case(self, repbase):
        """
        Create and initialize Code_Aster case directory.
        """

        if self.verbose > 0:
            sys.stdout.write("  o Creating Code_Aster case  '%s'...\n" %
                             self.ast_case_name)

        c = os.path.join(repbase, self.ast_case_name)
        os.mkdir(c)

        # All the following should be merged with create_coupling asap.

        resu = os.path.join(repbase, 'RESU_COUPLING')
        os.mkdir(resu)

        datadir = self.package.pkgdatadir
        try:
            shutil.copy(os.path.join(datadir, 'runcase_aster'),
                        os.path.join(repbase, 'runcase_coupling'))
        except:
            sys.stderr.write("Cannot copy runcase_coupling script: " + \
                             os.path.join(datadir, 'runcase_coupling') + ".\n")
            sys.exit(1)

        config = ConfigParser.ConfigParser()
        config.read([self.package.get_configfile(),
                     os.path.expanduser('~/.' + self.package.configfile)])
        asterhomedir = config.get('install', 'aster')
        astervers = config.get('install', 'aster_vers')

        runcase = os.path.join(repbase, 'runcase_coupling')
        runcase_tmp = runcase + '.tmp'

        kwd1 = re.compile('CASEDIRNAME')
        kwd2 = re.compile('CASENAME')
        kwd3 = re.compile('ASTERNAME')
        kwd4 = re.compile('STUDYNAME')
        kwd5 = re.compile('ASTERHOMEDIR')
        kwd6 = re.compile('ASTERVERSION')

        runcase_tmp = runcase + '.tmp'

        fd  = open(runcase, 'r')
        fdt = open(runcase_tmp,'w')

        for line in fd:
            line = re.sub(kwd1, repbase, line)
            line = re.sub(kwd2, self.cases[0], line)
            line = re.sub(kwd3, self.ast_case_name, line)
            line = re.sub(kwd4, self.name, line)
            line = re.sub(kwd5, asterhomedir, line)
            line = re.sub(kwd6, astervers, line)
            fdt.write(line)

        fd.close()
        fdt.close()

        shutil.move(runcase_tmp, runcase)
        make_executable(runcase)


    def create_coupling(self, repbase):
        """
        Create structure to enable code coupling.
        """

        if self.verbose > 0:
            sys.stdout.write("  o Creating coupling features ...\n")

        dict_str = ""
        e_pkg = re.compile('PACKAGE')
        e_dom = re.compile('DOMAIN')

        solver_name = self.package.code_name

        for c in self.cases:

            template = \
"""
    {'solver': 'PACKAGE',
     'domain': 'DOMAIN',
     'script': 'runcase',
     'n_procs_weight': None,
     'n_procs_min': 1,
     'n_procs_max': None}
"""
            template = re.sub(e_pkg, solver_name, template)
            template = re.sub(e_dom, c, template)

            dict_str += template

        for c in self.syr_case_names:

            if self.get_syrthes_version() == 3:
                template = \
"""
    ,
    {'solver': 'SYRTHES 3',
     'domain': 'DOMAIN',
     'script': 'syrthes.data',
     'echo_comm': None,        # verbosity if integer (> -1)
     'coupled_apps': None}     # list of domain names more than 1
"""
            else:
                template = \
"""
    ,
    {'solver': 'SYRTHES',
     'domain': 'DOMAIN',
     'script': 'syrthes_data.syd',
     'n_procs_weight': None,
     'n_procs_min': 1,
     'n_procs_max': None,
     'opt' : ''}               # Additional SYRTHES options
"""
            template = re.sub(e_dom, c, template)
            dict_str += template

        # Result directory for coupling execution

        resu = os.path.join(repbase, 'RESU_COUPLING')
        os.mkdir(resu)

        datadir = self.package.pkgdatadir
        try:
            shutil.copy(os.path.join(datadir, 'runcase_coupling'), repbase)
        except:
            sys.stderr.write("Cannot copy runcase_coupling script: " + \
                             os.path.join(datadir, 'runcase_coupling') + ".\n")
            sys.exit(1)

        runcase = os.path.join(repbase, 'runcase_coupling')
        runcase_tmp = runcase + '.tmp'

        e_dir = re.compile('CASEDIRNAME')
        e_apps = re.compile('APP_DICTS')

        syrthes_insert = syrthes_path_line(self.package)

        fd  = open(runcase, 'r')
        fdt = open(runcase_tmp,'w')

        for line in fd:

            line = re.sub(e_dir, repbase, line)
            line = re.sub(e_apps, dict_str, line)
            fdt.write(line)

            if syrthes_insert and line[0:15] == 'sys.path.insert':
                fdt.write(syrthes_insert)
                syrthes_insert = None

        fd.close()
        fdt.close()

        shutil.move(runcase_tmp, runcase)
        make_executable(runcase)

        self.build_batch_file(distrep = repbase,
                              casename = 'coupling',
                              scriptname = 'runcase_coupling')


    def create_case(self, casename):
        """
        Create a case for a Code_Saturne study.
        """

        if self.verbose > 0:
            sys.stdout.write("  o Creating case  '%s'...\n" % casename)

        datadir = self.package.pkgdatadir
        data_distpath  = os.path.join(datadir, 'data')
        users_distpath = os.path.join(datadir, 'users')

        try:
            os.mkdir(casename)
        except:
            sys.exit(1)

        os.chdir(casename)

        # Data directory

        data = 'DATA'
        os.mkdir(data)

        if self.use_ref:

            thch_distpath = os.path.join(data_distpath, 'thch')
            ref           = os.path.join(data, 'REFERENCE')
            os.mkdir(ref)
            for f in ['dp_C3P', 'dp_C3PSJ', 'dp_ELE', 'dp_FCP', 'dp_FUE', 'meteo']:
                abs_f = os.path.join(thch_distpath, f)
                if os.path.isfile(abs_f):
                    shutil.copy(abs_f, ref)
            abs_f = os.path.join(datadir, 'cs_user_scripts.py')
            shutil.copy(abs_f, ref)

        if self.use_gui:

            csguiname = self.package.guiname
            csguiscript = os.path.join(datadir, csguiname)

            shutil.copy(csguiscript, data)
            make_executable(os.path.join(data, csguiname))

        # User source files directory

        src = 'SRC'
        os.mkdir(src)

        if self.use_ref:

            users = os.path.join(src, 'REFERENCE')
            shutil.copytree(users_distpath, users)

            for file in ['usini1.f90','usalin.f90']:
                f = os.path.join(users, 'base', file)
                if os.path.isfile(f):
                    comments(f, self.use_gui)

        # Copy data and source files from another case

        if self.copy is not None:

            # Data files

            ref_data = os.path.join(self.copy, data)
            data_files = os.listdir(ref_data)

            for f in data_files:
                abs_f = os.path.join(ref_data, f)
                if os.path.isfile(abs_f) and \
                       f not in [self.package.guiname,
                                 'preprocessor_output']:
                    shutil.copy(os.path.join(ref_data, abs_f), data)

            # Source files

            ref_src = os.path.join(self.copy, src)
            src_files = os.listdir(ref_src)

            c_files = fnmatch.filter(src_files, '*.c')
            cxx_files = fnmatch.filter(src_files, '*.cxx')
            cpp_files = fnmatch.filter(src_files, '*.cpp')
            h_files = fnmatch.filter(src_files, '*.h')
            hxx_files = fnmatch.filter(src_files, '*.hxx')
            hpp_files = fnmatch.filter(src_files, '*.hpp')
            f_files = fnmatch.filter(src_files, '*.[fF]90')

            for f in c_files + h_files + f_files + \
                    cxx_files + cpp_files + hxx_files + hpp_files:
                shutil.copy(os.path.join(ref_src, f), src)

        # Results directory (only one for all instances)

        resu = 'RESU'
        os.mkdir(resu)

        # Script directory (only one for all instances)

        scripts = 'SCRIPTS'
        os.mkdir(scripts)

        self.build_batch_file(distrep = os.path.join(os.getcwd(), scripts),
                              casename = casename)


    def build_batch_file(self, distrep, casename, scriptname=None):
        """
        Retrieve batch file for the current system
        Update batch file for the study
        """

        user_shell = os.getenv('SHELL')
        if not user_shell:
            user_shell = '/bin/sh'

        batch_file = os.path.join(distrep, 'runcase')
        if scriptname == 'runcase_coupling':
            batch_file += '_batch'

        fd = open(batch_file, 'w')

        fd.write('#!' + user_shell + '\n')

        # Add batch system info if necessary

        config = ConfigParser.ConfigParser()
        config.read([self.package.get_configfile(),
                     os.path.expanduser('~/.' + self.package.configfile)])

        if config.has_option('install', 'batch'):

            template = config.get('install', 'batch')

            if not os.path.isabs(template):
                template = os.path.join(self.package.get_batchdir(),
                                        'batch.' + template)

            fdt = open(template, 'r')

            kwd1 = re.compile('nameandcase')

            studycasename = string.lower(self.name) + string.lower(casename)

            # For some systems, names are limited to 15 caracters
            studycasename = studycasename[:15]

            for line in fdt:
                line = re.sub(kwd1, studycasename, line)
                fd.write(line)

            fdt.close()

        else:
            fd.write('\n')

        # Add command to execute.

        if scriptname:
            fd.write('# Launch script:\n')
            fd.write('./' + scriptname + '\n\n')
        else:
            fd.write('# Ensure the correct command is found:\n')
            if user_shell[-3:] == 'csh': # handle C-type shells, just in case...
                export_cmd = 'setenv PATH '
            else:                        # handle Bourne-type shells (recommended)
                export_cmd = 'export PATH='
            export_cmd += '"' + self.package.bindir + '":$PATH'
            fd.write(export_cmd + '\n\n')
            fd.write('# Run command:\n')
            fd.write('\\' + self.package.name + ' run\n\n')

        fd.close()

        make_executable(batch_file)


    def dump(self):
        """
        Dump the structure of a study.
        """

        print()
        print("Name  of the study:", self.name)
        print("Names of the cases:", self.cases)
        if self.copy is not None:
            print("Copy from case:", self.copy)
        print("Use the GUI:", self.use_gui)
        print("Copy references:", self.use_ref)
        if self.n_sat > 1:
            print("Number of instances:", self.n_sat)
        if self.syr_case_names != None:
            print("SYRTHES instances:")
            for c in self.syr_case_names:
                print("  " + c)
        if self.ast_case_name != None:
            print("Code_Aster instance:", self.ast_case_name)
        print()


#-------------------------------------------------------------------------------
# Creation of the study directory
#-------------------------------------------------------------------------------

def main(argv, pkg):
    """
    Main function.
    """

    welcome = """\
%(name)s %(vers)s study/case generation
"""

    study = process_cmd_line(argv, pkg)

    if study.verbose > 0:
        sys.stdout.write(welcome % {'name':pkg.name, 'vers': pkg.version})

    study.create()

    if study.verbose > 1:
        study.dump()


if __name__ == "__main__":
    main(sys.argv[1:], None)


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
