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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <netinet/in.h>

#include "kodak_generic.h"
#include "state_machine.h"
#include "kodak_command.h"
#include "kodak_dc240_helpers.h"

/*==============================================================================
* Preprocessor Defines
==============================================================================*/

/* NOTE: These are really the data size, not the Host Packet Buffer Size
 * (HPBS) used in the protocol -- packet framing bytes are omitted.
 */
#define HPBS_MIN     0x0200           /* minimum data size */
#define HPBS_MAX     0x8000           /* maximum data size */

/* Currently getting USB data overruns on larger packets; not yet clear why.
 * Workaround: keep packet size under 8K.
 */
#define HPBS_USB_MAX 0x1FF0

/*==============================================================================
* Type definitions
==============================================================================*/
typedef struct
{
   unsigned char filename[11];
   unsigned char attributes;
   unsigned short creation_time;
   unsigned short creation_date;
   unsigned char size[4];
} DIRECTORY_ENTRY_TYPE;			/* from camera */

typedef struct
{
   unsigned char filename [50];
   unsigned long thumbnail_size;
   unsigned long picture_size;
   unsigned char valid;
} PICTURE_TYPE;				/* constructed here */

typedef struct
{
   BOOLEAN valid;
   unsigned char data_type;
   unsigned char camera_type;
   unsigned short firmware_version;
   unsigned short rom32_version;
   unsigned short rom8_version;
   unsigned char battery_status;
   unsigned char adapter_flag;
   unsigned char strobe_status;
   unsigned char padding[256 - 11];
} STATUS_PACKET_TYPE;			/* from camera */

typedef struct
{
   BOOLEAN valid;
   unsigned char *filename;
} PICTURE_FILENAME_PACKET_TYPE;

typedef struct
{
   unsigned short num_entries;
   DIRECTORY_ENTRY_TYPE entries[1];
} DIRECTORY_PACKET_DATA_TYPE;		/* from camera */

typedef struct
{
   BOOLEAN valid;
   unsigned char *filename;
   int rx_bytes;
   DIRECTORY_PACKET_DATA_TYPE *rx_buffer;
} DIRECTORY_PACKET_TYPE;

typedef struct
{
   BOOLEAN valid;
   unsigned char *filename;
   int size;
   int rx_bytes;
   unsigned char *rx_buffer;
} PICTURE_PACKET_TYPE;

typedef struct
{
   BOOLEAN valid;
   unsigned char *filename;
   unsigned char data[256];
} PICTURE_INFO_PACKET_TYPE;

/*==============================================================================
* Local Function Prototypes
==============================================================================*/
static void kdc240_post_initialize(STATE_MACHINE_INSTANCE *);

static int kdc240_detect(void);
static int kdc240_initialize(void);

static struct Image *kdc240_get_picture(int, int);
static unsigned char *kdc240_get_picture_tx_filename (PICTURE_PACKET_TYPE *);
static CC_RESPONSE_TYPE kdc240_get_picture_read (PICTURE_PACKET_TYPE *,
   unsigned char *);
static void kdc240_get_picture_info(PICTURE_TYPE *);
static unsigned char *kdc240_get_picture_info_tx_filename (
   PICTURE_INFO_PACKET_TYPE *);
static CC_RESPONSE_TYPE kdc240_get_picture_info_read(
   PICTURE_INFO_PACKET_TYPE *, unsigned char *);

static struct Image *kdc240_get_preview(void);

static int kdc240_delete_picture(int);
static unsigned char *kdc240_delete_picture_tx_filename (
   DIRECTORY_PACKET_TYPE *);

static int kdc240_take_picture(void);
static CC_RESPONSE_TYPE kdc240_take_picture_read (
   PICTURE_FILENAME_PACKET_TYPE *, unsigned char *);

static int kdc240_number_of_pictures(void);
static void kdc240_get_picture_directory (unsigned char *);
static void kdc240_add_picture (unsigned char *, unsigned char *);
static unsigned char *kdc240_number_of_pictures_tx_filename (
   DIRECTORY_PACKET_TYPE *);
static CC_RESPONSE_TYPE kdc240_number_of_pictures_read (
   DIRECTORY_PACKET_TYPE *, unsigned char *);

static int kdc240_configure(void);

