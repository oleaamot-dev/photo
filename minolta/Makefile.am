drivers_LTLIBRARIES = libgphoto_dimage_v.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@

libgphoto_dimage_v_la_SOURCES = dimage_v.h dimage_v_gphoto.c dimage_v_util.c \
	no_thumbs.h

bin_PROGRAMS = minolta-dump	

minolta_dump_SOURCES = dimage_v.h dimage_v_gphoto.c dimage_v_util.c test.c 

libgphoto_dimage_v_la_LDFLAGS = -version-info 0:0:0
