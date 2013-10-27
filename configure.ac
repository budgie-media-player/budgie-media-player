AC_INIT([Music Player], 1, [ikey.doherty@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wno-portability])
AC_PROG_CC
LT_PREREQ(2.2)
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
LT_INIT([disable-static])

# Package requirements
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.38.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.38.0])
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.10.0])
# We require GStreamer 1.0
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-1.0 >= 1.0.10])
# Use id3tag to check audio tags.
AC_CHECK_HEADERS([id3.h], [])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

