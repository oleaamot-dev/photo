
#ifndef _GPHOTO_H
#define _GPHOTO_H

/* This contains the things that Libraries will want access to.
 * Since they are not #included in main.c any more they need some
 * other way of finding this lot out. The overheads of main.h seemed
 * silly.
 */ 

#include <gpio.h>
#include <gnome.h>
#include <gdk_imlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "gphoto-debug.h"

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define N_(String) (String)
#endif

#define GPHOTO_DEBUG 1

/* Include the EXIF support functions */
// #include "exif.h"

struct Image {
	int     image_size;		/* # of bytes of image */
	char   *image;			/* image data */
	char    image_type[5];		/* extension of image (jpg, png, etc) */
	int     image_info_size;	/* # of entries in image_info */
	char  **image_info;		/* tag=value pairs */
};

typedef int 		(*_initialize)			();
typedef struct Image* 	(*_get_picture)			(int,int);
typedef struct Image*	(*_get_preview)			();
typedef int 		(*_delete_picture)		(int);
typedef int 		(*_take_picture)		();
typedef int 		(*_number_of_pictures)		();
typedef int 		(*_configure)			();
typedef char*		(*_summary)			();
typedef char*		(*_description)			();
typedef int		(*_bye)				();

struct _Camera {
	_initialize	initialize;
	_get_picture	get_picture;
	_get_preview 	get_preview;
	_delete_picture delete_picture;
	_take_picture	take_picture;
	_number_of_pictures number_of_pictures;
	_configure	configure;
	_summary	summary;
	_description	description;
	_bye		bye;
};

struct Model {
	char *name;
	struct _Camera *ops;
	int idVendor;
	int idProduct;
};

struct ImageMembers {
        GdkImlibImage	*imlibimage;
	GdkPixbuf       *pixbufimage;
        GtkWidget 	*image;
        GtkWidget 	*button;
        GtkWidget 	*page;
        GtkWidget 	*label;
	gint            active;
	GnomeCanvasItem *frame;
	GnomeCanvasItem *picture;
	GnomeCanvasItem *text;
	gint            x, y;
	guint           width, height;
        char      	*info;
        struct   	ImageMembers *next;
};


#define GPHOTO_CAMERA_NONE	0
#define GPHOTO_CAMERA_SERIAL	1
#define GPHOTO_CAMERA_USB	2

#ifdef DECLARE_GLOBAL_VARS_IN_GPHOTO_H
int	  command_line_mode;	/* TRUE if in command line mode */ 
char     *gphotoDir;		/* gPhoto directory		*/
char	  serial_port[20];	/* Serial port			*/
int	  camera_type;

gint debug;
gint debug_canvas;
gint debug_config;
gint debug_delete;
gint debug_dialog;
gint debug_format;
gint debug_gallery;
gint debug_get_index;
gint debug_get_image;
gint debug_get_thumb;
gint debug_index;
gint debug_init;
gint debug_live;
gint debug_load_cfg;
gint debug_load_img;
gint debug_next;
gint debug_prev;
gint debug_print;
gint debug_save_cfg;
gint debug_save_img;
gint debug_scale;
gint debug_set_model;
gint debug_summary;

#else
extern int       command_line_mode;    /* TRUE if in command line mode */
extern char      *gphotoDir;           /* gPhoto directory             */
extern char      serial_port[20];      /* Serial port                  */
extern int	camera_type;
#endif

void update_progress (float percentage);
void update_status   (char *newStatus);

#endif /* _GPHOTO_H */
