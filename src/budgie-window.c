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
#include <dlfcn.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#include <GL/glx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#include <windows.h>
#include <GL/gl.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <EGL/egl.h>
#endif
#include <mpv/client.h>
#include <mpv/render_gl.h>

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
        gboolean switching_tracks;  /* Flag to prevent recursive track switching */
        guintptr window_handle;

        /* MPV rendering context */
        mpv_render_context *mpv_gl;
        gboolean using_opengl;  /* Track rendering method */
        
        /* Video frame buffer for performance */
        unsigned char *frame_buffer;
        size_t frame_buffer_size;
        int frame_width;
        int frame_height;
        
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
static gboolean update_media_view(gpointer data);

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

/* MPV callbacks */
static void wakeup_cb(void *ctx);
static void *get_proc_address(void *ctx, const char *name);

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
        self->priv->switching_tracks = FALSE;

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

        /* Initialise MPV */
        self->mpv_player = mpv_create();
        if (!self->mpv_player) {
                g_error("Failed to create MPV instance");
        }
        
        /* Set some MPV options for embedded rendering - cross-platform */
        mpv_set_option_string(self->mpv_player, "vo", "libmpv");  /* Use libmpv for embedded rendering */
        mpv_set_option_string(self->mpv_player, "hwdec", "auto");
        mpv_set_option_string(self->mpv_player, "keep-open", "yes");
        
        /* Platform-specific optimizations */
#if defined(GDK_WINDOWING_QUARTZ)
        /* macOS optimizations */
        mpv_set_option_string(self->mpv_player, "video-sync", "audio");  /* Better for macOS */
        mpv_set_option_string(self->mpv_player, "vd-lavc-threads", "4");
#elif defined(GDK_WINDOWING_X11)
        /* Linux X11 optimizations */
        mpv_set_option_string(self->mpv_player, "video-sync", "display-resample");
        mpv_set_option_string(self->mpv_player, "x11-bypass-compositor", "yes");
#elif defined(GDK_WINDOWING_WAYLAND)
        /* Linux Wayland optimizations */
        mpv_set_option_string(self->mpv_player, "video-sync", "display-resample");
#elif defined(GDK_WINDOWING_WIN32)
        /* Windows optimizations */
        mpv_set_option_string(self->mpv_player, "video-sync", "display-resample");
        mpv_set_option_string(self->mpv_player, "d3d11-adapter", "auto");
