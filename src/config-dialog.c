/* config-dialog.c - camera configuration dialog
 *
 * Builds a notebook from libgphoto2's configuration tree: every
 * section becomes a page and every setting a labelled control.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "config-dialog.h"

#include <string.h>

typedef struct {
  CameraWidget *widget;
  GtkWidget    *control;
  CameraWidgetType type;
} Binding;

typedef struct {
  GArray *bindings;   /* of Binding */
} Dialog;

static GtkWidget *
make_control (CameraWidget *w, CameraWidgetType type)
{
  GtkWidget *control = NULL;

  switch (type)
    {
    case GP_WIDGET_TEXT:
      {
        char *value = NULL;
        control = gtk_entry_new ();
        gp_widget_get_value (w, &value);
        gtk_entry_set_text (GTK_ENTRY (control), value ? value : "");
        break;
      }
    case GP_WIDGET_RANGE:
      {
        float min = 0, max = 0, inc = 0, value = 0;
        gp_widget_get_range (w, &min, &max, &inc);
        gp_widget_get_value (w, &value);
        if (inc <= 0)
          inc = 1;
        if (max <= min)
          max = min + inc;
        control = gtk_spin_button_new_with_range (min, max, inc);
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (control),
                                    inc < 1 ? 2 : 0);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), value);
        break;
      }
    case GP_WIDGET_TOGGLE:
      {
        int value = 0;
        control = gtk_switch_new ();
        gtk_widget_set_halign (control, GTK_ALIGN_START);
        gp_widget_get_value (w, &value);
        gtk_switch_set_active (GTK_SWITCH (control), value != 0);
        break;
      }
    case GP_WIDGET_RADIO:
    case GP_WIDGET_MENU:
      {
        char *value = NULL;
        int n = gp_widget_count_choices (w);
        gboolean found = FALSE;
        control = gtk_combo_box_text_new ();
        gp_widget_get_value (w, &value);
        for (int i = 0; i < n; i++)
          {
            const char *choice = NULL;
            gp_widget_get_choice (w, i, &choice);
            if (choice == NULL)
              continue;
            gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (control), choice, choice);
            if (value && strcmp (value, choice) == 0)
              found = TRUE;
          }
        /* Some cameras report a current value outside the list. */
        if (value && !found)
          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (control), value, value);
        if (value)
          gtk_combo_box_set_active_id (GTK_COMBO_BOX (control), value);
        break;
      }
    case GP_WIDGET_DATE:
      {
        int value = 0;
        g_autoptr(GDateTime) dt = NULL;
        g_autofree gchar *text = NULL;
        control = gtk_entry_new ();
        gp_widget_get_value (w, &value);
        dt = g_date_time_new_from_unix_local (value);
        text = dt ? g_date_time_format (dt, "%Y-%m-%d %H:%M:%S") : g_strdup ("");
        gtk_entry_set_text (GTK_ENTRY (control), text);
        gtk_entry_set_placeholder_text (GTK_ENTRY (control), "YYYY-MM-DD HH:MM:SS");
        break;
      }
    default:
      break;
    }
  return control;
}

static void
add_settings (Dialog *d, GtkWidget *grid, CameraWidget *parent, int *row)
{
  int n = gp_widget_count_children (parent);

  for (int i = 0; i < n; i++)
    {
      CameraWidget *child = NULL;
      CameraWidgetType type;
      const char *label = NULL, *info = NULL;
      int readonly = 0;
      GtkWidget *control, *lab;
      Binding b;

      gp_widget_get_child (parent, i, &child);
      gp_widget_get_type (child, &type);

      if (type == GP_WIDGET_SECTION || type == GP_WIDGET_WINDOW)
        {
          add_settings (d, grid, child, row);
          continue;
        }

      control = make_control (child, type);
      if (control == NULL)
        continue;

      gp_widget_get_label (child, &label);
      gp_widget_get_info (child, &info);
      gp_widget_get_readonly (child, &readonly);

      lab = gtk_label_new (label);
      gtk_label_set_xalign (GTK_LABEL (lab), 1.0);
      gtk_widget_set_hexpand (control, TRUE);
      if (readonly)
        gtk_widget_set_sensitive (control, FALSE);
      if (info && *info)
        gtk_widget_set_tooltip_text (control, info);

      gtk_grid_attach (GTK_GRID (grid), lab, 0, *row, 1, 1);
      gtk_grid_attach (GTK_GRID (grid), control, 1, *row, 1, 1);
      (*row)++;

      if (!readonly)
        {
          b.widget = child;
          b.control = control;
          b.type = type;
          g_array_append_val (d->bindings, b);
        }
    }
}

static GtkWidget *
make_page (Dialog *d, CameraWidget *section)
{
  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *grid = gtk_grid_new ();
  int row = 0;

  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  add_settings (d, grid, section, &row);

  if (row == 0)
    {
      g_object_unref (g_object_ref_sink (grid));
      g_object_unref (g_object_ref_sink (scroll));
      return NULL;
    }

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroll), grid);
  return scroll;
}

