/* camera.c - gPhoto camera session on top of libgphoto2
 *
 * In gPhoto 0.4 every camera driver lived inside the program.  For
 * 0.5 the drivers moved out into libgphoto2, and this file is all
 * that remains of the old camera layer: a thin, GLib-friendly session
 * object that keeps the 0.4 notion of numbered pictures.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "camera.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct _GPhotoCamera {
  Camera    *camera;
  GPContext *context;
  gchar     *model;
  gchar     *port;
  gchar     *last_error;

  GPhotoProgressFunc progress_func;
  gpointer           progress_data;
  gfloat             progress_target;
  gchar             *progress_text;
};

G_DEFINE_QUARK (gphoto-error-quark, gphoto_error)

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

void
gphoto_picture_free (GPhotoPicture *pic)
{
  if (pic == NULL)
    return;
  g_free (pic->folder);
  g_free (pic->name);
  g_free (pic->mime_type);
  g_free (pic);
}

void
gphoto_detected_free (GPhotoDetected *d)
{
  if (d == NULL)
    return;
  g_free (d->model);
  g_free (d->port);
  g_free (d);
}

static gboolean
check (GPhotoCamera *camera, int ret, const gchar *what, GError **error)
{
  if (ret >= GP_OK)
    return TRUE;

  if (camera != NULL && camera->last_error != NULL)
    g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_CAMERA,
                 "%s: %s (%s)", what, gp_result_as_string (ret),
                 camera->last_error);
  else
    g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_CAMERA,
                 "%s: %s", what, gp_result_as_string (ret));

  if (camera != NULL)
    g_clear_pointer (&camera->last_error, g_free);
  return FALSE;
}

static void
context_error_cb (GPContext *context, const char *text, void *data)
{
  GPhotoCamera *camera = data;
  g_free (camera->last_error);
  camera->last_error = g_strdup (text);
}

static unsigned int
context_progress_start_cb (GPContext *context, float target,
                           const char *text, void *data)
{
  GPhotoCamera *camera = data;
  camera->progress_target = target > 0 ? target : 1;
  g_free (camera->progress_text);
  camera->progress_text = g_strdup (text);
  if (camera->progress_func)
    camera->progress_func (0.0, text, camera->progress_data);
  return 1;
}

static void
context_progress_update_cb (GPContext *context, unsigned int id,
                            float current, void *data)
{
  GPhotoCamera *camera = data;
  if (camera->progress_func)
    camera->progress_func (CLAMP (current / camera->progress_target, 0.0, 1.0),
                           camera->progress_text, camera->progress_data);
}

static void
context_progress_stop_cb (GPContext *context, unsigned int id, void *data)
{
  GPhotoCamera *camera = data;
  if (camera->progress_func)
    camera->progress_func (1.0, camera->progress_text, camera->progress_data);
}

/* ------------------------------------------------------------------ */
/* Life cycle                                                         */
/* ------------------------------------------------------------------ */

GPhotoCamera *
gphoto_camera_new (void)
{
  GPhotoCamera *camera = g_new0 (GPhotoCamera, 1);

  camera->context = gp_context_new ();
  gp_context_set_error_func (camera->context, context_error_cb, camera);
  gp_context_set_progress_funcs (camera->context,
                                 context_progress_start_cb,
                                 context_progress_update_cb,
                                 context_progress_stop_cb,
                                 camera);
  return camera;
}

void
gphoto_camera_free (GPhotoCamera *camera)
{
  if (camera == NULL)
    return;
  gphoto_camera_close (camera);
  gp_context_unref (camera->context);
  g_free (camera->last_error);
  g_free (camera->progress_text);
  g_free (camera);
}

void
gphoto_camera_set_progress (GPhotoCamera      *camera,
                            GPhotoProgressFunc func,
                            gpointer           user_data)
{
  camera->progress_func = func;
  camera->progress_data = user_data;
}

/* ------------------------------------------------------------------ */
/* Discovery                                                          */
/* ------------------------------------------------------------------ */