#endif
        
        mpv_set_option_string(self->mpv_player, "interpolation", "no");  /* Disable frame interpolation for performance */
        
        /* Disable album art display in separate window */
        mpv_set_option_string(self->mpv_player, "audio-display", "no");
        mpv_set_option_string(self->mpv_player, "force-window", "no");  /* Never create separate window */
        
        /* Set wakeup callback for events */
        mpv_set_wakeup_callback(self->mpv_player, wakeup_cb, self);
        
        /* Initialize MPV */
        if (mpv_initialize(self->mpv_player) < 0) {
                g_error("Failed to initialize MPV");
        }
        
        self->priv->duration = 0;

        g_timeout_add(1000, refresh_cb, self);

        tracks = budgie_db_get_all_media(self->db);
        length = g_list_length(tracks);
        g_print("Initial database check: found %d existing tracks\n", length);
        g_list_free_full(tracks, free_media_info);
        /* Start thread from idle queue */
        if (length == 0) {
                g_print("No existing tracks, starting media scan\n");
                g_idle_add(load_media_t, self);
        } else {
                g_print("Found existing tracks, setting database on view\n");
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

        if (self->icon_theme) {
                g_object_unref(self->icon_theme);
                self->icon_theme = NULL;
        }
        if (self->css_provider) {
                g_object_unref(self->css_provider);
                self->css_provider = NULL;
        }

        if (self->priv->uri) {
                g_free(self->priv->uri);
                self->priv->uri = NULL;
        }

        if (self->media_dirs) {
                g_strfreev(self->media_dirs);
                self->media_dirs = NULL;
        }
        if (self->priv->settings) {
                g_object_unref(self->priv->settings);
                self->priv->settings = NULL;
        }
        if (self->db) {
                g_object_unref(self->db);
                self->db = NULL;
        }

        /* Clean up MPV */
        if (self->mpv_player) {
                g_print("Cleaning up MPV player...\n");
                
                /* First disable the wakeup callback to prevent further events */
                mpv_set_wakeup_callback(self->mpv_player, NULL, NULL);
                
                /* Stop playback */
                const char *cmd[] = {"stop", NULL};
                mpv_command(self->mpv_player, cmd);
                
                /* Clean up render context first */
                if (self->priv->mpv_gl) {
                        mpv_render_context_free(self->priv->mpv_gl);
                        self->priv->mpv_gl = NULL;
                }
                
                /* Now terminate and destroy mpv */
                mpv_terminate_destroy(self->mpv_player);
                self->mpv_player = NULL;
                
                g_print("MPV cleanup completed\n");
        } else if (self->priv->mpv_gl) {
                /* Clean up render context if mpv_player is already NULL */
                mpv_render_context_free(self->priv->mpv_gl);
                self->priv->mpv_gl = NULL;
        }

        /* Clean up frame buffer */
        if (self->priv->frame_buffer) {
                g_free(self->priv->frame_buffer);
                self->priv->frame_buffer = NULL;
                self->priv->frame_buffer_size = 0;
        }
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
        
        if (!self || !self->mpv_player) {
                g_warning("play_cb: Invalid player or self");
                return;
        }
        
        media = self->priv->media;
        self->priv->duration = 0;
        if (!media) {
                g_warning("play_cb: No media to play");
                return;
        }
        
        if (!media->path || !g_file_test(media->path, G_FILE_TEST_EXISTS)) {
                g_warning("play_cb: Media file does not exist: %s", media->path ? media->path : "(null)");
                return;
        }
        
        g_print("play_cb: Playing file: %s\n", media->path);

        /* Dismiss existing errors */
        error_dismiss_cb(NULL, userdata);

        self->priv->current_page = gtk_stack_get_visible_child_name(GTK_STACK(self->stack));

        /* Switch to video view for video content */
        if (g_str_has_prefix(media->mime, "video/") || 
            g_str_has_prefix(media->mime, "org.matroska.mkv") ||
            g_str_has_prefix(media->mime, "com.apple.quicktime-movie")) {
                next_child = "video";
                if (!self->video_realized) {
                        gtk_widget_realize(self->video);
                }
                budgie_control_bar_set_show_video(BUDGIE_CONTROL_BAR(self->toolbar), TRUE);
                
                /* Keep force-window disabled to prevent separate windows */
                g_print("Playing video file in embedded mode\n");
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
                const char *cmd[] = {"stop", NULL};
                int result = mpv_command(self->mpv_player, cmd);
                if (result < 0) {
                        g_warning("play_cb: mpv stop command failed: %s", mpv_error_string(result));
                }
        }
        if (self->priv->uri) {
                g_free(self->priv->uri);
        }
        self->priv->uri = uri;
        
        /* Load file and play */
        const char *cmd[] = {"loadfile", media->path, NULL};
        int result = mpv_command(self->mpv_player, cmd);
        if (result < 0) {
                g_warning("play_cb: mpv loadfile command failed: %s", mpv_error_string(result));
                return;
        }
        
        /* Set pause property to false (start playing) */
        int pause = 0;
        result = mpv_set_property(self->mpv_player, "pause", MPV_FORMAT_FLAG, &pause);
        if (result < 0) {
                g_warning("play_cb: mpv pause property set failed: %s", mpv_error_string(result));
        }

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

        /* Set pause property to true */
        int pause = 1;
        mpv_set_property(self->mpv_player, "pause", MPV_FORMAT_FLAG, &pause);
        
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
        
        if (self->priv->switching_tracks) {
                g_print("Already switching tracks, ignoring next request\n");
                return;
        }
        
        self->priv->switching_tracks = TRUE;
        g_print("Next track requested\n");
        
        mode = self->priv->random ?
                MEDIA_SELECTION_RANDOM : MEDIA_SELECTION_NEXT;
        next = budgie_media_view_get_info(BUDGIE_MEDIA_VIEW(self->view),
                mode);
        if (!next) {
                g_print("No next track available\n");
                self->priv->switching_tracks = FALSE;
                return;
        }
        self->priv->media = next;
        /* MPV will automatically stop current file when loading new one */
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
        
        self->priv->switching_tracks = FALSE;
}

static void prev_cb(GtkWidget *widget, gpointer userdata)
{
        MediaInfo *prev = NULL;
        BudgieWindow *self;
        BudgieMediaSelection mode;

        self = BUDGIE_WINDOW(userdata);
        
        if (self->priv->switching_tracks) {
                g_print("Already switching tracks, ignoring prev request\n");
                return;
        }
        
        self->priv->switching_tracks = TRUE;
        g_print("Previous track requested\n");
        
        mode = self->priv->random ?
                MEDIA_SELECTION_RANDOM : MEDIA_SELECTION_PREVIOUS;
        prev = budgie_media_view_get_info(BUDGIE_MEDIA_VIEW(self->view),
                mode);
        if (!prev) {
                g_print("No previous track available\n");
                self->priv->switching_tracks = FALSE;
                return;
        }
        self->priv->media = prev;
        /* MPV will automatically stop current file when loading new one */
        /* In future only do this if not paused */
        play_cb(NULL, userdata);
        
        self->priv->switching_tracks = FALSE;
}

