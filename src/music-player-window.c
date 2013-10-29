/*
 * music-player-window.c
 * 
 * Copyright 2013 Ikey Doherty <ikey.doherty@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
#include "music-player-window.h"
#include "media.h"
#include <string.h>

G_DEFINE_TYPE_WITH_PRIVATE(MusicPlayerWindow, music_player_window, G_TYPE_OBJECT);

/* Utilities */
static GtkWidget* new_button_with_icon(MusicPlayerWindow *self, const gchar *icon_name);
static void init_styles(MusicPlayerWindow *self);

/* Callbacks */
static void play_cb(GtkWidget *widget, gpointer userdata);
static void pause_cb(GtkWidget *widget, gpointer userdata);
static void next_cb(GtkWidget *widget, gpointer userdata);
static void prev_cb(GtkWidget *widget, gpointer userdata);
static void volume_cb(GtkWidget *widget, gpointer userdata);
static gboolean refresh_cb(gpointer userdata);

/* GStreamer callbacks */
static void _gst_eos_cb (GstBus *bus, GstMessage *msg, gpointer userdata);

/* Initialisation */
static void music_player_window_class_init(MusicPlayerWindowClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &music_player_window_dispose;
}

static void music_player_window_init(MusicPlayerWindow *self)
{
        GtkWidget *window;
        GtkWidget *header;
        /* header buttons */
        GtkWidget *prev, *play, *pause, *next;
        GtkWidget *volume;
        GtkWidget *search;
        GtkWidget *status;
        GtkWidget *player;
        GstBus *bus;

        self->priv = music_player_window_get_instance_private(self);

        /* TODO: Handle this better */
        self->priv->music_directory = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
        if (!self->priv->music_directory)
                g_error("No music directory configured");

        init_styles(self);

        /* Initialize our window */
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        self->window = window;

        /* Set our window up */
        gtk_window_set_title(GTK_WINDOW(window), "Music Player");
        gtk_widget_set_size_request(window, 1100, 500);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_name(GTK_WINDOW(window), "gnome-music");
        gtk_window_set_wmclass(GTK_WINDOW(window), "MusicPlayer", "Music Player");
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        /* Icon theme for button utility */
        self->icon_theme = gtk_icon_theme_get_default();

        /* Create client side decorations */
        header = gtk_header_bar_new();
        self->header = header;
        gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Music Player");

        /* Always show the close button */
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
        gtk_window_set_titlebar(GTK_WINDOW(window), header);

        /* Media control buttons, placed on headerbar */
        prev = new_button_with_icon(self, "media-seek-backward");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), prev);
        g_signal_connect(prev, "clicked", G_CALLBACK(prev_cb), (gpointer)self);
        self->prev = prev;
        /* Set some left padding */
        gtk_widget_set_margin_left(prev, 20);

        play = new_button_with_icon(self, "media-playback-start");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), play);
        g_signal_connect(play, "clicked", G_CALLBACK(play_cb), (gpointer)self);
        self->play = play;

        pause = new_button_with_icon(self, "media-playback-pause");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), pause);
        g_signal_connect(pause, "clicked", G_CALLBACK(pause_cb), (gpointer)self);
        self->pause = pause;

        next = new_button_with_icon(self, "media-seek-forward");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), next);
        g_signal_connect(next, "clicked", G_CALLBACK(next_cb), (gpointer)self);
        self->next = next;

        /* volume control */
        volume = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                0.0, 1.0, 1.0/100);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), volume);
        gtk_scale_set_draw_value(GTK_SCALE(volume), FALSE);
        gtk_widget_set_size_request(volume, 100, 10);
        gtk_widget_set_can_focus(volume, FALSE);
        self->priv->volume_id = g_signal_connect(volume, "value-changed",
                G_CALLBACK(volume_cb), (gpointer)self);
        self->volume = volume;

        /* Status area */
        status = player_status_area_new();
        gtk_header_bar_set_custom_title(GTK_HEADER_BAR(header), status);
        self->status = status;

        /* Player */
        player = player_view_new();
        gtk_container_add(GTK_CONTAINER(window), player);
        self->player = player;

        /* search entry */
        search = gtk_search_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(search), "Search...");
        gtk_header_bar_pack_end(GTK_HEADER_BAR(header), search);
        gtk_widget_set_margin_right(search, 10);
        self->search = search;

        /* Initialise our tracks list */
        self->priv->tracks = NULL;

        /* Initialise gstreamer */
        self->gst_player = gst_element_factory_make("playbin", "player");
        bus = gst_element_get_bus(self->gst_player);
        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message::eos", G_CALLBACK(_gst_eos_cb), (gpointer)self);
        g_object_unref(bus);
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        self->priv->duration = GST_CLOCK_TIME_NONE;

        search_directory(self->priv->music_directory, &self->priv->tracks, "audio/");
        player_view_set_list(PLAYER_VIEW(player), self->priv->tracks);
        gtk_widget_show_all(window);
        gtk_widget_hide(pause);

        g_timeout_add(1000, refresh_cb, (gpointer)self);
}

