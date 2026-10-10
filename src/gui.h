/* gui.h - gPhoto main window
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#ifndef GPHOTO_GUI_H
#define GPHOTO_GUI_H

#include "cli.h"

/* Run the GTK+ interface.  model/port from the options, if set, are
 * opened at start-up; otherwise the first auto-detected camera is. */
int gphoto_gui_run (GPhotoOptions *options, const gchar *argv0);

#endif
