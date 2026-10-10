/* config-dialog.h - camera configuration dialog
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#ifndef GPHOTO_CONFIG_DIALOG_H
#define GPHOTO_CONFIG_DIALOG_H

#include <gtk/gtk.h>
#include "camera.h"

/* Runs modally.  Returns FALSE and sets error if the camera's
 * configuration could not be read or written. */
gboolean gphoto_config_dialog_run (GtkWindow    *parent,
                                   GPhotoCamera *camera,
                                   GError      **error);

#endif
