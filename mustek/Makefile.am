drivers_LTLIBRARIES = libgphoto_mustek_mdc800.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ -Wall

libgphoto_mustek_mdc800_la_SOURCES = rs232.c usb.c io.c image.c config.c core.c mdc800.c config.h \
                                     core.h image.h io.h mdc800.h mdc800_spec.h print.h rs232.h usb.h
libgphoto_mustek_mdc800_la_LDFLAGS = -version-info 1:2:0

