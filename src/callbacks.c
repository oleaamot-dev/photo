/* gPhoto - free digital camera utility - http://www.gphoto.org/
 *
 * Copyright (C) 1999  Scott Fritzinger <scottf@scs.unr.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "config.h"
#include "gphoto.h"
#include "util.h"
#include "callbacks.h"
#include "toolbar.h"
#include "canvas.h"
#include <gpio.h>
#ifdef GPIO_USB
#include <usb.h>
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <gnome.h>

#ifdef linux
/* devfs support */
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "icons/post_processing_on.xpm"
#include "icons/post_processing_off.xpm"

static gint okAction;

extern struct ImageMembers Thumbnails;
extern struct ImageMembers Images;
extern struct Model *Camera;
extern struct Model cameras[];
extern GtkWidget *library_name;
extern GtkWidget *notebook;
extern GtkWidget *stop_button;

extern int	camera_type;
extern char	serial_port[];

extern gint	  post_process;
extern char	  post_process_script[];
extern GtkWidget *post_process_pixmap;
extern GtkWidget *index_vp;
extern GtkWidget *index_table;

extern char 	 *filesel_cwd;

extern float progress_min;
extern float progress_max;
extern int cam_busy;

/* Search the image_info tags for "name", return its value (string) */
char *find_tag(struct Image *im, char *name)
{
	int i;

	if (im==NULL) return(NULL);
	if (im->image_info==NULL) return(NULL);

	for (i=0;i<im->image_info_size;i+=2)
		if (!strncmp((im->image_info)[i],name,100))
			return((im->image_info)[i+1]);

	if (!im || !im->image_info)
		return NULL;
	
	for (i = 0; i < im->image_info_size; i+=2)
		if (!strncmp((im->image_info)[i], name, 100))
			return (im->image_info)[i+1];

	return NULL;	/* Nothing Found */
}

void index_page(void)
{
    gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 0);
}

void next_page(void)
{
    gtk_notebook_next_page(GTK_NOTEBOOK(notebook));
}

void prev_page(void)
{
    gtk_notebook_prev_page(GTK_NOTEBOOK(notebook));
}

void set_camera(char *model)
{
	int i;
	int ret;

	for (i = 0; cameras[i].name; i++) {
		if (strcmp(model, cameras[i].name))
			continue;

		if (!command_line_mode)
			gtk_label_set(GTK_LABEL(library_name),
				      cameras[i].name);
		else {
#ifdef GPHOTO_DEBUG
			g_message ("set_camera: %s", cameras[i].name);
#endif
		}
		    
		Camera = &cameras[i];

		if (cam_busy) {
	 		dialog_error(N_("Camera is already busy"));
			return;
		}
		set_toolbar_busy();
		ret=Camera->ops->initialize();
		set_toolbar_ready();
		if (ret) return;
	}
}

void configure_call(void)
{
	int ret;

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	ret=Camera->ops->configure();
	set_toolbar_ready();
	if (ret == 0)
	 	dialog_error(N_("No configuration options."));
}

void mail_image_call(void)
{
	dialog_error(N_("Mail image needs to be implemented!"));
}

void takepicture_call(void)
{
	char status[256];
	int picNum;

	update_status(N_("Taking picture..."));
	
	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	picNum = Camera->ops->take_picture();
	set_toolbar_ready();

	if (!picNum) {
		dialog_error(N_("Could not take a picture."));
		return;
	}
	    
	appendpic(picNum, 0, TRUE, NULL, 100);
	sprintf(status, N_("New picture is #%03i"), picNum);
	gtk_notebook_set_page(GTK_NOTEBOOK(notebook), picNum);
	update_status(status);
}

void format(GtkWidget *dialog, GtkObject *button)
{
	gint current, no_pics, i;
	char error[32];

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	no_pics = Camera->ops->number_of_pictures();
	i = no_pics;

	gtk_widget_hide(dialog);
	update_status("Deleting ALL pictures.");

	/* empty camera memory... */
	while (i) {
		if (Camera->ops->delete_picture(i) != 0) {	
			remove_thumbnail(i);
			sprintf(error, N_("Deleting %i...\n"), i);
			update_status(error);
			if (no_pics)
				update_progress((float)i/(float)no_pics);
		} else  {
			sprintf(error, N_("Could not delete #%i\n"), i);
			dialog_error(error);
		}
		i--;
	}
	update_status("Deleted all images.");
	update_progress(0);
	gtk_widget_destroy(dialog);
	set_toolbar_ready();
}

void del_pics(GtkWidget *dialog, GtkObject *button)
{
	int i = 1;
	char error[32];
	struct ImageMembers *node = &Thumbnails;

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	gtk_widget_hide(dialog);
	update_status(N_("Deleting selected pictures..."));

	/* delete from camera... */
	node = node->next;  /* Point at first thumbnail */
	while (node != NULL) {
		if (node->active) {
			node = node->next;
			if (Camera->ops->delete_picture(i) != 0) {
				remove_thumbnail(i);
				i--;
			} else {
				sprintf(error, N_("Could not delete #%i"), i);
				dialog_error(error);
			}
		} else
			node = node->next;

		i++;
	}
	gtk_widget_destroy(dialog);
	update_status("Done.");
	set_toolbar_ready();
}

void del_dialog(int type)
{
	/*
	   Shows the delete confirmation dialog
	   type == 1: all
	   type == 0; selected
	*/
	int nothing_selected = 1;
	GtkWidget *dialog, *button, *label;

	if (!type) {
		struct ImageMembers *node = &Thumbnails;

		while (node->next != NULL) {
			node = node->next;
			if (node->active)
				nothing_selected = 0;
		}
		if (nothing_selected) {
			update_status("Nothing selected to be deleted.");
			return;
		}
	}

	dialog = gtk_dialog_new();
	
	if (!type)
	    label = gtk_label_new("Are you sure you want to DELETE the selected images?");
	else if (type == 1)
	    label = gtk_label_new("Are you sure you want to DELETE all the images from memory?");

	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
			   FALSE, FALSE, 0);

	button = gtk_button_new_with_label(N_("Yes"));
	gtk_widget_show(button);
	
	if (!type) {
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", 
				      GTK_SIGNAL_FUNC(del_pics), 
				      GTK_OBJECT(dialog));
	} else if (type == 1) {
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", 
				      GTK_SIGNAL_FUNC(format), 
				      GTK_OBJECT(dialog));
	}

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, FALSE, FALSE, 0);
	button = gtk_button_new_with_label(N_("No"));
	gtk_widget_show(button);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked", 
				  GTK_SIGNAL_FUNC(gtk_widget_destroy), 
				  GTK_OBJECT(dialog));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, FALSE, FALSE, 0);

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_widget_show(dialog);
}

void savepictodisk(gint picNum, gint thumbnail, char *prefix)
{
	/* Saves picture (thumbnail == 0) or thumbnail (thumbnail == 1)
	 * #picNum to disk as prefix.(returned image extension)
	 */
	struct Image *im = NULL; 
	char fname[1024], error[32], process[1024];
	struct ImageMembers *node = &Images;

	if (!thumbnail) {
		/* should be strtol. RAA */
		while (node && node->info && atoi(node->info) != picNum)
			node = node->next;

		sprintf(fname, "%s.%s", prefix, "jpg");
		if (node != NULL  && node->imlibimage != NULL) {
			/* A match was found */
			gdk_imlib_save_image(node->imlibimage, fname, NULL);
			return;
		}
	}

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	im = Camera->ops->get_picture(picNum, thumbnail);
	set_toolbar_ready();
	if (im == 0 || im == NULL) {
		sprintf(error, N_("Could not save #%i"), picNum);
		dialog_error(error);
		return;
	}
	sprintf(fname, "%s.%s", prefix, im->image_type);
	if (confirm_overwrite(fname)) {
		save_image(fname, im);
		if (post_process) {
			sprintf(process, post_process_script, fname);
			system(process);
		}
	}
	free_image(im);
}

