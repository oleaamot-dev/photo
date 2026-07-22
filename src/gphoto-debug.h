#ifndef __GPHOTO_DEBUG_H__
#define __GPHOTO_DEBUG_H__

#include <glib.h>

typedef enum {
  GP_DEBUG_CANVAS,
  GP_DEBUG_CONFIG,
  GP_DEBUG_DELETE,
  GP_DEBUG_DIALOG,
  GP_DEBUG_FORMAT,
  GP_DEBUG_GALLERY,
  GP_DEBUG_GET_INDEX,
  GP_DEBUG_GET_IMAGE,
  GP_DEBUG_GET_THUMB,
  GP_DEBUG_INDEX,
  GP_DEBUG_INIT,
  GP_DEBUG_LIVE,
  GP_DEBUG_LOAD_CFG,
  GP_DEBUG_LOAD_IMG,
  GP_DEBUG_NEXT,
  GP_DEBUG_PREV,
  GP_DEBUG_PRINT,
  GP_DEBUG_SAVE_CFG,
  GP_DEBUG_SAVE_IMG,
  GP_DEBUG_SCALE,
  GP_DEBUG_SET_MODEL,
  GP_DEBUG_SUMMARY,
} GPhotoDebugSection;

extern gint debug;
extern gint debug_canvas;
extern gint debug_config;
extern gint debug_delete;
extern gint debug_dialog;
extern gint debug_format;
extern gint debug_gallery;
extern gint debug_get_index;
extern gint debug_get_image;
extern gint debug_get_thumb;
extern gint debug_index;
extern gint debug_init;
extern gint debug_live;
extern gint debug_load_cfg;
extern gint debug_load_img;
extern gint debug_next;
extern gint debug_prev;
extern gint debug_print;
extern gint debug_save_cfg;
extern gint debug_save_img;
extern gint debug_scale;
extern gint debug_set_model;
extern gint debug_summary;

#ifndef __GNUC__
#warning "No GNU C library (http://www.gnu.org/glibc/glibc.html)."
#define __FUNCTION__     ""
#endif

#define DEBUG_CANVAS     GP_DEBUG_CANVAS,    __FILE__, __LINE__, __FUNCTION__
#define DEBUG_CONFIG     GP_DEBUG_CONFIG,    __FILE__, __LINE__, __FUNCTION__
#define DEBUG_DELETE     GP_DEBUG_DELETE,    __FILE__, __LINE__, __FUNCTION__
#define DEBUG_DIALOG     GP_DEBUG_DIALOG,    __FILE__, __LINE__, __FUNCTION__
#define DEBUG_FORMAT     GP_DEBUG_FORMAT,    __FILE__, __LINE__, __FUNCTION__
#define DEBUG_GALLERY    GP_DEBUG_GALLERY,   __FILE__, __LINE__, __FUNCTION__
#define DEBUG_GET_INDEX  GP_DEBUG_GET_INDEX, __FILE__, __LINE__, __FUNCTION__
#define DEBUG_GET_IMAGE  GP_DEBUG_GET_IMAGE, __FILE__, __LINE__, __FUNCTION__
#define DEBUG_GET_THUMB  GP_DEBUG_GET_THUMB, __FILE__, __LINE__, __FUNCTION__
#define DEBUG_INIT       GP_DEBUG_INIT,      __FILE__, __LINE__, __FUNCTION__
#define DEBUG_LIVE       GP_DEBUG_LIVE,      __FILE__, __LINE__, __FUNCTION__
#define DEBUG_LOAD_CFG   GP_DEBUG_LOAD_CFG,  __FILE__, __LINE__, __FUNCTION__
#define DEBUG_LOAD_IMG   GP_DEBUG_LOAD_IMG,  __FILE__, __LINE__, __FUNCTION__
#define DEBUG_NEXT       GP_DEBUG_NEXT,      __FILE__, __LINE__, __FUNCTION__
#define DEBUG_PREV       GP_DEBUG_PREV,      __FILE__, __LINE__, __FUNCTION__
#define DEBUG_PRINT      GP_DEBUG_PRINT,     __FILE__, __LINE__, __FUNCTION__
#define DEBUG_SAVE_CFG   GP_DEBUG_SAVE_CFG,  __FILE__, __LINE__, __FUNCTION__
#define DEBUG_SAVE_IMG   GP_DEBUG_SAVE_IMG,  __FILE__, __LINE__, __FUNCTION__
#define DEBUG_SCALE      GP_DEBUG_SCALE,     __FILE__, __LINE__, __FUNCTION__
#define DEBUG_SET_MODEL  GP_DEBUG_SET_MODEL, __FILE__, __LINE__, __FUNCTION__
#define DEBUG_SUMMARY    GP_DEBUG_SUMMARY,   __FILE__, __LINE__, __FUNCTION__

void    gphoto_debug_msg (gchar *message, GPhotoDebugSection type);
void    gphoto_debug (gint section, gchar *file, gint line, gchar *function, gchar *format, ...);

#endif /* __GPHOTO_DEBUG_H__ */

