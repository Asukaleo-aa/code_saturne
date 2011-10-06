#ifndef __CS_SYR3_COMM_H__
#define __CS_SYR3_COMM_H__

/*============================================================================
 * Communication with SYRTHES 3
 *============================================================================*/

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

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

#define CS_SYR3_COMM_FIN_FICHIER                           "EOF"

#define CS_SYR3_COMM_H_LEN       32   /* Length of a header name */

/* Socket communications: we suppose a maximum of 8 coupled SYRTHES instances;
   this value may be modified through the CS_SYR3_COMM_SOCKET_NBR_MAX
   environment variable */

/*============================================================================
 * Type definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Message type
 *----------------------------------------------------------------------------*/

typedef enum {

  CS_SYR3_COMM_TYPE_NONE,     /* No communication (pre-initialization) */
  CS_SYR3_COMM_TYPE_MPI,      /* MPI messages */
  CS_SYR3_COMM_TYPE_SOCKET    /* IP sockets */

} cs_syr3_comm_type_t;

/* Pointer associated with an opaque communicator structure. */

typedef struct _cs_syr3_comm_t cs_syr3_comm_t;

/* Structure used to save message header data, to simplify its use. */

typedef struct {

  char       sec_name[CS_SYR3_COMM_H_LEN + 1];
  cs_int_t   n_elts;
  cs_type_t  elt_type;

} cs_syr3_comm_msg_header_t;

/*============================================================================
 *  Global variables
 *============================================================================*/

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Initialize a communication
 *
 * parameters:
 *   number,       <-- coupling number
 *   proc_rank,    <-- communicating process rank (< 0 if using sockets)
 *   type,         <-- communication type
 *   echo          <-- echo on main output (< 0 if none, header if 0,
 *                     n first and last elements if n)
 *
 * returns:
 *   pointer to communication structure
 *----------------------------------------------------------------------------*/

cs_syr3_comm_t *
cs_syr3_comm_initialize(int                  number,
#if defined(HAVE_MPI)
                        int                  proc_rank,
#endif
                        cs_syr3_comm_type_t  type,
                        cs_int_t             echo);

/*----------------------------------------------------------------------------
 * Finalize a communication
 *----------------------------------------------------------------------------*/

cs_syr3_comm_t *
cs_syr3_comm_finalize(cs_syr3_comm_t *comm);

/*----------------------------------------------------------------------------
 * Return a pointer to a communicator name
 *
 * parameters:
 *   comm <-- communicator
 *
 * returns:
 *   pointer to communicator name
 *----------------------------------------------------------------------------*/

const char *
cs_syr3_comm_get_name(const cs_syr3_comm_t  *comm);

/*----------------------------------------------------------------------------
 * Send message
 *
 * parameters:
 *   nom_rub  <-- section name
 *   n_elts   <-- number of elements
 *   elt_type <-- element type if n_elts > 0
 *   elts     <-- elements if n_elts > 0
 *   comm     <-- communicator
 *----------------------------------------------------------------------------*/

void
cs_syr3_comm_send_message(const char             nom_rub[CS_SYR3_COMM_H_LEN],
                          cs_int_t               n_elts,
                          cs_type_t              elt_type,
                          void                  *elts,
                          const cs_syr3_comm_t  *comm);

/*----------------------------------------------------------------------------
 * Receive message header
 *
 * parameters:
 *   header --> message header
 *   comm   <-- communicator
 *
 * returns
 *   number of elements in message body
 *----------------------------------------------------------------------------*/

cs_int_t
cs_syr3_comm_receive_header(cs_syr3_comm_msg_header_t  *header,
                            const cs_syr3_comm_t       *comm);

/*----------------------------------------------------------------------------
 * Receive a message body
 *
 * parameters:
 *   header <-- message header
 *   elt    --> received body values
 *   comm   <-- communicator
 *----------------------------------------------------------------------------*/

void
cs_syr3_comm_receive_body(const cs_syr3_comm_msg_header_t  *header,
                          void                             *elt,
                          const cs_syr3_comm_t             *comm);

#if defined(HAVE_SOCKET)

/*----------------------------------------------------------------------------
 * Open an IP socket to prepare for this communication mode
 *
 * parameters:
 *   port_num <-- port number (only used for rank 0; automatic on others)
 *----------------------------------------------------------------------------*/

void
cs_syr3_comm_init_socket(int port_num);

/*----------------------------------------------------------------------------
 * Close an IP socket associated with this communication mode
 *----------------------------------------------------------------------------*/

void
cs_syr3_comm_finalize_socket(void);

#endif /* HAVE_SOCKET */

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_SYR3_COMM_H__ */
