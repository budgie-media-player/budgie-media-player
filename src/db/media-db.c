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
#include "media-db.h"

G_DEFINE_TYPE_WITH_PRIVATE(MediaDB, media_db, MEDIA_DB_TYPE);

/* Initialisation */
static void media_db_class_init(MediaDBClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &media_db_dispose;
}

static void media_db_init(MediaDB *self)
{
        /*TODO: Implement */
}

static void media_db_dispose(GObject *object)
{
        MediaDB *self;

        self = MEDIA_DB(object);

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