static gboolean refresh_cb(gpointer userdata) {
        BudgieWindow *self;
        double duration = 0;
        double position = 0;

        self = BUDGIE_WINDOW(userdata);

        /* Get media duration from MPV */
        if (self->priv->duration == 0) {
                if (mpv_get_property(self->mpv_player, "duration", MPV_FORMAT_DOUBLE, &duration) >= 0) {
                        self->priv->duration = (guint64)(duration * 1000000000); /* Convert to nanoseconds */
                }
        }
        
        /* Get current position from MPV */
        if (mpv_get_property(self->mpv_player, "time-pos", MPV_FORMAT_DOUBLE, &position) < 0) {
                return TRUE;
        }

        budgie_status_area_set_media_time(BUDGIE_STATUS_AREA(self->status),
                (gint64)self->priv->duration, (gint64)(position * 1000000000));
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
        const char *aspect_mode;

        self = BUDGIE_WINDOW(userdata);
        
        /* Set video aspect ratio mode for MPV */
        aspect_mode = self->priv->force_aspect ? "yes" : "no";
        mpv_set_option_string(self->mpv_player, "video-aspect-override", aspect_mode);
        
        /* Otherwise we get dirty regions on our drawing area */
        gtk_widget_queue_draw(self->window);
}

static void store_media(gpointer data1, gpointer data2)
{
        MediaInfo *info;
        BudgieWindow *self;

        info = (MediaInfo*)data1;
        self = BUDGIE_WINDOW(data2);

        g_print("    Storing media in DB: %s\n", info->title ? info->title : info->path);
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
        g_print("Scanning %d media directories...\n", length);
        
        mimes[0] = "audio/";
        mimes[1] = "video/";
        for (i=0; i < length; i++) {
                g_print("Scanning directory: %s\n", self->media_dirs[i]);
                search_directory(self->media_dirs[i], &tracks, 2, mimes);
        }

        g_print("Found %d media files\n", g_list_length(tracks));

        budgie_db_begin_transaction(self->db);
        g_list_foreach(tracks, store_media, self);
        g_list_free_full(tracks, free_media_info);
        budgie_db_end_transaction(self->db);

        g_print("Database transaction completed\n");

        budgie_control_bar_set_action_enabled(BUDGIE_CONTROL_BAR(self->toolbar),
                BUDGIE_ACTION_RELOAD, TRUE);

        g_print("Setting database on media view\n");
        /* Use g_idle_add to ensure UI update happens on main thread */
        g_idle_add((GSourceFunc)update_media_view, self);
        g_print("Media view database set completed\n");

        return NULL;
}

/* Helper function to update media view on main thread */
static gboolean update_media_view(gpointer data)
{
        BudgieWindow *self = BUDGIE_WINDOW(data);
        g_print("Updating media view on main thread\n");
        g_object_set(BUDGIE_MEDIA_VIEW(self->view), "database", self->db, NULL);
        return FALSE; /* Remove from idle queue */
}

/* MPV callback implementations */
static gboolean handle_end_file_idle(gpointer userdata)
{
        BudgieWindow *self = BUDGIE_WINDOW(userdata);
        
        if (!self || !G_IS_OBJECT(self)) {
                return FALSE;
        }
        
        /* Don't handle end-of-file if we're already switching tracks */
        if (self->priv->switching_tracks) {
                g_print("Ignoring end-of-file event during track switch\n");
                return FALSE;
        }
        
        g_print("Handling natural end-of-file event (track finished playing)\n");
        
        /* Handle end of file - similar to GStreamer EOS */
        if (!self->priv->repeat) {
                self->priv->switching_tracks = TRUE;
                next_cb(NULL, self);
                self->priv->switching_tracks = FALSE;
        } else {
                /* Restart the same file */
                play_cb(NULL, self);
        }
        
        return FALSE; /* Don't repeat this idle function */
}

static gboolean handle_error_idle(gpointer userdata)
{
        gchar *error_msg = (gchar *)userdata;
        
        /* Find the window instance - this is a simplified approach */
        /* In a real implementation, you'd want to pass both the window and error message */
        /* For now, we'll just print the error */
        g_warning("MPV Error: %s", error_msg);
        g_free(error_msg);
        
        return FALSE; /* Don't repeat this idle function */
}

