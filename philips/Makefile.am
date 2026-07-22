drivers_LTLIBRARIES = libgphoto_philips.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ -DGPHOTO

libgphoto_philips_la_SOURCES = philips_io.c gphoto_philips.c \
        gphoto_philips_cfg.c \
        gphoto_philips_callbacks.c \
        gphoto_philips_setup.c \
        crctab.h debug.h philips_io.h philips.h callbacks.h

libgphoto_philips_la_LDFLAGS = -version-info 0:5:0
