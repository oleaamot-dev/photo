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

#include "../src/gphoto.h"
#include "kodak_generic.h"

static struct Kodak_Camera *cams[KODAK_CAMERA_NUM_CAMERAS];
static int current;
static int registered = 0;

/*******************************************************************************
* FUNCTION: kodak_register
*
* DESCRIPTION:
*******************************************************************************/
void
kodak_register
(
   struct Kodak_Camera *camera
)
{
   cams[registered++] = camera;
}

/*******************************************************************************
* FUNCTION: kodak_initialize
*
* DESCRIPTION:
*******************************************************************************/
int
kodak_initialize
(
   void
)
{
   /*
    * Allow each of the Kodak cameras to register themselves with the generic
    * driver.
    *
    * When you add a new camera, add a call to your register function here.
    * The cameras are detected in the order that they registered, so pay
    * attention to the ordering if your camera has special detection
    * requirements.
    */
   kdc240_register();

   /* All of the cameras have registered, detect the connected camera. */
   for (current = 0; current < registered; current++)
   {
      /* Check to see if the "current" camera is connected */
      if (cams[current]->detect())
      {
         /* Camera was detected.  Exit the loop. */
         break;
      }
   }

   if (current < registered)
   {
      /* Allow the detected camera to initialize */
      return cams[current]->initialize();
   }
   else
   {
      return 0;
   }
}

/*******************************************************************************
* FUNCTION: kodak_get_picture
*
* DESCRIPTION:
*******************************************************************************/
struct Image *
kodak_get_picture
(
   int picture,
   int thumbnail
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->get_picture != NULL)
   {
      return cams[current]->get_picture(picture, thumbnail);
   }

   /* Camera does not support this operation. */
   return NULL;
}

/*******************************************************************************
* FUNCTION: kodak_get_preview
*
* DESCRIPTION:
*******************************************************************************/
struct Image *
kodak_get_preview
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->get_preview != NULL)
   {
      return cams[current]->get_preview();
   }

   /* Camera does not support this operation. */
   return NULL;
}

/*******************************************************************************
* FUNCTION: kodak_delete_picture
*
* DESCRIPTION:
*******************************************************************************/
int
kodak_delete_picture
(
   int picture
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->delete_picture != NULL)
   {
      return cams[current]->delete_picture(picture);
   }

   /* Camera does not support this operation. */
   return 0;
}

/*******************************************************************************
* FUNCTION: kodak_take_picture
*
* DESCRIPTION:
*******************************************************************************/
int
kodak_take_picture
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->take_picture != NULL)
   {
      return cams[current]->take_picture();
   }

   /* Camera does not support this operation. */
   return 0;
}

/*******************************************************************************
* FUNCTION: kodak_number_of_pictures
*
* DESCRIPTION:
*******************************************************************************/
int
kodak_number_of_pictures
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->number_of_pictures != NULL)
   {
      return cams[current]->number_of_pictures();
   }

   /* Camera does not support this operation. */
   return 0;
}

/*******************************************************************************
* FUNCTION: kodak_configure
*
* DESCRIPTION:
*******************************************************************************/
int
kodak_configure
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->configure != NULL)
   {
      return cams[current]->configure();
   }

   /* Camera does not support this operation. */
   return 0;
}

/*******************************************************************************
* FUNCTION: kodak_summary
*
* DESCRIPTION:
*******************************************************************************/
char *
kodak_summary
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current]->summary != NULL)
   {
      return cams[current]->summary();
   }

   /* Camera does not support this operation. */
   return NULL;
}

/*******************************************************************************
* FUNCTION: kodak_description
*
* DESCRIPTION:
*******************************************************************************/
char *
kodak_description
(
   void
)
{
   /* If the camera supports this operation, perform it. */
   if (cams[current] != NULL && cams[current]->description != NULL)
   {
      return cams[current]->description();
   }

   /* Camera does not support this operation. */
   return NULL;
}

/*******************************************************************************
* Function Table
*******************************************************************************/
struct _Camera kodak =
{
   kodak_initialize,
   kodak_get_picture,
   kodak_get_preview,
   kodak_delete_picture,
   kodak_take_picture,
   kodak_number_of_pictures,
   kodak_configure,
   kodak_summary,
   kodak_description,
   NULL
};