static char *kdc240_summary(void);
static CC_RESPONSE_TYPE kdc240_summary_read(STATUS_PACKET_TYPE *,
   unsigned char *);

static char *kdc240_description(void);

static void kdc240_open_card(void);
static void kdc240_close_card(void);
static int kdc240_get_reasonable_hpbs(void);
static void kdc240_set_hpbs(int);

static unsigned char *kdc240_create_filename(unsigned char *,
   unsigned long, unsigned long);

/*==============================================================================
* Global Variables
==============================================================================*/
STATE_MACHINE_INSTANCE *machine;
static PICTURE_TYPE *picture_index = NULL;
static int number_of_pictures = 0;
static int hpbs = HPBS_MIN;
static int num_errors = 0;

/*==============================================================================
* Constants
==============================================================================*/
STATE_MACHINE_TEMPLATE template =
{
   serial_port,
   9600,
   kdc240_post_initialize
};

struct Kodak_Camera kodak_dc240 =
{
   kdc240_detect,
   kdc240_initialize,
   kdc240_get_picture,
   kdc240_get_preview,
   kdc240_delete_picture,
   kdc240_take_picture,
   kdc240_number_of_pictures,
   kdc240_configure,
   kdc240_summary,
   kdc240_description,
   NULL
};

/*==============================================================================
* Local Functions
==============================================================================*/
/*******************************************************************************
* FUNCTION: kdc240_post_intialize
*
* DESCRIPTION:
*
*    This function is called after the state machine has powered up.  It is
*    responsible for ensuring that the camera is ready to talk with the
*    driver.
*
*******************************************************************************/
static void
kdc240_post_initialize
(
   STATE_MACHINE_INSTANCE *mach
)
{
   /* Store the machine instance */
   machine = mach;

   /* Fire up the camera */
   kdc240_restart();

   /* maximize USB packet sizes */
   if (mach->is_usb)
   {
      kdc240_set_hpbs(HPBS_USB_MAX);
   }
}

/*******************************************************************************
* FUNCTION: kdc240_detect
*
* DESCRIPTION:
*
*    Auto-detects the presence of the DC240.  Returns 1 if successfully
*    detected the camera, 0 otherwise.
*
*******************************************************************************/
static int
kdc240_detect
(
   void
)
{
   /* No detection yet */
   return 1;
}

/*******************************************************************************
* FUNCTION: kdc240_initialize
*
* DESCRIPTION:
*******************************************************************************/
static int
kdc240_initialize
(
   void
)
{
   machine = state_machine_construct(&template);
   if (machine == NULL)
   {
      printf ("kdc240_initialize: could not create state machine!\n");
      return 0;
   }

   return 1;
}

/*******************************************************************************
* FUNCTION: kdc240_get_picture
*
* DESCRIPTION:
*******************************************************************************/
static struct Image *
kdc240_get_picture
(
   int picture,
   int thumbnail
)
{
   unsigned char *status;
   struct Image *image = NULL;
   PICTURE_PACKET_TYPE buffer;
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      58,  (void *)kdc240_get_picture_tx_filename,
      hpbs, (void *)kdc240_get_picture_read
   };

   /* If we haven't retrieved the index yet, do that first. */
   if (picture_index == NULL)
   {
      kdc240_number_of_pictures();
   }

   /* Make sure that the picture number is present in the camera. */
   if (picture > number_of_pictures)
   {
      return NULL;
   }

   /* gphoto uses a one-based index, turn it into a zero-based one. */
   picture--;

   kdc240_open_card();

   kdc240_set_hpbs(hpbs);

   /* Get picture information if we haven't already */
   if (!picture_index[picture].valid)
   {
      kdc240_get_picture_info(&picture_index[picture]);
      if (!picture_index[picture].valid)
      {
         kdc240_close_card();
         return NULL;
      }
   }

   buffer.valid = FALSE;
   buffer.rx_bytes = 0;
   buffer.rx_buffer = NULL;
   buffer.filename = kdc240_create_filename(picture_index[picture].filename,
      0, 0);

   status = (unsigned char *)malloc(256);
   strcpy (status, "Fetching file ");
   strcat (status, picture_index[picture].filename);
   update_status(status);
   free(status);

   if (thumbnail)
   {
      int saved_hpbs = hpbs;
    
      buffer.size = picture_index[picture].thumbnail_size;

      /* Optimize HPBS for thumbnail retrieval */
      if (hpbs > buffer.size && buffer.size > HPBS_MIN)
      {
         kdc240_set_hpbs (buffer.size);
         cc_struct.rx_packet_size = buffer.size;
      }

      /*
       * NOTE: DC-{50,120} have a different thumbnail command.
       * NOTE: DC-{200,210} don't support format 2 (JPG) here.
       */
      kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240,
         CMD_READ_THUMBNAIL, 2);

      kdc240_set_hpbs (saved_hpbs);
   }
   else
   {
      buffer.size = picture_index[picture].picture_size;
      kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240,
         CMD_READ_FILE);
   }

   kdc240_close_card();

   if (buffer.valid)
   {
      image = (struct Image *)malloc(sizeof(struct Image));
      image->image_size = buffer.size;
      image->image = buffer.rx_buffer;
      strcpy(image->image_type, "jpg");
      image->image_info_size = 0;

      if (num_errors > 0)
      {
         num_errors--;
      }
   }
   else
   {
      if (buffer.rx_buffer)
      {
         free(buffer.rx_buffer);
      }

      num_errors++;
   }

   kdc240_set_hpbs(kdc240_get_reasonable_hpbs());

   return image;
}

