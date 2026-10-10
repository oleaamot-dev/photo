/* html.c - HTML gallery export
 *
 * Writes a self-contained web gallery: one index page of thumbnails
 * linking to the full-size pictures, in the spirit of the gPhoto 0.4
 * "Publish HTML Gallery" plug-in.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#include "html.h"

#include <glib/gstdio.h>

static const gchar *page_head =
  "<!DOCTYPE html>\n"
  "<html lang=\"en\">\n<head>\n"
  "<meta charset=\"utf-8\">\n"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
  "<meta name=\"generator\" content=\"gPhoto " PACKAGE_VERSION "\">\n"
  "<title>%s</title>\n"
  "<style>\n"
  "body{margin:0;padding:24px 16px;font:15px/1.4 sans-serif;"
  "background:#f6f5f4;color:#2e3436}\n"
  "h1{font-weight:600;margin:0 0 4px}\n"
  "p.meta{margin:0 0 24px;color:#5e5c64}\n"
  "ul{list-style:none;margin:0;padding:0;display:grid;gap:16px;"
  "grid-template-columns:repeat(auto-fill,minmax(160px,1fr))}\n"
  "li{background:#fff;border-radius:8px;padding:8px;"
  "box-shadow:0 1px 3px rgba(0,0,0,.15);text-align:center}\n"
  "li img{max-width:100%%;height:120px;object-fit:contain;display:block;"
  "margin:0 auto 6px}\n"
  "li a{color:inherit;text-decoration:none}\n"
  "li span{font-size:13px;word-break:break-all}\n"
  "footer{margin-top:32px;font-size:12px;color:#77767b}\n"
  "@media (prefers-color-scheme:dark){body{background:#242424;color:#eee}"
  "li{background:#303030}p.meta,footer{color:#aaa}}\n"
  "</style>\n</head>\n<body>\n";

gchar *
gphoto_html_export (GPhotoCamera *camera,
                    GPtrArray    *pictures,
                    GArray       *indices,
                    const gchar  *dir,
                    const gchar  *title,
                    GError      **error)
{
  g_autoptr(GString) html = g_string_new (NULL);
  g_autofree gchar *esc_title = NULL;
  g_autofree gchar *index_path = NULL;
  g_autoptr(GDateTime) now = g_date_time_new_now_local ();
  g_autofree gchar *stamp = g_date_time_format (now, "%Y-%m-%d %H:%M");
  g_autofree gchar *thumbs = NULL;
  guint count = indices ? indices->len : pictures->len;

  if (title == NULL || *title == '\0')
    title = "gPhoto gallery";
  esc_title = g_markup_escape_text (title, -1);

  thumbs = g_build_filename (dir, "thumbnails", NULL);
  if (g_mkdir_with_parents (thumbs, 0755) != 0)
    {
      g_set_error (error, GPHOTO_ERROR, GPHOTO_ERROR_IO,
                   "Could not create %s", thumbs);
      return NULL;
    }

  g_string_append_printf (html, page_head, esc_title);
  g_string_append_printf (html, "<h1>%s</h1>\n", esc_title);
  g_string_append_printf (html, "<p class=\"meta\">%u picture%s from %s</p>\n<ul>\n",
                          count, count == 1 ? "" : "s",
                          gphoto_camera_get_model (camera)
                            ? gphoto_camera_get_model (camera) : "camera");

  for (guint i = 0; i < count; i++)
    {
      guint idx = indices ? g_array_index (indices, guint, i) : i + 1;
      GPhotoPicture *pic = gphoto_pictures_lookup (pictures, idx);
      g_autofree gchar *full = NULL, *thumb = NULL;
      g_autofree gchar *esc_name = NULL, *uri_name = NULL, *uri_thumb = NULL;
      g_autofree gchar *thumb_base = NULL;
      GError *thumb_error = NULL;

      if (pic == NULL)
        continue;

      full = gphoto_camera_save (camera, pic, FALSE, dir, error);
      if (full == NULL)
        return NULL;

      /* Not every camera can make thumbnails; fall back to the image. */
      thumb = gphoto_camera_save (camera, pic, TRUE, thumbs, &thumb_error);
      if (thumb == NULL)
        g_clear_error (&thumb_error);

      esc_name = g_markup_escape_text (pic->name, -1);
      uri_name = g_uri_escape_string (pic->name, NULL, FALSE);
      if (thumb != NULL)
        {
          thumb_base = g_path_get_basename (thumb);
          g_autofree gchar *t = g_uri_escape_string (thumb_base, NULL, FALSE);
          uri_thumb = g_strconcat ("thumbnails/", t, NULL);
        }
      else
        uri_thumb = g_strdup (uri_name);

      g_string_append_printf (html,
        "<li><a href=\"%s\"><img src=\"%s\" alt=\"%s\" loading=\"lazy\">"
        "<span>%u. %s</span></a></li>\n",
        uri_name, uri_thumb, esc_name, pic->index, esc_name);
    }

  g_string_append_printf (html,
    "</ul>\n<footer>Published with gPhoto %s, %s</footer>\n"
    "</body>\n</html>\n", PACKAGE_VERSION, stamp);

  index_path = g_build_filename (dir, "index.html", NULL);
  if (!g_file_set_contents (index_path, html->str, html->len, error))
    return NULL;

  return g_steal_pointer (&index_path);
}
