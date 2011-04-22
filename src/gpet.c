/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * gpet.c
 * Copyright (C) Yoshihiro Kusuno 2010,2011 <yocto@users.sourceforge.jp>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gpet.h"

/*---------------------------------------------------------------------------*/
enum tree_column_pos {
	COLUMN_INDEX,			// data index (invisible)
	COLUMN_NUMBER,			// n
	COLUMN_COLON,			// :
	COLUMN_PROFILE,			// profile
	COLUMN_KEEPER_DOMAIN,		// #
	COLUMN_INITIALIZER_TARGET,	// *
	COLUMN_DOMAIN_UNREACHABLE,	// !
	COLUMN_DOMAIN_NAME,		// domain name
	COLUMN_COLOR,			// text color
	COLUMN_REDIRECT,		// redirect index (invisible)
	N_COLUMNS_TREE
};
/*---------------------------------------------------------------------------*/
static int add_tree_store(GtkTreeStore *store, GtkTreeIter *parent_iter,
			struct ccs_domain_policy *dp, int *index, int nest)
{
	GtkTreeIter	iter;
	gchar		*color = "black";
	gchar		*str_num, *str_prof;
	gchar		*line = NULL, *is_dis = NULL, *domain;
	const char	*sp;
	const struct ccs_transition_control_entry *transition_control;
	int		n, number, redirect_index = -1;

	sp = ccs_domain_name(dp, *index);
	for (n = 0; ; n++) {
		const char *cp = strchr(sp, ' ');
		if (!cp)
			break;
		sp = cp + 1;
	}

	gtk_tree_store_append(store, &iter, parent_iter);
	number = dp->list[*index].number;
	if (number >= 0) {
		str_num = g_strdup_printf("%4d", number);
		str_prof = dp->list[*index].profile_assigned ?
			g_strdup_printf("%3u", dp->list[*index].profile) :
			g_strdup("???");
	} else {
		str_num = g_strdup("");
		str_prof = g_strdup("");
	}

	gtk_tree_store_set(store, &iter,
		COLUMN_INDEX,		*index,
		COLUMN_NUMBER,  	str_num,
		COLUMN_COLON,		number >= 0 ? ":" : "",
		COLUMN_PROFILE,		str_prof,
		COLUMN_KEEPER_DOMAIN,	dp->list[*index].is_dk ? "#" : " ",
		COLUMN_INITIALIZER_TARGET, dp->list[*index].is_dit ? "*" : " ",
		COLUMN_DOMAIN_UNREACHABLE, dp->list[*index].is_du ? "!" : " ",
		-1);
	g_free(str_num);
	g_free(str_prof);

	transition_control = dp->list[*index].d_t;
	if (transition_control && !(dp->list[*index].is_dis)) {
		line = g_strdup_printf(" ( %s%s from %s )",
			get_transition_name(transition_control->type),
			transition_control->program ?
				transition_control->program->name : "any",
			transition_control->domainname ?
				transition_control->domainname->name : "any");
		color =
		transition_control->type == CCS_TRANSITION_CONTROL_KEEP ?
		"green" : "cyan";
	} else if (dp->list[*index].is_dis) {	/* initialize_domain */
		is_dis = g_strdup_printf(CCS_ROOT_NAME "%s",
				strrchr(ccs_domain_name(dp, *index), ' '));
		redirect_index = ccs_find_domain(dp, is_dis, false, false);
		g_free(is_dis);
		color = "blue";
		if (redirect_index >= 0)
			is_dis = g_strdup_printf(" ( -> %d )",
				       	dp->list[redirect_index].number);
		else
			is_dis = g_strdup_printf(" ( -> Not Found )");
	} else if (dp->list[*index].is_dd) {	/* delete_domain */
		color = "gray";
	}
	domain = g_strdup_printf("%s%s%s%s%s",
			dp->list[*index].is_dd ? "( " : "",
			sp,
			dp->list[*index].is_dd ? " )" : "",
			line ? line : "",
			is_dis ? is_dis : ""
			);
	gtk_tree_store_set(store, &iter, COLUMN_DOMAIN_NAME, domain,
					 COLUMN_COLOR, color,
					 COLUMN_REDIRECT, redirect_index, -1);
	g_free(line);
	g_free(is_dis);
	g_free(domain);

	(*index)++;

	while (*index < dp->list_len) {
		sp = ccs_domain_name(dp, *index);
		for (n = 0; ; n++) {
			const char *cp = strchr(sp, ' ');
			if (!cp)
				break;
			sp = cp + 1;
		}

		if (n > nest)
			n = add_tree_store(store, &iter, dp, index, n);
		else
			break;
	}

	return n;
}

