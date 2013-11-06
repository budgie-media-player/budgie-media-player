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
#ifndef budgie_db_h
#define budgie_db_h

#include <glib-object.h>
#include <gdbm.h>

typedef struct _BudgieDB BudgieDB;
typedef struct _BudgieDBClass   BudgieDBClass;
typedef struct _BudgieDBPrivate BudgieDBPrivate;

#define BUDGIE_DB_TYPE (budgie_db_get_type())
#define BUDGIE_DB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_DB_TYPE, BudgieDB))
#define IS_BUDGIE_DB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_DB_TYPE))
#define BUDGIE_DB_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_DB_TYPE, BudgieDBClass))
#define IS_BUDGIE_DB_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_DB_TYPE))
#define BUDGIE_DB_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_DB_TYPE, BudgieDBClass))

#define CONFIG_NAME "idmp-1.db"

/* Private storage */
struct _BudgieDBPrivate {
        gchar *storage_path;
        GDBM_FILE db;
};

/* BudgieDB object */
struct _BudgieDB {
        GObject parent;

        BudgieDBPrivate *priv;
};

/* BudgieDB class definition */
struct _BudgieDBClass {
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
        gchar *mime;
} MediaInfo;

/* Boilerplate GObject code */
static void budgie_db_class_init(BudgieDBClass *klass);
static void budgie_db_init(BudgieDB *self);
static void budgie_db_dispose(GObject *object);
GType budgie_db_get_type(void);

/* BudgieDB methods */
BudgieDB* budgie_db_new(void);
void budgie_db_store_media(BudgieDB *self, MediaInfo *info);
MediaInfo* budgie_db_get_media(BudgieDB *self, gchar *path);
GSList* budgie_db_get_all_media(BudgieDB* self);
#endif /* budgie_db_h */
