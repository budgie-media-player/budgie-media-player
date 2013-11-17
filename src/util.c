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
        if (!frame || !field)
                *out = NULL;
        return;
}

static MediaInfo* media_from_file(gchar *path, GFileInfo *file_info, const gchar *file_mime)
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
        media->mime = g_strdup(file_mime);

        if (tag)
                ID3Tag_Delete(tag);
        return media;
}

void search_directory(const gchar *path, GSList **list, int n_params, const gchar **mimes)
{
        GFile *file = NULL;
        GFileInfo *next_file;
        GFileType type;
        GFileEnumerator *listing;
        const gchar *next_path;
        const gchar *file_mime;
        gchar *full_path = NULL;
        MediaInfo *media;
        guint i;

        file = g_file_new_for_path(path);
        type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
        if (type == G_FILE_TYPE_DIRECTORY) {
                /* Enumerate children (needs less query flags!) */
                listing = g_file_enumerate_children(file, "standard::*", G_FILE_QUERY_INFO_NONE,
                        NULL, NULL);

                /* Lets go through them */
                while ((next_file = g_file_enumerator_next_file(listing, NULL, NULL)) != NULL) {
                        next_path = g_file_info_get_name(next_file);
                        full_path = g_strdup_printf("%s/%s", path, next_path);

                        /* Recurse if its a directory */
                        if (g_file_info_get_file_type(next_file) == G_FILE_TYPE_DIRECTORY) {
                                search_directory(full_path, list, n_params, mimes);
                        } else {
                                /* Not exactly a regex but it'll do for now */
                                file_mime = g_file_info_get_content_type(next_file);
                                for (i=0; i < n_params; i++) {
                                        if (g_str_has_prefix(file_mime, mimes[i])) {
                                                media = media_from_file(full_path, next_file, file_mime);
                                                /* Probably switch to a new struct in the future */
                                                *list = g_slist_append(*list, media);
                                        }
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

GtkWidget* new_button_with_icon(GtkIconTheme *theme,
                                const gchar *icon_name,
                                gboolean toolbar,
                                gboolean toggle,
                                const gchar *description)
{
        GtkWidget *button;
        GtkWidget *image;
        gint size;

        size = toolbar ? GTK_ICON_SIZE_SMALL_TOOLBAR : GTK_ICON_SIZE_DIALOG;
        image = gtk_image_new_from_icon_name(icon_name, size);
        /* Create the button */
        if (toggle)
                button = gtk_toggle_button_new();
        else
                button = gtk_button_new();
        gtk_widget_set_can_focus(button, FALSE);
        if (!toolbar)
                gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_container_add(GTK_CONTAINER(button), image);

        /* Set a helpful tooltip for toolbar buttons */
        if (toolbar)
                gtk_widget_set_tooltip_text(button, description);
        return button;
}

gchar *format_seconds(gint64 time, gboolean remaining)
{
        div_t dv, dv2;
        gchar *ret;
        gchar *prefix;

        if (remaining)
                prefix = "-";
        else
                prefix = "";

        dv = div(time, 60);
        if (dv.quot < 60) {
                ret = g_strdup_printf("%s%02d:%02d", prefix, dv.quot, dv.rem);
        } else {
                dv2 = div(dv.quot, 60);
                ret = g_strdup_printf("%s%02d:%02d:%02d", prefix, dv2.quot, dv2.rem, dv.rem);
        }
        return ret;
}

static gboolean
strip_find_next_block (const gchar    *original,
                       const gunichar  open_char,
                       const gunichar  close_char,
                       gint           *open_pos,
                       gint           *close_pos)
{
        const gchar *p1, *p2;

        if (open_pos) {
                *open_pos = -1;
        }

        if (close_pos) {
                *close_pos = -1;
        }

        p1 = g_utf8_strchr (original, -1, open_char);
        if (p1) {
                if (open_pos) {
                        *open_pos = p1 - original;
                }

                p2 = g_utf8_strchr (g_utf8_next_char (p1), -1, close_char);
                if (p2) {
                        if (close_pos) {
                                *close_pos = p2 - original;
                        }
                        
                        return TRUE;
                }
        }

        return FALSE;
}

gchar *
albumart_strip_invalid_entities (const gchar *original)
{
        GString         *str_no_blocks;
        gchar          **strv;
        gchar           *str;
        gboolean         blocks_done = FALSE;
        const gchar     *p;
        const gchar     *invalid_chars = "()[]<>{}_!@#$^&*+=|\\/\"'?~";
        const gchar     *invalid_chars_delimiter = "*";
        const gchar     *convert_chars = "\t";
        const gchar     *convert_chars_delimiter = " ";
        const gunichar   blocks[5][2] = {
                { '(', ')' },
                { '{', '}' }, 
                { '[', ']' }, 
                { '<', '>' }, 
                {  0,   0  }
        };

        str_no_blocks = g_string_new ("");

        p = original;

        while (!blocks_done) {
                gint pos1, pos2, i;

                pos1 = -1;
                pos2 = -1;
        
                for (i = 0; blocks[i][0] != 0; i++) {
                        gint start, end;
                        
                        /* Go through blocks, find the earliest block we can */
                        if (strip_find_next_block (p, blocks[i][0], blocks[i][1], &start, &end)) {
                                if (pos1 == -1 || start < pos1) {
                                        pos1 = start;
                                        pos2 = end;
                                }
                        }
                }
                
                /* If either are -1 we didn't find any */
                if (pos1 == -1) {
                        /* This means no blocks were found */
                        g_string_append (str_no_blocks, p);
                        blocks_done = TRUE;
                } else {
                        /* Append the test BEFORE the block */
                        if (pos1 > 0) {
                                g_string_append_len (str_no_blocks, p, pos1);
                        }

                        p = g_utf8_next_char (p + pos2);

                        /* Do same again for position AFTER block */
                        if (*p == '\0') {
                                blocks_done = TRUE;
                        }
                }       
        }

        str = g_string_free (str_no_blocks, FALSE);

        /* Now strip invalid chars */
        g_strdelimit (str, invalid_chars, *invalid_chars_delimiter);
        strv = g_strsplit (str, invalid_chars_delimiter, -1);
        g_free (str);
        str = g_strjoinv (NULL, strv);
        g_strfreev (strv);

        /* Now convert chars */
        g_strdelimit (str, convert_chars, *convert_chars_delimiter);
        strv = g_strsplit (str, convert_chars_delimiter, -1);
        g_free (str);
        str = g_strjoinv (convert_chars_delimiter, strv);
        g_strfreev (strv);

        /* Now remove double spaces */
        strv = g_strsplit (str, "  ", -1);
        g_free (str);
        str = g_strjoinv (" ", strv);
        g_strfreev (strv);
        
        /* Now strip leading/trailing white space */
        g_strstrip (str);

        return str;
}

gchar *cleaned_string(gchar *string)
{
        gchar *stripped, *normalized, *lower;

        stripped = albumart_strip_invalid_entities(string);
        normalized = g_utf8_normalize(stripped, -1, G_NORMALIZE_ALL);
        g_free(stripped);
        lower = g_utf8_strdown(normalized, -1);
        g_free(normalized);

        return lower;
}

gchar *albumart_name_for_media(MediaInfo *info, gchar *extension)
{
        if (!info->album || ! info->artist)
                return NULL;

        char *album = cleaned_string(info->album);
        gchar *artist = cleaned_string(info->album);
        gchar *artist_md5, *album_md5;
        gchar *album_string = NULL;

        artist_md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, artist, -1);
        album_md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, album, -1);

        album_string = g_strdup_printf("album-%s-%s.%s", artist_md5,
                album_md5, extension);

        g_free(artist_md5);
        g_free(artist);
        g_free(album_md5);
        g_free(album);

        return album_string;
}
