/* cli.c - gPhoto command-line mode
 *
 * gPhoto 0.4 could be driven from scripts with -n, -s, -t and -d.
 * Those switches keep working here, alongside long options shared
 * with gphoto2.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "cli.h"
#include "camera.h"
#include "html.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>

GOptionEntry *
gphoto_cli_get_entries (GPhotoOptions *o)
{
  GOptionEntry entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &o->version,
      "Show the gPhoto version", NULL },
    { "model", 'm', 0, G_OPTION_ARG_STRING, &o->model,
      "Camera model (default: auto-detect)", "MODEL" },
    { "port", 'p', 0, G_OPTION_ARG_STRING, &o->port,
      "Port, e.g. usb: or serial:/dev/ttyS0 or disk:/path", "PORT" },
    { "auto-detect", 'a', 0, G_OPTION_ARG_NONE, &o->auto_detect,
      "List cameras connected to this computer", NULL },
    { "list-cameras", 0, 0, G_OPTION_ARG_NONE, &o->list_cameras,
      "List every supported camera model", NULL },
    { "list-ports", 0, 0, G_OPTION_ARG_NONE, &o->list_ports,
      "List available ports", NULL },
    { "summary", 0, 0, G_OPTION_ARG_NONE, &o->summary,
      "Show the camera summary", NULL },
    { "manual", 0, 0, G_OPTION_ARG_NONE, &o->manual,
      "Show the camera driver manual", NULL },
    { "about", 0, 0, G_OPTION_ARG_NONE, &o->about,
      "Show information about the camera driver", NULL },
    { "num-pictures", 'n', 0, G_OPTION_ARG_NONE, &o->num_pictures,
      "Print the number of pictures on the camera", NULL },
    { "list", 'l', 0, G_OPTION_ARG_NONE, &o->list,
      "List the pictures on the camera", NULL },
    { "get", 's', 0, G_OPTION_ARG_STRING, &o->get,
      "Save pictures, e.g. 1,3,5-9, last or all", "RANGE" },
    { "get-thumbnail", 't', 0, G_OPTION_ARG_STRING, &o->get_thumbnail,
      "Save thumbnails of pictures", "RANGE" },
    { "delete", 'd', 0, G_OPTION_ARG_STRING, &o->delete_range,
      "Delete pictures from the camera", "RANGE" },
    { "capture", 'c', 0, G_OPTION_ARG_NONE, &o->capture,
      "Take a picture (saved with --output if given)", NULL },
    { "upload", 'u', 0, G_OPTION_ARG_FILENAME_ARRAY, &o->upload,
      "Upload a file to the camera (repeatable)", "FILE" },
    { "folder", 'f', 0, G_OPTION_ARG_STRING, &o->folder,
      "Camera folder for --upload (default /)", "FOLDER" },
    { "output", 'o', 0, G_OPTION_ARG_FILENAME, &o->output,
      "File or directory to save to (default .)", "PATH" },
    { "html", 0, 0, G_OPTION_ARG_FILENAME, &o->html,
      "Publish an HTML gallery of --get RANGE (or all) into DIR", "DIR" },
    { "title", 0, 0, G_OPTION_ARG_STRING, &o->title,
      "Title of the HTML gallery", "TITLE" },
    { "force", 0, 0, G_OPTION_ARG_NONE, &o->force,
      "Do not ask before deleting pictures", NULL },
    { "quiet", 'q', 0, G_OPTION_ARG_NONE, &o->quiet,
      "Print only what was asked for", NULL },
    { "no-gui", 0, 0, G_OPTION_ARG_NONE, &o->no_gui,
      "Never start the graphical interface", NULL },
    { NULL }
  };
  return g_memdup2 (entries, sizeof entries);
}

gboolean
gphoto_cli_has_action (const GPhotoOptions *o)
{
  return o->version || o->auto_detect || o->list_cameras || o->list_ports ||
         o->summary || o->manual || o->about || o->num_pictures || o->list ||
         o->get || o->get_thumbnail || o->delete_range || o->capture ||
         o->upload || o->html;
}

void
gphoto_cli_options_clear (GPhotoOptions *o)
{
  g_free (o->model);
  g_free (o->port);
  g_free (o->get);
  g_free (o->get_thumbnail);
  g_free (o->delete_range);
  g_strfreev (o->upload);
  g_free (o->folder);
  g_free (o->output);
  g_free (o->html);
  g_free (o->title);
  memset (o, 0, sizeof *o);
}

/* ------------------------------------------------------------------ */

static gboolean quiet;

static void
info (const gchar *fmt, ...) G_GNUC_PRINTF (1, 2);

static void
info (const gchar *fmt, ...)
{
  va_list ap;
  if (quiet)
    return;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
}

