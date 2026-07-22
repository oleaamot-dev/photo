#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <time.h>
#include <string.h>
#include "../src/gphoto.h"
#include "../src/util.h"

/* prototypes for io library calls */
/* this should be in a header file for the io library */
#define RICOH_300  1
#define RICOH_300Z 2
#define RICOH_4300 3
extern int ricoh_300_open();
extern int ricoh_300_close();
extern int ricoh_300_getnpicts();
extern int ricoh_300_takepicture();
extern int ricoh_300_getsize();
extern int ricoh_300_getdate();
extern int ricoh_300_getpict();
extern int ricoh_300_deletepict();
extern int ricoh_300_setqual();
extern int ricoh_300_setflash();
extern int ricoh_300_setID();
extern int ricoh_300_getcamdate();
extern int ricoh_300_setcamdate();
extern int ricoh_300_getID();
extern int ricoh_300_getqual();
extern int ricoh_300_getdesqual();
extern int ricoh_300_getflash();
extern int ricoh_300_getwhite();
extern int ricoh_300_getdeswhite();
extern int ricoh_300_setwhite();
extern int ricoh_300_getexposure();
extern int ricoh_300_getdesexposure();
extern int ricoh_300_setexposure();
extern int ricoh_300_getzoom();
extern int ricoh_300_setzoom();

/* forward references */
static void exposure_mode();
static void exposure_value();
static void zoom_change();
static void white_mode();
static void flash_mode();
static void DrawText_im();
/* forward references for jpeg decompress  */
GdkImlibImage *gdk_imlib_load_image_mem();
static void jpeg_FatalErrorHandler();
static void decom_init();
static boolean decom_fill();
static void decom_skip_input_data();
static void decom_term();
char *gdk_imlib_save_image_mem();
static void comp_init();
static boolean comp_empty();
static void comp_term();

/* include the xpm image with the "no preview support message */
#include "ricoh_nopreview.xpm"

static int ricoh_camera_model;

struct dataim {	/* local struct to hold pseudo thumbnail image */
    int width;
    int height;
    unsigned char fg_red;
    unsigned char fg_green;
    unsigned char fg_blue;
    unsigned char *data;
};

/* Ricoh 300Z Camera Functions ----------------------------------
   ----------------------------------------------------------- */

int ricoh_300z_open_camera () {

	/* Open the camera for reading/writing */

	/* should the baud rate be settable ? cliff */
	if(ricoh_300_open(serial_port, 115200, &ricoh_camera_model) == -1)
		return (0);
	return (1);
}

void ricoh_300z_close_camera() {

	/* Close the camera */

	ricoh_300_close();
}

int ricoh_300z_number_of_pictures () {

	long num_pictures_taken = 0;

	if (ricoh_300z_open_camera() == 0) {
                dialog_error("Could not open camera.");
                return (0);
        }
	if(ricoh_300_getnpicts(&num_pictures_taken) == 1)
	    num_pictures_taken = 0;
	ricoh_300z_close_camera();

	return ((int)num_pictures_taken);
}

int ricoh_300z_take_picture () {

	long num_pictures_taken = 0;

	if (ricoh_300z_open_camera() == 0) {
                dialog_error("Could not open camera.");
                return (0);
        }

	ricoh_300_takepicture();

	if(ricoh_300_getnpicts(&num_pictures_taken) == 1)
	    num_pictures_taken = 0;
	ricoh_300z_close_camera();

	return ((int)num_pictures_taken);
}

struct Image *ricoh_300z_get_picture (int picNum, int thumbnail) {

	/*
	   Reads image #picNum from the Ricoh 300Z camera.
	   If thumbnail == TRUE, it creates a thumbnail.
	   If thumbnail == FALSE, it reads the whole image.
	*/

	char textbuf[12];
	unsigned char date[6];
	struct Image *im;

	GdkImlibImage *imlibimage;

	if (picNum != 0) {
		if (ricoh_300z_open_camera() == 0) {
			dialog_error("Could not open camera.");
			return 0;
		}
	}
	else picNum = 1;

        im = (struct Image*)malloc(sizeof(struct Image));
	ricoh_300_getsize(picNum, &im->image_size);


	if(thumbnail) {
	    struct dataim pseudo_thumb;
	    pseudo_thumb.width = 84;
	    pseudo_thumb.height = 63;
	    pseudo_thumb.fg_red = 0;
	    pseudo_thumb.fg_green = 200;
	    pseudo_thumb.fg_blue = 0;
	    pseudo_thumb.data =
	      malloc(3 * pseudo_thumb.width * pseudo_thumb.height);
	    memset(pseudo_thumb.data, 0,
	      3 * pseudo_thumb.width * pseudo_thumb.height);
	    ricoh_300_getdate(picNum, date);
	    sprintf(textbuf, "Image %-3d", picNum);
	    DrawText_im(&pseudo_thumb, 5, 5, textbuf);
	    /* note: assumes camera not used after 90 years */
	    if((date[0] & 0xf0) >> 4 < 9)
	    sprintf(textbuf, "%02x/%02x/20%02x", date[1], date[2], date[0]);
	    else
	    sprintf(textbuf, "%02x/%02x/19%02x", date[1], date[2], date[0]);
	    DrawText_im(&pseudo_thumb, 0, 25, textbuf);
	    sprintf(textbuf, "%02x:%02x:%02x", date[3], date[4], date[5]);
	    DrawText_im(&pseudo_thumb, 10, 35, textbuf);
	    if(ricoh_camera_model != RICOH_300) {
	    	sprintf(textbuf, "%dk Bytes", (int)(im->image_size/1024));
	    	DrawText_im(&pseudo_thumb, 0, 45, textbuf);
	    }
	    imlibimage =
	      gdk_imlib_create_image_from_data(pseudo_thumb.data, NULL, 84, 63);
	    free(pseudo_thumb.data);
	    im->image = gdk_imlib_save_image_mem(imlibimage, &im->image_size); 
	}
	else {
            im->image = (char *)malloc(im->image_size);
	    ricoh_300_getpict(picNum, im->image);
	    imlibimage = gdk_imlib_load_image_mem(im->image, im->image_size);
	}
	strcpy(im->image_type, "jpg");
        im->image_info_size = 0;

