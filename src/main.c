/*
 * main.c
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
#include "budgie-window.h"

static void perform_migration(void)
{
        int i;
        gchar *path;
        const gchar *config;
        GFile *file;
        guint path_len;

        const gchar* paths[] = {
                "idmp-1.db",
                "budgie-1.db"
        };
        path_len = 2;

        /* $HOME/.config/ */
        config = g_get_user_config_dir();
        for (i=0; i < path_len; i++) {
                path = g_strdup_printf("%s/%s", config, paths[i]);
                file = g_file_new_for_path(path);
                /* Attempt to delete old database files */
                if (g_file_query_exists(file, NULL)) {
                        if (!g_file_delete(file, NULL, NULL)) {
                                g_warning("Unable to delete old DB: %s", path);
                        }
                }
                g_object_unref(file);
                g_free(path);
        }
}
int main(int argc, char **argv)
{
        BudgieWindow *window;
        gtk_init(&argc, &argv);

        perform_migration();
        window = budgie_window_new();
        gtk_main();

        g_object_unref(window);

        return EXIT_SUCCESS;
}

