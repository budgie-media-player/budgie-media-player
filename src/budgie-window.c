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

#include "common.h"
#include "budgie-window.h"
#include "budgie-media-view.h"

/* Private storage */
struct _BudgieWindowPrivate {
        const gchar *current_page;
        GSettings *settings;
        MediaInfo *media;
        gchar *uri;
        guint64 duration;
        gboolean repeat;
        gboolean random;
        gboolean force_aspect;
        gboolean full_screen;
        guintptr window_handle;

        /* Error stuffs */
        GtkWidget *error_revealer;
        GtkWidget *error_label;
};

G_DEFINE_TYPE_WITH_PRIVATE(BudgieWindow, budgie_window, G_TYPE_OBJECT)

/* BudgieWindow prototypes */
static void init_styles(BudgieWindow *self);

static void store_media(gpointer data1, gpointer data2);
static gboolean load_media_t(gpointer data);
static gpointer load_media(gpointer data);

/* Callbacks */
static void play_cb(GtkWidget *widget, gpointer userdata);
static void pause_cb(GtkWidget *widget, gpointer userdata);
static void next_cb(GtkWidget *widget, gpointer userdata);
static void prev_cb(GtkWidget *widget, gpointer userdata);
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer userdata);
static void realize_cb(GtkWidget *widget, gpointer userdata);
static gboolean refresh_cb(gpointer userdata);
static void reload_cb(GtkWidget *widget, gpointer userdata);
static void full_screen_cb(GtkWidget *widget, gpointer userdata);
static void aspect_cb(GtkWidget *widget, gpointer userdata);
static gboolean motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, gpointer userdata);
static gboolean key_cb(GtkWidget *widget, GdkEventKey *event, gpointer userdata);
static void settings_changed(GSettings *settings, gchar *key, gpointer userdata);
static void toolbar_cb(BudgieControlBar *bar, int action, gboolean toggle, gpointer userdata);
static void seek_cb(BudgieStatusArea *status, gint64 value, gpointer userdata);
static void media_selected_cb(BudgieMediaView *view, gpointer info, gpointer userdata);
static void error_dismiss_cb(GtkWidget *widget, gpointer userdata);

/* GStreamer callbacks */
static void _gst_eos_cb(GstBus *bus, GstMessage *msg, gpointer userdata);
static void _gst_error_cb(GstBus *bus, GstMessage *msg, gpointer userdata);

