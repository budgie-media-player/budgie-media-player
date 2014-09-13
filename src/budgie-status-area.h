/*
 * budgie-status-area.h
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
#ifndef budgie_status_area_h
#define budgie_status_area_h

#include <glib-object.h>
#include <gtk/gtk.h>

#include "db/budgie-db.h"

typedef struct _BudgieStatusArea BudgieStatusArea;
typedef struct _BudgieStatusAreaClass   BudgieStatusAreaClass;
typedef struct _BudgieStatusAreaPrivate BudgieStatusAreaPrivate;

#define BUDGIE_STATUS_AREA_TYPE (budgie_status_area_get_type())
#define BUDGIE_STATUS_AREA(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_STATUS_AREA_TYPE, BudgieStatusArea))
#define IS_BUDGIE_STATUS_AREA(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_STATUS_AREA_TYPE))
#define BUDGIE_STATUS_AREA_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_STATUS_AREA_TYPE, BudgieStatusAreaClass))
#define IS_BUDGIE_STATUS_AREA_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_STATUS_AREA_TYPE))
#define BUDGIE_STATUS_AREA_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_STATUS_AREA_TYPE, BudgieStatusAreaClass))

/* BudgieStatusArea object */
struct _BudgieStatusArea {
        GtkEventBox parent;

        GtkWidget *label;
        GtkWidget *time_label;
        GtkWidget *slider;

        BudgieStatusAreaPrivate *priv;
};

/* BudgieStatusArea class definition */
struct _BudgieStatusAreaClass {
        GtkEventBoxClass parent_class;
};

GType budgie_status_area_get_type(void);

/* BudgieStatusArea methods */
GtkWidget* budgie_status_area_new(void);

void budgie_status_area_set_media(BudgieStatusArea *self, MediaInfo *info);


/**
 * Set the time information for the current item
 * @param max Total duration
 * @param current Time passed
 */
void budgie_status_area_set_media_time(BudgieStatusArea *self, gint64 max, gint64 current);

#endif /* budgie_status_area_h */