GPtrArray *
gphoto_camera_autodetect (GError **error)
{
  GPContext *context = gp_context_new ();
  CameraList *list = NULL;
  GPtrArray *result = NULL;
  int ret, i, n;

  gp_list_new (&list);
  ret = gp_camera_autodetect (list, context);
  if (!check (NULL, ret, "Auto-detection failed", error))
    goto out;

  result = g_ptr_array_new_with_free_func ((GDestroyNotify) gphoto_detected_free);
  n = gp_list_count (list);
  for (i = 0; i < n; i++)
    {
      const char *name = NULL, *value = NULL;
      GPhotoDetected *d;

      gp_list_get_name (list, i, &name);
      gp_list_get_value (list, i, &value);
      d = g_new0 (GPhotoDetected, 1);
      d->model = g_strdup (name);
      d->port = g_strdup (value);
      g_ptr_array_add (result, d);
    }

out:
  gp_list_free (list);
  gp_context_unref (context);
  return result;
}

GPtrArray *
gphoto_camera_list_models (GError **error)
{
  GPContext *context = gp_context_new ();
  CameraAbilitiesList *al = NULL;
  GPtrArray *result = NULL;
  int ret, i, n;

  gp_abilities_list_new (&al);
  ret = gp_abilities_list_load (al, context);
  if (!check (NULL, ret, "Could not load camera drivers", error))
    goto out;

  result = g_ptr_array_new_with_free_func (g_free);
  n = gp_abilities_list_count (al);
  for (i = 0; i < n; i++)
    {
      CameraAbilities a;
      if (gp_abilities_list_get_abilities (al, i, &a) >= GP_OK)
        g_ptr_array_add (result, g_strdup (a.model));
    }

out:
  gp_abilities_list_free (al);
  gp_context_unref (context);
  return result;
}

GPtrArray *
gphoto_camera_list_ports (GError **error)
{
  GPPortInfoList *pil = NULL;
  GPtrArray *result = NULL;
  int ret, i, n;

  gp_port_info_list_new (&pil);
  ret = gp_port_info_list_load (pil);
  if (!check (NULL, ret, "Could not load port drivers", error))
    goto out;

  result = g_ptr_array_new_with_free_func (g_free);
  n = gp_port_info_list_count (pil);
  for (i = 0; i < n; i++)
    {
      GPPortInfo info;
      char *path = NULL;

      if (gp_port_info_list_get_info (pil, i, &info) < GP_OK)
        continue;
      gp_port_info_get_path (info, &path);
      /* Generic entries such as "^usb:" are patterns, not ports. */
      if (path != NULL && *path != '\0' && *path != '^')
        g_ptr_array_add (result, g_strdup (path));
    }

out:
  gp_port_info_list_free (pil);
  return result;
}

/* ------------------------------------------------------------------ */
/* Opening                                                            */
/* ------------------------------------------------------------------ */

static gboolean
set_model (GPhotoCamera *camera, const gchar *model, GError **error)
{
  CameraAbilitiesList *al = NULL;
  CameraAbilities a;
  gboolean ok = FALSE;
  int idx;

  gp_abilities_list_new (&al);
  if (!check (camera, gp_abilities_list_load (al, camera->context),
              "Could not load camera drivers", error))
    goto out;

  idx = gp_abilities_list_lookup_model (al, model);
  if (idx < GP_OK)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_NO_CAMERA,
                   "Unknown camera model \"%s\" "
                   "(see gphoto --list-cameras)", model);
      goto out;
    }
  gp_abilities_list_get_abilities (al, idx, &a);
  ok = check (camera, gp_camera_set_abilities (camera->camera, a),
              "Could not select camera model", error);

out:
  gp_abilities_list_free (al);
  return ok;
}

static gboolean
set_port (GPhotoCamera *camera, const gchar *port, GError **error)
{
  GPPortInfoList *pil = NULL;
  GPPortInfo info;
  gboolean ok = FALSE;
  int idx;

  gp_port_info_list_new (&pil);
  if (!check (camera, gp_port_info_list_load (pil),
              "Could not load port drivers", error))
    goto out;

  idx = gp_port_info_list_lookup_path (pil, port);
  if (idx < GP_OK)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_NO_CAMERA,
                   "Unknown port \"%s\" (see gphoto --list-ports)", port);
      goto out;
    }
  gp_port_info_list_get_info (pil, idx, &info);
  ok = check (camera, gp_camera_set_port_info (camera->camera, info),
              "Could not select port", error);

out:
  gp_port_info_list_free (pil);
  return ok;
}

