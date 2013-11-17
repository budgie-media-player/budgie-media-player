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

enum {
        ALBUM_TITLE,
        ALBUM_PIXBUF,
        ALBUM_COLUMNS
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Initialisation */
static void budgie_media_view_class_init(BudgieMediaViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        obj_properties[PROP_DATABASE] =
        g_param_spec_pointer("database", "Database", "Database",
                G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);

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
                        update_db(self);
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
        GtkWidget *icon_view, *scroll;

        /* Set up our icon view */
        icon_view = gtk_icon_view_new();
        self->icon_view = icon_view;
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), icon_view);
        gtk_container_add(GTK_CONTAINER(self), scroll);

        /* Relevant columns */
        gtk_icon_view_set_markup_column(GTK_ICON_VIEW(icon_view),
                ALBUM_TITLE);
        gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view),
                ALBUM_PIXBUF);
        gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), 300);
        gtk_icon_view_set_columns(GTK_ICON_VIEW(icon_view), 2);
        gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(icon_view),
                GTK_ORIENTATION_HORIZONTAL);
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

static void update_db(BudgieMediaView *self)
{
        GtkIconTheme *theme;
        GtkListStore *model;
        GPtrArray *albums = NULL;
        GPtrArray *results = NULL;
        GdkPixbuf *pixbuf;
        GtkTreeIter iter;
        gchar *album = NULL;
        gchar *markup = NULL;
        MediaInfo *current;
        int i;

        /* No albums */
        if (!budgie_db_get_all_by_field(self->db, MEDIA_QUERY_ALBUM, &albums))
                return;

        /* We don't *yet* support album art, use generic symbol */
        theme = gtk_icon_theme_get_default();
        pixbuf = gtk_icon_theme_load_icon(theme, "folder-music-symbolic",
                64, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
        model = gtk_list_store_new(ALBUM_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF);

        for (i=0; i < albums->len; i++) {
                album = (gchar *)albums->pdata[i];
                /* Try to gain at least one artist */
                if (!budgie_db_search_field(self->db, MEDIA_QUERY_ALBUM,
                        MATCH_QUERY_EXACT, album, 1, &results))
                        goto fail;
                current = results->pdata[0];
                if (current->album == NULL)
                        goto albumfail;

                /* Pretty label */
                markup = g_markup_printf_escaped("<b>%s</b>\n<span color='grey'>%s</span>",
                        current->album, current->artist);
                gtk_list_store_append(model, &iter);
                gtk_list_store_set(model, &iter, ALBUM_TITLE, markup, -1);
                gtk_list_store_set(model, &iter, ALBUM_PIXBUF, pixbuf, -1);
albumfail:
                free_media_info(current);
                g_ptr_array_free(results, TRUE);
fail:
                g_free(album);
        }
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
                ALBUM_TITLE, GTK_SORT_ASCENDING);
        gtk_icon_view_set_model(GTK_ICON_VIEW(self->icon_view),
                GTK_TREE_MODEL(model));
        g_ptr_array_free(albums, TRUE);

        g_object_unref(pixbuf);
}
