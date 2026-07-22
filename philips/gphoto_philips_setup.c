/*  $Id: gphoto_philips_setup.c,v 1.5 2000/10/28 10:48:55 ole Exp $ */

/* 
 * Philips Digital Camera gPhoto interface 
 *
 * Copyright (c) 1999 Bob Paauwe
 *
 *  Set all the widgets in the configuration dialog box to the values
 *  currently held in the camera.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include "philips.h"

extern	int	philips_debugflag;


/*
 *  phlips_set_config_options
 *
 *  Look up all the important widgets (must know their names) and
 *  set them to the values currently held by the camera. Also make
 *  sure that we only display the values that the camera supports.
 */

int	philips_set_config_options ( long camera_id,
								 GtkWidget *Camera_Configuration, 
                                 PhilipsCfgInfo *p_cfg_info )
{
	GtkWidget	*config_widget;
	guint		id;
	char		status[256];
	char		date_str[45];
	GList		*items = NULL;


	/* Start with the debug button, it is easy... */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "debug_togglebutton" );
	if ( !config_widget )
		printf ( "Can't find widget debug_togglebutton....\n" );
	else {  
		if ( philips_debugflag )
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), FALSE);
		}
	
	/* Set the copyright info, it is pretty easy too... */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "copyright_string" );
	if ( config_widget )
		gtk_entry_set_text (GTK_ENTRY (config_widget), p_cfg_info->copyright);

	/* How about setting the status bar, that would be nice... */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "statusbar1" );
	if ( config_widget ) {
		sprintf ( status, " %s",  philips_model ( (int)camera_id ) );
		id = gtk_statusbar_get_context_id( GTK_STATUSBAR(config_widget), "Model" );
		gtk_statusbar_push( GTK_STATUSBAR(config_widget), id, status );
		}

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "statusbar2" );
	if ( config_widget ) {
		sprintf ( status, " %ld pictures",  p_cfg_info->picts );
		id = gtk_statusbar_get_context_id( GTK_STATUSBAR(config_widget), "Pictures" );
		gtk_statusbar_push( GTK_STATUSBAR(config_widget), id, status );
		}

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "statusbar3" );
	if ( config_widget ) {
		sprintf ( status, " %d of %d bytes free",  p_cfg_info->a_memory, p_cfg_info->memory );
		id = gtk_statusbar_get_context_id( GTK_STATUSBAR(config_widget), "Memory" );
		gtk_statusbar_push( GTK_STATUSBAR(config_widget), id, status );
		}

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "statusbar4" );
	if ( config_widget ) {
		/* Get the date and time as a string and strip off the nl */
		sprintf ( date_str, " %s", ctime ( &(p_cfg_info->date) ) );
		date_str[strlen(date_str) - 1] = '\0';

		id = gtk_statusbar_get_context_id( GTK_STATUSBAR(config_widget), "Date" );
		gtk_statusbar_push( GTK_STATUSBAR(config_widget), id, date_str );
		}

	/* Macro Mode... */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "macro_checkbutton" );
	if ( config_widget ) {
		if ( camera_id == RDC_5000 ) {
			gtk_widget_set_sensitive (config_widget, FALSE);
			}
		else {
			if ( p_cfg_info->macro )
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), TRUE);
			else
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), FALSE);
			}
		}

	/* Manual Exposure Mode... */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "maunual_checkbutton" );
	if ( config_widget ) {
		if ( p_cfg_info->exposure != 0xff ) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), TRUE);
			config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "exposure_hscale" );
			switch ( p_cfg_info->exposure ) {
				case 1:
					gtk_scale_set_digits (GTK_SCALE (config_widget), -2.0);
					break;
				case 2:
					gtk_scale_set_digits (GTK_SCALE (config_widget), -1.5);
					break;
				case 3:
					gtk_scale_set_digits (GTK_SCALE (config_widget), -1.0);
					break;
				case 4:
					gtk_scale_set_digits (GTK_SCALE (config_widget), -0.5);
					break;
				case 5:
					gtk_scale_set_digits (GTK_SCALE (config_widget), 0.0);
					break;
				case 6:
					gtk_scale_set_digits (GTK_SCALE (config_widget), 0.5);
					break;
				case 7:
					gtk_scale_set_digits (GTK_SCALE (config_widget), 1.0);
					break;
				case 8:
					gtk_scale_set_digits (GTK_SCALE (config_widget), 1.5);
					break;
				case 9:
					gtk_scale_set_digits (GTK_SCALE (config_widget), 2.0);
					break;
				}
			}
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_widget), FALSE);
		}
	
	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "exposure_hscale" );
	if ( config_widget ) {
		gtk_scale_set_digits (GTK_SCALE (config_widget), p_cfg_info->zoom);
		}
	
	/* Lets try something harder like the resolution combo box */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "resolution_combo" );
	if ( config_widget ) {
		switch ( camera_id ) {
			case RDC_1:
			case RDC_2:
			case RDC_2E:
				items = g_list_append ( items, "768 x 576" );
				break;
			case RDC_300:
			case RDC_300Z:
			case ESP2:
				items = g_list_append ( items, "640 x 480" );
				break;
			case ESP60SXG:
			case ESP50:
				items = g_list_append ( items, "640 x 480" );
				break;
			case ESP80SXG:
			case RDC_4200:
			case RDC_4300:
				items = g_list_append ( items, "640 x 480" );
				items = g_list_append ( items, "1280 x 960" );
				break;
			case RDC_5000:
				items = g_list_append ( items, "896 x 600" );
				items = g_list_append ( items, "1792 x 1200" );
				break;
			case RDC_100G:
				items = g_list_append ( items, "1152 x 872" );
				break;
			default:
				items = g_list_append ( items, "640 x 480" );
			}
		gtk_combo_set_popdown_strings (GTK_COMBO (config_widget), items );
		g_list_free ( items );
		config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "combo_entry7" );
		if ( config_widget ) {
			switch ( p_cfg_info->resolution ) {
				case 1: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "640 x 480" );
					break;
				case 2:
				case 3:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Unkown" );
					break;
				case 4:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "1280 x 960" );
					break;
				case 5:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "896 x 600" );
					break;
				case 6:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "1792 x 1200" );
					break;
				case 7:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "1152 x 872" );
					break;
				}
			}
		}

	/* Quality Combo box */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "quality_combo" );
	if ( config_widget ) {
		items = NULL;
		items = g_list_append ( items, "Fine" );
		items = g_list_append ( items, "Normal" );
		items = g_list_append ( items, "Economy" );
		gtk_combo_set_popdown_strings (GTK_COMBO (config_widget), items );
		g_list_free ( items );
		config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "combo_entry8" );
		if ( config_widget ) {
			switch ( p_cfg_info->compression ) {
				case 1: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Economy" );
					break;
				case 2:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Normal" );
					break;
				case 4:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Fine" );
					break;
				}
			}
		}

	/* Record Mode combo box */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "record_combo" );
	if ( config_widget ) {
		items = NULL;
		switch ( camera_id ) {
			case RDC_1:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Images & Sound" );
				items = g_list_append ( items, "Sound only" );
				items = g_list_append ( items, "Video" );
				break;
			case RDC_2:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Images & Sound" );
				items = g_list_append ( items, "Sound only" );
				items = g_list_append ( items, "Character" );
				items = g_list_append ( items, "Character & Sound" );
				break;
			case RDC_2E:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Multi-Shot" );
				items = g_list_append ( items, "Character" );
				break;
			case RDC_300:
			case RDC_300Z:
			case ESP2:
			case ESP60SXG:
			case ESP50:
				items = g_list_append ( items, "Images only" );
				break;
			case ESP80SXG:
			case RDC_4300:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Images & Sound" );
				items = g_list_append ( items, "Sound only" );
				items = g_list_append ( items, "Character" );
				items = g_list_append ( items, "Character & Sound" );
				break;
			case RDC_4200:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Character" );
				break;
			case RDC_5000:
				items = g_list_append ( items, "Images only" );
				items = g_list_append ( items, "Multi-Shot" );
				items = g_list_append ( items, "Character" );
				break;
			default:
				items = g_list_append ( items, "Images only" );
			}
		gtk_combo_set_popdown_strings (GTK_COMBO (config_widget), items );
		g_list_free ( items );
		config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "combo_entry10" );
		if ( config_widget ) {
			switch ( p_cfg_info->mode ) {
				case 0: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Images only" );
					break;
				case 1: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Character" );
					break;
				case 2:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Multi-Shot" );
					break;
				case 3:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Sound only" );
					break;
				case 4:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Images & Sound" );
					break;
				case 5:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Video" );
					break;
				case 6:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Character & Sound" );
					break;
				}
			}
		}

	/*  White Balance combo box */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "white_combo" );
	if ( config_widget ) {
		items = NULL;
		switch ( camera_id ) {
			case RDC_2:
			case RDC_4300:
			case ESP80SXG:
			case RDC_300:
			case ESP60SXG:
			case ESP50:
			case ESP2:
				items = g_list_append ( items, "Automatic" );
				items = g_list_append ( items, "Outdoors" );
				items = g_list_append ( items, "Flourescent" );
				items = g_list_append ( items, "Incandescent" );
				break;
			case RDC_300Z:
				items = g_list_append ( items, "Automatic" );
				items = g_list_append ( items, "Outdoors" );
				items = g_list_append ( items, "Flourescent" );
				items = g_list_append ( items, "Incandescent" );
				items = g_list_append ( items, "Black & White" );
				break;
			case RDC_4200:
				items = g_list_append ( items, "Automatic" );
				items = g_list_append ( items, "Outdoors" );
				items = g_list_append ( items, "Flourescent" );
				items = g_list_append ( items, "Incandescent" );
				items = g_list_append ( items, "Black & White" );
				items = g_list_append ( items, "Sepia" );
				break;
			case RDC_5000:
				items = g_list_append ( items, "Automatic" );
				items = g_list_append ( items, "Outdoors" );
				items = g_list_append ( items, "Flourescent" );
				items = g_list_append ( items, "Incandescent" );
				items = g_list_append ( items, "Black & White" );
				items = g_list_append ( items, "Sepia" );
				items = g_list_append ( items, "Overcast" );
				break;
			}
		gtk_combo_set_popdown_strings (GTK_COMBO (config_widget), items );
		g_list_free ( items );
		config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "combo_entry9" );
		if ( config_widget ) {
			switch ( p_cfg_info->white ) {
				case 0: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Automatic" );
					break;
				case 1: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Outdoors" );
					break;
				case 2:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Flourescent" );
					break;
				case 3:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Incandescent" );
					break;
				case 4:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Black & White" );
					break;
				case 5:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Sepia" );
					break;
				case 6:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Overcast" );
					break;
				}
			}
		}

	/* Flash combo box */

	config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "flash_combo" );
	if ( config_widget ) {
		items = NULL;
		switch ( camera_id ) {
			case RDC_2:
			case RDC_300:
			case RDC_300Z:
			case RDC_4200:
			case RDC_4300:
			case ESP2:
			case ESP50:
			case ESP60SXG:
			case ESP80SXG:
				items = g_list_append ( items, "Auto" );
				items = g_list_append ( items, "Off" );
				items = g_list_append ( items, "On" );
				break;
			case RDC_5000:
				items = g_list_append ( items, "Auto" );
				items = g_list_append ( items, "Off" );
				items = g_list_append ( items, "On w/o Red-Eye" );
				items = g_list_append ( items, "Synchronized" );
				items = g_list_append ( items, "Auto with Red-Eye" );
				items = g_list_append ( items, "On with Red-Eye" );
				items = g_list_append ( items, "Synchronized with Red-Eye" );
				break;
			}
		gtk_combo_set_popdown_strings (GTK_COMBO (config_widget), items );
		g_list_free ( items );
		config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "combo_entry11" );
		if ( config_widget ) {
			switch ( p_cfg_info->flash ) {
				case 0: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Auto" );
					break;
				case 1: 
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Off" );
					break;
				case 2:
					if ( camera_id == RDC_5000 ) 
						gtk_entry_set_text (GTK_ENTRY (config_widget), "Synchronized" );
					else
						gtk_entry_set_text (GTK_ENTRY (config_widget), "On" );
					break;
				case 3:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "On w/o Red-Eye" );
					break;
				case 4:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Auto with Red-Eye" );
					break;
				case 5:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "Synchronized with Red-Eye" );
					break;
				case 6:
					gtk_entry_set_text (GTK_ENTRY (config_widget), "On with Red-Eye" );
					break;
				}
			}
		}
	return ( 0 );
}	/* End of philips_set_config_options */


