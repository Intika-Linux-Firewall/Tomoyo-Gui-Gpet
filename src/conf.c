/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * conf.c
 * Copyright (C) Yoshihiro Kusuno 2010 <yocto@users.sourceforge.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gpet.h"

#define PRINT_WARN(err) {g_warning("%s", err->message);g_error_free(err);err = NULL;}

#define DIRECTORY		"/apps/gpet"
#define WINDOW_POSITION	DIRECTORY"/window_position"
#define WINDOW_SIZE		DIRECTORY"/window_size"
#define PANED_POSITION	DIRECTORY"/paned_position"

void read_config(transition_t *tran)
{
	GConfClient	*client;
	GError		*err = NULL;
	gint		x, y, w, h;
	GtkPaned	*paned;
	gint		paned_position;

	gtk_widget_set_size_request(tran->window, 640, 480);

	client = gconf_client_get_default();
	if (!gconf_client_dir_exists(client, DIRECTORY, &err)) {
		return;
	}

	if (gconf_client_get_pair(client, WINDOW_POSITION,
			GCONF_VALUE_INT, GCONF_VALUE_INT, &x, &y, &err))
		gtk_window_move(GTK_WINDOW(tran->window), x, y);
	else
		PRINT_WARN(err);

	if (gconf_client_get_pair(client, WINDOW_SIZE,
			GCONF_VALUE_INT, GCONF_VALUE_INT, &w, &h, &err))
		gtk_window_set_default_size(GTK_WINDOW(tran->window), w, h);
	else
		PRINT_WARN(err);

	/* get gpet.c main()*/
	paned = g_object_get_data(G_OBJECT(tran->window), "pane");
	if (paned) {
		paned_position = gconf_client_get_int(
					client, PANED_POSITION, &err);
		if (!err)
			gtk_paned_set_position(paned, paned_position);
		else
			PRINT_WARN(err);
	}
}

void write_config(transition_t *tran)
{
	GConfClient	*client;
	GError		*err = NULL;
	gint		x, y, w, h;
	GtkPaned	*paned;
	gint		paned_position;

	client = gconf_client_get_default();
	if (!gconf_client_dir_exists(client, DIRECTORY, &err)) {
		gconf_client_add_dir(client, DIRECTORY,
				GCONF_CLIENT_PRELOAD_NONE, &err);
	}
	if (err) {
		PRINT_WARN(err);
		return;
	}

	gtk_window_get_position(GTK_WINDOW(tran->window), &x, &y);
	if (!gconf_client_set_pair(client, WINDOW_POSITION,
			GCONF_VALUE_INT, GCONF_VALUE_INT, &x, &y, &err)) {
		PRINT_WARN(err);
	}

	gtk_window_get_size(GTK_WINDOW(tran->window), &w, &h);
	if (!gconf_client_set_pair(client, WINDOW_SIZE,
			GCONF_VALUE_INT, GCONF_VALUE_INT, &w, &h, &err)) {
		PRINT_WARN(err);
	}

	paned = g_object_get_data(G_OBJECT(tran->window), "pane");
	if (paned) {
		paned_position = gtk_paned_get_position(paned);
		if (!gconf_client_set_int(client, PANED_POSITION,
						paned_position, &err)) {
			PRINT_WARN(err);
		}
	}
}