static void music_player_window_dispose(GObject *object)
{
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(object);

        g_object_unref(self->icon_theme);
        g_object_unref(self->css_provider);
        if (self->priv->tracks)
                g_slist_free_full (self->priv->tracks, free_media_info);

        if (self->priv->uri)
                g_free(self->priv->uri);

        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        gst_object_unref(self->gst_player);
        /* Destruct */
        G_OBJECT_CLASS (music_player_window_parent_class)->dispose (object);
}

/* Utility; return a new MusicPlayerWindow */
MusicPlayerWindow* music_player_window_new(void)
{
        MusicPlayerWindow *self;

        self = g_object_new(MUSIC_PLAYER_WINDOW_TYPE, NULL);
        return MUSIC_PLAYER_WINDOW(self);
}

static GtkWidget* new_button_with_icon(MusicPlayerWindow *self, const gchar *icon_name)
{
        GtkWidget *button;
        GtkWidget *image;
        GdkPixbuf *pixbuf;

        /* Load the image */
        pixbuf = gtk_icon_theme_load_icon(self->icon_theme, icon_name,
                48, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
        image = gtk_image_new_from_pixbuf(pixbuf);

        /* Create the button */
        button = gtk_button_new();
        gtk_widget_set_can_focus(button, FALSE);
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_container_add(GTK_CONTAINER(button), image);

        return button;
}

static void init_styles(MusicPlayerWindow *self)
{
        GtkCssProvider *css_provider;
        GdkScreen *screen;
        const gchar *data = PLAYER_CSS;

        css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(css_provider, data, strlen(data), NULL);
        screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_USER);
        self->css_provider = css_provider;
}

static void play_cb(GtkWidget *widget, gpointer userdata)
{
        MusicPlayerWindow *self;
        MediaInfo *media = NULL;
        gchar *uri;

        self = MUSIC_PLAYER_WINDOW(userdata);
        self->priv->duration = GST_CLOCK_TIME_NONE;
        media = player_view_get_current_selection(PLAYER_VIEW(self->player));
        if (!media) /* Revisit */
                return;

        uri = g_filename_to_uri(media->path, NULL, NULL);
        if (g_strcmp0(uri, self->priv->uri) != 0) {
                /* Media change between pausing */
                gst_element_set_state(self->gst_player, GST_STATE_NULL);
        }
        if (self->priv->uri)
                g_free(self->priv->uri);
        self->priv->uri = uri;
        g_object_set(self->gst_player, "uri", self->priv->uri, NULL);
        gst_element_set_state(self->gst_player, GST_STATE_PLAYING);
        player_view_set_current_selection(PLAYER_VIEW(self->player), media);
        gtk_widget_hide(self->play);
        gtk_widget_show(self->pause);

        /* Update status label */
        player_status_area_set_media(PLAYER_STATUS_AREA(self->status), media);
        refresh_cb(self);
}

static void pause_cb(GtkWidget *widget, gpointer userdata)
{
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(userdata);

        gst_element_set_state(self->gst_player, GST_STATE_PAUSED);
        gtk_widget_hide(self->pause);
        gtk_widget_show(self->play);
}

static void next_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *next = NULL;
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(userdata);
        next = player_view_get_next_item(PLAYER_VIEW(self->player));
        if (!next) /* Revisit */
                return;

        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        player_view_set_current_selection(PLAYER_VIEW(self->player), next);
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
}

static void prev_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *prev = NULL;
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(userdata);
        prev = player_view_get_previous_item(PLAYER_VIEW(self->player));
        if (!prev) /* Revisit */
                return;

        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        player_view_set_current_selection(PLAYER_VIEW(self->player), prev);
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
}

static void volume_cb(GtkWidget *widget, gpointer userdata)
{
        MusicPlayerWindow *self;
        gdouble volume_level;

        self = MUSIC_PLAYER_WINDOW(userdata);
        volume_level = gtk_range_get_value(GTK_RANGE(self->volume));
        g_object_set(self->gst_player, "volume", volume_level, NULL);
}

static gboolean refresh_cb(gpointer userdata) {
        MusicPlayerWindow *self;
        gdouble volume_level;
        gint64 track_current;
        GstFormat fmt = GST_FORMAT_TIME;

        self = MUSIC_PLAYER_WINDOW(userdata);
        g_object_get(self->gst_player, "volume", &volume_level, NULL);

        /* Don't cause events for this, endless volume battle */
        g_signal_handler_block(self->volume, self->priv->volume_id);
        gtk_range_set_value(GTK_RANGE(self->volume), volume_level);

        /* Reenable events */
        g_signal_handler_unblock(self->volume, self->priv->volume_id);

        /* Get media duration */
        if (!GST_CLOCK_TIME_IS_VALID (self->priv->duration)) {
                if (!gst_element_query_duration (self->gst_player, fmt, &self->priv->duration)) {
                        /* Not able to get the clock time, fix when
                         * we have added bus-state */
                        self->priv->duration = GST_CLOCK_TIME_NONE;
                        return TRUE;
                }
        }
        if (!gst_element_query_position (self->gst_player, fmt, &track_current))
                return TRUE;

        player_status_area_set_media_time(PLAYER_STATUS_AREA(self->status),
                self->priv->duration, track_current);
        return TRUE;
}

/* GStreamer callbacks */
static void _gst_eos_cb (GstBus *bus, GstMessage *msg, gpointer userdata)
{
        /* Skip to next track */
        next_cb(NULL, userdata);
}
