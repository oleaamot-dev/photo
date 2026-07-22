/* gPhoto - free digital camera utility - http://www.gphoto.org/
 *
 * Copyright (C) 1999  The gPhoto developers  <gphoto-devel@gphoto.org>
 *
 * Color Editing dialog shamelessly copied from Electirc Eyes  
 * by The Rasterman (Carsten Haitzler) with modifications 
 * for Gphoto by Matt Martin (matt.martin@gphoto.org)
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


#include <stdio.h>
#include <stdlib.h>

#include "gphoto.h"
#include <gdk/gdkkeysyms.h>
#include "icons/brightness.xpm"
#include "icons/contrast.xpm"
#include "icons/gamma.xpm"

static void
ee_edit_cb_always_toggle(GtkWidget *widget, gpointer data);
static void
ee_adjustment_value_changed(GtkAdjustment *a);
static void
ee_edit_cb_col_apply(GtkWidget *widget, gpointer data);
static void
ee_edit_cb_col_preview(GtkWidget *widget, gpointer data);

void
ee_edit_update_graphs(GtkWidget *w);

static gint adj_ignore_count = 0;

GdkImlibImage *thisim;
struct ImageMembers *imnode;

static void
ee_adjustment_value_changed(GtkAdjustment *a)
{
  /*  gtk_adjustment_value_changed(a);*/
    gtk_signal_emit_by_name(GTK_OBJECT(a), "value_changed");
}

static void
ee_edit_cb_destroy(GtkWidget * widget, gpointer * data)
{
  gtk_widget_destroy(widget);
  /*  gtk_widget_hide(widget);*/
  data = NULL;
}

static void
render_gray_mod(GtkWidget *a, GdkImlibImage *im, gint br)
{
  unsigned char         modr[256], modg[256], modb[256], modz[256];
  unsigned char        *ptr;
  int                   i, val, x, y;
  static GdkImlibImage *curve = NULL;
  GdkPixmap            *pmap;

  if (!curve)
    {  
      ptr = g_malloc(64 * 64 * 3);
      curve = gdk_imlib_create_image_from_data(ptr, NULL, 64, 64);
      g_free(ptr);
    }
  
  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  *ptr++ = 0;
	  *ptr++ = 0;
	  *ptr++ = 0;
	}
    }

  if (im) 
    {
      gdk_imlib_get_image_red_curve(im, modr);
      gdk_imlib_get_image_green_curve(im, modg);
      gdk_imlib_get_image_blue_curve(im, modb);

      for (i = 0; i < 256; i++)
	modz[i] = 
	  (unsigned char)(((int)modr[i] + (int)modg[i] + (int)modb[i]) / 3);

      for (i = 0; i < 64; i++)
	{
	  val = modz[i << 2] >> 2;
	  ptr = curve->rgb_data + (64 * 3 * 63) + (i * 3);
	  for (y = 0; y < val; y++)
	    {
	      ptr[0] = (i << 1) + (br >> 1);
	      ptr[1] = (i << 1) + (br >> 1);
	      ptr[2] = (i << 1) + (br >> 1);
	      ptr -= (64 * 3);
	    }
	}
    }

  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  if ((!(x % 8)) || (!(y % 8)))
	    {
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	    }
	  else
	    ptr += 3;
	}
    }
  
  gdk_imlib_changed_image(curve);
  gdk_imlib_render(curve, 64, 64);
  pmap = gdk_imlib_move_image(curve);
  if (pmap)
    {
      gdk_window_set_back_pixmap(a->window, pmap, FALSE);
      gdk_imlib_free_pixmap(pmap);
      gdk_window_clear(a->window);
      gdk_flush();
    }
}

