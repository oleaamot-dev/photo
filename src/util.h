#ifndef _UTIL_H
#define _UTIL_H

#define BROWSER "netscape"

#include <gdk_imlib.h>
#include <gtk/gtk.h>

/* I don't know what this is for but it can't hurt to leave it in - GDR */
#ifndef _util_h
#define _util_h
#endif

void dialog_error(char *Error);
        /*
           Standard, run-of-the-mill message box
        */

void dialog_notice(char *titel, char *message, GtkJustification  jtype);

int confirm_dialog (char *message);

int confirm_overwrite (char *filename);

void ok_click (GtkWidget *dialog);

int wait_for_hide (GtkWidget *dialog, GtkWidget *ok_button,
                   GtkWidget *cancel_button) ;

GtkWidget *gtk_directory_selection_new(char *title);

GdkImlibImage *gdk_imlib_load_image_mem(char *image, int size);

void free_image(struct Image *im);

void save_scaled_image (char *filename, struct Image *im, int factor);

void save_image(char *filename, struct Image *im);

void free_imagemembers (struct ImageMembers *im);

void url_send_browser(char *url);

#endif /* _UTIL_H */
