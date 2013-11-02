/*
 * media-db.h
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
#ifndef media_db_h
#define media_db_h

#include <glib-object.h>
#include <gdbm.h>

typedef struct _MediaDB MediaDB;
typedef struct _MediaDBClass   MediaDBClass;
typedef struct _MediaDBPrivate MediaDBPrivate;

#define MEDIA_DB_TYPE (media_db_get_type())
#define MEDIA_DB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEDIA_DB_TYPE, MediaDB))
#define IS_MEDIA_DB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEDIA_DB_TYPE))
#define MEDIA_DB_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MEDIA_DB_TYPE, MediaDBClass))
#define IS_MEDIA_DB_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MEDIA_DB_TYPE))
#define MEDIA_DB_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MEDIA_DB_TYPE, MediaDBClass))

#define CONFIG_NAME "idmp-1.db"

/* Private storage */
struct _MediaDBPrivate {
        gchar *storage_path;
        GDBM_FILE db;
};

/* MediaDB object */
struct _MediaDB {
        GObject parent;

        MediaDBPrivate *priv;
};

/* MediaDB class definition */
struct _MediaDBClass {
        GObjectClass parent_class;
};


/* MediaInfo API */
void free_media_info(gpointer p_info);

typedef struct MediaInfo {
        gchar *title;
        gchar *artist;
        gchar *album;
        gchar *genre;
        gchar *path;
} MediaInfo;

/* Boilerplate GObject code */
static void media_db_class_init(MediaDBClass *klass);
static void media_db_init(MediaDB *self);
static void media_db_dispose(GObject *object);
GType media_db_get_type(void);

/* MediaDB methods */
MediaDB* media_db_new(void);
void media_db_store_media(MediaDB *self, MediaInfo *info);

#endif /* media_db_h */
