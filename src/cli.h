/* cli.h - gPhoto command-line mode
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 * Licensed under the GNU General Public License, version 2 or later.
 */

#ifndef GPHOTO_CLI_H
#define GPHOTO_CLI_H

#include "gphoto.h"

typedef struct {
  /* camera selection */
  gchar   *model;
  gchar   *port;
  /* information */
  gboolean version;
  gboolean auto_detect;
  gboolean list_cameras;
  gboolean list_ports;
  gboolean summary;
  gboolean manual;
  gboolean about;
  /* pictures */
  gboolean num_pictures;
  gboolean list;
  gchar   *get;
  gchar   *get_thumbnail;
  gchar   *delete_range;
  gboolean capture;
  gchar  **upload;
  gchar   *folder;
  gchar   *output;
  gchar   *html;
  gchar   *title;
  gboolean force;
  gboolean quiet;
  /* user interface */
  gboolean no_gui;
} GPhotoOptions;

GOptionEntry *gphoto_cli_get_entries (GPhotoOptions *options);
gboolean      gphoto_cli_has_action  (const GPhotoOptions *options);
int           gphoto_cli_run         (GPhotoOptions *options);
void          gphoto_cli_options_clear (GPhotoOptions *options);

#endif
