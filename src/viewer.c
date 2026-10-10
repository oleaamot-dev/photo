/* viewer.c - picture viewer and simple editor
 *
 * gPhoto 0.4 opened each downloaded picture in its own notebook page
 * with rotate, flip and scale tools.  0.5 opens a window instead.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "viewer.h"
#include "gphoto.h"

#include <string.h>

typedef struct {
  GtkWidget *window;
  GtkWidget *image;
  GtkWidget *scrolled;
  GtkWidget *status;
  gchar     *name;
  GBytes    *original;
  GdkPixbuf *pixbuf;      /* edited picture at full size */
  gdouble    zoom;        /* 0 means fit to window */
  gboolean   edited;
} Viewer;

static void
viewer_free (Viewer *v)
{
  g_free (v->name);
  g_bytes_unref (v->original);
  g_clear_object (&v->pixbuf);
  g_free (v);
}

static void
viewer_redraw (Viewer *v)
{
  gint w = gdk_pixbuf_get_width (v->pixbuf);
  gint h = gdk_pixbuf_get_height (v->pixbuf);
  gdouble zoom = v->zoom;
  g_autoptr(GdkPixbuf) scaled = NULL;
  g_autofree gchar *msg = NULL;

  if (zoom <= 0)
    {
      GtkAllocation a;
      gtk_widget_get_allocation (v->scrolled, &a);
      if (a.width < 32 || a.height < 32)
        { a.width = 800; a.height = 600; }
      zoom = MIN ((a.width - 8) / (gdouble) w, (a.height - 8) / (gdouble) h);
      zoom = MIN (zoom, 1.0);
    }

  if (ABS (zoom - 1.0) < 0.001)
    scaled = g_object_ref (v->pixbuf);
  else
    scaled = gdk_pixbuf_scale_simple (v->pixbuf, MAX (1, (gint) (w * zoom)),
                                      MAX (1, (gint) (h * zoom)),
                                      GDK_INTERP_BILINEAR);

  gtk_image_set_from_pixbuf (GTK_IMAGE (v->image), scaled);

  msg = g_strdup_printf ("%d × %d pixels  ·  %d%%%s%s", w, h,
                         (int) (zoom * 100 + 0.5),
                         v->zoom <= 0 ? " (fit)" : "",
                         v->edited ? "  ·  edited" : "");
  gtk_label_set_text (GTK_LABEL (v->status), msg);
}

static void
replace_pixbuf (Viewer *v, GdkPixbuf *pb)
{
  if (pb == NULL)
    return;
  g_object_unref (v->pixbuf);
  v->pixbuf = pb;
  v->edited = TRUE;
  viewer_redraw (v);
}

static void rotate_left  (GtkWidget *w, Viewer *v) { replace_pixbuf (v, gdk_pixbuf_rotate_simple (v->pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE)); }
static void rotate_right (GtkWidget *w, Viewer *v) { replace_pixbuf (v, gdk_pixbuf_rotate_simple (v->pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE)); }
static void flip_h       (GtkWidget *w, Viewer *v) { replace_pixbuf (v, gdk_pixbuf_flip (v->pixbuf, TRUE)); }
static void flip_v       (GtkWidget *w, Viewer *v) { replace_pixbuf (v, gdk_pixbuf_flip (v->pixbuf, FALSE)); }

static void
zoom_by (Viewer *v, gdouble factor)
{
  if (v->zoom <= 0)
    {
      GtkAllocation a;
      gint w = gdk_pixbuf_get_width (v->pixbuf);
      gint h = gdk_pixbuf_get_height (v->pixbuf);
      gtk_widget_get_allocation (v->scrolled, &a);
      v->zoom = MIN (1.0, MIN ((a.width - 8) / (gdouble) w,
                               (a.height - 8) / (gdouble) h));
    }
  v->zoom = CLAMP (v->zoom * factor, 0.05, 8.0);
  viewer_redraw (v);
}