char saveselectedtodisk_dir[1024];

void saveselectedtodisk(GtkWidget *widget, char *type)
{
	/* Odd workaround:
	 * if type == "tn" (thumbnails/no directory needed)
	 * or type == "in" (images/no directory needed)
	 * then it is assume that saveselectedtodisk_dir contains
	 * the path to where we want to save. this allows the batch
	 * saving of both images and thumbnails without prompting
	 * for an output directory for thumbnails, and then images.
	 * if type == "t", selected thumbnails are saved.
	 * if type == "i", selected images are saved.
	 */
	int i = 0, pic = 1;
	char fname[1024];
	char status[32];
	char *filesel_dir, *filesel_prefix;

	struct ImageMembers *node = &Thumbnails;

	GtkWidget *filesel, *label;
	GtkWidget *entry, *hsep;

	if ((strcmp("tn", type) != 0) && (strcmp("in", type) != 0)) {
		/* Get an output directory */
		filesel = gtk_directory_selection_new(
				"Select a directory to store the images...");

		entry = gtk_entry_new();
		gtk_widget_show(entry);
		gtk_box_pack_end_defaults(GTK_BOX(GTK_FILE_SELECTION(
			filesel)->main_vbox), entry);

		label = gtk_label_new(N_("Filename prefix:"));
		gtk_widget_show(label);
		gtk_box_pack_end_defaults(GTK_BOX(GTK_FILE_SELECTION(
			filesel)->main_vbox), label);

		hsep = gtk_hseparator_new();
		gtk_widget_show(hsep);
		gtk_box_pack_end_defaults(GTK_BOX(GTK_FILE_SELECTION(
			filesel)->main_vbox), hsep);

		/* if they clicked cancel, return  ------------- */
		if (wait_for_hide(filesel, GTK_FILE_SELECTION(filesel)->ok_button, 
		    GTK_FILE_SELECTION(filesel)->cancel_button) == 0)
			return;
	        /* --------------------------------------------- */

		if (!GTK_IS_OBJECT(filesel))
			return;
		filesel_dir = gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(filesel));
		filesel_prefix = gtk_entry_get_text(GTK_ENTRY(entry));
		if (filesel_prefix==NULL) filesel_prefix="photo";
		if (strlen(filesel_prefix)==0)
			sprintf(saveselectedtodisk_dir, "%sphoto", filesel_dir);
		   else 
			sprintf(saveselectedtodisk_dir, "%s%s", filesel_dir,filesel_prefix);
	        sprintf(filesel_cwd, "%s", filesel_dir);
		gtk_widget_destroy(filesel);
	}

	while (node->next) {
		node = node->next; i++;
		if (node->active) {
			if ((strcmp("i", type)==0)||(strcmp("in", type)==0)) {
				sprintf(status, "Saving Image #%03i...", i);
				update_status(status);
				sprintf(fname, "%s-%03i",
					saveselectedtodisk_dir, pic);
				savepictodisk(i, 0, fname);
			} else {
				sprintf(status, "Saving Thumbnail #%03i...", i);
				update_status(status);
				sprintf(fname, "%s-thumbnail-%03i",
					saveselectedtodisk_dir, pic);
				savepictodisk(i, 1, fname);
			}
			pic++;
		}
	}
	sprintf(fname, N_("Done. Images saved in %s"), filesel_cwd);
	update_status(fname);
}

void appendpic(gint picNum, gint thumbnail, gint fromCamera, char *fileName, int factor)
{
	int w, h;
	char fname[15], error[32], process[1024],
	     imagename[1024],*openName;
	
	GtkWidget *scrwin, *label;
	GdkImlibImage *scaledImage;
	GdkPixmap *pixmap;
	struct Image *im;

	struct ImageMembers *node = &Images;

	while (node->next)
		node = node->next;

	node->next = malloc(sizeof(struct ImageMembers));
	
	node = node->next;
	node->imlibimage = NULL;
	node->image = NULL;
	node->button = NULL;
	node->page = NULL;
	node->label = NULL;
	node->frame = NULL;
	node->picture = NULL;
	node->text = NULL;
	node->info = NULL;
	node->next = NULL;
	node->active = 0;

	if (fromCamera) {
		if (cam_busy) {
 			dialog_error(N_("Camera is already busy"));
			return;
		}
		set_toolbar_busy();
		im = Camera->ops->get_picture(picNum, thumbnail);
		set_toolbar_ready();
		if (im == 0 || im == NULL) {
			sprintf(error, N_("Could not retrieve #%i"), picNum);
  			dialog_error(error);
  			return;
		}
		node->imlibimage =  gdk_imlib_load_image_mem(im->image,
				im->image_size);
		free_image(im);
		if (post_process) {
			sprintf(imagename, "%s/gphoto-image-%03i.jpg",
				gphotoDir, picNum);
			gdk_imlib_save_image(node->imlibimage,
				imagename, NULL);
			gdk_imlib_kill_image(node->imlibimage);
			sprintf(process, post_process_script, imagename);
			system(process);
			node->imlibimage = gdk_imlib_load_image(imagename);
			if (node->imlibimage==0) {
			        sprintf(error, N_("Could not load image %s to memory"), imagename);
				dialog_error(error);
			}
			remove(imagename);
		}
	 } else
		node->imlibimage = gdk_imlib_load_image(fileName);

	if (node->imlibimage==0  || node->imlibimage==NULL) {
		sprintf(error, N_("Could not load image #%i to memory"), picNum);
		dialog_error(error);
		return;
	}
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;

	w = (w * factor)/100;
	h = (h * factor)/100;

	scaledImage = gdk_imlib_clone_scaled_image(node->imlibimage,w,h);
	gdk_imlib_kill_image(node->imlibimage);

	if (scaledImage==0) {
		sprintf(error, N_("Could not load image #%i to memory"), picNum);
		dialog_error(error);
		return;
	}
	node->imlibimage = scaledImage;
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;

	gdk_imlib_render(node->imlibimage, w, h);
	pixmap = gdk_imlib_move_image(node->imlibimage);
	node->image = gtk_pixmap_new(pixmap, NULL);
	gtk_widget_show(node->image);

	/* Append to notebook -------------------------------- */
	sprintf(fname, "%03i", picNum);
	node->info = strdup(fname);
	if (fromCamera) {
		sprintf(fname, "%03i", picNum);
		label = gtk_label_new(fname);
	} else {
		openName = strrchr(fileName, (int)'/');
		label = gtk_label_new(&openName[1]);
	}
	node->page = gtk_vbox_new(FALSE,0);
	gtk_widget_show(node->page);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), node->page, label);
	scrwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show(scrwin);
	gtk_container_add(GTK_CONTAINER(node->page), scrwin);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrwin),
					      node->image);
}

/* GTK Functions ---------------------------------------------
   ----------------------------------------------------------- */

void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	destroy(widget, data);
	return FALSE;
}


/* Callbacks -------------------------------------------------
   ----------------------------------------------------------- */

void port_dialog_update(GtkWidget *entry, GtkWidget *label)
{
	int i;
	char model[60];

	sprintf(model, "%s", gtk_entry_get_text(GTK_ENTRY(entry)));

	for (i = 0; cameras[i].name; i++) {
		if (strcmp(model, cameras[i].name) == 0) {
			gtk_label_set_text(GTK_LABEL(label),
				cameras[i].ops->description());
			return;
		}
	}
}

char detected_camera[1024];

