/*
 * budgie-window.c
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
#include "config.h"

#include <string.h>
#include <gdk/gdkx.h>
#include <gst/video/videooverlay.h>
#include <gst/gstbus.h>

#include "budgie-window.h"
#include "budgie-control-bar.h"

G_DEFINE_TYPE_WITH_PRIVATE(BudgieWindow, budgie_window, G_TYPE_OBJECT);

static void init_styles(BudgieWindow *self);

static void store_media(gpointer data1, gpointer data2);
static gboolean load_media_t(gpointer data);
static gpointer load_media(gpointer data);

/* Callbacks */
static void about_cb(GtkWidget *widget, gpointer userdata);
static void play_cb(GtkWidget *widget, gpointer userdata);
static void pause_cb(GtkWidget *widget, gpointer userdata);
static void next_cb(GtkWidget *widget, gpointer userdata);
static void prev_cb(GtkWidget *widget, gpointer userdata);
static void volume_cb(GtkWidget *widget, gpointer userdata);
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer userdata);
static void realize_cb(GtkWidget *widget, gpointer userdata);
static gboolean refresh_cb(gpointer userdata);
static void reload_cb(GtkWidget *widget, gpointer userdata);
static void full_screen_cb(GtkWidget *widget, gpointer userdata);
static void aspect_cb(GtkWidget *widget, gpointer userdata);
static gboolean motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, gpointer userdata);
static gboolean key_cb(GtkWidget *widget, GdkEventKey *event, gpointer userdata);

static void toolbar_cb(BudgieControlBar *bar, int action, gboolean toggle, gpointer userdata);

/* GStreamer callbacks */
static void _gst_eos_cb(GstBus *bus, GstMessage *msg, gpointer userdata);

/* Initialisation */
static void budgie_window_class_init(BudgieWindowClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_window_dispose;
}