void add_tree_data(GtkTreeView *treeview, struct ccs_domain_policy *dp)
{
	GtkTreeStore	*store;
	GtkTreeIter	*iter = NULL;
	int		index = 0, nest = -1;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
	gtk_tree_store_clear(store);
	add_tree_store(store, iter, dp, &index, nest);
}
/*---------------------------------------------------------------------------*/
static GtkTreeViewColumn *column_add(
		GtkCellRenderer *renderer,
		const GtkWidget *treeview,
		const gchar *title,
		enum tree_column_pos pos,
		const gchar *attribute,
		const gfloat xalign
		)
{
	GtkTreeViewColumn	*column;

	g_object_set(renderer, "xalign", xalign, "ypad", 0, NULL);
	column = gtk_tree_view_column_new_with_attributes(
				title, renderer, attribute, pos, NULL);
	gtk_tree_view_column_add_attribute(column, renderer,
		       			"foreground", COLUMN_COLOR);
//	gtk_tree_view_column_set_alignment(column, xalign);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	return column;
}
/*---------------------------------------------------------------------------*/
/*
static void cb_status_toggled(GtkCellRendererToggle	*renderer,
			       gchar			*path_string,
			       gpointer			user_data)
{
	GtkTreeModel	*model;
	GtkTreeIter	iter;
	GtkTreePath	*path;
	gboolean	status;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(user_data));
	path  = gtk_tree_path_new_from_string(path_string);

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &status, -1);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		      COLUMN_INDEX, !status, -1);
	gtk_tree_path_free(path);
}
*/
/*---------------------------------------------------------------------------*/
static GtkWidget *create_tree_model(void)
{
	GtkWidget		*treeview;
	GtkTreeStore		*store;
//	GtkCellRenderer	*renderer;
	GtkTreeViewColumn	*column;

	store = gtk_tree_store_new(N_COLUMNS_TREE,
				G_TYPE_INT,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_INT);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

// TODO refactoring
//	renderer = gtk_cell_renderer_toggle_new();
//	g_signal_connect(renderer, "toggled",
//		    G_CALLBACK(cb_status_toggled), treeview);
/*
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "visible", FALSE, NULL);
	column = column_add(renderer, treeview,
			"", COLUMN_INDEX, "text", 0.0);
*/
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"No.", COLUMN_NUMBER, "text", 1.0);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			" ", COLUMN_COLON, "text", 0.5);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"prof", COLUMN_PROFILE, "text", 1.0);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"#", COLUMN_KEEPER_DOMAIN, "text", 0.5);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"*", COLUMN_INITIALIZER_TARGET, "text", 0.5);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"!", COLUMN_DOMAIN_UNREACHABLE, "text", 0.5);
	column = column_add(gtk_cell_renderer_text_new(), treeview,
			"Domain Name", COLUMN_DOMAIN_NAME, "text", 0.0);
	// 開く位置
	gtk_tree_view_set_expander_column(GTK_TREE_VIEW(treeview), column);
//	gtk_tree_view_column_set_spacing(column, 1);
	// ヘッダ非表示
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

	return treeview;
}
/*---------------------------------------------------------------------------*/
enum list_column_pos {
	LIST_NUMBER,		// n
	LIST_COLON,		// :
	LIST_ALIAS,		//
	LIST_OPERAND,		//
	N_COLUMNS_LIST
};

void add_list_data(generic_list_t *generic, gboolean alias_flag)
{
	GtkListStore	*store;
	GtkTreeIter	iter;
	int		i;
	gchar		*str_num, *profile, *alias;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(
				GTK_TREE_VIEW(generic->listview)));

	gtk_list_store_clear(store);
	for(i = 0; i < generic->count; i++){
		str_num = g_strdup_printf("%4d", i);
		gtk_list_store_append(store, &iter);

		if (alias_flag) {
			alias = (gchar *)
			  ccs_directives[generic->list[i].directive].alias;
			gtk_list_store_set(store, &iter,
				LIST_NUMBER, str_num,
				LIST_COLON,  ":",
				LIST_ALIAS, alias,
				LIST_OPERAND, generic->list[i].operand,
				-1);
		} else {
			profile = g_strdup_printf("%3u-",
					generic->list[i].directive);
			alias = g_strdup_printf("%s%s",
					generic->list[i].directive < 256 ?
					profile : "",
					generic->list[i].operand);
			gtk_list_store_set(store, &iter,
					LIST_NUMBER, str_num,
					LIST_COLON,  ":",
					LIST_OPERAND, alias,
					-1);
			g_free(profile);
			g_free(alias);
		}
		g_free(str_num);
	}
}

static void disable_header_focus(GtkTreeViewColumn *column, const gchar *str)
{
	GtkWidget		*label, *dummy;
	GtkButton		*button = GTK_BUTTON(gtk_button_new());

	label = gtk_label_new(str);
	gtk_tree_view_column_set_widget(column, label);
	gtk_widget_show(label);
	dummy = gtk_tree_view_column_get_widget(column);
	while (dummy)	 {
//		g_print("<%s:%p> ", g_type_name(G_OBJECT_TYPE(dummy)), dummy);

		g_object_set(G_OBJECT(dummy), "can-focus", FALSE, NULL);
//		gtk_widget_set_can_focus(dummy, FALSE);	// 2.18
		if (G_OBJECT_TYPE(dummy) == G_OBJECT_TYPE(button))
			break;
		dummy = gtk_widget_get_parent(dummy);
	}
//	g_print("\n");
}

static GtkWidget *create_list_model(gboolean alias_flag)
{
	GtkWidget		*treeview;
	GtkListStore		*liststore;
	GtkCellRenderer	*renderer;
	GtkTreeViewColumn	*column;

	liststore = gtk_list_store_new(N_COLUMNS_LIST,
				  G_TYPE_STRING, G_TYPE_STRING,
				  G_TYPE_STRING, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	g_object_unref(liststore);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0, "ypad", 0, NULL);
	column = gtk_tree_view_column_new_with_attributes(
			"No.", renderer, "text", LIST_NUMBER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_sort_column_id(column, LIST_NUMBER);
//	gtk_tree_view_column_set_alignment(column, 1.0);
	disable_header_focus(column, "No.");

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(" ", renderer,
					      "text", LIST_COLON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	disable_header_focus(column, " ");

	if (alias_flag) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
			"directive", renderer, "text", LIST_ALIAS, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
		gtk_tree_view_column_set_sort_column_id(column, LIST_ALIAS);
		disable_header_focus(column, "directive");
	}

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("operand", renderer,
					      "text", LIST_OPERAND, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_sort_column_id(column, LIST_OPERAND);
	disable_header_focus(column, "operand");

	// ヘッダ表示
//	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);

	return treeview;
}
/*---------------------------------------------------------------------------*/
gint get_current_domain_index(transition_t *transition)
{
	GtkTreeSelection	*selection;
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	GList			*list;
	gint			index = -1;

	selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->treeview));
	if (selection &&
	    gtk_tree_selection_count_selected_rows(selection)) {
		list = gtk_tree_selection_get_selected_rows(selection, NULL);
		model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(transition->treeview));
		gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
		gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
	}
	DEBUG_PRINT("select Domain index[%d]\n", index);
	return index;
}