void detected_select_row(GtkWidget *clist, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
	char *text;

	gtk_clist_get_text(GTK_CLIST(clist), row, column, &text);
	strcpy(detected_camera, text);
}

void detected_unselect_row(GtkWidget *clist, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
	strcpy(detected_camera, "");
}

void port_dialog(void)
{
	GtkWidget *dialog, *mainhbox;

	GtkWidget *label, *vbox, *scrwin, *notebook;

	GtkWidget *detectedbox, *detected, *scrolled_window;
	GtkWidget *detectedlist, *listitem;

	GtkWidget *choosebox, *choose;
	GtkWidget *topportbox, *midportbox, *bottomportbox;
	GtkWidget *port0, *port1, *port2, *port3, *other, *ent_other;
	GtkWidget *combo, *description;

	GtkWidget *button, *cbutton;

	GSList *listgroup, *portgroup;
	GList *cam_list;

	char serial_port_prefix[20], tempstring[64], status[128];
	char temp[64];
	char *camera_selected;
	int i, count;
	int sd = -1;

#ifdef GPIO_USB
	struct usb_bus *bus;
	struct usb_device *dev;
#endif

#ifdef linux
	struct stat st;
	strcpy(serial_port_prefix, "/dev/ttyS");
	/* check for devfs */
	if (stat("/dev/tts", &st) == 0) {
		if (S_ISDIR(st.st_mode))
			strcpy(serial_port_prefix, "/dev/tts/");
	}
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	strcpy(serial_port_prefix, "/dev/tty0");
#elif defined(sun)
        strcpy(serial_port_prefix, "/dev/ttya");
#else
	strcpy(serial_port_prefix, "/dev/tty0");
#endif

	/*
	 * Main window
	 */
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Choose a Camera");
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	gtk_widget_set_usize(dialog, 360, 360);

	/* Vertical box to store the 2 boxes side by side */
	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook,
		TRUE, TRUE, 0);
/*
	mainhbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(mainhbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), mainhbox,
		TRUE, TRUE, 0);
*/
	/*
	 * Detected camera section
	 */

	/* Vertical box to store label and list */
	label = gtk_label_new("Detected Cameras");
	gtk_widget_show(label);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

	/* Button to select between detected/choose */
/*
	detected = gtk_radio_button_new(NULL);
	gtk_widget_show(detected);
	listgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(detected));
	gtk_box_pack_start(GTK_BOX(vbox), detected, FALSE, FALSE, 0);
*/

	label = gtk_label_new("Detected Cameras");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* this is the scrolled window to put the GtkList widget inside */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolled_window, 250, 150);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);  
	gtk_widget_show(scrolled_window);

	strcpy(detected_camera, "");

	detectedlist = gtk_clist_new(1);
	gtk_widget_show(detectedlist);
	gtk_container_add(GTK_CONTAINER(scrolled_window), detectedlist);
	gtk_clist_set_shadow_type(GTK_CLIST(detectedlist), GTK_SHADOW_IN);
	gtk_clist_set_column_width(GTK_CLIST(detectedlist), 0, 10);
	gtk_clist_set_selection_mode(GTK_CLIST(detectedlist),
		GTK_SELECTION_SINGLE);

	gtk_signal_connect(GTK_OBJECT(detectedlist), "select_row",
		GTK_SIGNAL_FUNC(detected_select_row), NULL);

	gtk_signal_connect(GTK_OBJECT(detectedlist), "unselect_row",
		GTK_SIGNAL_FUNC(detected_unselect_row), NULL);

	count = 0;
#ifdef GPIO_USB
	usb_init();
	usb_find_busses();
	usb_find_devices();
	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			for (i = 0; cameras[i].name; i++) {
				if ((dev->descriptor.idVendor == 0) &&
				    (dev->descriptor.idProduct == 0))
					continue;

				if ((dev->descriptor.idVendor == cameras[i].idVendor) &&
				    (dev->descriptor.idProduct == cameras[i].idProduct)) {
					gtk_clist_append(GTK_CLIST(detectedlist), &cameras[i].name);
					count++;
				}
			}
		}
	}
#endif

	/*
	 * Select a camera manually
	 */

	/* Button to select between detected/choose */
/*
	choose = gtk_radio_button_new(listgroup);
	gtk_widget_show(choose);
	listgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(choose));
	gtk_box_pack_start(GTK_BOX(choosebox), choose, FALSE, FALSE, 0);

	if (!count)
		gtk_button_clicked(GTK_BUTTON(choose));
*/

	/* Vertical box to store label, combo, description and portboxes */
	label = gtk_label_new("Manually Select a Camera");
	gtk_widget_show(label);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

	label = gtk_label_new("Select a Camera");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	cam_list = NULL;
	combo = gtk_combo_new();
	gtk_widget_show(combo);	
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	for (i = 0; cameras[i].name; i++)
		cam_list = g_list_append(cam_list, cameras[i].name);

	gtk_combo_set_popdown_strings(GTK_COMBO(combo), cam_list);
	if (Camera)
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry),
			Camera->name);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
	
	label = gtk_label_new(" ");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("Library Description");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


	description = gtk_label_new(" ");
	gtk_widget_show(description);
	gtk_label_set_justify(GTK_LABEL(description), GTK_JUSTIFY_LEFT);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry),
		"changed", GTK_SIGNAL_FUNC(port_dialog_update), 
		(gpointer)description);

	scrwin = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin),
                                                GTK_POLICY_AUTOMATIC,
                                                GTK_POLICY_AUTOMATIC);
        gtk_widget_show(scrwin);
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrwin),
                                                description);
	gtk_box_pack_start(GTK_BOX(vbox), scrwin, TRUE, TRUE, 0);

	port_dialog_update(GTK_COMBO(combo)->entry, description);


	label = gtk_label_new(" ");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	bottomportbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(bottomportbox);
	gtk_box_pack_end(GTK_BOX(vbox), bottomportbox, FALSE, TRUE, 0);

	midportbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(midportbox);
	gtk_box_pack_end(GTK_BOX(vbox), midportbox, FALSE, TRUE, 0);

	topportbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(topportbox);
	gtk_box_pack_end(GTK_BOX(vbox), topportbox, FALSE, TRUE, 0);

	label = gtk_label_new("Select a Port");	gtk_widget_show(label);
	gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	/* Enumerate the serial port devices */

	portgroup = NULL;
	sprintf(temp, "%s0 (COM1)", serial_port_prefix);
	port0 = gtk_radio_button_new_with_label(NULL, temp);
	gtk_widget_show(port0);
	if (strcmp(serial_port, temp)==0)
		gtk_button_clicked(GTK_BUTTON(port0));

	portgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(port0));
	sprintf(temp, "%s1 (COM2)", serial_port_prefix);
	port1 = gtk_radio_button_new_with_label(portgroup, temp);
	gtk_widget_show(port1);
	if (strcmp(serial_port, temp)==0)
		gtk_button_clicked(GTK_BUTTON(port1));

	portgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(port1));
	sprintf(temp, "%s2 (COM3)", serial_port_prefix);
	port2 = gtk_radio_button_new_with_label(portgroup, temp);
	gtk_widget_show(port2);
	if (strcmp(serial_port, temp)==0)
		gtk_button_clicked(GTK_BUTTON(port2));

	portgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(port2));
	sprintf(temp, "%s3 (COM4)", serial_port_prefix);
	port3 = gtk_radio_button_new_with_label(portgroup, temp);
	gtk_widget_show(port3);
	if (strcmp(serial_port, temp)==0)
		gtk_button_clicked(GTK_BUTTON(port3));

	portgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(port3));
	other = gtk_radio_button_new_with_label(portgroup, "Other:");
	gtk_widget_show(other);
	portgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(other));

	ent_other = gtk_entry_new();
	gtk_widget_show(ent_other);

	switch (camera_type) {
	case GPHOTO_CAMERA_NONE:
		break;
	case GPHOTO_CAMERA_SERIAL:
		if (strlen(serial_port_prefix) == strlen(serial_port)-1) {
			if (strncmp(serial_port, serial_port_prefix, 
			    strlen(serial_port)-1) != 0) {
				gtk_entry_set_text(GTK_ENTRY(ent_other), serial_port);
				gtk_button_clicked(GTK_BUTTON(other));
			}
		} else {
			gtk_entry_set_text(GTK_ENTRY(ent_other), serial_port);
			gtk_button_clicked(GTK_BUTTON(other));
		}
		gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 1);
		break;
