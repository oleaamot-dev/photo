#ifndef KODAK_GENERIC_H
#define KODAK_GENERIC_H

/*******************************************************************************
* When you add a new camera model, add an entry in this enumeration.
*******************************************************************************/
typedef enum
{
   KODAK_CAMERA_DC20,
   KODAK_CAMERA_DC25,
   KODAK_CAMERA_DC200,
   KODAK_CAMERA_DC210,
   KODAK_CAMERA_DC240,
   KODAK_CAMERA_NUM_CAMERAS    /* This line must be last! */
} KODAK_CAMERA_TYPE;


/*******************************************************************************
* For each camera model supported, include the model's register function here.
*******************************************************************************/
void kdc240_register(void);


/*******************************************************************************
* Debugging symbols
*******************************************************************************/
#undef COMMAND_DEBUG
#undef DIRECTORY_DEBUG
#undef HPBS_DEBUG
#undef PICTURE_INFO_DEBUG
#undef READ_DEBUG
#undef SM_DEBUG

/*==============================================================================
* You should not need to modify anything after this point 
==============================================================================*/

#include "../src/gphoto.h"

#undef FALSE
#undef TRUE

typedef enum
{
   FALSE,
   TRUE
} BOOLEAN;

#define GPHOTO_SUCCESS  1
#define GPHOTO_FAIL     0

typedef int (*_detect)();

struct Kodak_Camera
{
   _detect             detect;
   _initialize         initialize;
   _get_picture        get_picture;
   _get_preview        get_preview;
   _delete_picture     delete_picture;
   _take_picture       take_picture;
   _number_of_pictures number_of_pictures;
   _configure          configure;
   _summary            summary;
   _description        description;
};

void kodak_register(struct Kodak_Camera *);

#endif /* KODAK_GENERIC_H */
