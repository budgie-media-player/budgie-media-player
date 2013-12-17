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

/* Private methods */
static void update_ui(BudgieMediaLabel *self);

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
        PROP_0, PROP_INFO, PROP_PLAYING, N_PROPERTIES
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
        obj_properties[PROP_PLAYING] =
        g_param_spec_boolean("playing", "Playing", "Playing",
                FALSE, G_PARAM_READWRITE);

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
                        update_ui(self);
                        break;
                case PROP_PLAYING:
                        self->playing = g_value_get_boolean((GValue*)value);
                        budgie_media_label_set_playing(self, self->playing);
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
                case PROP_PLAYING:
                        g_value_set_boolean((GValue *)value, self->playing);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                prop_id, pspec);
                        break;
        }
}

static void budgie_media_label_init(BudgieMediaLabel *self)
{
        GtkWidget *label;

        label = gtk_label_new("\u25B6");
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        self->play_sign = label;
        gtk_box_pack_start(GTK_BOX(self), label, FALSE, FALSE, 0);

        label = gtk_label_new("");
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        self->display = label;
        gtk_box_pack_start(GTK_BOX(self), label, TRUE, TRUE, 0);

        gtk_widget_show_all(self->display);
}

static void budgie_media_label_dispose(GObject *object)
{
        BudgieMediaLabel *self;

        self = BUDGIE_MEDIA_LABEL(object);
        if (self->info) {
                free_media_info(self->info);
                /* GtkListBox actually revisits us twice sometimes.. */
                self->info = NULL;
        }
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
                "playing", FALSE,
                NULL);
        return GTK_WIDGET(self);
}

void budgie_media_label_set_playing(BudgieMediaLabel *self,
                                    gboolean playing)
{
        GtkStyleContext *style;

        style = gtk_widget_get_style_context(self->display);
        if (playing) {
                gtk_style_context_remove_class(style, "info-label");
                g_object_set(self->display, "margin-left", 15, NULL);
        } else {
                gtk_style_context_add_class(style, "info-label");
                g_object_set(self->display, "margin-left", 25, NULL);
        }

        gtk_widget_set_visible(self->play_sign, playing);
        self->playing = playing;
}

static void update_ui(BudgieMediaLabel *self)
{
        gtk_label_set_text(GTK_LABEL(self->display), self->info->title);
}
