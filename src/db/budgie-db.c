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

/* Private storage */
struct _BudgieDBPrivate {
        gchar *storage_path;
        GDBM_FILE db;
};

G_DEFINE_TYPE_WITH_PRIVATE(BudgieDB, budgie_db, G_TYPE_OBJECT)

/* Private utilities */
static gboolean budgie_db_serialize(MediaInfo *info, uint8_t **target);
static gboolean budgie_db_deserialize(uint8_t* source, MediaInfo **target);

/* Boilerplate GObject code */
static void budgie_db_class_init(BudgieDBClass *klass);
static void budgie_db_init(BudgieDB *self);
static void budgie_db_dispose(GObject *object);

/* Used to keep the database thread safe */
static GMutex _lock;

/* MediaInfo API */
void free_media_info(gpointer p_info)
{
        MediaInfo *info;

        info = (MediaInfo*)p_info;
        if (info->title)
                g_free(info->title);
        if (info->artist)
                g_free(info->artist);
        if (info->album)
                g_free(info->album);
        if (info->band)
                g_free(info->band);
        if (info->genre)
                g_free(info->genre);
        if (info->path)
                g_free(info->path);
        if (info->mime)
                g_free(info->mime);
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
        const gchar *config;
        self->priv = budgie_db_get_instance_private(self);

        /* Our storage location */
        config = g_get_user_config_dir();
        self->priv->storage_path = g_strdup_printf("%s/%s", config,
                CONFIG_NAME);

        /* Open the database */
        self->priv->db = gdbm_open(self->priv->storage_path, 0,
                GDBM_WRCREAT, 0600, NULL);
        if (!self->priv->db)
                g_error("Failed to initialise database!");
}

static void budgie_db_dispose(GObject *object)
{
        BudgieDB *self;

        self = BUDGIE_DB(object);
        if (self->priv->storage_path)
                g_free(self->priv->storage_path);

        gdbm_close(self->priv->db);
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
        datum value;
        uint8_t *store = NULL;

        g_mutex_lock(&_lock);
        /* Path is the unique key */
        datum key = { (char*)info->path, ((int)strlen(info->path))+1 };

        if (!budgie_db_serialize(info, &store)) {
                g_warning("Unable to serialize data!");
                goto end;
        }
        value.dptr = (char*)store;
        value.dsize = (int)malloc_usable_size(store);;

        gdbm_store(self->priv->db, key, value, GDBM_REPLACE);
end:
        if (store)
                free(store);

        g_mutex_unlock(&_lock);
}

MediaInfo* budgie_db_get_media(BudgieDB *self, gchar *path)
{
        datum value;
        MediaInfo *ret = NULL;
        uint8_t* store = NULL;
        datum key = { (char*)path, (int)strlen(path)+1 };

        memset(&value, 0, sizeof(datum));
        value = gdbm_fetch(self->priv->db, key);
        if (value.dsize < 0 || value.dptr == NULL)
                goto end;
        store = (uint8_t*)value.dptr;

        if (!budgie_db_deserialize(store, &ret)) {
                g_message("Unable to deserialize");
                goto end;
        }
        ret->path = g_strdup(path);
end:
        if (store)
                free(store);

        return ret;
}

GSList* budgie_db_get_all_media(BudgieDB* self)
{
        datum key, nextkey;
        GSList* ret = NULL;
        char *path;
        MediaInfo *cur = NULL;

        g_mutex_lock(&_lock);

        key = gdbm_firstkey(self->priv->db);
        while (key.dptr) {
                path = (char*)key.dptr;
                cur = budgie_db_get_media(self, path);
                ret = g_slist_append(ret, cur);
                nextkey = gdbm_nextkey(self->priv->db, key);
                free(key.dptr);
                key = nextkey;
        }
        g_mutex_unlock(&_lock);

        return ret;
}

