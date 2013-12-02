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
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "budgie-media-view.h"
#include "budgie-media-label.h"
#include "util.h"

G_DEFINE_TYPE(BudgieMediaView, budgie_media_view, GTK_TYPE_BIN)

/* Boilerplate GObject code */
static void budgie_media_view_class_init(BudgieMediaViewClass *klass);
static void budgie_media_view_init(BudgieMediaView *self);
static void budgie_media_view_dispose(GObject *object);

static gboolean update_db_t(gpointer userdata);
static gpointer update_db(gpointer userdata);
static void set_display(BudgieMediaView *self, GPtrArray *results);
static void item_activated_cb(GtkWidget *widget,
                              GtkTreePath *tree_path,
                              gpointer userdata);
static void button_clicked_cb(GtkWidget *widget, gpointer userdata);
static void list_selection_cb(GtkListBox *list, GtkListBoxRow *row,
                              gpointer userdata);

static gint sort_list(GtkListBoxRow *row1,
                      GtkListBoxRow *row2,
                      gpointer userdata);
static GdkPixbuf *beautify(GdkPixbuf **source,
                           GdkPixbuf *base,
                           GdkPixbuf *overlay);

static void budgie_media_view_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);

static void budgie_media_view_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);

enum {
        PROP_0, PROP_DATABASE, N_PROPERTIES
};

enum {
        ALBUM_TITLE = 0,
        ALBUM_PIXBUF,
        ALBUM_ALBUM,
        ALBUM_ARTIST,
        ALBUM_ART_PATH,
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

        g_signal_new("media-selected",
                G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                1, G_TYPE_POINTER);
}

