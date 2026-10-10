/* gui.c - gPhoto main window
 *
 * The 0.4 layout, kept: a menu bar, a toolbar, the picture index as a
 * grid of numbered thumbnails, and a status bar with progress.  New in
 * 0.5: cameras come from libgphoto2, thumbnails load in the background,
 * and settings are read from the camera rather than hard-coded.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "gui.h"
#include "camera.h"
#include "config-dialog.h"
#include "html.h"
#include "viewer.h"

#include <gtk/gtk.h>
#include <string.h>

#define THUMB_SIZE 120

enum {
  COL_PIXBUF,
  COL_LABEL,
  COL_INDEX,
  COL_TOOLTIP,
  N_COLS
};

typedef struct {
  GtkApplication *app;
  GtkWidget      *window;
  GtkWidget      *iconview;
  GtkWidget      *stack;
  GtkWidget      *status;
  GtkWidget      *progress;
  GtkWidget      *camera_label;
  GtkListStore   *store;
  GdkPixbuf      *placeholder;

  GPhotoCamera   *camera;
  GPtrArray      *pictures;

  guint           loader_id;
  guint           loader_pos;
  gboolean        busy;
  gchar          *last_dir;

  GPhotoOptions  *options;
} Gui;

static void update_sensitivity (Gui *gui);
static void get_index (Gui *gui);

/* ------------------------------------------------------------------ */
/* Messages and progress                                              */
/* ------------------------------------------------------------------ */

static void
status (Gui *gui, const gchar *fmt, ...) G_GNUC_PRINTF (2, 3);

static void
status (Gui *gui, const gchar *fmt, ...)
{
  va_list ap;
  g_autofree gchar *msg = NULL;

  va_start (ap, fmt);
  msg = g_strdup_vprintf (fmt, ap);
  va_end (ap);
  gtk_label_set_text (GTK_LABEL (gui->status), msg);
}

static void
error_dialog (Gui *gui, const gchar *primary, GError *error)
{
  GtkWidget *msg = gtk_message_dialog_new (GTK_WINDOW (gui->window),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                           "%s", primary);
  if (error != NULL)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msg),
                                              "%s", error->message);
  gtk_dialog_run (GTK_DIALOG (msg));
  gtk_widget_destroy (msg);
  status (gui, "%s", primary);
}

static void
text_dialog (Gui *gui, const gchar *title, const gchar *text)
{
  GtkWidget *dialog, *scroll, *view;
  GtkTextBuffer *buffer;

  dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (gui->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Close", GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 560, 420);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 8);
  gtk_text_view_set_top_margin (GTK_TEXT_VIEW (view), 8);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_set_text (buffer, text, -1);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      scroll, TRUE, TRUE, 0);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
pump_events (void)
{
  while (gtk_events_pending ())
    gtk_main_iteration_do (FALSE);
}

static void
progress_cb (gdouble fraction, const gchar *message, gpointer data)
{
  Gui *gui = data;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress), fraction);
  if (message)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (gui->progress), message);
  pump_events ();
}

/* Wraps a synchronous camera operation: blocks re-entry, shows a
 * busy cursor and stops the background thumbnail loader. */
static gboolean
begin_busy (Gui *gui, const gchar *what)
{
  GdkWindow *win;

  if (gui->busy)
    return FALSE;
  gui->busy = TRUE;

  if (gui->loader_id)
    {
      g_source_remove (gui->loader_id);
      gui->loader_id = 0;
    }

  win = gtk_widget_get_window (gui->window);
  if (win)
    {
      g_autoptr(GdkCursor) cursor =
        gdk_cursor_new_from_name (gdk_window_get_display (win), "wait");
      gdk_window_set_cursor (win, cursor);
    }
  status (gui, "%s", what);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (gui->progress), NULL);
  gtk_widget_show (gui->progress);
  update_sensitivity (gui);
  pump_events ();
  return TRUE;
}

static void start_loader (Gui *gui);

static void
end_busy (Gui *gui)
{
  GdkWindow *win = gtk_widget_get_window (gui->window);
  if (win)
    gdk_window_set_cursor (win, NULL);
  gtk_widget_hide (gui->progress);
  gui->busy = FALSE;
  update_sensitivity (gui);
  /* Resume thumbnails that were interrupted. */
  if (gui->pictures && gui->loader_pos < gui->pictures->len)
    start_loader (gui);
}

/* ------------------------------------------------------------------ */
/* Thumbnails                                                         */
/* ------------------------------------------------------------------ */

static GdkPixbuf *
make_placeholder (Gui *gui)
{
  GtkIconTheme *theme = gtk_icon_theme_get_default ();
  GdkPixbuf *pb = gtk_icon_theme_load_icon (theme, "image-x-generic", 64, 0, NULL);
  GdkPixbuf *out;

  out = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, THUMB_SIZE, THUMB_SIZE * 3 / 4);
  gdk_pixbuf_fill (out, 0xdededeff);
  if (pb != NULL)
    {
      gint w = gdk_pixbuf_get_width (pb), h = gdk_pixbuf_get_height (pb);
      gdk_pixbuf_composite (pb, out,
                            (THUMB_SIZE - w) / 2, (THUMB_SIZE * 3 / 4 - h) / 2, w, h,
                            (THUMB_SIZE - w) / 2, (THUMB_SIZE * 3 / 4 - h) / 2,
                            1, 1, GDK_INTERP_BILINEAR, 160);
      g_object_unref (pb);
    }
  return out;
}