/* Boilerplate GObject code */
static void budgie_window_class_init(BudgieWindowClass *klass);
static void budgie_window_init(BudgieWindow *self);
static void budgie_window_dispose(GObject *object);

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
        GtkWidget *search;
        GtkWidget *status;
        GtkWidget *view;
        GtkWidget *toolbar;
        GtkWidget *south_reveal;
        GtkWidget *layout;
        GtkWidget *settings_view;
        GstBus *bus;
        GList *tracks;
        GdkVisual *visual;
        guint length;
        gchar **media_dirs = NULL;
        const gchar *dirs[3];
        gboolean b_value;
        GtkWidget *overlay;
        GtkWidget *error_frame;
        GtkWidget *error_layout;
        GtkWidget *error_revealer;
        GtkWidget *error_label;
        GtkStyleContext *style;
        GtkWidget *dismiss;

        self->priv = budgie_window_get_instance_private(self);
        /* Init our settings */
        self->priv->settings = g_settings_new(BUDGIE_SCHEMA);
        g_signal_connect(self->priv->settings, "changed",
                G_CALLBACK(settings_changed), self);

        /* Retrieve media directories */
        media_dirs = g_settings_get_strv(self->priv->settings, BUDGIE_MEDIA_DIRS);
        if (g_strv_length(media_dirs) == 0) {
                /* Set defaults */
                g_strfreev(media_dirs);
                dirs[0] = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
                if (!dirs[0]) {
                        g_warning("Music directory not found");
                }
                dirs[1] = g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS);
                if (!dirs[1]) {
                        g_warning("Video directory not found");
                }
                dirs[2] = NULL;
                g_settings_set_strv(self->priv->settings, BUDGIE_MEDIA_DIRS, dirs);
                media_dirs = g_settings_get_strv(self->priv->settings, BUDGIE_MEDIA_DIRS);
        }
        self->media_dirs = media_dirs;
        self->db = budgie_db_new();

        init_styles(self);

        /* Initialize our window */
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        self->window = window;
        visual = gtk_widget_get_visual(window);

        /* Set our window up */
        gtk_window_set_title(GTK_WINDOW(window), "Music Player");
        gtk_window_set_default_size(GTK_WINDOW(window), 950, 550);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_name(GTK_WINDOW(window), "budgie");
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
        prev = new_button_with_icon(self->icon_theme, "media-seek-backward-symbolic",
                FALSE, FALSE, "Previous track");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), prev);
        g_signal_connect(prev, "clicked", G_CALLBACK(prev_cb), self);
        self->prev = prev;
        /* Set some left padding */
        gtk_widget_set_margin_start(prev, 10);

        play = new_button_with_icon(self->icon_theme, "media-playback-start-symbolic",
                FALSE, FALSE, "Play");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), play);
        g_signal_connect(play, "clicked", G_CALLBACK(play_cb), self);
        self->play = play;

        pause = new_button_with_icon(self->icon_theme, "media-playback-pause-symbolic",
                FALSE, FALSE, "Pause");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), pause);
        g_signal_connect(pause, "clicked", G_CALLBACK(pause_cb), self);
        self->pause = pause;

        next = new_button_with_icon(self->icon_theme, "media-seek-forward-symbolic",
                FALSE, FALSE, "Next track");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), next);
        g_signal_connect(next, "clicked", G_CALLBACK(next_cb), self);
        self->next = next;

        /* Status area */
        status = budgie_status_area_new();
        gtk_header_bar_set_custom_title(GTK_HEADER_BAR(header), status);
        g_signal_connect(status, "seek", G_CALLBACK(seek_cb), self);
        self->status = status;

        /* main layout */
        layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        overlay = gtk_overlay_new();
        gtk_container_add(GTK_CONTAINER(overlay), layout);
        gtk_container_add(GTK_CONTAINER(window), overlay);

        /* We report errors using this. Epics. */
        error_revealer = gtk_revealer_new();
        self->priv->error_revealer = error_revealer;
        error_label = gtk_label_new("");
        self->priv->error_label = error_label;
        error_frame = gtk_event_box_new();
        error_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_set_border_width(GTK_CONTAINER(error_layout), 5);
        gtk_container_add(GTK_CONTAINER(error_revealer), error_frame);
        gtk_container_add(GTK_CONTAINER(error_frame), error_layout);
        gtk_box_pack_start(GTK_BOX(error_layout), error_label, TRUE, TRUE, 5);

        /* Dismiss the error. Coz ya can. */
        dismiss = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
        g_signal_connect(G_OBJECT(dismiss), "clicked", G_CALLBACK(error_dismiss_cb), self);
        gtk_button_set_relief(GTK_BUTTON(dismiss), GTK_RELIEF_NONE);
        style = gtk_widget_get_style_context(dismiss);
        gtk_style_context_add_class(style, "image-button");
        gtk_box_pack_end(GTK_BOX(error_layout), dismiss, FALSE, FALSE, 5);

        style = gtk_widget_get_style_context(error_frame);
        gtk_style_context_add_class(style, "app-notification");
        gtk_style_context_add_class(style, "error");
        gtk_widget_set_valign(error_revealer, GTK_ALIGN_START);
        gtk_widget_set_halign(error_revealer, GTK_ALIGN_CENTER);
        gtk_revealer_set_reveal_child(GTK_REVEALER(error_revealer), FALSE);
        gtk_overlay_add_overlay(GTK_OVERLAY(overlay), error_revealer);

        /* Toolbar revealer */
        south_reveal = gtk_revealer_new();
        self->south_reveal = south_reveal;
        gtk_box_pack_end(GTK_BOX(layout), south_reveal, FALSE, FALSE, 0);

        /* toolbar */
        toolbar = budgie_control_bar_new();
        self->toolbar = toolbar;
        g_signal_connect(toolbar, "action-selected",
                G_CALLBACK(toolbar_cb), self);

        /* Initialise repeat setting */
        b_value = g_settings_get_boolean(self->priv->settings, BUDGIE_REPEAT);
        budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_REPEAT, b_value);
        self->priv->repeat = b_value;

        /* Initialise random setting */
        b_value = g_settings_get_boolean(self->priv->settings, BUDGIE_RANDOM);
        budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RANDOM, b_value);
        self->priv->random = b_value;

        gtk_container_add(GTK_CONTAINER(south_reveal), toolbar);

        /* Stack */
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack),
                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        self->stack = stack;
        gtk_box_pack_start(GTK_BOX(layout), stack, TRUE, TRUE, 0);

        self->priv->current_page = "view";

        /* Browse view */
        view = budgie_media_view_new(NULL);
        g_signal_connect(view, "media-selected",
                G_CALLBACK(media_selected_cb), self);
        self->view = view;
        gtk_stack_add_named(GTK_STACK(stack), view, "view");

        /* Video */
        video = gtk_drawing_area_new();
        self->video = video;
        self->priv->window_handle = 0;
        gtk_stack_add_named(GTK_STACK(stack), video, "video");
        gtk_widget_set_visual(video, visual);
        g_signal_connect(video, "realize", G_CALLBACK(realize_cb), self);
        g_signal_connect(video, "draw", G_CALLBACK(draw_cb), self);

        /* Hook up events for video box */
        g_signal_connect(video, "motion-notify-event",
                G_CALLBACK(motion_notify_cb), self);
        g_signal_connect(window, "key-release-event",
                G_CALLBACK(key_cb), self);
        gtk_widget_set_events (video, gtk_widget_get_events (video) |
             GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
             GDK_POINTER_MOTION_HINT_MASK | GDK_KEY_RELEASE_MASK);

        /* Settings view */
        settings_view = budgie_settings_view_new();
        gtk_stack_add_named(GTK_STACK(stack), settings_view, "settings");
        self->settings = settings_view;

        /* search button. */
        search = gtk_button_new_from_icon_name("edit-find-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_button_set_relief(GTK_BUTTON(search), GTK_RELIEF_NONE);
        /* currently disabled. on account of its not implemeneted.
         * gtk_header_bar_pack_end(GTK_HEADER_BAR(header), search);*/
        gtk_widget_set_margin_end(search, 10);
        self->search = search;

        /* Initialise gstreamer */
        self->gst_player = gst_element_factory_make("playbin", "player");
        bus = gst_element_get_bus(self->gst_player);

        gst_bus_enable_sync_message_emission(bus);
        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message::eos", G_CALLBACK(_gst_eos_cb), self);
        g_signal_connect(bus, "message::error", G_CALLBACK(_gst_error_cb), self);
        g_object_unref(bus);
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        self->priv->duration = GST_CLOCK_TIME_NONE;

        g_timeout_add(1000, refresh_cb, self);

        tracks = budgie_db_get_all_media(self->db);
        length = g_list_length(tracks);
        g_list_free_full(tracks, free_media_info);
        /* Start thread from idle queue */
        if (length == 0) {
                g_idle_add(load_media_t, self);
        } else {
                g_object_set(view, "database", self->db, NULL);
        }

        gtk_widget_realize(window);
        gtk_widget_show_all(window);
        gtk_widget_hide(self->priv->error_revealer);
        budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(toolbar), FALSE);
        budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(toolbar), FALSE);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(toolbar),
                BUDGIE_ACTION_PAUSE, FALSE);
        budgie_status_area_set_media_time(BUDGIE_STATUS_AREA(self->status), -1, -1);

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

        if (self->priv->uri) {
                g_free(self->priv->uri);
                self->priv->uri = NULL;
        }

        g_strfreev(self->media_dirs);
        g_object_unref(self->priv->settings);
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
        gtk_css_provider_load_from_data(css_provider, data, (gssize)strlen(data)+1, NULL);
        screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        self->css_provider = css_provider;
}