static int
fail (GError *error)
{
  fprintf (stderr, "gphoto: %s\n", error ? error->message : "failed");
  g_clear_error (&error);
  return 1;
}

static void
progress_cb (gdouble fraction, const gchar *message, gpointer data)
{
  if (quiet || !isatty (STDERR_FILENO))
    return;
  fprintf (stderr, "\r  %-40.40s %3d%%", message ? message : "",
           (int) (fraction * 100.0));
  if (fraction >= 1.0)
    fputc ('\n', stderr);
}

static void
print_string_list (GPtrArray *list)
{
  for (guint i = 0; i < list->len; i++)
    printf ("%s\n", (const gchar *) g_ptr_array_index (list, i));
}

static gchar *
format_size (guint64 size)
{
  return size ? g_format_size (size) : g_strdup ("-");
}

static gchar *
format_time (gint64 t)
{
  g_autoptr(GDateTime) dt = NULL;
  if (t <= 0)
    return g_strdup ("-");
  dt = g_date_time_new_from_unix_local (t);
  return g_date_time_format (dt, "%Y-%m-%d %H:%M");
}

/* Decide where picture n of count goes when saving a range. */
static const gchar *
resolve_output (const GPhotoOptions *o, guint count, GError **error)
{
  const gchar *out = o->output ? o->output : ".";

  if (count > 1 && !g_file_test (out, G_FILE_TEST_IS_DIR))
    {
      if (g_mkdir_with_parents (out, 0755) != 0)
        {
          g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_IO,
                       "Could not create directory %s", out);
          return NULL;
        }
    }
  return out;
}

static int
save_range (GPhotoCamera *camera, GPtrArray *pics, const GPhotoOptions *o,
            const gchar *range, gboolean thumbnail)
{
  GError *error = NULL;
  g_autoptr(GArray) idx = gphoto_parse_range (range, pics->len, &error);
  const gchar *out;

  if (idx == NULL)
    return fail (error);
  out = resolve_output (o, idx->len, &error);
  if (out == NULL)
    return fail (error);

  for (guint i = 0; i < idx->len; i++)
    {
      GPhotoPicture *pic = gphoto_pictures_lookup (pics, g_array_index (idx, guint, i));
      g_autofree gchar *path = gphoto_camera_save (camera, pic, thumbnail, out, &error);
      if (path == NULL)
        return fail (error);
      info ("Saved %s %u (%s) to %s", thumbnail ? "thumbnail" : "picture",
            pic->index, pic->name, path);
    }
  return 0;
}

static gboolean
confirm (const gchar *question)
{
  gchar answer[16];

  if (!isatty (STDIN_FILENO))
    return FALSE;
  fprintf (stderr, "%s [y/N] ", question);
  fflush (stderr);
  if (fgets (answer, sizeof answer, stdin) == NULL)
    return FALSE;
  return answer[0] == 'y' || answer[0] == 'Y';
}

static int
delete_range (GPhotoCamera *camera, GPtrArray *pics, const GPhotoOptions *o)
{
  GError *error = NULL;
  g_autoptr(GArray) idx = gphoto_parse_range (o->delete_range, pics->len, &error);
  g_autofree gchar *q = NULL;

  if (idx == NULL)
    return fail (error);

  q = g_strdup_printf ("Delete %u picture%s from the camera?",
                       idx->len, idx->len == 1 ? "" : "s");
  if (!o->force && !confirm (q))
    {
      fprintf (stderr, "gphoto: not deleting (use --force in scripts)\n");
      return 1;
    }

  for (guint i = 0; i < idx->len; i++)
    {
      GPhotoPicture *pic = gphoto_pictures_lookup (pics, g_array_index (idx, guint, i));
      if (!gphoto_camera_delete (camera, pic, &error))
        return fail (error);
      info ("Deleted picture %u (%s%s%s)", pic->index, pic->folder,
            g_str_has_suffix (pic->folder, "/") ? "" : "/", pic->name);
    }
  return 0;
}

typedef gchar *(*TextGetter) (GPhotoCamera *, GError **);

static int
print_text (GPhotoCamera *camera, TextGetter get)
{
  GError *error = NULL;
  gchar *text = get (camera, &error);

  if (text == NULL)
    return fail (error);
  fputs (text, stdout);
  if (*text && text[strlen (text) - 1] != '\n')
    fputc ('\n', stdout);
  g_free (text);
  return 0;
}