#ifdef GPIO_USB
	case GPHOTO_CAMERA_USB:
		if (count)
			gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 0);
		break;
#endif
	}

/*	gtk_box_pack_start(GTK_BOX(choosebox), label, FALSE, FALSE, 0); */
	gtk_box_pack_start(GTK_BOX(topportbox), port0, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midportbox), port1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(topportbox), port2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midportbox), port3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottomportbox), other, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottomportbox), ent_other, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Save");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				button, TRUE, TRUE, 0);

	cbutton = gtk_button_new_with_label("Cancel");
	gtk_widget_show(cbutton);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				cbutton, TRUE, TRUE, 0);

	if (wait_for_hide(dialog, button, cbutton) == 0)
		return;

#ifdef GPIO_USB
	if (gtk_notebook_current_page(GTK_NOTEBOOK(notebook)) == 0) {
		camera_type = GPHOTO_CAMERA_USB;
		printf("[%s] selected\n", detected_camera);
		set_camera(detected_camera);
	} else  {
#endif
		camera_type = GPHOTO_CAMERA_SERIAL;
		camera_selected = gtk_entry_get_text(
			GTK_ENTRY(GTK_COMBO(combo)->entry));
		
		if (GTK_WIDGET_STATE(port0) == GTK_STATE_ACTIVE) {
			sprintf(tempstring, "%s0", serial_port_prefix);
			strcpy(serial_port, tempstring);
		} else if (GTK_WIDGET_STATE(port1) == GTK_STATE_ACTIVE) {
			sprintf(tempstring, "%s1", serial_port_prefix);
			strcpy(serial_port, tempstring);
		} else if (GTK_WIDGET_STATE(port2) == GTK_STATE_ACTIVE) {
			sprintf(tempstring, "%s2", serial_port_prefix);
			strcpy(serial_port, tempstring);
		} else if (GTK_WIDGET_STATE(port3) == GTK_STATE_ACTIVE) {
			sprintf(tempstring, "%s3", serial_port_prefix);
			strcpy(serial_port, tempstring);
		} else if (GTK_WIDGET_STATE(other) == GTK_STATE_ACTIVE) {
			sprintf(tempstring, "%s",
				gtk_entry_get_text(GTK_ENTRY(ent_other)));
			strcpy(serial_port, tempstring);
		}
#ifdef GPIO_USB
	}

	if (camera_type == GPHOTO_CAMERA_SERIAL) {
#endif
	   if (strcmp(camera_selected, "Browse Directory")!=0) {
		sd = open(serial_port, O_RDWR, 0);
		if (sd < 0) {
			sprintf(status, "Error: failed to open %s",serial_port);
			update_status(status);
			dialog_notice("Missing Serial Device Permissions",
					"Please check the permissions "
					"(see the manual).", GTK_JUSTIFY_FILL);
		} else {
			sprintf(status, "Opened %s.", serial_port);
			update_status(status);
			save_config();
		}
		close(sd);
	  }
#ifdef GPIO_USB
	} else {
		save_config();
		serial_port[0] = 0;
	}
#endif
	set_camera(camera_selected);
	gtk_widget_destroy(dialog);
}

int load_config(void)
{
	char fname[1024];
	FILE *conf;

	char *stripcrlf(char *string)
	{
		while (string[strlen(string) - 1] == '\n')
			string[strlen(string) - 1] = 0;

		return string;
	}

	sprintf(fname, "%s/gphotorc", gphotoDir);
	if (conf = fopen(fname, "r")) {
		char config_camera[1025] = "";

		if (fgets(fname, sizeof(fname) - 1, conf))
			strcpy(serial_port, stripcrlf(fname));
		if (fgets(fname, sizeof(fname) - 1, conf))
			strcpy(config_camera, stripcrlf(fname));
		if (fgets(fname, sizeof(fname) - 1, conf))
			strcpy(post_process_script, stripcrlf(fname));
		if (fgets(fname, sizeof(fname) - 1, conf))
			camera_type = atoi(stripcrlf(fname));
		fclose(conf);

		set_camera(config_camera);
		return 1;
	}

	/* reset to defaults, and save */
#ifdef __NetBSD__
	sprintf(serial_port, "/dev/tty00");
#else   
	sprintf(serial_port, "/dev/ttyS0");
#endif
	sprintf(post_process_script, "");
	camera_type = GPHOTO_CAMERA_NONE;
	save_config();
	return 0;
}

void save_config(void)
{
	char gphotorc[1024];
	FILE *conf;

	sprintf(gphotorc, "%s/gphotorc", gphotoDir);
	if ((conf = fopen(gphotorc, "w"))) {
		fprintf(conf, "%s\n", serial_port);
		if (Camera)
			fprintf(conf, "%s\n", Camera->name);
		else
			fprintf(conf, "\n");
		fprintf(conf, "%s\n", post_process_script);
		fprintf(conf, "%d\n", camera_type);
		fclose(conf);
		return;
	}
	if (!command_line_mode)
		dialog_error(N_("Could not open $HOME/.gphoto/gphotorc for writing."));
	else {
#ifdef GPHOTO_DEBUG
		g_message ("Could not open $HOME/.gphoto/gphotorc for writing.");
#endif
	}
}


/* used until callback is implemented */
void menu_selected(gchar *toPrint)
{
	char Error[1024];

	sprintf(Error, N_("%s is not yet implemented"), toPrint);
	dialog_error(Error);
}

void remove_thumbnail(int i)
{
	int x;
	struct ImageMembers *node = &Thumbnails;
	struct ImageMembers *prev = node;

	for (x = 0; x < i; x++) {
		prev = node;
		node = node->next;
	}

	if (node) {
	  prev->next = node->next;
	  free_imagemembers (node);
	}
}

