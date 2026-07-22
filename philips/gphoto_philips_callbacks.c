/*  $Id: gphoto_philips_callbacks.c,v 1.6 2000/10/28 10:48:55 ole Exp $ */

/* 
 * Philips Digital Camera configuration callbacks
 *
 * Copyright (c) 1999 Bob Paauwe
 *
 * Controlls what happens when the user manipulates the various
 * controls in the configuration dialog box. Mainly, it stores
 * the values the users selects so that they can be sent to the
 * camera if they choose to "apply" them.
 *
 * This has been tested with the Philips ESP80SXG digital camera
 * but should work with the Ricoh RDC-4300 and maybe the RDC-4200
 * and RDC-5000.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <time.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk_imlib.h>

/* prototypes for io library calls */
#include "philips.h"

extern char *Philips_models[];
extern long cameraid; /* this should be global or returned */
extern int philips_debugflag;
extern PhilipsCfgInfo *p_cfg_info;



gboolean
on_resolution_combo_focus_out_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	char *string;

	string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( strcmp ( string, "640 x 480" ) )
			p_cfg_info->resolution = 1;
		else if ( strcmp ( string, "1280 x 960" ) ) 
			p_cfg_info->resolution = 4;
		else if ( strcmp ( string, "900 x 600" ) ) 
			p_cfg_info->resolution = 5;
		else if ( strcmp ( string, "1800 x 1200" ) ) 
			p_cfg_info->resolution = 6;
		}
	
	return FALSE;
}


gboolean
on_quality_combo_focus_out_event       (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	char *string;

	string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( strcmp ( string, "fine" ) )
			p_cfg_info->compression = 4;
		else if ( strcmp ( string, "normal" ) )
			p_cfg_info->compression = 2;
		else if ( strcmp ( string, "economy" ) )
			p_cfg_info->compression = 1;
		else if ( strcmp ( string, "none" ) )
			p_cfg_info->compression = 0;
		}

  return FALSE;
}


gboolean
on_white_combo_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	char *string;

	string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( strcmp ( string, "Auto" ) )
			p_cfg_info->white = 0;
		else if ( strcmp ( string, "Outdoors" ) )
			p_cfg_info->white = 1;
		else if ( strcmp ( string, "Flourescent" ) )
			p_cfg_info->white = 2;
		else if ( strcmp ( string, "Incandescent" ) )
			p_cfg_info->white = 3;
		else if ( strcmp ( string, "Black & White" ) )
			p_cfg_info->white = 4;
		else if ( strcmp ( string, "Sepia" ) )
			p_cfg_info->white = 5;
		else if ( strcmp ( string, "Overcast" ) )
			p_cfg_info->white = 6;
		}

  return FALSE;
}


gboolean
on_record_combo_focus_out_event        (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	char *string;

	string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( strcmp ( string, "Image" ) )
			p_cfg_info->mode = 0;
		else if ( strcmp ( string, "Character" ) )
			p_cfg_info->mode = 1;
		else if ( strcmp ( string, "Multi-Shot" ) )
			p_cfg_info->mode = 2;
		else if ( strcmp ( string, "Sound only" ) )
			p_cfg_info->mode = 3;
		else if ( strcmp ( string, "Images & Sound" ) )
			p_cfg_info->mode = 4;
		else if ( strcmp ( string, "Character & Sound" ) )
			p_cfg_info->mode = 6;
		}

  return FALSE;
}


gboolean
on_flash_comb_focus_out_event          (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	char *string;

	string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
printf ( "Flash setting set to [%s]\n", string );
		if ( strncasecmp ( string, "Auto", 4 ) )
			p_cfg_info->flash = 0;
		else if ( strncasecmp ( string, "Off", 3 ) )
			p_cfg_info->flash = 1;
		else if ( strncasecmp ( string, "On", 2 ) )
			p_cfg_info->flash = 2;
		else if ( strncasecmp ( string, "Synchronized", 12 ) )
			p_cfg_info->flash = 2;
		else if ( strncasecmp ( string, "On w/o Red-Eye", 14 ) )
			p_cfg_info->flash = 3;
		else if ( strncasecmp ( string, "Auto with Red-Eye", 17 ) )
			p_cfg_info->flash = 4;
		else if ( strncasecmp ( string, "Synchronized with Red-Eye", 25 ) )
			p_cfg_info->flash = 5;
		else if ( strncasecmp ( string, "On with Red-Eye", 15 ) )
			p_cfg_info->flash = 6;
		}

  return FALSE;
}


void
on_copyright_string_activate           (GtkEditable     *editable,
                                        gpointer         user_data)
{
	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		strcpy ( p_cfg_info->copyright, (char *)gtk_entry_get_text(GTK_ENTRY(editable)) );
		}

}


gboolean
on_copyright_string_focus_out_event    (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		strcpy ( p_cfg_info->copyright, (char *)gtk_entry_get_text(GTK_ENTRY(widget)) );
		}

  return FALSE;
}

void
on_date_togglebutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	/* set date */

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( GTK_TOGGLE_BUTTON (togglebutton)->active )  {
			p_cfg_info->date = time(NULL);
			p_cfg_info->date_dirty = 1;
			}
		else 
			p_cfg_info->date_dirty = 0;
		}
}