static void
render_red_mod(GtkWidget *a, GdkImlibImage *im, gint br)
{
  unsigned char         modz[256];
  unsigned char        *ptr;
  int                   i, val, x, y;
  static GdkImlibImage *curve = NULL;
  GdkPixmap            *pmap;

  if (!curve)
    {  
      ptr = g_malloc(64 * 64 * 3);
      curve = gdk_imlib_create_image_from_data(ptr, NULL, 64, 64);
      g_free(ptr);
    }

  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  *ptr++ = 0;
	  *ptr++ = 0;
	  *ptr++ = 0;
	}
    }
  if (im)
    {
      gdk_imlib_get_image_red_curve(im, modz);

      for (i = 0; i < 64; i++)
	{
	  val = modz[i << 2] >> 2;
	  ptr = curve->rgb_data + (64 * 3 * 63) + (i * 3);
	  for (y = 0; y < val; y++)
	    {
	      ptr[0] = (i << 1) + (br >> 1);
	      ptr[1] = (y * br * i) / (val * 63);
	      ptr[2] = (y * br * i) / (val * 63);
	      ptr -= (64 * 3);
	    }
	}
    }

  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  if ((!(x % 8)) || (!(y % 8)))
	    {
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	    }
	  else
	    ptr += 3;
	}
    }
  
  gdk_imlib_changed_image(curve);
  gdk_imlib_render(curve, 64, 64);
  pmap = gdk_imlib_move_image(curve);
  if (pmap)
    {
      gdk_window_set_back_pixmap(a->window, pmap, FALSE);
      gdk_imlib_free_pixmap(pmap);
      gdk_window_clear(a->window);
      gdk_flush();
    }
}

static void
render_green_mod(GtkWidget *a, GdkImlibImage *im, gint br)
{
  unsigned char         modz[256];
  unsigned char        *ptr;
  int                   i, val, x, y;
  static GdkImlibImage *curve = NULL;
  GdkPixmap            *pmap;

  if (!curve)
    {  
      ptr = g_malloc(64 * 64 * 3);
      curve = gdk_imlib_create_image_from_data(ptr, NULL, 64, 64);
      g_free(ptr);
    }
  
  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  *ptr++ = 0;
	  *ptr++ = 0;
	  *ptr++ = 0;
	}
    }

  if (im)
    {
      gdk_imlib_get_image_green_curve(im, modz);

      for (i = 0; i < 64; i++)
	{
	  val = modz[i << 2] >> 2;
	  ptr = curve->rgb_data + (64 * 3 * 63) + (i * 3);
	  for (y = 0; y < val; y++)
	    {
	      ptr[0] = (y * br * i) / (val * 63);
	      ptr[1] = (i << 1) + (br >> 1);
	      ptr[2] = (y * br * i) / (val * 63);
	      ptr -= (64 * 3);
	    }
	}
    }

  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  if ((!(x % 8)) || (!(y % 8)))
	    {
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	    }
	  else
	    ptr += 3;
	}
    }
  
  gdk_imlib_changed_image(curve);
  gdk_imlib_render(curve, 64, 64);
  pmap = gdk_imlib_move_image(curve);
  if (pmap)
    {
      gdk_window_set_back_pixmap(a->window, pmap, FALSE);
      gdk_imlib_free_pixmap(pmap);
      gdk_window_clear(a->window);
      gdk_flush();
    }
}

static void
render_blue_mod(GtkWidget *a, GdkImlibImage *im, gint br)
{
  unsigned char         modz[256];
  unsigned char        *ptr;
  int                   i, val, x, y;
  static GdkImlibImage *curve = NULL;
  GdkPixmap            *pmap;

  if (!curve)
    {  
      ptr = g_malloc(64 * 64 * 3);
      curve = gdk_imlib_create_image_from_data(ptr, NULL, 64, 64);
      g_free(ptr);
    }
  
  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  *ptr++ = 0;
	  *ptr++ = 0;
	  *ptr++ = 0;
	}
    }

  if (im)
    {
      gdk_imlib_get_image_blue_curve(im, modz);

      for (i = 0; i < 64; i++)
	{
	  val = modz[i << 2] >> 2;
	  ptr = curve->rgb_data + (64 * 3 * 63) + (i * 3);
	  for (y = 0; y < val; y++)
	    {
	      ptr[0] = (y * br * i) / (val * 63);
	      ptr[1] = (y * br * i) / (val * 63);
	      ptr[2] = (i << 1) + (br >> 1);
	      ptr -= (64 * 3);
	    }
	}
    }

  ptr = curve->rgb_data;
  for (y = 0; y < 64; y++)
    {
      for (x = 0; x < 64; x++)
	{
	  if ((!(x % 8)) || (!(y % 8)))
	    {
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	      val = *ptr + 255 - (y << 2);
	      if (val > 255)
		val = 255;
	      *ptr++ = val;
	    }
	  else
	    ptr += 3;
	}
    }
  
  gdk_imlib_changed_image(curve);
  gdk_imlib_render(curve, 64, 64);
  pmap = gdk_imlib_move_image(curve);
  if (pmap)
    {
      gdk_window_set_back_pixmap(a->window, pmap, FALSE);
      gdk_imlib_free_pixmap(pmap);
      gdk_window_clear(a->window);
      gdk_flush();
    }
}