static unsigned char *
kdc240_get_picture_tx_filename
(
   PICTURE_PACKET_TYPE *buffer
)
{
   return buffer->filename;
}

static CC_RESPONSE_TYPE
kdc240_get_picture_read
(
   PICTURE_PACKET_TYPE *buffer,
   unsigned char *packet
)
{
   int bytes_to_copy;
   CC_RESPONSE_TYPE retval;

   /* Handle receive errors */
   if (packet == NULL)
   {
      return CC_RESPONSE_LAST_PACKET;
   }

   if (buffer->rx_bytes == 0)
   {
      buffer->rx_buffer = (unsigned char *)malloc(buffer->size);
   }

   bytes_to_copy = buffer->size - buffer->rx_bytes;

   if (bytes_to_copy > hpbs)
   {
      bytes_to_copy = hpbs;
      retval = CC_RESPONSE_MORE_PACKETS;
   }
   else
   {
      buffer->valid = TRUE;
      retval = CC_RESPONSE_LAST_PACKET;
   }

   memcpy(buffer->rx_buffer + buffer->rx_bytes, packet, bytes_to_copy);
   buffer->rx_bytes += bytes_to_copy;

#ifdef READ_DEBUG
   printf ("kdc240_get_picture_read: read %d bytes\n", buffer->rx_bytes);
#endif

   if (buffer->size)
	   update_progress(100 * buffer->rx_bytes / buffer->size);

   return retval;
}

static void
kdc240_get_picture_info
(
   PICTURE_TYPE *picture
)         
{     
   int size;
   PICTURE_INFO_PACKET_TYPE buffer;
   CC_STRUCT_TYPE cc_struct =
   {
      (int)&buffer,
      58,  (void *)kdc240_get_picture_info_tx_filename,
      256, (void *)kdc240_get_picture_info_read,
   };

#ifdef PICTURE_INFO_DEBUG
   printf ("kdc240_get_picture_info: filename %s\n", picture->filename);
#endif

   buffer.valid = FALSE;
   buffer.filename = kdc240_create_filename(picture->filename, 0, 0);
   
   /* NOTE:  DC-{50,120} don't support this command. */
   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_READ_PIC_INFO);

   if (buffer.valid)
   {
      size = (buffer.data[92] << 24) |
             (buffer.data[93] << 16) |
             (buffer.data[94] <<  8) |
             (buffer.data[95] <<  0);
      picture->thumbnail_size = size;

      size = (buffer.data[104] << 24) |
             (buffer.data[105] << 16) |
             (buffer.data[106] <<  8) |
             (buffer.data[107] <<  0);
      picture->picture_size = size;

      picture->valid = TRUE;

#ifdef PICTURE_INFO_DEBUG
      printf ("        thumbnail size: %ld\n", picture->thumbnail_size);
      printf ("        picture size:   %ld\n", picture->picture_size);
#endif
   }
}

