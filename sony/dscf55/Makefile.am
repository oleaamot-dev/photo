##
rs = rsony.c config.c config.h serio.c serio.h

drivers_LTLIBRARIES = libgphoto_sony_dscf55.la
bin_PROGRAMS = rsony

rsony_SOURCES =
rsony_LDADD = srsony.o config.o serio.o

srsony.o : $(srcdir)/rsony.c serio.o config.o $(srcdir)/serio.h
	$(COMPILE) -DSTAND_ALONE -o srsony.o -c $(srcdir)/rsony.c

## rsony_SOURCES = rmain.c rsony.c config.c serio.c serio.h

CFLAGS = @CFLAGS@ @GNOME_INCLUDEDIR@

libgphoto_sony_dscf55_la_SOURCES = $(rs)

libgphoto_sony_dscf55_la_LDFLAGS = -version-info 0:0:0

EXTRA_DIST = serio.h rsony.c serio.c config.c config.h
