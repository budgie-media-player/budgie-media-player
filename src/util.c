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

#include "util.h"
#include <gio/gio.h>

void search_directory(const gchar *path, GSList **list, const gchar *mime_pattern)
{
        GFile *file = NULL;
        GFileInfo *next_file;
        GFileType type;
        GFileEnumerator *listing;
        const gchar *next_path;
        const gchar *file_mime;
        gchar *full_path = NULL;

        file = g_file_new_for_path(path);
        type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
        if (type == G_FILE_TYPE_DIRECTORY) {
                /* Enumerate children (needs less query flags!) */
                listing = g_file_enumerate_children(file, "standard::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                        NULL, NULL);

                /* Lets go through them */
                while ((next_file = g_file_enumerator_next_file(listing, NULL, NULL)) != NULL) {
                        next_path = g_file_info_get_name(next_file);
                        full_path = g_strdup_printf("%s/%s", path, next_path);

                        /* Recurse if its a directory */
                        if (g_file_info_get_file_type(next_file) == G_FILE_TYPE_DIRECTORY) {
                                search_directory(full_path, list, mime_pattern);
                        } else {
                                /* Not exactly a regex but it'll do for now */
                                file_mime = g_file_info_get_content_type(next_file);
                                if (g_str_has_prefix(file_mime, mime_pattern)) {
                                        /* Probably switch to a new struct in the future */
                                        *list = g_slist_append(*list, g_strdup(full_path));
                                }
                        }
                        g_free(full_path);
                        g_object_unref(next_file);
                        full_path = NULL;
                }
                g_file_enumerator_close(listing, NULL, NULL);
                g_object_unref(listing);
        }

        g_object_unref(file);
}
