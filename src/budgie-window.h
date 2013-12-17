/*
 * budgie-window.h
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
#ifndef budgie_window_h
#define budgie_window_h

#include "config.h"

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include "budgie-status-area.h"
#include "budgie-control-bar.h"
#include "budgie-settings-view.h"
#include "util.h"
#include "db/budgie-db.h"

#define PLAYER_CSS "BudgieStatusArea {\
    border-radius: 4px;\
    background-image: linear-gradient(to bottom,\
                shade(@theme_bg_color, 0.9),\
                shade(@theme_bg_color, 1.1));\
    border: 1px shade(@theme_bg_color, 0.85) solid;\
}\
GtkIconView {\
    background-color: @theme_bg_color;\
}\
.info-label {\
    color: #707070;\
}\
.trough {\
    background-color: alpha(white, 0.0);\
}"

typedef struct _BudgieWindow BudgieWindow;
typedef struct _BudgieWindowClass   BudgieWindowClass;
typedef struct _BudgieWindowPrivate BudgieWindowPrivate;

#define BUDGIE_WINDOW_TYPE (budgie_window_get_type())
#define BUDGIE_WINDOW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_WINDOW_TYPE, BudgieWindow))
#define IS_BUDGIE_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_WINDOW_TYPE))
#define BUDGIE_WINDOW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_WINDOW_TYPE, BudgieWindowClass))
#define IS_BUDGIE_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_WINDOW_TYPE))
#define BUDGIE_WINDOW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_WINDOW_TYPE, BudgieWindowClass))

/* BudgieWindow object */
struct _BudgieWindow {
        GObject parent;

        gchar **media_dirs;

        GtkWidget *window;
        GtkWidget *header;
        GtkIconTheme *icon_theme;

        GtkWidget *video;
        gboolean video_realized;
        GtkWidget *stack;
        GtkWidget *toolbar;

        BudgieDB *db;

        GtkWidget *status;
        GtkWidget *view;
        GtkWidget *south_reveal;

        /* Header controls */
        GtkWidget *prev;
        GtkWidget *play;
        GtkWidget *pause;
        GtkWidget *next;
        GtkWidget *volume;
        GtkWidget *search;
        GtkWidget *settings;
        GstElement *gst_player;

        GtkCssProvider *css_provider;
        BudgieWindowPrivate *priv;
};

/* BudgieWindow class definition */
struct _BudgieWindowClass {
        GObjectClass parent_class;
};


GType budgie_window_get_type(void);

/* BudgieWindow methods */
BudgieWindow* budgie_window_new(void);

#endif /* budgie_window_h */