int
gphoto_cli_run (GPhotoOptions *o)
{
  GError *error = NULL;
  g_autoptr(GPtrArray) pics = NULL;
  GPhotoCamera *camera = NULL;
  int status = 0;

  quiet = o->quiet;

  if (o->version)
    {
      printf ("gPhoto %s - %s\n", PACKAGE_VERSION, GPHOTO_TAGLINE);
      {
        const char **v = gp_library_version (GP_VERSION_SHORT);
        printf ("libgphoto2 %s\n", v && v[0] ? v[0] : "unknown");
      }
      return 0;
    }

  /* Things that need no camera. */
  if (o->auto_detect)
    {
      g_autoptr(GPtrArray) found = gphoto_camera_autodetect (&error);
      if (found == NULL)
        return fail (error);
      printf ("%-40s %s\n", "Model", "Port");
      printf ("%-40s %s\n", "-----", "----");
      for (guint i = 0; i < found->len; i++)
        {
          GPhotoDetected *d = g_ptr_array_index (found, i);
          printf ("%-40s %s\n", d->model, d->port);
        }
    }
  if (o->list_cameras)
    {
      g_autoptr(GPtrArray) models = gphoto_camera_list_models (&error);
      if (models == NULL)
        return fail (error);
      print_string_list (models);
      info ("%u supported cameras", models->len);
    }
  if (o->list_ports)
    {
      g_autoptr(GPtrArray) ports = gphoto_camera_list_ports (&error);
      if (ports == NULL)
        return fail (error);
      print_string_list (ports);
    }

  if (!(o->summary || o->manual || o->about || o->num_pictures || o->list ||
        o->get || o->get_thumbnail || o->delete_range || o->capture ||
        o->upload || o->html))
    return 0;

  camera = gphoto_camera_new ();
  gphoto_camera_set_progress (camera, progress_cb, NULL);
  if (!gphoto_camera_open (camera, o->model, o->port, &error))
    {
      gphoto_camera_free (camera);
      return fail (error);
    }
  info ("Using %s on %s", gphoto_camera_get_model (camera),
        gphoto_camera_get_port (camera));

  if (o->summary && (status = print_text (camera, gphoto_camera_get_summary)))
    goto out;
  if (o->manual && (status = print_text (camera, gphoto_camera_get_manual)))
    goto out;
  if (o->about && (status = print_text (camera, gphoto_camera_get_about)))
    goto out;

  /* Uploads and captures change the listing, so do them first. */
  for (gchar **f = o->upload; f && *f; f++)
    {
      if (!gphoto_camera_upload (camera, *f, o->folder, &error))
        { status = fail (error); goto out; }
      info ("Uploaded %s to %s", *f, o->folder ? o->folder : "/");
    }

  if (o->capture)
    {
      GPhotoPicture *pic = gphoto_camera_capture (camera, &error);
      if (pic == NULL)
        { status = fail (error); goto out; }
      info ("New picture %s/%s", pic->folder, pic->name);
      if (o->output)
        {
          g_autofree gchar *path = gphoto_camera_save (camera, pic, FALSE,
                                                       o->output, &error);
          if (path == NULL)
            { gphoto_picture_free (pic); status = fail (error); goto out; }
          info ("Saved to %s", path);
        }
      gphoto_picture_free (pic);
    }

  if (o->num_pictures || o->list || o->get || o->get_thumbnail ||
      o->delete_range || o->html)
    {
      pics = gphoto_camera_list_pictures (camera, &error);
      if (pics == NULL)
        { status = fail (error); goto out; }
    }

  if (o->num_pictures)
    printf ("%u\n", pics->len);

  if (o->list)
    {
      for (guint i = 0; i < pics->len; i++)
        {
          GPhotoPicture *pic = g_ptr_array_index (pics, i);
          g_autofree gchar *size = format_size (pic->size);
          g_autofree gchar *when = format_time (pic->mtime);
          g_autofree gchar *path = g_build_filename (pic->folder, pic->name, NULL);
          printf ("#%-4u %-36s %10s  %-16s  %s\n", pic->index, path, size, when,
                  pic->mime_type ? pic->mime_type : "");
        }
    }

  if (o->html)
    {
      g_autoptr(GArray) idx = NULL;
      g_autofree gchar *index = NULL;

      if (o->get)
        {
          idx = gphoto_parse_range (o->get, pics->len, &error);
          if (idx == NULL)
            { status = fail (error); goto out; }
        }
      index = gphoto_html_export (camera, pics, idx, o->html, o->title, &error);
      if (index == NULL)
        { status = fail (error); goto out; }
      info ("Gallery written to %s", index);
    }
  else if (o->get && (status = save_range (camera, pics, o, o->get, FALSE)))
    goto out;

  if (o->get_thumbnail && (status = save_range (camera, pics, o, o->get_thumbnail, TRUE)))
    goto out;

  /* Delete last, after anything that was asked to be saved. */
  if (o->delete_range && (status = delete_range (camera, pics, o)))
    goto out;

out:
  gphoto_camera_free (camera);
  return status;
}
