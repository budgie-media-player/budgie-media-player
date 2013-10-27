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

G_DEFINE_TYPE_WITH_PRIVATE(MusicPlayerWindow, music_player_window, G_TYPE_OBJECT);

/* Initialisation */
static void music_player_window_class_init(MusicPlayerWindowClass *klass)
{
        /* Nothing yet */
}

static void music_player_window_init(MusicPlayerWindow *self)
{
        GtkWidget *window;
        GtkWidget *header;

        self->priv = music_player_window_get_instance_private(self);

        /* Initialize our window */
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        self->priv->window = window;

        /* Set our window up */
        gtk_window_set_title(GTK_WINDOW(window), "Music Player");
        gtk_widget_set_size_request(window, 900, 500);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_name(GTK_WINDOW(window), "gnome-music");
        gtk_window_set_wmclass(GTK_WINDOW(window), "MusicPlayer", "Music Player");
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        /* Create client side decorations */
        header = gtk_header_bar_new();
        self->priv->header = header;
        gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Music Player");

        /* Always show the close button */
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
        gtk_window_set_titlebar(GTK_WINDOW(window), header);

        gtk_widget_show_all(window);
}

static void music_player_window_dispose(GObject *object)
{
        MusicPlayerWindow *self;

        self = MUSIC_PLAYER_WINDOW(object);

        /* Destroy our window */
        gtk_window_destroy(self->priv->window);
        g_object_unref(self->priv->window);

        /* Destruct */
        G_OBJECT_CLASS (music_player_window_parent_class)->dispose (object);
}

/* Utility; return a new MusicPlayerWindow */
MusicPlayerWindow* music_player_window_new(void)
{
        MusicPlayerWindow *reader;

        reader = g_object_new(MUSIC_PLAYER_WINDOW_TYPE, NULL);
        return MUSIC_PLAYER_WINDOW(reader);
}
