#!/usr/bin/env python

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

import os

#===============================================================================
# Local functions
#===============================================================================

def domain_auto_restart(domain, n_add):
    """
    Select latest valid checkpoint for restart, and
    create ficstp file to add n_add time steps.
    """

    from cs_exec_environment import get_command_output

    results_dir = os.path.abspath(os.path.join(self.result_dir, '..'))
    results = os.listdir(results_dir)
    results.sort(reverse=True)
    for r in results:
        m = os.path.join(results_dir, r, 'checkpoint', 'main')
        if os.path.isfile(m):
            try:
                cmd = self.package.get_io_dump()
                cmd += ' --section nbre_pas_de_temps --extract ' + m
                res = get_command_output(cmd)
                n_steps = int(get_command_output(cmd))
            except Exception:
                print('checkpoint of result: ' + r + ' does not seem usable')
                continue
            info_line = 'Restart from iterations ' + str(n_steps)
            n_steps += n_add
            info_line += ' to ' + str(n_steps)
            print(info_line)
            f = open(os.path.join(self.exec_dir, 'control_file'), 'w')
            l = ['#Target number of time steps, determined by script\n',
                 str(n_steps)]
            f.writelines(l)
            f.close()
            domain.restart_input = os.path.join('RESU',
                                                r,
                                                'checkpoint')
            print('using ' + domain.restart_input)
            break

    return

#===============================================================================
# Defining parameters for a calculation domain
#===============================================================================

def domain_prepare_data_add(domain):
    """
    Additional steps to prepare data
    (called in data preparation stage, between copy of files
    in DATA and copy of link of restart files as defined by domain).
    """

    # Example: select latest valid checkpoint file for restart

    if False:
        domain_auto_restart(domain, 200)

    return

#-------------------------------------------------------------------------------

def domain_copy_results_add(domain):
    """
    Additional steps to copy results or cleanup execution directory
    (called at beginning of data copy stage).
    """

    # Example: clean some temporary files

    if False:
        import fnmatch
        dir_files = os.listdir(self.exec_dir)
        tmp_files = (fnmatch.filter(dir_files, '*.tmp')
                     + fnmatch.filter(dir_files, '*.fort'))
        for f in tmp_files:
            os.remove(os.path.join(self.exec_dir, f))

    return

#-------------------------------------------------------------------------------

def define_domain_parameters(domain):
    """
    Define domain execution parameters.
    """

    # Read parameters file
    # (already done just prior to this stage when
    # running script with --param option)

    if False:
        domain.read_parameter_file('param2.xml')

    # Reusing output from previous runs
    #----------------------------------

    # To use the output of a previous Preprocessor (mesh import) run, set:
    #   domain.mesh_input = 'RESU/<run_id>/mesh_input'

    # To use the mesh output of a previous solver run, set:
    #   domain.mesh_input = 'RESU/<run_id>/mesh_output'

    # To reuse a previous partitioning, set:
    #   domain.partition_input = 'RESU/<run_id>/partition'

    # To continue a calculation from a previous checkpoint, set:
    #  domain.restart_input = 'RESU/<run_id>/checkpoint'

    if domain.param == None:
        domain.mesh_input = None
        domain.partition_input = None
        domain.restart_input = None

    # Defining meshes to import (only if domain.mesh_input = None)
    #-------------------------------------------------------------

    # A case-specific mesh directory can be defined through domain.mesh_dir.

    if domain.param == None:
        domain.mesh_dir = None

    # Mesh names can be given as absolute path names; otherwise, file are
    # searched in the following directories, in the order:
    #   - the   case mesh database directory, domain.mesh_dir (if defined)
    #   - the  study mesh database directory, ../MESH directory (if present)
    #   - the   user mesh database directory, see ~/.code_saturne.cfg
    #   - the global mesh database directory, see $prefix/etc/code_saturne.cfg

    # Meshes should be defined as a list of strings defining mesh
    # file names. If preprocessor options must be used for some meshes,
    # a tuple containing a file name followed by additional options
    # may be used instead of a simple file name, for example:
    #   domain.meshes = ['part1.des',
    #                    ('part2_med', '--format med --num 2', '--reorient'),
    #                    '~/meshdatabase/part3.unv']

    if domain.param == None:
        domain.meshes = None

    # Logging arguments
    #------------------

    # Command-line arguments useful for logging, or determining the calculation
    # type may be defined here, for example:
    #   domain.logging_args = '--logp 1'

    if domain.param == None:
        domain.logging_args = None

    # Solver options
    #---------------

    # Running the solver may be deactivated by setting
    #   domain.exec_solver = False

    # The type of run may be determined using domain.solver_args.
    # For example:
    #   domain.solver_args = '--quality'
    # allows activation of elementary mesh quality and gradient tests
    # (for a fixed function: sin(x+2y+3z), while:
    #   domain.solver_args = '--benchmark'
    # allows running  basic linear algebra operation benchmarks.
    # To run the solver's preprocessing stage only (mesh joining, smoothing,
    # and other modifications), use:
    #   domain.solver_args = '--preprocess'

    if domain.param == None:
        domain.exec_solver = True
        domain.solver_args = None

    # Compile and build options
    #--------------------------

    # Additionnal compiler flags may be passed to the C, C++, or Fortran
    # compilers, and libraries may be added, in case linking of user
    # subroutines against external libraries is needed.

    # Note that compiler flags will be added before the default flags;
    # this helps ensure added search paths have priority, but also implies
    # that user optimization options may be superceded by the default ones.

    if domain.param == None:
        domain.compile_cflags = None
        domain.compile_cxxflags = None
        domain.compile_fcflags = None
        domain.compile_libs = None

    # Debugging options
    #------------------

    # The solver may be run through Valgrind if this memory-checking tool
    # is available. In this case, domain.valgrind should contain the matching
    # command-line arguments, such as:
    #   domain.valgrind = 'valgrind --tool=memcheck'

    if domain.param == None:
        domain.valgrind = None

    # import pprint
    # pprint.pprint(domain.__dict__)

    return