static void
ee_edit_cb_always_toggle(GtkWidget *widget, gpointer data)
{
  gint always;
  
  always = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(data), "always"));
  if (always)
    gtk_object_set_data(GTK_OBJECT(data), "always", GINT_TO_POINTER (0));
  else
    {
      gtk_object_set_data(GTK_OBJECT(data), "always", GINT_TO_POINTER (1));
      ee_edit_cb_col_apply(NULL, NULL);
    }
}

static void
ee_edit_cb_toggle(GtkWidget *widget, gpointer data)
{
  GtkWidget *w, *b;
  gint bt = 0;

  w = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "widget"));
  if (GTK_TOGGLE_BUTTON(widget)->active)
    {
      bt = 0;
      b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "gray_button");  
      if (b != widget)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(b), 0);
      else
	bt = 0;
      b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "red_button");
      if (b != widget)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(b), 0);
      else
	bt = 1;
      b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "green_button");
      if (b != widget)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(b), 0);
      else
	bt = 2;
      b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "blue_button");
      if (b != widget)
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(b), 0);
      else
	bt = 3;
    }
  switch (bt)
    {
     case 0:
      b = gtk_object_get_data(GTK_OBJECT(w), "red_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "green_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "blue_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "gray_controls");
      gtk_widget_show(b);
      break;
     case 1:
      b = gtk_object_get_data(GTK_OBJECT(w), "gray_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "green_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "blue_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "red_controls");
      gtk_widget_show(b);
      break;
     case 2:
      b = gtk_object_get_data(GTK_OBJECT(w), "red_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "gray_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "blue_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "green_controls");
      gtk_widget_show(b);
      break;
     case 3:
      b = gtk_object_get_data(GTK_OBJECT(w), "red_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "green_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "gray_controls");
      gtk_widget_hide(b);
      b = gtk_object_get_data(GTK_OBJECT(w), "blue_controls");
      gtk_widget_show(b);
      break;
     default:
      break;
    }
  ee_edit_update_graphs(w);
}

void
ee_edit_update_graphs(GtkWidget *w)
{
  GtkWidget *a, *b;
  GdkImlibImage *im;
  
  if (!w)
    return;
  im=thisim;
  /*  im = ee_image_get_image(image_display);*/
  a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "gray_area");
  if (!a)
    return;
  b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "gray_button");
  if (GTK_TOGGLE_BUTTON(b)->active)
    render_gray_mod(a, im, 255);
  else
    render_gray_mod(a, im, 63);
  a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "red_area");
  if (!a)
    return;
  b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "red_button");
  if (GTK_TOGGLE_BUTTON(b)->active)
    render_red_mod(a, im, 255);
  else
    render_red_mod(a, im, 63);
  a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "green_area");
  if (!a)
    return;
  b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "green_button");
  if (GTK_TOGGLE_BUTTON(b)->active)
    render_green_mod(a, im, 255);
  else
    render_green_mod(a, im, 63);
  a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "blue_area");
  if (!a)
    return;
  b = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(w), "blue_button");
  if (GTK_TOGGLE_BUTTON(b)->active)
    render_blue_mod(a, im, 255);
  else
    render_blue_mod(a, im, 63);
}