static void zoom_in  (GtkWidget *w, Viewer *v) { zoom_by (v, 1.25); }
static void zoom_out (GtkWidget *w, Viewer *v) { zoom_by (v, 0.8); }
static void zoom_fit (GtkWidget *w, Viewer *v) { v->zoom = 0; viewer_redraw (v); }
static void zoom_100 (GtkWidget *w, Viewer *v) { v->zoom = 1.0; viewer_redraw (v); }

static void
scale_picture (GtkWidget *w, Viewer *v)
{
  GtkWidget *dialog, *grid, *spin, *label;
  gint pw = gdk_pixbuf_get_width (v->pixbuf);
  gint ph = gdk_pixbuf_get_height (v->pixbuf);

  dialog = gtk_dialog_new_with_buttons ("Scale Picture", GTK_WINDOW (v->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Scale", GTK_RESPONSE_ACCEPT, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  label = gtk_label_new ("Scale (percent):");
  spin = gtk_spin_button_new_with_range (1, 400, 5);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 50);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), spin, 1, 0, 1, 1);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), grid);
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gdouble pct = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin)) / 100.0;
      replace_pixbuf (v, gdk_pixbuf_scale_simple (v->pixbuf,
                                                  MAX (1, (gint) (pw * pct)),
                                                  MAX (1, (gint) (ph * pct)),
                                                  GDK_INTERP_HYPER));
    }
  gtk_widget_destroy (dialog);
}

static const gchar *
format_for (const gchar *path)
{
  g_autofree gchar *lower = g_ascii_strdown (path, -1);
  if (g_str_has_suffix (lower, ".png"))  return "png";
  if (g_str_has_suffix (lower, ".bmp"))  return "bmp";
  if (g_str_has_suffix (lower, ".tif") || g_str_has_suffix (lower, ".tiff")) return "tiff";
  if (g_str_has_suffix (lower, ".jpg") || g_str_has_suffix (lower, ".jpeg")) return "jpeg";
  return NULL;
}

static void
save_as (GtkWidget *w, Viewer *v)
{
  GtkWidget *chooser;
  GError *error = NULL;

  chooser = gtk_file_chooser_dialog_new ("Save Picture", GTK_WINDOW (v->window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Save", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser), TRUE);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), v->name);

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      g_autofree gchar *path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      const gchar *fmt = format_for (path);
      gboolean ok;

      if (!v->edited)
        {
          /* Unchanged: write the camera's bytes, keeping EXIF intact. */
          gsize len;
          const gchar *data = g_bytes_get_data (v->original, &len);
          ok = g_file_set_contents (path, data, len, &error);
        }
      else if (fmt != NULL && strcmp (fmt, "jpeg") == 0)
        ok = gdk_pixbuf_save (v->pixbuf, path, fmt, &error, "quality", "92", NULL);
      else
        ok = gdk_pixbuf_save (v->pixbuf, path, fmt ? fmt : "png", &error, NULL);

      if (!ok)
        {
          GtkWidget *msg = gtk_message_dialog_new (GTK_WINDOW (chooser),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Could not save picture");
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msg),
                                                    "%s", error->message);
          gtk_dialog_run (GTK_DIALOG (msg));
          gtk_widget_destroy (msg);
          g_clear_error (&error);
        }
    }
  gtk_widget_destroy (chooser);
}

static void
on_size_allocate (GtkWidget *w, GdkRectangle *a, Viewer *v)
{
  static gint last_w, last_h;
  if (v->zoom <= 0 && (a->width != last_w || a->height != last_h))
    {
      last_w = a->width;
      last_h = a->height;
      viewer_redraw (v);
    }
}

static GtkToolItem *
tool (GtkWidget *bar, const gchar *icon, const gchar *tip,
      GCallback cb, Viewer *v)
{
  GtkToolItem *item = gtk_tool_button_new (NULL, tip);
  if (icon != NULL)
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), icon);
  else
    gtk_tool_item_set_is_important (item, TRUE);   /* show the label */
  gtk_tool_item_set_tooltip_text (item, tip);
  g_signal_connect (item, "clicked", cb, v);
  gtk_toolbar_insert (GTK_TOOLBAR (bar), item, -1);
  return item;
}