gchar *get_alias_and_operand(GtkWidget *view)
{
	GtkTreeSelection	*selection;
	GtkTreeIter		iter;
	GtkTreeModel		*model;
	GList			*list;
	gchar			*alias = NULL, *operand = NULL,
				*str_buff = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	if (selection &&
	    gtk_tree_selection_count_selected_rows(selection)) {
		list = gtk_tree_selection_get_selected_rows(selection, NULL);
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
		gtk_tree_model_get(model, &iter,
				LIST_ALIAS, &alias, LIST_OPERAND, &operand, -1);
		str_buff = g_strdup_printf("%s %s", alias, operand);
	}

	return str_buff;
}
/*---------------------------------------------------------------------------*/
static gboolean move_pos_list(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, transition_t *transition)
{
	GtkWidget		*view = NULL;
	GtkTreeSelection	*selection;
	const char		*domain;
	gint			index;
	gchar			*alias = NULL, *operand = NULL, *str_buff = NULL;
	gchar			*entry;
	int			cmp = -1;


	entry = get_combo_entry_last();

	switch((int)transition->addentry) {
	case ADDENTRY_DOMAIN_LIST :
		view = transition->treeview;
		gtk_tree_model_get(model, iter, COLUMN_INDEX, &index, -1);
		domain = ccs_domain_name(transition->dp, index);
		cmp = strcmp(entry, domain);
		break;
	case ADDENTRY_ACL_LIST :
		view = transition->acl.listview;
		gtk_tree_model_get(model, iter,
				LIST_ALIAS, &alias, LIST_OPERAND, &operand, -1);
		str_buff = g_strdup_printf("%s %s", alias, operand);	// TODO
		cmp = strcmp(entry, str_buff);
//g_print("%2d[%s][%s]\n", cmp, entry, str_buff);
		break;
	case ADDENTRY_EXCEPTION_LIST :
		view = transition->exp.listview;
		gtk_tree_model_get(model, iter,
				LIST_ALIAS, &alias, LIST_OPERAND, &operand, -1);
		str_buff = g_strdup_printf("%s %s", alias, operand);	// TODO
		cmp = strcmp(entry, str_buff);
		break;
	case ADDENTRY_PROFILE_LIST :
		view = transition->prf.listview;
		gtk_tree_model_get(model, iter, LIST_NUMBER, &str_buff, -1);
		cmp = atoi(entry) - transition->prf.list[atoi(str_buff)].directive;
//g_print("entry[%s] [%s:%d]\n", entry, str_buff, transition->prf.list[atoi(str_buff)].directive);
		break;
	}
	g_free(alias);
	g_free(operand);
	g_free(str_buff);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (cmp) {
		gtk_tree_selection_unselect_path(selection, path);
		return FALSE;
	} else {
		gtk_tree_selection_select_path(selection, path);
#if 0
	{// debug
		gchar *path_str = gtk_tree_path_to_string(path);
		g_print("Put Path[%s]\n", path_str);
		g_free(path_str);
	}
#endif
		return TRUE;
	}
}

void set_position_addentry(transition_t *transition, GtkTreePath **path)
{
	GtkWidget		*view = NULL;
	GtkTreeModel		*model;
	GtkTreeSelection	*selection;
	GList			*list;


	switch((int)transition->addentry) {
	case ADDENTRY_NON :
		return;
		break;
	case ADDENTRY_DOMAIN_LIST :
		view = transition->treeview;
		break;
	case ADDENTRY_ACL_LIST :
		view = transition->acl.listview;
		break;
	case ADDENTRY_EXCEPTION_LIST :
		view = transition->exp.listview;
		break;
	case ADDENTRY_PROFILE_LIST :
		view = transition->prf.listview;
		break;
	}

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	gtk_tree_model_foreach(model,
		(GtkTreeModelForeachFunc)move_pos_list, transition);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (list) {
		gtk_tree_path_free(*path);
		(*path) = gtk_tree_path_copy(g_list_first(list)->data);
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
	}
	transition->addentry = ADDENTRY_NON;

#if 0
	{// debug
		gchar *path_str = gtk_tree_path_to_string(*path);
		g_print("Get Path[%s]\n", path_str);
		g_free(path_str);
	}
#endif
}
/*---------------------------------------------------------------------------*/
static void cb_selection(GtkTreeSelection *selection,
				transition_t *transition)
{
	GtkTreeIter	iter;
	gint		select_count;
	GtkTreeModel	*model;
	GList		*list;
	gint		index;
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;

	DEBUG_PRINT("In  **************************** \n");
	select_count = gtk_tree_selection_count_selected_rows(selection);
	DEBUG_PRINT("select count[%d]\n", select_count);
	if (0 == select_count)
		return;

	model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(transition->treeview));
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);
	DEBUG_PRINT("--- index [%4d] ---\n", index);
	gtk_entry_set_text(GTK_ENTRY(transition->domainbar),
				ccs_domain_name(transition->dp, index));

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			transition->acl.listview), &path, &column);

	get_acl_list(transition->dp, index,
		&(transition->acl.list), &(transition->acl.count));
	add_list_data(&(transition->acl), TRUE);

	if (transition->acl.count) {
		set_position_addentry(transition, &path);
		DEBUG_PRINT("ACL count<%d>\n", transition->acl.count);
		DEBUG_PRINT("ACL ");
		view_cursor_set(transition->acl.listview, path, column);
//		gtk_widget_grab_focus(transition->acl.listview);
		disp_statusbar(transition, CCS_SCREEN_ACL_LIST);
	} else {	/* delete_domain or initializer_source */
		disp_statusbar(transition, CCS_MAXSCREEN);
	}
	DEBUG_PRINT("Out **************************** \n");
}
#if 0
static void cb_selection (GtkTreeView		*treeview,
				transition_t		*transition)
{
	GtkTreeSelection	*selection;
	GtkTreeIter	iter;
	gint		select_count;
	GtkTreeModel	*model;
	GList		*list;
	gint		index;
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;

	DEBUG_PRINT("In  **************************** \n");
	selection = gtk_tree_view_get_selection(treeview);
	select_count = gtk_tree_selection_count_selected_rows(selection);
	DEBUG_PRINT("select count[%d]\n", select_count);
	if (0 == select_count)
		return;

	model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(transition->treeview));
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);
	DEBUG_PRINT("--- index [%4d] ---\n", index);
	gtk_entry_set_text(GTK_ENTRY(transition->domainbar),
				ccs_domain_name(transition->dp, index));

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			transition->acl.listview), &path, &column);

	get_acl_list(transition->dp, index,
		&(transition->acl.list), &(transition->acl.count));
	add_list_data(&(transition->acl), TRUE);

	if (transition->acl.count) {
		DEBUG_PRINT("ACL count<%d>\n", transition->acl.count);
		DEBUG_PRINT("ACL ");
		view_cursor_set(transition->acl.listview, path, column);
		disp_statusbar(transition, CCS_SCREEN_ACL_LIST);
	} else {	/* delete_domain or initializer_source */
		disp_statusbar(transition, CCS_MAXSCREEN);
	}
	DEBUG_PRINT("Out **************************** \n");
}
#endif
/*---------------------------------------------------------------------------*/
struct FindIsDis_t {
	gint		redirect_index;
	GtkTreeIter	iter;
	GtkTreePath	*path;
};

