AC_INIT([Budgie], 2, [ikey.doherty@gmail.com], [budgie], [http://ikeydoherty.github.io/budgie/])
AM_INIT_AUTOMAKE([-Wno-portability no-dist-gzip dist-xz foreign subdir-objects])
AC_PROG_CC
AC_PROG_CC_STDC
LT_PREREQ(2.2)
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

GLIB_GSETTINGS

# Package requirements
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.40.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.40.0])
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.12.0])
# We require GStreamer 1.0
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-1.0 >= 1.0.10])
PKG_CHECK_MODULES([GSTREAMER_VIDEO], [gstreamer-video-1.0 >= 1.0.10])

# Taglib for .. tags
PKG_CHECK_MODULES([TAGLIB], [taglib_c >= 1.9.1])


# Sqlite for database
PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.8])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile src/Makefile data/Makefile)
AC_OUTPUT

