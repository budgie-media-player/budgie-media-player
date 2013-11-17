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
#include "db/budgie-db.h"

/**
 * Automate button creation
 * @param theme An initialised GtkIconTheme
 * @param icon_name Required icon name
 * @param toolbar Whether a toolbar size should be used
 * @param toggle Whether this should be a ToggleButton
 * @param description Tooltip for the new button
 * @return a GtkWidget of the correct button type
 */
GtkWidget* new_button_with_icon(GtkIconTheme *theme,
                                const gchar *icon_name,
                                gboolean toolbar,
                                gboolean toggle,
                                const gchar *description);

/**
 * Search a directory for files, and populate the list
 * @param dir The directory to search
 * @param list A singly-linked list to populate with search results
 * @param n_params Number of following mime type prefixes
 * @param mimes Array of mime prefixes to find (i.e. audio/)
 */
void search_directory(const gchar *dir, GSList **list, int n_params, const gchar **mimes);

/**
 * Convert seconds into human readable time
 *
 * The caller must free the returned string using g_free
 * @param time Seconds to convert
 * @param remaining Whether this is the remaining time (prefix with -)
 * @return a string representation of the time
 */
gchar *format_seconds(gint64 time, gboolean remaining);


/**
 * Get the albumart name for the given MediaInfo
 * Note this is deliberately designed to adhere to the spec put forth
 * by GNOME: https://wiki.gnome.org/MediaArtStorageSpec
 *
 * @param info MediaInfo to query
 * @param extension "jpeg" or "png", JPEG being more common
 * @return The albumart (allocated), or NULL
 */
gchar *albumart_name_for_media(MediaInfo *info, gchar *extension);

/**
 * Following are taken from GNOME Wiki/Tracker code to ensure we stay
 * compatible in our mediaart spec
 */
static gboolean
strip_find_next_block (const gchar    *original,
                       const gunichar  open_char,
                       const gunichar  close_char,
                       gint           *open_pos,
                       gint           *close_pos);

gchar *
albumart_strip_invalid_entities (const gchar *original);

/**
 * Utility of mine to clean the album string before processing
 */
gchar *cleaned_string(gchar *string);
