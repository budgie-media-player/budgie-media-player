/*
 * budgie-media-label.h
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
#ifndef budgie_media_label_h
#define budgie_media_label_h

#include <glib-object.h>
#include <gtk/gtk.h>

#include "db/budgie-db.h"

typedef struct _BudgieMediaLabel BudgieMediaLabel;
typedef struct _BudgieMediaLabelClass   BudgieMediaLabelClass;

#define BUDGIE_MEDIA_LABEL_TYPE (budgie_media_label_get_type())
#define BUDGIE_MEDIA_LABEL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUDGIE_MEDIA_LABEL_TYPE, BudgieMediaLabel))
#define IS_BUDGIE_MEDIA_LABEL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUDGIE_MEDIA_LABEL_TYPE))
#define BUDGIE_MEDIA_LABEL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUDGIE_MEDIA_LABEL_TYPE, BudgieMediaLabelClass))
#define IS_BUDGIE_MEDIA_LABEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUDGIE_MEDIA_LABEL_TYPE))
#define BUDGIE_MEDIA_LABEL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUDGIE_MEDIA_LABEL_TYPE, BudgieMediaLabelClass))

/* BudgieMediaLabel object */
struct _BudgieMediaLabel {
        GtkBox parent;
        MediaInfo *info;
        GtkWidget *display;
        gboolean playing;
};

/* BudgieMediaLabel class definition */
struct _BudgieMediaLabelClass {
        GtkBoxClass parent_class;
};

GType budgie_media_label_get_type(void);

/* BudgieMediaLabel methods */

/**
 * Construct a new BudgieMediaLabel
 * @param info MediaInfo to use
 * @return A new BudgieMediaLabel
 */
GtkWidget* budgie_media_label_new(MediaInfo *info);

/**
 * Marks this item as playing or not playing
 * @param self Current BudgieMediaLabel instance
 * @param playing Whether this item is considered to be playing
 */
void budgie_media_label_set_playing(BudgieMediaLabel *self,
                                    gboolean playing);

#endif /* budgie_media_label_h */