static unsigned char *
kdc240_get_picture_info_tx_filename
(  
   PICTURE_INFO_PACKET_TYPE *buffer
)  
{  
   return buffer->filename;
}     
         
static CC_RESPONSE_TYPE
kdc240_get_picture_info_read
(  
   PICTURE_INFO_PACKET_TYPE *buffer,
   unsigned char *packet
)     
{
   if (packet != NULL)
   {
      buffer->valid = TRUE;
      memcpy(buffer->data, packet, 256);
   }
      
   return CC_RESPONSE_LAST_PACKET;
}

/*******************************************************************************
* FUNCTION: kdc240_get_preview
*
* DESCRIPTION:
*******************************************************************************/
static struct Image *
kdc240_get_preview
(
   void
)
{
   struct Image *image;
   int picture;

   picture = kdc240_take_picture();
   image = kdc240_get_picture(picture, 0);
   kdc240_delete_picture(picture);

   return image;
}

/*******************************************************************************
* FUNCTION: kdc240_delete_picture
*
* DESCRIPTION:
*******************************************************************************/
static int
kdc240_delete_picture
(
   int picture
)
{
   DIRECTORY_PACKET_TYPE buffer;
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      58,  (void *)kdc240_delete_picture_tx_filename,
      0,   NULL
   };

   /* If we haven't retrieved the index yet, do that first. */
   if (picture_index == NULL)
   {
      kdc240_number_of_pictures();
   }

   /* Make sure that the picture number is present in the camera. */
   if (picture > number_of_pictures)
   {
      return GPHOTO_FAIL;
   }

   /* gphoto uses a one-based index, turn it into a zero-based one. */
   picture--;

   kdc240_open_card();

   /* Get picture information if we haven't already */
   if (!picture_index[picture].valid)
   {
      kdc240_get_picture_info(&picture_index[picture]);
      if (!picture_index[picture].valid)
      {
         kdc240_close_card();
         return GPHOTO_FAIL;
      }
   }
   
   buffer.valid = FALSE;
   buffer.rx_bytes = 0;
   buffer.rx_buffer = NULL;
   buffer.filename = kdc240_create_filename(picture_index[picture].filename,
      0, 0);

   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_DELETE_FILE);

   free(buffer.filename);

   kdc240_close_card();

   /* Re-sync the picture number mappings */
   kdc240_number_of_pictures();

   return GPHOTO_SUCCESS;
}

static unsigned char *
kdc240_delete_picture_tx_filename
(
   DIRECTORY_PACKET_TYPE *buffer
)
{
   return buffer->filename;
}

/*******************************************************************************
* FUNCTION: kdc240_take_picture
*
* DESCRIPTION:
*******************************************************************************/
static int
kdc240_take_picture
(
   void
)
{
   int i;

   PICTURE_FILENAME_PACKET_TYPE buffer = { FALSE, NULL };
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      0, NULL,
      256, (void *)kdc240_take_picture_read,
   };

   /* Take the picture */
   kdc240_command(KODAK_CAMERA_DC240, CMD_TAKE_PICTURE);

   /* Read the filename */
   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_SEND_LAST_PIC);

   if (buffer.valid == FALSE)
   {
      printf ("kdc240_take_picture: unable to determine filename of last picture!\n");
      return 0;
   }
   
   printf ("kdc240_take_picture: filename %s\n", buffer.filename);

   /* Re-sync the picture number mappings */
   kdc240_number_of_pictures();

   for (i = 0; i < number_of_pictures; i++)
   {
      printf ("kdc240_take_picture: directory entry %d = %s\n", i, picture_index[i].filename);
      if (strcmp(picture_index[i].filename, buffer.filename) == 0)
      {
         free(buffer.filename);
         return i + 1;
      }
   }

   free(buffer.filename);
   return 0;
}

static CC_RESPONSE_TYPE
kdc240_take_picture_read
(
   PICTURE_FILENAME_PACKET_TYPE *buffer,
   unsigned char *packet
)
{
   if (packet != NULL)
   {
      buffer->filename = (unsigned char *)malloc(256);

      if (buffer->filename != NULL)
      {
         buffer->valid = TRUE;
         memcpy(buffer->filename, packet, 256);
      }
   }

   return CC_RESPONSE_LAST_PACKET;
}

