/*  Note: You are free to use whatever license you want.
    Eventually you will be able to edit it within Glade. */

/*
 *  Copyright (C) <YEAR> <AUTHORS>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "config_handler.h"
#include "configure.h"

#include "casio_qv_defines.h"

GtkWidget*
get_widget                             (GtkWidget       *widget,
                                        gchar           *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
                                                   widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

/* This is an internally used function to set notebook tab widgets. */
void
set_notebook_tab                       (GtkWidget       *notebook,
                                        gint             page_num,
                                        GtkWidget       *widget)
{
  GtkNotebookPage *page;
  GtkWidget *notebook_page;

  page = (GtkNotebookPage*) g_list_nth (GTK_NOTEBOOK (notebook)->children, page_num)->data;
  notebook_page = page->child;
  gtk_widget_ref (notebook_page);
  gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page_num);
  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), notebook_page,
                            widget, page_num);
  gtk_widget_unref (notebook_page);
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (gchar           *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories, g_strdup (directory));
}

/* This is an internally used function to check if a pixmap file exists. */
#ifndef G_DIR_SEPARATOR_S
#define G_DIR_SEPARATOR_S "/"
#endif
gchar*
check_file_exists                      (gchar           *directory,
                                        gchar           *filename)
{
  gchar *full_filename;
  struct stat s;
  gint status;

  full_filename = g_malloc (strlen (directory) + 1 + strlen (filename) + 1);
  strcpy (full_filename, directory);
  strcat (full_filename, G_DIR_SEPARATOR_S);
  strcat (full_filename, filename);

  status = stat (full_filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return full_filename;
  g_free (full_filename);
  return NULL;
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        gchar           *filename)
{
  gchar *found_filename = NULL;
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;
  GList *elem;

  /* We first try any pixmaps directories set by the application. */
  elem = pixmaps_directories;
  while (elem)
    {
      found_filename = check_file_exists ((gchar*)elem->data, filename);
      if (found_filename)
        break;
      elem = elem->next;
    }

  /* If we haven't found the pixmap, try the source directory. */
  if (!found_filename)
    {
      found_filename = check_file_exists ("pixmaps", filename);
    }

  if (!found_filename)
    {
      g_print ("Couldn't find pixmap file: %s", filename);
      return NULL;
    }

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm (NULL, colormap, &mask,
                                                   NULL, found_filename);
  g_free (found_filename);
  if (gdkpixmap == NULL)
    return NULL;
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

GtkWidget*
create_casioConfigDlg ()
{
  GtkWidget *casioConfigDlg;
  GtkWidget *dialog_vbox1;
  GtkWidget *notebook1;
  GtkWidget *vbox2;
  GtkWidget *label4;
  GSList *Size_group = NULL;
  GtkWidget *normalSize;
  GtkWidget *doubleSize;
  GtkWidget *vbox1;
  GSList *Speed_group = NULL;
  GtkWidget *spd_9600;
  GtkWidget *spd_19200;
  GtkWidget *spd_38400;
  GtkWidget *spd_57600;
  GtkWidget *spd_115200;
  GtkWidget *vbox3;
  GtkWidget *debugToggle;
  GtkWidget *Photos;
  GtkWidget *PortSpeed;
  GtkWidget *debug;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox1;
  GtkWidget *okBtn;
  GtkWidget *cancelBtn;

  casioConfigDlg = gtk_dialog_new ();
  gtk_widget_set_name (casioConfigDlg, "casioConfigDlg");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "casioConfigDlg", casioConfigDlg);
  GTK_WIDGET_SET_FLAGS (casioConfigDlg, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (casioConfigDlg);
  gtk_widget_grab_default (casioConfigDlg);
  gtk_widget_set_extension_events (casioConfigDlg, GDK_EXTENSION_EVENTS_ALL);
  gtk_window_set_title (GTK_WINDOW (casioConfigDlg), "Configure Casio Camera");
  gtk_window_set_policy (GTK_WINDOW (casioConfigDlg), TRUE, TRUE, FALSE);

  dialog_vbox1 = GTK_DIALOG (casioConfigDlg)->vbox;
  gtk_widget_set_name (dialog_vbox1, "dialog_vbox1");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  notebook1 = gtk_notebook_new ();
  gtk_widget_set_name (notebook1, "notebook1");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "notebook1", notebook1);
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), notebook1, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "vbox2", vbox2);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox2);

  label4 = gtk_label_new ("Size to use for low resolution photos:");
  gtk_widget_set_name (label4, "label4");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "label4", label4);
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (vbox2), label4, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_LEFT);

  normalSize = gtk_radio_button_new_with_label (Size_group, "320 x 240");
  Size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (normalSize));
  gtk_widget_set_name (normalSize, "normalSize");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "normalSize", normalSize);
  gtk_widget_show (normalSize);
  gtk_box_pack_start (GTK_BOX (vbox2), normalSize, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (normalSize), "pressed",
                      GTK_SIGNAL_FUNC (setSize),
                      (gpointer) 0);

  doubleSize = gtk_radio_button_new_with_label (Size_group, "640 x 480");
  Size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (doubleSize));
  gtk_widget_set_name (doubleSize, "doubleSize");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "doubleSize", doubleSize);
  gtk_widget_show (doubleSize);
  gtk_box_pack_start (GTK_BOX (vbox2), doubleSize, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (doubleSize), "pressed",
                      GTK_SIGNAL_FUNC (setSize),
                      (gpointer) 1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox1);

  spd_9600 = gtk_radio_button_new_with_label (Speed_group, "9600 (Default)");
  Speed_group = gtk_radio_button_group (GTK_RADIO_BUTTON (spd_9600));
  gtk_widget_set_name (spd_9600, "spd_9600");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "spd_9600", spd_9600);
  gtk_widget_show (spd_9600);
  gtk_box_pack_start (GTK_BOX (vbox1), spd_9600, FALSE, FALSE, 0);
  gtk_signal_connect_after (GTK_OBJECT (spd_9600), "pressed",
                            GTK_SIGNAL_FUNC (on_spd_pressed),
                            (gpointer) DEFAULT);

  spd_19200 = gtk_radio_button_new_with_label (Speed_group, "19200");
  Speed_group = gtk_radio_button_group (GTK_RADIO_BUTTON (spd_19200));
  gtk_widget_set_name (spd_19200, "spd_19200");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "spd_19200", spd_19200);
  gtk_widget_show (spd_19200);
  gtk_box_pack_start (GTK_BOX (vbox1), spd_19200, FALSE, FALSE, 0);
  gtk_signal_connect_after (GTK_OBJECT (spd_19200), "pressed",
                            GTK_SIGNAL_FUNC (on_spd_pressed),
                            (gpointer) MID);

  spd_38400 = gtk_radio_button_new_with_label (Speed_group, "38400");
  Speed_group = gtk_radio_button_group (GTK_RADIO_BUTTON (spd_38400));
  gtk_widget_set_name (spd_38400, "spd_38400");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "spd_38400", spd_38400);
  gtk_widget_show (spd_38400);
  gtk_box_pack_start (GTK_BOX (vbox1), spd_38400, FALSE, FALSE, 0);
  gtk_signal_connect_after (GTK_OBJECT (spd_38400), "pressed",
                            GTK_SIGNAL_FUNC (on_spd_pressed),
                            (gpointer) HIGH);

  spd_57600 = gtk_radio_button_new_with_label (Speed_group, "57600");
  Speed_group = gtk_radio_button_group (GTK_RADIO_BUTTON (spd_57600));
  gtk_widget_set_name (spd_57600, "spd_57600");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "spd_57600", spd_57600);
  gtk_widget_show (spd_57600);
  gtk_box_pack_start (GTK_BOX (vbox1), spd_57600, FALSE, FALSE, 0);
  gtk_signal_connect_after (GTK_OBJECT (spd_57600), "pressed",
                            GTK_SIGNAL_FUNC (on_spd_pressed),
                            (gpointer) TOP);

  spd_115200 = gtk_radio_button_new_with_label (Speed_group, "115200");
  Speed_group = gtk_radio_button_group (GTK_RADIO_BUTTON (spd_115200));
  gtk_widget_set_name (spd_115200, "spd_115200");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "spd_115200", spd_115200);
  gtk_widget_show (spd_115200);
  gtk_box_pack_start (GTK_BOX (vbox1), spd_115200, FALSE, FALSE, 0);
  gtk_signal_connect_after (GTK_OBJECT (spd_115200), "pressed",
                            GTK_SIGNAL_FUNC (on_spd_pressed),
                            (gpointer) LIGHT);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox3, "vbox3");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "vbox3", vbox3);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox3);

  debugToggle = gtk_check_button_new_with_label ("Turn on debugging");
  gtk_widget_set_name (debugToggle, "debugToggle");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "debugToggle", debugToggle);
  gtk_widget_show (debugToggle);
  gtk_box_pack_start (GTK_BOX (vbox3), debugToggle, FALSE, FALSE, 0);

  Photos = gtk_label_new ("Photos");
  gtk_widget_set_name (Photos, "Photos");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "Photos", Photos);
  gtk_widget_show (Photos);
  set_notebook_tab (notebook1, 0, Photos);
  gtk_label_set_justify (GTK_LABEL (Photos), GTK_JUSTIFY_LEFT);

  PortSpeed = gtk_label_new ("Port Speed");
  gtk_widget_set_name (PortSpeed, "PortSpeed");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "PortSpeed", PortSpeed);
  gtk_widget_show (PortSpeed);
  set_notebook_tab (notebook1, 1, PortSpeed);

  debug = gtk_label_new ("Debug");
  gtk_widget_set_name (debug, "debug");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "debug", debug);
  gtk_widget_show (debug);
  set_notebook_tab (notebook1, 2, debug);

  dialog_action_area1 = GTK_DIALOG (casioConfigDlg)->action_area;
  gtk_widget_set_name (dialog_action_area1, "dialog_action_area1");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "hbuttonbox1", hbuttonbox1);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbuttonbox1, TRUE, TRUE, 0);

  okBtn = gtk_button_new_with_label ("Ok");
  gtk_widget_set_name (okBtn, "okBtn");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "okBtn", okBtn);
  gtk_widget_show (okBtn);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), okBtn);
  gtk_signal_connect (GTK_OBJECT (okBtn), "clicked",
                      GTK_SIGNAL_FUNC (on_okBtn_clicked),
                      casioConfigDlg);
  gtk_signal_connect_object_after (GTK_OBJECT (okBtn), "clicked",
                                   GTK_SIGNAL_FUNC (gtk_widget_destroy),
                                   GTK_OBJECT (casioConfigDlg));

  cancelBtn = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_name (cancelBtn, "cancelBtn");
  gtk_object_set_data (GTK_OBJECT (casioConfigDlg), "cancelBtn", cancelBtn);
  gtk_widget_show (cancelBtn);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), cancelBtn);
  gtk_signal_connect_object (GTK_OBJECT (cancelBtn), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (casioConfigDlg));

  return casioConfigDlg;
}

