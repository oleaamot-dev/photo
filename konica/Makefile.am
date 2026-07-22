bin_PROGRAMS = qm100 
drivers_LTLIBRARIES = libgphoto_konica_qm100.la

CFLAGS   = @CFLAGS@ @GNOME_INCLUDEDIR@ @GDK_IMLIB_CFLAGS@ 
qm100_LDFLAGS = 
EXTRA_DIST = konicarc.sample ChangeLog TODO gettraces 

libgphoto_konica_qm100_la_SOURCES = konica_qm100.c getPicInfo.c \
	transmission.c lowlevel.c getStatus.c close.c savePic.c saveThumb.c \
	erasePic.c formatCF.c takePic.c error.c setSpeed.c open.c \
	konica_qm100.h getPicInfo.h \
	transmission.h lowlevel.h getStatus.h close.h savePic.h saveThumb.h \
	erasePic.h formatCF.h takePic.h error.h setSpeed.h open.h defs.h \
	dump.c dump.h qm100.h configDialog.h configDialog.c \
   rcutil.h rcutil.c

qm100_SOURCES= qm100.c qm100.h getPicInfo.c \
	transmission.c lowlevel.c getStatus.c close.c savePic.c saveThumb.c \
	erasePic.c formatCF.c takePic.c error.c setSpeed.c open.c \
	konica_qm100.h getPicInfo.h \
	transmission.h lowlevel.h getStatus.h close.h savePic.h saveThumb.h \
	erasePic.h formatCF.h takePic.h error.h setSpeed.h open.h defs.h \
	dump.c dump.h qm100.h configDialog.h configDialog.c \
   rcutil.h rcutil.c

libgphoto_konica_qm100_la_LDFLAGS = -version-info 0:0:0

qm100_LDADD = 	@GTK_LIBS@
