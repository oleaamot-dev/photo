/*
 *      Copyright (C) 1999 Randy Scott <scottr@wwa.com>
 *
 *      Linux USB support from David Brownell <david-b@pacbell.net>.
 *
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

/*==============================================================================
* Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "state_machine.h"
#include "kodak_generic.h"
#include "kodak_command.h"
#include "kodak_dc240.h"
#include "kodak_dc240_helpers.h"

/*==============================================================================
* Preprocessor defines
==============================================================================*/

/*
 * Splitting reads/writes of messages doesn't work with USB, but I'm leaving
 * that serial-only code in for the moment so it's easy to turn it back on
 * in emergencies.  Eventually the split message code should go.
 */
#undef DC2XX_SPLIT_MESSAGES

#define TRANSMIT_COMMAND(desc) \
  { (desc), 8, 1, kdc240_get_command, kdc240_read_ack, kdc240_read_ack_error }

#define COMMAND_COMPLETE(desc) \
  { (desc), 0, 1, NULL, kdc240_read_cmd_complete, kdc240_read_cmd_complete_error }

#define HIGH_BAUD_RATE      115200
#define HIGH_BAUD_RATE_CODE 0x1152

#define MAX_CHECKSUM_ERRORS 8

/*==============================================================================
* Types
==============================================================================*/

/*==============================================================================
* Local Variables
==============================================================================*/
static BOOLEAN camera_init = FALSE;
static int checksum_errors = 0;

/*==============================================================================
* Local Function Prototypes
==============================================================================*/
void kdc240_baud_command(void);

unsigned char * kdc240_get_command (int);
NEXT_STATE_TYPE kdc240_read_ack (int, unsigned char *);
NEXT_STATE_TYPE kdc240_read_ack_error (int, ERROR_TYPE);
NEXT_STATE_TYPE kdc240_read_cmd_complete (int, unsigned char *);
NEXT_STATE_TYPE kdc240_read_cmd_complete_error (int, ERROR_TYPE);

/*==============================================================================
* Local Functions
==============================================================================*/
/*******************************************************************************
* FUNCTION: kdc240_get_command
*
* DESCRIPTION:
*******************************************************************************/
unsigned char *
kdc240_get_command
(
   int descriptor
)
{
   return kodak_command_get(descriptor);
}

/*******************************************************************************
* FUNCTION: kdc240_read_ack
*
* DESCRIPTION:
*******************************************************************************/
NEXT_STATE_TYPE
kdc240_read_ack
(
   int descriptor,
   unsigned char *data
)
{
   if (*data == COMMAND_RESPONSE_ACK)
   {
      return STATE_NEXT;
   }
   else
   {
      printf ("kdc240_read_ack: got a 0x%02X\n", *data);
      camera_init = FALSE;
      return STATE_ABORT;
   }
}

/*******************************************************************************
* FUNCTION: kdc240_read_ack_error
*
* DESCRIPTION:
*******************************************************************************/
NEXT_STATE_TYPE
kdc240_read_ack_error
(
   int descriptor,
   ERROR_TYPE error
)
{
   printf ("kdc240_read_ack_error: read error\n");
   camera_init = FALSE;
   return STATE_ABORT;
}

/*******************************************************************************
* FUNCTION: kdc240_read_cmd_complete
*
* DESCRIPTION:
*******************************************************************************/
NEXT_STATE_TYPE
kdc240_read_cmd_complete
(
   int descriptor,
   unsigned char *data
)
{
   if (*data == COMMAND_RESPONSE_COMPLETE)
   {
      return STATE_NEXT;
   }
   else if (*data == COMMAND_RESPONSE_BUSY)
   {
      printf ("kdc_read_cmd_complete: busy\n");
      return STATE_REPEAT;
   }
   else
   {
      printf ("kdc_read_cmd_complete: got a 0x%02X\n", *data);
      camera_init = FALSE;
      return STATE_ABORT;
   }
}

/*******************************************************************************
* FUNCTION: kdc240_read_cmd_complete_error
*
* DESCRIPTION:
*******************************************************************************/
NEXT_STATE_TYPE
kdc240_read_cmd_complete_error
(
   int descriptor,
   ERROR_TYPE error
)
{
   printf ("kdc240_read_cmd_complete_error: read error\n");
   camera_init = FALSE;
   return STATE_ABORT;
}

NEXT_STATE_TYPE
kdc240_baud_switch
(
   int descriptor,
   unsigned char *packet
)
{
   printf ("kdc240_baud_switch: switching to 115200\n");

   /* Pause after changing baud rate */
   sleep(1);

   /* Set the new baud rate */
   state_machine_set_baud(machine, HIGH_BAUD_RATE);

   /* Indicate camera is initialized */
   camera_init = TRUE;

   /* Pause again */
   sleep(1);

   return STATE_NEXT;
}

