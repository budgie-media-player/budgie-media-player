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
#include <string.h>

G_DEFINE_TYPE_WITH_PRIVATE(MusicPlayerWindow, music_player_window, G_TYPE_OBJECT);

/* Utilities */
static GtkWidget* new_button_with_icon(MusicPlayerWindow *self, const gchar *icon_name);
static void init_styles(MusicPlayerWindow *self);

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
        GtkWidget *prev, *play, *next;
        GtkWidget *volume;
        GtkWidget *search;
        GtkWidget *status;
        GtkWidget *player;

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
        self->prev = prev;
        /* Set some left padding */
        gtk_widget_set_margin_left(prev, 20);

        play = new_button_with_icon(self, "media-playback-start");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), play);
        self->play = play;

        next = new_button_with_icon(self, "media-seek-forward");
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), next);
        self->next = next;

        /* volume control */
        volume = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                0, 100, 1);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header), volume);
        gtk_scale_set_draw_value(GTK_SCALE(volume), FALSE);
        gtk_widget_set_size_request(volume, 100, 10);
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

        search_directory(self->priv->music_directory, &self->priv->tracks, "audio/");
        gtk_widget_show_all(window);
}

static void music_player_window_dispose(GObject *object)
{
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(object);

        g_object_unref(self->icon_theme);
        g_object_unref(self->css_provider);
        if (self->priv->tracks)
                g_slist_free_full (self->priv->tracks, g_free);

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
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_container_add(GTK_CONTAINER(button), image);

        return button;
}

static void init_styles(MusicPlayerWindow *self)
{
        GtkCssProvider *css_provider;
        GdkScreen *screen;
        const gchar *data = TEMP_CSS;

        css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(css_provider, data, strlen(data), NULL);
        screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_USER);
        self->css_provider = css_provider;
}