static void
ee_edit_cb_reset(GtkWidget *widget, gpointer data)
{
  GtkObject *adj;
  
  adj = gtk_object_get_data(GTK_OBJECT(widget), "adjustment");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
}

static void
ee_edit_cb_mod_gray_gamma_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  /*  gint w, h;*/
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_modifier(im, &modz);
      modz.gamma = v;
      gdk_imlib_set_image_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_gray_brightness_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;

  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }

  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_modifier(im, &modz);
      modz.brightness = v;
      gdk_imlib_set_image_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }

  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_gray_contrast_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_modifier(im, &modz);
      modz.contrast = v;
      gdk_imlib_set_image_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_red_gamma_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_red_modifier(im, &modz);
      modz.gamma = v;
      gdk_imlib_set_image_red_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_red_brightness_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_red_modifier(im, &modz);
      modz.brightness = v;
      gdk_imlib_set_image_red_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_red_contrast_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_red_modifier(im, &modz);
      modz.contrast = v;
      gdk_imlib_set_image_red_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_green_gamma_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_green_modifier(im, &modz);
      modz.gamma = v;
      gdk_imlib_set_image_green_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_green_brightness_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_green_modifier(im, &modz);
      modz.brightness = v;
      gdk_imlib_set_image_green_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_green_contrast_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_green_modifier(im, &modz);
      modz.contrast = v;
      gdk_imlib_set_image_green_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_blue_gamma_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_blue_modifier(im, &modz);
      modz.gamma = v;
      gdk_imlib_set_image_blue_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_blue_brightness_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;
  
  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_blue_modifier(im, &modz);
      modz.brightness = v;
      gdk_imlib_set_image_blue_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

static void
ee_edit_cb_mod_blue_contrast_changed(GtkObject *widget, gpointer data)
{
  gint v;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;
  GtkWidget *ww;
  gint apply;

  if (adj_ignore_count)
    {
      adj_ignore_count--;
      return;
    }
  v = GTK_ADJUSTMENT(widget)->value;
  im = thisim;
  ww = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(widget), "widget");
  if (im)
    {
      gdk_imlib_get_image_blue_modifier(im, &modz);
      modz.contrast = v;
      gdk_imlib_set_image_blue_modifier(im, &modz);
      apply = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(ww), "always"));
      if (apply)
	ee_edit_cb_col_apply(NULL, NULL);
      ee_edit_cb_col_preview(NULL, ww);
    }
  ee_edit_update_graphs(ww);
}

GtkWidget *
ee_edit_make_adjust(GtkWidget *w, GtkWidget *p, GtkSignalFunc change_func)
{
  GtkWidget *h, *b, *a;
  GtkObject *adj;
  
  h = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(h);
  
  adj = gtk_adjustment_new(256.0, 0.0, 1024.0, 1.0, 4.0, 0.0);
  gtk_object_set_data(GTK_OBJECT(h), "adjustment", adj);
  gtk_object_set_data(GTK_OBJECT(adj), "widget", w);
  
  a = gtk_hscale_new(GTK_ADJUSTMENT(adj));
  gtk_widget_show(a);
  gtk_range_set_update_policy(GTK_RANGE(a), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE(a), FALSE);
  gtk_box_pack_start(GTK_BOX(h), a, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed", 
		     GTK_SIGNAL_FUNC(change_func), a);
  b = gtk_button_new();
  gtk_widget_show(b);
  gtk_container_add(GTK_CONTAINER(b), p);
      gtk_widget_show(p);
  gtk_box_pack_start(GTK_BOX(h), b, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(b), "clicked", 
		     GTK_SIGNAL_FUNC(ee_edit_cb_reset), NULL);
  gtk_object_set_data(GTK_OBJECT(b), "adjustment", adj);
  gtk_object_set_data(GTK_OBJECT(b), "slider", a);  
  return h;
}

void
ee_edit_update_preview(GtkWidget *widget)
{
  if (widget)
    ee_edit_cb_col_preview(NULL, widget);
}

