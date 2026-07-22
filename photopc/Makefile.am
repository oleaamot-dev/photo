drivers_LTLIBRARIES = libgphoto_photopc.la

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ \
	-DUNIX -DINT16=short -DHAVE_CFMAKERAW -DHAVE_STRFTIME \
	-DHAVE_PRIOCTL -DHAVE_NANOSLEEP

libgphoto_photopc_la_SOURCES = eph_read.c eph_iob.c eph_err.c eph_cmd.c \
	eph_io.c eph_open.c olympus.c photopc-usb.c \
	eph_io.h eph_priv.h photopc-usb.h olympus.h

libgphoto_photopc_la_LDFLAGS = -version-info 0:0:0