#ifdef DC2XX_SPLIT_MESSAGES
NEXT_STATE_TYPE
kdc240_read_packet_control
(
   CC_STRUCT_TYPE *cc_struct,
   unsigned char *packet
)
{
   if (*packet != 0x01)
   {
      printf ("kdc240_read_packet_control: bad control 0x%02X\n", *packet);
      cc_struct->rx_func(cc_struct->descriptor, NULL);
      camera_init = FALSE;
      return STATE_ABORT;
   }

   checksum_errors = 0;

   return STATE_NEXT;
}

NEXT_STATE_TYPE
kdc240_read_packet_control_error
(
   CC_STRUCT_TYPE *cc_struct,
   ERROR_TYPE error
)
{
   printf ("kdc240_read_packet_control_error: read error\n");
   return STATE_ABORT;
}
#endif

NEXT_STATE_TYPE
kdc240_read_packet
(
   CC_STRUCT_TYPE *cc_struct,
   unsigned char *packet
)
{
   int i;
   int checksum = 0;

#ifndef DC2XX_SPLIT_MESSAGES
   if (*packet != 0x01)
   {
      printf ("kdc240_read_packet_control: bad control 0x%02X\n", *packet);
      cc_struct->rx_func(cc_struct->descriptor, NULL);
      camera_init = FALSE;
      return STATE_ABORT;
   }

   checksum_errors = 0;

   for (i = 1; i <= cc_struct->rx_packet_size; i++)
#else
   for (i = 0; i < cc_struct->rx_packet_size; i++)
#endif
   {
      checksum ^= packet[i];
   }

   if (checksum != packet[i])
   {
      printf ("kdc240_read_packet: checksum error\n");
      if (++checksum_errors > MAX_CHECKSUM_ERRORS)
      {
         cc_struct->packet_response = COMMAND_RESPONSE_CANCEL;
         cc_struct->rx_func(cc_struct->descriptor, NULL);
      }
      else
      {
         cc_struct->packet_response = COMMAND_RESPONSE_ILL_PACKET;
      }
   }
   else
   {
      cc_struct->packet_response = COMMAND_RESPONSE_CORRECT;
#ifdef DC2XX_SPLIT_MESSAGES
      cc_struct->resp = cc_struct->rx_func(cc_struct->descriptor, packet);
#else
      cc_struct->resp = cc_struct->rx_func(cc_struct->descriptor, packet + 1);
#endif
   }

   checksum_errors = 0;

   return STATE_NEXT;
}

NEXT_STATE_TYPE
kdc240_read_packet_error
(
   CC_STRUCT_TYPE *cc_struct,
   ERROR_TYPE error
)
{
#ifndef DC2XX_SPLIT_MESSAGES
   /* XXX previously just returned ABORT if couldn't read 1st byte  */
#warning "If not using USB, missing camera won't ABORT quickly..."
#endif 

   printf ("kdc240_read_packet_error: read error\n");

   cc_struct->packet_response = COMMAND_RESPONSE_CANCEL;
   cc_struct->rx_func(cc_struct->descriptor, NULL);
   return STATE_NEXT;
}

unsigned char *
kdc240_send_packet_response
(
   CC_STRUCT_TYPE *cc_struct
)
{
   return &(cc_struct->packet_response);
}

NEXT_STATE_TYPE
kdc240_packet_response_done
(
   CC_STRUCT_TYPE *cc_struct,
   unsigned char *packet
)
{
   if (cc_struct->packet_response == COMMAND_RESPONSE_ILL_PACKET)
   {
      return cc_struct->loop_top;
   }
   else if (cc_struct->packet_response == COMMAND_RESPONSE_CANCEL)
   {
      return STATE_ABORT;
   }
   else if (cc_struct->resp == CC_RESPONSE_LAST_PACKET)
   {
      return STATE_NEXT;
   }
   else
   {
      return cc_struct->loop_top;
   }
}

#ifdef DC2XX_SPLIT_MESSAGES
static unsigned char *
kdc240_send_packet_control
(
   CC_STRUCT_TYPE *cc_struct
)
{
   cc_struct->packet_response = 0x80;
   return &(cc_struct->packet_response);
}

static NEXT_STATE_TYPE
kdc240_packet_control_done
(
   CC_STRUCT_TYPE *cc_struct,
   unsigned char *packet
)
{
   return STATE_NEXT;
}
#endif

