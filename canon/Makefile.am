drivers_LTLIBRARIES = libgphoto_canon.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ -Wall

libgphoto_canon_la_SOURCES = canon.c canon.h psa50.c psa50.h serial.c serial.h \
	crc.c crc.h util.c util.h

libgphoto_canon_la_LDFLAGS = -version-info 0:0:0

#bin_PROGRAMS = gphoto-exifdump
#
#gphoto_exifdump_SOURCES = fuji_exif.c exif.c gphoto-exifdump.c\
#                exif.h exif_tags.h
#
#EXTRA_DIST = blank.xpm
