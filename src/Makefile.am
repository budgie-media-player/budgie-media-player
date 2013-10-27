-include $(top_srcdir)/Makefile.am

bin_PROGRAMS = music-player

music_player_SOURCES = \
	music-player-window.c \
	music-player-window.h \
	player-status-area.c \
	player-status-area.h \
	main.c

music_player_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(AM_CFLAGS)

music_player_LDADD = \
	$(GTK3_LIBS) \
	$(GSTREAMER_LIBS)