static unsigned char *
kdc240_send_packet
(
   CC_STRUCT_TYPE *cc_struct
)
{
   unsigned char *data;
   unsigned char *buffer;
   int checksum;
   int len = cc_struct->tx_packet_size + 1;
   int i;

#ifndef	DC2XX_SPLIT_MESSAGES
   len++;
#endif

   buffer = (unsigned char *)malloc(len);
   data = cc_struct->tx_func(cc_struct->descriptor);

   checksum = 0;
#ifndef	DC2XX_SPLIT_MESSAGES
   buffer [0] = 0x80;
   for (i = 1; i <= cc_struct->tx_packet_size; i++)
#else
   for (i = 0; i < cc_struct->tx_packet_size; i++)
#endif
   {
      buffer[i] = data[i];
      checksum ^= data[i];
   }

   buffer[i] = checksum;

   cc_struct->tx_buffer = buffer;

   return buffer;
}

static NEXT_STATE_TYPE
kdc240_read_packet_resp
(
   CC_STRUCT_TYPE *cc_struct,
   unsigned char *packet
)
{
   free(cc_struct->tx_buffer);

   if (*packet == COMMAND_RESPONSE_CORRECT)
   {
      return STATE_NEXT;
   }
   else
   {
      printf ("kdc240_read_packet_resp: bad response 0x%02X\n", *packet);
      return STATE_1;
   }
}

static NEXT_STATE_TYPE
kdc240_read_packet_resp_error
(
   CC_STRUCT_TYPE *cc_struct,
   ERROR_TYPE error
)
{
   free(cc_struct->tx_buffer);

   printf ("kdc240_read_packet_resp_error: aborting\n");
   return STATE_ABORT;
}

/*==============================================================================
* Global Functions
==============================================================================*/
/*******************************************************************************
* FUNCTION: kdc240_restart
*
* DESCRIPTION:
*******************************************************************************/
void
kdc240_restart
(
   void
)  
{  
   /* If we are using USB, there is no need to initialize the camera */
   if (machine->is_usb)
   {
      camera_init = TRUE;
   }

   /* Else, we need to fool around to make sure the camera is going */
   else
   {
      state_machine_assert_break(machine);
      kdc240_baud_command();
   }
}

/*******************************************************************************
* FUNCTION: kdc240_command
*
* DESCRIPTION:
*
*    Creates and sends a simple command.  Simple commands are defined as those
*    involving no data packets sent between the computer and camera.
*
*******************************************************************************/
void
kdc240_command
(
   KODAK_CAMERA_TYPE camera,
   unsigned char command,
   ...
)
{
   int desc;
   va_list ap;

   if (!camera_init)
   {
      /* Re-initialize the state machine */
      state_machine_reinitialize(machine);

      if (!camera_init)
      {
         printf ("kdc240_command: Is your camera turned on?\n");
         return;
      }
   }

   va_start(ap, command);
   desc = kodak_command_vcreate(camera, command, ap);
   va_end(ap);

   {
      STATE_MACHINE_LINE lines[2] = {
         TRANSMIT_COMMAND(desc),
         COMMAND_COMPLETE(desc)
      };
      STATE_MACHINE_PROGRAM program = { 2, lines };

      state_machine_program(machine, &program);
      while (state_machine_run(machine))
      {
         /* Loop until done */
      }
   }
}

