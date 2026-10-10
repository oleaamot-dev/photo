/* main.c - gPhoto, the GNU Digital Camera Program
 *
 * With no action switches gPhoto starts its graphical interface; any
 * of -n, -l, -s, -t, -d, -c (see gphoto --help) runs it as a
 * command-line tool instead, exactly as gPhoto 0.4 did.
 *
 * Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "gphoto.h"
#include "cli.h"
#ifdef HAVE_GUI
#include "gui.h"
#endif

#include <locale.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  GPhotoOptions options = { 0 };
  g_autofree GOptionEntry *entries = gphoto_cli_get_entries (&options);
  g_autoptr(GOptionContext) context = NULL;
  g_autoptr(GError) error = NULL;
  int status;

  setlocale (LC_ALL, "");

  context = g_option_context_new ("- " GPHOTO_TAGLINE);
  g_option_context_set_summary (context,
    "Without options gPhoto opens its window and connects to the first\n"
    "camera it finds.  With any picture or information option it runs\n"
    "on the command line.\n\n"
    "Pictures are numbered from 1 as in the index; RANGE is a list such\n"
    "as 1,3,5-9 or the words \"last\" and \"all\".");
  g_option_context_set_description (context,
    "Examples:\n"
    "  gphoto -a                         list connected cameras\n"
    "  gphoto -n                         how many pictures are there?\n"
    "  gphoto -s all -o ~/Pictures/trip  download everything\n"
    "  gphoto -t 1-12 -o thumbs          save the first twelve thumbnails\n"
    "  gphoto -c -o now.jpg              take a picture and save it\n"
    "  gphoto --html web --title Oslo    publish a gallery\n"
    "  gphoto -m \"Directory Browse\" -p disk:/media/card -l\n\n"
    "Report bugs to <" PACKAGE_BUGREPORT ">.");
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      fprintf (stderr, "gphoto: %s\nTry \"gphoto --help\".\n", error->message);
      return 2;
    }

  if (argc > 1)
    {
      fprintf (stderr, "gphoto: unexpected argument \"%s\"\n"
               "Try \"gphoto --help\".\n", argv[1]);
      return 2;
    }

  if (gphoto_cli_has_action (&options))
    status = gphoto_cli_run (&options);
#ifdef HAVE_GUI
  else if (!options.no_gui)
    status = gphoto_gui_run (&options, argv[0]);
#endif
  else
    {
      g_autofree gchar *help = g_option_context_get_help (context, TRUE, NULL);
      fputs (help, stderr);
      status = 2;
    }

  gphoto_cli_options_clear (&options);
  return status;
}
