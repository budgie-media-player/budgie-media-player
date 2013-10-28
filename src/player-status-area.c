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
#include "player-status-area.h"

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
        GtkStyleContext *context;

        self->priv = player_status_area_get_instance_private(self);

        /* Construct our dummy label */
        label = gtk_label_new("MusicPlayer");
        gtk_container_add(GTK_CONTAINER(self), label);
        gtk_widget_set_name(label, "title");
        gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
        self->label = label;

        context = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(context, "dim-label");

        gtk_widget_set_size_request(GTK_WIDGET(self), 300, 70);
}

static void player_status_area_dispose(GObject *object)
{
        PlayerStatusArea *self;

        self = PLAYER_STATUS_AREA(object);
        if (self->priv->title_string)
                g_free(self->priv->title_string);

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
