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

static void handler_cb(GtkWidget *widget, gpointer userdata);

/* Initialisation */
static void budgie_control_bar_class_init(BudgieControlBarClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_control_bar_dispose;

        g_signal_new("action-selected",
                G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
                0, NULL, NULL, NULL, G_TYPE_NONE,
                2, G_TYPE_INT, G_TYPE_BOOLEAN);
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
        GtkWidget *settings;
        /* playback */
        GtkWidget *playback;
        GtkToolItem *playback_item;
        GtkWidget *play, *pause;
        GtkWidget *prev, *next;
        GtkToolItem *about_item;
        GtkToolItem *settings_item;
        GtkToolItem *separator1, *separator2, *separator3;
        guint *data = NULL;

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
        repeat = new_button_with_icon(self->icon_theme, "media-playlist-repeat",
                TRUE, TRUE, "Toggle repeat mode");
        self->repeat = repeat;
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_REPEAT;
        g_object_set_data_full(G_OBJECT(repeat), "budgie", data, g_free);
        g_signal_connect(repeat, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_box_pack_start(GTK_BOX(control_box), repeat, FALSE, FALSE, 0);

        /* random */
        random = new_button_with_icon(self->icon_theme, "media-playlist-shuffle",
                TRUE, TRUE, "Toggle random playback (shuffle)");
        self->random = random;
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_RANDOM;
        g_object_set_data_full(G_OBJECT(random), "budgie", data, g_free);
        g_signal_connect(random, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
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
        full_screen = new_button_with_icon(self->icon_theme, "view-fullscreen-symbolic",
                TRUE, TRUE, "Toggle full-screen mode");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_FULL_SCREEN;
        g_object_set_data_full(G_OBJECT(full_screen), "budgie", data, g_free);
        g_signal_connect(full_screen, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_container_add(GTK_CONTAINER(control_video_box), full_screen);
        self->full_screen = full_screen;

        /* Force aspect ratio - enabled by default */
        aspect = new_button_with_icon(self->icon_theme, "window-maximize-symbolic",
                TRUE, TRUE, "Toggle aspect-ratio mode");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_ASPECT_RATIO;
        g_object_set_data_full(G_OBJECT(aspect), "budgie", data, g_free);
        g_signal_connect(aspect, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aspect), TRUE);
        gtk_container_add(GTK_CONTAINER(control_video_box), aspect);

        /* Visual separation between video and playback controls */
        separator2 = gtk_separator_tool_item_new();
        gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(separator2), FALSE);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(separator2));

        /* Playback controls */
        playback = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        self->playback = playback;
        style = gtk_widget_get_style_context(playback);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_LINKED);
        gtk_style_context_add_class(style, GTK_STYLE_CLASS_RAISED);
        playback_item = gtk_tool_item_new();
        gtk_container_add(GTK_CONTAINER(playback_item), playback);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(playback_item));

        /* previous */
        prev = new_button_with_icon(self->icon_theme, "media-seek-backward", TRUE, FALSE,
                "Previous track");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_PREVIOUS;
        g_object_set_data_full(G_OBJECT(prev), "budgie", data, g_free);
        g_signal_connect(prev, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_box_pack_start(GTK_BOX(playback), prev, FALSE, FALSE, 0);

        /* play */
        play = new_button_with_icon(self->icon_theme, "media-playback-start",
                TRUE, FALSE, "Play");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_PLAY;
        self->play = play;
        g_object_set_data_full(G_OBJECT(play), "budgie", data, g_free);
        g_signal_connect(play, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_box_pack_start(GTK_BOX(playback), play, FALSE, FALSE, 0);

        /* pause */
        pause = new_button_with_icon(self->icon_theme, "media-playback-pause",
                TRUE, FALSE, "Pause");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_PAUSE;
        self->pause = pause;
        g_object_set_data_full(G_OBJECT(pause), "budgie", data, g_free);
        g_signal_connect(pause, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_box_pack_start(GTK_BOX(playback), pause, FALSE, FALSE, 0);

        /* next */
        next = new_button_with_icon(self->icon_theme, "media-seek-forward",
                TRUE, FALSE, "Next track");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_NEXT;
        g_object_set_data_full(G_OBJECT(next), "budgie", data, g_free);
        g_signal_connect(next, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        gtk_box_pack_start(GTK_BOX(playback), next, FALSE, FALSE, 0);

        /* separator (shift following items to right hand side */
        separator3 = gtk_separator_tool_item_new();
        gtk_tool_item_set_expand(separator3, TRUE);
        gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(separator3),
                FALSE);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(separator3));

        /* reload */
        reload = new_button_with_icon(self->icon_theme, "view-refresh",
                TRUE, FALSE, "Reload media library");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_RELOAD;
        g_object_set_data_full(G_OBJECT(reload), "budgie", data, g_free);
        g_signal_connect(reload, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        reload_item = gtk_tool_item_new();
        self->reload = reload;
        gtk_container_add(GTK_CONTAINER(reload_item), reload);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(reload_item));

        /* settings */
        settings = new_button_with_icon(self->icon_theme, "preferences-system-symbolic",
                TRUE, TRUE, "Preferences");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_SETTINGS;
        self->settings = settings;
        g_object_set_data_full(G_OBJECT(settings), "budgie", data, g_free);
        g_signal_connect(settings, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
        settings_item = gtk_tool_item_new();
        gtk_container_add(GTK_CONTAINER(settings_item), settings);
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(settings_item));

        /* about */
        about = new_button_with_icon(self->icon_theme, "help-about-symbolic",
                TRUE, FALSE, "About Budgie..");
        data = g_malloc(sizeof(guint));
        *data = BUDGIE_ACTION_ABOUT;
        g_object_set_data_full(G_OBJECT(about), "budgie", data, g_free);
        g_signal_connect(about, "clicked", G_CALLBACK(handler_cb), (gpointer)self);
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

static void handler_cb(GtkWidget *widget, gpointer userdata)
{
        BudgieControlBar *self;
        guint data;
        gboolean toggle = FALSE;

        self = BUDGIE_CONTROL_BAR(userdata);
        if (GTK_IS_TOGGLE_BUTTON(widget))
                toggle = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

        data = *(guint*)g_object_get_data(G_OBJECT(widget), "budgie");

        g_signal_emit_by_name(self, "action-selected", data, toggle);
}

void budgie_control_bar_set_show_video(BudgieControlBar *self, gboolean show)
{
        gtk_widget_set_visible(self->video_controls, show);
}

void budgie_control_bar_set_show_playback(BudgieControlBar *self, gboolean show)
{
        gtk_widget_set_visible(self->playback, show);
}

void budgie_control_bar_set_action_enabled(BudgieControlBar *self,
                                           BudgieAction action,
                                           gboolean enabled)
{
        GtkWidget *wid = NULL;
        gboolean sensitive = FALSE;

        switch (action) {
                case BUDGIE_ACTION_RELOAD:
                        wid = GTK_WIDGET(self->reload);
                        sensitive = TRUE;
                        break;
                case BUDGIE_ACTION_PLAY:
                        wid = GTK_WIDGET(self->play);
                        break;
                case BUDGIE_ACTION_PAUSE:
                        wid = GTK_WIDGET(self->pause);
                        break;
                case BUDGIE_ACTION_SETTINGS:
                        wid = GTK_WIDGET(self->settings);
                        break;
                default:
                        break;
        }
        if (wid == NULL)
                return;
        if (sensitive)
                gtk_widget_set_sensitive(wid, enabled);
        else
                gtk_widget_set_visible(wid, enabled);
}

void budgie_control_bar_set_action_state(BudgieControlBar *self,
                                         BudgieAction action,
                                         gboolean state)
{
        GtkToggleButton *wid = NULL;

        switch (action) {
                case BUDGIE_ACTION_FULL_SCREEN:
                        wid = GTK_TOGGLE_BUTTON(self->full_screen);
                        break;
                case BUDGIE_ACTION_REPEAT:
                        wid = GTK_TOGGLE_BUTTON(self->repeat);
                        break;
                case BUDGIE_ACTION_RANDOM:
                        wid = GTK_TOGGLE_BUTTON(self->random);
                        break;
                default:
                        break;
        }
        if (wid == NULL)
                return;
        gtk_toggle_button_set_active(wid, state);
}