static GdkPixbuf *
pixbuf_from_bytes (GBytes *bytes, gint size)
{
  g_autoptr(GdkPixbufLoader) loader = gdk_pixbuf_loader_new ();
  GdkPixbuf *pb, *oriented, *scaled;
  gsize len;
  const guchar *data = g_bytes_get_data (bytes, &len);
  gint w, h;
  gdouble s;

  if (!gdk_pixbuf_loader_write (loader, data, len, NULL) ||
      !gdk_pixbuf_loader_close (loader, NULL))
    return NULL;
  pb = gdk_pixbuf_loader_get_pixbuf (loader);
  if (pb == NULL)
    return NULL;

  oriented = gdk_pixbuf_apply_embedded_orientation (pb);
  w = gdk_pixbuf_get_width (oriented);
  h = gdk_pixbuf_get_height (oriented);
  s = MIN ((gdouble) size / w, (gdouble) size / h);
  if (s >= 1.0)
    return oriented;
  scaled = gdk_pixbuf_scale_simple (oriented, MAX (1, (gint) (w * s)),
                                    MAX (1, (gint) (h * s)), GDK_INTERP_BILINEAR);
  g_object_unref (oriented);
  return scaled;
}

static GdkPixbuf *
load_thumbnail (Gui *gui, GPhotoPicture *pic)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkPixbuf *pb = NULL;

  /* The camera layer shrinks the picture itself when the camera
   * has no thumbnail to offer. */
  bytes = gphoto_camera_get_data (gui->camera, pic, TRUE, NULL);
  if (bytes != NULL)
    pb = pixbuf_from_bytes (bytes, THUMB_SIZE);
  return pb;
}

static gboolean
loader_step (gpointer data)
{
  Gui *gui = data;
  GPhotoPicture *pic;
  GtkTreeIter iter;
  GdkPixbuf *pb;

  if (gui->pictures == NULL || gui->loader_pos >= gui->pictures->len)
    {
      gui->loader_id = 0;
      if (gui->pictures)
        status (gui, "%u picture%s on %s", gui->pictures->len,
                gui->pictures->len == 1 ? "" : "s",
                gphoto_camera_get_model (gui->camera));
      return G_SOURCE_REMOVE;
    }

  pic = g_ptr_array_index (gui->pictures, gui->loader_pos);
  status (gui, "Loading thumbnail %u of %u…", gui->loader_pos + 1,
          gui->pictures->len);

  pb = load_thumbnail (gui, pic);
  if (pb != NULL &&
      gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (gui->store), &iter, NULL,
                                     gui->loader_pos))
    gtk_list_store_set (gui->store, &iter, COL_PIXBUF, pb, -1);
  g_clear_object (&pb);

  gui->loader_pos++;
  return G_SOURCE_CONTINUE;
}

static void
start_loader (Gui *gui)
{
  if (gui->loader_id == 0)
    gui->loader_id = g_idle_add_full (G_PRIORITY_LOW, loader_step, gui, NULL);
}

/* ------------------------------------------------------------------ */
/* Index                                                              */
/* ------------------------------------------------------------------ */

static void
clear_index (Gui *gui)
{
  if (gui->loader_id)
    {
      g_source_remove (gui->loader_id);
      gui->loader_id = 0;
    }
  gtk_list_store_clear (gui->store);
  g_clear_pointer (&gui->pictures, g_ptr_array_unref);
  gui->loader_pos = 0;
}

static void
show_page (Gui *gui)
{
  const gchar *page;
  if (!gphoto_camera_is_open (gui->camera))
    page = "nocamera";
  else if (gui->pictures && gui->pictures->len == 0)
    page = "empty";
  else
    page = "index";
  gtk_stack_set_visible_child_name (GTK_STACK (gui->stack), page);
}

static void
get_index (Gui *gui)
{
  GError *error = NULL;
  GPtrArray *pics;

  if (!gphoto_camera_is_open (gui->camera))
    return;
  if (!begin_busy (gui, "Reading the picture index…"))
    return;

  clear_index (gui);
  pics = gphoto_camera_list_pictures (gui->camera, &error);
  end_busy (gui);

  if (pics == NULL)
    {
      error_dialog (gui, "Could not read the picture index", error);
      g_clear_error (&error);
      show_page (gui);
      return;
    }

  gui->pictures = pics;
  for (guint i = 0; i < pics->len; i++)
    {
      GPhotoPicture *pic = g_ptr_array_index (pics, i);
      g_autofree gchar *label = g_strdup_printf ("%u. %s", pic->index, pic->name);
      g_autofree gchar *size = pic->size ? g_format_size (pic->size) : g_strdup ("unknown size");
      g_autofree gchar *path = g_build_filename (pic->folder, pic->name, NULL);
      g_autofree gchar *esc = g_markup_escape_text (path, -1);
      g_autofree gchar *tip = g_strdup_printf ("<b>%s</b>\n%s · %s", esc, size,
                                               pic->mime_type ? pic->mime_type : "");
      gtk_list_store_insert_with_values (gui->store, NULL, -1,
                                         COL_PIXBUF, gui->placeholder,
                                         COL_LABEL, label,
                                         COL_INDEX, pic->index,
                                         COL_TOOLTIP, tip,
                                         -1);
    }

  status (gui, "%u picture%s on %s", pics->len, pics->len == 1 ? "" : "s",
          gphoto_camera_get_model (gui->camera));
  show_page (gui);
  update_sensitivity (gui);
  start_loader (gui);
}

/* Selected picture indices (1-based), sorted. */
static GArray *
get_selection (Gui *gui)
{
  GList *paths = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (gui->iconview));
  GArray *out = g_array_new (FALSE, FALSE, sizeof (guint));

  for (GList *l = paths; l; l = l->next)
    {
      GtkTreeIter iter;
      guint idx;
      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->store), &iter, l->data))
        {
          gtk_tree_model_get (GTK_TREE_MODEL (gui->store), &iter, COL_INDEX, &idx, -1);
          g_array_append_val (out, idx);
        }
    }
  g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);

  for (guint i = 1; i < out->len; i++)
    for (guint j = i; j > 0 && g_array_index (out, guint, j - 1) > g_array_index (out, guint, j); j--)
      {
        guint t = g_array_index (out, guint, j);
        g_array_index (out, guint, j) = g_array_index (out, guint, j - 1);
        g_array_index (out, guint, j - 1) = t;
      }
  return out;
}