static void budgie_window_init(BudgieWindow *self)
{
        GtkWidget *window;
        GtkWidget *header;
        GtkWidget *video;
        GtkWidget *stack;
        /* header buttons */
        GtkWidget *prev, *play, *pause, *next;
        GtkWidget *volume;
        GtkWidget *search;
        GtkWidget *status;
        GtkWidget *player;
        GtkWidget *toolbar;
        GtkWidget *south_reveal;
        GtkWidget *layout;
        GstBus *bus;
        GtkSettings *settings;
        GdkVisual *visual;
        guint length;

        self->priv = budgie_window_get_instance_private(self);

        /* TODO: Handle this better */
        self->priv->music_directory = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
        if (!self->priv->music_directory)
                g_error("No music directory configured");
        self->priv->video_directory = g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS);
        if (!self->priv->video_directory)
                g_error("No video directory configured");

        init_styles(self);

        /* Initialize our window */
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        self->window = window;
        settings = gtk_widget_get_settings(window);
        visual = gtk_widget_get_visual(window);
        g_object_set(settings,
                "gtk-application-prefer-dark-theme", TRUE, NULL);
        g_object_unref(settings);

        /* Set our window up */
        gtk_window_set_title(GTK_WINDOW(window), "Music Player");
        gtk_widget_set_size_request(window, 1100, 500);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_name(GTK_WINDOW(window), "gnome-music");
        gtk_window_set_wmclass(GTK_WINDOW(window), "Budgie", "Budgie");
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        /* Icon theme for button utility */
        self->icon_theme = gtk_icon_theme_get_default();

        /* Create client side decorations */
        header = gtk_header_bar_new();
        self->header = header;

        /* Always show the close button */
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
        gtk_window_set_titlebar(GTK_WINDOW(window), header);

        /* Media control buttons, placed on headerbar */
        prev = new_button_with_icon(self->icon_theme, "media-seek-backward", FALSE, FALSE);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), prev);
        g_signal_connect(prev, "clicked", G_CALLBACK(prev_cb), (gpointer)self);
        self->prev = prev;
        /* Set some left padding */
        gtk_widget_set_margin_left(prev, 20);

        play = new_button_with_icon(self->icon_theme, "media-playback-start", FALSE, FALSE);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), play);
        g_signal_connect(play, "clicked", G_CALLBACK(play_cb), (gpointer)self);
        self->play = play;

        pause = new_button_with_icon(self->icon_theme, "media-playback-pause", FALSE, FALSE);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), pause);
        g_signal_connect(pause, "clicked", G_CALLBACK(pause_cb), (gpointer)self);
        self->pause = pause;

        next = new_button_with_icon(self->icon_theme, "media-seek-forward", FALSE, FALSE);
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

        /* main layout */
        layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), layout);

        /* Toolbar revealer */
        south_reveal = gtk_revealer_new();
        self->south_reveal = south_reveal;
        gtk_box_pack_end(GTK_BOX(layout), south_reveal, FALSE, FALSE, 0);

        /* toolbar */
        toolbar = budgie_control_bar_new();
        self->toolbar = toolbar;
        g_signal_connect(toolbar, "action-selected",
                G_CALLBACK(toolbar_cb), (gpointer)self);

        gtk_container_add(GTK_CONTAINER(south_reveal), toolbar);

        /* Stack */
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack),
                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        self->stack = stack;
        gtk_box_pack_start(GTK_BOX(layout), stack, TRUE, TRUE, 0);

        /* Player */
        player = player_view_new();
        self->player = player;
        gtk_stack_add_named(GTK_STACK(stack), player, "player");

        /* Video */
        video = gtk_drawing_area_new();
        self->video = video;
        gtk_widget_set_double_buffered(video, FALSE);
        self->priv->window_handle = 0;
        gtk_stack_add_named(GTK_STACK(stack), video, "video");
        gtk_widget_set_visual(video, visual);
        g_signal_connect(video, "realize", G_CALLBACK(realize_cb), (gpointer)self);
        g_signal_connect(video, "draw", G_CALLBACK(draw_cb), (gpointer)self);

        /* Hook up events for video box */
        g_signal_connect(video, "motion-notify-event",
                G_CALLBACK(motion_notify_cb), (gpointer)self);
        g_signal_connect(window, "key-release-event",
                G_CALLBACK(key_cb), (gpointer)self);
        gtk_widget_set_events (video, gtk_widget_get_events (video) |
             GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
             GDK_POINTER_MOTION_HINT_MASK | GDK_KEY_RELEASE_MASK);

        /* search entry */
        search = gtk_search_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(search), "Search...");
        gtk_header_bar_pack_end(GTK_HEADER_BAR(header), search);
        gtk_widget_set_margin_right(search, 10);
        self->search = search;

        /* Initialise our tracks list */
        self->priv->tracks = NULL;

        self->db = budgie_db_new();

        /* Initialise gstreamer */
        self->gst_player = gst_element_factory_make("playbin", "player");
        bus = gst_element_get_bus(self->gst_player);

        gst_bus_enable_sync_message_emission(bus);
        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message::eos", G_CALLBACK(_gst_eos_cb), (gpointer)self);
        g_object_unref(bus);
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        self->priv->duration = GST_CLOCK_TIME_NONE;

        self->priv->tracks = budgie_db_get_all_media(self->db);

        g_timeout_add(1000, refresh_cb, (gpointer)self);

        length = g_slist_length(self->priv->tracks);
        player_view_set_list(PLAYER_VIEW(self->player), self->priv->tracks);
        /* Start thread from idle queue */
        if (length == 0)
                g_idle_add(load_media_t, (gpointer)self);

        gtk_widget_realize(window);
        gtk_widget_show_all(window);
        budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(toolbar), FALSE);
        budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(toolbar), FALSE);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(toolbar),
                BUDGIE_ACTION_PAUSE, FALSE);

        /* Show the toolbar */
        gtk_revealer_set_reveal_child(GTK_REVEALER(south_reveal), TRUE);

        gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Budgie");
        gtk_widget_hide(pause);
}

static void budgie_window_dispose(GObject *object)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(object);

        g_object_unref(self->icon_theme);
        g_object_unref(self->css_provider);
        if (self->priv->tracks)
                g_slist_free_full(self->priv->tracks, free_media_info);

        if (self->priv->uri)
                g_free(self->priv->uri);

        g_object_unref(self->db);

        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        gst_object_unref(self->gst_player);
        /* Destruct */
        G_OBJECT_CLASS(budgie_window_parent_class)->dispose(object);
}

