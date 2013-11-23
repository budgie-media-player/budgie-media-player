/*
 * budgie-media-view.h
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
#ifndef budgie_media_view_h
#define budgie_media_view_h

#include <glib-object.h>
#include <gtk/gtk.h>

#include "db/budgie-db.h"

typedef struct _BudgieMediaView BudgieMediaView;
typedef struct _BudgieMediaViewClass   BudgieMediaViewClass;

#define BUDGIE_MEDIA_VIEW_TYPE (budgie_media_view_get_type())
#define BUDGIE_MEDIA_VIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_MEDIA_VIEW_TYPE, BudgieMediaView))
#define IS_BUDGIE_MEDIA_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_MEDIA_VIEW_TYPE))
#define BUDGIE_MEDIA_VIEW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_MEDIA_VIEW_TYPE, BudgieMediaViewClass))
#define IS_BUDGIE_MEDIA_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_MEDIA_VIEW_TYPE))
#define BUDGIE_MEDIA_VIEW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_MEDIA_VIEW_TYPE, BudgieMediaViewClass))

/* BudgieMediaView object */
struct _BudgieMediaView {
        GtkBin parent;
        BudgieDB *db;

        GtkWidget *stack;

        GtkWidget *icon_view;

        /* Selection mode */
        GtkWidget *albums;
        GtkWidget *songs;
        GtkWidget *videos;

        /* Tracks page */
        GtkWidget *image;
        GtkWidget *list;
};

/* BudgieMediaView class definition */
struct _BudgieMediaViewClass {
        GtkBinClass parent_class;
};

GType budgie_media_view_get_type(void);

/* BudgieMediaView methods */

/**
 * Construct a new BudgieMediaView
 * @param database Database to use
 * @return A new BudgieMediaView
 */
GtkWidget* budgie_media_view_new(BudgieDB *database);

#endif /* budgie_media_view_h */
