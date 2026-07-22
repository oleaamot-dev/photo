SUBDIRS = dc21x

drivers_LTLIBRARIES = libgphoto_kodak_dc2x.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ -DDC25=1

libgphoto_kodak_dc2x_la_SOURCES = kodak_dc210.c kodak_dc2x.c change_res.c \
	comet_to_ppm.c \
	convert_pic.c erase.c get_info.c get_pic.c get_thumb.c \
	hash_mark.c init_dc20.c pics_to_file.c pixmaps.c \
	read_data.c send_pck.c session.c shoot.c thumbs_to_file.c \
	change_res.h comet_to_ppm.h convert_pic.h dc20.h dc210.h erase.h \
	get_info.h get_pic.h get_thumb.h hash_mark.h init_dc20.h \
	kodak_dc210.h main.h pics_to_file.h pixmaps.h read_data.h \
	send_pck.h session.h shoot.h thumbs_to_file.h

libgphoto_kodak_dc2x_la_LDFLAGS = -version-info 0:0:0