static gboolean update_db_t(gpointer userdata)
{
        __attribute__((unused)) GThread *thread;
        thread = g_thread_new("update-db", &update_db, userdata);
        return FALSE;
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
                        g_idle_add(update_db_t, (gpointer)self);
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
        GtkWidget *controls;
        GtkWidget *main_layout;
        GtkWidget *stack;
        GtkWidget *icon_view, *scroll;
        GtkWidget *button;
        GtkStyleContext *style;
        GtkWidget *image, *list, *view_page;
        GtkWidget *top_frame;
        GtkWidget *label;
        GtkWidget *info_box;

        /* Main layout of view */
        top_frame = gtk_frame_new(NULL);
        main_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(self), top_frame);
        gtk_container_add(GTK_CONTAINER(top_frame), main_layout);

        style = gtk_widget_get_style_context(top_frame);
        gtk_style_context_add_class(style, "view");
        gtk_style_context_add_class(style, "content-view");

        /* To switch between media types */
        controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_set_border_width(GTK_CONTAINER(controls), 3);
        style = gtk_widget_get_style_context(controls);
        gtk_style_context_add_class(style, "linked");
        gtk_box_pack_start(GTK_BOX(main_layout), controls, FALSE, FALSE, 0);

        button = gtk_radio_button_new_with_label(NULL, "Albums");
        self->albums = button;
        g_object_set(button, "draw-indicator", FALSE, NULL);
        g_signal_connect(button, "clicked", G_CALLBACK(button_clicked_cb),
                (gpointer)self);
        gtk_widget_set_can_focus(button, FALSE);
        gtk_box_pack_start(GTK_BOX(controls), button, FALSE, FALSE, 0);

        button = gtk_radio_button_new_with_label_from_widget(
                GTK_RADIO_BUTTON(self->albums), "Songs");
        self->songs = button;
        g_object_set(button, "draw-indicator", FALSE, NULL);
        g_signal_connect(button, "clicked", G_CALLBACK(button_clicked_cb),
                (gpointer)self);
        gtk_widget_set_can_focus(button, FALSE);
        gtk_box_pack_start(GTK_BOX(controls), button, FALSE, FALSE, 0);

        button = gtk_radio_button_new_with_label_from_widget(
                GTK_RADIO_BUTTON(self->albums), "Videos");
        self->videos = button;
        g_object_set(button, "draw-indicator", FALSE, NULL);
        g_signal_connect(button, "clicked", G_CALLBACK(button_clicked_cb),
                (gpointer)self);
        gtk_widget_set_can_focus(button, FALSE);
        gtk_box_pack_start(GTK_BOX(controls), button, FALSE, FALSE, 0);

        gtk_widget_set_halign(controls, GTK_ALIGN_CENTER);

        /* Stack happens to be our main content */
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack),
                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        gtk_box_pack_start(GTK_BOX(main_layout), stack, TRUE, TRUE, 0);
        self->stack = stack;

        /* Set up our icon view */
        icon_view = gtk_icon_view_new();
        self->icon_view = icon_view;
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(scroll),
                TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), icon_view);
        gtk_stack_add_named(GTK_STACK(stack), scroll, "albums");

        style = gtk_widget_get_style_context(icon_view);
        gtk_style_context_add_class(style, "view");
        gtk_style_context_add_class(style, "content-view");
        style = gtk_widget_get_style_context(scroll);
        gtk_style_context_add_class(style, "osd");

        /* Relevant columns */
        gtk_icon_view_set_markup_column(GTK_ICON_VIEW(icon_view),
                ALBUM_TITLE);
        gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view),
                ALBUM_PIXBUF);
        gtk_icon_view_set_item_padding(GTK_ICON_VIEW(icon_view), 15);
        gtk_icon_view_set_spacing(GTK_ICON_VIEW(icon_view), 10);
        gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), 290);
        gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(icon_view),
                GTK_ORIENTATION_HORIZONTAL);

        /* Fire on single click, display tracks */
        gtk_icon_view_set_activate_on_single_click(GTK_ICON_VIEW(icon_view),
                TRUE);
        g_signal_connect(icon_view, "item-activated",
                G_CALLBACK(item_activated_cb), (gpointer)self);

        /* Set up our view page (tracks) */
        view_page = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_set_border_width(GTK_CONTAINER(view_page), 0);
        gtk_stack_add_named(GTK_STACK(stack), view_page, "tracks");

        /* Info box stores image, count, etc. */
        info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(view_page), info_box, FALSE, FALSE, 0);
        gtk_widget_set_valign(info_box, GTK_ALIGN_FILL);

        /* Image for album art */
        image = gtk_image_new();
        gtk_widget_set_halign(image, GTK_ALIGN_START);
        gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
        g_object_set(image, "margin-top", 20, NULL);
        g_object_set(image, "margin-left", 20, NULL);
        g_object_set(image, "margin-right", 20, NULL);
        gtk_box_pack_start(GTK_BOX(info_box), image, FALSE, FALSE, 0);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 256);
        self->image = image;

        /* Album, etc, */
        label = gtk_label_new("");
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 30);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        g_object_set(label, "margin-bottom", 10,
                "margin-left", 5, NULL);
        self->current_label = label;
        gtk_box_pack_start(GTK_BOX(info_box), label, FALSE, FALSE, 0);
        style = gtk_widget_get_style_context(label);

        /* Count label */
        label = gtk_label_new("");
        self->count_label = label;
        gtk_box_pack_start(GTK_BOX(info_box), label, FALSE, FALSE, 0);
        gtk_widget_set_valign(label, GTK_ALIGN_END);
        style = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(style, "dim-label");

        /* Append tracks to a pretty listbox */
        list = gtk_list_box_new();
        g_signal_connect(list, "row-activated",
                G_CALLBACK(list_selection_cb), (gpointer)self);
        gtk_widget_set_halign(list, GTK_ALIGN_FILL);
        self->list = list;

        /* Sort the column */
        gtk_list_box_set_sort_func(GTK_LIST_BOX(list), sort_list,
                (gpointer)self, NULL);
        /* Scroller for the listbox */
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(scroll),
                TRUE);
        gtk_container_add(GTK_CONTAINER(scroll), list);
        gtk_box_pack_start(GTK_BOX(view_page), scroll, TRUE, TRUE, 0);
        g_object_set(scroll, "margin-top", 20, NULL);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
                GTK_SHADOW_NONE);

        style = gtk_widget_get_style_context(list);
        /* Grainy style seen in Adwaita, might remove it and add padding */
        gtk_style_context_add_class(style, "view");
        gtk_style_context_add_class(style, "content-view");
        /* Better looking scroll bars */
        style = gtk_widget_get_style_context(scroll);
        gtk_style_context_add_class(style, "osd");
}