gboolean
gphoto_camera_open (GPhotoCamera *camera,
                    const gchar  *model,
                    const gchar  *port,
                    GError      **error)
{
  g_autofree gchar *use_model = g_strdup (model);
  g_autofree gchar *use_port = g_strdup (port);

  gphoto_camera_close (camera);

  /* Fill in whatever the caller left out from auto-detection. */
  if (use_model == NULL || use_port == NULL)
    {
      g_autoptr(GPtrArray) found = gphoto_camera_autodetect (error);
      guint i;

      if (found == NULL)
        return FALSE;

      for (i = 0; i < found->len; i++)
        {
          GPhotoDetected *d = g_ptr_array_index (found, i);
          if (use_model == NULL || g_strcmp0 (use_model, d->model) == 0)
            {
              if (use_model == NULL)
                use_model = g_strdup (d->model);
              if (use_port == NULL)
                use_port = g_strdup (d->port);
              break;
            }
        }

      if (use_model == NULL)
        {
          g_set_error_literal (error, GPHOTO_ERROR, GPHOTO_ERROR_NO_CAMERA,
                               "No camera detected. Connect and switch on "
                               "the camera, or choose a model and port.");
          return FALSE;
        }
      if (use_port == NULL)
        use_port = g_strdup ("usb:");
    }

  gp_camera_new (&camera->camera);

  if (!set_model (camera, use_model, error) ||
      !set_port (camera, use_port, error) ||
      !check (camera, gp_camera_init (camera->camera, camera->context),
              "Could not initialise camera", error))
    {
      gp_camera_unref (camera->camera);
      camera->camera = NULL;
      return FALSE;
    }

  camera->model = g_steal_pointer (&use_model);
  camera->port = g_steal_pointer (&use_port);
  return TRUE;
}

void
gphoto_camera_close (GPhotoCamera *camera)
{
  if (camera->camera != NULL)
    {
      gp_camera_exit (camera->camera, camera->context);
      gp_camera_unref (camera->camera);
      camera->camera = NULL;
    }
  g_clear_pointer (&camera->model, g_free);
  g_clear_pointer (&camera->port, g_free);
}

gboolean     gphoto_camera_is_open (GPhotoCamera *c)       { return c->camera != NULL; }
const gchar *gphoto_camera_get_model (GPhotoCamera *c)     { return c->model; }
const gchar *gphoto_camera_get_port (GPhotoCamera *c)      { return c->port; }
Camera      *gphoto_camera_get_gp_camera (GPhotoCamera *c) { return c->camera; }
GPContext   *gphoto_camera_get_context (GPhotoCamera *c)   { return c->context; }

static gboolean
require_open (GPhotoCamera *camera, GError **error)
{
  if (camera->camera != NULL)
    return TRUE;
  g_set_error_literal (error, GPHOTO_ERROR, GPHOTO_ERROR_NO_CAMERA,
                       "No camera is open");
  return FALSE;
}

/* ------------------------------------------------------------------ */
/* Pictures                                                           */
/* ------------------------------------------------------------------ */

static gchar *
join_folder (const gchar *folder, const gchar *name)
{
  if (g_str_has_suffix (folder, "/"))
    return g_strconcat (folder, name, NULL);
  return g_strconcat (folder, "/", name, NULL);
}

static gint
compare_strings (gconstpointer a, gconstpointer b)
{
  return g_strcmp0 (*(const gchar **) a, *(const gchar **) b);
}

