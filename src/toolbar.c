#include <config.h>
#include "gphoto.h"
#include "callbacks.h"
#include "gallery.h"
#include "live.h"
#include "toolbar.h"

/* The toolbar xpm icons */

#include "icons/close_image.xpm"
#include "icons/colors.xpm"
#include "icons/configure.xpm"
#include "icons/delete_images.xpm"
#include "icons/exit.xpm"
#include "icons/fliph.xpm"
#include "icons/flipv.xpm"
#include "icons/get_index.xpm"
#include "icons/get_index_empty.xpm"
#include "icons/get_selected_images.xpm"
#include "icons/help.xpm"
#include "icons/left_arrow.xpm"
#include "icons/mail_image.xpm"
#include "icons/open_image.xpm"
#include "icons/print_image.xpm"
#include "icons/resize.xpm"
#include "icons/right_arrow.xpm"
#include "icons/rotc.xpm"
#include "icons/rotcc.xpm"
#include "icons/save_current_image.xpm"
#include "icons/stop.xpm"
#include "icons/web_browse.xpm"
#include "icons/take_picture.xpm"

/* #include "web_browse.xpm"   */
/* #include "take_picture.xpm" */
/* GtkWidget *browse_button = NULL; */

GtkWidget *buttons[NB_BUTTONS];
extern int cam_busy;

GtkWidget *add_to_toolbar (GtkWidget *mainWin, 
			int button_number, gchar *tooltipText, 
			gchar ** xpmIcon, GtkSignalFunc f, gpointer data,
			GtkWidget *box, int Beginning) {
  
  GtkWidget *button, *gpixmap;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  GtkTooltips *tooltip;
  GtkStyle *style;
  
  if (f == NULL)
    button = gtk_label_new("     ");
  else {
    buttons[button_number] = button = gtk_button_new();
    tooltip = gtk_tooltips_new();
    gtk_tooltips_set_tip(tooltip,button,tooltipText, NULL);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      f, data);
    style = gtk_widget_get_style(mainWin);
    pixmap = gdk_pixmap_create_from_xpm_d(mainWin->window,&bitmap,
					  &style->bg[GTK_STATE_NORMAL],
					  xpmIcon);
    gpixmap = gtk_pixmap_new(pixmap,bitmap);
    gtk_widget_show(gpixmap);
    gtk_container_add(GTK_CONTAINER(button), gpixmap);
  }
  gtk_widget_show(button);
  if (Beginning)
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  else
    gtk_box_pack_end(GTK_BOX(box), button, FALSE, FALSE, 0);
  return (button);
}

void deactivate_button (int button_number) {
	if (buttons[button_number])
		gtk_widget_set_sensitive(GTK_WIDGET(buttons[button_number]), FALSE);
}

void activate_button (int button_number) {
	if (buttons[button_number])
		gtk_widget_set_sensitive(GTK_WIDGET(buttons[button_number]), TRUE);
}

void
set_toolbar_busy ()
{
        int i;

        for (i = 0; i < NB_BUTTONS; i++)
                deactivate_button (i);
        activate_button (STOP_BUTTON);
        cam_busy=1;
}

void
set_toolbar_ready ()
{
        int i;

        for (i = 0; i < NB_BUTTONS; i++)
                activate_button (i);
        deactivate_button (STOP_BUTTON);
        cam_busy=0;
}