static void budgie_media_view_dispose(GObject *object)
{
        BudgieMediaView *self;

        self = BUDGIE_MEDIA_VIEW(object);

        if (self->results) {
                g_ptr_array_free(self->results, TRUE);
                self->results = NULL;
        }

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

static gpointer update_db(gpointer userdata)
{
        BudgieMediaView *self;
        GtkListStore *model;
        GPtrArray *albums = NULL;
        GPtrArray *results = NULL;
        GdkPixbuf *pixbuf;
        GdkPixbuf *base, *overlay;
        GtkTreeIter iter;
        gchar *album = NULL;
        gchar *markup = NULL;
        MediaInfo *current;
        const gchar *cache;
        gchar *album_id = NULL, *path = NULL;
        int i;

        self = BUDGIE_MEDIA_VIEW(userdata);

        /* No albums */
        if (!budgie_db_get_all_by_field(self->db, MEDIA_QUERY_ALBUM, &albums))
                return NULL;

        cache = g_get_user_cache_dir();
        model = gtk_list_store_new(ALBUM_COLUMNS, G_TYPE_STRING,
                GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                G_TYPE_STRING);

        /* base and overlay image for album art */
        base = gdk_pixbuf_new_from_file(DATADIR "/budgie/album-base.png", NULL);
        overlay = gdk_pixbuf_new_from_file(DATADIR "/budgie/album-overlay.png", NULL);

        for (i=0; i < albums->len; i++) {
                album = (gchar *)albums->pdata[i];
                /* Try to gain at least one artist */
                if (!budgie_db_search_field(self->db, MEDIA_QUERY_ALBUM,
                        MATCH_QUERY_EXACT, album, 1, &results))
                        goto fail;
                current = results->pdata[0];
                if (current->album == NULL)
                        goto albumfail;

                album_id = albumart_name_for_media(current, "jpeg");
                path = g_strdup_printf("%s/media-art/%s", cache, album_id);
                g_free(album_id);
                pixbuf = gdk_pixbuf_new_from_file(path, NULL);
                if (!pixbuf)
                        pixbuf = beautify(NULL, base, overlay);
                else
                        pixbuf = beautify(&pixbuf, base, overlay);
                /* Pretty label */
                if (current->band)
                        markup = g_markup_printf_escaped("<big>%s\n<span color='darkgrey'>%s</span></big>",
                                current->album, current->band);
                else
                        markup = g_markup_printf_escaped("<big>%s\n<span color='darkgrey'>%s</span></big>",
                                current->album, current->artist);
                gtk_list_store_append(model, &iter);

                /* Set it in the list store */
                gtk_list_store_set(model, &iter,
                        ALBUM_TITLE, markup,
                        ALBUM_PIXBUF, pixbuf,
                        ALBUM_ALBUM, current->album,
                        ALBUM_ARTIST, current->artist,
                        ALBUM_ART_PATH, path,
                        -1);

                if (pixbuf)
                        g_object_unref(pixbuf);
                g_free(markup);

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
        g_object_unref(base);
        g_object_unref(overlay);

        return NULL;
}

static void item_activated_cb(GtkWidget *widget,
                              GtkTreePath *tree_path,
                              gpointer userdata)
{
        BudgieMediaView *self;
        GtkTreeModel *model;
        GtkTreeIter iter;
        GValue v_album = G_VALUE_INIT;
        GValue v_path = G_VALUE_INIT;
        GdkPixbuf *pixbuf;
        const char *album, *path;
        gchar *artist;
        GPtrArray *results = NULL;
        gchar *info_string = NULL;
        MediaInfo *current = NULL;

        /* Grab the model and iter */
        self = BUDGIE_MEDIA_VIEW(userdata);
        model = gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
        gtk_tree_model_get_iter(model, &iter, tree_path);

        /* Get the album in question */
        gtk_tree_model_get_value(model, &iter, ALBUM_ALBUM, &v_album);
        album = g_value_get_string(&v_album);

        /* Path of image */
        gtk_tree_model_get_value(model, &iter, ALBUM_ART_PATH, &v_path);
        path= g_value_get_string(&v_path);

        /* Consider splitting this into another function.. */
        gtk_stack_set_visible_child_name(GTK_STACK(self->stack),
                "tracks");

        /* Set the image */
        pixbuf = gdk_pixbuf_new_from_file_at_size(path, 256, 256, NULL);
        if (pixbuf)
                gtk_image_set_from_pixbuf(GTK_IMAGE(self->image), pixbuf);
        else
                gtk_image_set_from_icon_name(GTK_IMAGE(self->image),
                        "folder-music-symbolic", GTK_ICON_SIZE_INVALID);

        if (!budgie_db_search_field(self->db, MEDIA_QUERY_ALBUM,
                MATCH_QUERY_EXACT, (gchar*)album, -1, &results))
                goto end;

        current = (MediaInfo*)results->pdata[0];
        if (current->band)
                artist = current->band;
        else
                artist = current->artist;

        info_string = g_markup_printf_escaped(
                "<big>%s</big><span color='darkgrey'>\n%s</span>", album,
                artist);
        gtk_label_set_markup(GTK_LABEL(self->current_label),
                info_string);
        g_free(info_string);

        /* Got this far */
        set_display(self, results);
end:
        g_value_unset(&v_path);
        g_value_unset(&v_album);
}

static void button_clicked_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieMediaView *self;
        GPtrArray *results = NULL;

        self = BUDGIE_MEDIA_VIEW(userdata);

        if (widget == self->albums) {
                self->mode = MEDIA_MODE_ALBUMS;
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack),
                        "albums");
        } else if (widget == self->songs) {
                self->mode = MEDIA_MODE_SONGS;

                /* Populate all songs */
                if (!budgie_db_search_field(self->db, MEDIA_QUERY_MIME,
                        MATCH_QUERY_START, "audio/", -1, &results))
                        /** Raise a warning somewhere? */
                        g_warning("No tracks found");

                set_display(self, results);
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack),
                        "tracks");
        } else if (widget == self->videos) {
                self->mode = MEDIA_MODE_VIDEOS;

                /* Populate all songs */
                if (!budgie_db_search_field(self->db, MEDIA_QUERY_MIME,
                        MATCH_QUERY_START, "video/", -1, &results))
                        /** Raise a warning somewhere? */
                        g_warning("No tracks found");

                set_display(self, results);
                gtk_stack_set_visible_child_name(GTK_STACK(self->stack),
                        "tracks");
        }
}

