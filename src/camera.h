/* camera.h - gPhoto camera session on top of libgphoto2
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef GPHOTO_CAMERA_H
#define GPHOTO_CAMERA_H

#include "gphoto.h"
#include <gphoto2/gphoto2.h>
#include <gphoto2/gphoto2-version.h>

/* A picture stored on the camera.  gPhoto 0.4 addressed pictures by
 * their index (1..n); we keep that numbering for the user interface
 * and the command line, and map it onto libgphoto2's folder/name pairs. */
typedef struct {
  guint  index;    /* 1-based, stable for one listing */
  gchar *folder;
  gchar *name;
  gchar *mime_type;
  guint64 size;
  gint64  mtime;   /* seconds since epoch, 0 if unknown */
} GPhotoPicture;

void gphoto_picture_free (GPhotoPicture *pic);

/* A camera found by auto-detection. */
typedef struct {
  gchar *model;
  gchar *port;
} GPhotoDetected;

void gphoto_detected_free (GPhotoDetected *d);

typedef struct _GPhotoCamera GPhotoCamera;

typedef void (*GPhotoProgressFunc) (gdouble fraction, const gchar *message,
                                    gpointer user_data);

GPhotoCamera *gphoto_camera_new            (void);
void          gphoto_camera_free           (GPhotoCamera *camera);

/* Returns a GPtrArray of GPhotoDetected*. */
GPtrArray    *gphoto_camera_autodetect     (GError **error);
/* Returns a GPtrArray of gchar* model names known to libgphoto2. */
GPtrArray    *gphoto_camera_list_models    (GError **error);
/* Returns a GPtrArray of gchar* "path" entries (usb:, serial:, disk:...). */
GPtrArray    *gphoto_camera_list_ports     (GError **error);

/* Open a camera.  model and port may be NULL to use the first
 * auto-detected camera. */
gboolean      gphoto_camera_open           (GPhotoCamera *camera,
                                            const gchar  *model,
                                            const gchar  *port,
                                            GError      **error);
void          gphoto_camera_close          (GPhotoCamera *camera);
gboolean      gphoto_camera_is_open        (GPhotoCamera *camera);
const gchar  *gphoto_camera_get_model      (GPhotoCamera *camera);
const gchar  *gphoto_camera_get_port       (GPhotoCamera *camera);
Camera       *gphoto_camera_get_gp_camera  (GPhotoCamera *camera);
GPContext    *gphoto_camera_get_context    (GPhotoCamera *camera);

void          gphoto_camera_set_progress   (GPhotoCamera      *camera,
                                            GPhotoProgressFunc func,
                                            gpointer           user_data);

/* Index every picture on the camera, walking all folders.
 * Returns a GPtrArray of GPhotoPicture*. */
GPtrArray    *gphoto_camera_list_pictures  (GPhotoCamera *camera,
                                            GError      **error);

/* Fetch a picture (or its thumbnail) into memory. */
GBytes       *gphoto_camera_get_data       (GPhotoCamera        *camera,
                                            const GPhotoPicture *pic,
                                            gboolean             thumbnail,
                                            GError             **error);

/* Save a picture to a local file.  If dest is a directory the
 * camera's file name is kept.  Returns the path written. */
gchar        *gphoto_camera_save           (GPhotoCamera        *camera,
                                            const GPhotoPicture *pic,
                                            gboolean             thumbnail,
                                            const gchar         *dest,
                                            GError             **error);

gboolean      gphoto_camera_delete         (GPhotoCamera        *camera,
                                            const GPhotoPicture *pic,
                                            GError             **error);

/* Take a picture.  Returns the new picture (index 0, not yet listed). */
GPhotoPicture *gphoto_camera_capture       (GPhotoCamera *camera,
                                            GError      **error);

/* Upload a local file into the given camera folder ("/" if NULL). */
gboolean      gphoto_camera_upload         (GPhotoCamera *camera,
                                            const gchar  *local_path,
                                            const gchar  *folder,
                                            GError      **error);

gchar        *gphoto_camera_get_summary    (GPhotoCamera *camera,
                                            GError      **error);
gchar        *gphoto_camera_get_manual     (GPhotoCamera *camera,
                                            GError      **error);
gchar        *gphoto_camera_get_about      (GPhotoCamera *camera,
                                            GError      **error);

/* Look a picture up by its 1-based index in a listing. */
GPhotoPicture *gphoto_pictures_lookup      (GPtrArray *pictures,
                                            guint      index);

/* Parse "1,3,5-9" style ranges as accepted by gPhoto 0.4. */
GArray       *gphoto_parse_range           (const gchar *spec,
                                            guint        max,
                                            GError     **error);

#endif /* GPHOTO_CAMERA_H */
