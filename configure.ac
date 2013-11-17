AC_INIT([Budgie], 2, [ikey.doherty@gmail.com], [budgie], [http://ikeydoherty.github.io/budgie/])
AM_INIT_AUTOMAKE([-Wno-portability no-dist-gzip dist-xz foreign])
AC_PROG_CC
AC_PROG_CC_STDC
LT_PREREQ(2.2)
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

# Package requirements
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.38.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.38.0])
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.10.0])
# We require GStreamer 1.0
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-1.0 >= 1.0.10])
PKG_CHECK_MODULES([GSTREAMER_VIDEO], [gstreamer-video-1.0 >= 1.0.10])

# Use id3tag to check audio tags.
AC_CHECK_HEADERS([id3.h], [music_player_found_id3_headers=yes; break;])
AS_IF([test "x$music_player_found_id3_headers" != "xyes"],
	[AC_MSG_ERROR([Unable to find the id3 headers])])

# This is so I can make changes that won't show up in 3rd party builds
AC_ARG_ENABLE(testing, AS_HELP_STRING([--enable-testing], [enable testing features]))
AS_IF([test "x$enable_testing" = "xyes"],
        [AC_DEFINE([TESTING], [1], [Building testing features])],
        [])
AM_CONDITIONAL([TESTING], [test x$enable_testing = x"yes"])

AC_CHECK_HEADERS([gdbm.h], [], [AC_MSG_ERROR([Unable to find gdbm headers])])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile src/Makefile data/Makefile)
AC_OUTPUT

