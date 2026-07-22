drivers_LTLIBRARIES = libgphoto_kodak_generic.la

CFLAGS = @CFLAGS@ -Wall @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ 

libgphoto_kodak_generic_la_SOURCES = \
   state_machine.c state_machine.h \
   kodak_command.c kodak_command.h \
   kodak_generic.c kodak_generic.h \
   kodak_dc240_helpers.c kodak_dc240_helpers.h \
   kodak_dc240.c kodak_dc240.h

libgphoto_kodak_generic_la_LDFLAGS = -version-info 0:0:0