static gboolean
walk_folder (GPhotoCamera *camera, const gchar *folder,
             GPtrArray *out, GError **error)
{
  CameraList *list = NULL;
  g_autoptr(GPtrArray) names = g_ptr_array_new_with_free_func (g_free);
  gboolean ok = FALSE;
  int i, n;

  gp_list_new (&list);

  /* Files first, sorted, so numbering is predictable. */
  if (!check (camera, gp_camera_folder_list_files (camera->camera, folder,
                                                   list, camera->context),
              "Could not list files", error))
    goto out;

  n = gp_list_count (list);
  for (i = 0; i < n; i++)
    {
      const char *name;
      gp_list_get_name (list, i, &name);
      g_ptr_array_add (names, g_strdup (name));
    }
  g_ptr_array_sort (names, compare_strings);

  for (guint j = 0; j < names->len; j++)
    {
      const gchar *name = g_ptr_array_index (names, j);
      GPhotoPicture *pic = g_new0 (GPhotoPicture, 1);
      CameraFileInfo info;

      pic->folder = g_strdup (folder);
      pic->name = g_strdup (name);

      memset (&info, 0, sizeof info);
      if (gp_camera_file_get_info (camera->camera, folder, name,
                                   &info, camera->context) >= GP_OK)
        {
          if (info.file.fields & GP_FILE_INFO_TYPE)
            pic->mime_type = g_strdup (info.file.type);
          if (info.file.fields & GP_FILE_INFO_SIZE)
            pic->size = info.file.size;
          if (info.file.fields & GP_FILE_INFO_MTIME)
            pic->mtime = info.file.mtime;
        }
      g_clear_pointer (&camera->last_error, g_free);

      pic->index = out->len + 1;
      g_ptr_array_add (out, pic);
    }

  /* Then sub-folders. */
  gp_list_reset (list);
  g_ptr_array_set_size (names, 0);
  if (!check (camera, gp_camera_folder_list_folders (camera->camera, folder,
                                                     list, camera->context),
              "Could not list folders", error))
    goto out;

  n = gp_list_count (list);
  for (i = 0; i < n; i++)
    {
      const char *name;
      gp_list_get_name (list, i, &name);
      g_ptr_array_add (names, g_strdup (name));
    }
  g_ptr_array_sort (names, compare_strings);

  for (guint j = 0; j < names->len; j++)
    {
      g_autofree gchar *sub = join_folder (folder, g_ptr_array_index (names, j));
      if (!walk_folder (camera, sub, out, error))
        goto out;
    }
  ok = TRUE;

out:
  gp_list_free (list);
  return ok;
}

GPtrArray *
gphoto_camera_list_pictures (GPhotoCamera *camera, GError **error)
{
  GPtrArray *out;

  if (!require_open (camera, error))
    return NULL;

  out = g_ptr_array_new_with_free_func ((GDestroyNotify) gphoto_picture_free);
  if (!walk_folder (camera, "/", out, error))
    {
      g_ptr_array_unref (out);
      return NULL;
    }
  return out;
}

GPhotoPicture *
gphoto_pictures_lookup (GPtrArray *pictures, guint index)
{
  if (pictures == NULL || index < 1 || index > pictures->len)
    return NULL;
  return g_ptr_array_index (pictures, index - 1);
}

/* Many cameras (and the Directory Browse driver) cannot hand out
 * previews.  gPhoto 0.4 then built its own index thumbnails; so do we,
 * by shrinking the full picture to GPHOTO_THUMB_SIZE. */
#define GPHOTO_THUMB_SIZE 160

static GBytes *
make_thumbnail (GBytes *full)
{
  g_autoptr(GdkPixbufLoader) loader = gdk_pixbuf_loader_new ();
  g_autoptr(GdkPixbuf) oriented = NULL, scaled = NULL;
  GdkPixbuf *pb;
  gchar *buf = NULL;
  gsize len = 0, flen;
  const guchar *fdata = g_bytes_get_data (full, &flen);
  gint w, h;
  gdouble s;

  if (!gdk_pixbuf_loader_write (loader, fdata, flen, NULL) ||
      !gdk_pixbuf_loader_close (loader, NULL) ||
      (pb = gdk_pixbuf_loader_get_pixbuf (loader)) == NULL)
    return NULL;

  oriented = gdk_pixbuf_apply_embedded_orientation (pb);
  w = gdk_pixbuf_get_width (oriented);
  h = gdk_pixbuf_get_height (oriented);
  s = MIN (1.0, MIN ((gdouble) GPHOTO_THUMB_SIZE / w, (gdouble) GPHOTO_THUMB_SIZE / h));
  scaled = gdk_pixbuf_scale_simple (oriented, MAX (1, (gint) (w * s)),
                                    MAX (1, (gint) (h * s)), GDK_INTERP_BILINEAR);
  if (gdk_pixbuf_get_has_alpha (scaled))
    {
      if (!gdk_pixbuf_save_to_buffer (scaled, &buf, &len, "png", NULL, NULL))
        return NULL;
    }
  else if (!gdk_pixbuf_save_to_buffer (scaled, &buf, &len, "jpeg", NULL,
                                       "quality", "85", NULL))
    return NULL;
  return g_bytes_new_take (buf, len);
}

static GBytes *get_data_raw (GPhotoCamera *, const GPhotoPicture *, gboolean, GError **);