static gboolean find_is_dis(GtkTreeModel *model, GtkTreePath *path,
		GtkTreeIter *iter, struct FindIsDis_t *data)
{
	gint		index;

	gtk_tree_model_get(model, iter, COLUMN_INDEX, &index, -1);
	if (data->redirect_index == index) {
		data->iter = *iter;
		data->path = gtk_tree_path_copy(path);
		return TRUE;
	}
	return FALSE;
}

static void cb_initialize_domain(GtkTreeView *treeview, GtkTreePath *treepath,
			GtkTreeViewColumn treeviewcolumn, gpointer dummy)
{
	GtkTreeIter	iter;
	GtkTreeModel	*model;
	gboolean	ret;
	struct FindIsDis_t	data;

	DEBUG_PRINT("In  **************************** \n");
	model = gtk_tree_view_get_model(treeview);
	ret = gtk_tree_model_get_iter(model, &iter, treepath);
	if (ret)
		gtk_tree_model_get(model, &iter,
				COLUMN_REDIRECT, &data.redirect_index, -1);
	DEBUG_PRINT("redirect_index[%d]\n", data.redirect_index);
	if (!ret || data.redirect_index < 0)
		return;		/* not initialize_domain */

	data.path = NULL;
	gtk_tree_model_foreach(model,
	     	(GtkTreeModelForeachFunc)find_is_dis, &data);

	if (data.path) {
	  {
		gchar *path_str = gtk_tree_path_to_string(data.path);
		DEBUG_PRINT("TreePath[%s]\n", path_str);
		g_free(path_str);
	  }
		GtkTreeViewColumn	*column = NULL;
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(treeview), data.path);
		gtk_tree_selection_select_iter(
			gtk_tree_view_get_selection(
			GTK_TREE_VIEW(treeview)), &data.iter);
		DEBUG_PRINT("Domain ");
		view_cursor_set(GTK_WIDGET(treeview), data.path, column);
	}
	DEBUG_PRINT("Out **************************** \n");
}
/*---------------------------------------------------------------------------*/
void set_sensitive(GtkActionGroup *actions, int task_flag,
				enum ccs_screen_type current_page)
{
	gboolean	sens_edt, sens_add, sens_del, sens_tsk,
			sens_dch, sens_cpy, sens_opt;

	sens_edt = sens_add = sens_del = sens_tsk =
			sens_dch = sens_cpy = sens_opt = FALSE;

	switch((int)current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_MAXSCREEN :
		sens_edt = TRUE;
		sens_tsk = TRUE;
		sens_dch = TRUE;
		if (!task_flag) {
			sens_add = TRUE;
			sens_del = TRUE;
			sens_cpy = TRUE;
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		sens_add = TRUE;
		sens_del = TRUE;
		sens_tsk = TRUE;
		sens_dch = TRUE;
		sens_cpy = TRUE;
		sens_opt = TRUE;
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		sens_add = TRUE;
		sens_del = TRUE;
		sens_cpy = TRUE;
		break;
	case CCS_SCREEN_PROFILE_LIST :
		sens_edt = TRUE;
		sens_add = TRUE;
		break;
	}

	gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "Edit"), sens_edt);
	gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "Add"), sens_add);
	gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "Delete"), sens_del);
	gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "ACL"), sens_dch);
	gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "Copy"), sens_cpy);
	gtk_action_set_sensitive(gtk_action_group_get_action(
				actions, "OptimizationSupport"), sens_opt);

	if (!is_offline())
		gtk_action_set_sensitive(gtk_action_group_get_action(
					actions, "Process"), sens_tsk);
}

static gint popup_menu(transition_t *transition, guint button)
{
	if (button == 3) {
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

static gboolean cb_select_domain(GtkTreeView *treeview,  GdkEventButton *event,
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

static gboolean cb_select_acl(GtkTreeView *listview,  GdkEventButton *event,
				transition_t *transition)
{
	transition->current_page = CCS_SCREEN_ACL_LIST;
	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	return popup_menu(transition, event->button);
}

static gboolean cb_select_exp(GtkTreeView *listview,  GdkEventButton *event,
				transition_t *transition)
{
	transition->current_page = CCS_SCREEN_EXCEPTION_LIST;
	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	return popup_menu(transition, event->button);
}

static gboolean cb_select_prf(GtkTreeView *listview,  GdkEventButton *event,
				transition_t *transition)
{
	transition->current_page = CCS_SCREEN_PROFILE_LIST;
	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	return popup_menu(transition, event->button);
}
/*---------------------------------------------------------------------------*/
static gboolean inc_search(GtkTreeModel *model, gint column,
		const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	gchar	*buff;
	gboolean ret;

	gtk_tree_model_get(model, iter, column, &buff, -1);
	DEBUG_PRINT("key[%s] buff[%s]\n", key, buff);
	ret = g_strrstr(buff, key) ? FALSE : TRUE;
	g_free(buff);

	return ret;
}

void view_setting(GtkWidget *treeview, gint search_column)
{
	// 複数行選択可
	gtk_tree_selection_set_mode(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
			GTK_SELECTION_MULTIPLE);
	// マウス複数行選択
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(treeview), TRUE);
	// インクリメンタルサーチ
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview),
							search_column);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview),
			(GtkTreeViewSearchEqualFunc)inc_search, NULL, NULL);
}

