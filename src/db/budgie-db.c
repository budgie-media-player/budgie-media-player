/*
 * budgie-db.c
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "budgie-db.h"

#include <sqlite3.h>

/* Private storage */
struct _BudgieDBPrivate {
        gchar *storage_path;
        sqlite3 *db;
        sqlite3_stmt *insert;
        sqlite3_stmt *get_all;
        GHashTable *exact_table;
        GHashTable *like_table;
        GHashTable *field_table;
};

G_DEFINE_TYPE_WITH_PRIVATE(BudgieDB, budgie_db, G_TYPE_OBJECT)

/* Boilerplate GObject code */
static void budgie_db_class_init(BudgieDBClass *klass);
static void budgie_db_init(BudgieDB *self);
static void budgie_db_dispose(GObject *object);


/* MediaInfo API */
void free_media_info(gpointer p_info)
{
        MediaInfo *info;

        info = (MediaInfo*)p_info;
        if (info->title) {
                g_free(info->title);
        }
        if (info->artist) {
                g_free(info->artist);
        }
        if (info->album) {
                g_free(info->album);
        }
        if (info->band) {
                g_free(info->band);
        }
        if (info->genre) {
                g_free(info->genre);
        }
        if (info->path) {
                g_free(info->path);
        }
        if (info->mime) {
                g_free(info->mime);
        }
}

/* Initialisation */
static void budgie_db_class_init(BudgieDBClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_db_dispose;
}

static void budgie_db_init(BudgieDB *self)
{
        const gchar *config = NULL;
        self->priv = budgie_db_get_instance_private(self);
        const char *sql = NULL;
        int rc = 0;
        char *err = NULL;
        sqlite3 *db = NULL;
        sqlite3_stmt *stm = NULL;
        GHashTable *table = NULL;
        const gchar *interests[] = {
                "TITLE",  "ARTIST", "ALBUM", "GENRE", "MIME"
        };
        

        /* Our storage location */
        config = g_get_user_config_dir();
        self->priv->storage_path = g_strdup_printf("%s/%s", config,
                CONFIG_NAME);

        rc = sqlite3_open(self->priv->storage_path, &db);
        if (rc != SQLITE_OK) {
                g_critical("Unable to create/open database");
                return;
        }

        self->priv->db = db;

        /* rep */
        sql = "CREATE TABLE IF NOT EXISTS MEDIA (ID PATH UNIQUE, TITLE TEXT, ARTIST TEXT, ALBUM TEXT, BAND TEXT, GENRE TEXT, MIME TEXT);";
        rc = sqlite3_exec(db, sql, NULL, NULL, &err);
        if (rc != SQLITE_OK) {
                g_critical("Unable to initialise database");
                free(err);
                return;
        }

        /* statement preparation */
        sql = "INSERT OR REPLACE INTO MEDIA VALUES (?, ?, ?, ?, ?, ?, ?);";
        rc = sqlite3_prepare_v2(db, sql, -1, &stm, NULL);
        if (rc != SQLITE_OK) {
                g_critical("DB Error: %s", sqlite3_errmsg(db));
                return;
        }
        self->priv->insert = stm;

        /* Get all by field */
        table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)sqlite3_finalize);
        for (int i = 0; i < G_N_ELEMENTS(interests); i++) {
                gchar *s = g_strdup_printf("SELECT DISTINCT %s FROM MEDIA;", interests[i]);
                
                /* Search field, exact  */
                rc = sqlite3_prepare_v2(db, s, -1, &stm, NULL);
                if (rc != SQLITE_OK) {
                        g_critical("DB Error: %s", sqlite3_errmsg(db));
                        return;
                }
                g_free(s);
                g_hash_table_insert(table, (gchar*)interests[i], stm);
        }
        self->priv->field_table = table;

        /* Get all  */
        sql = "SELECT * FROM MEDIA;";
        rc = sqlite3_prepare_v2(db, sql, -1, &stm, NULL);
        if (rc != SQLITE_OK) {
                g_critical("DB Error: %s", sqlite3_errmsg(db));
                return;
        }
        self->priv->get_all = stm;

        table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)sqlite3_finalize);
        for (int i = 0; i < G_N_ELEMENTS(interests); i++) {
                gchar *s = g_strdup_printf("SELECT * FROM MEDIA WHERE %s = ? COLLATE NOCASE;", interests[i]);
                
                /* Search field, exact  */
                rc = sqlite3_prepare_v2(db, s, -1, &stm, NULL);
                if (rc != SQLITE_OK) {
                        g_critical("DB Error: %s", sqlite3_errmsg(db));
                        return;
                }
                g_free(s);
                g_hash_table_insert(table, (gchar*)interests[i], stm);
        }
        self->priv->exact_table = table;

        table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)sqlite3_finalize);
        for (int i = 0; i < G_N_ELEMENTS(interests); i++) {
                gchar *s = g_strdup_printf("SELECT * FROM MEDIA WHERE %s LIKE ? COLLATE NOCASE;", interests[i]);
                
                /* Search field, exact  */
                rc = sqlite3_prepare_v2(db, s, -1, &stm, NULL);
                if (rc != SQLITE_OK) {
                        g_critical("DB Error: %s", sqlite3_errmsg(db));
                        return;
                }
                g_free(s);
                g_hash_table_insert(table, (gchar*)interests[i], stm);
        }
        self->priv->like_table = table;


        if (err) {
                free(err);
        }
}