GBytes *
gphoto_camera_get_data (GPhotoCamera        *camera,
                        const GPhotoPicture *pic,
                        gboolean             thumbnail,
                        GError             **error)
{
  GError *thumb_error = NULL;
  GBytes *bytes;
  g_autoptr(GBytes) full = NULL;

  if (!thumbnail)
    return get_data_raw (camera, pic, FALSE, error);

  bytes = get_data_raw (camera, pic, TRUE, &thumb_error);
  if (bytes != NULL)
    return bytes;

  /* Only images can be shrunk; leave movies and sounds alone. */
  if (pic->mime_type != NULL && !g_str_has_prefix (pic->mime_type, "image/"))
    {
      g_propagate_error (error, thumb_error);
      return NULL;
    }

  full = get_data_raw (camera, pic, FALSE, NULL);
  if (full != NULL && (bytes = make_thumbnail (full)) != NULL)
    {
      g_clear_error (&thumb_error);
      return bytes;
    }
  g_propagate_error (error, thumb_error);
  return NULL;
}

static GBytes *
get_data_raw (GPhotoCamera        *camera,
              const GPhotoPicture *pic,
              gboolean             thumbnail,
              GError             **error)
{
  CameraFile *file = NULL;
  const char *data = NULL;
  unsigned long size = 0;
  GBytes *bytes = NULL;

  if (!require_open (camera, error))
    return NULL;

  gp_file_new (&file);
  if (check (camera,
             gp_camera_file_get (camera->camera, pic->folder, pic->name,
                                 thumbnail ? GP_FILE_TYPE_PREVIEW
                                           : GP_FILE_TYPE_NORMAL,
                                 file, camera->context),
             thumbnail ? "Could not get thumbnail" : "Could not get picture",
             error) &&
      check (camera, gp_file_get_data_and_size (file, &data, &size),
             "Could not read picture data", error))
    bytes = g_bytes_new (data, size);

  gp_file_unref (file);
  return bytes;
}

gchar *
gphoto_camera_save (GPhotoCamera        *camera,
                    const GPhotoPicture *pic,
                    gboolean             thumbnail,
                    const gchar         *dest,
                    GError             **error)
{
  g_autoptr(GBytes) bytes = NULL;
  g_autofree gchar *path = NULL;
  gsize len;
  const gchar *data;

  if (dest == NULL || g_file_test (dest, G_FILE_TEST_IS_DIR))
    {
      g_autofree gchar *name = thumbnail
        ? g_strconcat ("thumb_", pic->name, NULL)
        : g_strdup (pic->name);
      path = g_build_filename (dest ? dest : ".", name, NULL);
    }
  else
    path = g_strdup (dest);

  bytes = gphoto_camera_get_data (camera, pic, thumbnail, error);
  if (bytes == NULL)
    return NULL;

  data = g_bytes_get_data (bytes, &len);
  if (!g_file_set_contents (path, data, len, error))
    return NULL;

  /* Keep the camera's time stamp on the saved file, like 0.4 did. */
  if (pic->mtime > 0 && !thumbnail)
    {
      struct timeval tv[2];
      tv[0].tv_sec = pic->mtime; tv[0].tv_usec = 0;
      tv[1].tv_sec = pic->mtime; tv[1].tv_usec = 0;
      (void) utimes (path, tv);
    }

  return g_steal_pointer (&path);
}

gboolean
gphoto_camera_delete (GPhotoCamera        *camera,
                      const GPhotoPicture *pic,
                      GError             **error)
{
  if (!require_open (camera, error))
    return FALSE;
  return check (camera,
                gp_camera_file_delete (camera->camera, pic->folder,
                                       pic->name, camera->context),
                "Could not delete picture", error);
}

GPhotoPicture *
gphoto_camera_capture (GPhotoCamera *camera, GError **error)
{
  CameraFilePath path;
  GPhotoPicture *pic;

  if (!require_open (camera, error))
    return NULL;

  memset (&path, 0, sizeof path);
  if (!check (camera,
              gp_camera_capture (camera->camera, GP_CAPTURE_IMAGE,
                                 &path, camera->context),
              "Could not take picture", error))
    return NULL;

  pic = g_new0 (GPhotoPicture, 1);
  pic->folder = g_strdup (path.folder);
  pic->name = g_strdup (path.name);
  return pic;
}