gboolean budgie_db_get_all_by_field(BudgieDB *self,
                                    MediaQuery query,
                                    GPtrArray **results)
{
        g_assert(query >= 0 && query < MEDIA_QUERY_MAX);

        datum key, nextkey;
        GPtrArray *_results = NULL;
        char *path;
        gboolean ret = FALSE;
        MediaInfo *media = NULL;
        char *append = NULL;
        gboolean should_append = TRUE;
        int i;

        g_mutex_lock(&_lock);

        _results = g_ptr_array_new();

        /* Iterate through every key in the database */
        key = gdbm_firstkey(self->priv->db);
        while (key.dptr) {
                path = (char*)key.dptr;

                media = budgie_db_get_media(self, path);
                /* Only interested in one field really */
                switch (query) {
                        case MEDIA_QUERY_TITLE:
                                append = media->title;
                                break;
                        case MEDIA_QUERY_ALBUM:
                                append = media->album;
                                break;
                        case MEDIA_QUERY_ARTIST:
                                append = media->artist;
                                break;
                        case MEDIA_QUERY_GENRE:
                                append = media->genre;
                                break;
                        case MEDIA_QUERY_MIME:
                                append = media->mime;
                                break;
                        default:
                                break;
                }
                if (append != NULL) {
                        for (i=0; i < _results->len; i++) {
                                if (g_str_equal(_results->pdata[i], append)) {
                                        should_append = FALSE;
                                        break;
                                }
                        }
                }
                if (should_append && append != NULL)
                        g_ptr_array_add(_results, g_strdup(append));
                should_append = TRUE;
                free_media_info(media);
                /* Visit the next key */
                nextkey = gdbm_nextkey(self->priv->db, key);
                free(key.dptr);
                key = nextkey;
                append = NULL;
        }
        g_mutex_unlock(&_lock);

        /* No results */
        if (_results->len < 1) {
                g_ptr_array_free(_results, TRUE);
                return ret;
        }
        ret = TRUE;
        *results = _results;

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

        datum key, nextkey;
        GPtrArray *_results = NULL;
        char *path;
        gboolean ret = FALSE;
        MediaInfo *media = NULL;
        char *test = NULL;
        int count = 0;
        gboolean succ = FALSE;

        g_mutex_lock(&_lock);

        _results = g_ptr_array_new();

        /* Iterate through every key in the database */
        key = gdbm_firstkey(self->priv->db);
        while (key.dptr) {
                /* Only return a limited set of results */
                if (count >= max && max != -1) {
                        free(key.dptr);
                        break;
                }
                path = (char*)key.dptr;

                media = budgie_db_get_media(self, path);
                /* Only interested in one field really */
                switch (query) {
                        case MEDIA_QUERY_TITLE:
                                test = media->title;
                                break;
                        case MEDIA_QUERY_ALBUM:
                                test = media->album;
                                break;
                        case MEDIA_QUERY_ARTIST:
                                test = media->artist;
                                break;
                        case MEDIA_QUERY_GENRE:
                                test = media->genre;
                                break;
                        case MEDIA_QUERY_MIME:
                                test = media->mime;
                                break;
                        default:
                                break;
                }
                if (!test)
                        goto clear;
                /* Test the search term */
                switch (match) {
                        case MATCH_QUERY_END:
                                succ = g_str_has_suffix(test, term);
                                break;
                        case MATCH_QUERY_START:
                                succ = g_str_has_prefix(test, term);
                                break;
                        case MATCH_QUERY_EXACT:
                                succ = g_str_equal(test, term);
                                break;
                        default:
                                break;
                }
                if (succ) {
                        g_ptr_array_add(_results, media);
                        count += 1;
                        goto next;
                }
clear:
                free_media_info(media);
                /* Visit the next key */
next:
                nextkey = gdbm_nextkey(self->priv->db, key);
                free(key.dptr);
                key = nextkey;
                test = NULL;
                succ = FALSE;
        }
        /* No results */
        if (_results->len < 1) {
                g_ptr_array_free(_results, TRUE);
                return ret;
        }
        ret = TRUE;
        *results = _results;
        g_mutex_unlock(&_lock);
        return ret;
}

