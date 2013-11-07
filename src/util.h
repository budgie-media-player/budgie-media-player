/*
 * util.h
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
#pragma once
#include <glib.h>
#include <gtk/gtk.h>

/**
 * Automate button creation
 * @param theme An initialised GtkIconTheme
 * @param icon_name Required icon name
 * @param toolbar Whether a toolbar size should be used
 * @param toggle Whether this should be a ToggleButton
 * @return a GtkWidget of the correct button type
 */
GtkWidget* new_button_with_icon(GtkIconTheme *theme,
                                const gchar *icon_name,
                                gboolean toolbar,
                                gboolean toggle);

/**
 * Search a directory for files, and populate the list
 * @param dir The directory to search
 * @param list A singly-linked list to populate with search results
 * @param mime_pattern Prefix mime to find (i.e. audio/)
 */
void search_directory(const gchar *dir, GSList **list, const gchar *mime_pattern);

/**
 * Convert seconds into human readable time
 *
 * The caller must free the returned string using g_free
 * @param time Seconds to convert
 * @param remaining Whether this is the remaining time (prefix with -)
 * @return a string representation of the time
 */
gchar *format_seconds(gint64 time, gboolean remaining);