/* Utility; return a new BudgieWindow */
BudgieWindow* budgie_window_new(void)
{
        BudgieWindow *self;

        self = g_object_new(BUDGIE_WINDOW_TYPE, NULL);
        return BUDGIE_WINDOW(self);
}

static void init_styles(BudgieWindow *self)
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

static void about_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);

        const gchar* authors[] = {
                "Ikey Doherty <ikey.doherty@gmail.com>",
                NULL
        };
        const gchar* comments = "Modern, Lightweight and distraction free media experience.";
        const gchar* copyright = "Copyright \u00A9 Ikey Doherty 2013";
        gtk_show_about_dialog(GTK_WINDOW(self->window),
                "authors", authors,
                "comments", comments,
                "copyright", copyright,
                "logo-icon-name", "gnome-music",
                "program-name", PACKAGE_NAME,
                "license-type", GTK_LICENSE_GPL_2_0,
                "version", PACKAGE_VERSION,
                "website", PACKAGE_URL,
                NULL);
}
static void play_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;
        MediaInfo *media = NULL;
        gchar *uri;

        self = BUDGIE_WINDOW(userdata);
        self->priv->duration = GST_CLOCK_TIME_NONE;
        media = player_view_get_current_selection(PLAYER_VIEW(self->player));
        if (!media) /* Revisit */
                return;

        /* Switch to video view for video content */
        if (g_str_has_prefix(media->mime, "video/")) {
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack), "video");
                budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(self->toolbar), TRUE);
        } else {
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack), "player");
                budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(self->toolbar), FALSE);
        }

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
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);

        gst_element_set_state(self->gst_player, GST_STATE_PAUSED);
        gtk_widget_hide(self->pause);
        gtk_widget_show(self->play);
}

static void next_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *next = NULL;
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (!self->priv->random)
                next = player_view_get_next_item(PLAYER_VIEW(self->player));
        else
                next = player_view_get_random_item(PLAYER_VIEW(self->player));
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
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (!self->priv->random)
                prev = player_view_get_previous_item(PLAYER_VIEW(self->player));
        else
                prev = player_view_get_random_item(PLAYER_VIEW(self->player));
        if (!prev) /* Revisit */
                return;

        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        player_view_set_current_selection(PLAYER_VIEW(self->player), prev);
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
}

static void volume_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;
        gdouble volume_level;

        self = BUDGIE_WINDOW(userdata);
        volume_level = gtk_range_get_value(GTK_RANGE(self->volume));
        g_object_set(self->gst_player, "volume", volume_level, NULL);
}

static gboolean refresh_cb(gpointer userdata) {
        BudgieWindow *self;
        gdouble volume_level;
        gint64 track_current;
        GstFormat fmt = GST_FORMAT_TIME;

        self = BUDGIE_WINDOW(userdata);
        g_object_get(self->gst_player, "volume", &volume_level, NULL);

        /* Don't cause events for this, endless volume battle */
        g_signal_handler_block(self->volume, self->priv->volume_id);
        gtk_range_set_value(GTK_RANGE(self->volume), volume_level);

        /* Reenable events */
        g_signal_handler_unblock(self->volume, self->priv->volume_id);

        /* Get media duration */
        if (!GST_CLOCK_TIME_IS_VALID (self->priv->duration)) {
                if (!gst_element_query_duration(self->gst_player, fmt, &self->priv->duration)) {
                        /* Not able to get the clock time, fix when
                         * we have added bus-state */
                        self->priv->duration = GST_CLOCK_TIME_NONE;
                        return TRUE;
                }
        }
        if (!gst_element_query_position(self->gst_player, fmt, &track_current))
                return TRUE;

        player_status_area_set_media_time(PLAYER_STATUS_AREA(self->status),
                self->priv->duration, track_current);
        return TRUE;
}

static void reload_cb(GtkWidget *widget, gpointer userdata)
{
        g_idle_add(load_media_t, userdata);
}

static void full_screen_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;
        gboolean full;

        self = BUDGIE_WINDOW(userdata);
        if (self->priv->full_screen) {
                gtk_window_fullscreen(GTK_WINDOW(self->window));
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), FALSE);
                budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(self->toolbar), TRUE);
        } else {
                gtk_window_unfullscreen(GTK_WINDOW(self->window));
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
                budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(self->toolbar), FALSE);
        }
}

