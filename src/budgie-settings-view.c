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

#include "budgie-settings-view.h"
#include "util.h"

G_DEFINE_TYPE(BudgieSettingsView, budgie_settings_view, GTK_TYPE_BOX);

enum SettingsColumns {
        SETTINGS_COLUMN_PATH,
        SETTINGS_N_COLUMNS
};

/* Initialisation */
static void budgie_settings_view_class_init(BudgieSettingsViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_settings_view_dispose;
}

static void budgie_settings_view_init(BudgieSettingsView *self)
{
        GtkWidget *paths, *label;
        GtkWidget *tree, *scroll, *box;
        GtkTreeViewColumn *column;
        GtkCellRenderer *cell;
        GtkWidget *toolbar;
        GtkStyleContext *style;
        GtkToolItem *tool;

        /* Bit of sane padding around all components */
        gtk_container_set_border_width(GTK_CONTAINER(self), 30);

        /* Construct our paths frame */
        paths = gtk_frame_new("");
        label = gtk_label_new("<big>Media directories</big>");
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_frame_set_label_widget(GTK_FRAME(paths), label);

        gtk_container_add(GTK_CONTAINER(self), paths);
        gtk_container_set_border_width(GTK_CONTAINER(paths), 5);
        gtk_widget_set_halign(paths, GTK_ALIGN_START);
        gtk_widget_set_valign(paths, GTK_ALIGN_START);
        gtk_frame_set_shadow_type(GTK_FRAME(paths), GTK_SHADOW_NONE);

        /* Paths treeview */
        tree = gtk_tree_view_new();
        scroll = gtk_scrolled_window_new(NULL, NULL);
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
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool),
                "list-add-symbolic");
        gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool));

        tool = gtk_tool_button_new(NULL, "Remove");
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool),
                "list-remove-symbolic");
        gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(tool));
}

static void budgie_settings_view_dispose(GObject *object)
{
        BudgieSettingsView *self;

        self = BUDGIE_SETTINGS_VIEW(object);

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
