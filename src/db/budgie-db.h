/*
 * budgie-db.h
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

#define CONFIG_NAME "budgie-1.db"

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

/**
 * Free a MediaInfo
 * @param p_info MediaInfo pointer
 */
void free_media_info(gpointer p_info);

/**
 * Represents relevant media information
 */
typedef struct MediaInfo {
        gchar *title; /**<Title */
        gchar *artist; /**<Artist or author */
        gchar *album; /**<Album */
        gchar *genre; /**<Genre */
        gchar *path; /**<File system path */
        gchar *mime; /**<File mime type */
} MediaInfo;

/**
 * Used to query the database for matches
 */
typedef enum {
        MEDIA_QUERY_TITLE = 0, /**<Query the title */
        MEDIA_QUERY_ARTIST, /**<Query the artist */
        MEDIA_QUERY_ALBUM, /**<Query the album */
        MEDIA_QUERY_GENRE, /**<Query the genre */
        MEDIA_QUERY_MAX
} MediaQuery;

/**
 * Used to determine search matching
 */
typedef enum {
        MATCH_QUERY_START = 0, /**<Match the start of the search term */
        MATCH_QUERY_END, /**<Match the end of the search term */
        MATCH_QUERY_EXACT, /**<Match if search term is identical */
        MATCH_QUERY_MAX
} MatchQuery;

GType budgie_db_get_type(void);

/* BudgieDB methods */

/**
 * Construct a new BudgieDB
 */
BudgieDB* budgie_db_new(void);

/**
 * Store media in BudgieDB
 * @param self BudgieDB instance
 * @param info Media to store
 */
void budgie_db_store_media(BudgieDB *self, MediaInfo *info);

/**
 * Retrieve media information from BudgieDB by filesystem path
 * You must free the result of this call using free_media_info
 * @param self BudgieDB instance
 * @param path Path to media file on the filesystem
 * @return a MediaInfo if the file is known, or NULL
 */
MediaInfo* budgie_db_get_media(BudgieDB *self, gchar *path);

/**
 * Get all media known to BudgieDB
 * You must free the result of this call using g_slist_free_full
 * @param self BudgieDB instance
 * @return a singly linked list of results, or NULL
 */
GSList* budgie_db_get_all_media(BudgieDB* self);

/**
 * Return string values of one field for all MediaInfo in the database
 * @param self BudgieDB instance
 * @param query The query to perform
 * @param results Pointer to store results in
 * @return a boolean value, indicating success of the operation
 */
gboolean budgie_db_get_all_by_field(BudgieDB *self,
                                    MediaQuery query,
                                    GPtrArray **results);

/**
 * Search all media for a given term
 * @param self BudgieDB instance
 * @param query The query to perform
 * @param match Type of match to perform
 * @param term Term to search for
 * @param max Maximum results to return, or -1 for unlimited
 * @param results Pointer to store results in
 * @return a boolean value, indicating success of the operation
 */
gboolean budgie_db_search_field(BudgieDB *self,
                                MediaQuery query,
                                MatchQuery match,
                                gchar *term,
                                guint max,
                                GPtrArray **results);

#endif /* budgie_db_h */
