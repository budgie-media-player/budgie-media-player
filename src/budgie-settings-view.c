/*
 * budgie-settings-view.c
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

#include "budgie-settings-view.h"
#include "util.h"

G_DEFINE_TYPE(BudgieSettingsView, budgie_settings_view, GTK_TYPE_BOX);

/* Initialisation */
static void budgie_settings_view_class_init(BudgieSettingsViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_settings_view_dispose;
}

static void budgie_settings_view_init(BudgieSettingsView *self)
{
        /* TODO: Add codes */
}

static void budgie_settings_view_dispose(GObject *object)
{
        BudgieSettingsView *self;

        self = BUDGIE_SETTINGS_VIEW(object);

        /* Destruct */
        G_OBJECT_CLASS (budgie_settings_view_parent_class)->dispose (object);
}

/* Utility; return a new BudgieSettingsView */
GtkWidget* budgie_settings_view_new(void)
{
        BudgieSettingsView *self;

        self = g_object_new(BUDGIE_SETTINGS_VIEW_TYPE, "orientation",
                GTK_ORIENTATION_VERTICAL, NULL);
        return GTK_WIDGET(self);
}
