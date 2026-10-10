bin_PROGRAMS = gphoto

gphoto_SOURCES = \
	main.c \
	gphoto.h \
	camera.c \
	camera.h \
	cli.c \
	cli.h \
	html.c \
	html.h

gphoto_CPPFLAGS = \
	-DDATADIR=\""$(datadir)"\" \
	$(GLIB_CFLAGS) \
	$(LIBGPHOTO2_CFLAGS)

gphoto_CFLAGS = -Wall -Wextra -Wno-unused-parameter

gphoto_LDADD = \
	$(GLIB_LIBS) \
	$(LIBGPHOTO2_LIBS)

if HAVE_GUI
gphoto_SOURCES += \
	gui.c \
	gui.h \
	viewer.c \
	viewer.h \
	config-dialog.c \
	config-dialog.h
gphoto_CPPFLAGS += $(GTK_CFLAGS)
gphoto_LDADD += $(GTK_LIBS)
endif
