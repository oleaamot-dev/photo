drivers_LTLIBRARIES = libgphoto_fuji.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@

libgphoto_fuji_la_SOURCES = fujilib.c fuji_exif.c exif.c\
	configdialog.c exif.h exif_tags.h gphoto_fuji.h

libgphoto_fuji_la_LDFLAGS = -version-info 0:0:0

bin_PROGRAMS = gphoto-exifdump

gphoto_exifdump_SOURCES = fuji_exif.c exif.c gphoto-exifdump.c\
		exif.h exif_tags.h 

EXTRA_DIST = blank.xpm