static int compare_picture_names (
   const void *left,
   const void *right
)
{
   const PICTURE_TYPE *l = (const PICTURE_TYPE *) left;
   const PICTURE_TYPE *r = (const PICTURE_TYPE *) right;
   return strncmp (l->filename, r->filename, sizeof *l);
}

/*******************************************************************************
* FUNCTION: kdc240_number_of_pictures
*
* DESCRIPTION:
*******************************************************************************/
static int
kdc240_number_of_pictures
(
   void
)
{
   int i;

   DIRECTORY_PACKET_TYPE buffer;
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      58,  (void *)kdc240_number_of_pictures_tx_filename,
      256, (void *)kdc240_number_of_pictures_read
   };

   kdc240_open_card();

   buffer.valid = FALSE;
   buffer.filename = kdc240_create_filename("\\DCIM\\*.*", 0, 0);
   buffer.rx_bytes = 0;
   buffer.rx_buffer = NULL;

   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_READ_DIRECTORY);

   free(buffer.filename);

   /* Free the old picture index (if any) */
   if (picture_index)
   {
      free(picture_index);
   }

   number_of_pictures = 0;

   if (buffer.valid)
   {
      for (i = 0; i < buffer.rx_buffer->num_entries; i++)
      {
         if (buffer.rx_buffer->entries[i].filename[0] != '.')
         {
            unsigned char *filename = (unsigned char *)malloc(20);

            memset(filename, 0, 20);
            strcpy(filename, "\\DCIM\\");
            memcpy(filename + 6, buffer.rx_buffer->entries[i].filename, 11);
            *strchr(filename, ' ') = '\0';
            strcat(filename, "\\");

            kdc240_get_picture_directory(filename);

            free(filename);
         }
      }

      /* Sort the pictures in the list */
      qsort (picture_index, number_of_pictures, sizeof (picture_index[0]),
         compare_picture_names);
   }

   kdc240_close_card();

   if (buffer.rx_buffer)
   {
      free(buffer.rx_buffer);
   }

   return number_of_pictures;
}

static void
kdc240_get_picture_directory
(
   unsigned char *directory
)
{
   unsigned char *filename;
   int i;

   DIRECTORY_PACKET_TYPE buffer;
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      58,  (void *)kdc240_number_of_pictures_tx_filename,
      256, (void *)kdc240_number_of_pictures_read
   };

   filename = (unsigned char *)malloc(strlen(directory) + 10);
   strcpy(filename, directory);
   strcat(filename, "*.JPG");

#ifdef DIRECTORY_DEBUG
   printf ("  searching: %s\n", filename);
#endif

   buffer.valid = FALSE;
   buffer.filename = kdc240_create_filename(filename, 0, 0);
   buffer.rx_bytes = 0;
   buffer.rx_buffer = NULL;

   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_READ_DIRECTORY);

   free(filename);
   free(buffer.filename);

   if (buffer.valid)
   {
      for (i = 0; i < buffer.rx_buffer->num_entries; i++)
      {
         kdc240_add_picture(directory, buffer.rx_buffer->entries[i].filename);
      }
   }

   if (buffer.rx_buffer)
   {
      free(buffer.rx_buffer);
   }
}

static void
kdc240_add_picture
(
   unsigned char *directory,
   unsigned char *filename
)
{
   PICTURE_TYPE *t;

   /* If this is the first picture, initialize the pointer */
   if (number_of_pictures == 0)
   {
      picture_index = NULL;
   }

   /* Increment the total number of pictures */
   number_of_pictures++;

   /* Allocate space for the new picture */
   picture_index = (PICTURE_TYPE *)realloc(picture_index,
      sizeof(PICTURE_TYPE) * number_of_pictures);

   /* Create a temporary pointer to the new picture */
   t = &picture_index[number_of_pictures - 1];

   /* Indicate that we haven't gotten information on this picture yet */
   t->valid = FALSE;

   /* Fill in the filename */
   memset(t->filename, 0, sizeof(t->filename));
   strcpy (t->filename, directory);
   memcpy (t->filename + strlen(t->filename), filename, 8);
   t->filename[strlen(t->filename)] = '.';
   memcpy (t->filename + strlen(t->filename), filename + 8, 3);

#ifdef DIRECTORY_DEBUG
   printf ("adding %s\n", t->filename);
#endif
}