static void play_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;
        gchar *uri;
        const gchar *next_child;
        MediaInfo *media;

        self = BUDGIE_WINDOW(userdata);
        media = self->priv->media;
        self->priv->duration = GST_CLOCK_TIME_NONE;
        if (!media) {
                /* Revisit */
                return;
        }

        /* Dismiss existing errors */
        error_dismiss_cb(NULL, userdata);

        self->priv->current_page = gtk_stack_get_visible_child_name(GTK_STACK(self->stack));

        /* Switch to video view for video content */
        if (g_str_has_prefix(media->mime, "video/")) {
                next_child = "video";
                if (!self->video_realized) {
                        gtk_widget_realize(self->video);
                }
                budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(self->toolbar), TRUE);
        } else {
                next_child = "view";
                budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(self->toolbar), FALSE);
                self->priv->full_screen = FALSE;
                full_screen_cb(widget, userdata);
        }
        if (!g_str_equal(self->priv->current_page, "settings")) {
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack), next_child);
        }
        self->priv->current_page = next_child;

        uri = g_filename_to_uri(media->path, NULL, NULL);
        if (g_strcmp0(uri, self->priv->uri) != 0) {
                /* Media change between pausing */
                gst_element_set_state(self->gst_player, GST_STATE_NULL);
        }
        if (self->priv->uri) {
                g_free(self->priv->uri);
        }
        self->priv->uri = uri;
        g_object_set(self->gst_player, "uri", self->priv->uri, NULL);
        gst_element_set_state(self->gst_player, GST_STATE_PLAYING);

        /* Update media controls */
        gtk_widget_hide(self->play);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PLAY, FALSE);
        gtk_widget_show(self->pause);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PAUSE, TRUE);

        /* Update status label */
        budgie_status_area_set_media(BUDGIE_STATUS_AREA(self->status), media);
        budgie_media_view_set_active(BUDGIE_MEDIA_VIEW(self->view), media);
        refresh_cb(self);
}

