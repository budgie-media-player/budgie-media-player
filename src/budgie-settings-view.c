/*
 * budgie-settings-view.c
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
#include "budgie-settings-view.h"
#include "util.h"
#include "common.h"

G_DEFINE_TYPE(BudgieSettingsView, budgie_settings_view, GTK_TYPE_BOX)

enum SettingsColumns {
        SETTINGS_COLUMN_PATH,
        SETTINGS_N_COLUMNS
};

static void paths_cursor_cb(GtkWidget *widget, gpointer userdata);
static void paths_add_cb(GtkWidget *widget, gpointer userdata);
static void paths_remove_cb(GtkWidget *widget, gpointer userdata);
/* Refresh view from settings */
static void budgie_settings_refresh(BudgieSettingsView *self);
static GtkWidget *create_about(BudgieSettingsView *self);
static void settings_changed(GSettings *settings,
                             gchar *key,
                             gpointer userdata);

/* Boilerplate GObject code */
static void budgie_settings_view_class_init(BudgieSettingsViewClass *klass);
static void budgie_settings_view_init(BudgieSettingsView *self);
static void budgie_settings_view_dispose(GObject *object);

/* Initialisation */
static void budgie_settings_view_class_init(BudgieSettingsViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_settings_view_dispose;
}

static void budgie_settings_view_init(BudgieSettingsView *self)
{
        GtkWidget *paths;
        GtkWidget *tree, *scroll, *box;
        GtkTreeViewColumn *column;
        GtkCellRenderer *cell;
        GtkWidget *toolbar;
        GtkStyleContext *style;
        GtkToolItem *tool;
        /* Global stack */
        GtkWidget *stack, *chooser;
        GtkWidget *about;
        GValue value = G_VALUE_INIT;
        /* Layout */
        GtkWidget *layout;

        /* Settings
         * TODO: Connect gsettings and refresh when the value changes.
         * i.e. someone being rightfully awkward and doing this in
         * dconf-editor */
        self->settings = g_settings_new(BUDGIE_SCHEMA);
        g_signal_connect(self->settings, "changed",
                G_CALLBACK(settings_changed), self);
        /* Fire settings */
        settings_changed(self->settings, BUDGIE_DARK, self);

        layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(self), layout);

        /* Add our stack chooser, etc. */
        stack = gtk_stack_new();
        chooser = gtk_stack_switcher_new();
        gtk_stack_set_transition_type(GTK_STACK(stack),
                GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
        gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(chooser),
            GTK_STACK(stack));
        gtk_box_pack_start(GTK_BOX(layout), chooser, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(layout), stack, TRUE, TRUE, 0);

        /* Center the switcher */
        gtk_widget_set_halign(chooser, GTK_ALIGN_CENTER);

        /* Bit of sane padding around all components */
        gtk_container_set_border_width(GTK_CONTAINER(self), 30);
        gtk_container_set_border_width(GTK_CONTAINER(stack), 20);

        paths = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_stack_add_named(GTK_STACK(stack), paths, "paths");
        /* Set the title. I love how this is a simple task.. */
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_static_string(&value, "Media Directories");
        gtk_container_child_set_property(GTK_CONTAINER(stack), paths,
            "title", &value);
        g_value_unset(&value);

        /* Add the about page */
        about = create_about(self);
        gtk_stack_add_named(GTK_STACK(stack), about, "about");
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_static_string(&value, "About Budgie");
        gtk_container_child_set_property(GTK_CONTAINER(stack), about,
            "title", &value);
        g_value_unset(&value);

        /* Paths treeview */
        tree = gtk_tree_view_new();
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
        g_signal_connect(tree, "cursor-changed",
                G_CALLBACK(paths_cursor_cb), self);
        self->paths = tree;
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        style = gtk_widget_get_style_context(scroll);
        gtk_style_context_set_junction_sides(style, GTK_JUNCTION_BOTTOM);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
                GTK_SHADOW_ETCHED_IN);

        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(paths), box);
        gtk_container_add(GTK_CONTAINER(scroll), tree);
        gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

        cell = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Path",
                cell, "text", SETTINGS_COLUMN_PATH, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

        /* Create the toolbar */
        toolbar = gtk_toolbar_new();
        gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
        style = gtk_widget_get_style_context(toolbar);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_INLINE_TOOLBAR);
        gtk_style_context_set_junction_sides(style, GTK_JUNCTION_TOP);
        gtk_box_pack_start(GTK_BOX(box), toolbar, FALSE, FALSE, 0);

        /* Manipulation buttons (+/-) */
        tool = gtk_tool_button_new(NULL, "Add");
        self->path_add = GTK_WIDGET(tool);
        g_signal_connect(tool, "clicked", G_CALLBACK(paths_add_cb),
                self);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool),
                "list-add-symbolic");
        gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool));

        tool = gtk_tool_button_new(NULL, "Remove");
        self->path_remove = GTK_WIDGET(tool);
        g_signal_connect(tool, "clicked", G_CALLBACK(paths_remove_cb),
                self);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool),
                "list-remove-symbolic");
        gtk_widget_set_sensitive(GTK_WIDGET(tool), FALSE);
        gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool));

        budgie_settings_refresh(self);
}