static void
separator (GtkWidget *bar)
{
  gtk_toolbar_insert (GTK_TOOLBAR (bar), gtk_separator_tool_item_new (), -1);
}

GtkWidget *
gphoto_viewer_new (GtkWindow   *parent,
                   const gchar *name,
                   GBytes      *data,
                   GError     **error)
{
  g_autoptr(GdkPixbufLoader) loader = gdk_pixbuf_loader_new ();
  GdkPixbuf *pb;
  Viewer *v;
  GtkWidget *box, *bar, *viewport;
  gsize len;
  const guchar *bytes = g_bytes_get_data (data, &len);

  if (!gdk_pixbuf_loader_write (loader, bytes, len, error) ||
      !gdk_pixbuf_loader_close (loader, error))
    return NULL;

  pb = gdk_pixbuf_loader_get_pixbuf (loader);
  if (pb == NULL)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_IO,
                   "%s is not a picture gPhoto can display", name);
      return NULL;
    }

  v = g_new0 (Viewer, 1);
  v->name = g_strdup (name);
  v->original = g_bytes_ref (data);
  /* Apply the EXIF orientation so portrait shots stand up. */
  v->pixbuf = gdk_pixbuf_apply_embedded_orientation (pb);

  v->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (v->window), name);
  gtk_window_set_transient_for (GTK_WINDOW (v->window), parent);
  gtk_window_set_default_size (GTK_WINDOW (v->window), 860, 680);
  g_object_set_data_full (G_OBJECT (v->window), "viewer", v,
                          (GDestroyNotify) viewer_free);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (v->window), box);

  bar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (bar), GTK_TOOLBAR_BOTH_HORIZ);
  tool (bar, "document-save-as-symbolic", "Save As…", G_CALLBACK (save_as), v);
  separator (bar);
  tool (bar, "object-rotate-left-symbolic", "Rotate Left", G_CALLBACK (rotate_left), v);
  tool (bar, "object-rotate-right-symbolic", "Rotate Right", G_CALLBACK (rotate_right), v);
  tool (bar, "object-flip-horizontal-symbolic", "Flip Horizontally", G_CALLBACK (flip_h), v);
  tool (bar, "object-flip-vertical-symbolic", "Flip Vertically", G_CALLBACK (flip_v), v);
  tool (bar, NULL, "Scale…", G_CALLBACK (scale_picture), v);
  separator (bar);
  tool (bar, "zoom-in-symbolic", "Zoom In", G_CALLBACK (zoom_in), v);
  tool (bar, "zoom-out-symbolic", "Zoom Out", G_CALLBACK (zoom_out), v);
  tool (bar, "zoom-original-symbolic", "Actual Size", G_CALLBACK (zoom_100), v);
  tool (bar, "zoom-fit-best-symbolic", "Fit to Window", G_CALLBACK (zoom_fit), v);
  gtk_box_pack_start (GTK_BOX (box), bar, FALSE, FALSE, 0);

  v->scrolled = gtk_scrolled_window_new (NULL, NULL);
  viewport = gtk_viewport_new (NULL, NULL);
  v->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (viewport), v->image);
  gtk_container_add (GTK_CONTAINER (v->scrolled), viewport);
  gtk_box_pack_start (GTK_BOX (box), v->scrolled, TRUE, TRUE, 0);
  g_signal_connect (v->scrolled, "size-allocate", G_CALLBACK (on_size_allocate), v);

  v->status = gtk_label_new (NULL);
  gtk_widget_set_halign (v->status, GTK_ALIGN_START);
  gtk_widget_set_margin_start (v->status, 8);
  gtk_widget_set_margin_top (v->status, 4);
  gtk_widget_set_margin_bottom (v->status, 4);
  gtk_box_pack_start (GTK_BOX (box), v->status, FALSE, FALSE, 0);

  viewer_redraw (v);
  gtk_widget_show_all (v->window);
  return v->window;
}
