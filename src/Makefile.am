-include $(top_srcdir)/Makefile.am

bin_PROGRAMS = music-player

lib_LTLIBRARIES = \
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

music_player_SOURCES = \
	music-player-window.c \
	music-player-window.h \
	player-status-area.c \
	player-status-area.h \
	player-view.c \
	player-view.h \
	util.c \
	util.h \
	main.c

music_player_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(ID3_FLAGS) \
	$(AM_CFLAGS)

music_player_LDADD = \
	$(GTK3_LIBS) \
	$(GSTREAMER_LIBS) \
	-lid3 \
	libmediadb.la