static void aspect_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        g_object_set(self->gst_player, "force-aspect-ratio", self->priv->force_aspect, NULL);
        /* Otherwise we get dirty regions on our drawing area */
        gtk_widget_queue_draw(self->window);
}

/* GStreamer callbacks */
static void _gst_eos_cb(GstBus *bus, GstMessage *msg, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        /* Skip to next track */
        if (!self->priv->repeat) {
                next_cb(NULL, userdata);
                return;
        }
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        /* repeat the same track again */
        play_cb(NULL, (gpointer)self);
}

static void store_media(gpointer data1, gpointer data2)
{
        MediaInfo *info;
        BudgieWindow *self;

        info = (MediaInfo*)data1;
        self = BUDGIE_WINDOW(data2);

        budgie_db_store_media(self->db, info);
}

static gboolean load_media_t(gpointer data)
{
        BudgieWindow *self;
        GThread *thread;

        self = BUDGIE_WINDOW(data);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RELOAD, FALSE);

        thread = g_thread_new("reload-media", &load_media, data);

        return FALSE;
}

static gpointer load_media(gpointer data)
{
        BudgieWindow *self;
        GSList *tracks = NULL;

        self = BUDGIE_WINDOW(data);
        search_directory(self->priv->music_directory,
                &tracks, "audio/");
        search_directory(self->priv->video_directory,
                &tracks, "video/");
        g_slist_foreach(tracks, store_media, (gpointer)self);
        /* Reset tracks */
        if (self->priv->tracks)
                g_slist_free_full (self->priv->tracks, free_media_info);
        g_slist_free_full(tracks, free_media_info);
        /* Use budgiedb's tracks, not our own list */
        self->priv->tracks = budgie_db_get_all_media(self->db);
        player_view_set_list(PLAYER_VIEW(self->player), self->priv->tracks);

        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RELOAD, TRUE);

        return NULL;
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer userdata) {
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        gst_video_overlay_expose(GST_VIDEO_OVERLAY(self->gst_player));
        return FALSE;
}


static void realize_cb(GtkWidget *widg, gpointer userdata)
{
        BudgieWindow *self;
        GdkWindow *window;

        self = BUDGIE_WINDOW(userdata);
        window = gtk_widget_get_window(self->video);
        if (!gdk_window_ensure_native(window))
                g_error("Unable to initialize video");
        self->priv->window_handle = GDK_WINDOW_XID(window);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(self->gst_player), self->priv->window_handle);
}

static gboolean hide_bar(gpointer udata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(udata);

        if (self->priv->full_screen)
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), FALSE);
        else
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        return FALSE;
}

static gboolean motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (gtk_revealer_get_child_revealed(GTK_REVEALER(self->south_reveal)))
                return FALSE;
        if (!self->priv->full_screen)
                return FALSE;

        gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        g_timeout_add(3000, hide_bar, userdata);
        return FALSE;
}

static gboolean key_cb(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (event->keyval != GDK_KEY_Escape)
                return FALSE;

        if (!self->priv->full_screen)
                return FALSE;
        gtk_window_unfullscreen(GTK_WINDOW(self->window));
        gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        self->priv->full_screen = FALSE;
        budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_FULL_SCREEN, FALSE);
        return TRUE;
}

static void toolbar_cb(BudgieControlBar *bar, int action, gboolean toggle, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);

        switch (action) {
                case BUDGIE_ACTION_ABOUT:
                        return about_cb(GTK_WIDGET(bar), userdata);
                case BUDGIE_ACTION_RELOAD:
                        return reload_cb(GTK_WIDGET(bar), userdata);
                case BUDGIE_ACTION_RANDOM:
                        self->priv->random = toggle;
                        break;
                case BUDGIE_ACTION_REPEAT:
                        self->priv->repeat = toggle;
                        break;
                case BUDGIE_ACTION_ASPECT_RATIO:
                        self->priv->force_aspect = toggle;
                        return aspect_cb(GTK_WIDGET(bar), userdata);
                case BUDGIE_ACTION_FULL_SCREEN:
                        self->priv->full_screen = toggle;
                        return full_screen_cb(GTK_WIDGET(bar), userdata);
                default:
                        break;
        }
}
