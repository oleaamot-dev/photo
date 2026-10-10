dnl Process this file with autoconf to produce a configure script.
dnl
dnl gPhoto 0.5.0-pre - the GNU Digital Camera Program
dnl Copyright (C) 1998-2026 Ole Kristian Aamot and the gPhoto authors

AC_PREREQ([2.69])
AC_INIT([gphoto], [0.5.0-pre], [ole.aamot@aamotinnovation.com], [gphoto],
        [http://www.gphoto.org/])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip subdir-objects -Wall])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

dnl --------------------------------------------------------------------
dnl Camera backend: libgphoto2 (the camera library split out of gPhoto)
dnl --------------------------------------------------------------------
LIBGPHOTO2_REQUIRED=2.5.0
PKG_CHECK_MODULES([LIBGPHOTO2], [libgphoto2 >= $LIBGPHOTO2_REQUIRED libgphoto2_port])

dnl --------------------------------------------------------------------
dnl GUI toolkit (optional: --disable-gui builds the command-line client)
dnl --------------------------------------------------------------------
AC_ARG_ENABLE([gui],
  [AS_HELP_STRING([--disable-gui], [build without the GTK+ user interface])],
  [enable_gui=$enableval], [enable_gui=yes])

GTK_REQUIRED=3.22.0
if test "x$enable_gui" = "xyes"; then
  PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= $GTK_REQUIRED])
  AC_DEFINE([HAVE_GUI], [1], [Define to build the GTK+ user interface])
fi
AM_CONDITIONAL([HAVE_GUI], [test "x$enable_gui" = "xyes"])

dnl gdk-pixbuf makes index thumbnails for cameras that have none,
dnl so it is needed by the command-line client too.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.68 gio-2.0 gdk-pixbuf-2.0 >= 2.36])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  doc/Makefile
  data/Makefile
])
AC_OUTPUT

echo "
  gPhoto $VERSION

  Prefix ............ : ${prefix}
  Camera library .... : libgphoto2 `$PKG_CONFIG --modversion libgphoto2`
  GTK+ interface .... : ${enable_gui}
"