/* Place a thumbnail into the index at location corresponding to node */
void insert_thumbnail(struct ImageMembers *node)
{
	char status[256], error[32];
	GdkPixmap *pixmap;
	GdkImlibImage *scaledImage;
	GtkWidget *vbox;
	GtkTooltips *tooltip;
	int i, x;
	int w, h;
	struct ImageMembers *other=&Thumbnails;
	struct Image *im;
	double aspectRatio;
	GString *tag;

	/* maybe this info should be part of the imageinfo structure */
	/* Find node in the list */
	for (i = 0; other && other != node; i++)
		other = other->next;

	if (!other)	/* didn't find node in the list of thumbs */
		return;

	/* If the thumbnail is already there, then just load the image */
	if (node->image && node->imlibimage) {
		appendpic(i, 0, TRUE, NULL, 100);
		return;
	}

	sprintf(status, "Getting thumbnail %i...", i);
	update_status(status);

	im = Camera->ops->get_picture(i, 1);
	if (im == 0 || im == NULL) {
		sprintf(error, N_("Could not retrieve #%i"), i);
		dialog_error(error);
		return;
	}

	tag=g_string_new("");
	g_string_sprintf(tag, "Picture #%i\n", i); 
	for (x=0; x<im->image_info_size; x+=2) {
		g_string_sprintfa(tag,
			"%s:%s\n",im->image_info[x],im->image_info[x+1]);
	}
  
	tooltip = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltip,node->button,tag->str, NULL);

	node->imlibimage = gdk_imlib_load_image_mem(im->image, im->image_size);

	free_image (im);
	if (node->imlibimage==0) {
		sprintf(error, N_("Could not load image #%i to memory"), i);
		dialog_error(error);
		return;
	}

	if (!node->imlibimage) {
		sprintf(error, N_("Could not open #%i"), i);
		dialog_error(error);
		return;
	}

	w = node->imlibimage->rgb_width; 
	h = node->imlibimage->rgb_height;
	gdk_imlib_render(node->imlibimage, w, h);

	/* Scale thumbnail to button size. Max height is 60. Max width is 80 so
	 * adjust dimensions while preserving aspect ratio.
	 */
	aspectRatio = h/w; 
	if (aspectRatio > 0.75) {
		/* Thumbnail is tall. Adjust height to 60. Width will be < 80. */
		w = (gint)(60.0 * w / h);
		h = 60;
	} else {
		/* Thumbnail is wide. Adjust width to 80. Height will be < 60. */
		h = (gint)(80.0 * h / w);
		w = 80;
	}
	scaledImage = gdk_imlib_clone_scaled_image(node->imlibimage,w,h);
	gdk_imlib_kill_image(node->imlibimage);
	node->imlibimage = scaledImage;
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;
	gdk_imlib_render(node->imlibimage, w, h);
	pixmap = gdk_imlib_move_image(node->imlibimage);
	node->image = gtk_pixmap_new(pixmap, NULL);  
	gtk_widget_show(node->image);

	/* this approach is a little dangerous...*/
	vbox = gtk_container_children(GTK_CONTAINER(node->button))->data;
	gtk_container_add(GTK_CONTAINER(vbox), 
		    node->image);
}

/* intercept mouse click on a thumbnail button */
int thumb_click(GtkWidget *widget, GdkEventButton *event, gpointer callback_data)
{
	if (callback_data)
		return FALSE;

	/* Double click will (re)-load the thumbnail */
	if (event->type == GDK_2BUTTON_PRESS) {
		if (cam_busy) {
			dialog_error(N_("Camera is already busy"));
			return;
		}
		set_toolbar_busy();
		insert_thumbnail((struct ImageMembers *)callback_data);
		set_toolbar_ready();
		return TRUE;
	}

	return FALSE;
}

/* 
   get index of images and place in main page table 
        calling with getthumbs==0  makes a set of blank buttons
	                      !=0  downloads the thumbs for each
*/
void makeindex(gint getthumbs)
{
	int i;
	int num_pictures_taken;
	char status[256];

	GtkStyle *style;
	GtkWidget *vbox;

	struct ImageMembers *node = &Thumbnails;

	update_status(N_("Getting Index..."));

	while (Thumbnails.next != NULL)
		remove_thumbnail(1);

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	num_pictures_taken = Camera->ops->number_of_pictures();
	set_toolbar_ready();

#ifdef GPHOTO_DEBUG
	g_message ("num_pictures_taken is %d", num_pictures_taken);
#endif

	if (num_pictures_taken == -1) {
		dialog_error(N_("Could not get the number of pictures"));
		return;
	}
	
	if (index_table) {
                gtk_container_remove (GTK_CONTAINER(index_vp), index_table);
                index_table=NULL;
                gphoto_canvas_destroy();
        }
        index_table=gphoto_canvas_create (97*6+5,97*(num_pictures_taken/6)+5);
        gtk_container_add(GTK_CONTAINER(index_vp), index_table);

	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}

	set_toolbar_busy();
	okAction = 1;
	for (i = 0; i < num_pictures_taken; i++) {
		if (!okAction) {
			set_toolbar_ready();
			update_progress(0);
			update_status(N_("Download cancelled."));
			return;
		}
		node->next = malloc (sizeof(struct ImageMembers));
		node = node->next;
		node->imlibimage = NULL;
		node->image = NULL;
		node->button = NULL;
		node->page = NULL;
		node->label = NULL;
		node->info = NULL;
		node->next = NULL;
		node->frame = NULL;
		node->picture = NULL;
		node->text = NULL;
		node->active = 0;
		
		node->button = gtk_toggle_button_new();
		gtk_widget_show(node->button);

		/* make a label for the thumbnail */
		sprintf(status, "%03i", i + 1);
		node->info=strdup(status);
		node->label=gtk_label_new(node->info);
		gtk_widget_show(node->label);

		/* gtk_widget_set_rc_style(node->button);*/
		style = gtk_style_copy(gtk_widget_get_style(node->button));
		style->bg[GTK_STATE_ACTIVE].green = 0;
		style->bg[GTK_STATE_ACTIVE].blue = 0;
		gtk_widget_set_style(node->button, style);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_border_width(GTK_CONTAINER(vbox), 3);
		gtk_widget_set_usize(vbox, 87, 78);

		gtk_widget_show(vbox);

		gtk_container_add(
			GTK_CONTAINER(node->button),vbox);
		gtk_container_add(GTK_CONTAINER(vbox),
				  node->label);

		/* Get the thumbnail */
		node->image=NULL;
		node->imlibimage=NULL;

		progress_min=(float)i/(float)num_pictures_taken;
		progress_max=(float)(i+1)/(float)num_pictures_taken;
		if (getthumbs)
			insert_thumbnail(node);

		gtk_signal_connect(GTK_OBJECT(node->button), 
				   "button_press_event",
				   GTK_SIGNAL_FUNC(thumb_click),
				   node);

		node->x = 97*(i%6)+5;
                node->y = 97*(i/6)+5;
                node->width = 87;
                node->height= 87;
                add_to_canvas (node);
                update_progress(progress_max);
	}
	set_toolbar_ready();
	update_progress(0);
	update_status(N_("Done getting index."));
}

/* get index of images and place in main page table */
void getindex(void)
{
	makeindex(1);
}

/* get empty index of images and place in main page table */
void getindex_empty(void)
{
	makeindex(0);
}

void halt_action(void)
{
	okAction = 0;
}

/* get selected pictures */
void getpics(char *pictype)
{
	getpics_with_scale (pictype, 100);
}

/* get selected pictures at a given scale */
void getpics_with_scale(char *pictype, int factor)
{
	char status[256];
	gint i=0;
	gint x=0, y=0;	

	struct ImageMembers *node = &Thumbnails;
	
	while (node->next) {	
		node = node->next;
		if (node->active)
			x++;
	}
	if (!x) {
		update_status("Nothing selected for download.");
		return;
	}

	node = &Thumbnails;
	update_progress(0);
	okAction = 1;
	while (node->next && okAction) {
		node = node->next;
		i++;
		if (node->active) {
			progress_min=(float)y/(float)x;
			progress_max=(float)(y+1)/(float)x;
			y++;
			if ((strcmp("i", pictype) == 0) ||
			    (strcmp("ti", pictype) == 0)) {
				sprintf(status, "Getting Image #%03i...", i);
				update_status(status);
				appendpic(i, 0, TRUE, NULL, factor);
			}
			if ((strcmp("t", pictype) == 0) ||
			    (strcmp("ti", pictype) == 0)) {			
				appendpic(i, 1, TRUE, NULL, factor);
				sprintf(status, "Getting Thumbnail #%03i...", i);
				update_status(status);
			}
			gphoto_canvas_item_toggle_active (node);
			update_progress(progress_max);
		}
	}

	if (okAction)
		update_status("Done downloading.");
	else
		update_status("Download halted.");
	update_progress(0);
}

