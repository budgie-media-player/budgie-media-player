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

/* BudgieControlBar object */
struct _BudgieControlBar {
        GtkToolbar parent;
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

#endif /* budgie_control_bar_h */
