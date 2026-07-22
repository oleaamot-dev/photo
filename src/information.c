#include <config.h>
#include <gnome.h>

#include "information.h"

void
dialog_version(void)
{
	gchar *message;
	message = g_strdup_printf (N_("This is GPhoto version %s.\n"
				      "Built %s by %s on %s.\n"
				      "Report bugs to gphoto-devel@gphoto.org"), 
				   VERSION, __DATE__, COMPILE_WHOAMI, COMPILE_HOST);
	dialog_notice(N_("GPhoto Release"), message, GTK_JUSTIFY_FILL);
	g_free (message);
}

void 
dialog_manual(void)
{
        GtkWidget *dialog, *gless;
	gchar *filename;

	dialog = gnome_dialog_new(N_("User manual"), NULL);
	filename = g_strconcat (DOCDIR, "/", "MANUAL", NULL);
	gless = gnome_less_new();
	if (filename) gnome_less_show_file (GNOME_LESS(gless), filename);
	g_free (filename);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), gless, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
}

void
dialog_faq(void)
{
	dialog_error("Please visit http://www.gphoto.org/help.php3\n" \
			"for the current FAQ list.");
}
  
void
dialog_license(void)
{
	dialog_notice(N_("GPhoto License"), 
		      N_("This program is free software; you can redistribute it and/or modify\n"
			 "it under the terms of the GNU General Public License as published by\n"
			 "the Free Software Foundation; either version 2 of the License, or\n"
			 "(at your option) any later version.\n\n"
			 "This program is distributed in the hope that it will be useful,\n"
			 "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			 "GNU General Public License for more details.\n\n"
			 "You should have received a copy of the GNU General Public License\n"
			 "along with this program; if not, write to the Free Software\n"
			 "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n"
			 "See http://www.gnu.org for more details on the GNU project.\n"),
		      GTK_JUSTIFY_FILL); 
}

/* 
 * Copied from Gnumeric and adopted for GPhoto.
 *  
 * Original Author:
 *  Miguel de Icaza (miguel@gnu.org)
 *
 */

/*
 * We need to get rid of that so that we will be able
 * to list everybody.  Something like guname would be
 * nice
 */
void
dialog_about (void)
{
	GtkWidget *about, *g1, *g2, *g3, *hbox;
	
	const gchar *authors[] = {
		N_("Scott Fritzinger, original implementation."),
		N_("Eugene Crosser, Fujitsu chipset protocol."),
		N_("Matt Martin, Fuji library, bug fixing."),
		N_("Mike Labriola, HTML gallery options."),
		N_("Werner Almesberger, Canon library."),
		N_("Ole W. Saastad, Canon library."),
		N_("Wolfgang Reissnegger, Canon library."),
		N_("Ray Newman, Canon PS350 protocol."),
		N_("Anne Bezemer, Canon PS350 protocol."),
		N_("Philippe Marzouk, Canon library."),
		N_("Edouard Lafargue, Canon library."),
		N_("Rasmus Andersen, Canon library, bugfixing"),
		N_("Gary Ross, Casio QV library."),
		N_("Del Simmons, Kodak DC20/25 library."),
		N_("Timecop, Kodak DC21x library and dc21x_cam utility."),
		N_("Brent D. Metz, Kodak 200/210 library"),
		N_("Brian Hirt, original Kodak DC210 library"),
		N_("Randy D. Scott, Kodak Generic serial library"),
		N_("David Brownell, Kodak Generic USB patch and Linux driver"),
		N_("Phill Hugo, Original Konica Q-M100, HP-C20/30 library"),
		N_("Ed Legowski, Konica Q-M100, HP-C20/30 library hacking"),
		N_("Toshiki Fujisawa, Konica Q-M200, FreeBSD patches"),
		N_("Gus Hartmann, Minolta Dimage V library"),
		N_("Mark Davies, Sony DSC-F55E/505 library"),
		N_("Michael McCormack, Nikon CoolPix 600 library"),
		N_("Henning Zabel, Mustek MDC800 library and Linux driver"),
		N_("Pedro Caria, Original Mustek MDC800 library"),
		N_("Bob Paauwe, Philips library"),
		N_("Cliff Wright, Ricoh RDC 300/300Z library"),
		N_("James McKenzie, Samsung Digimax 800k library"),
		N_("M. Adam Kendall, Sony DSC-F1 library"),
		N_("Warren Baird, Canon G1, bugfixes"),
		N_("Paul S. Jenner, initial GNU autoconf/automake scripts."),
		N_("Tuomas Kuosmanen, toolbar icons"),
		N_("Ole Aamot, bug fixing."),
		N_("Fabrice Bellet, Olympus C3030Z USB support, Index Canvas."),
		NULL
	};

#ifdef ENABLE_NLS
	{
 	    int i;

	    for (i = 0; authors[i] != NULL; i++){
		    authors [i] = _(authors [i]);
	    }
	}
#endif
        about = gnome_about_new (N_("GPhoto"), VERSION,
				 N_("(C) 1998-2000 Scott Fritzinger and many others"),
				 authors,
				 N_("The GNOME Digital Camera client"),
				 NULL);

	hbox = gtk_hbox_new (TRUE, 0);
	g1 = gnome_href_new ("http://www.gnu.org/", N_("GNU Project"));
	g2 = gnome_href_new ("http://www.gnome.org/", N_("GNOME Project"));
	g3 = gnome_href_new ("http://www.gphoto.org/", N_("GPhoto Project"));
	gtk_box_pack_start (GTK_BOX (hbox), g1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), g2, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), g3, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (about)->vbox), hbox, TRUE, FALSE, 0);
	gtk_widget_show_all (about);
}