static void pause_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);

        gst_element_set_state(self->gst_player, GST_STATE_PAUSED);
        gtk_widget_hide(self->pause);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PAUSE, FALSE);
        gtk_widget_show(self->play);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PLAY, TRUE);
}

static void next_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *next = NULL;
        BudgieWindow *self;
        BudgieMediaSelection mode;

        self = BUDGIE_WINDOW(userdata);
        mode = self->priv->random ?
                MEDIA_SELECTION_RANDOM : MEDIA_SELECTION_NEXT;
        next = budgie_media_view_get_info(BUDGIE_MEDIA_VIEW(self->view),
                mode);
        if (!next) {
                /* Revisit */
                return;
        }
        self->priv->media = next;
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
}

static void prev_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *prev = NULL;
        BudgieWindow *self;
        BudgieMediaSelection mode;

        self = BUDGIE_WINDOW(userdata);
        mode = self->priv->random ?
                MEDIA_SELECTION_RANDOM : MEDIA_SELECTION_PREVIOUS;
        prev = budgie_media_view_get_info(BUDGIE_MEDIA_VIEW(self->view),
                mode);
        if (!prev) {
                /* Revisit */
                return;
        }
        self->priv->media = prev;
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
}

static gboolean refresh_cb(gpointer userdata) {
        BudgieWindow *self;
        gint64 track_current;
        GstFormat fmt = GST_FORMAT_TIME;

        self = BUDGIE_WINDOW(userdata);

        /* Get media duration */
        if (!GST_CLOCK_TIME_IS_VALID (self->priv->duration)) {
                if (!gst_element_query_duration(self->gst_player, fmt, (gint64*)&self->priv->duration)) {
                        /* Not able to get the clock time, fix when
                         * we have added bus-state */
                        self->priv->duration = GST_CLOCK_TIME_NONE;
                        return TRUE;
                }
        }
        if (!gst_element_query_position(self->gst_player, fmt, &track_current)) {
                return TRUE;
        }

        budgie_status_area_set_media_time(BUDGIE_STATUS_AREA(self->status),
                (gint64)self->priv->duration, track_current);
        return TRUE;
}

