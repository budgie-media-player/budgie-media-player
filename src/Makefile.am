-include $(top_srcdir)/Makefile.am

bin_PROGRAMS = budgie

noinst_LTLIBRARIES = \
	libmediadb.la

libmediadb_la_SOURCES = \
	db/media-db.h \
	db/media-db.c

libmediadb_la_CFLAGS = \
	$(GIO_CFLAGS) \
	$(AM_CFLAGS)

libmediadb_la_LIBADD = \
	$(GIO_LIBS) \
	-lgdbm

budgie_SOURCES = \
	budgie-window.c \
	budgie-window.h \
	player-status-area.c \
	player-status-area.h \
	player-view.c \
	player-view.h \
	util.c \
	util.h \
	main.c

budgie_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(GSTREAMER_VIDEO_CFLAGS) \
	$(ID3_FLAGS) \
	$(AM_CFLAGS)

budgie_LDADD = \
	$(GTK3_LIBS) \
	$(GSTREAMER_LIBS) \
	$(GSTREAMER_VIDEO_LIBS) \
	-lid3 \
	libmediadb.la
