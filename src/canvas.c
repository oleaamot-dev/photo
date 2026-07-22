/* gPhoto - Thumbnail handling with the Gnome Canvas
 *
 * Copyright (C) 2000 The Free Software Foundation
 *
 * Author: Fabrice Bellet <fabrice@gnu.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "gphoto.h"
#include "callbacks.h"

GnomeCanvasGroup *gphoto_canvas_root = NULL;
GtkWidget *gphoto_canvas = NULL;
GtkWidget *gphoto_canvas_table = NULL;
static GnomeCanvasItem *gphoto_rect;

extern struct ImageMembers Thumbnails;

static void
gphoto_zoom_changed (GtkAdjustment *adj, gpointer data)
{
	gnome_canvas_set_pixels_per_unit (data, adj->value);
}

void
gphoto_canvas_item_destroy (struct ImageMembers *node)
{
	if (node) {
		node->active=0;
		if (node->frame) {
			gtk_object_destroy (GTK_OBJECT (node->frame));
			node->frame=NULL;
		}
		if (node->picture) {
			gtk_object_destroy (GTK_OBJECT (node->picture));
			node->picture=NULL;
		}
		if (node->text) {
			gtk_object_destroy (GTK_OBJECT (node->text));
			node->text=NULL;
		}
	}
}

void
gphoto_canvas_destroy ()
{
	struct ImageMembers *node = &Thumbnails;

	gphoto_rect=NULL;
	gphoto_canvas_root=NULL;
	gphoto_canvas=NULL;
	gphoto_canvas_table=NULL;

	while (node->next) {
		node = node->next;
		gphoto_canvas_item_destroy (node);
	}
}

void
gphoto_canvas_item_set_active (struct ImageMembers *node)
{
	if (node && !node->active) {
		if (node->frame) {
			gnome_canvas_item_set (node->frame,
					"x1",(double)(node->x-2),
					"y1",(double)(node->y-2),
					"x2",(double)(node->x+81),
					"y2",(double)(node->y+61),
					NULL);
			gnome_canvas_item_show (node->frame);
		} else
			node->frame = gnome_canvas_item_new (gphoto_canvas_root, 
					gnome_canvas_rect_get_type(),
					"x1",(double)(node->x-2),
					"y1",(double)(node->y-2),
					"x2",(double)(node->x+81),
					"y2",(double)(node->y+61),
					"outline_color", "red",
					"width_pixels", 3,
					NULL);
		node->active=1;
	}
}

void
gphoto_canvas_item_unset_active (struct ImageMembers *node)
{
	if (node && node->active) {
		if (node->frame)
			gnome_canvas_item_hide (node->frame);
		node->active=0;
	}
}

void
gphoto_canvas_item_toggle_active (struct ImageMembers *node)
{
	if (node->active)
		gphoto_canvas_item_unset_active (node);
	else
		gphoto_canvas_item_set_active (node);
}

static gint
event_handler (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	static double x1, y1;
	static double x2, y2;
	double item_x, item_y;
	GdkCursor *fleur;
	static int dragging;
	struct ImageMembers *node=data;

	item_x = event->button.x;
	item_y = event->button.y;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button==1) {
			if (GNOME_IS_CANVAS (widget)) {
				gnome_canvas_window_to_world (GNOME_CANVAS (widget), 
					item_x, item_y, &x1, &y1);

				fleur = gdk_cursor_new (GDK_FLEUR);
				gdk_cursor_destroy (fleur);
				dragging = TRUE;
				x2=x1;
				y2=y1;
				if (gphoto_rect) {
					gnome_canvas_item_set (gphoto_rect,
								"x1",x1,
								"y1",y1,
								"x2",x2,
								"y2",y2,
								NULL);
					gnome_canvas_item_show (gphoto_rect);
				} else
					gphoto_rect = gnome_canvas_item_new (gphoto_canvas_root,
							gnome_canvas_rect_get_type(),
							"x1",x1,
							"y1",y1,
							"x2",x2,
							"y2",y2,
							"outline_color", "red",
							"width_pixels", 1,
							NULL);
				gnome_canvas_item_raise_to_top (gphoto_rect);
			} else 
				if (node)
					gphoto_canvas_item_toggle_active (node);
		}
		return TRUE;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			gnome_canvas_window_to_world (GNOME_CANVAS (gphoto_canvas), 
				item_x, item_y, &x2, &y2);
			gnome_canvas_item_set (gphoto_rect,
					"x1",MIN(x1,x2),
					"y1",MIN(y1,y2),
					"x2",MAX(x1,x2),
					"y2",MAX(y1,y2),
					NULL);
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (dragging) {
			dragging = FALSE;
			gnome_canvas_item_hide (gphoto_rect);
			toggle_in_area(MIN(x1,x2), MIN(y1,y2),
				MAX(x1,x2), MAX(y1,y2));
		}
		break;

	default:
		break;
	}
	return FALSE;
}

static GnomeCanvasItem *
setup_item (GnomeCanvasItem *item, gpointer data)
{
	gtk_signal_connect (GTK_OBJECT (item), "event",
			(GtkSignalFunc) event_handler,
			data);
	return item;
}

GtkWidget *
gphoto_canvas_create (guint width, guint height)
{
	GtkWidget *w;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkAdjustment *adj;

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	gphoto_canvas = gnome_canvas_new ();

	/* Setup canvas items */

	gphoto_canvas_root = gnome_canvas_root (GNOME_CANVAS (gphoto_canvas));

	gtk_signal_connect (GTK_OBJECT (gphoto_canvas), "event",
			(GtkSignalFunc) event_handler,
			NULL);

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	/*
	 * Layout
	 */

	gphoto_canvas_table = gtk_table_new (2, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (gphoto_canvas_table), 4);
	gtk_table_set_col_spacings (GTK_TABLE (gphoto_canvas_table), 4);
	gtk_widget_show (gphoto_canvas_table);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_table_attach (GTK_TABLE (gphoto_canvas_table), frame,
			0, 1, 0, 1,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			0, 0);
	gtk_widget_show (frame);

	gtk_widget_set_usize (gphoto_canvas, width, height);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (gphoto_canvas),
					0, 0, width, height);
	gtk_container_add (GTK_CONTAINER (frame), gphoto_canvas);
	gtk_widget_show (gphoto_canvas);

	w = gtk_hscrollbar_new (GTK_LAYOUT (gphoto_canvas)->hadjustment);
	gtk_table_attach (GTK_TABLE (gphoto_canvas_table), w,
			0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			GTK_FILL,
			0, 0);
	gtk_widget_show (w);

	w = gtk_vscrollbar_new (GTK_LAYOUT (gphoto_canvas)->vadjustment);
	gtk_table_attach (GTK_TABLE (gphoto_canvas_table), w,
			1, 2, 0, 1,
			GTK_FILL,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			0, 0);
	gtk_widget_show (w);

	GTK_WIDGET_SET_FLAGS (gphoto_canvas, GTK_CAN_FOCUS);
	gtk_widget_grab_focus (gphoto_canvas);

	/*
	 * Zoom
	 */
	
	hbox = gtk_hbox_new (FALSE, 4);
	w = gtk_label_new ("Zoom:");
	gtk_box_pack_start (GTK_BOX(hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	adj = gtk_adjustment_new (1.00, 0.05, 4.00, 0.05, 0.50, 0.50);
	gtk_signal_connect (GTK_OBJECT(adj), "value_changed",
		(GtkSignalFunc) gphoto_zoom_changed,
		gphoto_canvas);
	w = gtk_spin_button_new (adj, 0.0, 2);
	gtk_widget_set_usize (w, 50, 0);
	gtk_box_pack_start (GTK_BOX(hbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	gtk_widget_show (hbox);

	gtk_table_attach (GTK_TABLE(gphoto_canvas_table), hbox,
			0, 1, 2, 3,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			GTK_FILL,
			0, 0);

	return gphoto_canvas_table;;
}

void
add_to_canvas (struct ImageMembers *node)
{
	GnomeCanvasItem *item;
	GdkImlibImage *im;

	item = setup_item (gnome_canvas_item_new (gphoto_canvas_root,
				gnome_canvas_image_get_type(),
				"image", node->imlibimage,
				"x", (double)node->x,
				"y", (double)node->y,
				"width", (double)node->imlibimage->rgb_width,
				"height", (double)node->imlibimage->rgb_height,
				"anchor", GTK_ANCHOR_NW,
				NULL), node);
	gnome_canvas_item_lower (item, 1);
	node->picture = item;
	item = setup_item (gnome_canvas_item_new (gphoto_canvas_root,
				gnome_canvas_text_get_type(),
				"text", node->info,
				"x", (double)node->x,
				"y", (double)node->y,
				"font", "-*-clean-medium-r-*-*-12-*-*-*-*-*-*-*",
				"anchor", GTK_ANCHOR_NW,
				"clip_width", (double)node->imlibimage->rgb_width,
				"clip_height", (double)node->imlibimage->rgb_height,
				"x_offset", 30.0,
				"y_offset", 64.0,
				"fill_color", "darkgreen",
				NULL), NULL);
	gnome_canvas_item_lower (item, 1);
	node->text = item;
}