/** PRIVATE **/
static gboolean budgie_db_serialize(MediaInfo *info, uint8_t **target)
{
        uint8_t* data = NULL;
        gboolean ret = FALSE;
        size_t length = 0;
        size_t size = 0;
        size_t offset = 0;

        /* 6 member fields */
        if (info->title)
                size = strlen(info->title)+1;
        if (info->artist)
                size += strlen(info->artist)+1;
        if (info->album)
                size += strlen(info->album)+1;
        if (info->band)
                size += strlen(info->band)+1;
        if (info->genre)
                size += strlen(info->genre)+1;
        size += strlen(info->mime)+1;

        /* 6 size fields */
        size += sizeof(unsigned int)*6;

        data = malloc(size);
        if (!data)
                goto end;

        /* Title */
        if (info->title)
                length = strlen(info->title)+1;
        else
                length = 0;
        memcpy(data, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (info->title)
                memcpy(data+offset, info->title, length);
        offset += length;

        /* Artist */
        if (info->artist)
                length = strlen(info->artist)+1;
        else
                length = 0;
        memcpy(data+offset, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (info->artist)
                memcpy(data+offset, info->artist, length);
        offset += length;

        /* Album */
        if (info->album)
                length = strlen(info->album)+1;
        else
                length = 0;
        memcpy(data+offset, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (info->album)
                memcpy(data+offset, info->album, length);
        offset += length;

        /* Band */
        if (info->band)
                length = strlen(info->band)+1;
        else
                length = 0;
        memcpy(data+offset, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (info->band)
                memcpy(data+offset, info->band, length);
        offset += length;

        /* Genre */
        if (info->genre)
                length = strlen(info->genre)+1;
        else
                length = 0;
        memcpy(data+offset, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (info->genre)
                memcpy(data+offset, info->genre, length);
        offset += length;

        /* Mime */
        length = strlen(info->mime)+1;
        memcpy(data+offset, &length, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        memcpy(data+offset, info->mime, length);


        ret = TRUE;
        *target = data;
end:
        if (!ret && data)
                free(data);
        return ret;
}

static gboolean budgie_db_deserialize(uint8_t* source, MediaInfo **target)
{
        MediaInfo *ret = NULL;
        gchar *title = NULL;
        gchar *artist = NULL;
        gchar *album = NULL;
        gchar *band = NULL;
        gchar *genre = NULL;
        gchar *mime = NULL;
        unsigned int title_len, artist_len;
        unsigned int album_len, genre_len;
        unsigned int band_len, mime_len;
        size_t offset = 0;
        gboolean op = FALSE;

        ret = malloc(sizeof(MediaInfo));
        if (!ret)
                goto end;

        memset(ret, 0, sizeof(MediaInfo));
        /* Title */
        memcpy(&title_len, source, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (title_len > 0) {
                title = malloc(title_len);
                memcpy(title, source+offset, title_len);
        }
        offset += title_len;

        /* Artist */
        memcpy(&artist_len, source+offset, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (artist_len > 0) {
                artist = malloc(artist_len);
                memcpy(artist, source+offset, artist_len);
        }
        offset += artist_len;

        /* Album */
        memcpy(&album_len, source+offset, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (album_len > 0) {
                album = malloc(album_len);
                memcpy(album, source+offset, album_len);
        }
        offset += album_len;

        /* Band */
        memcpy(&band_len, source+offset, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (band_len > 0) {
                band = malloc(band_len);
                memcpy(band, source+offset, band_len);
        }
        offset += band_len;

        /* Genre */
        memcpy(&genre_len, source+offset, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        if (genre_len > 0) {
                genre = malloc(genre_len);
                memcpy(genre, source+offset, genre_len);
        }
        offset += genre_len;

        /* Mime */
        memcpy(&mime_len, source+offset, sizeof(unsigned int));
        offset += sizeof(unsigned int);
        mime = malloc(mime_len);
        memcpy(mime, source+offset, mime_len);

        offset += mime_len; /* Reserved */
        /* Copy the data instead of exposing internals */
        if (title)
                ret->title = g_strdup(title);
        if (artist)
                ret->artist = g_strdup(artist);
        if (album)
                ret->album = g_strdup(album);
        if (band)
                ret->band = g_strdup(band);
        if (genre)
                ret->genre = g_strdup(genre);

        ret->mime = g_strdup(mime);
        op = TRUE;
        *target = ret;

end:
        if (title)
                free(title);
        if (artist)
                free(artist);
        if (album)
                free(album);
        if (band)
                free(band);
        if (genre)
                free(genre);
        if (mime)
                free(mime);
        return op;
}

gint budgie_db_sort(gconstpointer a, gconstpointer b)
{
        MediaInfo* m1 = NULL;
        MediaInfo* m2 = NULL;

        /* Alphabetically compare */
        m1 = *(MediaInfo**)a;
        m2 = *(MediaInfo**)b;

        if (!m1 || !m2)
                return 0;

        return g_ascii_strcasecmp(m1->title, m2->title);
}
