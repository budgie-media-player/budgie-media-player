
bin_PROGRAMS = budgie

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

budgie_SOURCES = \
	budgie-window.c \
	budgie-window.h \
	budgie-control-bar.c \
	budgie-control-bar.h \
	budgie-settings-view.c \
	budgie-settings-view.h \
	player-status-area.c \
	player-status-area.h \
	player-view.c \
	player-view.h \
	util.c \
	util.h \
	common.h \
	main.c
if TESTING
budgie_SOURCES += \
	budgie-media-view.c \
	budgie-media-view.h
endif

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
	libbudgiedb.la
