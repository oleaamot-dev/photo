/*  Note: You are free to use whatever license you want.
    Eventually you will be able to edit it within Glade. */

/*  project1
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "configure.h"
#include "config_handler.h"

#include "casio_qv_defines.h"


static int picSize = 0;
static int baudrate = DEFAULT;

void
on_okBtn_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
    extern void casio_set_config(int photoSize, int portSpeed, int debug);
    GtkWidget *cfgDlg;
    GtkWidget *debugToggle;
    int debug;

    cfgDlg = (GtkWidget *)user_data;
    debugToggle = gtk_object_get_data(GTK_OBJECT(cfgDlg), "debugToggle");
    
    gtk_widget_unmap(cfgDlg);
/*    debug =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(debugToggle)) ? 1 : 0;
*/
    if (GTK_TOGGLE_BUTTON(debugToggle)->active)
	debug = 1;
      else
	debug = 0;
    casio_set_config(picSize, baudrate, debug);
}


void
setSize                                (GtkButton       *button,
                                        gpointer         user_data)
{
    picSize = (int)user_data;
}

void
on_spd_pressed                         (GtkButton       *button,
                                        gpointer         user_data)
{
    baudrate = (int)user_data;
}

void
cancel_btn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    gtk_widget_unmap((GtkWidget *)user_data);
}