void remove_image(int i)
{
	int x=0;

	struct ImageMembers *node = &Images;
	struct ImageMembers *prev = node;

	for (; x < i; x++) {
		prev = node;
		node = node->next;
	}

	prev->next = node->next;
	free_imagemembers(node);
}

void closepic(void)
{
	int currentPage;

	currentPage = gtk_notebook_current_page(GTK_NOTEBOOK(notebook));
	if (currentPage)
		remove_image(currentPage);
}

void save_opened_image(int i, char *filename)
{
	int x=0;

	struct ImageMembers *node = &Images;

	if (!confirm_overwrite(filename))
		return;

	for (; x < i; x++)
		node = node->next;
	
	if (gdk_imlib_save_image(node->imlibimage,filename, NULL) == 0) {
		dialog_error(N_("Could not save image. Please make\n"
				"sure that you typed the image\n"
				"extension and that permissions for\n"
				"the directory are correct."));
		return;
	}
}

GtkWidget *save_dialog_filew;

void save_dialog_update(GtkWidget *button, GtkWidget *label)
{
	GList *child;

	child = gtk_container_children(
		GTK_CONTAINER(GTK_FILE_SELECTION(save_dialog_filew)->main_vbox));
	/* get the dir/file list box children */
	child = gtk_container_children(
        	GTK_CONTAINER(child->next->next->data));

	if (GTK_TOGGLE_BUTTON(button)->active) {
		gtk_widget_hide(GTK_WIDGET(child->next->data));
		gtk_label_set_text(GTK_LABEL(label), 
		"Save all opened images (enter the filename prefix below)");
	} else {
		gtk_widget_show(GTK_WIDGET(child->next->data));
		gtk_label_set_text(GTK_LABEL(label),"Save all opened images");
	}
}

void save_dialog(GtkWidget *widget, gpointer data)
{
	GtkWidget *saveall_checkbutton;
	int currentPic, i;
	char filename[1024];

	sprintf(filename, "%s/*.*", filesel_cwd);

	save_dialog_filew = gtk_file_selection_new("Save Image(s)...");
	gtk_window_set_position(GTK_WINDOW(save_dialog_filew), GTK_WIN_POS_CENTER);
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(save_dialog_filew), 
		filename);
	saveall_checkbutton = gtk_check_button_new_with_label(
		"Save all opened images");
	gtk_widget_show(saveall_checkbutton);
	gtk_signal_connect(GTK_OBJECT(saveall_checkbutton),
		"clicked", GTK_SIGNAL_FUNC(save_dialog_update),
		GTK_BIN(saveall_checkbutton)->child);
	gtk_box_pack_start_defaults(
		GTK_BOX(GTK_FILE_SELECTION(save_dialog_filew)->main_vbox),
		saveall_checkbutton);

	if (wait_for_hide(save_dialog_filew,
	    GTK_FILE_SELECTION(save_dialog_filew)->ok_button,
	    GTK_FILE_SELECTION(save_dialog_filew)->cancel_button) == 0)
		return;

	currentPic = gtk_notebook_current_page(GTK_NOTEBOOK(notebook));
	if ((!currentPic) &&
	   (!GTK_TOGGLE_BUTTON(saveall_checkbutton)->active)) {
		update_status(N_("Saving the index is not yet supported"));
		return;
	}
	if (!GTK_TOGGLE_BUTTON(saveall_checkbutton)->active) {
		save_opened_image(currentPic, 
			gtk_file_selection_get_filename(
			GTK_FILE_SELECTION(save_dialog_filew)));
		return;
	}

	i = 1;
	while (gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i) != NULL){
		sprintf(filename, "%s-%03i.jpg",
			gtk_file_selection_get_filename(
			GTK_FILE_SELECTION(save_dialog_filew)), i);
		save_opened_image(i, filename);
		i++;
	}
	gtk_widget_destroy(save_dialog_filew);
}

void open_dialog(GtkWidget *widget, gpointer data)
{
	GtkWidget *filew;

	filew = gtk_file_selection_new("Open Image...");
	gtk_window_set_position(GTK_WINDOW(filew),
		GTK_WIN_POS_CENTER);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION
		(filew), filesel_cwd);
	if (wait_for_hide(filew, GTK_FILE_SELECTION(filew)->ok_button,
	    GTK_FILE_SELECTION(filew)->cancel_button) == 0)
		return;

	appendpic(0, 0, FALSE, gtk_file_selection_get_filename(
		GTK_FILE_SELECTION(filew)), 100);	
	gtk_widget_destroy(filew);
}

void print_pic(void)
{
	int i, currentPic, pid;
	char command[1024], fname[1024];
	struct ImageMembers *node = &Images;

	GtkWidget *dialog, *label, *entry, *hbox, *okbutton, *cancelbutton;

	currentPic = gtk_notebook_current_page (GTK_NOTEBOOK(notebook));
	if (!currentPic) {
		update_status("Can't print the index yet.");
		return;
	}

        dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), N_("Print Image..."));
	gtk_window_set_position (GTK_WINDOW (dialog),GTK_WIN_POS_CENTER);
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_widget_show(hbox);
        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			hbox);

        label = gtk_label_new(N_("Print Command:"));
        gtk_widget_show(label);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
                
        entry = gtk_entry_new();
        gtk_widget_show(entry);
        gtk_entry_set_text(GTK_ENTRY(entry), "lpr -r");
        gtk_box_pack_end_defaults(GTK_BOX(hbox),entry); 

	label = gtk_label_new(
"* The filename is appended to the end\
   of the command.\
* The \"-r\" flag is needed to delete \
  the temporary file. If not used, a \
  temporary file will be in your $HOME/.gphoto\
  directory.");
        gtk_widget_show(label);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
				    label);

        okbutton = gtk_button_new_with_label(N_("Print"));
        gtk_widget_show(okbutton);
        GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
                                    okbutton);

        cancelbutton = gtk_button_new_with_label(N_("Cancel"));
        gtk_widget_show(cancelbutton);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
                                    cancelbutton);

        gtk_widget_show(dialog);
        gtk_widget_grab_default(okbutton);

	if (wait_for_hide(dialog, okbutton, cancelbutton) == 0)
		return;

	for (i = 0; i < currentPic; i++)
		node = node->next;

	pid = getpid();
	sprintf(fname, "%s/gphoto-%i-%03i.jpg", gphotoDir, pid, currentPic);
	gdk_imlib_save_image(node->imlibimage, fname,NULL);

	update_status(N_("Now spooling..."));
	sprintf(command, "%s %s", gtk_entry_get_text(GTK_ENTRY(entry)),
		fname);

	system(command);
	update_status(N_("Spooling done. Printing may take a minute."));
	gtk_widget_destroy(dialog);
}

void select_all(void)
{
	struct ImageMembers *node = &Thumbnails;

	while (node->next) {
		node = node->next;
		gphoto_canvas_item_set_active (node);
	}
}

void select_inverse(void)
{
	struct ImageMembers *node = &Thumbnails;

	while (node->next) {
		node = node->next;
		gphoto_canvas_item_toggle_active (node);
	}
}

void select_none(void)
{
	struct ImageMembers *node = &Thumbnails;

	while (node->next) {
		node = node->next;
		gphoto_canvas_item_unset_active (node);
	}
}

static int
test_overlap (double x0, double x1, double x2, double x3)
{
        double x0_,x1_,x2_,x3_;

        x0_=MIN (x0,x1);
        x1_=MAX (x0,x1);
        x2_=MIN (x2,x3);
        x3_=MAX (x2,x3);
        return ((x0_<=x2_ && x2_<=x1_)
              ||(x0_<=x3_ && x3_<=x1_)
              ||(x2_<=x0_ && x0_<=x3_)
              ||(x2_<=x1_ && x1_<=x3_));
}

