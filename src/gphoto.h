/* gphoto.h - common declarations for gPhoto
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef GPHOTO_H
#define GPHOTO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#define GPHOTO_NAME      "gPhoto"
#define GPHOTO_TAGLINE   "the GNU Digital Camera Program"
#define GPHOTO_APP_ID    "org.gphoto.gPhoto"

/* Error domain used by every gPhoto module. */
#define GPHOTO_ERROR (gphoto_error_quark ())
GQuark gphoto_error_quark (void);

typedef enum {
  GPHOTO_ERROR_NO_CAMERA,
  GPHOTO_ERROR_CAMERA,
  GPHOTO_ERROR_IO,
  GPHOTO_ERROR_RANGE
} GPhotoError;

#endif /* GPHOTO_H */
