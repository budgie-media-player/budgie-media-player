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
        /* TODO: Add codes */
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