/* ------------------------------------------------------------------ */
/* Camera selection                                                   */
/* ------------------------------------------------------------------ */

static void
camera_opened (Gui *gui)
{
  g_autofree gchar *markup = g_markup_printf_escaped (
      "<b>%s</b>  <span alpha=\"60%%\">%s</span>",
      gphoto_camera_get_model (gui->camera), gphoto_camera_get_port (gui->camera));
  gtk_label_set_markup (GTK_LABEL (gui->camera_label), markup);
  gtk_window_set_title (GTK_WINDOW (gui->window), gphoto_camera_get_model (gui->camera));
  get_index (gui);
}

static gboolean
open_camera (Gui *gui, const gchar *model, const gchar *port, gboolean quiet)
{
  GError *error = NULL;
  gboolean ok;

  if (!begin_busy (gui, "Connecting to camera…"))
    return FALSE;
  clear_index (gui);
  ok = gphoto_camera_open (gui->camera, model, port, &error);
  end_busy (gui);

  if (!ok)
    {
      gtk_label_set_text (GTK_LABEL (gui->camera_label), "No camera");
      gtk_window_set_title (GTK_WINDOW (gui->window), GPHOTO_NAME);
      if (!quiet)
        error_dialog (gui, "Could not open the camera", error);
      else
        status (gui, "%s", error->message);
      g_clear_error (&error);
      show_page (gui);
      update_sensitivity (gui);
      return FALSE;
    }

  camera_opened (gui);
  return TRUE;
}

static void
on_detected_changed (GtkComboBox *combo, gpointer data)
{
  GtkWidget **w = data;  /* { model_entry, port_entry } */
  GtkTreeIter iter;
  g_autofree gchar *model = NULL, *port = NULL;

  if (!gtk_combo_box_get_active_iter (combo, &iter))
    return;
  gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter, 0, &model, 1, &port, -1);
  if (model == NULL)
    return;
  gtk_entry_set_text (GTK_ENTRY (w[0]), model);
  gtk_entry_set_text (GTK_ENTRY (w[1]), port);
}

static void
action_open_camera (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  GtkWidget *dialog, *grid, *detected, *model_entry, *port_entry, *hint;
  GtkListStore *det_store, *model_store, *port_store;
  GtkEntryCompletion *completion;
  GtkCellRenderer *cell;
  GtkWidget *link[2];
  g_autoptr(GPtrArray) found = NULL, models = NULL, ports = NULL;
  int row = 0;

  if (gui->busy)
    return;

  found = gphoto_camera_autodetect (NULL);
  models = gphoto_camera_list_models (NULL);
  ports = gphoto_camera_list_ports (NULL);

  dialog = gtk_dialog_new_with_buttons ("Select Camera", GTK_WINDOW (gui->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 8);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 16);

  /* Detected cameras */
  det_store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  for (guint i = 0; found && i < found->len; i++)
    {
      GPhotoDetected *d = g_ptr_array_index (found, i);
      g_autofree gchar *text = g_strdup_printf ("%s  (%s)", d->model, d->port);
      gtk_list_store_insert_with_values (det_store, NULL, -1, 0, d->model,
                                         1, d->port, 2, text, -1);
    }
  detected = gtk_combo_box_new_with_model (GTK_TREE_MODEL (det_store));
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (detected), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (detected), cell, "text", 2);
  gtk_widget_set_sensitive (detected, found && found->len > 0);
  g_object_unref (det_store);

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Detected:"), 0, row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), detected, 1, row++, 1, 1);

  /* Model */
  model_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (model_entry), "Camera model");
  gtk_entry_set_width_chars (GTK_ENTRY (model_entry), 40);
  model_store = gtk_list_store_new (1, G_TYPE_STRING);
  for (guint i = 0; models && i < models->len; i++)
    gtk_list_store_insert_with_values (model_store, NULL, -1, 0,
                                       g_ptr_array_index (models, i), -1);
  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (model_store));
  gtk_entry_completion_set_text_column (completion, 0);
  gtk_entry_completion_set_minimum_key_length (completion, 2);
  gtk_entry_set_completion (GTK_ENTRY (model_entry), completion);
  g_object_unref (completion);
  g_object_unref (model_store);
  if (gphoto_camera_get_model (gui->camera))
    gtk_entry_set_text (GTK_ENTRY (model_entry), gphoto_camera_get_model (gui->camera));

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Model:"), 0, row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), model_entry, 1, row++, 1, 1);

  /* Port */
  port_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (port_entry), "usb:, serial:/dev/ttyS0, disk:/path…");
  port_store = gtk_list_store_new (1, G_TYPE_STRING);
  for (guint i = 0; ports && i < ports->len; i++)
    gtk_list_store_insert_with_values (port_store, NULL, -1, 0,
                                       g_ptr_array_index (ports, i), -1);
  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (port_store));
  gtk_entry_completion_set_text_column (completion, 0);
  gtk_entry_completion_set_minimum_key_length (completion, 0);
  gtk_entry_set_completion (GTK_ENTRY (port_entry), completion);
  g_object_unref (completion);
  g_object_unref (port_store);
  if (gphoto_camera_get_port (gui->camera))
    gtk_entry_set_text (GTK_ENTRY (port_entry), gphoto_camera_get_port (gui->camera));
  gtk_entry_set_activates_default (GTK_ENTRY (port_entry), TRUE);

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Port:"), 0, row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), port_entry, 1, row++, 1, 1);

  hint = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (hint),
    "<small>Leave both empty to use the first detected camera.\n"
    "To browse a folder as a camera, use model “Directory Browse”\n"
    "and port “disk:/path/to/folder”.</small>");
  gtk_label_set_xalign (GTK_LABEL (hint), 0);
  gtk_widget_set_sensitive (hint, FALSE);
  gtk_grid_attach (GTK_GRID (grid), hint, 1, row++, 1, 1);

  for (GList *l = gtk_container_get_children (GTK_CONTAINER (grid)), *h = l; l; l = l->next)
    {
      if (GTK_IS_LABEL (l->data) && l->data != hint)
        gtk_label_set_xalign (GTK_LABEL (l->data), 1.0);
      if (l->next == NULL)
        g_list_free (h);
    }

  link[0] = model_entry;
  link[1] = port_entry;
  g_signal_connect (detected, "changed", G_CALLBACK (on_detected_changed), link);
  if (found && found->len > 0 && gphoto_camera_get_model (gui->camera) == NULL)
    gtk_combo_box_set_active (GTK_COMBO_BOX (detected), 0);

  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), grid);
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      g_autofree gchar *model = g_strstrip (g_strdup (gtk_entry_get_text (GTK_ENTRY (model_entry))));
      g_autofree gchar *port = g_strstrip (g_strdup (gtk_entry_get_text (GTK_ENTRY (port_entry))));
      gtk_widget_destroy (dialog);
      open_camera (gui, *model ? model : NULL, *port ? port : NULL, FALSE);
      return;
    }
  gtk_widget_destroy (dialog);
}

