/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2011 EDF S.A.

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

/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>

#include "bft_error.h"
#include "bft_mem_usage.h"
#include "bft_sys_info.h"
#include "bft_timer.h"

#if defined(__MINGW32__)
#define sleep Sleep
#include <windows.h>
#endif

int
main (int argc, char *argv[])
{
  int i;
  double walltime, cputime;

  /* Internationalization */

#ifdef HAVE_SETLOCALE
  if (!setlocale (LC_ALL,"")) {
#if defined (DEBUG)
     printf("locale not supported by C library"
            " or bad LANG environment variable");
#endif
  }
#endif /* HAVE_SETLOCALE */

#if ENABLE_NLS
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  /* Initialization and environment */

  (void)bft_timer_wtime();

  printf("command line arguments\n");
  for (i = 0; i < argc; i++)
    printf("%s\n", argv[i]);

  if (bft_sys_info_cpu() != NULL)
    printf("CPU type: %s\n", bft_sys_info_cpu());

  sleep(1);

  walltime = bft_timer_wtime();
  cputime  = bft_timer_cpu_time();

  printf("Wallclock time: %f (method: %s)\n",
         walltime, bft_timer_wtime_method());
  printf("CPU time: %f (method: %s)\n",
         cputime, bft_timer_cpu_time_method());

  exit (EXIT_SUCCESS);
}