static void reload_cb(GtkWidget *widget, gpointer userdata)
{
        g_idle_add(load_media_t, userdata);
}

static void full_screen_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (self->priv->full_screen) {
                gtk_window_fullscreen(GTK_WINDOW(self->window));
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), FALSE);
                budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(self->toolbar), TRUE);
                budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                        BUDGIE_ACTION_SETTINGS, FALSE);
        } else {
                gtk_window_unfullscreen(GTK_WINDOW(self->window));
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
                budgie_control_bar_set_show_playback(BUDGIE_CONTROL_BAR(self->toolbar), FALSE);
                budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                        BUDGIE_ACTION_SETTINGS, TRUE);
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
        play_cb(NULL, self);
}

static void _gst_error_cb(GstBus *bus, GstMessage *msg, gpointer userdata)
{
        BudgieWindow *self;
        GError *error = NULL;
        gchar *debug_info = NULL;
        gchar *label_msg = NULL;

        self = BUDGIE_WINDOW(userdata);

        gst_message_parse_error(msg, &error, &debug_info);

        label_msg = g_strdup_printf("Encountered the following error:\n%s", error->message);
        gtk_label_set_markup(GTK_LABEL(self->priv->error_label), label_msg);
        gtk_widget_show(self->priv->error_revealer);
        gtk_revealer_set_reveal_child(GTK_REVEALER(self->priv->error_revealer), TRUE);
        g_message("GStreamer issue: %s", error->message);
        g_message("GStreamer debug info: %s", debug_info);

        g_error_free(error);
        g_free(label_msg);
        if (debug_info) {
                g_free(debug_info);
        }

        /* Stop everything */
        gst_element_set_state(self->gst_player, GST_STATE_NULL);
        gtk_widget_hide(self->pause);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PAUSE, FALSE);
        gtk_widget_show(self->play);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_PLAY, TRUE);

        /* Reset playing info */
        budgie_status_area_set_media_time(BUDGIE_STATUS_AREA(self->status),
                -1, -1);
        budgie_status_area_set_media(BUDGIE_STATUS_AREA(self->status), NULL);
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
        __attribute__((unused)) GThread *thread;

        self = BUDGIE_WINDOW(data);
        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RELOAD, FALSE);

        thread = g_thread_new("reload-media", &load_media, data);

        return FALSE;
}

static gpointer load_media(gpointer data)
{
        BudgieWindow *self;
        GList *tracks = NULL;
        guint length, i;
        const gchar *mimes[2];

        self = BUDGIE_WINDOW(data);
        if (self->media_dirs) {
                g_strfreev(self->media_dirs);
                self->media_dirs = g_settings_get_strv(self->priv->settings, BUDGIE_MEDIA_DIRS);
        }

        length = g_strv_length(self->media_dirs);
        mimes[0] = "audio/";
        mimes[1] = "video/";
        for (i=0; i < length; i++)
                search_directory(self->media_dirs[i], &tracks, 2, mimes);

        g_list_foreach(tracks, store_media, self);
        g_list_free_full(tracks, free_media_info);

        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RELOAD, TRUE);

        g_object_set(BUDGIE_MEDIA_VIEW(self->view), "database", self->db, NULL);

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
        if (!gdk_window_ensure_native(window)) {
                g_error("Unable to initialize video");
        }
        self->priv->window_handle = GDK_WINDOW_XID(window);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(self->gst_player), self->priv->window_handle);
        self->video_realized = TRUE;
}

