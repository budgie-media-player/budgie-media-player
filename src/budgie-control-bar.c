/*
 * budgie-control-bar.c
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

#include "budgie-control-bar.h"
#include "util.h"

G_DEFINE_TYPE(BudgieControlBar, budgie_control_bar, GTK_TYPE_TOOLBAR);

/* Initialisation */
static void budgie_control_bar_class_init(BudgieControlBarClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_control_bar_dispose;
}

static void budgie_control_bar_init(BudgieControlBar *self)
{
        GtkStyleContext *style;
        GtkWidget *control_box;
        GtkWidget *control_video_box;
        GtkToolItem *control_item;
        GtkToolItem *control_video_item;
        GtkWidget *repeat, *random;
        GtkWidget *reload;
        GtkToolItem *reload_item;
        GtkWidget *full_screen;
        GtkWidget *aspect;
        GtkWidget *about;
        GtkToolItem *about_item;
        GtkToolItem *separator1, *separator2;

        /* Initialise IconTheme */
        self->icon_theme = gtk_icon_theme_get_default();

        /* Use dark toolbar style (primary) */
        style = gtk_widget_get_style_context(GTK_WIDGET(self));
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

        /* Our media controls */
        control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        style = gtk_widget_get_style_context(control_box);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_LINKED);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_RAISED);
        control_item = gtk_tool_item_new();
        gtk_container_add(GTK_CONTAINER(control_item), control_box);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(control_item));

        /* repeat */
        repeat = new_button_with_icon(self->icon_theme, "media-playlist-repeat", TRUE, TRUE);
        gtk_box_pack_start(GTK_BOX(control_box), repeat, FALSE, FALSE, 0);

        /* random */
        random = new_button_with_icon(self->icon_theme, "media-playlist-shuffle", TRUE, TRUE);
        gtk_box_pack_start(GTK_BOX(control_box), random, FALSE, FALSE, 0);

        /* Visual separation between generic media and video controls */
        separator1 = gtk_separator_tool_item_new();
        gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(separator1), FALSE);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(separator1));

        /* Our video controls */
        control_video_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        self->video_controls = control_video_box;
        style = gtk_widget_get_style_context(control_video_box);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_LINKED);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_RAISED);
        control_video_item = gtk_tool_item_new();
        gtk_container_add(GTK_CONTAINER(control_video_item), control_video_box);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(control_video_item));

        /* full screen */
        full_screen = new_button_with_icon(self->icon_theme, "view-fullscreen-symbolic", TRUE, TRUE);
        gtk_container_add(GTK_CONTAINER(control_video_box), full_screen);
        self->full_screen = full_screen;

        /* Force aspect ratio - enabled by default */
        aspect = new_button_with_icon(self->icon_theme, "window-maximize-symbolic", TRUE, TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aspect), TRUE);
        gtk_container_add(GTK_CONTAINER(control_video_box), aspect);

        /* separator (shift following items to right hand side */
        separator2 = gtk_separator_tool_item_new();
        gtk_tool_item_set_expand(separator2, TRUE);
        gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(separator2),
                FALSE);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(separator2));

        /* reload */
        reload = new_button_with_icon(self->icon_theme, "view-refresh", TRUE, FALSE);
        reload_item = gtk_tool_item_new();
        self->reload = reload;
        gtk_container_add(GTK_CONTAINER(reload_item), reload);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(reload_item));

        /* about */
        about = new_button_with_icon(self->icon_theme, "help-about-symbolic", TRUE, FALSE);
        about_item = gtk_tool_item_new();
        gtk_container_add(GTK_CONTAINER(about_item), about);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(about_item));
}

static void budgie_control_bar_dispose(GObject *object)
{
        BudgieControlBar *self;

        self = BUDGIE_CONTROL_BAR(object);

        /* Destruct */
        G_OBJECT_CLASS (budgie_control_bar_parent_class)->dispose (object);
}

/* Utility; return a new BudgieControlBar */
GtkWidget* budgie_control_bar_new(void)
{
        BudgieControlBar *self;

        self = g_object_new(BUDGIE_CONTROL_BAR_TYPE, NULL);
        return GTK_WIDGET(self);
}