void
on_maunual_checkbutton_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkAdjustment	*adj;

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( ! GTK_TOGGLE_BUTTON (togglebutton)->active ) 
			p_cfg_info->exposure = 0xff;
		else {
			adj = gtk_range_get_adjustment( GTK_RANGE(user_data) );
			if ( adj->value < -1.5 ) 
				p_cfg_info->exposure = 1;
			else if ( (adj->value >= -1.5) && (adj->value < -1.0) ) 
				p_cfg_info->exposure = 2;
			else if ( (adj->value >= -1.0) && (adj->value < -0.5) ) 
				p_cfg_info->exposure = 3;
			else if ( (adj->value >= -0.5) && (adj->value < 0.0) ) 
				p_cfg_info->exposure = 4;
			else if ( (adj->value >= 0.0) && (adj->value < 0.5) ) 
				p_cfg_info->exposure = 5;
			else if ( (adj->value >= 0.5) && (adj->value < 1.0) ) 
				p_cfg_info->exposure = 6;
			else if ( (adj->value >= 1.0) && (adj->value < 1.5) ) 
				p_cfg_info->exposure = 7;
			else if ( (adj->value >= 1.5) && (adj->value < 2.0) ) 
				p_cfg_info->exposure = 8;
			else if ( adj->value >= 2.0  ) 
				p_cfg_info->exposure = 9;
			}
		}

}


gboolean
on_exposure_hscale_focus_out_event     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	GtkAdjustment	*adj;

	adj = gtk_range_get_adjustment( GTK_RANGE(widget) );
	if ( GTK_TOGGLE_BUTTON (user_data)->active ) {
		if ( adj->value < -1.5 ) 
			p_cfg_info->exposure = 1;
		else if ( adj->value >= -1.5 && adj->value < -1.0 ) 
			p_cfg_info->exposure = 2;
		else if ( adj->value >= -1.0 && adj->value < -0.5 ) 
			p_cfg_info->exposure = 3;
		else if ( adj->value >= -0.5 && adj->value < 0.0 ) 
			p_cfg_info->exposure = 4;
		else if ( adj->value >= 0.0 && adj->value < 0.5 ) 
			p_cfg_info->exposure = 5;
		else if ( adj->value >= 0.5 && adj->value < 1.0 ) 
			p_cfg_info->exposure = 6;
		else if ( adj->value >= 1.0 && adj->value < 1.5 ) 
			p_cfg_info->exposure = 7;
		else if ( adj->value >= 1.5 && adj->value < 2.0 ) 
			p_cfg_info->exposure = 8;
		else if ( adj->value >= 2.0  ) 
			p_cfg_info->exposure = 9;
		}
	else 
		p_cfg_info->exposure = 0xff;

  return FALSE;
}


void
on_macro_checkbutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( GTK_TOGGLE_BUTTON (togglebutton)->active ) 
			p_cfg_info->macro = 1;
		else
			p_cfg_info->macro = 0;
		}

}


gboolean
on_zoom_hscale_focus_out_event         (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
	GtkAdjustment	*adj;

	adj = gtk_range_get_adjustment( GTK_RANGE(widget) );

	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		p_cfg_info->zoom = (int)adj->value;
		}

  return FALSE;
}


/* Change label on button and clear or set philips_debugflag */
void
on_debug_togglebutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if ( p_cfg_info == NULL ) {
		printf ( "Error: No configuration data structure.\n" );
		}
	else {
		if ( GTK_TOGGLE_BUTTON (togglebutton)->active ) 
			philips_debugflag = 1;
		else
			philips_debugflag = 0;
		}
}


void
on_ok_button_clicked                   (GtkButton       *button,
                                        GtkWidget       *user_data)
{
	int	error;

	if ( p_cfg_info != NULL ) {
		philips_get_config_options ( user_data, p_cfg_info );
		if ( (error = philips_setcfginfo ( p_cfg_info )) ) {
			printf ( "Error occured setting camera configuration. %d\n", error );
			}
		p_cfg_info = NULL;
		}
	gtk_widget_hide ( user_data );

}


void
on_apply_button_clicked                (GtkButton       *button,
                                        GtkWidget       *user_data)
{
	int	error;

	if ( p_cfg_info != NULL ) {
		philips_get_config_options ( user_data, p_cfg_info );
		if ( (error = philips_setcfginfo ( p_cfg_info )) ) {
			printf ( "Error occured setting camera configuration. %d\n", error );
			}
		p_cfg_info = philips_getcfginfo ( &error );
		}

}


void
on_cancel_button_clicked               (GtkButton       *button,
                                        GtkWidget       *user_data)
{
	if ( p_cfg_info != NULL ) {
		free ( p_cfg_info );
		p_cfg_info = NULL;
		}
	gtk_widget_hide ( user_data );

}


void
on_Camera_Configuration_destroy        (GtkObject       *object,
                                        gpointer         user_data)
{

}

/* 
 * Memory togglebutton
 *
 * The memory togglebutton is reserved for switching between
 * smart media card memory and internal  camera memory once
 * I know how to do it.
 */

gboolean
on_memory_togglebutton_event           (GtkWidget       *widget,
                                        GdkEventClient  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


/* 
 * Date Imprint togglebutton
 *
 * The date imprint togglebutton is reserved for turning on the
 * date imprint mode of the RDC-5000. 
 */

void
on_date_imprint_togglebutton_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


