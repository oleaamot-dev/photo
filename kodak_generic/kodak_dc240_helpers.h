/*
 *      Copyright (C) 1999 Randy Scott <scottr@wwa.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published 
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "kodak_generic.h"
#include "state_machine.h"

typedef enum
{
   CC_RESPONSE_LAST_PACKET,  /* This is the last Rx packet */
   CC_RESPONSE_MORE_PACKETS, /* More Rx packets expected */
} CC_RESPONSE_TYPE;

/* Complex command description structure */
typedef struct
{
   int descriptor;
   int tx_packet_size;
   unsigned char *(*tx_func)(int);
   int rx_packet_size;
   CC_RESPONSE_TYPE (*rx_func)(int, unsigned char *);

   /* Internal use only, don't need to fill in */
   unsigned char packet_response;
   unsigned char *tx_buffer;
   CC_RESPONSE_TYPE resp;
   int loop_top;
} CC_STRUCT_TYPE;

void kdc240_restart(void);
void kdc240_command(KODAK_CAMERA_TYPE, unsigned char, ...);
void kdc240_complex_command(CC_STRUCT_TYPE *, KODAK_CAMERA_TYPE,
   unsigned char, ...);
