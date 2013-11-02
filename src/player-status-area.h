/*
 * player-status-area.h
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
#ifndef player_status_area_h
#define player_status_area_h

#include <glib-object.h>
#include <gtk/gtk.h>

#include "db/media-db.h"

typedef struct _PlayerStatusArea PlayerStatusArea;
typedef struct _PlayerStatusAreaClass   PlayerStatusAreaClass;
typedef struct _PlayerStatusAreaPrivate PlayerStatusAreaPrivate;

#define PLAYER_STATUS_AREA_TYPE (player_status_area_get_type())
#define PLAYER_STATUS_AREA(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAYER_STATUS_AREA_TYPE, PlayerStatusArea))
#define IS_PLAYER_STATUS_AREA(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAYER_STATUS_AREA_TYPE))
#define PLAYER_STATUS_AREA_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_STATUS_AREA_TYPE, PlayerStatusAreaClass))
#define IS_PLAYER_STATUS_AREA_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_STATUS_AREA_TYPE))
#define PLAYER_STATUS_AREA_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_STATUS_AREA_TYPE, PlayerStatusAreaClass))

/* Private storage */
struct _PlayerStatusAreaPrivate {
        gchar *title_string;
        gchar *time_string;
};

/* PlayerStatusArea object */
struct _PlayerStatusArea {
        GtkEventBox parent;

        GtkWidget *label;
        GtkWidget *time_label;

        PlayerStatusAreaPrivate *priv;
};

/* PlayerStatusArea class definition */
struct _PlayerStatusAreaClass {
        GtkEventBoxClass parent_class;
};


/* Boilerplate GObject code */
static void player_status_area_class_init(PlayerStatusAreaClass *klass);
static void player_status_area_init(PlayerStatusArea *self);
static void player_status_area_dispose(GObject *object);
GType player_status_area_get_type(void);

/* PlayerStatusArea methods */
GtkWidget* player_status_area_new(void);

void player_status_area_set_media(PlayerStatusArea *self, MediaInfo *info);


/**
 * Set the time information for the current item
 * @param max Total duration
 * @param current Time passed
 */
void player_status_area_set_media_time(PlayerStatusArea *self, gint64 max, gint64 current);

#endif /* player_status_area_h */
