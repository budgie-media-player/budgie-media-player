-include $(top_srcdir)/common.mk

bin_PROGRAMS = budgie-media-player

noinst_LTLIBRARIES = \
	libbudgiedb.la

libbudgiedb_la_SOURCES = \
	db/budgie-db.h \
	db/budgie-db.c

libbudgiedb_la_CFLAGS = \
	$(GIO_CFLAGS) \
	$(AM_CFLAGS)

libbudgiedb_la_LIBADD = \
	$(GIO_LIBS) \
	-lgdbm

budgie_media_player_SOURCES = \
	budgie-window.c \
	budgie-window.h \
	budgie-control-bar.c \
	budgie-control-bar.h \
	budgie-media-label.c \
	budgie-media-label.h \
	budgie-media-view.c \
	budgie-media-view.h \
	budgie-settings-view.c \
	budgie-settings-view.h \
	budgie-status-area.c \
	budgie-status-area.h \
	util.c \
	util.h \
	common.h \
	main.c

budgie_media_player_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(GSTREAMER_VIDEO_CFLAGS) \
	$(ID3_FLAGS) \
	$(AM_CFLAGS)

budgie_media_player_LDADD = \
	$(GTK3_LIBS) \
	$(GSTREAMER_LIBS) \
	$(GSTREAMER_VIDEO_LIBS) \
	-lid3 \
	libbudgiedb.la
