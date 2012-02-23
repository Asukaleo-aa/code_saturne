#!/usr/bin/env python
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

import ConfigParser
import datetime
import fnmatch
import os
import os.path
import sys
import shutil
import stat

import cs_config
import cs_compile
import cs_xml_reader

from cs_exec_environment import run_command


#===============================================================================
# Utility functions
#===============================================================================

def any_to_str(arg):
    """Transform single values or lists to a whitespace-separated string"""

    s = ''

    if type(arg) == tuple or type(arg) == list:
        for e in arg:
            s += ' ' + str(e)
        return s[1:]

    else:
        return str(arg)

#-------------------------------------------------------------------------------

class RunCaseError(Exception):
    """Base class for exception handling."""

    def __init__(self, *args):
        self.args = args

    def __str__(self):
        if len(self.args) == 1:
            return str(self.args[0])
        else:
            return str(self.args)

#   def __repr__(self):
#       return "%s(*%s)" % (self.__class__.__name__, repr(self.args))

#===============================================================================
# Classes
#===============================================================================

class base_domain:
    """
    Base class from which classes handling running case should inherit.
    """

    #---------------------------------------------------------------------------

    def __init__(self,
                 package,                 # main package
                 name = None,             # domain name
                 n_procs_weight = None,   # recommended number of processes
                 n_procs_min = 1,         # min. number of processes
                 n_procs_max = None):     # max. number of processes

        # Package specific information

        self.package = package

        # Names, directories, and files in case structure

        self.case_dir = None

        self.name = name # used for multiple domains only

        self.data_dir = None
        self.result_dir = None
        self.src_dir = None

        self.mesh_dir = None

        # Working directory and executable

        self.exec_dir = None
        self.solver_path = None

        # Execution and debugging options

        self.n_procs = n_procs_weight
        self.n_procs_min = max(1, n_procs_min)
        self.n_procs_max = n_procs_max

        if self.n_procs == None:
            self.n_procs = 1
        self.n_procs = max(self.n_procs, self.n_procs_min)
        if self.n_procs_max != None:
            self.n_procs = min(self.n_procs, self.n_procs_max)

        self.valgrind = None

        # Error reporting
        self.error = ''

    #---------------------------------------------------------------------------

    def set_case_dir(self, case_dir):

        # Names, directories, and files in case structure

        self.case_dir = case_dir

        if self.name != None:
            self.case_dir = os.path.join(self.case_dir, self.name)

        self.data_dir = os.path.join(self.case_dir, 'DATA')
        self.result_dir = os.path.join(self.case_dir, 'RESU')
        self.src_dir = os.path.join(self.case_dir, 'SRC')

    #---------------------------------------------------------------------------

    def set_exec_dir(self, exec_dir):

        if os.path.isabs(exec_dir):
            self.exec_dir = exec_dir
        else:
            self.exec_dir = os.path.join(self.case_dir, 'RESU', exec_dir)

        if self.name != None:
            self.exec_dir = os.path.join(self.exec_dir, self.name)

        if not os.path.isdir(self.exec_dir):
            os.makedirs(self.exec_dir)

    #---------------------------------------------------------------------------

    def set_result_dir(self, name, given_dir = None):
        """
        If suffix = true, add suffix to all names in result dir.
        Otherwise, create subdirectory
        """

        if given_dir == None:
            self.result_dir = os.path.join(self.case_dir, 'RESU', name)
        else:
            self.result_dir = given_dir

        if self.name != None:
            self.result_dir = os.path.join(self.result_dir, self.name)

        if not os.path.isdir(self.result_dir):
            os.makedirs(self.result_dir)

    #---------------------------------------------------------------------------

    def copy_data_file(self, name, copy_name=None, description=None):
        """
        Copy a data file to the execution directory.
        """
        if os.path.isabs(name):
            source = name
            if copy_name == None:
                dest = os.path.join(self.exec_dir, os.path.basename(name))
            elif os.path.isabs(copy_name):
                dest = copy_name
            else:
                dest = os.path.join(self.exec_dir, copy_name)
        else:
            source = os.path.join(self.data_dir, name)
            if copy_name == None:
                dest = os.path.join(self.exec_dir, name)
            elif os.path.isabs(copy_name):
                dest = copy_name
            else:
                dest = os.path.join(self.exec_dir, copy_name)

        if os.path.isfile(source):
            shutil.copy2(source, dest)
        else:
            if description != None:
                err_str = \
                    'The ' + description + ' file: ', name, '\n' \
                    'can not be accessed.'
            else:
                err_str = \
                    'File: ', name, '\n' \
                    'can not be accessed.'
            raise RunCaseError(err_str)

    #---------------------------------------------------------------------------

    def copy_result(self, name, purge=False):
        """
        Copy a file or directory to the results directory,
        optionally removing it from the source.
        """

        # Determine absolute source and destination names

        if os.path.isabs(name):
            src = name
            dest = os.path.join(self.result_dir, os.path.basename(name))
        else:
            src = os.path.join(self.exec_dir, name)
            dest = os.path.join(self.result_dir, name)

        # If source and destination are identical, return

        if src == dest:
            return

        # Copy single file

        if os.path.isfile(src):
            shutil.copy2(src, dest)
            if purge:
                os.remove(src)

        # Copy single directory (possibly recursive)
        # Unkike os.path.copytree, the destination directory
        # may already exist.

        elif os.path.isdir(src):

            if not os.path.isdir(dest):
                os.mkdir(dest)
            list = os.listdir(src)
            for f in list:
                f_src = os.path.join(src, f)
                f_dest = os.path.join(dest, f)
                if os.path.isfile(f_src):
                    shutil.copy2(f_src, f_dest)
                elif os.path.isdir(f_src):
                    self.copy_result(f_src, f_dest)

            if purge:
                shutil.rmtree(src)

    #---------------------------------------------------------------------------

    def purge_result(self, name):
        """
        Remove a file or directory from execution directory.
        """

        # Determine absolute name

        if os.path.isabs(name):
            f = name
        else:
            f = os.path.join(self.exec_dir, name)

        # Remove file or directory

        if os.path.isfile(f) or os.path.islink(f):
            os.remove(f)

        elif os.path.isdir(f):
            shutil.rmtree(f)

    #---------------------------------------------------------------------------

    def get_n_procs(self):
        """
        Returns an array (list) containing the current number of processes
        associated with a solver stage followed by the minimum and maximum
        number of processes.
        """

        return [self.n_procs, self.n_procs_min, self.n_procs_max]

    #---------------------------------------------------------------------------

    def set_n_procs(self, n_procs):
        """
        Assign a number of processes to a solver stage.
        """

        self.n_procs = n_procs

    #---------------------------------------------------------------------------

    def solver_command(self, **kw):
        """
        Returns a tuple indicating the solver's working directory,
        executable path, and associated command-line arguments.
        """

        return self.exec_dir, self.solver_path, ''

    #---------------------------------------------------------------------------

    def summary_info(self, s):
        """
        Output summary data into file s
        """

        if self.name:
            name = self.name
            exec_dir = os.path.join(self.exec_dir, name)
            result_dir = os.path.join(self.result_dir, name)
        else:
            name = os.path.basename(self.case_dir)
            exec_dir = self.exec_dir
            result_dir = self.result_dir

        s.write('  Case           : ' + name + '\n')
        s.write('    directory    : ' + self.case_dir + '\n')
        s.write('    results dir. : ' + self.result_dir + '\n')
        if exec_dir != result_dir:
            s.write('    exec. dir.   : ' + self.exec_dir + '\n')