static void budgie_db_dispose(GObject *object)
{
        BudgieDB *self;

        self = BUDGIE_DB(object);
        if (self->priv->storage_path) {
                g_free(self->priv->storage_path);
                self->priv->storage_path = NULL;
        }

        if (self->priv->insert) {
                sqlite3_finalize(self->priv->insert);
                self->priv->insert = NULL;
        }
        if (self->priv->get_all) {
                sqlite3_finalize(self->priv->get_all);
                self->priv->get_all = NULL;
        }
        if (self->priv->like_table) {
                g_hash_table_unref(self->priv->like_table);
                self->priv->like_table = NULL;
        }
        if (self->priv->exact_table) {
                g_hash_table_unref(self->priv->exact_table);
                self->priv->exact_table = NULL;
        }
        if (self->priv->field_table) {
                g_hash_table_unref(self->priv->field_table);
                self->priv->field_table = NULL;
        }
        
        if (self->priv->db) {
                sqlite3_close(self->priv->db);
                self->priv->db = NULL;
        }
        /* Destruct */
        G_OBJECT_CLASS(budgie_db_parent_class)->dispose(object);
}

/* Utility; return a new BudgieDB */
BudgieDB* budgie_db_new(void)
{
        BudgieDB *self;

        self = g_object_new(BUDGIE_DB_TYPE, NULL);
        return BUDGIE_DB(self);
}