static gboolean hide_bar(gpointer udata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(udata);

        if (self->priv->full_screen) {
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), FALSE);
        } else {
                gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        }
        return FALSE;
}

static gboolean motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (gtk_revealer_get_child_revealed(GTK_REVEALER(self->south_reveal))) {
                return FALSE;
        }
        if (!self->priv->full_screen) {
                return FALSE;
        }

        gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        g_timeout_add(3000, hide_bar, userdata);
        return FALSE;
}

static gboolean key_cb(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        if (event->keyval != GDK_KEY_Escape) {
                return FALSE;
        }

        if (!self->priv->full_screen) {
                return FALSE;
        }
        gtk_window_unfullscreen(GTK_WINDOW(self->window));
        gtk_revealer_set_reveal_child(GTK_REVEALER(self->south_reveal), TRUE);
        self->priv->full_screen = FALSE;
        budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_FULL_SCREEN, FALSE);
        return TRUE;
}

static void settings_changed(GSettings *settings, gchar *key, gpointer userdata)
{
        BudgieWindow *self;
        gboolean bool_value;

        self = BUDGIE_WINDOW(userdata);

        /* Test keys. */
        if (g_str_equal(key, BUDGIE_RANDOM)) {
                bool_value = g_settings_get_boolean(self->priv->settings, BUDGIE_RANDOM);
                budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                        BUDGIE_ACTION_RANDOM, bool_value);
                self->priv->random = bool_value;
        } else if (g_str_equal(key, BUDGIE_REPEAT)) {
                bool_value = g_settings_get_boolean(self->priv->settings, BUDGIE_REPEAT);
                budgie_control_bar_set_action_state(BUDGIE_CONTROL_BAR(self->toolbar),
                        BUDGIE_ACTION_REPEAT, bool_value);
                self->priv->repeat = bool_value;
        }
}
static void toolbar_cb(BudgieControlBar *bar, int action, gboolean toggle, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);

        switch (action) {
                case BUDGIE_ACTION_RELOAD:
                        reload_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_RANDOM:
                        self->priv->random = toggle;
                        g_settings_set_boolean(self->priv->settings,
                                BUDGIE_RANDOM, toggle);
                        break;
                case BUDGIE_ACTION_REPEAT:
                        self->priv->repeat = toggle;
                        g_settings_set_boolean(self->priv->settings,
                                BUDGIE_REPEAT, toggle);
                        break;
                case BUDGIE_ACTION_ASPECT_RATIO:
                        self->priv->force_aspect = toggle;
                        aspect_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_FULL_SCREEN:
                        self->priv->full_screen = toggle;
                        full_screen_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_PLAY:
                        play_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_PAUSE:
                        pause_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_PREVIOUS:
                        prev_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_NEXT:
                        next_cb(GTK_WIDGET(bar), userdata);
                        break;
                case BUDGIE_ACTION_SETTINGS:
                        if (toggle) {
                                gtk_stack_set_visible_child_name(GTK_STACK(self->stack), "settings");
                        } else {
                                gtk_stack_set_visible_child_name(GTK_STACK(self->stack), self->priv->current_page);
                        }
                        break;
                default:
                        break;
        }
}

static void seek_cb(BudgieStatusArea *status, gint64 value, gpointer userdata)
{
        BudgieWindow *self;
        GstSeekFlags flags;

        self = BUDGIE_WINDOW(userdata);
        flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;

        gst_element_seek_simple(GST_ELEMENT(self->gst_player), GST_FORMAT_TIME,
                flags, value);
}

static void media_selected_cb(BudgieMediaView *view, gpointer info, gpointer userdata)
{
        BudgieWindow *self;
        MediaInfo *media;

        self = BUDGIE_WINDOW(userdata);
        media = (MediaInfo*)info;
        self->priv->media = media;
        play_cb(NULL, userdata);
}

static void error_dismiss_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        gtk_widget_hide(self->priv->error_revealer);
        gtk_revealer_set_reveal_child(GTK_REVEALER(self->priv->error_revealer), FALSE);
}