#-------------------------------------------------------------------------------

class domain(base_domain):
    """Handle running case."""

    #---------------------------------------------------------------------------

    def __init__(self,
                 package,                     # main package
                 package_compute = None,      # package for compute environment
                 name = None,                 # domain name
                 n_procs_weight = None,       # recommended number of processes
                 n_procs_min = None,          # min. number of processes
                 n_procs_max = None,          # max. number of processes
                 n_procs_partition = None,    # n. processes for partitioner
                 logging_args = None,         # command-line options for logging
                 param = None,                # XML parameters file
                 prefix = None,               # installation prefix
                 lib_add = None,              # linker command-line options
                 adaptation = None):          # HOMARD adaptation script

        base_domain.__init__(self, package,
                             name,
                             n_procs_weight,
                             n_procs_min,
                             n_procs_max)

        # Compute package if different from front-end

        if package_compute:
            self.package_compute = package_compute
        else:
            self.package_compute = self.package

        # Directories, and files in case structure

        self.restart_input = None
        self.mesh_input = None
        self.partition_input = None

        # Default executable

        self.solver_path = self.package_compute.get_solver()

        # Preprocessor options

        self.mesh_input = None

        self.mesh_dir = None
        self.meshes = None

        # Partition options

        self.exec_partition = True

        self.partition_n_procs = n_procs_partition
        self.partition_list = None
        self.partition_args = None

        # Solver options

        self.exec_solver = True

        self.param = param
        self.logging_args = logging_args
        self.solver_args = None

        self.valgrind = None

        # Additional data

        self.thermochemistry_data = None
        self.meteo_data = None

        self.user_input_files = None
        self.user_scratch_files = None

        self.prefix = prefix
        self.lib_add = lib_add

        # MPI IO (if available: options are: 'off', 'eo', 'ip')

        self.mpi_io = None

        # Adaptation using HOMARD

        self.adaptation = adaptation

    #---------------------------------------------------------------------------

    def for_domain_str(self):

        if self.name == None:
            return ''
        else:
            return 'for domain ' + str(self.name)

    #---------------------------------------------------------------------------

    def set_case_dir(self, case_dir):

        # Names, directories, and files in case structure

        base_domain.set_case_dir(self, case_dir)

        # We may now import user python script functions if present.

        user_scripts = os.path.join(self.data_dir, 'cs_user_scripts.py')
        if os.path.isfile(user_scripts):

            sys.path.insert(0, self.data_dir)
            import cs_user_scripts
            reload(cs_user_scripts) # In case of multiple domains
            sys.path.pop(0)

            try:
                cs_user_scripts.define_domain_parameter_file(self)
                del cs_user_scripts.define_domain_parameter_file
            except AttributeError:
                pass

            try:
                self.define_case_parameters \
                    = cs_user_scripts.define_case_parameters
                del cs_user_scripts.define_case_parameters
            except AttributeError:
                pass

            try:
                self.define_mpi_environment \
                    = cs_user_scripts.define_mpi_environment
                del cs_user_scripts.define_mpi_environment
            except AttributeError:
                pass

        # We may now parse the optional XML parameter file
        # now that its path may be built and checked.

        if self.param != None:
            root_str = self.package.code_name + '_GUI'
            version_str = '2.0'
            P = cs_xml_reader.Parser(os.path.join(self.data_dir, self.param),
                                     root_str = root_str,
                                     version_str = version_str)
            params = P.getParams()
            for k in params.keys():
                self.__dict__[k] = params[k]

        # Now override or complete data from the XML file.

        if os.path.isfile(user_scripts):
            try:
                cs_user_scripts.define_domain_parameters(self)
                del cs_user_scripts.define_domain_parameters
            except AttributeError:
                pass

        # Finally, ensure some fields are of the required types

        if type(self.meshes) != list:
            self.meshes = [self.meshes,]

    #---------------------------------------------------------------------------

    def symlink(self, target, link=None, check_type=None):
        """
        Create a symbolic link to a file, or copy it if links are
        not possible
        """

        if target == None and link == None:
            return
        elif target == None:
            err_str = 'No target for link: ' + link
            raise RunCaseError(err_str)
        elif link == None:
            if self.exec_dir != None:
                link = os.path.join(self.exec_dir,
                                    os.path.basename(target))
            else:
                err_str = 'No path name given for link to: ' + target
                raise RunCaseError(err_str)

        if not os.path.exists(target):
            err_str = 'File: ' + target + ' does not exist.'
            raise RunCaseError(err_str)

        elif check_type == 'file':
            if not os.path.isfile(target):
                err_str = target + ' is not a regular file.'
                raise RunCaseError(err_str)

        elif check_type == 'dir':
            if not os.path.isdir(target):
                err_str = target + ' is not a directory.'
                raise RunCaseError(err_str)

        try:
            os.symlink(target, link)
        except AttributeError:
            shutil.copy2(target, link)

    #---------------------------------------------------------------------------

    def needs_compile(self):
        """
        Compile and link user subroutines if necessary
        """
        # Check if there are files to compile in source path

        dir_files = os.listdir(self.src_dir)

        src_files = (fnmatch.filter(dir_files, '*.c')
                     + fnmatch.filter(dir_files, '*.cxx')
                     + fnmatch.filter(dir_files, '*.cpp')
                     + fnmatch.filter(dir_files, '*.[fF]90'))

        if self.exec_solver and len(src_files) > 0:
            return True
        else:
            return False

    #---------------------------------------------------------------------------

    def copy_user_script(self):
        """
        Copy the user script to the execution directory
        """
        user_scripts = os.path.join(self.data_dir, 'cs_user_scripts.py')
        if os.path.isfile(user_scripts):
            dest = os.path.join(self.result_dir, 'cs_user_scripts.py')
            shutil.copy2(user_scripts, dest)

    #---------------------------------------------------------------------------

    def compile_and_link(self):
        """
        Compile and link user subroutines if necessary
        """
        # Check if there are files to compile in source path

        dir_files = os.listdir(self.src_dir)

        src_files = (fnmatch.filter(dir_files, '*.c')
                     + fnmatch.filter(dir_files, '*.cxx')
                     + fnmatch.filter(dir_files, '*.cpp')
                     + fnmatch.filter(dir_files, '*.[fF]90'))

        if len(src_files) > 0:

            # Add header files to list so as not to forget to copy them

            src_files = src_files + (  fnmatch.filter(dir_files, '*.h')
                                     + fnmatch.filter(dir_files, '*.hxx')
                                     + fnmatch.filter(dir_files, '*.hpp'))

            exec_src = os.path.join(self.exec_dir, self.package.srcdir)

            # Copy source files to execution directory

            os.mkdir(exec_src)
            for f in src_files:
                src_file = os.path.join(self.src_dir, f)
                dest_file = os.path.join(exec_src, f)
                shutil.copy2(src_file, dest_file)

            log_name = os.path.join(self.exec_dir, 'compile.log')
            log = open(log_name, 'w')

            retval = cs_compile.compile_and_link(self.package_compute,
                                                 exec_src,
                                                 self.exec_dir,
                                                 self.lib_add,
                                                 keep_going=True,
                                                 stdout=log,
                                                 stderr=log)

            log.close()

            if retval == 0:
                self.solver_path = os.path.join(self.exec_dir,
                                                self.package_compute.solver)
            else:
                # In case of error, copy source to results directory now,
                # as no calculation is possible, then raise exception
                for f in [self.package.srcdir, 'compile.log']:
                    self.copy_result(f)
                raise RunCaseError('Compile or link error.')

    #---------------------------------------------------------------------------

    def copy_preprocessor_data(self):
        """
        Copy preprocessor data to execution directory
        """

        # If we are using a prior preprocessing, simply link to it here
        if self.mesh_input:
            mesh_input = os.path.expanduser(self.mesh_input)
            if not os.path.isabs(mesh_input):
                mesh_input = os.path.join(self.case_dir, mesh_input)
            link_path = os.path.join(self.exec_dir, 'mesh_input')
            self.symlink(mesh_input, link_path)
            return

    #---------------------------------------------------------------------------

    def copy_solver_data(self):
        """
        Copy solver data to the execution directory
        """

        if not self.exec_solver:
            return

        if self.n_procs < 2:
            self.exec_partition = False
        elif self.exec_partition == False and self.partition_input != None:
            partition_input = os.path.expanduser(self.partition_input)
            if not os.path.isabs(partition_input):
                partition_input = os.path.join(self.case_dir, partition_input)
            part_name = 'domain_number_' + str(self.n_procs)
            partition = os.path.join(partition_input, part_name)
            if os.path.isfile(partition):
                part_dir = os.path.join(self.exec_dir, 'partition')
                if not os.path.isdir(part_dir):
                    os.mkdir(part_dir)
                self.symlink(partition, os.path.join(part_dir, part_name))
            else:
                w_str = \
                    'Warning: no partitioning file is available\n' \
                    '         (no ' + partition + ').\n' \
                    '\n' \
                    '         Geometry-based partitioning will be used.\n'
                sys.stderr.write(w_str)

        # Parameters file

        if self.param != None:
            self.copy_data_file(self.param,
                                os.path.basename(self.param),
                                'parameters')

        # Restart files

        if self.restart_input != None:

            restart_input =  os.path.expanduser(self.restart_input)
            if not os.path.isabs(restart_input):
                restart_input = os.path.join(self.case_dir, restart_input)

            if os.path.exists(restart_input):

                if not os.path.isdir(restart_input):
                    err_str = restart_input + ' is not a directory.'
                    raise RunCaseError(err_str)
                else:
                    self.symlink(restart_input,
                                 os.path.join(self.exec_dir, 'restart'))

        # Data for specific physics

        if self.thermochemistry_data != None:
            self.copy_data_file(self.thermochemistry_data,
                                'dp_thch',
                                'thermochemistry')
            if not os.path.isfile('JANAF'):
                self.copy_data_file(os.path.join(self.package.pkgdatadir,
                                                 'data',
                                                 'thch',
                                                 'JANAF'),
                                    'JANAF')

        if self.meteo_data != None:
            self.copy_data_file(self.meteo_data,
                                'meteo',
                                'meteo profile')
            # Second copy so as to have correct name upon backup
            if self.meteo_data != 'meteo':
                self.copy_data_file(self.meteo_data)

        # Presence of user input files

        if self.user_input_files != None:
            for f in self.user_input_files:
                self.copy_data_file(f)

    #---------------------------------------------------------------------------

    def run_preprocessor(self):
        """
        Runs the preprocessor in the execution directory
        """

        if self.mesh_input:
            return

        # Study directory
        study_dir = os.path.split(self.case_dir)[0]

        # User config file
        u_cfg = ConfigParser.ConfigParser()
        u_cfg.read(os.path.expanduser('~/.' + self.package.configfile))

        # Global config file
        g_cfg = ConfigParser.ConfigParser()
        g_cfg.read(self.package.get_configfile())

        # A mesh can be found in different mesh database directories
        # (case, study, user, global -- in this order)
        mesh_dirs = []
        if self.mesh_dir is not None:
            mesh_dir = os.path.expanduser(self.mesh_dir)
            if not os.path.isabs(mesh_dir):
                mesh_dir = os.path.join(self.case_dir, mesh_dir)
            mesh_dirs.append(mesh_dir)
        if os.path.isdir(os.path.join(study_dir, 'MESH')):
            mesh_dirs.append(os.path.join(study_dir, 'MESH'))
        if u_cfg.has_option('run', 'meshdir'):
            add_path = u_cfg.get('run', 'meshdir').split(':')
            for d in add_path:
                mesh_dirs.append(d)
        if g_cfg.has_option('run', 'meshdir'):
            add_path = g_cfg.get('run', 'meshdir').split(':')
            for d in add_path:
                mesh_dirs.append(d)

        # Switch to execution directory

        cur_dir = os.path.realpath(os.getcwd())
        if cur_dir != self.exec_dir:
            os.chdir(self.exec_dir)

        mesh_id = None

        if len(self.meshes) > 1:
            mesh_id = 0
            destdir = 'mesh_input'
            if not os.path.isdir(destdir):
                os.mkdir(destdir)
            else:
                list = os.listdir(destdir)
                for f in list:
                    os.remove(os.path.join(destdir,f))

        # Run once per mesh

        for m in self.meshes:

            # Get absolute mesh paths

            if m is None:
                err_str = 'Preprocessing stage required but no mesh is given'
                raise RunCaseError(err_str)

            if (type(m) == tuple):
                m = m[0]

            m = os.path.expanduser(m)

            mesh_path = m
            if (not os.path.isabs(m)) and len(mesh_dirs) > 0:
                for mesh_dir in mesh_dirs:
                    mesh_path = os.path.join(mesh_dir, m)
                    if os.path.isfile(mesh_path):
                        break

            if not os.path.isfile(mesh_path):
                err_str = 'Mesh file ' + m + ' not found'
                if not (os.path.isabs(mesh_path) or mesh_dirs):
                    err_str += '(no mesh directory given)'
                raise RunCaseError(err_str)

            # Build command

            cmd = self.package.get_preprocessor()

            if (type(m) == tuple):
                for opt in m[1:]:
                    cmd += ' ' + opt

            if (mesh_id != None):
                mesh_id += 1
                cmd += ' --log preprocessor_%02d.log' % (mesh_id)
                cmd += ' --out ' + os.path.join('mesh_input',
                                                'mesh_%02d' % (mesh_id))
            else:
                cmd += ' --log'
                cmd += ' --out mesh_input'

            cmd += ' ' + mesh_path

            # Run command

            retcode = run_command(cmd)

            if retcode != 0:
                err_str = \
                    'Error running the preprocessor.\n' \
                    'Check the preprocessor.log file for details.\n\n'
                sys.stderr.write(err_str)

                self.exec_partition = False
                self.exec_solver = False

                self.error = 'preprocess'

                break

        # Revert to initial directory

        if cur_dir != self.exec_dir:
            os.chdir(cur_dir)

        return retcode

    #---------------------------------------------------------------------------

    def check_partitioner(self):
        """
        Tests if the partitioner is available and partitioning is defined.
        """

        if self.n_procs < 2 and self.partition_list == None:
            self.exec_partition = False

        if (self.exec_partition == False):
            return

        w_str = None

        if self.partition_n_procs < 2:
            partitioner = self.package.get_partitioner()
        else:
            partitioner = self.package_compute.get_partitioner()

        if not os.path.isfile(partitioner):
            if self.n_procs > 1:
                w_str = \
                    'Warning: ' + partitioner + ' not found.\n\n' \
                    'The partitioner may not have been installed' \
                    '  (this is the case if neither METIS nor SCOTCH ' \
                    ' are available).\n\n'

        if os.path.isdir(os.path.join(self.exec_dir, 'mesh_input')):
            w_str = \
                'Warning: mesh_input is a directory\n\n' \
                'The Kernel must be run to concatenate its contents\n' \
                ' before graph-based partitioning is available.\n\n'

        if w_str:
            w_str += 'Partitioning by a space-filling curve will be used.\n\n'
            sys.stderr.write(w_str)
            self.exec_partition = False
            self.partition_n_procs = None

    #---------------------------------------------------------------------------

    def run_partitioner(self):
        """
        Runs the partitioner in the execution directory
        """

        self.check_partitioner()
        if self.exec_partition == False:
            return

        # Build command

        cmd = self.package.get_partitioner()

        if self.logging_args != None:
            cmd += ' ' + self.logging_args

        if self.partition_args != None:
            cmd += ' ' + self.partition_args

        if self.partition_list != None:
            cmd += ' ' + any_to_str(self.partition_list)

        if self.exec_solver and self.n_procs != None:
            np = self.n_procs
            if self.partition_list == None:
                cmd += ' ' + str(np)
            elif np > 1 and not np in self.partition_list:
                cmd += ' ' + str(np)

        # Run command

        cur_dir = os.path.realpath(os.getcwd())
        if cur_dir != self.exec_dir:
            os.chdir(self.exec_dir)

        retcode = run_command(cmd)

        if retcode != 0:
            err_str = \
                'Error running the partitioner.\n' \
                'Check the partition.log file for details.\n\n'
            sys.stderr.write(err_str)

            self.exec_solver = False

            self.error = 'partition'

        if cur_dir != self.exec_dir:
            os.chdir(cur_dir)

        return retcode

    #---------------------------------------------------------------------------

    def partitioner_command(self):
        """
        Returns a tuple indicating the partitioner's working directory,
        executable path, and associated command-line arguments.
        """
        # Working directory and executable path

        wd = self.exec_dir
        exec_path = self.package_compute.get_partitioner()

        # Build kernel command-line arguments

        args = ''

        if self.logging_args != None:
            args += ' ' + self.logging_args

        if self.partition_n_procs > 1:
            args += ' --mpi'

        if self.mpi_io != None:
            args += ' --mpi-io ' + self.mpi_io

        if self.partition_args != None:
            args += ' ' + self.partition_args

        if self.partition_list != None:
            args += ' ' + any_to_str(self.partition_list)

        if self.exec_solver and self.n_procs != None:
            np = self.n_procs
            if self.partition_list == None:
                args += ' ' + str(np)
            elif np > 1 and not np in self.partition_list:
                args += ' ' + str(np)

        # Adjust for Valgrind if used

        if self.valgrind != None:
            args = self.solver_path + ' ' + args
            exec_path = self.valgrind

        return wd, exec_path, args

    #---------------------------------------------------------------------------

    def solver_command(self, **kw):
        """
        Returns a tuple indicating the solver's working directory,
        executable path, and associated command-line arguments.
        """

        wd = self.exec_dir              # Working directory
        exec_path = self.solver_path    # Executable

        # Build kernel command-line arguments

        args = ''

        if self.param != None:
            args += ' --param ' + self.param

        if self.logging_args != None:
            args += ' ' + self.logging_args

        if self.solver_args != None:
            args += ' ' + self.solver_args

        if self.name != None:
            args += ' --mpi --app-name ' + self.name
        elif self.n_procs > 1:
            args += ' --mpi'

        if self.mpi_io != None:
            args += ' --mpi-io ' + self.mpi_io

        if 'syr_port' in kw:
            args += ' --syr-socket ' + str(kw['syr_port'])

        # Adjust for Valgrind if used

        if self.valgrind != None:
            args = self.solver_path + ' ' + args
            exec_path = self.valgrind + ' '

        return wd, exec_path, args

    #---------------------------------------------------------------------------

    def copy_preprocessor_results(self):
        """
        Retrieve preprocessor results from the execution directory
        and remove preprocessor input files if necessary.
        """

        if self.mesh_input:
            return

        # Determine if we should purge the execution directory

        purge = True
        if self.error == 'preprocess':
            purge = False

        # Copy log file(s) first

        if len(self.meshes) == 1:
            f = os.path.join(self.exec_dir, 'preprocessor.log')
            if os.path.isfile(f):
                self.copy_result(f, purge)
        else:
            mesh_id = 0
            for m in self.meshes:
                mesh_id += 1
                f = os.path.join(self.exec_dir,
                                 'preprocessor_%02d.log' % (mesh_id))
                if os.path.isfile(f):
                    self.copy_result(f, purge)

        # Copy output if required (only purge if we have no further
        # errors, as it may be necessary for future debugging).

        if self.error != '':
            purge = False

        f = os.path.join(self.exec_dir, 'mesh_input')

        if not self.exec_solver:
            if os.path.isfile(f) or os.path.isdir(f):
                self.copy_result(f, purge)
        elif purge:
            self.purge_result(f)

    #---------------------------------------------------------------------------

    def copy_partition_results(self):
        """
        Retrieve partition results from the execution directory
        """

        if not self.exec_partition:
            return

        # Determine if we should purge the execution directory

        purge = True
        if self.error == 'partition':
            purge = False

        # Copy log file first.

        f = os.path.join(self.exec_dir, 'partition.log')
        if os.path.isfile(f):
            self.copy_result(f, purge)

        # Copy output if required (only purge if we have no further
        # errors, as it may be necessary for future debugging).

        if self.error != '':
            purge = False

        if purge:
            f = os.path.join(self.exec_dir, 'partition.sh')
            if os.path.isfile(f):
                self.purge_result(f)

        d = os.path.join(self.exec_dir, 'partition')

        if self.partition_list or not self.exec_solver:
            if os.path.isdir(d):
                self.copy_result(d, purge)
        elif purge:
            self.purge_result(d)

        # Purge mesh_input if it was used solely by the partitioner

        if self.mesh_input and self.exec_solver == False:
            if self.error == '':
                self.purge_result('mesh_input')

    #---------------------------------------------------------------------------

    def copy_solver_results(self):
        """
        Retrieve solver results from the execution directory
        """

        if not self.exec_solver:
            return

        # Determine all files present in execution directory

        dir_files = os.listdir(self.exec_dir)

        # Determine if we should purge the execution directory

        valid_dir = False
        purge = True

        if self.error != '':
            purge = False

        # Determine patterns from previous stages to ignore or possibly remove

        purge_list = []

        for f in ['mesh_input', 'restart']:
            if f in dir_files:
                purge_list.append(f)

        f = 'partition'
        if f in dir_files and (not self.partition_list):
            purge_list.append(f)

        # Determine files from this stage to ignore or to possibly remove

        for f in [self.package.solver, 'run_solver.sh']:
            if f in dir_files:
                purge_list.append(f)
        purge_list.extend(fnmatch.filter(dir_files, 'core*'))

        if self.user_scratch_files != None:
            for f in self.user_scratch_files:
                purge_list.extend = fnmatch.filter(dir_files, f)

        for f in purge_list:
            dir_files.remove(f)
            if purge:
                self.purge_result(f)

        if len(purge_list) > 0:
            valid_dir = True

        # Copy user sources, compile log, and xml file if present

        for f in [self.package.srcdir, 'compile.log', self.param]:
            if f in dir_files:
                valid_dir = True
                self.copy_result(f, purge)
                dir_files.remove(f)

        # Copy log files

        log_files = fnmatch.filter(dir_files, 'listing*')
        log_files.extend(fnmatch.filter(dir_files, '*.log'))
        log_files.extend(fnmatch.filter(dir_files, 'error*'))

        for f in log_files:
            self.copy_result(f, purge)
            dir_files.remove(f)

        if (len(log_files) > 0):
            valid_dir = True

        # Copy checkpoint files (in case of full disk, copying them
        # before other large data such as postprocessing output
        # increases chances of being able to continue).

        cpt = 'checkpoint'
        if cpt in dir_files:
            valid_dir = True
            self.copy_result(cpt, purge)
            dir_files.remove(cpt)

        # Now copy all other files

        if not valid_dir:
            return

        for f in dir_files:
            self.copy_result(f, purge)

    #---------------------------------------------------------------------------

    def summary_info(self, s):
        """
        Output summary data into file s
        """

        base_domain.summary_info(self, s)

        if not self.mesh_input:
            p = self.package.get_preprocessor()
            s.write('    preprocessor : ' + any_to_str(p) + '\n')

        if self.exec_partition:
            p = self.package.get_partitioner()
            s.write('    partitioner  : ' + any_to_str(p) + '\n')

        if self.exec_solver:
            s.write('    solver       : ' + self.solver_path + '\n')

