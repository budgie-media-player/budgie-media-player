/*
 * player-view.c
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
#include "player-view.h"

G_DEFINE_TYPE_WITH_PRIVATE(PlayerView, player_view, GTK_TYPE_BIN);

/* Initialisation */
static void player_view_class_init(PlayerViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &player_view_dispose;
}

static void player_view_init(PlayerView *self)
{
        GtkWidget *scroller;

        scroller = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(self), scroller);
}

static void player_view_dispose(GObject *object)
{
        PlayerView *self;

        self = PLAYER_VIEW(object);

        /* Destruct */
        G_OBJECT_CLASS (player_view_parent_class)->dispose (object);
}

/* Utility; return a new PlayerView */
GtkWidget* player_view_new(void)
{
        PlayerView *self;

        self = g_object_new(PLAYER_VIEW_TYPE, NULL);
        return GTK_WIDGET(self);
}