void budgie_db_store_media(BudgieDB *self, MediaInfo *info)
{
        sqlite3_stmt *stm = self->priv->insert;
        gboolean dwarn = TRUE;
        int rc;
        sqlite3_reset(stm);

        /* (ID PATH UNIQUE, TITLE TEXT, ALBUM TEXT, BAND TEXT, GENRE TEXT, MIME TEXT) */
        if (sqlite3_bind_text(stm, 1, info->path, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 2, info->title, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 3, info->artist, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 4, info->album, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 5, info->band, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 6, info->genre, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }
        if (sqlite3_bind_text(stm, 7, info->mime, -1, SQLITE_STATIC) != SQLITE_OK) {
                goto end;
        }

        rc = sqlite3_step(stm);
        if (rc != SQLITE_DONE) {
                goto end;
        }

        dwarn = FALSE;
end:
        if (dwarn) {
                g_critical("Error inserting media: %s", sqlite3_errmsg(self->priv->db));
                return;
        }
}

static inline gchar *name_for_field(MediaQuery query)
{
        switch (query) {
                case MEDIA_QUERY_ALBUM:
                        return "ALBUM";
                case MEDIA_QUERY_ARTIST:
                        return "ARTIST";
                case MEDIA_QUERY_GENRE:
                        return "GENRE";
                case MEDIA_QUERY_MIME:
                        return "MIME";
                case MEDIA_QUERY_TITLE:
                        return "TITLE";
                default:
                        g_assert_not_reached();
                        return NULL;
        }
}

gboolean budgie_db_get_all_by_field(BudgieDB *self,
                                    MediaQuery query,
                                    GPtrArray **results)
{
        sqlite3_stmt *stm = NULL;
        int rc;
        const char *select = NULL;
        GPtrArray *res = NULL;


        select = name_for_field(query);
        stm = g_hash_table_lookup(self->priv->field_table, select);
        sqlite3_reset(stm);

        res = g_ptr_array_new();
        while ((rc = sqlite3_step(stm)) == SQLITE_ROW) {
                const gchar *txt = (const gchar*)sqlite3_column_text(stm, 0);
                if (!txt) {
                        continue;
                }
                g_ptr_array_add(res, g_strdup(txt));

        }
        *results = res;
        return TRUE;
}

static inline MediaInfo *media_info_from_statement(sqlite3_stmt *stm)
{
        MediaInfo *ret = NULL;
        const unsigned char *txt = NULL;

        ret = calloc(1, sizeof(MediaInfo));
        if (!ret) {
                return NULL;
        }

        txt = sqlite3_column_text(stm, 0);
        if (txt) {
                ret->path = g_strdup((const gchar*)txt);
        }
        /* Title */
        txt = sqlite3_column_text(stm, 1);
        if (txt) {
                ret->title = g_strdup((const gchar*)txt);
        }
        /* Artist */
        txt = sqlite3_column_text(stm, 2);
        if (txt) {
                ret->artist = g_strdup((const gchar*)txt);
        }
        /* Album */
        txt = sqlite3_column_text(stm, 3);
        if (txt) {
                ret->album = g_strdup((const gchar*)txt);
        }
        /* Band */
        txt = sqlite3_column_text(stm, 4);
        if (txt) {
                ret->band = g_strdup((const gchar*)txt);
        }
        /* Genre */
        txt = sqlite3_column_text(stm, 5);
        if (txt) {
                ret->genre = g_strdup((const gchar*)txt);
        }
        /* Mime */
        txt = sqlite3_column_text(stm, 6);
        if (txt) {
                ret->mime = g_strdup((const gchar*)txt);
        }

        return ret;
}

GList* budgie_db_get_all_media(BudgieDB* self)
{
        sqlite3_stmt *stm = self->priv->get_all;
        int rc;
        sqlite3_reset(stm);
        GList *ret = NULL;

        while ((rc = sqlite3_step(stm)) == SQLITE_ROW) {
                MediaInfo *info = media_info_from_statement(stm);
                ret = g_list_append(ret, info);
        }
        return ret;
}

gboolean budgie_db_search_field(BudgieDB *self,
                                MediaQuery query,
                                MatchQuery match,
                                gchar *term,
                                guint max,
                                GPtrArray **results)
{
        g_assert(query >= 0 && query < MEDIA_QUERY_MAX);
        g_assert(match >= 0 && match < MATCH_QUERY_MAX);
        g_assert(term != NULL);
        sqlite3_stmt *stm = NULL;
        int rc;
        GPtrArray *ret = NULL;
        gchar *what = NULL;
        const gchar *select = NULL;

        select = name_for_field(query);

        switch (match) {
                case MATCH_QUERY_EXACT:
                        stm = g_hash_table_lookup(self->priv->exact_table, select);
                        what = g_strdup(term);
                        break;
                case MATCH_QUERY_START:
                        what = g_strdup_printf("%s%%", term);
                        stm = g_hash_table_lookup(self->priv->like_table, select);
                        break;
                case MATCH_QUERY_END:
                        what = g_strdup_printf("%%%s", term);
                        stm = g_hash_table_lookup(self->priv->like_table, select);
                default:
                        break;
        }
        sqlite3_reset(stm);

        if (sqlite3_bind_text(stm, 1, what, -1, SQLITE_STATIC) != SQLITE_OK) {
                g_critical("Unable to bind sqlite statement: %s", sqlite3_errmsg(self->priv->db));
                return FALSE;
        }

        ret = g_ptr_array_new();

        while ((rc = sqlite3_step(stm)) == SQLITE_ROW) {
                MediaInfo *info = media_info_from_statement(stm);
                g_ptr_array_add(ret, info);
        }
        g_free(what);

        *results = ret;
        return TRUE;
}

gint budgie_db_sort(gconstpointer a, gconstpointer b)
{
        MediaInfo* m1 = NULL;
        MediaInfo* m2 = NULL;

        /* Alphabetically compare */
        m1 = *(MediaInfo**)a;
        m2 = *(MediaInfo**)b;

        if (!m1 || !m2) {
                return 0;
        }

        return g_ascii_strcasecmp(m1->title, m2->title);
}

void budgie_db_begin_transaction(BudgieDB *self)
{
        int rc;
        rc = sqlite3_exec(self->priv->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

        if (rc != SQLITE_OK) {
                g_warning("Unable to begin transaction");
        }
}
void budgie_db_end_transaction(BudgieDB *self)
{
        int rc;
        rc = sqlite3_exec(self->priv->db, "COMMIT;", NULL, NULL, NULL);

        if (rc != SQLITE_OK) {
                g_warning("Unable to end transaction");
        }
}