static unsigned char *
kdc240_number_of_pictures_tx_filename
(
   DIRECTORY_PACKET_TYPE *buffer
)
{
   return buffer->filename;
}

static CC_RESPONSE_TYPE
kdc240_number_of_pictures_read
(
   DIRECTORY_PACKET_TYPE *buffer,
   unsigned char *packet
)
{
   int num_entries;
   int bytes_to_copy;
   CC_RESPONSE_TYPE retval;

   /* Handle receive errors */
   if (packet == NULL)
   {
      return CC_RESPONSE_LAST_PACKET;
   }

   if (buffer->rx_bytes == 0)
   {
      num_entries = (packet[0] << 8) | packet[1];
      buffer->rx_buffer = (DIRECTORY_PACKET_DATA_TYPE *)
         malloc((sizeof(DIRECTORY_ENTRY_TYPE) * num_entries) + 2);
   }
   else
   {
      num_entries = buffer->rx_buffer->num_entries;
   }

   bytes_to_copy = (sizeof(DIRECTORY_ENTRY_TYPE) * num_entries) + 2;
   bytes_to_copy -= buffer->rx_bytes;

   if (bytes_to_copy > 256)
   {
      bytes_to_copy = 256;
      retval = CC_RESPONSE_MORE_PACKETS;
   }
   else
   {
      buffer->valid = TRUE;
      retval = CC_RESPONSE_LAST_PACKET;
   }

   /* Copy the packet into the buffer */
   memcpy((unsigned char *)buffer->rx_buffer + buffer->rx_bytes,
      packet, bytes_to_copy);

   /* Update the num_entries field the first time through */
   if (buffer->rx_bytes == 0)
   {
      buffer->rx_buffer->num_entries = ntohs(buffer->rx_buffer->num_entries);
   }

   buffer->rx_bytes += bytes_to_copy;

   return retval;
}

/*******************************************************************************
* FUNCTION: kdc240_configure
*
* DESCRIPTION:
*******************************************************************************/
static int
kdc240_configure
(
   void
)
{
   return 0;
}

/*******************************************************************************
* FUNCTION: kdc240_summary
*
* DESCRIPTION:
*******************************************************************************/
static char *
kdc240_summary
(
   void
)
{
   STATUS_PACKET_TYPE buffer = { FALSE };
   CC_STRUCT_TYPE cc_struct = 
   {
      (int)&buffer,
      0, NULL,
      256, (void *)kdc240_summary_read,
   };

   kdc240_complex_command(&cc_struct, KODAK_CAMERA_DC240, CMD_STATUS);

   if (buffer.valid)
   {
      unsigned char format_buf[100];
      unsigned char *summary_buf = (char *)calloc(4096, sizeof(unsigned char));

      strcat (summary_buf, "Camera is a Kodak Digital Science ");
      switch (buffer.camera_type)
      {
         case 1:
            strcat (summary_buf, "DC50\n");
            break;

         case 2:
            strcat (summary_buf, "DC120\n");
            break;

         case 3:
            strcat (summary_buf, "DC200\n");
            break;

         case 4:
            strcat (summary_buf, "DC210\n");
            break;

         case 5:
            strcat (summary_buf, "DC240\n");
            break;

         case 6:
            strcat (summary_buf, "DC280\n");
            break;

         default:
            strcat (summary_buf, "(unknown)\n");
            break;
      }

      strcat (summary_buf,"Firmware version: ");
      sprintf(format_buf, "0x%04X\n", buffer.firmware_version);
      strcat (summary_buf, format_buf);

      strcat (summary_buf,"Battery status: ");
      switch (buffer.battery_status)
      {
         case 0:
            strcat (summary_buf, "OK\n");
            break;

         case 1:
            strcat (summary_buf, "Weak\n");
            break;

         case 2:
            strcat (summary_buf, "Empty\n");
            break;

         default:
            strcat (summary_buf, "(error)\n");
            break;
      }

      strcat (summary_buf,"AC Adapter status: ");
      switch (buffer.adapter_flag)
      {
         case 0:
            strcat (summary_buf, "not connected\n");
            break;

         case 1:
            strcat (summary_buf, "connected\n");
            break;

         default:
            strcat (summary_buf, "(error)\n");
            break;
      }

      strcat (summary_buf,"Connection: ");
      if (machine->is_usb)
      {
         strcat (summary_buf,"USB\n");
      }
      else
      {
         strcat (summary_buf,"Serial\n");
      }

      return summary_buf;
   }
   else
   {
      return "Camera Error";
   }
}

