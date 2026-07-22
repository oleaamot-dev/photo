bin_PROGRAMS = qmxxx 
drivers_LTLIBRARIES = libgphoto_konica_qmxxx.la

CFLAGS   = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@
# EXTRA_DIST = ChangeLog TODO gettraces 

libgphoto_konica_qmxxx_la_SOURCES =  os.c log.c qm.c dcx.c dcx.h log.h os.h

qmxxx_SOURCES=  main.c os.c log.c qm.c dcx.c dcx.h log.h os.h

libgphoto_konica_qmxxx_la_LDFLAGS = -version-info 0:0:0

qmxxx_LDADD = 	@GTK_LIBS@