static void wakeup_cb(void *ctx)
{
        BudgieWindow *self;
        
        if (!ctx || !G_IS_OBJECT(ctx)) {
                return;
        }
        
        self = BUDGIE_WINDOW(ctx);
        
        if (!self->mpv_player) {
                return;
        }
        
        /* Process MPV events */
        while (1) {
                mpv_event *event = mpv_wait_event(self->mpv_player, 0);
                if (event->event_id == MPV_EVENT_NONE) {
                        break;
                }
                
                g_print("MPV Event received: %s\n", mpv_event_name(event->event_id));
                
                if (event->event_id == MPV_EVENT_END_FILE) {
                        mpv_event_end_file *end_file = (mpv_event_end_file *)event->data;
                        g_print("End file reason: %d (0=eof, 1=stop, 2=quit, 3=error, 4=redirect)\n", end_file->reason);
                        
                        /* Only handle natural end of file, not stops or errors */
                        if (end_file->reason == MPV_END_FILE_REASON_EOF) {
                                /* Use g_idle_add to handle this on the main thread */
                                g_idle_add(handle_end_file_idle, self);
                        } else {
                                g_print("Ignoring end-file event with reason: %d\n", end_file->reason);
                        }
                } else if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
                        mpv_event_log_message *msg = (mpv_event_log_message *)event->data;
                        if (g_strcmp0(msg->level, "error") == 0) {
                                /* Handle error on main thread */
                                gchar *error_msg = g_strdup(msg->text);
                                g_idle_add(handle_error_idle, error_msg);
                        }
                } else if (event->event_id == MPV_EVENT_FILE_LOADED) {
                        g_print("File loaded successfully\n");
                } else if (event->event_id == MPV_EVENT_PLAYBACK_RESTART) {
                        g_print("Playback restarted\n");
                }
        }
}

static void *get_proc_address(void *ctx, const char *name)
{
        /* Cross-platform OpenGL function loading */
        (void)ctx; /* Unused parameter */
        
#if defined(GDK_WINDOWING_X11)
        /* Linux X11 - use glXGetProcAddress */
        return (void*)glXGetProcAddress((const GLubyte*)name);
#elif defined(GDK_WINDOWING_WAYLAND)
        /* Linux Wayland - use eglGetProcAddress */
        return (void*)eglGetProcAddress(name);
#elif defined(GDK_WINDOWING_WIN32)
        /* Windows - use wglGetProcAddress */
        return (void*)wglGetProcAddress(name);
#elif defined(GDK_WINDOWING_QUARTZ)
        /* macOS - use dlsym with OpenGL framework */
        static void *opengl_handle = NULL;
        if (!opengl_handle) {
                opengl_handle = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
        }
        return opengl_handle ? dlsym(opengl_handle, name) : NULL;
#else
        /* Fallback */
        return NULL;
#endif
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer userdata) {
        BudgieWindow *self;

        self = BUDGIE_WINDOW(userdata);
        
        if (self->priv->mpv_gl) {
                /* Get widget dimensions */
                int width = gtk_widget_get_allocated_width(widget);
                int height = gtk_widget_get_allocated_height(widget);
                
                if (width > 0 && height > 0) {
                        if (self->priv->using_opengl) {
                                /* Use OpenGL rendering */
                                mpv_render_param gl_params[] = {
                                        {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){
                                                .fbo = 0,  /* Default framebuffer */
                                                .w = width,
                                                .h = height,
                                        }},
                                        {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
                                        {0}
                                };
                                
                                mpv_render_context_render(self->priv->mpv_gl, gl_params);
                        } else {
                                /* Use software rendering */
                                int render_width = width;
                                int render_height = height;
                                
                                /* Limit rendering size for performance on software rendering */
                                if (width > 720) {
                                        render_width = 720;
                                        render_height = (height * 720) / width;
                                }
                                
                                size_t stride = render_width * 4;
                                size_t needed_size = render_height * stride;
                                
                                /* Reuse or allocate frame buffer */
                                if (self->priv->frame_buffer_size != needed_size || 
                                    self->priv->frame_width != render_width || 
                                    self->priv->frame_height != render_height) {
                                        
                                        g_free(self->priv->frame_buffer);
                                        self->priv->frame_buffer = g_malloc0(needed_size);
                                        self->priv->frame_buffer_size = needed_size;
                                        self->priv->frame_width = render_width;
                                        self->priv->frame_height = render_height;
                                }
                                
                                /* Set up software rendering parameters */
                                mpv_render_param sw_params[] = {
                                        {MPV_RENDER_PARAM_SW_SIZE, &(int[2]){render_width, render_height}},
                                        {MPV_RENDER_PARAM_SW_FORMAT, "bgr0"},  /* Cairo prefers BGR */
                                        {MPV_RENDER_PARAM_SW_STRIDE, &stride},
                                        {MPV_RENDER_PARAM_SW_POINTER, self->priv->frame_buffer},
                                        {0}
                                };
                                
                                /* Render frame with software */
                                if (mpv_render_context_render(self->priv->mpv_gl, sw_params) >= 0) {
                                        /* Create Cairo surface and draw */
                                        cairo_surface_t *surface = cairo_image_surface_create_for_data(
                                                self->priv->frame_buffer, CAIRO_FORMAT_RGB24, render_width, render_height, stride);
                                        if (surface) {
                                                /* Scale up if needed */
                                                if (render_width != width || render_height != height) {
                                                        cairo_scale(cr, (double)width / render_width, (double)height / render_height);
                                                }
                                                cairo_set_source_surface(cr, surface, 0, 0);
                                                cairo_paint(cr);
                                                cairo_surface_destroy(surface);
                                        }
                                }
                        }
                } else {
                        /* Invalid dimensions - fill with black */
                        cairo_set_source_rgb(cr, 0, 0, 0);
                        cairo_paint(cr);
                }
        } else {
                /* No render context - fill with black */
                cairo_set_source_rgb(cr, 0, 0, 0);
                cairo_paint(cr);
        }
        
        return FALSE;
}


