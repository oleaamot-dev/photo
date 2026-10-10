/* html.h - HTML gallery export
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#ifndef GPHOTO_HTML_H
#define GPHOTO_HTML_H

#include "camera.h"

/* Download the selected pictures (indices into pictures) and their
 * thumbnails into dir and write dir/index.html.  If indices is NULL
 * every picture is published.  Returns the path of index.html. */
gchar *gphoto_html_export (GPhotoCamera *camera,
                           GPtrArray    *pictures,
                           GArray       *indices,
                           const gchar  *dir,
                           const gchar  *title,
                           GError      **error);

#endif