gboolean
gphoto_camera_upload (GPhotoCamera *camera,
                      const gchar  *local_path,
                      const gchar  *folder,
                      GError      **error)
{
  CameraFile *file = NULL;
  g_autofree gchar *base = NULL;
  gboolean ok;

  if (!require_open (camera, error))
    return FALSE;

  gp_file_new (&file);
  if (!check (camera, gp_file_open (file, local_path),
              "Could not open local file", error))
    {
      gp_file_unref (file);
      return FALSE;
    }

  base = g_path_get_basename (local_path);
  ok = check (camera,
              gp_camera_folder_put_file (camera->camera,
                                         folder ? folder : "/", base,
                                         GP_FILE_TYPE_NORMAL, file,
                                         camera->context),
              "Could not upload file", error);
  gp_file_unref (file);
  return ok;
}

/* ------------------------------------------------------------------ */
/* Text                                                               */
/* ------------------------------------------------------------------ */

typedef int (*TextFunc) (Camera *, CameraText *, GPContext *);

static gchar *
get_text (GPhotoCamera *camera, TextFunc func, const gchar *what,
          GError **error)
{
  CameraText text;

  if (!require_open (camera, error))
    return NULL;

  memset (&text, 0, sizeof text);
  if (!check (camera, func (camera->camera, &text, camera->context),
              what, error))
    return NULL;
  return g_strdup (text.text);
}

gchar *
gphoto_camera_get_summary (GPhotoCamera *camera, GError **error)
{
  return get_text (camera, gp_camera_get_summary,
                   "Could not get camera summary", error);
}

gchar *
gphoto_camera_get_manual (GPhotoCamera *camera, GError **error)
{
  return get_text (camera, gp_camera_get_manual,
                   "Could not get driver manual", error);
}

gchar *
gphoto_camera_get_about (GPhotoCamera *camera, GError **error)
{
  return get_text (camera, gp_camera_get_about,
                   "Could not get driver information", error);
}

/* ------------------------------------------------------------------ */
/* Ranges                                                             */
/* ------------------------------------------------------------------ */

static gboolean
parse_uint (const gchar *s, guint max, guint *out, GError **error)
{
  guint64 v;

  if (g_ascii_strcasecmp (s, "last") == 0)
    {
      *out = max;
      return max > 0 || (g_set_error_literal (error, GPHOTO_ERROR,
                                              GPHOTO_ERROR_RANGE,
                                              "There are no pictures"), FALSE);
    }
  if (!g_ascii_string_to_unsigned (s, 10, 1, G_MAXUINT, &v, NULL))
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_RANGE,
                   "\"%s\" is not a picture number", s);
      return FALSE;
    }
  if (v > max)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_RANGE,
                   "Picture %" G_GUINT64_FORMAT " does not exist "
                   "(the camera holds %u)", v, max);
      return FALSE;
    }
  *out = (guint) v;
  return TRUE;
}

GArray *
gphoto_parse_range (const gchar *spec, guint max, GError **error)
{
  g_autoptr(GArray) out = g_array_new (FALSE, FALSE, sizeof (guint));
  g_auto(GStrv) parts = NULL;
  g_autofree guchar *seen = NULL;

  if (spec == NULL || *spec == '\0')
    {
      g_set_error_literal (error, GPHOTO_ERROR, GPHOTO_ERROR_RANGE,
                           "Empty picture range");
      return NULL;
    }

  seen = g_malloc0 (max + 1);

  if (g_ascii_strcasecmp (spec, "all") == 0)
    {
      for (guint i = 1; i <= max; i++)
        g_array_append_val (out, i);
      return g_steal_pointer (&out);
    }

  parts = g_strsplit (spec, ",", -1);
  for (gchar **p = parts; *p; p++)
    {
      g_autofree gchar *item = g_strstrip (g_strdup (*p));
      gchar *dash = strchr (item, '-');
      guint lo, hi;

      if (*item == '\0')
        continue;

      if (dash != NULL)
        {
          *dash = '\0';
          if (!parse_uint (item, max, &lo, error) ||
              !parse_uint (dash + 1, max, &hi, error))
            return NULL;
          if (lo > hi)
            { guint t = lo; lo = hi; hi = t; }
        }
      else
        {
          if (!parse_uint (item, max, &lo, error))
            return NULL;
          hi = lo;
        }

      for (guint i = lo; i <= hi; i++)
        if (!seen[i])
          {
            seen[i] = 1;
            g_array_append_val (out, i);
          }
    }

  if (out->len == 0)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_RANGE,
                   "No pictures selected by \"%s\"", spec);
      return NULL;
    }
  return g_steal_pointer (&out);
}
