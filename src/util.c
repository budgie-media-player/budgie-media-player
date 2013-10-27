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
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "media.h"

MediaInfo* media_from_file(gchar *path, GFileInfo *file_info)
{
        MediaInfo* media;
        media = malloc(sizeof(MediaInfo));
        memset(media, 0, sizeof(MediaInfo));

        media->title = g_strdup(g_file_info_get_display_name(file_info));
        media->path = g_strdup(path);

        return media;
}

void free_media_info(gpointer p_info)
{
        MediaInfo *info;

        info = (MediaInfo*)p_info;
        if (info->title)
                g_free(info->title);
        if (info->artist)
                g_free(info->artist);
        if (info->album)
                g_free(info->album);
        if (info->genre)
                g_free(info->genre);
        if (info->path)
                g_free(info->path);
}

void search_directory(const gchar *path, GSList **list, const gchar *mime_pattern)
{
        GFile *file = NULL;
        GFileInfo *next_file;
        GFileType type;
        GFileEnumerator *listing;
        const gchar *next_path;
        const gchar *file_mime;
        gchar *full_path = NULL;
        MediaInfo *media;

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
                                        media = media_from_file(full_path, next_file);
                                        /* Probably switch to a new struct in the future */
                                        *list = g_slist_append(*list, media);
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