#-------------------------------------------------------------------------------

# SYRTHES 3 coupling

class syrthes3_domain(base_domain):

    def __init__(self,
                 package,
                 name = None,
                 echo_comm = None,       # coupling verbosity
                 coupling_mode = 'MPI',  # 'MPI' or 'sockets'
                 coupled_apps = None,    # coupled domain names if n_domains > 1
                 pset_size = 1):         # size of processor set (for IBM BG/P)


        base_domain.__init__(self, package, name, 1, pset_size, pset_size)

        self.log_file = 'syrthes.log'

        # Directories, and files in case structure

        self.data_dir = None
        self.result_dir = None
        self.src_dir = None
        self.echo_comm = echo_comm

        self.set_coupling_mode(coupling_mode)

        self.coupled_apps = coupled_apps

        self.exec_solver = True

    #---------------------------------------------------------------------------

    def set_case_dir(self, case_dir):

        base_domain.set_case_dir(self, case_dir)

        # Names, directories, and files in case structure

        # No RESU dir for SYRTHES 3 case, as toplevel RESU_COUPLING
        # is used in coupled case, and the script does not handle
        # standalone SYRTHES 3 calculations (which require a different
        # executable)

        self.result_dir = None

    #---------------------------------------------------------------------------

    def set_coupling_mode(self, coupling_mode):

        # Check that coupling mode is either 'MPI' or 'sockets'
        coupling_modes = ('MPI', 'sockets')
        if coupling_mode not in coupling_modes:
            err_str = \
                'SYRTHES3 coupling mode "' + str(coupling_mode) + '" unknown.\n' \
                + 'Allowed modes: ' + str(coupling_modes) + '.\n'
            raise RunCaseError(err_str)

        # Coupling mode
        self.coupling_mode = coupling_mode

    #---------------------------------------------------------------------------

    def compile_and_link(self):
        """
        Compile and link user subroutines if necessary
        """

        # Check if there are files to compile in source path

        dir_files = os.listdir(self.src_dir)

        src_files = (fnmatch.filter(dir_files, '*.c')
                     + fnmatch.filter(dir_files, '*.[fF]'))

        copy_dir = None
        exec_src = None

        if len(src_files) > 0:

            # Add header files to list so as not to forget to copy them

            src_files = src_files + fnmatch.filter(dir_files, '*.h')

            # Copy source files to execution directory

            exec_src = os.path.join(self.exec_dir, 'src')
            os.mkdir(exec_src)

            for f in src_files:
                src_file = os.path.join(self.src_dir, f)
                dest_file = os.path.join(exec_src, f)
                shutil.copy2(src_file, dest_file)

        log_name = os.path.join(self.exec_dir, 'compile.log')
        log = open(log_name, 'w')

        retval = cs_compile.compile_and_link_syrthes(self.package,
                                                     exec_src,
                                                     self.exec_dir,
                                                     stdout=log,
                                                     stderr=log)

        log.close()

        if retval != 0:
            # In case of error, copy source to results directory now,
            # as no calculation is possible, then raise exception.
            for f in [self.package.srcdir, 'compile.log']:
                self.copy_result(f)
            raise RunCaseError('Compile or link error.')

        self.solver_path = os.path.join(self.exec_dir, 'syrthes')

    #---------------------------------------------------------------------------

    def solver_command(self, **kw):
        """
        Returns a tuple indicating SYRTHES's working directory,
        executable path, and associated command-line arguments.
        """

        wd = self.exec_dir              # Working directory
        exec_path = self.solver_path    # Executable

        # Build kernel command-line arguments

        args = ' --log syrthes.log'
        if self.echo_comm != None:
            args += ' --echo-comm ' + str(self.echo_comm)

        if self.coupling_mode == 'MPI':
            args += ' --app-name ' + os.path.basename(self.case_dir)
            if self.coupled_apps != None:
                args += ' --comm-mpi ' + any_to_str(self.coupled_apps)
            else:
                args += ' --comm-mpi '

        elif self.coupling_mode == 'sockets':
            if 'host_port' in kw:
                args += ' --comm-socket ' + any_to_str(kw['host_port'])

        # handled directly

        # Adjust for Valgrind if used

        if self.valgrind != None:
            args = self.solver_path + ' ' + args
            exec_path = self.valgrind

        return wd, exec_path, args

    #---------------------------------------------------------------------------

    def prepare_data(self):
        """
        Copy data to the execution directory
        """

        cwd = os.getcwd()
        os.chdir(self.exec_dir)

        syrthes_env = os.path.join(self.data_dir, 'syrthes.env')

        cmd = self.package.get_runcase_script('runcase_syrthes')
        cmd += ' -copy-data -syrthes-env=' + syrthes_env

        if run_command(cmd) != 0:
            raise RunCaseError

        os.chdir(cwd)

    #---------------------------------------------------------------------------

    def preprocess(self):
        """
        Empty for SYRTHES 3
        """
        pass

    #---------------------------------------------------------------------------

    def preprocess(self):
        """
        Run preprocessing stages (empty for SYRTHES 3)
        """

        return

    #---------------------------------------------------------------------------

    def copy_results(self):
        """
        Retrieve results from the execution directory
        """

        dir_files = os.listdir(self.src_dir)

        purge = True
        if self.error != '':
            purge = False

        # Copy user sources, compile log, and execution log if present

        for f in ['src', 'compile.log', 'syrthes.log']:
            if f in dir_files:
                self.copy_result(f, purge)
                dir_files.remove(f)

        if self.exec_dir == self.result_dir:
            return

        cwd = os.getcwd()
        os.chdir(self.exec_dir)

        cmd = self.package.get_runcase_script('runcase_syrthes') \
            + ' -copy-results -result-dir=' + self.result_dir

        if run_command(cmd) != 0:
            raise RunCaseError

        os.chdir(cwd)

    #---------------------------------------------------------------------------

    def summary_info(self, s):
        """
        Output summary data into file s
        """

        base_domain.summary_info(self, s)

        if self.solver_path:
            s.write('    SYRTHES      : ' + self.solver_path + '\n')