void create_toolbar (GtkWidget *box, GtkWidget *mainWin) {

  add_to_toolbar(mainWin, OPEN_BUTTON, N_("Open Image"), open_image_xpm,
                 GTK_SIGNAL_FUNC(open_dialog), NULL, box, 1);
  add_to_toolbar(mainWin, SAVE_BUTTON, N_("Save Opened Image(s)"), save_current_image_xpm,
                 GTK_SIGNAL_FUNC(save_dialog), NULL, box, 1);
  add_to_toolbar(mainWin, PRINT_BUTTON, N_("Print Image"), print_image_xpm,
                 GTK_SIGNAL_FUNC(print_pic), NULL, box, 1);
  add_to_toolbar(mainWin, CLOSE_BUTTON, N_("Close Image"), delete_images_xpm,
                 GTK_SIGNAL_FUNC(closepic), "c", box, 1);  
  add_to_toolbar(mainWin, 0, NULL, NULL, NULL, NULL, box, 1);
  add_to_toolbar(mainWin, PREVIOUS_BUTTON, "Previous page", left_arrow_xpm,
		 GTK_SIGNAL_FUNC(prev_page), "i", box, 1);
  add_to_toolbar(mainWin, NEXT_BUTTON, "Next page", right_arrow_xpm,
		 GTK_SIGNAL_FUNC(next_page), "i", box, 1);
  add_to_toolbar(mainWin, 0, NULL, NULL, NULL, NULL, box, 1);
  add_to_toolbar(mainWin, DOWNLOAD_THUMB_BUTTON, "Download Thumbnail Index", get_index_xpm,
                 GTK_SIGNAL_FUNC(getindex), NULL, box, 1);
  add_to_toolbar(mainWin, DOWNLOAD_EMPTY_BUTTON, "Download Empty Index", get_index_empty_xpm,
                 GTK_SIGNAL_FUNC(getindex_empty), NULL, box, 1);
  add_to_toolbar(mainWin, DOWNLOAD_SELECTED_BUTTON, "Download Selected Images", get_selected_images_xpm,
		 GTK_SIGNAL_FUNC(getpics), "i", box, 1);
  add_to_toolbar(mainWin, DELETE_BUTTON, "Delete Selected Images", close_image_xpm, 
                 GTK_SIGNAL_FUNC(del_dialog), NULL, box, 1);
  add_to_toolbar(mainWin, STOP_BUTTON, "Halt Download", stop_xpm,
		 GTK_SIGNAL_FUNC(halt_action), NULL, box, 1);
  add_to_toolbar(mainWin, 0, NULL, NULL, NULL, NULL, box, 1);
  add_to_toolbar(mainWin, ROTATE_CLOCKWISE_BUTTON, N_("Rotate Clockwise"), rotc_xpm,
		 GTK_SIGNAL_FUNC(manip_pic), "r", box, 1);
  add_to_toolbar(mainWin, ROTATE_CCLOCKWISE_BUTTON, N_("Rotate Counter-Clockwise"), rotcc_xpm,
		 GTK_SIGNAL_FUNC(manip_pic), "l", box, 1);
  add_to_toolbar(mainWin, FLIP_HORIZ_BUTTON, N_("Flip Horizontal"), fliph_xpm,
		 GTK_SIGNAL_FUNC(manip_pic), "h", box, 1);
  add_to_toolbar(mainWin, FLIP_VERT_BUTTON, N_("Flip Vertical"), flipv_xpm,
		 GTK_SIGNAL_FUNC(manip_pic), "v", box, 1);
  add_to_toolbar(mainWin, RESIZE_BUTTON, N_("Resize"), resize_xpm,
		 GTK_SIGNAL_FUNC(resize_dialog),
		 N_("Resize"), box, 1);
  add_to_toolbar(mainWin, COLORS_BUTTON, N_("Colors"), colors_xpm,
		 GTK_SIGNAL_FUNC(color_dialog),
		 N_("Colors"), box, 1);
  add_to_toolbar(mainWin, 0, NULL, NULL, NULL, NULL, box, 1);
  add_to_toolbar(mainWin, GALLERY_BUTTON, "HTML Gallery", web_browse_xpm,
		 GTK_SIGNAL_FUNC(gallery_main), NULL, box, 1);
  add_to_toolbar(mainWin, LIVE_BUTTON, "Live Camera!", take_picture_xpm,
                 GTK_SIGNAL_FUNC(live_main), NULL, box, 1);
  add_to_toolbar(mainWin, CONFIG_BUTTON, N_("Camera Configuration"), configure_xpm, 
		 GTK_SIGNAL_FUNC(configure_call), NULL, box, 1);
  add_to_toolbar(mainWin, 0, NULL, NULL, NULL, NULL, box, 1);
/*    add_to_toolbar(mainWin, HELP_BUTTON, N_("Help"), help_xpm,  */
/*                   GTK_SIGNAL_FUNC(usersmanual_dialog), NULL, box, 1); */
  add_to_toolbar(mainWin, EXIT_BUTTON, N_("Exit gPhoto"), exit_xpm,
                 GTK_SIGNAL_FUNC(delete_event), NULL, box, 1);
}