static void set_display(BudgieMediaView *self, GPtrArray *results)
{
        /* Media infos */
        MediaInfo *current = NULL;
        int i;
        /* Item to append to list */
        GtkWidget *label;
        /* Info label */
        gchar *info_string = NULL;

        gtk_container_foreach(GTK_CONTAINER(self->list),
                (GtkCallback)gtk_widget_destroy, NULL);

        /* Only store one set at a time */
        if (self->results) {
                g_ptr_array_free(self->results, TRUE);
                self->results = NULL;
        }

        /* Extract the fields */
        for (i=0; i < results->len; i++) {
                current = (MediaInfo*)results->pdata[i];
                label = budgie_media_label_new(current);
                gtk_container_add(GTK_CONTAINER(self->list), label);
                gtk_widget_set_halign(label, GTK_ALIGN_START);
                /* Little bit of left padding */
                g_object_set(label, "margin-left", 10, NULL);
                gtk_widget_show(label);
        }


        switch (self->mode) {
                case MEDIA_MODE_SONGS:
                        gtk_image_set_from_icon_name(GTK_IMAGE(self->image),
                                "folder-music-symbolic", GTK_ICON_SIZE_INVALID);
                        if (results->len == 0)
                                info_string = g_strdup_printf("No songs");
                        else if (results->len == 1)
                                info_string = g_strdup_printf("%d song",
                                        results->len);
                        else
                                info_string = g_strdup_printf("%d songs",
                                        results->len);
                        break;
                case MEDIA_MODE_VIDEOS:
                        gtk_image_set_from_icon_name(GTK_IMAGE(self->image),
                                "folder-videos-symbolic", GTK_ICON_SIZE_INVALID);
                        if (results->len == 0)
                                info_string = g_strdup_printf("No videos");
                        else if (results->len == 1)
                                info_string = g_strdup_printf("%d video",
                                        results->len);
                        else
                                info_string = g_strdup_printf("%d videos",
                                        results->len);
                        break;
                default:
                        if (results->len == 0)
                                info_string = g_strdup_printf("No songs");
                        else if (results->len == 1)
                                info_string = g_strdup_printf("%d song",
                                        results->len);
                        else
                                info_string = g_strdup_printf("%d songs",
                                        results->len);
                        break;
        }

        gtk_label_set_text(GTK_LABEL(self->count_label), info_string);
        if (self->mode != MEDIA_MODE_ALBUMS)
                gtk_label_set_text(GTK_LABEL(self->current_label), "");
        g_free(info_string);
        self->results = results;
}