#-------------------------------------------------------------------------------

def define_case_parameters(case):
    """
    Define global case execution parameters.
    """

    # The parameters defined here apply for the whole calculation.
    # In case of coupled calculations with multiple domains,
    # this function is ignored, and the matching parameters
    # should be set using the runcase_coupling script.

    # Number of MPI processes (automatic if none).
    #------------------------

    # Warning: this value should be set to None when running under a batch
    #          system/resource manager, unless a different number of
    #          processes than the number allocated is required.

    # case.n_procs = None

    # Temporary execution directory
    #-------------------------------

    # The temporary directory in which the calculation will run may be
    # set by defining case.scratchdir. If set to None, the calculation
    # will be run in the case domain's results directory.

    # Using separate results and execution directories is only
    # of interest on machines which have multiple filesystems,
    # with different filesystem recommendations for storage and
    # computations.

    # The default (initial) value may be defined by the
    #   - the   user mesh database directory, see ~/.code_saturne.cfg
    #   - the global mesh database directory, see $prefix/etc/code_saturne.cfg

    # If a value is specified, the temporary directory will be of the form:
    #  <SCRATCHDIR>/tmp_Saturne/<STUDY>.<CASE>.<DATE>

    # case.scratchdir = None

    return

#-------------------------------------------------------------------------------

def define_mpi_environment(mpi_env):
    """
    Redefine global MPI execution command parameters.
    """

    # The parameters defined here apply for the whole calculation.
    # In case of coupled calculations with multiple domains,
    # this function is ignored, and the matching parameters
    # should be set using the runcase_coupling script.

    # Some MPI execution parameter fields may need to be modified in case of
    # incorrect defaults
    # (due to the wide variety of MPI implementations and build options,
    # the default configuration may not give correct values in some cases).

    # mpi_env.bindir             : path to mpi binaries

    # mpi_env.mpiexec            : mpiexec, mpirun, or equivalent command
    # mpi_env.mpiexec_opts       : mpiexec command options

    # mpi_env.mpiexec_args       : option to pass arguments (usually None,
    #                              or '-args')
    # mpi_env.mpiexec_exe        : option to define executable (usually None,
    #                              or '-exe')
    # mpi_env.mpiexec_n          : option to define number of ranks
    #                              (usually ' -n ' or ' -np '; trailing
    #                              whitespace is significant, as SLURM for
    #                              example requires ' -n', for a -n<n_procs>
    #                              type syntax)
    # mpi_env.mpiexec_n_per_node : option to define number of ranks per node
    #                              (e.g. ' -ppn 6 ', ' --ranks-per-node 16 ')
    # mpi_env.mpiexec_separator  : separator after MPI options section
    #                              (usually None, ':' for Blue Gene/Q)

    # mpi_env.gen_hostsfile      : shell command to generate hostsfile if
    #                              required. When using a fixed hostfile, passing
    #                              it in mpiexec_opts is simpler, so this command
    #                              is only useful when using a resource manager
    #                              which is not handled correctly by the MPI
    #                              library
    # mpi_env.del_hostsfile      : shell command to delete hostsfile if required

    # mpi_env.mpiboot            : command to start environment (e.g. 'mpdboot'
    #                              for some MPICH2 configurations using MPD,
    #                              'lamboot' required for obsolescent LAM-MPI)
    # mpi_env.mpihalt            : command to halt environment (e.g. 'mpdallexit'
    #                              after 'mpdboot', lamhalt after 'lamboot')

    # mpi_env.mpmd               : Multiple program/multiple data mode for
    #                              couplings:
    #                              MPI_MPMD_mpiexec (mpiexec ':'-separated
    #                                                syntax), or
    #                              MPI_MPMD_configfile (mpiexec -configfile), or
    #                              MPI_MPMD_script, or
    #                              MPI_MPMD_execve

    return

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
