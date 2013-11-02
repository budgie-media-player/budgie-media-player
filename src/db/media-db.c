/*
 * media-db.c
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

#include "media-db.h"

G_DEFINE_TYPE_WITH_PRIVATE(MediaDB, media_db, G_TYPE_OBJECT);

/* Private utilities */
static gboolean media_db_serialize(MediaInfo *info, uint8_t **target);

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
        if (info->genre)
                g_free(info->genre);
        if (info->path)
                g_free(info->path);
}

/* Initialisation */
static void media_db_class_init(MediaDBClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &media_db_dispose;
}

static void media_db_init(MediaDB *self)
{
        const gchar *config;
        self->priv = media_db_get_instance_private(self);

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

static void media_db_dispose(GObject *object)
{
        MediaDB *self;

        self = MEDIA_DB(object);
        if (self->priv->storage_path)
                g_free(self->priv->storage_path);

        gdbm_close(self->priv->db);
        /* Destruct */
        G_OBJECT_CLASS (media_db_parent_class)->dispose (object);
}

/* Utility; return a new MediaDB */
MediaDB* media_db_new(void)
{
        MediaDB *self;

        self = g_object_new(MEDIA_DB_TYPE, NULL);
        return MEDIA_DB(self);
}

void media_db_store_media(MediaDB *self, MediaInfo *info)
{
        datum value;
        uint8_t *store = NULL;

        /* Path is the unique key */
        datum key = { (char*)info->path, strlen(info->path)+1 };

        if (!media_db_serialize(info, &store)) {
                g_warning("Unable to serialize data!");
                goto end;
        }
        value.dptr = (char*)store;
        value.dsize = malloc_usable_size(store);;

        gdbm_store(self->priv->db, key, value, GDBM_REPLACE);
end:
        if (store)
                free(store);
}

/** PRIVATE **/
static gboolean media_db_serialize(MediaInfo *info, uint8_t **target)
{
        uint8_t* data = NULL;
        gboolean ret = FALSE;
        unsigned int length;
        unsigned int size;
        unsigned int offset = 0;

        /* 4 member fields */
        if (info->title)
                size = strlen(info->title)+1;
        if (info->artist)
                size += strlen(info->artist)+1;
        if (info->album)
                size += strlen(info->album)+1;
        if (info->genre)
                size += strlen(info->genre)+1;

        /* 4 size fields */
        size += sizeof(unsigned int)*4;

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

        ret = TRUE;
        *target = data;
end:
        if (!ret && data)
                free(data);
        return ret;
}