/*---------------------------------------------------------------------------*/
static GtkContainer *create_domain_view(GtkWidget *paned,
		GtkWidget *treeview, void (*paned_pack)(),
		gboolean resize,
		GtkWidget *acl_window, gboolean *acl_detached)
{
	GtkWidget	*scrolledwin;
	GtkContainer	*container;

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(scrolledwin), GTK_SHADOW_IN);

	paned_pack(GTK_PANED(paned), scrolledwin, resize, TRUE);

	if (!resize) {
		view_setting(treeview, LIST_OPERAND);
	} else {
		view_setting(treeview, COLUMN_DOMAIN_NAME);
	}

	container = GTK_CONTAINER(scrolledwin);
	gtk_container_add(container, treeview);

	return container;
}
/*---------------------------------------------------------------------------*/
static void create_list_view(GtkWidget *box, GtkWidget *listview,
				gboolean multi_flag)
{
	GtkWidget	*scrolledwin;

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(scrolledwin), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(box), scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolledwin), listview);

	if (multi_flag) {
		// 複数行選択可
		gtk_tree_selection_set_mode(
			gtk_tree_view_get_selection(
				GTK_TREE_VIEW(listview)),
				GTK_SELECTION_MULTIPLE);
		// マウス複数行選択
		gtk_tree_view_set_rubber_banding(
					GTK_TREE_VIEW(listview), TRUE);
	}
	// インクリメンタルサーチ
	gtk_tree_view_set_search_column(
		GTK_TREE_VIEW(listview), LIST_OPERAND);
	gtk_tree_view_set_enable_search(
		GTK_TREE_VIEW(listview), TRUE);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(listview),
		(GtkTreeViewSearchEqualFunc)inc_search, NULL, NULL);
}
/*---------------------------------------------------------------------------*/
static void set_select_flag_domain(gpointer data,
					transition_t *transition)
{
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	gint			index;
	struct ccs_domain_policy *dp = transition->dp;


	model = gtk_tree_view_get_model(
				GTK_TREE_VIEW(transition->treeview));
	if (!model || !gtk_tree_model_get_iter(model, &iter, data)) {
		g_warning("ERROR: %s(%d)", __FILE__, __LINE__);
		return;
	}
	gtk_tree_model_get(model, &iter, COLUMN_INDEX, &index, -1);

	DEBUG_PRINT("index[%d]\n", index);
	/* deleted_domain or initializer_source */
	if (!(dp->list[index].is_dd) && !(dp->list[index].is_dis))
		dp->list_selected[index] = 1;
}

gint delete_domain(transition_t *transition,
			GtkTreeSelection *selection, gint count)
{
	GList		*list;
	char		*err_buff = NULL;
	int		result;


	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	g_list_foreach(list, (GFunc)set_select_flag_domain, transition);

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	result = delete_domain_policy(transition->dp, &err_buff);
	if (result) {
		g_warning("%s", err_buff);
		free(err_buff);
	}

	return result;
}
/*---------------------------------------------------------------------------*/
gint set_domain_profile(transition_t *transition,
			GtkTreeSelection *selection, guint profile)
{
	char		*profile_str;
	GList		*list;
	char		*err_buff = NULL;
	int		result;

	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (transition->task_flag)
		g_list_foreach(list, (GFunc)set_select_flag_process,
					&(transition->tsk));
	else
		g_list_foreach(list, (GFunc)set_select_flag_domain,
					transition);

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	profile_str = g_strdup_printf("%u", profile);
	result = set_profile(transition->dp, profile_str, &err_buff);
//	g_free(profile_str);	→ editpolicy.c
	if (result) {
		g_warning("%s", err_buff);
		free(err_buff);
	}

	return result;
}
/*---------------------------------------------------------------------------*/
gboolean disp_acl_line(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, generic_list_t *acl)
{
	GtkTreeSelection	*selection;
	gchar			*str_num;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(acl->listview));

	gtk_tree_model_get(model, iter, LIST_NUMBER, &str_num, -1);

	if (acl->list[atoi(str_num)].selected) {
#if 0
gchar *str_path = gtk_tree_path_to_string(path);
g_print("select[%d] path[%s]\n", atoi(str_num), str_path);
g_free(str_path);
#endif
		gtk_tree_selection_select_path(selection, path);
	} else {
		gtk_tree_selection_unselect_path(selection, path);
	}

	return FALSE;
}

gint select_list_line(generic_list_t *gen)
{
	GtkTreeSelection	*selection;
	GtkTreeIter		iter;
	GtkTreeModel		*model;
	GList			*list;
	gchar			*str_num;
	gint			index = -1;

	selection = gtk_tree_view_get_selection(
					GTK_TREE_VIEW(gen->listview));
	if (!selection ||
	    !gtk_tree_selection_count_selected_rows(selection))
		return index;

	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (!list)
		return index;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gen->listview));
	gtk_tree_model_get_iter(model, &iter, g_list_first(list)->data);
	gtk_tree_model_get(model, &iter, LIST_NUMBER, &str_num, -1);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	index = atoi(str_num);
	g_free(str_num);

	return index;
}
/*---------------------------------------------------------------------------*/
static void set_delete_flag_gen(gpointer data, generic_list_t *gen)
{
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	gchar			*str_num;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gen->listview));
	if (!model || !gtk_tree_model_get_iter(model, &iter, data)) {
		g_warning("ERROR: %s(%d)", __FILE__, __LINE__);
		return;
	}
	gtk_tree_model_get(model, &iter, LIST_NUMBER, &str_num, -1);

	DEBUG_PRINT("index[%d]\n", atoi(str_num));
	gen->list[atoi(str_num)].selected = 1;
	g_free(str_num);
}

