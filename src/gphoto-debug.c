#include "gphoto-debug.h"

void
gphoto_debug (gint section, gchar *file, gint line, gchar *function, gchar *format, ...)
{
  va_list args;
  gchar *msg;

  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);

  if (debug || 
      (debug_canvas        && section == GP_DEBUG_CANVAS)           ||
      (debug_config        && section == GP_DEBUG_CONFIG)           ||
      (debug_delete        && section == GP_DEBUG_DELETE)           ||
      (debug_dialog        && section == GP_DEBUG_DIALOG)           ||
      (debug_format        && section == GP_DEBUG_FORMAT)           ||
      (debug_gallery       && section == GP_DEBUG_GALLERY)          ||
      (debug_get_index     && section == GP_DEBUG_GET_INDEX)        ||
      (debug_get_image     && section == GP_DEBUG_GET_IMAGE)        ||
      (debug_get_thumb     && section == GP_DEBUG_GET_THUMB)        ||
      (debug_index         && section == GP_DEBUG_INDEX)            ||
      (debug_init          && section == GP_DEBUG_INIT)             ||
      (debug_live          && section == GP_DEBUG_LIVE)             ||
      (debug_load_cfg      && section == GP_DEBUG_LOAD_CFG)         ||
      (debug_load_img      && section == GP_DEBUG_LOAD_IMG)         ||
      (debug_next          && section == GP_DEBUG_NEXT)             ||
      (debug_prev          && section == GP_DEBUG_PREV)             ||
      (debug_print         && section == GP_DEBUG_PRINT)            ||
      (debug_save_cfg      && section == GP_DEBUG_SAVE_CFG)         ||
      (debug_save_img      && section == GP_DEBUG_SAVE_IMG)         ||
      (debug_scale         && section == GP_DEBUG_SCALE)            ||
      (debug_set_model     && section == GP_DEBUG_SET_MODEL)        || 
      (debug_summary       && section == GP_DEBUG_SUMMARY) )
    g_print ("%s:%d (%s) %s\n", file, line, function, msg);
  g_free (msg);
}