static void
ee_edit_cb_col_reset(GtkWidget *widget, gpointer data)
{
  GtkObject *adj;
  GdkImlibImage *im;
  GdkImlibColorModifier modz;

  adj_ignore_count = 12;
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_1");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_2");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_3");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_4");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_5");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_6");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_7");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_8");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_9");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_10");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_11");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));
  adj = gtk_object_get_data(GTK_OBJECT(data), "adj_12");
  GTK_ADJUSTMENT(adj)->value = (gfloat)256;
  ee_adjustment_value_changed(GTK_ADJUSTMENT(adj));

  im = thisim;
  if (im)
    {
      modz.gamma = 256.0;
      modz.brightness = 256.0;
      modz.contrast = 256.0;
      gdk_imlib_set_image_modifier(im, &modz);
      gdk_imlib_set_image_red_modifier(im, &modz);
      gdk_imlib_set_image_green_modifier(im, &modz);
      gdk_imlib_set_image_blue_modifier(im, &modz);
      ee_edit_update_preview(data);
      /*      ee_image_get_size(image_display, &w, &h);
	      ee_image_set_image_size(image_display, w, h);*/
    }
  ee_edit_update_graphs(GTK_WIDGET(data));
}

static void
ee_edit_cb_col_keep(GtkWidget *widget, gpointer data)
{
  GdkImlibImage *im;
  
  im = thisim;
  if (im)
    gdk_imlib_apply_modifiers_to_rgb(im);
  ee_edit_cb_col_reset(widget, data);
}

static void
ee_edit_cb_col_apply(GtkWidget *widget, gpointer data)
{
  gint w, h;
  GdkPixmap *pixmap;

  /*  ee_image_get_size(image_display, &w, &h);
      ee_image_set_image_size(image_display, w, h);*/
  w = imnode->imlibimage->rgb_width;
  h = imnode->imlibimage->rgb_height;
  gdk_imlib_render(imnode->imlibimage, w, h);
  pixmap = gdk_imlib_move_image(imnode->imlibimage);
  //  gtk_widget_hide(imnode->image);
  gtk_pixmap_set(GTK_PIXMAP(imnode->image), pixmap, NULL);
  gtk_widget_show(imnode->image);
  gdk_imlib_free_pixmap(pixmap);

  widget = NULL;
  data = NULL;
}

static void
ee_edit_cb_col_preview(GtkWidget *widget, gpointer data)
{
  GdkImlibImage *im;
  GtkWidget *a, *f;
  gint w, h;
  GdkPixmap *pmap, *mask;
  
  widget = NULL;
  im = thisim;
  if (im)
    {
      a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(data), "mini");
      f = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(data), "frame");
      w = im->rgb_width;
      h = im->rgb_height;
      if ((w / 4) > (h / 3))
	{
	  w = 160;
	  h = (160 *im->rgb_height) / im->rgb_width;
	}
      else
	{
	  h = 120;
	  w = (120 *im->rgb_width) / im->rgb_height;
	}
      gtk_widget_set_usize(a, w, h);
      gtk_widget_queue_resize(f);
      gdk_imlib_render(im, w, h);
      pmap = gdk_imlib_move_image(im);
      mask = gdk_imlib_move_mask(im);
      gdk_window_set_back_pixmap(a->window, pmap, FALSE);
      gdk_window_clear(a->window);
      gdk_window_shape_combine_mask(a->window, mask, 0, 0);
      gdk_imlib_free_pixmap(pmap);
    }
  else
    {
      a = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(data), "mini");
      gdk_window_set_back_pixmap(a->window, NULL, TRUE);
      gdk_window_clear(a->window);
      gdk_window_shape_combine_mask(a->window, NULL, 0, 0);
    }
}

void
ee_edit_set_crop_label(GtkWidget *widget, gchar *txt)
{
  GtkWidget *l;
  
  if (!widget)
    return;
  l = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "crop_label"));
  gtk_label_set(GTK_LABEL(l), txt);
}

void
ee_edit_set_size_label(GtkWidget *widget, gchar *txt)
{
  GtkWidget *l;
  
  if (!widget)
    return;
  l = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "size_label"));
  gtk_label_set(GTK_LABEL(l), txt);
}