/* ------------------------------------------------------------------ */
/* Actions                                                            */
/* ------------------------------------------------------------------ */

static void
action_get_index (GSimpleAction *a, GVariant *p, gpointer data)
{
  get_index (data);
}

static gchar *
choose_folder (Gui *gui, const gchar *title, const gchar *accept)
{
  GtkWidget *chooser;
  gchar *dir = NULL;

  chooser = gtk_file_chooser_dialog_new (title, GTK_WINDOW (gui->window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         accept, GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (chooser), TRUE);
  if (gui->last_dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), gui->last_dir);
  else
    {
      const gchar *pics = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                           pics ? pics : g_get_home_dir ());
    }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      g_free (gui->last_dir);
      gui->last_dir = g_strdup (dir);
    }
  gtk_widget_destroy (chooser);
  return dir;
}

static void
save_pictures (Gui *gui, gboolean thumbnails)
{
  g_autoptr(GArray) sel = get_selection (gui);
  g_autofree gchar *dir = NULL;
  GError *error = NULL;
  guint saved = 0;

  if (sel->len == 0)
    {
      /* Nothing selected: offer the whole index, as 0.4 did. */
      for (guint i = 1; gui->pictures && i <= gui->pictures->len; i++)
        g_array_append_val (sel, i);
    }
  if (sel->len == 0)
    return;

  dir = choose_folder (gui, thumbnails ? "Save Thumbnails To" : "Save Pictures To", "_Save");
  if (dir == NULL || !begin_busy (gui, "Saving…"))
    return;

  for (guint i = 0; i < sel->len; i++)
    {
      GPhotoPicture *pic = gphoto_pictures_lookup (gui->pictures, g_array_index (sel, guint, i));
      g_autofree gchar *path = NULL;

      status (gui, "Saving %s (%u of %u)…", pic->name, i + 1, sel->len);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress),
                                     (gdouble) i / sel->len);
      pump_events ();
      path = gphoto_camera_save (gui->camera, pic, thumbnails, dir, &error);
      if (path == NULL)
        break;
      saved++;
    }
  end_busy (gui);

  if (error != NULL)
    {
      error_dialog (gui, "Could not save all pictures", error);
      g_clear_error (&error);
    }
  status (gui, "Saved %u %s%s to %s", saved, thumbnails ? "thumbnail" : "picture",
          saved == 1 ? "" : "s", dir);
}

static void action_save (GSimpleAction *a, GVariant *p, gpointer d)        { save_pictures (d, FALSE); }
static void action_save_thumbs (GSimpleAction *a, GVariant *p, gpointer d) { save_pictures (d, TRUE); }

static void
action_delete (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  g_autoptr(GArray) sel = get_selection (gui);
  GtkWidget *msg;
  GError *error = NULL;
  gint response;
  guint deleted = 0;

  if (sel->len == 0)
    return;

  msg = gtk_message_dialog_new (GTK_WINDOW (gui->window),
                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                "Delete %u picture%s from the camera?",
                                sel->len, sel->len == 1 ? "" : "s");
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msg),
                                            "This cannot be undone.");
  gtk_dialog_add_buttons (GTK_DIALOG (msg), "_Cancel", GTK_RESPONSE_CANCEL,
                          "_Delete", GTK_RESPONSE_ACCEPT, NULL);
  gtk_style_context_add_class (gtk_widget_get_style_context (
      gtk_dialog_get_widget_for_response (GTK_DIALOG (msg), GTK_RESPONSE_ACCEPT)),
      "destructive-action");
  response = gtk_dialog_run (GTK_DIALOG (msg));
  gtk_widget_destroy (msg);
  if (response != GTK_RESPONSE_ACCEPT || !begin_busy (gui, "Deleting…"))
    return;

  for (guint i = 0; i < sel->len; i++)
    {
      GPhotoPicture *pic = gphoto_pictures_lookup (gui->pictures, g_array_index (sel, guint, i));
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->progress), (gdouble) i / sel->len);
      pump_events ();
      if (!gphoto_camera_delete (gui->camera, pic, &error))
        break;
      deleted++;
    }
  end_busy (gui);

  if (error != NULL)
    {
      error_dialog (gui, "Could not delete all pictures", error);
      g_clear_error (&error);
    }
  get_index (gui);
  status (gui, "Deleted %u picture%s", deleted, deleted == 1 ? "" : "s");
}

