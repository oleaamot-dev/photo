#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([gphoto],[0.7.0],[gphoto-devel@gphoto.org])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AR
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace 'main' with a function in '-lgpio':
AC_CHECK_LIB([gpio], [main])
# FIXME: Replace 'main' with a function in '-ljpeg':
AC_CHECK_LIB([jpeg], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h malloc.h netinet/in.h sgtty.h sys/ioctl.h sys/param.h sys/time.h termio.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit dup2 getcwd gettimeofday memmove memset mkdir pow putenv select sqrt strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtol])

AC_PATH_PROG(LIBUSB_CONFIG,libusb-config)
if test -n "${LIBUSB_CONFIG}"; then
  CFLAGS="$CFLAGS `$LIBUSB_CONFIG --cflags`"
  GPHOTO2_CFLAGS="$GPHOTO2_CFLAGS `$LIBUSB_CONFIG --cflags`"
  LIBS="$LIBS `$LIBUSB_CONFIG --libs`"
  GPHOTO2_USB=1
else
  GPHOTO2_USB=0
fi

AM_CONDITIONAL(GPHOTO2_USB, test $GPHOTO2_USB = "1")
AC_SUBST(GPHOTO2_USB)

dnl Local setup
dnl Is there a better way of doing docs?
AC_DEFINE_UNQUOTED(DOCDIR, "$datadir/gphoto/doc", "Documentation")

dnl Or gallery themes?
AC_DEFINE_UNQUOTED(GALLERYDIR, "$datadir/gphoto/gallery", "Gallery Directory")

driversdir='${pkglibdir}'
AC_SUBST(driversdir)

gallerydir='${pkgdatadir}/gallery'
AC_SUBST(gallerydir)

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(GPHOTO,
		  gdk-pixbuf-2.0 >= 2.0.8 \
		  glib-2.0 >= 2.38.0 \
		  gobject-2.0 >= 2.38.0 \
		  gtk+-3.0 >= 3.24.34 \
		  imlib2 >= 1.12.3 \
		  libgphoto2 >= 2.0 \
		  libxml-2.0 >= 2.0 \
		  pangoft2 >= 0.28)

GPHOTO_PKG_DEPS="gdk-pixbuf-2.0 glib-2.0 gobject-2.0 gtk+-3.0 imlib2 libgphoto2 libxml-2.0 pangoft2"

AC_SUBST(GPHOTO_LIBS)
AC_SUBST(GPHOTO_CFLAGS)
AC_SUBST(GPHOTO_PKG_DEPS)
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(GPHOTO,
		  gdk-pixbuf-2.0 >= 2.0.8 \
		  glib-2.0 >= 2.38.0 \
		  gobject-2.0 >= 2.38.0 \
		  gtk+-3.0 >= 3.24.34 \
		  imlib2 >= 1.12.3 \
		  libgphoto2 >= 2.0 \
		  libxml-2.0 >= 2.0 \
		  pangoft2 >= 0.28)

GPHOTO_PKG_DEPS="gdk-pixbuf-2.0 glib-2.0 gobject-2.0 gtk+-3.0 imlib2 libgphoto2 libxml-2.0 pangoft2"

AC_SUBST(GPHOTO_LIBS)
AC_SUBST(GPHOTO_CFLAGS)
AC_SUBST(GPHOTO_PKG_DEPS)

AC_CONFIG_FILES([Makefile
                 barbie/Makefile
                 canon/Makefile
                 casio/Makefile
                 digita/Makefile
                 dimera/Makefile
                 directory/Makefile
                 fuji/Makefile
                 gallery/CSSjimmac/Makefile
                 gallery/CSStheme/Makefile
                 gallery/Default/Makefile
                 gallery/Makefile
                 gallery/RedNGray/Makefile
                 jd11/Makefile
                 kodak/Makefile
                 kodak/dc21x/Makefile
                 kodak_generic/Makefile
                 konica/Makefile
                 konica_qmxxx/Makefile
                 man/Makefile
                 minolta/Makefile
                 mustek/Makefile
                 nikon/Makefile
                 philips/Makefile
                 photopc/Makefile
                 polaroid/Makefile
                 ricoh/Makefile
                 samsung/Makefile
                 scripts/Makefile
                 sony/Makefile
                 sony/dscf55/Makefile
                 src/Makefile
                 src/icons/Makefile])
AC_OUTPUT