void
toggle_in_area(double x1, double y1, double x2, double y2)
{
        struct ImageMembers *node = &Thumbnails;
        double nx1,nx2,ny1,ny2;

        while (node->next) {
                node = node->next;
                nx1=node->x;
                ny1=node->y;
                nx2=node->x+80;
                ny2=node->y+60;
                if (test_overlap (nx1, nx2, x1, x2)
                    && test_overlap (ny1, ny2, y1, y2))
                        gphoto_canvas_item_toggle_active (node);
        }
}

void color_dialog(void)
{
	int i, currentPic;
	struct ImageMembers *node = &Images;

	extern GtkWidget *img_edit_new(struct ImageMembers *n);

	currentPic = gtk_notebook_current_page(GTK_NOTEBOOK(notebook));
	if (!currentPic) {
		update_status("Can't modify the index colors.");
		return;
	}

	for (i = 0; i < currentPic; i++)
                node = node->next;

	gtk_widget_show(GTK_WIDGET(img_edit_new(node)));
}

GtkWidget *resize_dialog_width, *resize_dialog_height,
	  *resize_dialog_constrain;
char	  resize_dialog_w[10], resize_dialog_h[10];
guint	  resize_dialog_width_handler,resize_dialog_height_handler;

void resize_dialog_update(GtkWidget *entry)
{
	char newentry[12];
	int oldw, neww, oldh, newh, newvalue;
	
	if (!GTK_TOGGLE_BUTTON(resize_dialog_constrain)->active)
		return;

	gtk_signal_handler_block(GTK_OBJECT(resize_dialog_width),
		resize_dialog_width_handler);
	gtk_signal_handler_block(GTK_OBJECT(resize_dialog_height),
		resize_dialog_height_handler);

	oldw = atoi(resize_dialog_w);
	oldh = atoi(resize_dialog_h);
	neww = atoi(gtk_entry_get_text(GTK_ENTRY(resize_dialog_width)));
	newh = atoi(gtk_entry_get_text(GTK_ENTRY(resize_dialog_height)));

	if (entry == resize_dialog_width) {
		/* update height */
		newvalue = neww * oldh / oldw;
		if (!newvalue)
			newvalue = 1;
		sprintf(newentry, "%i", newvalue);
		gtk_entry_set_text(GTK_ENTRY(resize_dialog_height),
			newentry);
	}

	if (entry == resize_dialog_height) {
		/* update width */
		newvalue = newh * oldw / oldh;
		if (!newvalue)
			newvalue = 1;
		sprintf(newentry, "%i", newvalue);
		gtk_entry_set_text(GTK_ENTRY(resize_dialog_width),
			newentry);
	}

	gtk_signal_handler_unblock(GTK_OBJECT(resize_dialog_width),
		resize_dialog_width_handler);
	gtk_signal_handler_unblock(GTK_OBJECT(resize_dialog_height),
		resize_dialog_height_handler);
}

void resize_dialog(void)
{
	int i, w, h, currentPic;
	char *dimension;
	char error[128];

	GtkWidget *dialog, *rbutton, *button;
	GtkWidget *label, *hbox;

	GdkImlibImage *scaledImage;
	GdkPixmap *pixmap;

	struct ImageMembers *node = &Images;

	currentPic = gtk_notebook_current_page (GTK_NOTEBOOK(notebook));
	if (!currentPic) {
		update_status("Can't scale the index.");
		return;
	}

	for (i = 0; i < currentPic; i++)
		node = node->next;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), N_("Resize Image..."));
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox);

	label = gtk_label_new(N_("Width:"));
	gtk_widget_show(label);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

	resize_dialog_width = gtk_entry_new();
	gtk_widget_show(resize_dialog_width);
	sprintf(resize_dialog_w, "%i", node->imlibimage->rgb_width);
	gtk_entry_set_max_length(GTK_ENTRY(resize_dialog_width), 10);
	gtk_entry_set_text(GTK_ENTRY(resize_dialog_width),
		resize_dialog_w);
	resize_dialog_width_handler = 
		gtk_signal_connect_object(GTK_OBJECT(resize_dialog_width),
		"changed", GTK_SIGNAL_FUNC(resize_dialog_update),	
		GTK_OBJECT(resize_dialog_width));
	gtk_box_pack_start_defaults(GTK_BOX(hbox),resize_dialog_width);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
				hbox);
		
	label = gtk_label_new(N_("Height:"));
	gtk_widget_show(label);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

	resize_dialog_height = gtk_entry_new();
	gtk_widget_show(resize_dialog_height);
	sprintf(resize_dialog_h, "%i", node->imlibimage->rgb_height);
	gtk_entry_set_max_length(GTK_ENTRY(resize_dialog_height), 10);
	gtk_entry_set_text(GTK_ENTRY(resize_dialog_height), 
		resize_dialog_h);
	resize_dialog_height_handler = 
		gtk_signal_connect_object(GTK_OBJECT(resize_dialog_height),
		"changed", GTK_SIGNAL_FUNC(resize_dialog_update),	
		GTK_OBJECT(resize_dialog_height));
	gtk_box_pack_start_defaults(GTK_BOX(hbox), resize_dialog_height);

	resize_dialog_constrain = gtk_check_button_new_with_label(
		"Constrain Proportions");
	gtk_widget_show(resize_dialog_constrain);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(resize_dialog_constrain), TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),
				resize_dialog_constrain);

	rbutton = gtk_button_new_with_label(N_("Resize"));
	gtk_widget_show(rbutton);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
					rbutton);

	button = gtk_button_new_with_label(N_("Cancel"));
	gtk_widget_show(button);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
					button);

	/* Wait for the user to close the window */
	if (wait_for_hide(dialog, rbutton, button) == 0)
		return;
	/* ------------------------------------- */

	dimension = gtk_entry_get_text(GTK_ENTRY(resize_dialog_width));
	if (!strlen(dimension))
		return;
	w = atoi(dimension);
	dimension = gtk_entry_get_text(GTK_ENTRY(resize_dialog_height));
	if (!strlen(dimension))
		return;
	h = atoi(dimension);

	scaledImage = gdk_imlib_clone_scaled_image(node->imlibimage,w,h);
	if (!scaledImage) {
		sprintf(error, N_("Could not scale the image"), i);
		dialog_error(error);
		gtk_widget_destroy(dialog);
		return;
	}

	gdk_imlib_kill_image(node->imlibimage);

	node->imlibimage = scaledImage;
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;
	gdk_imlib_render(node->imlibimage, w, h);
	pixmap = gdk_imlib_move_image(node->imlibimage);
	gtk_widget_hide(node->image);
	gtk_pixmap_set(GTK_PIXMAP(node->image), pixmap, NULL);
	gtk_widget_show(node->image);
	gtk_widget_destroy(dialog);
}

void manip_pic(gchar *Option)
{
	int i=0, currentPic, w, h;
	struct ImageMembers *node = &Images;

	GdkPixmap *pixmap;
	
	currentPic = gtk_notebook_current_page(GTK_NOTEBOOK(notebook));
	if (!currentPic) {
		update_status("Can't manipulate the index.");
		return;
	}
	while (i < currentPic) {
		node = node->next;
		i++;
	}

	switch (Option[0]) {
		case 'r':
			gdk_imlib_rotate_image(node->imlibimage, 0);
			gdk_imlib_flip_image_horizontal(node->imlibimage);
			break;
		case 'l':
			gdk_imlib_flip_image_horizontal(node->imlibimage);
			gdk_imlib_rotate_image(node->imlibimage, 0);
			break;
		case 'h':
			gdk_imlib_flip_image_horizontal(node->imlibimage);
			break;
		case 'v':
			gdk_imlib_flip_image_vertical(node->imlibimage);
			break;
		default:
			break;
	}
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;
	gdk_imlib_render(node->imlibimage, w, h);
	pixmap = gdk_imlib_move_image(node->imlibimage);
	gtk_widget_hide(node->image);
	gtk_pixmap_set(GTK_PIXMAP(node->image), pixmap, NULL);
	gtk_widget_show(node->image);
}