static void
action_capture (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  GError *error = NULL;
  GPhotoPicture *pic;

  if (!begin_busy (gui, "Taking picture…"))
    return;
  pic = gphoto_camera_capture (gui->camera, &error);
  end_busy (gui);

  if (pic == NULL)
    {
      error_dialog (gui, "Could not take a picture", error);
      g_clear_error (&error);
      return;
    }
  get_index (gui);
  /* Select the new picture in the refreshed index. */
  for (guint i = 0; gui->pictures && i < gui->pictures->len; i++)
    {
      GPhotoPicture *q = g_ptr_array_index (gui->pictures, i);
      if (g_strcmp0 (q->folder, pic->folder) == 0 && g_strcmp0 (q->name, pic->name) == 0)
        {
          GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
          gtk_icon_view_unselect_all (GTK_ICON_VIEW (gui->iconview));
          gtk_icon_view_select_path (GTK_ICON_VIEW (gui->iconview), path);
          gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (gui->iconview), path, FALSE, 0, 0);
          gtk_tree_path_free (path);
          break;
        }
    }
  status (gui, "New picture %s", pic->name);
  gphoto_picture_free (pic);
}

static void
action_upload (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  GtkWidget *chooser;
  GSList *files = NULL;
  GError *error = NULL;
  guint done = 0;

  chooser = gtk_file_chooser_dialog_new ("Upload to Camera", GTK_WINDOW (gui->window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Upload", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), TRUE);
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (chooser));
  gtk_widget_destroy (chooser);

  if (files == NULL || !begin_busy (gui, "Uploading…"))
    {
      g_slist_free_full (files, g_free);
      return;
    }

  for (GSList *l = files; l; l = l->next)
    {
      if (!gphoto_camera_upload (gui->camera, l->data, "/", &error))
        break;
      done++;
    }
  g_slist_free_full (files, g_free);
  end_busy (gui);

  if (error)
    {
      error_dialog (gui, "Could not upload all files", error);
      g_clear_error (&error);
    }
  get_index (gui);
  status (gui, "Uploaded %u file%s", done, done == 1 ? "" : "s");
}

static void
open_viewer (Gui *gui, guint index)
{
  GPhotoPicture *pic = gphoto_pictures_lookup (gui->pictures, index);
  g_autoptr(GBytes) bytes = NULL;
  GError *error = NULL;

  if (pic == NULL || !begin_busy (gui, "Downloading picture…"))
    return;
  bytes = gphoto_camera_get_data (gui->camera, pic, FALSE, &error);
  end_busy (gui);

  if (bytes == NULL || gphoto_viewer_new (GTK_WINDOW (gui->window), pic->name,
                                          bytes, &error) == NULL)
    {
      error_dialog (gui, "Could not open the picture", error);
      g_clear_error (&error);
      return;
    }
  status (gui, "Opened %s", pic->name);
}

static void
action_open_picture (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  g_autoptr(GArray) sel = get_selection (gui);
  for (guint i = 0; i < sel->len && i < 8; i++)
    open_viewer (gui, g_array_index (sel, guint, i));
}

static void
on_item_activated (GtkIconView *view, GtkTreePath *path, Gui *gui)
{
  GtkTreeIter iter;
  guint idx;
  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (gui->store), &iter, path))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (gui->store), &iter, COL_INDEX, &idx, -1);
      open_viewer (gui, idx);
    }
}

static void
action_select_all (GSimpleAction *a, GVariant *p, gpointer data)
{
  gtk_icon_view_select_all (GTK_ICON_VIEW (((Gui *) data)->iconview));
}

static void
action_select_none (GSimpleAction *a, GVariant *p, gpointer data)
{
  gtk_icon_view_unselect_all (GTK_ICON_VIEW (((Gui *) data)->iconview));
}

static void
action_html (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  g_autoptr(GArray) sel = get_selection (gui);
  g_autofree gchar *dir = NULL, *index = NULL;
  GError *error = NULL;

  dir = choose_folder (gui, "Publish HTML Gallery In", "_Publish");
  if (dir == NULL || !begin_busy (gui, "Publishing gallery…"))
    return;

  index = gphoto_html_export (gui->camera, gui->pictures, sel->len ? sel : NULL,
                              dir, gphoto_camera_get_model (gui->camera), &error);
  end_busy (gui);

  if (index == NULL)
    {
      error_dialog (gui, "Could not publish the gallery", error);
      g_clear_error (&error);
      return;
    }
  status (gui, "Gallery written to %s", index);
  {
    g_autofree gchar *uri = g_filename_to_uri (index, NULL, NULL);
    if (uri)
      gtk_show_uri_on_window (GTK_WINDOW (gui->window), uri, GDK_CURRENT_TIME, NULL);
  }
}

static void
action_configure (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  GError *error = NULL;

  if (gui->busy)
    return;
  gui->busy = TRUE;
  if (gui->loader_id)
    {
      g_source_remove (gui->loader_id);
      gui->loader_id = 0;
    }
  if (!gphoto_config_dialog_run (GTK_WINDOW (gui->window), gui->camera, &error))
    {
      gui->busy = FALSE;
      error_dialog (gui, "Camera configuration", error);
      g_clear_error (&error);
    }
  gui->busy = FALSE;
  end_busy (gui);
}

typedef gchar *(*TextGetter) (GPhotoCamera *, GError **);

