drivers_LTLIBRARIES = libgphoto_casio_qv.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@

libgphoto_casio_qv_la_SOURCES = getuint.c ppm.c jpeg.c printMsg.c sdComm.c \
	configure.c config_handler.c qv_io.c command.c casio_qv.c \
	cam2jpgtab.h casio_qv_defines.h command.h config_handler.h \
	configure.h getuint.h jpeg.h jpegtab_f.h messages.h printMsg.h \
	qv_io.h sdComm.h sdCommDefines.h sdcMessages.h ppm.h

libgphoto_casio_qv_la_LDFLAGS = -version-info 0:0:0

EXTRA_DIST = README