gint delete_acl(transition_t *transition,
			GtkTreeSelection *selection, gint count)
{
	GList		*list;
	char		*err_buff = NULL;
	int		result;


	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	g_list_foreach(list, (GFunc)set_delete_flag_gen, &(transition->acl));
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	result = delete_acl_policy(transition->dp, &err_buff,
			transition->acl.list, transition->acl.count);
	if (result) {
		g_warning("%s", err_buff);
		free(err_buff);
	}

	return result;
}

gint delete_exp(transition_t *transition,
			GtkTreeSelection *selection, gint count)
{
	GList		*list;
	char		*err_buff = NULL;
	int		result;

	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	g_list_foreach(list, (GFunc)set_delete_flag_gen, &(transition->exp));
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	result = delete_exp_policy(transition->dp, &err_buff,
			transition->exp.list, transition->exp.count);
	if (result) {
		g_warning("%s", err_buff);
		free(err_buff);
	}

	return result;
}
/*---------------------------------------------------------------------------*/
static void create_tabs(GtkWidget *notebook, GtkWidget *box, gchar *str)
{
	GtkWidget	*label, *label_box, *menu_box;

	// create tab
	label_box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(label_box), label, FALSE, TRUE, 0);
	gtk_widget_show_all(label_box);

	// create context menu for tab
	menu_box = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(str);
	gtk_box_pack_start(GTK_BOX(menu_box), label, FALSE, TRUE, 0);
	gtk_widget_show_all(menu_box);

	gtk_notebook_append_page_menu(GTK_NOTEBOOK(notebook),
					box, label_box, menu_box);
}

gchar *disp_window_title(enum ccs_screen_type current_page)
{
	char	str_ip[32];
	gchar	*title = NULL;
	const char *dir = get_policy_dir();
	enum	mode_enum {e_off, e_net, e_on, e_end};
	const gchar *mode[e_end] = {_("offline"), _("nework"), _("online")};


	switch((int)current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_SCREEN_ACL_LIST :
		if (is_offline())
			title = g_strdup_printf("%s - %s%s%s", mode[e_off], dir,
			 CCS_DISK_POLICY_DIR, CCS_DISK_POLICY_DOMAIN_POLICY);
		else if (is_network())
			title = g_strdup_printf("%s(%s) - %s", mode[e_net],
					get_remote_ip(str_ip),
					CCS_PROC_POLICY_DOMAIN_POLICY);
		else
			title = g_strdup_printf("%s - %s", mode[e_on],
					CCS_PROC_POLICY_DOMAIN_POLICY);
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		if (is_offline())
			title = g_strdup_printf("%s - %s%s%s", mode[e_off], dir,
			 CCS_DISK_POLICY_DIR, CCS_DISK_POLICY_EXCEPTION_POLICY);
		else if (is_network())
			title = g_strdup_printf("%s(%s) - %s", mode[e_net],
					get_remote_ip(str_ip),
					CCS_PROC_POLICY_EXCEPTION_POLICY);
		else
			title = g_strdup_printf("%s - %s", mode[e_on],
					CCS_PROC_POLICY_EXCEPTION_POLICY);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		if (is_offline())
			title = g_strdup_printf("%s - %s%s%s", mode[e_off], dir,
			 CCS_DISK_POLICY_DIR, CCS_DISK_POLICY_PROFILE);
		else if (is_network())
			title = g_strdup_printf("%s(%s) - %s", mode[e_net],
					get_remote_ip(str_ip),
					CCS_PROC_POLICY_PROFILE);
		else
			title = g_strdup_printf("%s - %s", mode[e_on],
					CCS_PROC_POLICY_PROFILE);
		break;
	case CCS_SCREEN_STAT_LIST :
		if (is_network())
			title = g_strdup_printf("%s: %s(%s) - %s",
					_("Statistics"), mode[e_net],
					get_remote_ip(str_ip),
					CCS_PROC_POLICY_STAT);
		else
			title = g_strdup_printf("%s: %s - %s",
					_("Statistics"), mode[e_on],
					CCS_PROC_POLICY_STAT);
		break;
	case CCS_SCREEN_MANAGER_LIST :
		if (is_offline())
			title = g_strdup_printf("%s: %s - %s%s%s",
				_("Manager Policy"), mode[e_off], dir,
				CCS_DISK_POLICY_DIR, CCS_DISK_POLICY_MANAGER);
		else if (is_network())
			title = g_strdup_printf("%s: %s(%s) - %s",
				_("Manager Policy"), mode[e_net],
				get_remote_ip(str_ip),
				CCS_PROC_POLICY_MANAGER);
		else
			title = g_strdup_printf("%s: %s - %s",
				_("Manager Policy"), mode[e_on],
				CCS_PROC_POLICY_MANAGER);
		break;
	case CCS_MAXSCREEN :
		if (is_offline())
			title = g_strdup_printf("%s: %s - %s%s%s",
				_("Domain Policy Editor"), mode[e_off], dir,
			 CCS_DISK_POLICY_DIR, CCS_DISK_POLICY_DOMAIN_POLICY);
		else if (is_network())
			title = g_strdup_printf("%s: %s(%s) - %s",
					_("Domain Policy Editor"), mode[e_net],
					get_remote_ip(str_ip),
					CCS_PROC_POLICY_DOMAIN_POLICY);
		else
			title = g_strdup_printf("%s: %s - %s",
					_("Domain Policy Editor"), mode[e_on],
					CCS_PROC_POLICY_DOMAIN_POLICY);
		break;
	}

	return title;
}

static void control_acl_window(transition_t *tran)
{
	static gint		x, y, w, h;
	static gboolean	saved_flag = FALSE;

	if (tran->current_page == CCS_SCREEN_ACL_LIST) {
		gtk_window_move(GTK_WINDOW(tran->acl_window), x, y);
		gtk_window_set_default_size(
				GTK_WINDOW(tran->acl_window), w, h);
		saved_flag = FALSE;
		gtk_widget_show(tran->acl_window);
	} else {
		if (!saved_flag) {
			gtk_window_get_position(
				GTK_WINDOW(tran->acl_window), &x, &y);
			gtk_window_get_size(
				GTK_WINDOW(tran->acl_window), &w, &h);
			saved_flag = TRUE;
		}
		gtk_widget_hide(tran->acl_window);
	}
}

