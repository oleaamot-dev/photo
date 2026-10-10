/* viewer.h - picture viewer and simple editor
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#ifndef GPHOTO_VIEWER_H
#define GPHOTO_VIEWER_H

#include <gtk/gtk.h>

/* Open a viewer window on picture data already fetched from the
 * camera.  The window keeps its own reference to data. */
GtkWidget *gphoto_viewer_new (GtkWindow   *parent,
                              const gchar *name,
                              GBytes      *data,
                              GError     **error);

#endif
