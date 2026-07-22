## Process this file with automake to generate a GNU Makefile

SUBDIRS = po barbie canon casio digita dimera directory fuji gallery jd11 kodak \
kodak_generic konica konica_qmxxx man minolta mustek nikon \
philips photopc polaroid ricoh samsung sony src scripts

gphotodocdir = $(datadir)/gphoto/doc

gphotodoc_DATA = AUTHORS MANUAL

EXTRA_DIST = m4/ChangeLog  $(gphotodoc_DATA) NEWS README THEMES TODO COPYING THANKS \
	ChangeLog FAQ INSTALL PROGRAMMERS gphoto.spec configure \
	gphoto.desktop

sysdir = $(datadir)/gnome/apps/Graphics
sys_DATA = gphoto.desktop

ACLOCAL_AMFLAGS = -I m4
