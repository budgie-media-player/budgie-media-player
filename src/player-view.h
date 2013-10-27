/*
 * player-view.h
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
#ifndef player_view_h
#define player_view_h

#include <glib-object.h>
#include <gtk/gtk.h>

typedef struct _PlayerView PlayerView;
typedef struct _PlayerViewClass   PlayerViewClass;
typedef struct _PlayerViewPrivate PlayerViewPrivate;

#define PLAYER_VIEW_TYPE (player_view_get_type())
#define PLAYER_VIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAYER_VIEW_TYPE, PlayerView))
#define IS_PLAYER_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAYER_VIEW_TYPE))
#define PLAYER_VIEW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_VIEW_TYPE, PlayerViewClass))
#define IS_PLAYER_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_VIEW_TYPE))
#define PLAYER_VIEW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_VIEW_TYPE, PlayerViewClass))

/* TreeView columns */
typedef enum PlayerViewColumns {
        PLAYER_COLUMN_NAME = 0,
        PLAYER_COLUMN_TIME,
        PLAYER_COLUMN_ARTIST,
        PLAYER_COLUMN_ALBUM,
        PLAYER_COLUMN_GENRE,
        PLAYER_COLUMNS
} PlayerViewColumns;

/* Private storage */
struct _PlayerViewPrivate {
        unsigned int x; /* Reserved */
};

/* PlayerView object */
struct _PlayerView {
        GtkBin parent;

        GtkWidget *tree;

        PlayerViewPrivate *priv;
};

/* PlayerView class definition */
struct _PlayerViewClass {
        GtkBinClass parent_class;
};


/* Boilerplate GObject code */
static void player_view_class_init(PlayerViewClass *klass);
static void player_view_init(PlayerView *self);
static void player_view_dispose(GObject *object);
GType player_view_get_type(void);

/* PlayerView methods */
GtkWidget* player_view_new(void);

/**
 * Set the current list
 * @param list Current list
 */
void player_view_set_list(PlayerView *self, GSList* list);

#endif /* player_view_h */