/*
 *  phlips_get_config_options
 *
 *  Look up all the important widgets (must know their names) and
 *  get the values currently set. Put store the values in the
 *  camera config structure so the camera can get updated.
 */

int	philips_get_config_options ( GtkWidget *Camera_Configuration, 
                                 PhilipsCfgInfo *p_cfg_info )
{
	GtkWidget	*config_widget;
	char		*string;

	if ( (config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "resolution_combo" )) != NULL ) {
		string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(config_widget)->entry));
		if ( strcmp ( string, "640 x 480" ) == 0 )
		    p_cfg_info->resolution = 1;
		else if ( strcmp ( string, "1280 x 960" ) == 0 )
		    p_cfg_info->resolution = 4;
		else if ( strcmp ( string, "896 x 600" ) == 0 )
		    p_cfg_info->resolution = 5;
		else if ( strcmp ( string, "1792 x 1200" ) == 0 )
		    p_cfg_info->resolution = 6;
		}
	else {
		printf ( "Resolution combo is NULL!\n" );
		}

	if ( (config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "quality_combo" )) != NULL ) {
		string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(config_widget)->entry));
		if ( strcmp ( string, "fine" ) == 0 )
		    p_cfg_info->compression = 4;
		else if ( strcmp ( string, "normal" ) == 0 )
		    p_cfg_info->compression = 2;
		else if ( strcmp ( string, "economy" ) == 0 )
		    p_cfg_info->compression = 1;
		else if ( strcmp ( string, "none" ) == 0 )
		    p_cfg_info->compression = 0;
		}
	else {
		printf ( "Resolution combo is NULL!\n" );
		}

	if ( (config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "white_combo" )) != NULL ) {
		string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(config_widget)->entry));
		if ( strcmp ( string, "Auto" ) == 0 )
		    p_cfg_info->white = 0;
		else if ( strcmp ( string, "Outdoors" ) == 0 )
		    p_cfg_info->white = 1;
		else if ( strcmp ( string, "Flourescent" ) == 0 )
		    p_cfg_info->white = 2;
		else if ( strcmp ( string, "Incandescent" ) == 0 )
		    p_cfg_info->white = 3;
		else if ( strcmp ( string, "Black & White" ) == 0 )
		    p_cfg_info->white = 4;
		else if ( strcmp ( string, "Sepia" ) == 0 )
		    p_cfg_info->white = 5;
		else if ( strcmp ( string, "Overcast" ) == 0 )
		    p_cfg_info->white = 6;
		}
	else {
		printf ( "White combo is NULL!\n" );
		}

	if ( (config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "record_combo" )) != NULL ) {
		string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(config_widget)->entry));

		if ( strncasecmp ( string, "Images only", 11 ) == 0 )
		    p_cfg_info->mode = 0;
		else if ( strncasecmp ( string, "Character", 9 ) == 0 )
		    p_cfg_info->mode = 1;
		else if ( strncasecmp ( string, "Multi-Shot", 10 ) == 0 )
		    p_cfg_info->mode = 2;
		else if ( strncasecmp ( string, "Sound only", 10 ) == 0 )
		    p_cfg_info->mode = 3;
		else if ( strncasecmp ( string, "Images & Sound", 14 ) == 0 )
		    p_cfg_info->mode = 4;
		else if ( strncasecmp ( string, "Character & Sound", 17 ) == 0 )
		    p_cfg_info->mode = 6;
		}
	else {
		printf ( "Record combo is NULL!\n" );
		}

	if ( (config_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (Camera_Configuration), "flash_combo" )) != NULL ) {
		string = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(config_widget)->entry));

		if ( strncasecmp ( string, "Auto", 4 ) == 0 )
		    p_cfg_info->flash = 0;
		else if ( strncasecmp ( string, "Off", 3 ) == 0 )
		    p_cfg_info->flash = 1;
		else if ( strncasecmp ( string, "On", 2 ) == 0 )
		    p_cfg_info->flash = 2;
		else if ( strncasecmp ( string, "Synchronized", 12 ) == 0 )
		    p_cfg_info->flash = 2;
		else if ( strncasecmp ( string, "On w/o Red-Eye", 14 ) == 0 )
		    p_cfg_info->flash = 3;
		else if ( strncasecmp ( string, "Auto with Red-Eye", 17 ) == 0 )
		    p_cfg_info->flash = 4;
		else if ( strncasecmp ( string, "Synchronized with Red-Eye", 25 ) == 0 )
		    p_cfg_info->flash = 5;
		else if ( strncasecmp ( string, "On with Red-Eye", 15 ) == 0 )
		    p_cfg_info->flash = 6;
		}
	else {
		printf ( "Flash combo is NULL!\n" );
		}
	return ( 0 );
} /* End of get widget info */