GtkWidget *
img_edit_new(struct ImageMembers *ii)
{
  GtkWidget *w, *v, *h, *vv, *hh, *f, *a, *b, *aa, *r, *vvv, *p, *t, *l, *c;
  GtkWidget *vm;
  GdkImlibColor icl;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  GtkStyle *style;
  GtkAccelGroup*  cbag;

  thisim=ii->imlibimage;
  imnode=ii;

  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_wmclass(GTK_WINDOW(w), N_("Color Adjustment"), "gPhoto");
  gtk_window_set_title(GTK_WINDOW(w), N_("gPhoto - Color Adjustment"));
  gtk_container_border_width(GTK_CONTAINER(w), 2);
  gtk_window_set_policy(GTK_WINDOW(w), 0, 0, 1);
  gtk_signal_connect(GTK_OBJECT(w), "delete_event",
		     GTK_SIGNAL_FUNC(ee_edit_cb_destroy), w);
  gtk_widget_realize(w);

  cbag=gtk_accel_group_new();
  gtk_accel_group_attach(cbag,GTK_OBJECT(w));

  style = gtk_widget_get_style(w);
  vm = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(vm);
  gtk_container_add(GTK_CONTAINER(w), vm);

  f = gtk_frame_new(N_("Colour Settings"));
  gtk_widget_show(f);
  gtk_box_pack_start(GTK_BOX(vm), f, TRUE, TRUE, 0);
  
  v = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(v);
  gtk_container_add(GTK_CONTAINER(f), v);
  
  h = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(h);
  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 0);
  
  f = gtk_frame_new(NULL);
  gtk_widget_show(f);
  gtk_widget_set_usize(f, 168, -1);
  gtk_box_pack_start(GTK_BOX(h), f, FALSE, FALSE, 0);
  
  a = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_widget_show(a);
  gtk_container_add(GTK_CONTAINER(f), a);
  
  f = gtk_frame_new(NULL);
  gtk_widget_show(f);
  gtk_container_add(GTK_CONTAINER(a), f);
  
  gtk_object_set_data(GTK_OBJECT(w), "frame", f);
  
  a = gtk_drawing_area_new();
  gtk_widget_set_usize(a, 160, 120);
  gtk_widget_show(a);
  gtk_container_add(GTK_CONTAINER(f), a);
  gtk_widget_realize(a);
  gdk_window_set_back_pixmap(a->window, NULL, TRUE);
  
  gtk_object_set_data(GTK_OBJECT(w), "mini", a);
  
  vv = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(vv);
  gtk_box_pack_start(GTK_BOX(h), vv, FALSE, FALSE, 0);
  
  hh = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hh);
  gtk_box_pack_start(GTK_BOX(vv), hh, FALSE, FALSE, 0);

  b = gtk_toggle_button_new();
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(b), 1);
  gtk_object_set_data(GTK_OBJECT(w), "gray_button", b);
  gtk_object_set_data(GTK_OBJECT(b), "widget", w);
  gtk_widget_show(b);
  gtk_box_pack_start(GTK_BOX(hh), b, FALSE, FALSE, 0);
  aa = gtk_drawing_area_new();
  gtk_widget_set_usize(aa, 64, 64);
  gtk_widget_show(aa);
  gtk_container_add(GTK_CONTAINER(b), aa);
  gtk_object_set_data(GTK_OBJECT(w), "gray_area", aa);
  gtk_signal_connect(GTK_OBJECT(b), "clicked", 
		     GTK_SIGNAL_FUNC(ee_edit_cb_toggle), NULL);

  b = gtk_toggle_button_new();
  gtk_object_set_data(GTK_OBJECT(w), "red_button", b);
  gtk_object_set_data(GTK_OBJECT(b), "widget", w);
  gtk_widget_show(b);
  gtk_box_pack_start(GTK_BOX(hh), b, FALSE, FALSE, 0);
  aa = gtk_drawing_area_new();
  gtk_widget_set_usize(aa, 64, 64);
  gtk_widget_show(aa);
  gtk_container_add(GTK_CONTAINER(b), aa);
  gtk_object_set_data(GTK_OBJECT(w), "red_area", aa);
  gtk_signal_connect(GTK_OBJECT(b), "clicked", 
		     GTK_SIGNAL_FUNC(ee_edit_cb_toggle), NULL);
  
  b = gtk_toggle_button_new();
  gtk_object_set_data(GTK_OBJECT(w), "green_button", b);
  gtk_object_set_data(GTK_OBJECT(b), "widget", w);
  gtk_widget_show(b);
  gtk_box_pack_start(GTK_BOX(hh), b, FALSE, FALSE, 0);
  aa = gtk_drawing_area_new();
  gtk_widget_set_usize(aa, 64, 64);
  gtk_widget_show(aa);
  gtk_container_add(GTK_CONTAINER(b), aa);
  gtk_object_set_data(GTK_OBJECT(w), "green_area", aa);
  gtk_signal_connect(GTK_OBJECT(b), "clicked", 
		     GTK_SIGNAL_FUNC(ee_edit_cb_toggle), NULL);

  b = gtk_toggle_button_new();
  gtk_object_set_data(GTK_OBJECT(w), "blue_button", b);
  gtk_object_set_data(GTK_OBJECT(b), "widget", w);
  gtk_widget_show(b);
  gtk_box_pack_start(GTK_BOX(hh), b, FALSE, FALSE, 0);
  aa = gtk_drawing_area_new();
  gtk_widget_set_usize(aa, 64, 64);
  gtk_widget_show(aa);
  gtk_container_add(GTK_CONTAINER(b), aa);
  gtk_object_set_data(GTK_OBJECT(w), "blue_area", aa);
  gtk_signal_connect(GTK_OBJECT(b), "clicked", 
		     GTK_SIGNAL_FUNC(ee_edit_cb_toggle), NULL);
  ee_edit_update_graphs(w);

  icl.r = 255;
  icl.g = 0;
  icl.b = 255;

  vvv = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(vvv);
  gtk_object_set_data(GTK_OBJECT(w), "gray_controls", vvv);
  gtk_box_pack_start(GTK_BOX(vv), vvv, FALSE, FALSE, 0);
  l = gtk_label_new(N_("Gray Controls"));
  gtk_widget_show(l);

  gtk_box_pack_start(GTK_BOX(vvv), l, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					gamma_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_gray_gamma_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_1", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					brightness_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_gray_brightness_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_2", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					contrast_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);

  /*  p = gnome_pixmap_new_from_rgb_d_shaped(contrast_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_gray_contrast_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_3", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  
  vvv = gtk_vbox_new(FALSE, 2);
  gtk_object_set_data(GTK_OBJECT(w), "red_controls", vvv);
  gtk_box_pack_start(GTK_BOX(vv), vvv, FALSE, FALSE, 0);
  l = gtk_label_new(N_("Red Controls"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(vvv), l, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					gamma_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);

  /*  p = gnome_pixmap_new_from_rgb_d_shaped(gamma_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_red_gamma_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_4", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					brightness_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_red_brightness_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_5", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					contrast_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(contrast_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_red_contrast_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_6", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);

  vvv = gtk_vbox_new(FALSE, 2);
  gtk_object_set_data(GTK_OBJECT(w), "green_controls", vvv);
  gtk_box_pack_start(GTK_BOX(vv), vvv, FALSE, FALSE, 0);
  l = gtk_label_new(N_("Green Controls"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(vvv), l, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					gamma_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(gamma_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_green_gamma_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_7", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					brightness_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(brightness_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_green_brightness_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_8", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					contrast_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(contrast_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_green_contrast_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_9", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);

  vvv = gtk_vbox_new(FALSE, 2);
  gtk_object_set_data(GTK_OBJECT(w), "blue_controls", vvv);
  gtk_box_pack_start(GTK_BOX(vv), vvv, FALSE, FALSE, 0);
  l = gtk_label_new(N_("Blue Controls"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(vvv), l, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					gamma_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(gamma_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_blue_gamma_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_10", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					brightness_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(brightness_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_blue_brightness_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_11", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d(w->window,&bitmap,
					&style->bg[GTK_STATE_NORMAL],
					contrast_xpm);
  p = gtk_pixmap_new(pixmap,bitmap);
  /*  p = gnome_pixmap_new_from_rgb_d_shaped(contrast_icon, NULL, 12, 12, &icl);*/
  r = ee_edit_make_adjust(w, p, 
			  (GtkSignalFunc)ee_edit_cb_mod_blue_contrast_changed);
  gtk_object_set_data(GTK_OBJECT(w), "adj_12", (GtkObject *)
		      gtk_object_get_data(GTK_OBJECT(r), "adjustment"));
  gtk_widget_show(r);
  gtk_box_pack_start(GTK_BOX(vvv), r, FALSE, FALSE, 0);

  h = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(h);
  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 0);
  
  l = gtk_label_new(N_("Colour Modifications:"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(h), l, FALSE, FALSE, 0);
  gtk_object_set_data(GTK_OBJECT(w), "size_label", l);
  t = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_TEXT);
  gtk_widget_show(t);
  gtk_box_pack_start(GTK_BOX(h), t, FALSE, FALSE, 0);
  gtk_toolbar_append_item(GTK_TOOLBAR(t), N_("Apply"),
			  "Apply the current color changes to the main image",
			  "Apply the current color changes to the main image",
			  NULL, (GtkSignalFunc)ee_edit_cb_col_apply, w);
  gtk_toolbar_append_item(GTK_TOOLBAR(t), N_("Keep"),
			  "Apply the current color changes to the image data, and reset",
			  "Apply the current color changes to the image data, and reset",
			  NULL, (GtkSignalFunc)ee_edit_cb_col_keep, w);
  gtk_toolbar_append_item(GTK_TOOLBAR(t), N_("Reset"),
			  N_("Reset all the color changes to normal values"),
			  N_("Reset all the color changes to normal values"),
			  NULL, (GtkSignalFunc)ee_edit_cb_col_reset, w);
  c = gtk_check_button_new_with_label(N_("Always Apply"));
  gtk_widget_show(c);
  gtk_signal_connect(GTK_OBJECT(c), "clicked",
		     GTK_SIGNAL_FUNC(ee_edit_cb_always_toggle),
		     w);
  gtk_toolbar_append_widget(GTK_TOOLBAR(t), c,
			    "Always apply any changes to the main image",
			    "Always apply any changes to the main image");

  c = gtk_button_new_with_label(N_("Close"));
  gtk_signal_connect_object(GTK_OBJECT(c), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(w));
  gtk_toolbar_append_widget(GTK_TOOLBAR(t), c,
			  "Quit the color editor",
			  "Quit the color editor");
  gtk_accel_group_add(cbag,GDK_Escape,0,
		      GTK_ACCEL_VISIBLE||GTK_ACCEL_LOCKED,
		      GTK_OBJECT(c),"clicked");

  gtk_widget_show(c);

  /*
  f = gtk_frame_new(N_("Geometry Settings"));
  gtk_widget_show(f);
  gtk_box_pack_start(GTK_BOX(vm), f, TRUE, TRUE, 0);
  
  h = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(h);
  gtk_container_add(GTK_CONTAINER(f), h);
  
  v = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(v);
  gtk_box_pack_start(GTK_BOX(h), v, TRUE, TRUE, 0);
  
  l = gtk_label_new(N_("Image Size:"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(v), l, FALSE, FALSE, 0);
  gtk_object_set_data(GTK_OBJECT(w), "size_label", l);
  
  l = gtk_label_new(N_("Crop:"));
  gtk_widget_show(l);
  gtk_box_pack_start(GTK_BOX(v), l, FALSE, FALSE, 0);
  gtk_object_set_data(GTK_OBJECT(w), "crop_label", l);
  */  
  v = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(v);
  gtk_box_pack_start(GTK_BOX(h), v, TRUE, TRUE, 0);
  gtk_object_set_data(GTK_OBJECT(w), "always", GINT_TO_POINTER (0));
  ee_edit_update_preview(w);
  return w;
}