static CC_RESPONSE_TYPE
kdc240_summary_read
(
   STATUS_PACKET_TYPE *buffer,
   unsigned char *packet
)
{
   if (packet != NULL)
   {
      buffer->valid = TRUE;
      memcpy(&(buffer->data_type), packet, 256);
   }

   return CC_RESPONSE_LAST_PACKET;
}

/*******************************************************************************
* FUNCTION: kdc240_description
*
* DESCRIPTION:
*******************************************************************************/
static char *
kdc240_description
(
   void
)
{
   return "Kodak DC240.  Copyright (c) 1999, Randy Scott <scottr@wwa.com>";
}

static void
kdc240_open_card
(
   void
)
{
   kdc240_command(KODAK_CAMERA_DC240, CMD_OPEN_CARD);
}

static void
kdc240_close_card
(
   void
)
{
   kdc240_command(KODAK_CAMERA_DC240, CMD_CLOSE_CARD);
}

static int
kdc240_get_reasonable_hpbs
(
   void
)
{
   int new_hpbs = hpbs;

   /* HPBS for USB is fixed */
   if (machine->is_usb)
   {
      new_hpbs = HPBS_USB_MAX;
   }

   /* For serial, we attempt to base the HPBS on the error rate */
   else
   {
      if (hpbs == HPBS_MIN && num_errors > 0)
      {
         /* Do nothing, we can't lower HPBS any more */
      }
      else if (num_errors > 0)
      {
         /* Lower HPBS */
         new_hpbs = (HPBS_MIN + hpbs) >> 1;
      }
      else
      {
         /* Raise HPBS */
         new_hpbs = (HPBS_MAX + hpbs) >> 1;
      }

      /* Error check the resulting HPBS before sending to camera */
      if (new_hpbs > HPBS_MAX)
      {
         new_hpbs = HPBS_MAX;
      }
      else if (new_hpbs < HPBS_MIN)
      {
         new_hpbs = HPBS_MIN;
      }
   }

   return new_hpbs;
}

static void
kdc240_set_hpbs
(
   int new_hpbs
)
{
   /* If the HPBS hasn't changed, don't bother writing to the camera. */
   if (new_hpbs == hpbs)
   {
      return;
   }

   /* NOTE: DC-{50,120,200,210} have fixed HPBS */
   kdc240_command(KODAK_CAMERA_DC240, CMD_SET_HPBS, new_hpbs + 2);

#ifdef HPBS_DEBUG
   printf ("kdc240_set_hpbs: hpbs set to %d\n", new_hpbs + 2);
#endif

   hpbs = new_hpbs;
}

static unsigned char *
kdc240_create_filename
(
   unsigned char *filename,
   unsigned long start,
   unsigned long number
)
{
   unsigned char *buf; 
 
   buf = (unsigned char *)malloc(58);
   memset(buf, 0, 58);

   strcpy (buf, filename);

   buf[48] = (start >> 24) & 0xff;
   buf[49] = (start >> 16) & 0xff;
   buf[50] = (start >>  8) & 0xff;
   buf[51] = (start >>  0) & 0xff;

   buf[52] = (number >> 24) & 0xff;
   buf[53] = (number >> 16) & 0xff;
   buf[54] = (number >>  8) & 0xff;
   buf[55] = (number >>  0) & 0xff;

   return buf;
}

/*==============================================================================
* Global Functions
==============================================================================*/
/*******************************************************************************
* FUNCTION: kdc240_register
*
* DESCRIPTION:
*
*    Registers the camera model's existance with the Kodak generic handler.
*    Expects to be called when the generic handler is initialized.
*
*******************************************************************************/
void
kdc240_register
(
   void
)
{
   kodak_register(&kodak_dc240);
}