	if(!thumbnail) {
	    /* replace the pseudo thumbnail by a real one */
	    extern struct ImageMembers Thumbnails;
	    struct ImageMembers *node;
	    int i;

	    for(i = 0, node = &Thumbnails; i < picNum && node; i++)
		node = node->next;
	    if(node && node->imlibimage) {
	    	gdk_imlib_kill_image(node->imlibimage);
	    	node->imlibimage =
	    	  gdk_imlib_clone_scaled_image(imlibimage, 84, 63);
	    	gdk_imlib_paste_image(node->imlibimage,
		  GTK_PIXMAP(node->image)->pixmap, 0, 0, 84, 63);
	    	gtk_widget_show(node->image);
	    }
	}
	ricoh_300z_close_camera();
	gdk_imlib_destroy_image(imlibimage);
	return (im);
}
/*****************************************************************************
* From Gif-Lib
* 25 Sep 92 - Draw Text by Eric S. Raymond				     *
*****************************************************************************/


/*****************************************************************************
* Ascii 8 by 8 regular font - only first 128 characters are supported.	     *
*****************************************************************************/
/*
 * Each array entry holds the bits for 8 horizontal scan lines, topmost
 * first.  The most significant bit of each constant is the leftmost bit of
 * the scan line.
 */
#define GIF_FONT_WIDTH 8
#define GIF_FONT_HEIGHT 8
unsigned char AsciiTable[][GIF_FONT_WIDTH] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* Ascii 0  */
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x42, 0x3c, 0x00 }, /* Ascii 1  */
    { 0x3c, 0x7e, 0xdb, 0xff, 0xc3, 0x7e, 0x3c, 0x00 }, /* Ascii 2  */
    { 0x00, 0xee, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00 }, /* Ascii 3  */
    { 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00 }, /* Ascii 4  */
    { 0x00, 0x3c, 0x18, 0xff, 0xff, 0x08, 0x18, 0x00 }, /* Ascii 5  */
    { 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x10, 0x38, 0x00 }, /* Ascii 6  */
    { 0x00, 0x00, 0x18, 0x3c, 0x18, 0x00, 0x00, 0x00 }, /* Ascii 7  */
    { 0xff, 0xff, 0xe7, 0xc3, 0xe7, 0xff, 0xff, 0xff }, /* Ascii 8  */
    { 0x00, 0x3c, 0x42, 0x81, 0x81, 0x42, 0x3c, 0x00 }, /* Ascii 9  */
    { 0xff, 0xc3, 0xbd, 0x7e, 0x7e, 0xbd, 0xc3, 0xff }, /* Ascii 10 */
    { 0x1f, 0x07, 0x0d, 0x7c, 0xc6, 0xc6, 0x7c, 0x00 }, /* Ascii 11 */
    { 0x00, 0x7e, 0xc3, 0xc3, 0x7e, 0x18, 0x7e, 0x18 }, /* Ascii 12 */
    { 0x04, 0x06, 0x07, 0x04, 0x04, 0xfc, 0xf8, 0x00 }, /* Ascii 13 */
    { 0x0c, 0x0a, 0x0d, 0x0b, 0xf9, 0xf9, 0x1f, 0x1f }, /* Ascii 14 */
    { 0x00, 0x92, 0x7c, 0x44, 0xc6, 0x7c, 0x92, 0x00 }, /* Ascii 15 */
    { 0x00, 0x00, 0x60, 0x78, 0x7e, 0x78, 0x60, 0x00 }, /* Ascii 16 */
    { 0x00, 0x00, 0x06, 0x1e, 0x7e, 0x1e, 0x06, 0x00 }, /* Ascii 17 */
    { 0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x18 }, /* Ascii 18 */
    { 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00 }, /* Ascii 19 */
    { 0xff, 0xb6, 0x76, 0x36, 0x36, 0x36, 0x36, 0x00 }, /* Ascii 20 */
    { 0x7e, 0xc1, 0xdc, 0x22, 0x22, 0x1f, 0x83, 0x7e }, /* Ascii 21 */
    { 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x00, 0x00, 0x00 }, /* Ascii 22 */
    { 0x18, 0x7e, 0x18, 0x18, 0x7e, 0x18, 0x00, 0xff }, /* Ascii 23 */
    { 0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 }, /* Ascii 24 */
    { 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x00 }, /* Ascii 25 */
    { 0x00, 0x04, 0x06, 0xff, 0x06, 0x04, 0x00, 0x00 }, /* Ascii 26 */
    { 0x00, 0x20, 0x60, 0xff, 0x60, 0x20, 0x00, 0x00 }, /* Ascii 27 */
    { 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xff, 0x00 }, /* Ascii 28 */
    { 0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00 }, /* Ascii 29 */
    { 0x00, 0x00, 0x10, 0x38, 0x7c, 0xfe, 0x00, 0x00 }, /* Ascii 30 */
    { 0x00, 0x00, 0x00, 0xfe, 0x7c, 0x38, 0x10, 0x00 }, /* Ascii 31 */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*   */
    { 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, 0x00 }, /* ! */
    { 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* " */
    { 0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00 }, /* # */
    { 0x10, 0x7c, 0xd2, 0x7c, 0x86, 0x7c, 0x10, 0x00 }, /* $ */
    { 0xf0, 0x96, 0xfc, 0x18, 0x3e, 0x72, 0xde, 0x00 }, /* % */
    { 0x30, 0x48, 0x30, 0x78, 0xce, 0xcc, 0x78, 0x00 }, /* & */
    { 0x0c, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ' */
    { 0x10, 0x60, 0xc0, 0xc0, 0xc0, 0x60, 0x10, 0x00 }, /* ( */
    { 0x10, 0x0c, 0x06, 0x06, 0x06, 0x0c, 0x10, 0x00 }, /* ) */
    { 0x00, 0x54, 0x38, 0xfe, 0x38, 0x54, 0x00, 0x00 }, /* * */
    { 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00 }, /* + */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x70 }, /* , */
    { 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00 }, /* - */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00 }, /* . */
    { 0x02, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x00 }, /* / */
    { 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, /* 0 */
    { 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x3c, 0x00 }, /* 1 */
    { 0x7c, 0xc6, 0x06, 0x0c, 0x30, 0x60, 0xfe, 0x00 }, /* 2 */
    { 0x7c, 0xc6, 0x06, 0x3c, 0x06, 0xc6, 0x7c, 0x00 }, /* 3 */
    { 0x0e, 0x1e, 0x36, 0x66, 0xfe, 0x06, 0x06, 0x00 }, /* 4 */
    { 0xfe, 0xc0, 0xc0, 0xfc, 0x06, 0x06, 0xfc, 0x00 }, /* 5 */
    { 0x7c, 0xc6, 0xc0, 0xfc, 0xc6, 0xc6, 0x7c, 0x00 }, /* 6 */
    { 0xfe, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x00 }, /* 7 */
    { 0x7c, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0x7c, 0x00 }, /* 8 */
    { 0x7c, 0xc6, 0xc6, 0x7e, 0x06, 0xc6, 0x7c, 0x00 }, /* 9 */
    { 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00 }, /* : */
    { 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x20, 0x00 }, /* }, */
    { 0x00, 0x1c, 0x30, 0x60, 0x30, 0x1c, 0x00, 0x00 }, /* < */
    { 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00 }, /* = */
    { 0x00, 0x70, 0x18, 0x0c, 0x18, 0x70, 0x00, 0x00 }, /* > */
    { 0x7c, 0xc6, 0x0c, 0x18, 0x30, 0x00, 0x30, 0x00 }, /* ? */
    { 0x7c, 0x82, 0x9a, 0xaa, 0xaa, 0x9e, 0x7c, 0x00 }, /* @ */
    { 0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0x00 }, /* A */
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xfc, 0x00 }, /* B */
    { 0x7c, 0xc6, 0xc6, 0xc0, 0xc0, 0xc6, 0x7c, 0x00 }, /* C */
    { 0xf8, 0xcc, 0xc6, 0xc6, 0xc6, 0xcc, 0xf8, 0x00 }, /* D */
    { 0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xfe, 0x00 }, /* E */
    { 0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0x00 }, /* F */
    { 0x7c, 0xc6, 0xc0, 0xce, 0xc6, 0xc6, 0x7e, 0x00 }, /* G */
    { 0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00 }, /* H */
    { 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00 }, /* I */
    { 0x1e, 0x06, 0x06, 0x06, 0xc6, 0xc6, 0x7c, 0x00 }, /* J */
    { 0xc6, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0xc6, 0x00 }, /* K */
    { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0x00 }, /* L */
    { 0xc6, 0xee, 0xfe, 0xd6, 0xc6, 0xc6, 0xc6, 0x00 }, /* M */
    { 0xc6, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0xc6, 0x00 }, /* N */
    { 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, /* O */
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0, 0xc0, 0x00 }, /* P */
    { 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x06 }, /* Q */
    { 0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xc6, 0x00 }, /* R */
    { 0x78, 0xcc, 0x60, 0x30, 0x18, 0xcc, 0x78, 0x00 }, /* S */
    { 0xfc, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00 }, /* T */
    { 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, /* U */
    { 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00 }, /* V */
    { 0xc6, 0xc6, 0xc6, 0xd6, 0xfe, 0xee, 0xc6, 0x00 }, /* W */
    { 0xc6, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0xc6, 0x00 }, /* X */
    { 0xc3, 0xc3, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00 }, /* Y */
    { 0xfe, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0xfe, 0x00 }, /* Z */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* [ */
    { 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00 }, /* \ */
    { 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00 }, /* ] */
    { 0x00, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00 }, /* ^ */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff }, /* _ */
    { 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* ` */
    { 0x00, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00 }, /* a */
    { 0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xe6, 0xdc, 0x00 }, /* b */
    { 0x00, 0x00, 0x7c, 0xc6, 0xc0, 0xc0, 0x7e, 0x00 }, /* c */
    { 0x06, 0x06, 0x7e, 0xc6, 0xc6, 0xce, 0x76, 0x00 }, /* d */
    { 0x00, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0x7e, 0x00 }, /* e */
    { 0x1e, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x30, 0x00 }, /* f */
    { 0x00, 0x00, 0x7e, 0xc6, 0xce, 0x76, 0x06, 0x7c }, /* g */
    { 0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00 }, /*  */
    { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00 }, /* i */
    { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0xf0 }, /* j */
    { 0xc0, 0xc0, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0x00 }, /* k */
    { 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00 }, /* l */
    { 0x00, 0x00, 0xcc, 0xfe, 0xd6, 0xc6, 0xc6, 0x00 }, /* m */
    { 0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00 }, /* n */
    { 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00 }, /* o */
    { 0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xe6, 0xdc, 0xc0 }, /* p */
    { 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xce, 0x76, 0x06 }, /* q */
    { 0x00, 0x00, 0x6e, 0x70, 0x60, 0x60, 0x60, 0x00 }, /* r */
    { 0x00, 0x00, 0x7c, 0xc0, 0x7c, 0x06, 0xfc, 0x00 }, /* s */
    { 0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x1c, 0x00 }, /* t */
    { 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00 }, /* u */
    { 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00 }, /* v */
    { 0x00, 0x00, 0xc6, 0xc6, 0xd6, 0xfe, 0x6c, 0x00 }, /* w */
    { 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0x00 }, /* x */
    { 0x00, 0x00, 0xc6, 0xc6, 0xce, 0x76, 0x06, 0x7c }, /* y */
    { 0x00, 0x00, 0xfc, 0x18, 0x30, 0x60, 0xfc, 0x00 }, /* z */
    { 0x0e, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0e, 0x00 }, /* { */
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, /* | */
    { 0xe0, 0x30, 0x30, 0x1c, 0x30, 0x30, 0xe0, 0x00 }, /* } */
    { 0x00, 0x00, 0x70, 0x9a, 0x0e, 0x00, 0x00, 0x00 }, /* ~ */
    { 0x00, 0x00, 0x18, 0x3c, 0x66, 0xff, 0x00, 0x00 } /* Ascii 127 */
};

void DrawText_im(struct dataim *Image,
		     const int x, const int y,
		     const char *legend)
{
    int i, j;
    int base;
    const char *cp;

    for (i = 0; i < GIF_FONT_HEIGHT; i++)
    {
	base = 3 * (Image->width * (y + i) + x);

	for (cp = legend; *cp; cp++)
	    for (j = 0; j < GIF_FONT_WIDTH; j++)
	    {
		if (AsciiTable[(int)*cp][i] & (1 << (GIF_FONT_WIDTH - j))) {
		    Image->data[base] = Image->fg_red;
		    Image->data[base + 1] = Image->fg_green;
		    Image->data[base + 2] = Image->fg_blue;
		}
		base += 3;
	    }
    }
}

struct Image *ricoh_300z_get_preview () {
/* I don't beleive the Ricoh has preview capability, so this returns
 * an image with the no support message the first time it it called,
 * unless this is a command line mode call, in which case it takes
 * a picture.
 */

	static int not_first_time = 0;
	int picnum;

	FILE *jpgfile;
	long jpgfile_size;
	struct Image *im;
	char filename[1024];	

	extern command_line_mode;

	GdkImlibImage *imlibimage;

	if(not_first_time || command_line_mode) {
	    picnum = ricoh_300z_take_picture();
	    return ricoh_300z_get_picture (picnum, 0);
	} else {
	    not_first_time = 1;
	    imlibimage = gdk_imlib_create_image_from_xpm_data ((char **) ricoh_nopreview_xpm);
          sprintf(filename, "%s/gphoto-preview.jpg", gphotoDir);
          gdk_imlib_save_image (imlibimage, filename, NULL);
          jpgfile = fopen(filename, "r");
          fseek(jpgfile, 0, SEEK_END);
          jpgfile_size = ftell(jpgfile);      
          rewind(jpgfile);
          im = (struct Image*)malloc(sizeof(struct Image));
          im->image = (char *)malloc(sizeof(char)*jpgfile_size);
  fread(im->image, (size_t)sizeof(char), (size_t)jpgfile_size, jpgfile); 
          strcpy(im->image_type, "jpg");
          im->image_size = (int)jpgfile_size;
          im->image_info_size = 0;
          remove(filename);
		return (im);
	}
}

int ricoh_300z_configure () {

	/*
	   Shows the Ricoh 300Z config dialog
	*/

	char *info, *camID;
	time_t camtime;
	char *atime;

	long value;
	float fvalue;

	GtkWidget *dialog, *table, *label, *spacer, *toggle;
	GtkWidget *save_button, *cancel_button;
	GtkObject *adj;
	GtkObject *adjzoom;
	GSList *group;

	struct ConfigValues {
		GtkWidget *cam_id;
	        GtkWidget *qual_econ;
	        GtkWidget *qual_norm;
		GtkWidget *qual_fine;
	        GtkWidget *flash_auto;
		GtkWidget *flash_force;
		GtkWidget *flash_none;
		GtkWidget *white_auto;
		GtkWidget *white_outdoors;
		GtkWidget *white_fluorescent;
		GtkWidget *white_incandescent;
		GtkWidget *white_bw;
		GtkWidget *zoom;
		GtkWidget *exposure;
		GtkWidget *expose_auto;
		GtkWidget *expose_manual;
	        GtkWidget *date_yymmdd;
	 	GtkWidget *date_ddmmhh;
	        GtkWidget *clk_comp;
		GtkWidget *clk_none;
	} Config;

	info = malloc(2048);

	update_status("Getting Camera Configuration...");

	/* dialog window ---------------------- */
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW(dialog), "Configure Camera");
	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
				   10);
	table = gtk_table_new(15,5,FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	spacer = gtk_vseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,2,3,2,15);	

	if (ricoh_300z_open_camera() == 0) {
		dialog_error("Could not open camera.");
		return 0;
	}

	/* camera id ---------------------- */
	label = gtk_label_new("Camera ID:");
	gtk_widget_show(label);
	Config.cam_id = gtk_entry_new();
	gtk_widget_show(Config.cam_id);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.cam_id,1,5,0,1);

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,5,1,2);	

	ricoh_300_getID(info);
	gtk_entry_set_text(GTK_ENTRY(Config.cam_id), info);
	update_progress(12);

	/* image quality ---------------------- */
	label = gtk_label_new("Image Quality:");
	gtk_widget_show(label);
	Config.qual_econ = gtk_radio_button_new_with_label(NULL, "Economy");
	gtk_widget_show(Config.qual_econ);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.qual_econ));
	Config.qual_norm = gtk_radio_button_new_with_label(group, "Normal");
	gtk_widget_show(Config.qual_norm);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.qual_norm));
	Config.qual_fine = gtk_radio_button_new_with_label(group, "Fine");
	gtk_widget_show(Config.qual_fine);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_econ,1,2,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_norm,1,2,3,4);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.qual_fine,1,2,4,5);	

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,2,5,6);	

	ricoh_300_getdesqual(&value);
	update_progress(25);
	switch (value) {
		case 1:
			gtk_widget_activate(Config.qual_econ);
			break;
		case 4:
			gtk_widget_activate(Config.qual_fine);
			break;
		default:
			gtk_widget_activate(Config.qual_norm);
	}
			

	update_progress(50);

	/* white balance ---------------------- */
	label = gtk_label_new("White Balance:");
	gtk_widget_show(label);
	Config.white_auto = gtk_radio_button_new_with_label(NULL, "Automatic");
	gtk_widget_show(Config.white_auto);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.white_auto));
	Config.white_outdoors =
	  gtk_radio_button_new_with_label(group, "Outdoors");
	gtk_widget_show(Config.white_outdoors);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.white_outdoors));
	Config.white_fluorescent =
	  gtk_radio_button_new_with_label(group, "Fluorescent");
	gtk_widget_show(Config.white_fluorescent);
	group =
	  gtk_radio_button_group(GTK_RADIO_BUTTON(Config.white_fluorescent));
	Config.white_incandescent =
	  gtk_radio_button_new_with_label(group, "Incandescent");
	gtk_widget_show(Config.white_incandescent);
	if(ricoh_camera_model == RICOH_300 || ricoh_camera_model == RICOH_300Z){
	    group =
	    gtk_radio_button_group(GTK_RADIO_BUTTON(Config.white_incandescent));
	    Config.white_bw = gtk_radio_button_new_with_label(group, "B & W");
	    gtk_widget_show(Config.white_bw);
	}
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,6,7);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.white_auto,1,2,6,7);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.white_outdoors,
	  1,2,7,8);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.white_fluorescent,
	  1,2,8,9);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.white_incandescent,
	  1,2,9,10);	
	if(ricoh_camera_model == RICOH_300 || ricoh_camera_model == RICOH_300Z)
	  gtk_table_attach_defaults(GTK_TABLE(table),Config.white_bw,1,2,10,11);	

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,0,2,11,12);	

	ricoh_300_getdeswhite(&value);
	update_progress(65);
	switch (value) {
		case 4:
			gtk_widget_activate(Config.white_bw);
			break;
		case 3:
			gtk_widget_activate(Config.white_incandescent);
			break;
		case 2:
			gtk_widget_activate(Config.white_fluorescent);
			break;
		case 1:
			gtk_widget_activate(Config.white_outdoors);
			break;
		default:
			gtk_widget_activate(Config.white_auto);
	}
			
	/* Zoom lens position  ---------------------- */
	if(ricoh_camera_model == RICOH_300Z ||
	  ricoh_camera_model == RICOH_4300) {
	    label = gtk_label_new("Zoom lens position:");
	    gtk_widget_show(label);
	    ricoh_300_getzoom(&value);
	    adjzoom = gtk_adjustment_new((float)value, 0, 8, 1, 0, 0);
	    Config.zoom = gtk_hscale_new(GTK_ADJUSTMENT(adjzoom));
	    gtk_range_set_update_policy(GTK_RANGE(Config.zoom),
	      GTK_UPDATE_DISCONTINUOUS);
	    gtk_scale_set_draw_value(GTK_SCALE(Config.zoom), TRUE);
	    gtk_scale_set_digits(GTK_SCALE(Config.zoom), 0);
	    gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,12,13);	
	    gtk_table_attach_defaults(GTK_TABLE(table),Config.zoom,0,2,13,14);	
	    gtk_widget_show(Config.zoom);
	    gtk_signal_connect(GTK_OBJECT (adjzoom),
	      "value_changed", GTK_SIGNAL_FUNC(zoom_change), NULL);
	}

	/* flash mode ---------------------- */
	label = gtk_label_new("Flash Mode:");
	gtk_widget_show(label);
	Config.flash_auto = gtk_radio_button_new_with_label(NULL, "Auto");
	gtk_widget_show(Config.flash_auto);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.flash_auto));
	Config.flash_force = gtk_radio_button_new_with_label(group, "Force");
	gtk_widget_show(Config.flash_force);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.flash_force));
	Config.flash_none = gtk_radio_button_new_with_label(group, "None");
	gtk_widget_show(Config.flash_none);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_auto,4,5,2,3);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_force,4,5,3,4);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.flash_none,4,5,4,5);	
	
	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,3,5,5,6);	

	ricoh_300_getflash(&value);
	update_progress(75);
	switch (value) {
	    case 2:
		gtk_widget_activate(Config.flash_force);
		break;
	    case 1:
		gtk_widget_activate(Config.flash_none);
		break;
	    default:
		gtk_widget_activate(Config.flash_auto);
	}
	gtk_signal_connect(GTK_OBJECT (Config.flash_none),
	  "clicked",GTK_SIGNAL_FUNC(flash_mode), Config.white_auto);
	gtk_signal_connect(GTK_OBJECT (Config.white_auto),
	  "clicked",GTK_SIGNAL_FUNC(white_mode), Config.flash_none);

	
	update_progress(87);

	/* exposure compensation  ---------------------- */
	label = gtk_label_new("Exposure Compensation:");
	gtk_widget_show(label);
	Config.expose_auto = gtk_radio_button_new_with_label(NULL, "Auto");
	gtk_widget_show(Config.expose_auto);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.expose_auto));
	Config.expose_manual = gtk_radio_button_new_with_label(group, "Manual");
	gtk_widget_show(Config.expose_manual);
	ricoh_300_getdesexposure(&value);
	fvalue = (value > 9 ? 0 : value - 5) * .5;
	update_progress(95);
	adj = gtk_adjustment_new(fvalue, -2, 2, .5, 0, 0);
	Config.exposure = gtk_hscale_new(GTK_ADJUSTMENT(adj));
	gtk_range_set_update_policy(GTK_RANGE(Config.exposure),
	  GTK_UPDATE_DISCONTINUOUS);
	gtk_scale_set_draw_value(GTK_SCALE(Config.exposure), TRUE);
	gtk_scale_set_digits(GTK_SCALE(Config.exposure), 1);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,6,7);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.expose_auto,3,4,7,8);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.expose_manual,
	  4,5,7,8);	
	gtk_table_attach_defaults(GTK_TABLE(table),Config.exposure,3,5,8,9);	
	if(value > 9) {
	    gtk_widget_activate(Config.expose_auto);
	} else {
	    gtk_widget_activate(Config.expose_manual);
	    gtk_widget_show(Config.exposure);
	}
	gtk_signal_connect(GTK_OBJECT (Config.expose_manual),
	  "clicked",GTK_SIGNAL_FUNC(exposure_mode), Config.exposure);
	gtk_signal_connect(GTK_OBJECT (adj),
	  "value_changed", GTK_SIGNAL_FUNC(exposure_value), Config.exposure);

	spacer = gtk_hseparator_new();
	gtk_widget_show(spacer);
	gtk_table_attach_defaults(GTK_TABLE(table),spacer,3,5,9,10);	

	/* set clock ---------------------------- */
	label = gtk_label_new("Current Camera Time:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,4,10,11);

	ricoh_300_getcamdate(&camtime);
	update_progress(100);
	atime = ctime(&camtime);
	label = gtk_label_new(atime);
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,3,5,11,12);

	Config.clk_comp = gtk_radio_button_new_with_label(NULL,
							 "Set to Computer");
	gtk_widget_show(Config.clk_comp);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(Config.clk_comp));
	Config.clk_none = gtk_radio_button_new_with_label(group, "No Change");
	gtk_widget_show(Config.clk_none);

	gtk_table_attach_defaults(GTK_TABLE(table),Config.clk_comp,4,5,12,13);
	gtk_table_attach_defaults(GTK_TABLE(table),Config.clk_none,4,5,13,14);

	gtk_widget_activate(Config.clk_none);

	/* WOW that was a lot of code... now connect some stuff... */

	ricoh_300z_close_camera();
        toggle = gtk_toggle_button_new();
	gtk_widget_show(toggle);
	gtk_widget_hide(toggle);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
                           toggle, TRUE, TRUE, 0);

	save_button = gtk_button_new_with_label("Save");
	gtk_widget_show(save_button);
	GTK_WIDGET_SET_FLAGS (save_button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   save_button, FALSE, FALSE, 0);
	cancel_button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(cancel_button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   cancel_button, FALSE, FALSE, 0);
	gtk_widget_grab_default (save_button);

	gtk_widget_show(dialog);
	update_status("Done.");
	update_progress(0);

/* ----------------------------------------------------------- */
	if (wait_for_hide(dialog, save_button, cancel_button) == 0)
		return (1);
/* ----------------------------------------------------------- */

	update_status("Saving Configuration...");

	if (ricoh_300z_open_camera() == 0) {
		dialog_error("Could not open camera.");
		return 0;
	}
	update_progress(0);

	/* Set camera name... */
	camID = gtk_entry_get_text(GTK_ENTRY(Config.cam_id));
	if(strcmp(camID, info) != 0)
	    ricoh_300_setID(camID);
	update_progress(12);


	/* Set image quality... */
	if (GTK_WIDGET_STATE(Config.qual_econ) == GTK_STATE_ACTIVE)
		value = 1;
	  else if
	    (GTK_WIDGET_STATE(Config.qual_fine) == GTK_STATE_ACTIVE)
		value = 4;
	  else
		value = 2;
	ricoh_300_setqual(value);
	update_progress(25);

	/* white balance */
	if((ricoh_camera_model == RICOH_300 || ricoh_camera_model == RICOH_300Z)
	  && GTK_WIDGET_STATE(Config.white_bw) == GTK_STATE_ACTIVE)
	    value = 4;
	else if(GTK_WIDGET_STATE(Config.white_incandescent) == GTK_STATE_ACTIVE)
	    value = 3;
	else if(GTK_WIDGET_STATE(Config.white_fluorescent) == GTK_STATE_ACTIVE)
	    value = 2;
	else if(GTK_WIDGET_STATE(Config.white_outdoors) == GTK_STATE_ACTIVE)
	    value = 1;
	else
	    value = 0;
	ricoh_300_setwhite(value);
	update_progress(37);

	/* Set flash mode... */
	if (GTK_WIDGET_STATE(Config.flash_auto) == GTK_STATE_ACTIVE)
		value = 0;
	 else if (GTK_WIDGET_STATE(Config.flash_force) == GTK_STATE_ACTIVE)
 		value = 2;
	 else
 		value = 1;
	ricoh_300_setflash(value);
	update_progress(50);

	/* exposure compensation */
	if(GTK_WIDGET_STATE(Config.expose_auto) == GTK_STATE_ACTIVE)
	    value = 255;
	else
	    value = (int)(((GtkAdjustment *)adj)->value * 2.) + 5;
	ricoh_300_setexposure(value);
	update_progress(62);

	update_progress(75);

	update_progress(87);

	/* Set the clock... */
	if (GTK_WIDGET_STATE(Config.clk_comp) == GTK_STATE_ACTIVE) {
		camtime = time(&camtime);
		ricoh_300_setcamdate(camtime);
	}
	update_progress(100);
	ricoh_300z_close_camera();
	gtk_widget_destroy(dialog);
	update_status("Done.");
	update_progress(0);
	return 1;
}

/* signal handlers for the configure screen */

/* toggle viewing the exposure slider widget as appropriate */
static void
exposure_mode(widget, exposure)
GtkWidget *widget;
gpointer exposure;
{
    if (GTK_TOGGLE_BUTTON (widget)->active) {
	gtk_widget_show(exposure);
    } else {
	gtk_widget_hide(exposure);
    }
}

/* snap the exposure slider to valid values
 * should GTK really be doing this ? */
static void
exposure_value(adj, scale)
GtkAdjustment *adj;
GtkWidget *scale;
{
    adj->value = ((int)(adj->value * 2. +
      copysign(.5, (double)adj->value))) / 2.;
    gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
}

/* adjust the camera zoom, so one can see what it looks like */
static void
zoom_change(adj, dumb)
GtkAdjustment *adj;
gpointer dumb;
{
    ricoh_300z_open_camera();
    ricoh_300_setzoom((int)adj->value);
    ricoh_300z_close_camera();
}

/* white balance may effect the flash setting
 * the flash is turned off, if the white balance is not in auto */
static void
white_mode(white_auto, flash_none)
GtkWidget *white_auto;
gpointer flash_none;
{
    if (GTK_TOGGLE_BUTTON (white_auto)->active)
   	 return;
    else if (!GTK_TOGGLE_BUTTON ((GtkWidget *)flash_none)->active) {
	gtk_widget_activate((GtkWidget *)flash_none);
    }
}
static void
flash_mode(flash_none, white_auto)
GtkWidget *flash_none;
gpointer white_auto;
{
    if (GTK_TOGGLE_BUTTON ((GtkWidget *)white_auto)->active)
	return;
    else if (!GTK_TOGGLE_BUTTON (flash_none)->active)
	gtk_widget_activate(flash_none);
}

int ricoh_300z_delete_image (int picNum) {

	/*
	   deletes image #picNum from the Ricoh 300Z camera.
	*/

	if (ricoh_300z_open_camera() == 0) {
                dialog_error("Could not open camera.");
                return (0);
        }
	ricoh_300_deletepict(picNum);
	ricoh_300z_close_camera();

	return (1);
}

int ricoh_300z_initialize() {

	return 1;
}

char *ricoh_300z_summary() {

	return("Not Yet Supported.");
}

char *ricoh_300z_description() {

	return(
"Ricoh 300Z
Clifford Wright <cliff@snipe444.org>

*The Ricoh 300Z does not support thumbnail
previewing, so the thumbnails that appear
are place holders until the actual images
are downloaded.");
}

struct JPEG_error_mgr
{
   struct jpeg_error_mgr pub;
   sigjmp_buf          setjmp_buffer;
};

#if 0 /* fujisawa */
GdkImlibImage *
gdk_imlib_load_image_mem(char *image, int size)
{
   struct jpeg_decompress_struct cinfo;
   struct JPEG_error_mgr jerr;
   unsigned char      *data, *line[16], *ptr;
   int                 x, y, i;
    int w, h;
    GdkImlibImage *imlibimage;

   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit = jpeg_FatalErrorHandler;

   /* error handler to longjmp to, we want to preserve signals */
   if (sigsetjmp(jerr.setjmp_buffer, 1))
     {
	/* Whoops there was a jpeg error */
	jpeg_destroy_decompress(&cinfo);
	return NULL;
     }

   jpeg_create_decompress(&cinfo);
    cinfo.src = malloc(sizeof(struct jpeg_source_mgr));
    cinfo.src->next_input_byte = image;
    cinfo.src->bytes_in_buffer = size;
    cinfo.src->init_source = decom_init;
    cinfo.src->fill_input_buffer = decom_fill;/* just retrun fake EOI marker */
    cinfo.src->skip_input_data = decom_skip_input_data;
    cinfo.src->resync_to_restart = jpeg_resync_to_restart;
    cinfo.src->term_source = decom_term;
   jpeg_read_header(&cinfo, TRUE);
   cinfo.do_fancy_upsampling = FALSE;
   cinfo.do_block_smoothing = FALSE;
   jpeg_start_decompress(&cinfo);
   w = cinfo.output_width;
   h = cinfo.output_height;
   data = (unsigned char *)malloc(w * h * 3);
   if (!data)
     {
	jpeg_destroy_decompress(&cinfo);
	return NULL;
     }
   ptr = data;

   if (cinfo.rec_outbuf_height > 16)
     {
	fprintf(stderr, "JPEG uses line buffers > 16. Cannot load.\n");
	return NULL;
     }
   if (cinfo.output_components == 3)
     {
	for (y = 0; y < h; y += cinfo.rec_outbuf_height)
	  {
	    for (i = 0; i < cinfo.rec_outbuf_height; i++)
	       {
		  line[i] = ptr;
		  ptr += w * 3;
	       }
	    jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
	  }
     }
   else if (cinfo.output_components == 1)
     {
	for (i = 0; i < cinfo.rec_outbuf_height; i++)
	  {
	     if ((line[i] = (unsigned char *)malloc(w)) == NULL)
	       {
		  int                 t = 0;

		  for (t = 0; t < i; t++)
		     free(line[t]);
		  jpeg_destroy_decompress(&cinfo);
		  return NULL;
	       }
	  }
	for (y = 0; y < h; y += cinfo.rec_outbuf_height)
	  {
	     jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
	     for (i = 0; i < cinfo.rec_outbuf_height; i++)
	       {
		  for (x = 0; x < w; x++)
		    {
		       *ptr++ = line[i][x];
		       *ptr++ = line[i][x];
		       *ptr++ = line[i][x];
		    }
	       }
	  }
	for (i = 0; i < cinfo.rec_outbuf_height; i++)
	   free(line[i]);
     }
   free(cinfo.src);
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

    imlibimage = gdk_imlib_create_image_from_data(data, NULL, w, h);
    free(data);
    return imlibimage;
}
#endif

METHODDEF(void)
decom_init (j_decompress_ptr cinfo)
{
}
METHODDEF(boolean)
decom_fill(struct jpeg_decompress_struct *cinfo)
{
    /*unsigned char *ptr;
    ptr = (unsigned char*)cinfo->src->next_input_byte;
    ptr[0] = (JOCTET) 0xFF;
    ptr[1] = (JOCTET) JPEG_EOI;
    cinfo->src->bytes_in_buffer = 2;*/
    return TRUE;
}
METHODDEF(void)
decom_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{  
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}
METHODDEF(void)
decom_term (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}
char*
gdk_imlib_save_image_mem(GdkImlibImage *imlibimage, int *jpg_size)
{
    struct	jpeg_compress_struct cinfo;
    struct	JPEG_error_mgr jerr;
    JSAMPROW	row_pointer[1];
    int		row_stride;
    char	*jpg_image;		/* jpeg image */
    int		size;			/* size of jpeg image */

    /* preallocate max possible size for jpeg image, then reallocate later */
    size = imlibimage->rgb_width * imlibimage->rgb_height * 3 + 500;
    jpg_image = malloc(size);
    cinfo.err = jpeg_std_error(&(jerr.pub));
    jerr.pub.error_exit = jpeg_FatalErrorHandler;

    /* error handler to longjmp to, we want to preserve signals */
    if (sigsetjmp(jerr.setjmp_buffer, 1))
    {
	/* Whoops there was a jpeg error */
	jpeg_destroy_compress(&cinfo);
	return NULL;
    }

    jpeg_create_compress(&cinfo);
    cinfo.dest = malloc(sizeof(struct jpeg_destination_mgr));
    cinfo.dest->next_output_byte = jpg_image;
    cinfo.dest->free_in_buffer = size;
    cinfo.dest->init_destination = comp_init;
    cinfo.dest->empty_output_buffer = comp_empty;
    cinfo.dest->term_destination = comp_term;
    cinfo.image_width = imlibimage->rgb_width;
    cinfo.image_height = imlibimage->rgb_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, (100 * 208) >> 8, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = cinfo.image_width * 3;
    while (cinfo.next_scanline < cinfo.image_height)
    {
	row_pointer[0] =
	  imlibimage->rgb_data + (cinfo.next_scanline * row_stride);
	jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    *jpg_size =  size - cinfo.dest->free_in_buffer;
    realloc(jpg_image, *jpg_size);
    free(cinfo.dest);
    jpeg_destroy_compress(&cinfo);
    return jpg_image;
}
METHODDEF(void)
comp_init(j_compress_ptr cinfo)
{
}
METHODDEF(boolean)
comp_empty(j_compress_ptr cinfo)
{
    fprintf(stderr,"jpeg compresion buffer was to small\n");
    return TRUE;
}
METHODDEF(void)
comp_term(j_compress_ptr cinfo)
{
}

static void 
jpeg_FatalErrorHandler(j_common_ptr cinfo)
{
   /* 
    * FIXME:
    * We should somehow signal what error occurred to the caller so the
    * caller can handle the error message 
    */
   struct JPEG_error_mgr               *errmgr;

   errmgr = (struct JPEG_error_mgr *) cinfo->err;
   cinfo->err->output_message(cinfo);
   siglongjmp(errmgr->setjmp_buffer, 1);
   return;
}
/* Declare the camera function pointers */

struct _Camera ricoh_300z = {ricoh_300z_initialize,
			  ricoh_300z_get_picture,
			  ricoh_300z_get_preview,
			  ricoh_300z_delete_image,
			  ricoh_300z_take_picture,
			  ricoh_300z_number_of_pictures,
			  ricoh_300z_configure,
			  ricoh_300z_summary, 
			  ricoh_300z_description,
			  NULL};

/* End of Ricoh 300Z Camera functions ------------------------------
   -------------------------------------------------------------- */
