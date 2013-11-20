/*
 * player-view.c
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

#include "player-view.h"

G_DEFINE_TYPE(PlayerView, player_view, GTK_TYPE_BIN)

/* Used only in append_track */
struct TrackAddStore {
        GtkListStore *model;
        GtkTreeIter iter;
};

/* Boilerplate GObject code */
static void player_view_class_init(PlayerViewClass *klass);
static void player_view_init(PlayerView *self);
static void player_view_dispose(GObject *object);

/* Initialisation */
static void player_view_class_init(PlayerViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &player_view_dispose;
}

static void player_view_init(PlayerView *self)
{
        GtkWidget *scroller;
        GtkWidget *tree;
        GtkTreeViewColumn *column;
        GtkCellRenderer *cell_text;

        /* Create our TreeView */
        tree = gtk_tree_view_new();
        self->tree = tree;
        cell_text = gtk_cell_renderer_text_new();

        /* Playing column */
        column = gtk_tree_view_column_new_with_attributes("",
                cell_text, "markup", PLAYER_COLUMN_STATUS, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

        /* Name */
        cell_text = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Name",
                cell_text, "text", PLAYER_COLUMN_NAME, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
        g_object_set(cell_text, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, PLAYER_COLUMN_NAME);

        /* Time not yet handled
        cell_text = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Time",
                cell_text, "text", PLAYER_COLUMN_TIME, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
        gtk_tree_view_column_set_resizable(column, TRUE);*/

        /* Artist */
        cell_text = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Artist",
                cell_text, "text", PLAYER_COLUMN_ARTIST, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, PLAYER_COLUMN_ARTIST);

        /* Album */
        cell_text = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Album",
                cell_text, "text", PLAYER_COLUMN_ALBUM, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, PLAYER_COLUMN_ALBUM);

        /* Genre */
        cell_text = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Genre",
                cell_text, "text", PLAYER_COLUMN_GENRE, NULL);
        g_object_set(cell_text, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_sort_column_id(column, PLAYER_COLUMN_GENRE);

        scroller = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
                GTK_SHADOW_ETCHED_IN);
        gtk_container_add(GTK_CONTAINER(scroller), tree);
        gtk_container_add(GTK_CONTAINER(self), scroller);
}

static void player_view_dispose(GObject *object)
{
        /* Destruct */
        G_OBJECT_CLASS (player_view_parent_class)->dispose (object);
}

/* Utility; return a new PlayerView */
GtkWidget* player_view_new(void)
{
        PlayerView *self;

        self = g_object_new(PLAYER_VIEW_TYPE, NULL);
        return GTK_WIDGET(self);
}

/* Append a track to our model */
static void append_track(gpointer data, gpointer p_store)
{
        struct TrackAddStore* store;
        MediaInfo *media;

        media = (MediaInfo*)data;
        store = (struct TrackAddStore*)p_store;

        gtk_list_store_append(store->model, &(store->iter));
        gtk_list_store_set(store->model, &(store->iter),
                PLAYER_COLUMN_NAME, media->title, -1);

        if (media->artist)
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_ARTIST, media->artist, -1);
        else
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_ARTIST, "Unknown Artist", -1);
        if (media->album)
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_ALBUM, media->album, -1);
        else
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_ALBUM, "Unknown Album", -1);
        if (media->genre)
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_GENRE, media->genre, -1);
        else
                gtk_list_store_set(store->model, &(store->iter),
                        PLAYER_COLUMN_GENRE, "Unknown Genre", -1);

        gtk_list_store_set(store->model, &(store->iter),
                PLAYER_COLUMN_INFO, media, -1);
}

void player_view_set_list(PlayerView *self, GSList* list)
{
        struct TrackAddStore store;
        memset(&store, 0, sizeof(struct TrackAddStore));

        if (!list)
                return;

        store.model = gtk_list_store_new(PLAYER_COLUMNS,
                G_TYPE_STRING, /* Playing char */
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

        g_slist_foreach(list, &append_track, (gpointer)&store);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store.model),
                PLAYER_COLUMN_NAME, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->tree),
                GTK_TREE_MODEL(store.model));
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(self->tree),
                PLAYER_COLUMN_NAME);
}

MediaInfo* player_view_get_current_selection(PlayerView *self)
{
        MediaInfo *media;
        GtkTreeSelection *selection = NULL;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;
        GValue value = G_VALUE_INIT;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree));
        if (!gtk_tree_selection_get_selected(selection, &model, &iter))
                return NULL;
        gtk_tree_model_get_value(model, &iter, PLAYER_COLUMN_INFO, &value);

        media = g_value_get_pointer(&value);
        g_value_unset(&value);

        return media;
}

void player_view_set_current_selection(PlayerView *self, MediaInfo *media)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        GtkTreePath *path;
        GtkTreeIter row;
        gboolean found = TRUE;
        GValue value = G_VALUE_INIT;
        GtkTreeSelection *selection = NULL;
        MediaInfo *test;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree));
            
        gtk_tree_model_get_iter_first(model, &iter);
        while (found) {
                gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, PLAYER_COLUMN_INFO, &value);
                test = g_value_get_pointer(&value);
                g_value_unset(&value);

                if (test == media) {
                        row = iter;
                        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                                PLAYER_COLUMN_STATUS, PLAY_CHAR, -1);
                } else {
                        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                                PLAYER_COLUMN_STATUS, "", -1);
                }
                found = gtk_tree_model_iter_next(model, &iter);
        }
        path = gtk_tree_model_get_path(model, &row);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(self->tree), path, NULL,
                TRUE, 0.5f, 0.5f);
        gtk_tree_path_free(path);
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree));
        gtk_tree_selection_select_iter(selection, &row);
}

MediaInfo* player_view_get_next_item(PlayerView *self)
{
        MediaInfo *media;
        GtkTreeSelection *selection = NULL;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;
        GValue value = G_VALUE_INIT;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree));
        if (!gtk_tree_selection_get_selected(selection, &model, &iter))
                return NULL;


        if (!gtk_tree_model_iter_next(model, &iter))
                return NULL;
        gtk_tree_model_get_value(model, &iter, PLAYER_COLUMN_INFO, &value);

        media = g_value_get_pointer(&value);
        g_value_unset(&value);

        return media;
}

MediaInfo* player_view_get_previous_item(PlayerView *self)
{
        MediaInfo *media;
        GtkTreeSelection *selection = NULL;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;
        GValue value = G_VALUE_INIT;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree));
        if (!gtk_tree_selection_get_selected(selection, &model, &iter))
                return NULL;

        if (!gtk_tree_model_iter_previous(model, &iter))
                return NULL;
        gtk_tree_model_get_value(model, &iter, PLAYER_COLUMN_INFO, &value);

        media = g_value_get_pointer(&value);
        g_value_unset(&value);

        return media;
}

MediaInfo *player_view_get_random_item(PlayerView *self)
{
        guint32 rand;
        guint length;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;
        int i;
        MediaInfo *media = NULL;
        GValue value = G_VALUE_INIT;
        GRand *random;

        random = g_rand_new();

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->tree));
        length = gtk_tree_model_iter_n_children(model, NULL);
        rand = g_rand_int_range(random, 0, length);
        g_rand_free(random);

        gtk_tree_model_get_iter_first(model, &iter);
        for (i = 0; i < rand; i++) {
                gtk_tree_model_iter_next(model, &iter);
        }
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, PLAYER_COLUMN_INFO, &value);
        media = g_value_get_pointer(&value);
        g_value_unset(&value);

        return media;
}
