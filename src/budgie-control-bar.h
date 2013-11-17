/*
 * budgie-control-bar.h
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
#ifndef budgie_control_bar_h
#define budgie_control_bar_h

#include <glib-object.h>
#include <gtk/gtk.h>

typedef struct _BudgieControlBar BudgieControlBar;
typedef struct _BudgieControlBarClass   BudgieControlBarClass;

#define BUDGIE_CONTROL_BAR_TYPE (budgie_control_bar_get_type())
#define BUDGIE_CONTROL_BAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_CONTROL_BAR_TYPE, BudgieControlBar))
#define IS_BUDGIE_CONTROL_BAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_CONTROL_BAR_TYPE))
#define BUDGIE_CONTROL_BAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_CONTROL_BAR_TYPE, BudgieControlBarClass))
#define IS_BUDGIE_CONTROL_BAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_CONTROL_BAR_TYPE))
#define BUDGIE_CONTROL_BAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_CONTROL_BAR_TYPE, BudgieControlBarClass))

/* Representative of supported actions */
typedef enum {
        BUDGIE_ACTION_RANDOM = 0,
        BUDGIE_ACTION_REPEAT,
        BUDGIE_ACTION_FULL_SCREEN,
        BUDGIE_ACTION_ASPECT_RATIO,
        BUDGIE_ACTION_RELOAD,
        BUDGIE_ACTION_ABOUT,
        BUDGIE_ACTION_SETTINGS,
        BUDGIE_ACTION_PLAY,
        BUDGIE_ACTION_PAUSE,
        BUDGIE_ACTION_PREVIOUS,
        BUDGIE_ACTION_NEXT,
#ifdef TESTING
        BUDGIE_ACTION_BROWSE_VIEW,
#endif
        BUDGIE_MAX_ACTIONS
} BudgieAction;

/* BudgieControlBar object */
struct _BudgieControlBar {
        GtkToolbar parent;

        GtkIconTheme *icon_theme;
        GtkWidget *reload;
        GtkWidget *video_controls;
        GtkWidget *full_screen;

        GtkWidget *repeat;
        GtkWidget *random;

        GtkWidget *settings;

        /* Playback controls */
        GtkWidget *playback;
        GtkWidget *play;
        GtkWidget *pause;
};

/* BudgieControlBar class definition */
struct _BudgieControlBarClass {
        GtkToolbarClass parent_class;
};


/* Boilerplate GObject code */
static void budgie_control_bar_class_init(BudgieControlBarClass *klass);
static void budgie_control_bar_init(BudgieControlBar *self);
static void budgie_control_bar_dispose(GObject *object);
GType budgie_control_bar_get_type(void);

/* BudgieControlBar methods */
GtkWidget* budgie_control_bar_new(void);

/**
 * Show or hide the video specific controls
 * @param show Whether to show or hide the controls
 */
void budgie_control_bar_set_show_video(BudgieControlBar *self, gboolean show);

/**
 * Show or hide the playback controls
 * @param show Whether to show or hide the playback controls
 */
void budgie_control_bar_set_show_playback(BudgieControlBar *self, gboolean show);

/**
 * Enable or disable a selected actions button
 * @param action The action to enable or disable
 * @param enabled Whether this action should be enabled or disabled
 */
void budgie_control_bar_set_action_enabled(BudgieControlBar *self,
                                           BudgieAction action,
                                           gboolean enabled);


/**
 * Set the selection action button to active or inactive
 * @param action The action to modify
 * @param enabled Whether this action should be active or inactive
 */
void budgie_control_bar_set_action_state(BudgieControlBar *self,
                                         BudgieAction action,
                                         gboolean state);

#endif /* budgie_control_bar_h */
