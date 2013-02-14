/*============================================================================
 * Code_Saturne documentation page
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2013 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*-----------------------------------------------------------------------------*/

/*!
  \page cs_user_extra_operations_examples cs_user_extra_operations.f90
 

  \section intro Introduction

  This page provides several examples of code blocks that may be used
  to perform energy balnce, 1D profiles, etc.
  in \ref cs_user_boundary_conditions.


  \section cs_user_extra_op_examples Extra operations examples


  \subsection energy_balance Energy balance 

  \subsubsection loc_var Local variables to be added

  The following local variables need to be defined for the examples
  in this section:

  \snippet cs_user_extra_operations-energy_balance.f90 loc_var_dec

  \subsubsection init Initialization and finalization

  The following initialization block needs to be added for the following examples:

  \snippet cs_user_extra_operations-energy_balance.f90 init

  Ad the end of the subroutine, it is recommended to deallocate the work array:

  \snippet cs_user_extra_operations-energy_balance.f90 finalize

  In theory Fortran 95 deallocates locally-allocated arrays automatically,
  but deallocating arrays in a symmetric manner to their allocation is good
  practice, and it avoids using a different logic for C and Fortran.

  \subsubsection body Body

  This example computes energy balance relative to temperature
  We assume that we want to compute balances  (convective and diffusive)
  at the boundaries of the calculation domain represented below
  (with boundaries marked by colors).
  
  The scalar considered if the temperature. We will also use the
  specific heat (to obtain balances in Joules)
  
  
  Domain and associated boundary colors:
  - 2, 4, 7 : adiabatic walls
  - 6       : wall with fixed temperature
  - 3       : inlet
  - 5       : outlet
  - 1       : symmetry
  
  
  To ensure calculations have physical meaning, it is best to use
  a spatially uniform time step (\ref idtvar = 0 or 1).
  In addition, when restarting a calculation, the balance is
  incorrect if \ref inpdt0 = 1 (visct not initialized and t(n-1) not known)
  
  
  Temperature variable
  - ivar = \ref isca(\ref  iscalt) (use rtp(iel, ivar))
  
  Boundary coefficients coefap/coefbp are those of \ref ivarfl(ivar)
  
  
  The balance at time step n is equal to:
  
  \f[
  \begin{array}{r c l}
  Blance^n &=& \displaystyle
               \sum_{\celli=1}^{\ncell}
                  \norm{\vol{\celli}} C_p \rho_\celli
                  \left(T_\celli^{n-1} -T_\celli^n \right)  \\
           &+& \displaystyle
               \sum_{\fib}
                  C_p \Delta t_\celli \norm{\vect{S}_\ib}
                  \left(A_\ib^f + B_\ib^f T_\celli^n \right) \\
           &+& \displaystyle
               \sum_{\fib}
                  C_p \Delta t_\celli \dot{m}_\ib
                  \left(A_\ib^g + B_\ib^g T_\celli^n \right)
  \end{array}
  \f]
  
  The first term is negative if the amount of energy in the volume
  has decreased (it is 0 in a steady regime).
  
  The other terms (convection, diffusion) are positive if the amount
  of energy in the volume has increased due to boundary conditions.
  
  In a steady regime, a positive balance thus indicates an energy gain.
  
  
  With \f$ \rho \f$ (\c rom) calculated using the density law from the
  \ref usphyv subroutine, for example:
  
  \f[
  \rho^{n-1}_\celli = P_0 / \left( R T_\celli^{n-1} + T_0 \right)
  \f]
  where \f$ R\f$ is \c rr and \f$ T_0 \f$ is \c tkelv.
  
  
  \f$ C_p \f$ and \f$ \lambda/C_p \f$ may vary.
  
  
  Here is the corresponding code:
  
  \snippet cs_user_extra_operations-energy_balance.f90 example_1
  

  \subsection oned_profile Extract a 1D profile 
 
  This is an example of \ref cs_user_extra_operations which performs 1D profile.

  \subsubsection loc_var Local variables to be added

  \snippet cs_user_extra_operations-extract_1d_profile.f90 loc_var_dec

  \subsubsection body Body
 
   We seek here to extract the profile of U, V, W, k and epsilon on an
   arbitrary 1D curve based on a curvilear abscissa.
   The profile is described in the 'profile.dat' file (do not forget to
   define it as user data in the run script).
 
   - the curve used here is the segment: [(0;0;0),(0;0.1;0)], but the
     generalization to an arbitrary curve is simple.
   - the routine handles parallelism an periodicity, as well as the different
     turbulence models.
   - the 1D curve is discretized into 'npoint' points. For each of these
     points, we search for the closest cell center and we output the variable
     values at this cell center. For better consistency, the coordinate
     which is output is that of the cell center (instead of the initial point).
   - we avoid using the same cell multiple times (in case several points
     an the curve are associated with the same cell).
 
  Here is the corresponding code:
 
  \snippet cs_user_extra_operations-extract_1d_profile.f90 example_1
 
*/
