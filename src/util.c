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
#include <id3.h>

#include "util.h"
#include "db/media-db.h"

/* Unneeded constants but improve readability */
#define MINUTE 60
#define HOUR MINUTE*60

/* Set a field (by pointer) to the requested id3 value */
static void set_field(char** out, ID3Tag *tag, ID3_FieldID id, ID3_FrameID fid)
{
        ID3Frame *frame = NULL;
        ID3Field *field = NULL;
        size_t size;
        char *set;

        frame = ID3Tag_FindFrameWithID(tag, fid);
        if (frame == NULL)
                goto end;

        field = ID3Frame_GetField(frame, id);
        if (field == NULL)
                goto end;

        size = ID3Field_Size(field);
        size += 1;
        set = malloc(size);
        if (!set)
                goto end;
        ID3Field_GetASCII(field, set, size);
        *out = g_strescape(set, NULL);
        free(set);
end:
        return;
}

MediaInfo* media_from_file(gchar *path, GFileInfo *file_info)
{
        MediaInfo* media;
        ID3Tag *tag = NULL;

        media = malloc(sizeof(MediaInfo));
        memset(media, 0, sizeof(MediaInfo));

        tag = ID3Tag_New();
        ID3Tag_Link(tag, path);
        if (tag) {
                set_field(&(media->title), tag, ID3FN_TEXT, ID3FID_TITLE);

                set_field(&(media->artist), tag, ID3FN_TEXT, ID3FID_LEADARTIST);

                set_field(&(media->album), tag, ID3FN_TEXT, ID3FID_ALBUM);

                set_field(&(media->genre), tag, ID3FN_TEXT, ID3FID_CONTENTTYPE);
        }
        if (!media->title)
                media->title = g_strdup(g_file_info_get_display_name(file_info));
        media->path = g_strdup(path);

        if (tag)
                ID3Tag_Delete(tag);
        return media;
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

gchar *format_seconds(gint64 time, gboolean remaining)
{
        guint seconds = 0;
        guint minutes = 0;
        guint hours = 0;
        gchar *ret;
        gchar *prefix;

        if (remaining)
                prefix = "-";
        else
                prefix = "";

        while (time >= HOUR) {
                time -= HOUR;
                hours++;
        }
        while (time >= MINUTE) {
                time -= MINUTE;
                minutes++;
        }
        seconds = time;

        if (hours > 0)
                ret = g_strdup_printf("%s%02d:%02d:%02d", prefix, hours, minutes, seconds);
        else
                ret = g_strdup_printf("%s%02d:%02d", prefix, minutes, seconds);
        return ret;
}