/*******************************************************************************
* FUNCTION: kdc240_complex_command
*
* DESCRIPTION:
*
*    Creates and sends a complex command.
*
*******************************************************************************/
void
kdc240_complex_command
(
   CC_STRUCT_TYPE *cc_struct,
   KODAK_CAMERA_TYPE camera,
   unsigned char command,
   ...
)
{
   int desc;
   va_list ap;

   if (!camera_init)
   {
      /* Re-initialize the state machine */
      state_machine_reinitialize(machine);

      if (!camera_init)
      {
         printf ("kdc240_command: Is your camera turned on?\n");
         return;
      }
   }

   va_start(ap, command);
   desc = kodak_command_vcreate(camera, command, ap);
   va_end(ap);

   /*
    * Check to see if the command is not going to be transmitting any
    * packets but will expect X packets returned from the camera.
    */
   if (cc_struct->tx_func == NULL)
   {
      STATE_MACHINE_LINE lines[] = {
         TRANSMIT_COMMAND(desc),
#ifdef DC2XX_SPLIT_MESSAGES
         { (int)cc_struct, 0, 1, NULL,
            (void *)kdc240_read_packet_control,
            (void *)kdc240_read_packet_control_error },
         { (int)cc_struct, 0, cc_struct->rx_packet_size + 1, NULL,
            (void *)kdc240_read_packet, (void *)kdc240_read_packet_error },
#else
         { (int)cc_struct, 0, cc_struct->rx_packet_size + 2, NULL,
            (void *)kdc240_read_packet, (void *)kdc240_read_packet_error },
#endif
         { (int)cc_struct, 1, 0, (void *)kdc240_send_packet_response,
            (void *)kdc240_packet_response_done, NULL },
         COMMAND_COMPLETE(desc)
      };

#ifdef DC2XX_SPLIT_MESSAGES
      STATE_MACHINE_PROGRAM program = { 5, lines };
#else
      STATE_MACHINE_PROGRAM program = { 4, lines };
#endif

      cc_struct->loop_top = STATE_1;

      state_machine_program(machine, &program);
      while (state_machine_run(machine))
      {
         /* Loop until done */
      }
   }

   /*
    * Else, check to see if this is a command where we will transmit a
    * single packet and then receive X packets from the camera.
    */
   else if (cc_struct->rx_func != NULL)
   {
      STATE_MACHINE_LINE lines[] = {
         TRANSMIT_COMMAND(desc),
#ifdef DC2XX_SPLIT_MESSAGES
         { (int)cc_struct, 1, 0, (void *)kdc240_send_packet_control,
            (void *)kdc240_packet_control_done, NULL },
         { (int)cc_struct, cc_struct->tx_packet_size + 1, 1,
            (void *)kdc240_send_packet, (void *)kdc240_read_packet_resp,
            (void *)kdc240_read_packet_resp_error },
         { (int)cc_struct, 0, 1, NULL,
            (void *)kdc240_read_packet_control,
            (void *)kdc240_read_packet_control_error },
         { (int)cc_struct, 0, cc_struct->rx_packet_size + 1, NULL,
            (void *)kdc240_read_packet, (void *)kdc240_read_packet_error },
#else
         { (int)cc_struct, cc_struct->tx_packet_size + 2, 1,
            (void *)kdc240_send_packet, (void *)kdc240_read_packet_resp,
            (void *)kdc240_read_packet_resp_error },
         { (int)cc_struct, 0, cc_struct->rx_packet_size + 2, NULL,
            (void *)kdc240_read_packet, (void *)kdc240_read_packet_error },
#endif
         { (int)cc_struct, 1, 0, (void *)kdc240_send_packet_response,
            (void *)kdc240_packet_response_done, NULL },
         COMMAND_COMPLETE(desc)
      };

#ifdef DC2XX_SPLIT_MESSAGES
      STATE_MACHINE_PROGRAM program = { 7, lines };
      cc_struct->loop_top = STATE_3;
#else
      STATE_MACHINE_PROGRAM program = { 5, lines };
      cc_struct->loop_top = STATE_2;
#endif

      state_machine_program(machine, &program);
      while (state_machine_run(machine))
      {
         /* Loop until done */
      }
   }

   /*
    * Else, this is a command where a single packet is transmitted to the
    * camera but no packets are returned.
    */
   else
   {
      STATE_MACHINE_LINE lines[] = {
         TRANSMIT_COMMAND(desc),
#ifdef DC2XX_SPLIT_MESSAGES
         { (int)cc_struct, 1, 0, (void *)kdc240_send_packet_control,
            (void *)kdc240_packet_control_done, NULL },
         { (int)cc_struct, cc_struct->tx_packet_size + 1, 1,
            (void *)kdc240_send_packet, (void *)kdc240_read_packet_resp,
            (void *)kdc240_read_packet_resp_error },
#else
         { (int)cc_struct, cc_struct->tx_packet_size + 2, 1,
            (void *)kdc240_send_packet, (void *)kdc240_read_packet_resp,
            (void *)kdc240_read_packet_resp_error },
#endif
         COMMAND_COMPLETE(desc)
      };

#ifdef DC2XX_SPLIT_MESSAGES
      STATE_MACHINE_PROGRAM program = { 4, lines };
#else
      STATE_MACHINE_PROGRAM program = { 3, lines };
#endif

      state_machine_program(machine, &program);
      while (state_machine_run(machine))
      {
         /* Loop until done */
      }
   }
}

/*******************************************************************************
* FUNCTION: kdc240_baud_command
*
* DESCRIPTION:
*
*
*******************************************************************************/
void
kdc240_baud_command
(
   void
)
{
   int desc;

   desc = kodak_command_create(KODAK_CAMERA_DC240, CMD_BAUD_RATE,
      HIGH_BAUD_RATE_CODE);

   {
      STATE_MACHINE_LINE lines[] = {
         TRANSMIT_COMMAND(desc),
         { desc, 0, 0, NULL, kdc240_baud_switch, NULL } 
      };
      STATE_MACHINE_PROGRAM program = { 2, lines };

      state_machine_program(machine, &program);
      while (state_machine_run(machine))
      {
         /* Loop until done */
      }
   }

   /* Pause after changing baud rate */
   sleep(1);

   /* Set the new baud rate */
   state_machine_set_baud(machine, HIGH_BAUD_RATE);
}
