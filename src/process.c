/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * process.c
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gpet.h"

enum task_column_pos {
	COLUMN_INDEX,			// data index (invisible)
	COLUMN_NUMBER,		// n
	COLUMN_COLON,			// :
	COLUMN_PROFILE,		// profile
	COLUMN_NAME,			// process name + pid + domain name
	N_COLUMNS_TREE
};

/*---------------------------------------------------------------------------*/
static int add_task_tree_store(GtkTreeStore *store,
			GtkTreeIter *parent_iter,
			task_list_t *tsk, int *number, int nest)
{
	GtkTreeIter	iter;
	gchar		*str_prof;
	gchar		*line;
	int		n = 0, index;

	gtk_tree_store_append(store, &iter, parent_iter);

	index = tsk->task[*number].index;
	str_prof = g_strdup_printf("%3u", tsk->task[*number].profile);
	line = g_strdup_printf("%s (%u) %s",
				tsk->task[*number].name,
				tsk->task[*number].pid,
				tsk->task[*number].domain);
	gtk_tree_store_set(store, &iter,
				COLUMN_INDEX,		index,
				COLUMN_NUMBER,  	*number,
				COLUMN_COLON,		":",
				COLUMN_PROFILE,	str_prof,
				COLUMN_NAME,		line,
				-1);
	DEBUG_PRINT("[%3d]%3d(%d):%s %s\n", index, *number, nest, str_prof, line);
	g_free(str_prof);
	g_free(line);

	(*number)++;
	while (*number < tsk->count) {
		n = tsk->task[*number].depth;
		if (n > nest) {
			n = add_task_tree_store(store, &iter, tsk, number, n);
		} else
			break;
	}

	return n;
}

void add_task_tree_data(GtkTreeView *treeview, task_list_t *tsk)
{
	GtkTreeStore	*store;
	GtkTreeIter	*iter = NULL;
	int		number = 0, nest = -1;
	
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_store_clear(store);
	add_task_tree_store(store, iter, tsk, &number, nest);
}

void set_select_flag_process(gpointer data, task_list_t *tsk)
{
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	gint			index;

	model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(tsk->treeview));
	if (!model || !gtk_tree_model_get_iter(model, &iter, data)) {
		g_warning("ERROR: %s(%d)", __FILE__, __LINE__);
		return;
	}
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);

	DEBUG_PRINT("index[%d]\n", index);
	tsk->task[index].selected = 1;
}
/*---------------------------------------------------------------------------*/
static GtkTreeViewColumn *column_add(
		GtkCellRenderer *renderer, const GtkWidget *treeview,
		const gchar *title, enum task_column_pos pos,
		const gchar *attribute, const gfloat xalign)
{
	GtkTreeViewColumn	*column;

	g_object_set(renderer, "xalign", xalign, "ypad", 0, NULL);
	column = gtk_tree_view_column_new_with_attributes(
				title, renderer, attribute, pos, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	return column;
}
/*---------------------------------------------------------------------------*/
static gboolean cb_select_process(GtkTreeView *treeview,
					GdkEventButton *event,
					transition_t *transition)
{
	transition->current_page = CCS_SCREEN_DOMAIN_LIST;

	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	if (event->button == 3) {
		GtkWidget	*popup;
		/* get menu.c create_menu()*/
		popup = g_object_get_data(
				G_OBJECT(transition->window), "popup");
		
		gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
					0, gtk_get_current_event_time());
		return TRUE;
	}

	return FALSE;
}

static void cb_selection_proc(GtkTreeSelection *selection,
					transition_t *transition)
{
	GtkTreeIter	iter;
	gint		select_count;
	GtkTreeModel	*model;
	GList		*list;
	gint		index;
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;

	select_count = gtk_tree_selection_count_selected_rows(selection);
	if (0 == select_count)
		return;

	model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(transition->tsk.treeview));
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);
	gtk_entry_set_text(GTK_ENTRY(transition->domainbar),
				transition->tsk.task[index].domain);

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			transition->acl.listview), &path, &column);

	get_process_acl_list(index,
		&(transition->acl.list), &(transition->acl.count));
	add_list_data(&(transition->acl), TRUE);

	view_cursor_set(transition->acl.listview, path, column);

	disp_statusbar(transition, CCS_SCREEN_ACL_LIST);
}
/*---------------------------------------------------------------------------*/
GtkWidget *create_task_tree_model(transition_t *transition)
{
	GtkWidget		*treeview;
	GtkTreeStore		*store;
	GtkTreeViewColumn	*column;

	store = gtk_tree_store_new(N_COLUMNS_TREE,
				G_TYPE_INT,
				G_TYPE_INT,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"No.", COLUMN_NUMBER, "text", 1.0);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			" ", COLUMN_COLON, "text", 0.5);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"prof", COLUMN_PROFILE, "text", 1.0);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"Name", COLUMN_NAME, "text", 0.0);
	// 開く位置
	gtk_tree_view_set_expander_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_spacing(column, 1);
		// ツリーインデント pixel
	gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(treeview), 2);
		// ツリー開くマーク
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(treeview), TRUE);
	g_object_set(G_OBJECT(treeview), "enable-tree-lines", TRUE,
						NULL);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeview),
					GTK_TREE_VIEW_GRID_LINES_NONE);
	// ヘッダ非表示
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	view_setting(treeview, COLUMN_NAME);

	// cursor move  process window
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
		"changed", G_CALLBACK(cb_selection_proc), transition);
	// mouse click  process window
	g_signal_connect(G_OBJECT(treeview), "button-press-event", 
			 G_CALLBACK(cb_select_process), transition);

	return treeview;
}