static void realize_cb(GtkWidget *widg, gpointer userdata)
{
        BudgieWindow *self;
        GdkWindow *window;
        mpv_render_param params[3];
        int param_count = 0;

        self = BUDGIE_WINDOW(userdata);
        window = gtk_widget_get_window(self->video);
        if (!gdk_window_ensure_native(window)) {
                g_error("Unable to initialize video");
        }

        /* Try OpenGL first, fallback to software rendering */
        gboolean use_opengl = FALSE;
        
#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND)
        /* On Linux, try OpenGL first as it's usually well-supported */
        use_opengl = TRUE;
#elif defined(GDK_WINDOWING_WIN32)
        /* On Windows, try OpenGL */
        use_opengl = TRUE;
#elif defined(GDK_WINDOWING_QUARTZ)
        /* On macOS, use software rendering for now due to context issues */
        use_opengl = FALSE;
#endif

        if (use_opengl) {
                /* Try OpenGL rendering first */
                params[param_count++] = (mpv_render_param){MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL};
                params[param_count++] = (mpv_render_param){MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
                        .get_proc_address = get_proc_address,
                        .get_proc_address_ctx = self,
                }};
                params[param_count++] = (mpv_render_param){0};

                if (mpv_render_context_create(&self->priv->mpv_gl, self->mpv_player, params) < 0) {
                        g_warning("OpenGL rendering failed, falling back to software rendering");
                        use_opengl = FALSE;
                        self->priv->using_opengl = FALSE;
                } else {
                        self->priv->using_opengl = TRUE;
                }
        }
        
        if (!use_opengl) {
                /* Fallback to software rendering */
                param_count = 0;
                params[param_count++] = (mpv_render_param){MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_SW};
                params[param_count++] = (mpv_render_param){0};

                if (mpv_render_context_create(&self->priv->mpv_gl, self->mpv_player, params) < 0) {
                        g_warning("Failed to initialize MPV render context");
                        self->priv->mpv_gl = NULL;
                        self->priv->using_opengl = FALSE;
                } else {
                        g_print("Using software rendering for video\n");
                        self->priv->using_opengl = FALSE;
                }
        } else {
                g_print("Using OpenGL rendering for video\n");
        }

        if (self->priv->mpv_gl) {
                /* Set up update callback for redraws */
                mpv_render_context_set_update_callback(self->priv->mpv_gl, 
                                                        (mpv_render_update_fn)gtk_widget_queue_draw,
                                                        self->video);
        }

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
        double seek_time;

        self = BUDGIE_WINDOW(userdata);
        
        /* Convert nanoseconds to seconds for MPV */
        seek_time = (double)value / 1000000000.0;
        
        /* Seek to position */
        mpv_set_property(self->mpv_player, "time-pos", MPV_FORMAT_DOUBLE, &seek_time);
}

static void media_selected_cb(BudgieMediaView *view, gpointer info, gpointer userdata)
{
        BudgieWindow *self;
        MediaInfo *media;

        self = BUDGIE_WINDOW(userdata);
        media = (MediaInfo*)info;
        
        g_print("media_selected_cb: Selected media: %s\n", media ? media->path : "(null)");
        
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
