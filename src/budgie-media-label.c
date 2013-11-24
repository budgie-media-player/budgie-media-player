/*
 * budgie-media-label.c
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

#include "budgie-media-label.h"

G_DEFINE_TYPE(BudgieMediaLabel, budgie_media_label, GTK_TYPE_BOX)

/* Boilerplate GObject code */
static void budgie_media_label_class_init(BudgieMediaLabelClass *klass);
static void budgie_media_label_init(BudgieMediaLabel *self);
static void budgie_media_label_dispose(GObject *object);

static void budgie_media_label_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);

static void budgie_media_label_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);

enum {
        PROP_0, PROP_INFO, N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Initialisation */
static void budgie_media_label_class_init(BudgieMediaLabelClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        obj_properties[PROP_INFO] =
        g_param_spec_pointer("info", "Info", "Info",
                G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);

        g_object_class->dispose = &budgie_media_label_dispose;
        g_object_class->set_property = &budgie_media_label_set_property;
        g_object_class->get_property = &budgie_media_label_get_property;
        g_object_class_install_properties(g_object_class, N_PROPERTIES,
                obj_properties);
}

static void budgie_media_label_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
        BudgieMediaLabel *self;

        self = BUDGIE_MEDIA_LABEL(object);
        switch (prop_id) {
                case PROP_INFO:
                        self->info = g_value_get_pointer((GValue*)value);
                        /* TODO: Add update_ui */
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                prop_id, pspec);
                        break;
        }
}

static void budgie_media_label_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
        BudgieMediaLabel *self;

        self = BUDGIE_MEDIA_LABEL(object);
        switch (prop_id) {
                case PROP_INFO:
                        g_value_set_pointer((GValue *)value, (gpointer)self->info);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                prop_id, pspec);
                        break;
        }
}

static void budgie_media_label_init(BudgieMediaLabel *self)
{
        /* TODO: Implement */
}

static void budgie_media_label_dispose(GObject *object)
{
        /* Destruct */
        G_OBJECT_CLASS (budgie_media_label_parent_class)->dispose (object);
}

/* Utility; return a new BudgieMediaLabel */
GtkWidget* budgie_media_label_new(MediaInfo *info)
{
        BudgieMediaLabel *self;

        self = g_object_new(BUDGIE_MEDIA_LABEL_TYPE,
                "info", info,
                "orientation", GTK_ORIENTATION_HORIZONTAL,
                NULL);
        return GTK_WIDGET(self);
}