static void cb_switch_page(GtkWidget    *notebook,
				GtkNotebookPage *page,
				gint            page_num,
				transition_t    *tran)
{
	GtkTreeSelection	*selection_tree, *selection_list;
	gint			old_page_num;
	gchar			*title;

	old_page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	if (page_num == old_page_num)
		return;

	DEBUG_PRINT("In  Tab[%d]\n", page_num);
	switch(page_num) {
	case 0 :
		selection_tree = gtk_tree_view_get_selection(
					GTK_TREE_VIEW(tran->treeview));
		selection_list = gtk_tree_view_get_selection(
					GTK_TREE_VIEW(tran->acl.listview));

		if (tran->acl_detached) {
			if(selection_list &&
			    gtk_tree_selection_count_selected_rows(selection_list))
				tran->current_page = CCS_SCREEN_ACL_LIST;
			control_acl_window(tran);
			g_object_set(G_OBJECT(notebook), "can-focus", FALSE, NULL);
		} else {
			if (tran->task_flag ||
				(selection_tree &&
			    gtk_tree_selection_count_selected_rows(selection_tree)))
				tran->current_page = CCS_SCREEN_DOMAIN_LIST;
			else if(selection_list &&
			    gtk_tree_selection_count_selected_rows(selection_list))
				tran->current_page = CCS_SCREEN_ACL_LIST;
			else
				tran->current_page = CCS_MAXSCREEN;
			g_object_set(G_OBJECT(notebook), "can-focus", TRUE, NULL);
		}
		break;
	case 1 :
		tran->current_page = CCS_SCREEN_EXCEPTION_LIST;
		if (tran->acl_detached) {
			control_acl_window(tran);
		}
		g_object_set(G_OBJECT(notebook), "can-focus", FALSE, NULL);
		break;
	case 2 :
		tran->current_page = CCS_SCREEN_PROFILE_LIST;
		if (tran->acl_detached) {
			control_acl_window(tran);
		}
		g_object_set(G_OBJECT(notebook), "can-focus", FALSE, NULL);
		break;
	}

	title = disp_window_title(tran->current_page);
	gtk_window_set_title(GTK_WINDOW(tran->window), title);
	g_free(title);
//	disp_statusbar(tran, tran->current_page);
	set_sensitive(tran->actions, tran->task_flag,
					tran->current_page);

	refresh_transition(NULL, tran);
	DEBUG_PRINT("Out Tab[%d]\n", page_num);
}
/*---------------------------------------------------------------------------*/
static void cb_set_focus(GtkWindow *window,
				GtkWidget *view, transition_t *tran)
{
	if (view == tran->treeview) {
		DEBUG_PRINT("Focus changed!!<Domain>\n");
		tran->current_page = CCS_SCREEN_DOMAIN_LIST;
	} else if (view == tran->acl.listview) {
		DEBUG_PRINT("Focus changed!!<Acl>(%p)\n", view);
		tran->current_page = CCS_SCREEN_ACL_LIST;
	} else if (view == tran->exp.listview) {
		DEBUG_PRINT("Focus changed!!<Exception>\n");
		tran->current_page = CCS_SCREEN_EXCEPTION_LIST;
	} else if (view == tran->prf.listview) {
		DEBUG_PRINT("Focus changed!!<Profile>\n");
		tran->current_page = CCS_SCREEN_PROFILE_LIST;
	} else if (view == tran->tsk.treeview) {
		DEBUG_PRINT("Focus changed!!<Process>\n");
		tran->current_page = CCS_SCREEN_DOMAIN_LIST;
	} else if (G_IS_OBJECT(view)) {
		DEBUG_PRINT("Focus changed!![Other:%s(%p)]\n",
				g_type_name(G_OBJECT_TYPE(view)), view);
//		g_object_set(G_OBJECT(view), "can-focus", FALSE, NULL);
	} else {
		DEBUG_PRINT("Focus changed!![Not object(%p)]\n", view);
	}
}

static void cb_set_focus_acl(GtkWindow *window,
				GtkWidget *view, transition_t *tran)
{
	tran->current_page = CCS_SCREEN_ACL_LIST;

	if (view == tran->acl.listview) {
		DEBUG_PRINT("Focus changed!!<Acl>(%p)\n", view);
	} else if (G_IS_OBJECT(view)) {
		DEBUG_PRINT("Focus changed!![Other:%s(%p)]\n",
				g_type_name(G_OBJECT_TYPE(view)), view);
		g_object_set(G_OBJECT(view), "can-focus", FALSE, NULL);
	} else {
		DEBUG_PRINT("Focus changed!![Not object(%p)]\n", view);
	}
}

static void cb_show_acl(GtkWidget *view, transition_t *tran)
{
	DEBUG_PRINT("Show ACL!!(%p)==(%p)\n", tran->acl.listview, view);
}
/*---------------------------------------------------------------------------*/
int gpet_main(void)
{
	GtkWidget	*window;
	GtkWidget	*menubar, *toolbar = NULL;
	GtkWidget	*statusbar;
	gint		contextid;
	GtkWidget	*vbox;
	GtkWidget	*tab1, *tab2, *tab3;
	GtkWidget	*notebook;
	GtkWidget	*pane;
	GtkWidget	*domainbar;
	GtkWidget	*acl_window;
	GtkWidget	*treeview, *listview;
	GtkContainer	*container, *acl_container;
	gchar		*title;
	struct ccs_domain_policy dp = { NULL, 0, NULL };
	transition_t	transition;

	transition.task_flag = 0;
	if (get_domain_policy(&dp, &(transition.domain_count)))
		return 1;
	/*-----------------------------------------------*/

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	transition.window = window;
	gtk_window_set_title(GTK_WINDOW(window), _("gpet"));
	gtk_window_set_icon(GTK_WINDOW(window), get_png_file());
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	g_signal_connect(G_OBJECT(window), "destroy",
				G_CALLBACK(gtk_main_quit), NULL);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	// create menu bar & tool bar & popup menu
	menubar = create_menu(window, &transition, &toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	// create notebook
	notebook = gtk_notebook_new();
//	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	g_object_set(G_OBJECT(notebook), "can-focus", FALSE, NULL);

	// create status bar
	statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
	contextid = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "");
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), contextid, _("gpet"));


	tab1 = gtk_vbox_new(FALSE, 1);
	// create name bar for full domain name
	domainbar = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(tab1), domainbar, FALSE, FALSE, 0);
	gtk_editable_set_editable(GTK_EDITABLE(domainbar), FALSE);
	gtk_entry_set_has_frame(GTK_ENTRY(domainbar), FALSE);
	gtk_entry_set_text(GTK_ENTRY(domainbar), "");