static gboolean
store_value (Binding *b)
{
  switch (b->type)
    {
    case GP_WIDGET_TEXT:
      {
        const char *old = NULL;
        const char *now = gtk_entry_get_text (GTK_ENTRY (b->control));
        gp_widget_get_value (b->widget, &old);
        if (g_strcmp0 (old ? old : "", now) == 0)
          return FALSE;
        gp_widget_set_value (b->widget, now);
        return TRUE;
      }
    case GP_WIDGET_RANGE:
      {
        float old = 0;
        float now = gtk_spin_button_get_value (GTK_SPIN_BUTTON (b->control));
        gp_widget_get_value (b->widget, &old);
        if (old == now)
          return FALSE;
        gp_widget_set_value (b->widget, &now);
        return TRUE;
      }
    case GP_WIDGET_TOGGLE:
      {
        int old = 0;
        int now = gtk_switch_get_active (GTK_SWITCH (b->control)) ? 1 : 0;
        gp_widget_get_value (b->widget, &old);
        if ((old != 0) == now)
          return FALSE;
        gp_widget_set_value (b->widget, &now);
        return TRUE;
      }
    case GP_WIDGET_RADIO:
    case GP_WIDGET_MENU:
      {
        const char *old = NULL;
        const char *now = gtk_combo_box_get_active_id (GTK_COMBO_BOX (b->control));
        gp_widget_get_value (b->widget, &old);
        if (now == NULL || g_strcmp0 (old, now) == 0)
          return FALSE;
        gp_widget_set_value (b->widget, now);
        return TRUE;
      }
    case GP_WIDGET_DATE:
      {
        int old = 0, now;
        int y, mo, d, h, mi, s;
        const char *text = gtk_entry_get_text (GTK_ENTRY (b->control));
        g_autoptr(GDateTime) dt = NULL;
        g_autoptr(GTimeZone) tz = g_time_zone_new_local ();

        if (sscanf (text, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) != 6)
          return FALSE;
        dt = g_date_time_new (tz, y, mo, d, h, mi, s);
        if (dt == NULL)
          return FALSE;
        now = (int) g_date_time_to_unix (dt);
        gp_widget_get_value (b->widget, &old);
        if (old == now)
          return FALSE;
        gp_widget_set_value (b->widget, &now);
        return TRUE;
      }
    default:
      return FALSE;
    }
}

gboolean
gphoto_config_dialog_run (GtkWindow    *parent,
                          GPhotoCamera *camera,
                          GError      **error)
{
  Camera *cam = gphoto_camera_get_gp_camera (camera);
  GPContext *ctx = gphoto_camera_get_context (camera);
  CameraWidget *root = NULL;
  GtkWidget *dialog, *notebook, *content;
  Dialog d = { 0 };
  const char *title = NULL;
  int ret, n, pages = 0;
  gboolean ok = TRUE;

  if (cam == NULL)
    {
      g_set_error_literal (error, GPHOTO_ERROR, GPHOTO_ERROR_NO_CAMERA,
                           "No camera is open");
      return FALSE;
    }

  ret = gp_camera_get_config (cam, &root, ctx);
  if (ret == GP_ERROR_NOT_SUPPORTED)
    {
      GtkWidget *msg = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                               "This camera has no settings "
                                               "gPhoto can change.");
      gtk_dialog_run (GTK_DIALOG (msg));
      gtk_widget_destroy (msg);
      return TRUE;
    }
  if (ret < GP_OK)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_CAMERA,
                   "Could not read camera configuration: %s",
                   gp_result_as_string (ret));
      return FALSE;
    }

  d.bindings = g_array_new (FALSE, FALSE, sizeof (Binding));
  gp_widget_get_label (root, &title);

  dialog = gtk_dialog_new_with_buttons (title ? title : "Camera Configuration",
                                        parent,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Apply", GTK_RESPONSE_APPLY, NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 520, 480);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  notebook = gtk_notebook_new ();
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_box_pack_start (GTK_BOX (content), notebook, TRUE, TRUE, 0);

  /* Top-level sections become pages.  Cameras without sections get a
   * single "Settings" page below. */
  n = gp_widget_count_children (root);
  for (int i = 0; i < n; i++)
    {
      CameraWidget *child = NULL;
      CameraWidgetType type;
      const char *label = NULL;
      GtkWidget *page;

      gp_widget_get_child (root, i, &child);
      gp_widget_get_type (child, &type);
      if (type != GP_WIDGET_SECTION)
        continue;
      page = make_page (&d, child);
      if (page == NULL)
        continue;
      gp_widget_get_label (child, &label);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                                gtk_label_new (label));
      pages++;
    }

  if (pages == 0)
    {
      GtkWidget *page = make_page (&d, root);
      if (page != NULL)
        {
          gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page,
                                    gtk_label_new ("Settings"));
          pages++;
        }
    }

  if (pages == 0)
    {
      gtk_widget_destroy (dialog);
      g_array_unref (d.bindings);
      gp_widget_free (root);
      {
        GtkWidget *msg = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                                 "This camera has no settings "
                                                 "gPhoto can change.");
        gtk_dialog_run (GTK_DIALOG (msg));
        gtk_widget_destroy (msg);
      }
      return TRUE;
    }

  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY)
    {
      guint changed = 0;
      for (guint i = 0; i < d.bindings->len; i++)
        if (store_value (&g_array_index (d.bindings, Binding, i)))
          changed++;

      if (changed > 0)
        {
          ret = gp_camera_set_config (cam, root, ctx);
          if (ret < GP_OK)
            {
              g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_CAMERA,
                           "Could not change camera settings: %s",
                           gp_result_as_string (ret));
              ok = FALSE;
            }
        }
    }

  gtk_widget_destroy (dialog);
  g_array_unref (d.bindings);
  gp_widget_free (root);
  return ok;
}
