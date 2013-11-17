/*
 * budgie-media-view.c
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
#include <stdlib.h>
#include <string.h>

#include "budgie-media-view.h"

G_DEFINE_TYPE(BudgieMediaView, budgie_media_view, GTK_TYPE_BIN);

enum {
        PROP_0, PROP_DATABASE, N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Initialisation */
static void budgie_media_view_class_init(BudgieMediaViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        obj_properties[PROP_DATABASE] =
        g_param_spec_pointer("database", "Database", "Database",
                G_PARAM_READWRITE);

        g_object_class->dispose = &budgie_media_view_dispose;
        g_object_class->set_property = &budgie_media_view_set_property;
        g_object_class->get_property = &budgie_media_view_get_property;
        g_object_class_install_properties(g_object_class, N_PROPERTIES,
                obj_properties);
}

static void budgie_media_view_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
        BudgieMediaView *self;

        self = BUDGIE_MEDIA_VIEW(object);
        switch (prop_id) {
                case PROP_DATABASE:
                        self->db = g_value_get_pointer((GValue*)value);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                prop_id, pspec);
                        break;
        }
}

static void budgie_media_view_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
        BudgieMediaView *self;

        self = BUDGIE_MEDIA_VIEW(object);
        switch (prop_id) {
                case PROP_DATABASE:
                        g_value_set_pointer((GValue *)value, (gpointer)self->db);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                prop_id, pspec);
                        break;
        }
}

static void budgie_media_view_init(BudgieMediaView *self)
{
        /* Not yet implemented */
}

static void budgie_media_view_dispose(GObject *object)
{
        BudgieMediaView *self;

        self = BUDGIE_MEDIA_VIEW(object);

        /* Destruct */
        G_OBJECT_CLASS (budgie_media_view_parent_class)->dispose (object);
}

/* Utility; return a new BudgieMediaView */
GtkWidget* budgie_media_view_new(BudgieDB *database)
{
        BudgieMediaView *self;

        self = g_object_new(BUDGIE_MEDIA_VIEW_TYPE, "database", database, NULL);
        return GTK_WIDGET(self);
}