static void budgie_settings_view_dispose(GObject *object)
{
        /* Destruct */
        G_OBJECT_CLASS (budgie_settings_view_parent_class)->dispose (object);
}

/* Utility; return a new BudgieSettingsView */
GtkWidget* budgie_settings_view_new(void)
{
        BudgieSettingsView *self;

        self = g_object_new(BUDGIE_SETTINGS_VIEW_TYPE, "orientation",
                GTK_ORIENTATION_VERTICAL, NULL);
        return GTK_WIDGET(self);
}

static void budgie_settings_refresh(BudgieSettingsView *self)
{
        gchar **media_dirs = NULL;
        gchar *path = NULL;
        gint length = 0, i;
        /* paths view */
        GtkListStore *store;
        GtkTreeIter iter;

        media_dirs = g_settings_get_strv(self->settings, BUDGIE_MEDIA_DIRS);
        length = g_strv_length(media_dirs);

        store = gtk_list_store_new(SETTINGS_N_COLUMNS, G_TYPE_STRING);
        /* Add all media directories to paths view */
        for (i=0; i < length; i++) {
                path = media_dirs[i];
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, SETTINGS_COLUMN_PATH,
                        path, -1);
        }
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->paths),
                GTK_TREE_MODEL(store));

        if (media_dirs) {
                g_strfreev(media_dirs);
        }
}

static void paths_cursor_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieSettingsView *self;
        GtkTreeSelection *selection = NULL;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;

        self = BUDGIE_SETTINGS_VIEW(userdata);

        /* If nothing is selected, disable the remove button */
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->paths));
        if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
                gtk_widget_set_sensitive(self->path_remove, FALSE);
                return;
        }
        gtk_widget_set_sensitive(self->path_remove, TRUE);
}

static void paths_add_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieSettingsView *self;
        GtkWidget *dialog;
        GtkWidget *top;
        gchar *filename;
        /* settings */
        gchar **paths = NULL;
        GPtrArray *new_paths = NULL;
        guint length, i;

        self = BUDGIE_SETTINGS_VIEW(userdata);
        top = gtk_widget_get_toplevel(GTK_WIDGET(self));
        dialog = gtk_file_chooser_dialog_new("Choose a directory",
                GTK_WINDOW(top), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                "Cancel", GTK_RESPONSE_REJECT,
                "Select", GTK_RESPONSE_ACCEPT, NULL);

        /* Present the dialog */
        if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
                goto end;
        }

        /* Get paths */
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        paths = g_settings_get_strv(self->settings, BUDGIE_MEDIA_DIRS);
        length = g_strv_length(paths);

        /* Make a new array to copy the original */
        new_paths = g_ptr_array_new();
        for (i=0; i < length; i++) {
                if (!g_str_equal(filename, paths[i])) {
                        g_ptr_array_add(new_paths, paths[i]);
                }
        }
        /* Copy path in */
        g_ptr_array_add(new_paths, filename);
        g_ptr_array_add(new_paths, NULL);

        /* Update GSettings with new directories */
        g_settings_set_strv(self->settings, BUDGIE_MEDIA_DIRS,
                (const gchar* const*)new_paths->pdata);
        budgie_settings_refresh(self);
        g_ptr_array_free(new_paths, TRUE);

        g_free(filename);