void summary_dialog(void)
{
	if (cam_busy) {
		dialog_error(N_("Camera is already busy"));
		return;
	}
	set_toolbar_busy();
	dialog_notice(N_("Camera Summary"), 
		      Camera->ops->summary(), 
		      GTK_JUSTIFY_FILL);
	set_toolbar_ready();
}

/* Decreases image size by factor n % */
void scale(int factor)
{
	char error[128];
	int i=0, currentPic;
	int w, h;
/*
	char w_size[10], h_size[10];
*/
	GdkImlibImage *scaledImage;
	GdkPixmap *pixmap;

	struct ImageMembers *node = &Images;

	w = h = 0;

	currentPic = gtk_notebook_current_page (GTK_NOTEBOOK(notebook));
	if (!currentPic) {
		update_status("Can't scale the index!");
		return;
	}
  
	while (i < currentPic) {
		node = node->next;
		i++;
	}
  
	/* Why is this done? -jerdfelt */
/*
	sprintf(w_size, "%i", node->imlibimage->rgb_width);
	sprintf(h_size, "%i", node->imlibimage->rgb_height);

	w = atoi(w_size);
	h = atoi(h_size);
*/
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;
  
	w = (w * factor)/100;
	h = (h * factor)/100;
  
	scaledImage = gdk_imlib_clone_scaled_image(node->imlibimage,w,h);
	if (!scaledImage) {
		sprintf(error, N_("Could not scale the image"), i);
		dialog_error(error);
		return;
	}
	
	gdk_imlib_kill_image(node->imlibimage);
  
	node->imlibimage = scaledImage;
	w = node->imlibimage->rgb_width;
	h = node->imlibimage->rgb_height;
	gdk_imlib_render(node->imlibimage, w, h);
	pixmap = gdk_imlib_move_image(node->imlibimage);
	gtk_widget_hide(node->image);
	gtk_pixmap_set(GTK_PIXMAP(node->image), pixmap, NULL);
	gtk_widget_show(node->image);
	update_status("Done.");
}
 
/* Scales image size by 50% */
void scale_half(void)
{
	update_status("Scaling image by 50%...");
	scale(50);
}

/* Scales image size by 200% */
void scale_double(void)
{
	update_status("Scaling image by 200%...");
	scale(200);
}

void save_images (gpointer data, guint action, GtkWidget *widget)
{
	saveselectedtodisk(widget, "i");
}

void open_images(gpointer data, guint action, GtkWidget *widget)
{
	getpics_with_scale("i",100);
}

void open_images_and_scale50(gpointer data, guint action, GtkWidget *widget)
{
	getpics_with_scale("i",50);
}

void open_images_and_scale25(gpointer data, guint action, GtkWidget *widget)
{
	getpics_with_scale("i",25);
}

void save_thumbs(gpointer data, guint action, GtkWidget *widget)
{
	saveselectedtodisk(widget, "t");
}

void open_thumbs(gpointer data, guint action, GtkWidget *widget)
{
	getpics_with_scale("t",100);
}

void save_both (gpointer data, guint action, GtkWidget *widget)
{
	saveselectedtodisk(widget, "ti");
	/* don't prompt for directory when saving images */
}

void open_both(gpointer data, guint action, GtkWidget *widget)
{
	getpics_with_scale("ti",100);
}


void post_process_change(GtkWidget *widget, GtkWidget *win)
{
	GtkWidget *dialog, *ok, *cancel, *label, *pp, *script;

	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GtkStyle *style;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), N_("Post-Processing Options")); 
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	ok = gtk_button_new_with_label(N_("OK"));
	gtk_widget_show(ok);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		ok);

	cancel = gtk_button_new_with_label(N_("Cancel"));
	gtk_widget_show(cancel);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		cancel);

	pp = gtk_check_button_new_with_label(N_("Enable post-processing"));
	gtk_widget_show(pp);
	if (post_process)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pp), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pp,
		FALSE, FALSE, 0);

	label = gtk_label_new(N_("Post-processing command-line:"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
		TRUE, TRUE, 0);
	
	script = gtk_entry_new();
	gtk_widget_show(script);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_entry_set_text(GTK_ENTRY(script), post_process_script);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), script,
		FALSE, FALSE, 0);

	label = gtk_label_new(N_("Note: gPhoto will replace \"%s\"\n"
				 "in the script command-line with\n"
				 "the full path to the selected image.\n"
				 "See the User's Manual\in the Help menu\n"
				 "for more information. "));
	gtk_widget_show(label);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
		TRUE, TRUE, 4);

	gtk_widget_show(dialog);

	/* Wait for them to close the dialog */
	if (wait_for_hide(dialog, ok, cancel) == 0)
		return;

	if ((strstr(gtk_entry_get_text(GTK_ENTRY(script)), "%s") == NULL)
	   && GTK_TOGGLE_BUTTON(pp)->active) {
		dialog_error(N_("Missing \"%s\" in the post-processing entry. "
				"This is required so the post-processing program "
				"knows where the image is located."));
		return;
	}

	style = gtk_widget_get_style(win);

	post_process = (int)GTK_TOGGLE_BUTTON(pp)->active;
	sprintf(post_process_script, "%s",
		gtk_entry_get_text(GTK_ENTRY(script)));
	save_config();
	if (post_process)
		/* We are turning on post_process'ing */
		pixmap = gdk_pixmap_create_from_xpm_d(win->window, &bitmap,
                 	&style->bg[GTK_STATE_NORMAL],
			(gchar **)post_processing_on_xpm);
	   else
		/* We are turning off post_process'ing */
		pixmap = gdk_pixmap_create_from_xpm_d(win->window, &bitmap,
                	&style->bg[GTK_STATE_NORMAL],
			(gchar **)post_processing_off_xpm);
        gtk_pixmap_set(GTK_PIXMAP(post_process_pixmap), pixmap, bitmap);
	gtk_widget_destroy(dialog);
}

void browse_gallery(void)
{
	char *buf;

	buf = malloc(256);
	snprintf(buf, 256, "file:%sindex.html", filesel_cwd);
	url_send_browser(buf);
	free(buf);
}

void browse_help(void)
{
	url_send_browser("http://www.gphoto.org/help.php3");
}

void browse_gphoto(void)
{
	url_send_browser("http://www.gphoto.org/");
}

void browse_feedback(void)
{
	url_send_browser("http://www.gphoto.org/feedback.php3");
}

void browse_news(void)
{
	url_send_browser("http://www.gphoto.org/news.php3");
}

void browse_download(void)
{
	url_send_browser("http://www.gphoto.org/download.php3");
}

void browse_cameras(void)
{
	url_send_browser("http://www.gphoto.org/cameras.php3");
}

void browse_supporting(void)
{
	url_send_browser("http://www.gphoto.org/supporting.php3");
}

void browse_discussion(void)
{
	url_send_browser("http://www.gphoto.org/lists.php3");
}

void browse_team(void)
{
	url_send_browser("http://www.gphoto.org/team.php3");
}

void browse_themes(void)
{
	url_send_browser("http://www.gphoto.org/themes.php3");
}

void browse_links(void)
{
	url_send_browser("http://www.gphoto.org/links.php3");
}

void browse_todo(void)
{
	url_send_browser("http://www.gphoto.org/todo.php3");
}
