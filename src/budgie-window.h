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
#include "player-view.h"
#include "budgie-control-bar.h"
#include "budgie-settings-view.h"
#include "util.h"
#include "db/budgie-db.h"

#define PLAYER_CSS "BudgieStatusArea {\
    border-radius: 4px;\
    border-width: 1px;\
    background-image: linear-gradient(to bottom,\
                                      shade(shade(@theme_bg_color, 0.89), 1.05),\
                                      shade(shade(@theme_bg_color, 0.96), 0.97)\
                                      );\
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

/* Private storage */
struct _BudgieWindowPrivate {
        const gchar *current_page;
        GSettings *settings;
        GSList *tracks;
        gchar *uri;
        gulong volume_id;
        gint64 duration;
        gboolean repeat;
        gboolean random;
        gboolean force_aspect;
        gboolean full_screen;
        guintptr window_handle;
};

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
        GtkWidget *player;
#ifdef TESTING
        GtkWidget *view;
#endif
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


/* Boilerplate GObject code */
static void budgie_window_class_init(BudgieWindowClass *klass);
static void budgie_window_init(BudgieWindow *self);
static void budgie_window_dispose(GObject *object);
GType budgie_window_get_type(void);

/* BudgieWindow methods */
BudgieWindow* budgie_window_new(void);

/* BudgieWindow prototypes */
static void init_styles(BudgieWindow *self);

static void store_media(gpointer data1, gpointer data2);
static gboolean load_media_t(gpointer data);
static gpointer load_media(gpointer data);

/* Callbacks */
static void about_cb(GtkWidget *widget, gpointer userdata);
static void play_cb(GtkWidget *widget, gpointer userdata);
static void pause_cb(GtkWidget *widget, gpointer userdata);
static void next_cb(GtkWidget *widget, gpointer userdata);
static void prev_cb(GtkWidget *widget, gpointer userdata);
static void volume_cb(GtkWidget *widget, gpointer userdata);
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer userdata);
static void realize_cb(GtkWidget *widget, gpointer userdata);
static gboolean refresh_cb(gpointer userdata);
static void reload_cb(GtkWidget *widget, gpointer userdata);
static void full_screen_cb(GtkWidget *widget, gpointer userdata);
static void aspect_cb(GtkWidget *widget, gpointer userdata);
static gboolean motion_notify_cb(GtkWidget *widget, GdkEventMotion *event, gpointer userdata);
static gboolean key_cb(GtkWidget *widget, GdkEventKey *event, gpointer userdata);
static void settings_changed(GSettings *settings, gchar *key, gpointer userdata);
static void toolbar_cb(BudgieControlBar *bar, int action, gboolean toggle, gpointer userdata);
static void seek_cb(BudgieStatusArea *status, gint64 value, gpointer userdata);

/* GStreamer callbacks */
static void _gst_eos_cb(GstBus *bus, GstMessage *msg, gpointer userdata);

#endif /* budgie_window_h */