end:
        gtk_widget_destroy(dialog);
}

static void paths_remove_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieSettingsView *self;
        GtkTreeSelection *selection = NULL;
        GtkTreeModel *model = NULL;
        GtkTreeIter iter;
        GValue value = G_VALUE_INIT;
        const gchar *path;
        /* settings */
        gchar **paths = NULL;
        GPtrArray *new_paths = NULL;
        guint length, i;

        self = BUDGIE_SETTINGS_VIEW(userdata);

        /* Shouldn't be a null selection. */
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->paths));
        if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
                return;
        }

        /* Retrieve old values */
        paths = g_settings_get_strv(self->settings, BUDGIE_MEDIA_DIRS);
        gtk_tree_model_get_value(model, &iter, SETTINGS_COLUMN_PATH, &value);
        path = g_value_get_string(&value);
        length = g_strv_length(paths);

        /* Make a new array, with one less element */
        new_paths = g_ptr_array_new();
        /* Copy all elements except the one we're removing */
        for (i=0; i < length; i++) {
                if (!g_str_equal(path, paths[i])) {
                        g_ptr_array_add(new_paths, paths[i]);
                }
        }
        g_ptr_array_add(new_paths, NULL);

        /* Update GSettings with new directories */
        g_settings_set_strv(self->settings, BUDGIE_MEDIA_DIRS,
                (const gchar* const*)new_paths->pdata);
        budgie_settings_refresh(self);
        g_ptr_array_free(new_paths, TRUE);
        g_value_unset(&value);
}

static GtkWidget *create_about(BudgieSettingsView *self)
{
        GtkWidget *horiz, *verti;
        GtkWidget *label, *desc, *image;
        GtkWidget *link;
        gchar *title;

        horiz = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_valign(horiz, GTK_ALIGN_CENTER);

        /* Left image */
        image = gtk_image_new_from_icon_name("budgie", GTK_ICON_SIZE_INVALID);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 256);
        gtk_box_pack_start(GTK_BOX(horiz), image, FALSE, FALSE, 0);

        /* Stores labels */
        verti = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_valign(verti, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(horiz), verti, FALSE, FALSE, 0);

        /* Main label */
        title = g_strdup_printf("<span size='x-large'>%s</span> <small>v%s</small>",
                PACKAGE_NAME, PACKAGE_VERSION);
        label = gtk_label_new(title);
        g_free(title);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_box_pack_start(GTK_BOX(verti), label, FALSE, FALSE, 0);

        /* Description */
        desc = gtk_label_new("Modern, lightweight and distraction free media experience");
        gtk_box_pack_start(GTK_BOX(verti), desc, FALSE, FALSE, 0);

        /* Link */
        link = gtk_link_button_new_with_label(PACKAGE_URL, "Visit Budgie's home");
        gtk_box_pack_start(GTK_BOX(verti), link, FALSE, FALSE, 0);

        return horiz;
}

static void settings_changed(GSettings *settings,
                             gchar *key,
                             gpointer userdata)
{
        BudgieSettingsView *self;
        GtkSettings *ui_settings;
        gboolean b_ret;

        self = BUDGIE_SETTINGS_VIEW(userdata);

        if (g_str_equal(key, BUDGIE_DARK)) {
                /* Set a dark theme :) */
                ui_settings = gtk_widget_get_settings(GTK_WIDGET(self));
                b_ret = g_settings_get_boolean(settings, key);
                g_object_set(ui_settings, "gtk-application-prefer-dark-theme",
                        b_ret, NULL);
        }
}