static void list_selection_cb(GtkListBox *list, GtkListBoxRow *row,
                              gpointer userdata)
{
        BudgieMediaView *self;
        BudgieMediaLabel *label;
        MediaInfo *info;
        GList *children;

        self = BUDGIE_MEDIA_VIEW(userdata);
        if (!row)
                return;

        children = gtk_container_get_children(GTK_CONTAINER(row));
        label = (BudgieMediaLabel*)g_list_nth_data(children, 0);
        info = label->info;

        g_signal_emit_by_name(self, "media-selected", (gpointer)info);
        g_list_free(children);

}
static gint sort_list(GtkListBoxRow *row1,
                      GtkListBoxRow *row2,
                      gpointer userdata)
{
        /* Must revisit this function, but we need to switch to a new
         * class first, so that we can track MediaInfo, and do album
         * comparisons */
        GList *children;
        BudgieMediaLabel *label1, *label2;
        gchar *text1 = NULL;
        gchar *text2 = NULL;
        int ret;

        children = gtk_container_get_children(GTK_CONTAINER(row1));
        label1 = (BudgieMediaLabel*)g_list_nth_data(children, 0);
        text1 = label1->info->title;
        g_list_free(children);

        children = gtk_container_get_children(GTK_CONTAINER(row2));
        label2 = (BudgieMediaLabel*)g_list_nth_data(children, 0);
        text2 = label2->info->title;
        g_list_free(children);

        ret = g_ascii_strcasecmp(text1, text2);

        return ret;
}

static GdkPixbuf *beautify(GdkPixbuf **source,
                           GdkPixbuf *base,
                           GdkPixbuf *overlay)
{
        int width, height;
        int new_width, new_height;
        cairo_surface_t *surface;
        cairo_t *ctx;
        int x_pad = 20;
        int y_pad = 6;
        GdkPixbuf *scaled, *ret;

        /* Create a new surface to work from */
        width = gdk_pixbuf_get_width(base);
        height = gdk_pixbuf_get_height(base);
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                width, height);
        ctx = cairo_create(surface);

        /* Draw the base first */
        gdk_cairo_set_source_pixbuf(ctx, base, 0, 0);
        cairo_paint(ctx);

        /* Draw the source image (album cover) */
        if (source) {
                new_width = (width-x_pad)-7;
                new_height = (height-y_pad)-10;
                scaled = gdk_pixbuf_scale_simple(*source,
                        new_width, new_height, GDK_INTERP_BILINEAR);
                g_object_unref(*source);
                *source = NULL;
                gdk_cairo_set_source_pixbuf(ctx, scaled, x_pad, y_pad);
                cairo_paint(ctx);
                g_object_unref(scaled);
        }

        /* Draw the overlay */
        gdk_cairo_set_source_pixbuf(ctx, overlay, 0, 0);
        cairo_paint(ctx);

        /* Create a new image to return from this painting op */
        ret = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);

        /* Cleanup */
        cairo_surface_destroy(surface);
        cairo_destroy(ctx);

        return ret;
}


MediaInfo* budgie_media_view_get_info(BudgieMediaView *self,
                                      BudgieMediaSelection select)
{
        MediaInfo *ret = NULL;
        int index = self->index;

        /* No results yet */
        if (!self->results)
                return NULL;

        switch (select) {
                case MEDIA_SELECTION_NEXT:
                        index++;
                        break;
                case MEDIA_SELECTION_PREVIOUS:
                        index--;
                        break;
                default:
                        /* Random not yet implemented */
                        break;
        }
        /* Out of bounds */
        if (index < 0 || index >= self->results->len)
                return NULL;

        ret = self->results->pdata[index];
        return ret;
}

void budgie_media_view_set_active(BudgieMediaView *self,
                                  MediaInfo *active)
{
        BudgieMediaLabel *label;
        GList *children, *child;
        GtkListBoxRow *row;

        /* Visit all children in our list box */
        children = gtk_container_get_children(GTK_CONTAINER(self->list));
        for (child = g_list_first(children); child; child = child->next) {
                row = GTK_LIST_BOX_ROW(child->data);
                label = (BudgieMediaLabel*)gtk_bin_get_child(GTK_BIN(row));
                if (label->info != active)
                        g_object_set(label, "playing", FALSE, NULL);
                else {
                        self->index = gtk_list_box_row_get_index(row);
                        g_object_set(label, "playing", TRUE, NULL);
                }
        }

        g_list_free(children);
}
