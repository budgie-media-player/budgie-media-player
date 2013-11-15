/*
 * player-status-area.c
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
#include <gst/gst.h>

#include "player-status-area.h"
#include "util.h"

G_DEFINE_TYPE_WITH_PRIVATE(PlayerStatusArea, player_status_area, GTK_TYPE_EVENT_BOX);

/* Initialisation */
static void player_status_area_class_init(PlayerStatusAreaClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &player_status_area_dispose;
}

static void player_status_area_init(PlayerStatusArea *self)
{
        GtkWidget *label;
        GtkWidget *time_label, *remaining_label;
        GtkWidget *slider;
        GtkStyleContext *context;
        GtkWidget *box, *bottom;

        self->priv = player_status_area_get_instance_private(self);

        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        /* Construct our main label */
        label = gtk_label_new("Budgie");
        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
        gtk_widget_set_name(label, "title");
        gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
        self->label = label;

        context = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(context, "dim-label");

        /* Bottom row */
        bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(box), bottom, FALSE, FALSE, 0);

        /* Time passed */
        time_label = gtk_label_new("");
        context = gtk_widget_get_style_context(time_label);
        gtk_style_context_add_class(context, "dim-label");
        gtk_box_pack_start(GTK_BOX(bottom), time_label, FALSE, FALSE, 2);
        self->time_label = time_label;

        /* Slider */
        slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
            0.0, 1.0, 1.0/100);
        gtk_box_pack_start(GTK_BOX(bottom), slider, TRUE, TRUE, 2);
        self->slider = slider;
        gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);

        /* Remaining time */
        remaining_label = gtk_label_new("");
        context = gtk_widget_get_style_context(remaining_label);
        gtk_style_context_add_class(context, "dim-label");
        gtk_box_pack_end(GTK_BOX(bottom), remaining_label, FALSE, FALSE, 2);
        self->remaining_label = remaining_label;

        gtk_container_add(GTK_CONTAINER(self), box);

        gtk_widget_set_size_request(GTK_WIDGET(self), 300, 70);
}

static void player_status_area_dispose(GObject *object)
{
        PlayerStatusArea *self;

        self = PLAYER_STATUS_AREA(object);
        if (self->priv->title_string)
                g_free(self->priv->title_string);
        if (self->priv->time_string)
                g_free(self->priv->time_string);
        if (self->priv->remaining_string)
                g_free(self->priv->remaining_string);

        /* Destruct */
        G_OBJECT_CLASS (player_status_area_parent_class)->dispose (object);
}

/* Utility; return a new PlayerStatusArea */
GtkWidget* player_status_area_new(void)
{
        PlayerStatusArea *self;

        self = g_object_new(PLAYER_STATUS_AREA_TYPE, NULL);
        return GTK_WIDGET(self);
}

void player_status_area_set_media(PlayerStatusArea *self, MediaInfo *info)
{
        if (self->priv->title_string)
                g_free(self->priv->title_string);

        if (info->artist)
                self->priv->title_string = g_strdup_printf("<big>%s</big>\n<small>%s</small>",
                        info->title, info->artist);
        else
                self->priv->title_string = g_strdup_printf("<big>%s</big>\n", info->title);
        gtk_label_set_markup(GTK_LABEL(self->label), self->priv->title_string);
}

void player_status_area_set_media_time(PlayerStatusArea *self, gint64 max, gint64 current)
{
        if (self->priv->time_string)
                g_free(self->priv->time_string);
        if (self->priv->remaining_string)
                g_free(self->priv->remaining_string);

        /* Clear info */
        if (max < 0) {
                gtk_widget_set_visible(self->slider, FALSE);
                gtk_label_set_markup(GTK_LABEL(self->time_label), "");
                gtk_label_set_markup(GTK_LABEL(self->remaining_label), "");
                return;
        }
        gtk_widget_set_visible(self->slider, TRUE);
        gint64 remaining, elapsed;

        /* Update slider */
        gtk_range_set_range(GTK_RANGE(self->slider), 0, max);
        gtk_range_set_value(GTK_RANGE(self->slider), current);

        remaining = (max - current)/GST_SECOND;
        elapsed = current/GST_SECOND;
        self->priv->time_string = format_seconds(elapsed, FALSE);
        self->priv->remaining_string = format_seconds(remaining, TRUE);

        /* Update labels */
        gtk_label_set_markup(GTK_LABEL(self->time_label), self->priv->time_string);
        gtk_label_set_markup(GTK_LABEL(self->remaining_label), self->priv->remaining_string);
}
