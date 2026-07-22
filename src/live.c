#include <config.h>
#include "gphoto.h"
#include "callbacks.h"
#include "util.h"
#include "live.h"
#include <stdlib.h>

/* 
	Live Camera! 
*/

extern struct Model *Camera;

int live_video_mode = 0;
int live_continue = 1;

void live_snapshot(GtkWidget *dialog) {

	int w, h;

	GdkPixmap *pixmap;
	GdkImlibImage *imlibimage;
	struct Image *im;

	GList *child = gtk_container_children(
                        GTK_CONTAINER(GTK_DIALOG(dialog)->vbox));
	GtkWidget *gpixmap = GTK_WIDGET(child->data);

	update_status("Getting live image...");
        if ((im = Camera->ops->get_preview()) == 0) {
		dialog_error("Could not get preview");
		return;
	}
	imlibimage = gdk_imlib_load_image_mem(im->image, im->image_size);
	free_image(im);
        w = imlibimage->rgb_width;
        h = imlibimage->rgb_height;
        gdk_imlib_render(imlibimage, w, h);
        pixmap = gdk_imlib_move_image(imlibimage);
        gtk_pixmap_set(GTK_PIXMAP(gpixmap), pixmap, NULL);
	update_status(N_("Done."));
}

void live_close_window (GtkWidget *button, GtkWidget *dialog) {

	if (live_video_mode) {
		live_continue = 0;
		return;
	}
	gtk_widget_destroy(dialog);
}

void live_video (GtkWidget *button, GtkWidget *dialog) {

	if (!GTK_TOGGLE_BUTTON(button)->active) {
		live_video_mode = 0;
		return;
	}
	live_video_mode = 1;
	while ((live_video_mode)&&(live_continue)) {
		while (gtk_events_pending())
			gtk_main_iteration();
		live_snapshot(dialog);
	}
	if (!live_continue)
		gtk_widget_destroy(dialog);
}

void live_main () {
	int w, h, error = 0;

	GtkWidget *dialog;
	GtkWidget *button, *ubutton, *cbutton, *tbutton, *hbox;

	GtkWidget *gpixmap;
	GdkPixmap *pixmap;
	GdkImlibImage *imlibimage;
	struct Image *im;

	update_status(N_("Getting live image..."));
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), N_("Live Camera!"));
	gtk_container_border_width (GTK_CONTAINER(dialog), 10);

	hbox = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox);
	gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),hbox);

	ubutton = gtk_button_new_with_label("Update Picture");
	gtk_widget_show(ubutton);
	gtk_box_pack_start_defaults(GTK_BOX(hbox),ubutton);
        gtk_signal_connect_object(GTK_OBJECT(ubutton), "clicked",
                           GTK_SIGNAL_FUNC(live_snapshot),
                           GTK_OBJECT(dialog));

        tbutton = gtk_button_new_with_label("Take Picture");
        gtk_widget_show(tbutton);
        gtk_box_pack_start_defaults(GTK_BOX(hbox),tbutton);
        gtk_signal_connect_object(GTK_OBJECT(tbutton), "clicked",
                           GTK_SIGNAL_FUNC(takepicture_call),
                           GTK_OBJECT(dialog));

	button = gtk_check_button_new_with_label(
	"Video Mode (Warning: Heavy processor usage)");
	gtk_widget_show(button);
        gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
        	GTK_SIGNAL_FUNC(live_video), dialog);


	cbutton = gtk_button_new_with_label("Close");
	gtk_widget_show(cbutton);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				  cbutton);
        gtk_signal_connect(GTK_OBJECT(cbutton), "clicked",
                           GTK_SIGNAL_FUNC(live_close_window),
                           dialog); 

        if ((im = Camera->ops->get_preview()) == 0) {
	        dialog_error(N_("Could not get preview"));
		error = 1;
	} else {
		imlibimage = gdk_imlib_load_image_mem(im->image, im->image_size);
		if (imlibimage) {
			free(im->image);
			free(im);
			w = imlibimage->rgb_width;
		        h = imlibimage->rgb_height;
		        gdk_imlib_render(imlibimage, w, h);
		        pixmap = gdk_imlib_move_image(imlibimage);
			gpixmap = gtk_pixmap_new(pixmap, NULL);
			gtk_widget_show(gpixmap);
			gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
	                         gpixmap);
		} else {
			error = 1;
		}
	}
	if (error) {
		dialog_notice(
N_("Live Preview Error"),
N_("Could not retrieve the live preview.\nThe camera either couldn't be accessed\n or it doesn't support live previews."),
GTK_JUSTIFY_FILL);
		gtk_widget_destroy(dialog);
	} else {
		gtk_widget_show(dialog);
		update_status(N_("Done."));
	}
}