#-------------------------------------------------------------------------------

# SYRTHES 4 coupling

class syrthes_domain(base_domain):

    def __init__(self,
                 package,
                 cmd_line = None,     # Command line to define optional syrthes4 behaviour
                 name = None,
                 param = 'syrthes.data',
                 log_file = None,
                 n_procs_weight = None,
                 n_procs_min = 1,
                 n_procs_max = None,
                 n_procs_radiation = None):

        base_domain.__init__(self,
                             package,
                             name,
                             n_procs_weight,
                             n_procs_min,
                             n_procs_max)

        self.n_procs_radiation = n_procs_radiation

        # Additional parameters for Code_Saturne/SYRTHES coupling
        # Directories, and files in case structure

        self.cmd_line = cmd_line
        self.param = param

        self.logfile = log_file
        if self.logfile == None:
            self.logfile = 'syrthes.log'

        self.case_dir = None
        self.exec_dir = None
        self.data_dir = None
        self.src_dir = None
        self.result_dir = None
        self.echo_comm = None

        self.set_coupling_mode('MPI')

        self.exec_solver = True

        # Generation of SYRTHES case deferred until we know how
        # many processors are really required

        self.syrthes_case = None

    #---------------------------------------------------------------------------

    def set_case_dir(self, case_dir):

        base_domain.set_case_dir(self, case_dir)

        # Names, directories, and files in case structure

        self.data_dir = self.case_dir
        self.src_dir = self.case_dir

    #---------------------------------------------------------------------------

    def set_coupling_mode(self, coupling_mode):

        # Check that coupling mode is either 'MPI' or 'sockets'
        coupling_modes = ('MPI')
        if coupling_mode not in coupling_modes:
            err_str = \
                'SYRTHES4 coupling mode "' + str(coupling_mode) + '" unknown.\n' \
                + 'Allowed modes: ' + str(coupling_modes) + '.\n'
            raise RunCaseError(err_str)

        # Coupling mode
        self.coupling_mode = coupling_mode

    #---------------------------------------------------------------------------

    def set_exec_dir(self, exec_dir):

        if os.path.isabs(exec_dir):
            self.exec_dir = exec_dir
        else:
            self.exec_dir = os.path.join(self.case_dir, 'RESU', exec_dir)

        self.exec_dir = os.path.join(self.exec_dir, self.name)

        if not os.path.isdir(self.exec_dir):
            os.mkdir(self.exec_dir)

    #---------------------------------------------------------------------------

    def set_result_dir(self, name, given_dir = None):

        if given_dir == None:
            self.result_dir = os.path.join(self.result_dir,
                                           'RESU_' + self.name,
                                           name)
        else:
            self.result_dir = os.path.join(given_dir, self.name)

        if not os.path.isdir(self.result_dir):
            os.makedirs(self.result_dir)

    #---------------------------------------------------------------------------

    def solver_command(self, **kw):
        """
        Returns a tuple indicating SYRTHES's working directory,
        executable path, and associated command-line arguments.
        """

        wd = self.exec_dir              # Working directory
        exec_path = self.solver_path    # Executable

        # Build kernel command-line arguments

        args = ''

        args += ' -d ' + self.syrthes_case.data_file
        args += ' -n ' + str(self.syrthes_case.n_procs)

        if self.syrthes_case.n_procs_ray > 0:
            args += ' -r ' + str(self.n_procs_ray)

        if self.coupling_mode == 'MPI':

            args += ' --name ' + self.name

        # Output to a logfile
        args += ' --log ' + self.logfile

        # Adjust for Valgrind if used

        if self.valgrind != None:
            args = self.solver_path + ' ' + args
            exec_path = self.valgrind

        return wd, exec_path, args

    #---------------------------------------------------------------------------

    def prepare_data(self):
        """
        Fill SYRTHES domain structure
        Copy data to the execution directory
        Compile and link syrthes executable
        """

        # Build command-line arguments

        args = '-d ' + os.path.join(self.case_dir, self.param)
        args += ' --name ' + self.name

        if self.n_procs != None and self.n_procs != 1:
            args += ' -n ' + str(self.n_procs)

        if self.n_procs_radiation > 0:
            args += ' -r ' + str(self.n_procs_radiation)

        if self.data_dir != None:
            args += ' --data-dir ' + str(self.data_dir)

        if self.src_dir != None:
            args += ' --src-dir ' + str(self.src_dir)

        if self.exec_dir != None:
            args += ' --exec-dir ' + str(self.exec_dir)

        if self.cmd_line != None and len(self.cmd_line) > 0:
            args += ' ' + self.cmd_line

        # Define syrthes case structure

        try:
            import syrthes
        except Exception:
            raise RunCaseError("Cannot locate SYRTHES installation.\n")
            sys.exit(1)

        self.syrthes_case = syrthes.process_cmd_line(args.split())

        if self.syrthes_case.logfile == None:
            self.syrthes_case.set_logfile(self.logfile)
        else:
            self.logfile = self.syrthes_case.logfile

        # Read data file and store parameters

        self.syrthes_case.read_data_file()

        # Build exec_srcdir

        exec_srcdir = os.path.join(self.exec_dir, 'src')
        os.makedirs(exec_srcdir)

        # Preparation of the execution directory and compile and link executable

        compile_logname = os.path.join(self.exec_dir, 'compile.log')

        retval = self.syrthes_case.prepare_run(exec_srcdir, compile_logname)

        self.copy_result(compile_logname)

        if retval != 0:
            err_str = '\n   Error during the SYRTHES preparation step\n'
            if retval == 1:
                err_str += '   Error during data copy\n'
            elif retval == 2:
                err_str += '   Error during syrthes compilation and link\n'
                # Copy source to results directory, as no run is possible
                for f in ['src', 'compile.log']:
                    self.copy_result(f)
            raise RunCaseError(err_str)

        # Set executable

        self.solver_path = os.path.join(self.exec_dir, 'syrthes')

    #---------------------------------------------------------------------------

    def preprocess(self):
        """
        Read syrthes.data file
        Partition mesh for parallel run if required by user
        """

        # Sumary of the parameters
        self.syrthes_case.dump()

        # Initialize output file if needed
        self.syrthes_case.logfile_init()

        # Pre-processing (including partitioning only if SYRTHES
        # computation is done in parallel)
        retval = self.syrthes_case.preprocessing()
        if retval != 0:
            err_str = '\n  Error during the SYRTHES preprocessing step\n'
            raise RunCaseError(err_str)

    #---------------------------------------------------------------------------

    def copy_results(self):
        """
        Retrieve results from the execution directory
        """

        if self.exec_dir == self.result_dir:
            return

        retval = self.syrthes_case.save_results(save_dir = self.result_dir,
                                                horodat = False,
                                                overwrite = True)
        if retval != 0:
            err_str = '\n   Error saving SYRTHES results\n'
            raise RunCaseError(err_str)

    #---------------------------------------------------------------------------

    def summary_info(self, s):
        """
        Output summary data into file s
        """

        base_domain.summary_info(self, s)

        if self.solver_path:
            s.write('    SYRTHES      : ' + self.solver_path + '\n')

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
