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

/*==============================================================================
* Include files
==============================================================================*/
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "kodak_generic.h"
#include "kodak_command.h"

/*==============================================================================
* Preprocessor defines
==============================================================================*/
#define COMMAND_PACKET_SIZE  8
#define NUM_DESCRIPTORS      1

/*==============================================================================
* Types
==============================================================================*/
typedef unsigned char DESCRIPTOR_TYPE[COMMAND_PACKET_SIZE];

typedef struct
{
   int command;
   unsigned char *description;
} COMMAND_DESCRIPTION_TYPE;

/*==============================================================================
* Local Variables
==============================================================================*/
static const COMMAND_DESCRIPTION_TYPE command_descs[] =
{
   { CMD_SET_HPBS, "Set HPBS" },
   { CMD_BAUD_RATE, "Set Baud Rate" },
   { CMD_SEND_LAST_PIC, "Send Last Taken Image Name" },
   { CMD_TAKE_PICTURE, "Take Picture" },
   { CMD_STATUS, "Status" },
   { CMD_READ_PIC_INFO, "Read Picture Information" },
   { CMD_READ_THUMBNAIL, "Read Thumbnail" },
   { CMD_OPEN_CARD, "Open Card" },
   { CMD_CLOSE_CARD, "Close Card" },
   { CMD_READ_DIRECTORY, "Read Directory" },
   { CMD_READ_FILE, "Read File" },
   { CMD_DELETE_FILE, "Delete File" },
};

static DESCRIPTOR_TYPE descriptors[NUM_DESCRIPTORS];

/*==============================================================================
* Local Function Prototypes
==============================================================================*/
static int get_descriptor(void);

/*==============================================================================
* Local Functions
==============================================================================*/

/*******************************************************************************
* FUNCTION: get_descriptor
*
* DESCRIPTION:
*
*    Gets the next available command descriptor.
*
*******************************************************************************/
static int
get_descriptor
(
   void
)
{
   return 0;
}

/*==============================================================================
* Global Functions
==============================================================================*/
/*******************************************************************************
* FUNCTION: kodak_command_create
*
* DESCRIPTION:
*
*    This function builds a command of the given (the command parameter) for
*    for the given camera type (the camera parameter).  The function returns
*    the descriptor for the new command.
*
*    If you need to do something special for a specific camera model,
*    do so based on the value of the camera parameter.  This way,
*    changes for one camera model do not break the other models.
*
*******************************************************************************/
int
kodak_command_create
(
   KODAK_CAMERA_TYPE camera,
   unsigned char command,
   ...
)
{
   int desc;
   va_list ap;

   /* Prepare to handle the variable length argument string */
   va_start(ap, command);

   /* Pass the va_list to the _real_ function */
   desc = kodak_command_vcreate(camera, command, ap);

   /* Clean up after va_start */
   va_end(ap);

   return desc;
}

/*******************************************************************************
* FUNCTION: kodak_command_vcreate
*
* DESCRIPTION:
*
*    This function builds a command of the given (the command parameter) for
*    for the given camera type (the camera parameter).  The function returns
*    the descriptor for the new command.
*
*    If you need to do something special for a specific camera model,
*    do so based on the value of the camera parameter.  This way,
*    changes for one camera model do not break the other models.
*
*******************************************************************************/
int
kodak_command_vcreate
(
   KODAK_CAMERA_TYPE camera,
   unsigned char command,
   va_list ap
)
{
   unsigned char *cmdbuf;
   int iarg;
   int descriptor;

   /* Get a new descriptor for this command */
   descriptor = get_descriptor();
   if (descriptor == -1)
   {
      return descriptor;
   }

   /* Get a pointer to the descriptor data */
   cmdbuf = descriptors[descriptor];

   /* Fill in the default fields of the command */
   memset(cmdbuf, 0, COMMAND_PACKET_SIZE);
   cmdbuf[0] = command;
   cmdbuf[7] = 0x1A;

   switch (command)
   {
      case CMD_BAUD_RATE:
         iarg = va_arg(ap, int);
         cmdbuf[2] = (iarg >> 8) & 0xFF;
         cmdbuf[3] = iarg & 0xFF;
         break;

      case CMD_READ_THUMBNAIL:
         iarg = va_arg(ap, int);
         cmdbuf[4] = iarg & 0xFF;
         break;

      case CMD_SET_HPBS:
         iarg = va_arg(ap, int);
         cmdbuf[2] = (iarg >> 8) & 0xFF;
         cmdbuf[3] = iarg & 0xFF;
         break;
         
      /* By default, the command takes no parameters */
      default:
         break;
   }

#ifdef COMMAND_DEBUG
   {
      int i;

      printf ("Command: ");

      /* Print a description of the command */
      for (i = 0; i < sizeof(command_descs); i++)
      {
         if (command_descs[i].command == cmdbuf[0])
         {
            printf ("(%s) ", command_descs[i].description);
            break;
         }
      }
      
      /* Print the command packet itself */
      for (i = 0; i < COMMAND_PACKET_SIZE; i++)
      {
         printf ("%02X ", cmdbuf[i]);
      }

      printf ("\n");
   }
#endif

   /* Return the descriptor for this command */
   return descriptor;
}

/*******************************************************************************
* FUNCTION: kodak_command_get
*
* DESCRIPTION:
*******************************************************************************/
unsigned char *
kodak_command_get
(
   int descriptor
)
{
   return descriptors[descriptor];
}