//	gtk_widget_set_can_focus(domainbar, FALSE);	// 2.18
	g_object_set(G_OBJECT(domainbar), "can-focus", FALSE, NULL);

	pane = gtk_vpaned_new();
	g_object_set_data(G_OBJECT(window), "pane", pane);/* to save config */
	gtk_box_pack_start(GTK_BOX(tab1), pane, TRUE, TRUE, 0);

	// create domain transition view
	treeview = create_tree_model();
	container = create_domain_view(
			pane, treeview, gtk_paned_pack1, TRUE, NULL, NULL);
//a	add_tree_data(GTK_TREE_VIEW(treeview), &dp);
//a	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
		// ツリーインデント pixel(2.12)
//	gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(treeview), 0);
		// ツリー開くマーク(2.12)
//	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(treeview), TRUE);
	g_object_set(G_OBJECT(treeview), "enable-tree-lines", FALSE,
						NULL);
//	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeview),
//					GTK_TREE_VIEW_GRID_LINES_NONE);
	// create domain policy float window
	acl_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	title = disp_window_title(CCS_MAXSCREEN);
	gtk_window_set_title(GTK_WINDOW(acl_window), title);
	g_free(title);
	gtk_container_set_border_width(GTK_CONTAINER(acl_window), 0);
	gtk_window_set_position(GTK_WINDOW(acl_window),
						GTK_WIN_POS_CENTER);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(acl_window), TRUE);
	gtk_window_add_accel_group(GTK_WINDOW(acl_window),
		g_object_get_data(G_OBJECT(window), "AccelGroup"));
	gtk_widget_hide(acl_window);

	// create domain policy view
	listview = create_list_model(TRUE);
	transition.acl_detached = FALSE;
	acl_container = create_domain_view(pane, listview, gtk_paned_pack2,
			FALSE, acl_window, &transition.acl_detached);
	/* to save menu.c Detach_acl() */
	g_object_set_data(G_OBJECT(window), "acl_container", acl_container);

	// copy data pointer
	transition.domainbar = domainbar;
	transition.container = container;
	transition.treeview = treeview;
	transition.acl.listview = listview;
	transition.acl_window = acl_window;
	transition.statusbar = statusbar;
	transition.contextid = contextid;
	transition.dp = &dp;
	transition.acl.count = 0;
	transition.acl.list = NULL;
	transition.tsk.treeview = create_task_tree_model(&transition);
	transition.addentry = ADDENTRY_NON;

	// cursor move  domain window
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
				"changed", G_CALLBACK(cb_selection), &transition);
//	g_signal_connect(GTK_TREE_VIEW(treeview), "cursor-changed",
//			G_CALLBACK(cb_selection), &transition);
	// double click or enter key  domain window
	g_signal_connect(G_OBJECT(treeview), "row-activated",
			G_CALLBACK(cb_initialize_domain), NULL);
	// mouse click  domain window
	g_signal_connect(G_OBJECT(treeview), "button-press-event",
			G_CALLBACK(cb_select_domain), &transition);
	// mouse click  acl window
	g_signal_connect(G_OBJECT(listview), "button-press-event",
			G_CALLBACK(cb_select_acl), &transition);


	tab2 = gtk_vbox_new(FALSE, 1);
	// create exception policy view
	listview = create_list_model(TRUE);
	create_list_view(tab2, listview, TRUE);
	transition.exp.listview = listview;
	// mouse click  exception window
	g_signal_connect(G_OBJECT(listview), "button-press-event",
			 G_CALLBACK(cb_select_exp), &transition);

	tab3 = gtk_vbox_new(FALSE, 1);
	// create profile view
	listview = create_list_model(FALSE);
	create_list_view(tab3, listview, FALSE);
	transition.prf.listview = listview;
	transition.prf.count = 0;
	transition.prf.list = NULL;
	if (get_profile(&(transition.prf.list), &(transition.prf.count)))
		g_warning("Read error : profile");
	else
		add_list_data(&(transition.prf), FALSE);
	// mouse click  profile window
	g_signal_connect(G_OBJECT(listview), "button-press-event",
			 G_CALLBACK(cb_select_prf), &transition);

	// create tab
	create_tabs(notebook, tab1, _("Domain Transition"));
	create_tabs(notebook, tab2, _("Exception Policy"));
	create_tabs(notebook, tab3, _("Profile"));

	/* to save menu.c Process_state() */
	g_object_set_data(G_OBJECT(window), "notebook", notebook);
	g_object_set_data(G_OBJECT(window), "tab1", tab1);

	// tab change
	g_signal_connect(G_OBJECT(notebook), "switch_page",
				G_CALLBACK(cb_switch_page), &transition);
	read_config(&transition);

	/* set widget names for .gpetrc */
	gtk_widget_set_name(transition.window, "GpetMainWindow");
	gtk_widget_set_name(transition.domainbar, "GpetDomainbar");
	gtk_widget_set_name(transition.acl_window, "GpetAclWindow");

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	const gchar *homedir = g_getenv("HOME");
	gchar	*gpetrc;
	int	result;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_init(&argc, &argv);

	if (!homedir)
		homedir = g_get_home_dir ();
	gpetrc = g_build_path("/", homedir, ".gpetrc", NULL);
	gtk_rc_parse(gpetrc);
	g_free(gpetrc);

	result = ccs_main(argc, argv);

	return result;
}
/*---------------------------------------------------------------------------*/