static void
show_camera_text (Gui *gui, TextGetter get, const gchar *title)
{
  GError *error = NULL;
  g_autofree gchar *text = NULL;

  if (!begin_busy (gui, title))
    return;
  text = get (gui->camera, &error);
  end_busy (gui);
  if (text == NULL)
    {
      error_dialog (gui, title, error);
      g_clear_error (&error);
      return;
    }
  text_dialog (gui, title, text);
}

static void action_summary (GSimpleAction *a, GVariant *p, gpointer d) { show_camera_text (d, gphoto_camera_get_summary, "Camera Summary"); }
static void action_manual  (GSimpleAction *a, GVariant *p, gpointer d) { show_camera_text (d, gphoto_camera_get_manual,  "Driver Manual"); }
static void action_driver  (GSimpleAction *a, GVariant *p, gpointer d) { show_camera_text (d, gphoto_camera_get_about,   "About the Driver"); }

static void
action_close_camera (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  if (gui->busy)
    return;
  clear_index (gui);
  gphoto_camera_close (gui->camera);
  gtk_label_set_text (GTK_LABEL (gui->camera_label), "No camera");
  gtk_window_set_title (GTK_WINDOW (gui->window), GPHOTO_NAME);
  status (gui, "Camera closed");
  show_page (gui);
  update_sensitivity (gui);
}

static void
action_about (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  const gchar *authors[] = {
    "Ole Kristian Aamot <ole.aamot@aamotinnovation.com>",
    "Scott Fritzinger (gPhoto founder)",
    "and the gPhoto and libgphoto2 contributors",
    NULL
  };
  const char **libv = gp_library_version (GP_VERSION_SHORT);
  g_autofree gchar *comments = g_strdup_printf (
      "The GNU Digital Camera Program\n\nUsing libgphoto2 %s",
      libv && libv[0] ? libv[0] : "");

  gtk_show_about_dialog (GTK_WINDOW (gui->window),
                         "program-name", GPHOTO_NAME,
                         "version", PACKAGE_VERSION,
                         "comments", comments,
                         "website", PACKAGE_URL,
                         "website-label", "www.gphoto.org",
                         "copyright", "Copyright © 1998–2026 the gPhoto authors",
                         "license-type", GTK_LICENSE_GPL_2_0,
                         "authors", authors,
                         "logo-icon-name", GPHOTO_APP_ID,
                         NULL);
}

static void
action_quit (GSimpleAction *a, GVariant *p, gpointer data)
{
  Gui *gui = data;
  gtk_widget_destroy (gui->window);
}

static GActionEntry win_entries[] = {
  { .name = "open-camera",   .activate = action_open_camera },
  { .name = "close-camera",  .activate = action_close_camera },
  { .name = "get-index",     .activate = action_get_index },
  { .name = "open-picture",  .activate = action_open_picture },
  { .name = "save",          .activate = action_save },
  { .name = "save-thumbs",   .activate = action_save_thumbs },
  { .name = "delete",        .activate = action_delete },
  { .name = "capture",       .activate = action_capture },
  { .name = "upload",        .activate = action_upload },
  { .name = "html",          .activate = action_html },
  { .name = "configure",     .activate = action_configure },
  { .name = "summary",       .activate = action_summary },
  { .name = "manual",        .activate = action_manual },
  { .name = "driver",        .activate = action_driver },
  { .name = "select-all",    .activate = action_select_all },
  { .name = "select-none",   .activate = action_select_none },
  { .name = "about",         .activate = action_about },
  { .name = "quit",          .activate = action_quit },
};

static void
set_enabled (Gui *gui, const gchar *name, gboolean enabled)
{
  GAction *action = g_action_map_lookup_action (G_ACTION_MAP (gui->window), name);
  if (action)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
update_sensitivity (Gui *gui)
{
  gboolean open = gphoto_camera_is_open (gui->camera) && !gui->busy;
  gboolean have = open && gui->pictures && gui->pictures->len > 0;
  GList *sel = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (gui->iconview));
  gboolean some = have && sel != NULL;
  CameraAbilities ab;
  gboolean can_capture = FALSE, can_upload = FALSE, can_delete = TRUE, can_config = FALSE;

  g_list_free_full (sel, (GDestroyNotify) gtk_tree_path_free);

  if (open && gp_camera_get_abilities (gphoto_camera_get_gp_camera (gui->camera), &ab) >= GP_OK)
    {
      can_capture = (ab.operations & GP_OPERATION_CAPTURE_IMAGE) != 0;
      can_config = (ab.operations & GP_OPERATION_CONFIG) != 0;
      can_upload = (ab.folder_operations & GP_FOLDER_OPERATION_PUT_FILE) != 0;
      can_delete = (ab.file_operations & GP_FILE_OPERATION_DELETE) != 0;
    }

  set_enabled (gui, "open-camera", !gui->busy);
  set_enabled (gui, "close-camera", open);
  set_enabled (gui, "get-index", open);
  set_enabled (gui, "open-picture", some);
  set_enabled (gui, "save", have);
  set_enabled (gui, "save-thumbs", have);
  set_enabled (gui, "delete", some && can_delete);
  set_enabled (gui, "capture", open && can_capture);
  set_enabled (gui, "upload", open && can_upload);
  set_enabled (gui, "html", have);
  set_enabled (gui, "configure", open && can_config);
  set_enabled (gui, "summary", open);
  set_enabled (gui, "manual", open);
  set_enabled (gui, "driver", open);
  set_enabled (gui, "select-all", have);
  set_enabled (gui, "select-none", have);
}

