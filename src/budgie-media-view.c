/*
 * budgie-media-view.c
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
#include <stdlib.h>
#include <string.h>

#include "budgie-media-view.h"

G_DEFINE_TYPE(BudgieMediaView, budgie_media_view, GTK_TYPE_BIN);

/* Initialisation */
static void budgie_media_view_class_init(BudgieMediaViewClass *klass)
{
        GObjectClass *g_object_class;

        g_object_class = G_OBJECT_CLASS(klass);
        g_object_class->dispose = &budgie_media_view_dispose;
}

static void budgie_media_view_init(BudgieMediaView *self)
{
        /* Not yet implemented */
}

static void budgie_media_view_dispose(GObject *object)
{
        BudgieMediaView *self;

        self = BUDGIE_MEDIA_VIEW(object);

        /* Destruct */
        G_OBJECT_CLASS (budgie_media_view_parent_class)->dispose (object);
}

/* Utility; return a new BudgieMediaView */
GtkWidget* budgie_media_view_new(void)
{
        BudgieMediaView *self;

        self = g_object_new(BUDGIE_MEDIA_VIEW_TYPE, NULL);
        return GTK_WIDGET(self);
}