static void
on_selection_changed (GtkIconView *view, Gui *gui)
{
  g_autoptr(GArray) sel = NULL;
  update_sensitivity (gui);
  if (gui->busy || gui->pictures == NULL)
    return;
  sel = get_selection (gui);
  if (sel->len == 1)
    {
      GPhotoPicture *pic = gphoto_pictures_lookup (gui->pictures, g_array_index (sel, guint, 0));
      g_autofree gchar *size = pic->size ? g_format_size (pic->size) : g_strdup ("");
      status (gui, "#%u  %s%s%s  %s", pic->index, pic->folder,
              g_str_has_suffix (pic->folder, "/") ? "" : "/", pic->name, size);
    }
  else if (sel->len > 1)
    status (gui, "%u of %u pictures selected", sel->len, gui->pictures->len);
}

/* ------------------------------------------------------------------ */
/* Window                                                             */
/* ------------------------------------------------------------------ */

static GMenuModel *
build_menu (void)
{
  GMenu *bar = g_menu_new ();
  GMenu *m, *s;

  m = g_menu_new ();
  s = g_menu_new ();
  g_menu_append (s, "_Select Camera…", "win.open-camera");
  g_menu_append (s, "_Close Camera", "win.close-camera");
  g_menu_append_section (m, NULL, G_MENU_MODEL (s)); g_object_unref (s);
  s = g_menu_new ();
  g_menu_append (s, "_Save Pictures…", "win.save");
  g_menu_append (s, "Save _Thumbnails…", "win.save-thumbs");
  g_menu_append (s, "_Upload to Camera…", "win.upload");
  g_menu_append (s, "Publish _HTML Gallery…", "win.html");
  g_menu_append_section (m, NULL, G_MENU_MODEL (s)); g_object_unref (s);
  s = g_menu_new ();
  g_menu_append (s, "_Quit", "win.quit");
  g_menu_append_section (m, NULL, G_MENU_MODEL (s)); g_object_unref (s);
  g_menu_append_submenu (bar, "_File", G_MENU_MODEL (m)); g_object_unref (m);

  m = g_menu_new ();
  g_menu_append (m, "Select _All", "win.select-all");
  g_menu_append (m, "Select _None", "win.select-none");
  g_menu_append_submenu (bar, "_Edit", G_MENU_MODEL (m)); g_object_unref (m);

  m = g_menu_new ();
  s = g_menu_new ();
  g_menu_append (s, "Get _Index", "win.get-index");
  g_menu_append (s, "_Open Picture", "win.open-picture");
  g_menu_append (s, "_Take Picture", "win.capture");
  g_menu_append (s, "_Delete Pictures…", "win.delete");
  g_menu_append_section (m, NULL, G_MENU_MODEL (s)); g_object_unref (s);
  s = g_menu_new ();
  g_menu_append (s, "_Configure…", "win.configure");
  g_menu_append (s, "Camera _Summary", "win.summary");
  g_menu_append (s, "Driver _Manual", "win.manual");
  g_menu_append (s, "About the _Driver", "win.driver");
  g_menu_append_section (m, NULL, G_MENU_MODEL (s)); g_object_unref (s);
  g_menu_append_submenu (bar, "_Camera", G_MENU_MODEL (m)); g_object_unref (m);

  m = g_menu_new ();
  g_menu_append (m, "_About gPhoto", "win.about");
  g_menu_append_submenu (bar, "_Help", G_MENU_MODEL (m)); g_object_unref (m);

  return G_MENU_MODEL (bar);
}

static void
add_tool (GtkWidget *bar, const gchar *icon, const gchar *label,
          const gchar *tip, const gchar *action)
{
  GtkToolItem *item = gtk_tool_button_new (NULL, label);
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), icon);
  gtk_tool_item_set_tooltip_text (item, tip);
  gtk_tool_item_set_is_important (item, TRUE);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), action);
  gtk_toolbar_insert (GTK_TOOLBAR (bar), item, -1);
}

static GtkWidget *
placeholder_page (const gchar *icon, const gchar *title, const gchar *body)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_DIALOG);
  GtkWidget *t = gtk_label_new (NULL);
  GtkWidget *b = gtk_label_new (body);
  g_autofree gchar *markup = g_markup_printf_escaped ("<span size=\"x-large\" weight=\"bold\">%s</span>", title);

  gtk_image_set_pixel_size (GTK_IMAGE (image), 96);
  gtk_widget_set_opacity (image, 0.5);
  gtk_label_set_markup (GTK_LABEL (t), markup);
  gtk_label_set_justify (GTK_LABEL (b), GTK_JUSTIFY_CENTER);
  gtk_widget_set_opacity (b, 0.7);
  gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), t, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), b, FALSE, FALSE, 0);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  return box;
}

static void
on_destroy (GtkWidget *w, Gui *gui)
{
  clear_index (gui);
  gphoto_camera_free (gui->camera);
  gui->camera = NULL;
  g_clear_object (&gui->placeholder);
  g_clear_object (&gui->store);
  g_free (gui->last_dir);
}

static gboolean
startup_open (gpointer data)
{
  Gui *gui = data;
  /* Explicit model/port: report errors.  Auto-detect: stay quiet. */
  gboolean explicit = gui->options->model || gui->options->port;
  open_camera (gui, gui->options->model, gui->options->port, !explicit);
  return G_SOURCE_REMOVE;
}

static void
on_activate (GtkApplication *app, Gui *gui)
{
  GtkWidget *box, *menubar, *toolbar, *scroll, *statusbar, *sep;
  g_autoptr(GMenuModel) menu = NULL;
  static const struct { const gchar *action; const gchar *accels[3]; } accels[] = {
    { "win.open-camera",  { "<Primary>o", NULL } },
    { "win.get-index",    { "<Primary>r", "F5", NULL } },
    { "win.save",         { "<Primary>s", NULL } },
    { "win.capture",      { "<Primary>t", NULL } },
    { "win.delete",       { "Delete", NULL } },
    { "win.configure",    { "<Primary>comma", NULL } },
    { "win.select-all",   { "<Primary>a", NULL } },
    { "win.select-none",  { "<Primary><Shift>a", NULL } },
    { "win.quit",         { "<Primary>q", NULL } },
  };

  if (gui->window != NULL)
    {
      gtk_window_present (GTK_WINDOW (gui->window));
      return;
    }

  gui->window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (gui->window), GPHOTO_NAME);
  gtk_window_set_default_size (GTK_WINDOW (gui->window), 900, 640);
  gtk_window_set_icon_name (GTK_WINDOW (gui->window), GPHOTO_APP_ID);
  g_action_map_add_action_entries (G_ACTION_MAP (gui->window), win_entries,
                                   G_N_ELEMENTS (win_entries), gui);
  for (guint i = 0; i < G_N_ELEMENTS (accels); i++)
    gtk_application_set_accels_for_action (app, accels[i].action, accels[i].accels);
  g_signal_connect (gui->window, "destroy", G_CALLBACK (on_destroy), gui);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (gui->window), box);

  menu = build_menu ();
  menubar = gtk_menu_bar_new_from_model (menu);
  gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, FALSE, 0);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
  gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                               GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
  add_tool (toolbar, "camera-photo-symbolic", "Camera", "Select camera", "win.open-camera");
  add_tool (toolbar, "view-refresh-symbolic", "Get Index", "Read the picture index", "win.get-index");
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
  add_tool (toolbar, "media-record-symbolic", "Take Picture", "Take a picture", "win.capture");
  add_tool (toolbar, "document-save-symbolic", "Save", "Save selected pictures", "win.save");
  add_tool (toolbar, "edit-delete-symbolic", "Delete", "Delete selected pictures", "win.delete");
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
  add_tool (toolbar, "preferences-system-symbolic", "Configure", "Camera settings", "win.configure");
  add_tool (toolbar, "web-browser-symbolic", "Gallery", "Publish an HTML gallery", "win.html");
  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);

  /* Index */
  gui->store = gtk_list_store_new (N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                   G_TYPE_UINT, G_TYPE_STRING);
  gui->placeholder = make_placeholder (gui);
  gui->iconview = gtk_icon_view_new_with_model (GTK_TREE_MODEL (gui->store));
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (gui->iconview), COL_PIXBUF);
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (gui->iconview), COL_LABEL);
  gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (gui->iconview), COL_TOOLTIP);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (gui->iconview), GTK_SELECTION_MULTIPLE);
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (gui->iconview), THUMB_SIZE + 16);
  gtk_icon_view_set_item_padding (GTK_ICON_VIEW (gui->iconview), 8);
  gtk_icon_view_set_spacing (GTK_ICON_VIEW (gui->iconview), 4);
  g_signal_connect (gui->iconview, "item-activated", G_CALLBACK (on_item_activated), gui);
  g_signal_connect (gui->iconview, "selection-changed", G_CALLBACK (on_selection_changed), gui);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scroll), gui->iconview);

  gui->stack = gtk_stack_new ();
  gtk_stack_add_named (GTK_STACK (gui->stack), scroll, "index");
  gtk_stack_add_named (GTK_STACK (gui->stack),
                       placeholder_page ("camera-photo", "No Camera",
                                         "Connect and switch on your camera,\n"
                                         "then choose Camera ▸ Select Camera."),
                       "nocamera");
  gtk_stack_add_named (GTK_STACK (gui->stack),
                       placeholder_page ("image-x-generic", "No Pictures",
                                         "The camera is empty.\nTake a picture or upload one."),
                       "empty");
  gtk_box_pack_start (GTK_BOX (box), gui->stack, TRUE, TRUE, 0);

  /* Status bar */
  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (box), sep, FALSE, FALSE, 0);
  statusbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (statusbar), 6);
  gui->camera_label = gtk_label_new ("No camera");
  gtk_label_set_ellipsize (GTK_LABEL (gui->camera_label), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_max_width_chars (GTK_LABEL (gui->camera_label), 48);
  gui->status = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (gui->status), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_xalign (GTK_LABEL (gui->status), 0);
  gui->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (gui->progress), TRUE);
  gtk_widget_set_size_request (gui->progress, 200, -1);
  gtk_widget_set_valign (gui->progress, GTK_ALIGN_CENTER);
  gtk_widget_set_no_show_all (gui->progress, TRUE);
  gtk_box_pack_start (GTK_BOX (statusbar), gui->camera_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (statusbar), gtk_separator_new (GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (statusbar), gui->status, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (statusbar), gui->progress, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), statusbar, FALSE, FALSE, 0);

  gtk_widget_show_all (gui->window);
  show_page (gui);
  status (gui, "Welcome to gPhoto %s", PACKAGE_VERSION);
  update_sensitivity (gui);

  g_idle_add (startup_open, gui);
}

int
gphoto_gui_run (GPhotoOptions *options, const gchar *argv0)
{
  Gui gui = { 0 };
  char *argv[] = { (char *) argv0, NULL };
  int status;

  gui.options = options;
  gui.camera = gphoto_camera_new ();
  gphoto_camera_set_progress (gui.camera, progress_cb, &gui);

  g_set_application_name (GPHOTO_NAME);
  gtk_window_set_default_icon_name (GPHOTO_APP_ID);

  gui.app = gtk_application_new (GPHOTO_APP_ID, G_APPLICATION_NON_UNIQUE);
  g_signal_connect (gui.app, "activate", G_CALLBACK (on_activate), &gui);
  status = g_application_run (G_APPLICATION (gui.app), 1, argv);
  g_object_unref (gui.app);
  if (gui.camera)
    gphoto_camera_free (gui.camera);
  return status;
}
