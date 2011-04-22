/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * search.c
 * Copyright (C) Yoshihiro Kusuno 2011 <yocto@users.sourceforge.jp>
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

typedef struct _search_t {
	transition_t	*tran;
	GtkWidget	*combo;
} search_t;

#define COMBO_LIST_LIMIT	10
static GList		*combolist = NULL;
static gchar		*S_entry = NULL;

enum screen_type_e {
	SCREEN_DOMAIN_LIST,
	SCREEN_PROCESS_LIST,
	SCREEN_ACL_LIST,
	SCREEN_EXCEPTION_LIST,
	SCREEN_PROFILE_LIST,
	SCREEN_MANAGER_LIST,
	SCREEN_STAT_LIST,
	MAXSCREEN
};
static	gint		locate_index[MAXSCREEN+1] = {0, };

static gint get_locate_index(transition_t *transition)
{
	enum screen_type_e	screen;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			screen = SCREEN_PROCESS_LIST;
		} else {
			screen = SCREEN_DOMAIN_LIST;
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		screen = SCREEN_ACL_LIST;
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		screen = SCREEN_EXCEPTION_LIST;
		break;
	case CCS_SCREEN_PROFILE_LIST :
		screen = SCREEN_PROFILE_LIST;
		break;
	case CCS_SCREEN_MANAGER_LIST :
	default :
		screen = MAXSCREEN;
		break;
	}

	return locate_index[screen];
}

static void put_locate_index(transition_t *transition, gint index)
{
	enum screen_type_e	screen;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			screen = SCREEN_PROCESS_LIST;
		} else {
			screen = SCREEN_DOMAIN_LIST;
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		screen = SCREEN_ACL_LIST;
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		screen = SCREEN_EXCEPTION_LIST;
		break;
	case CCS_SCREEN_PROFILE_LIST :
		screen = SCREEN_PROFILE_LIST;
		break;
	case CCS_SCREEN_MANAGER_LIST :
	default :
		screen = MAXSCREEN;
		break;
	}

	locate_index[screen] = index;
}

static gboolean search_pos_list(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, transition_t *transition)
{
	GtkWidget		*view = NULL;
	GtkTreeSelection	*selection;
	gint			index;
	gchar			*number = NULL;


	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			view = transition->tsk.treeview;
		} else {
			view = transition->treeview;
		}
		gtk_tree_model_get(model, iter, 0, &index, -1);
		break;
	case CCS_SCREEN_ACL_LIST :
		view = transition->acl.listview;
		gtk_tree_model_get(model, iter, 0, &number, -1);
		index = atoi(number);
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		view = transition->exp.listview;
		gtk_tree_model_get(model, iter, 0, &number, -1);
		index = atoi(number);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		view = transition->prf.listview;
		gtk_tree_model_get(model, iter, 0, &number, -1);
		index = atoi(number);
		break;
	}
	g_free(number);

	if (index == get_locate_index(transition)) {
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
		gtk_tree_selection_select_path(selection, path);
		view_cursor_set(view, gtk_tree_path_copy(path), NULL);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gint get_list_count(transition_t *transition)
{
	gint	count;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			count = transition->tsk.count;
		} else {
			count = transition->dp->list_len;
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		count = transition->acl.count;
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		count = transition->exp.count;
		break;
	case CCS_SCREEN_PROFILE_LIST :
		count = transition->prf.count;
		break;
	case CCS_SCREEN_MANAGER_LIST :
	default :
		count = -1;
		break;
	}
	return count;
}

static gint get_current_index(transition_t *transition)
{
	gint	index;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			index = get_current_process_index(&(transition->tsk));
		} else {
			index = get_current_domain_index(transition);
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		index = select_list_line(&(transition->acl));
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		index = select_list_line(&(transition->exp));
		break;
	case CCS_SCREEN_PROFILE_LIST :
		index = select_list_line(&(transition->prf));
		break;
	case CCS_SCREEN_MANAGER_LIST :
	default :
		index = 0;
		break;
	}
	return index;
}

static void search(transition_t *transition, gboolean forward)
{
	GtkWidget	*view = NULL;
	gint		index, count;
	gchar		*str_p = NULL;

	index = get_current_index(transition);
	count = get_list_count(transition);
	while (S_entry) {
		forward ? index++ : index--;
		if (index < 0 || count <= index)
			break;

		switch((int)transition->current_page) {
		case CCS_SCREEN_DOMAIN_LIST :
			if (transition->task_flag) {
				view = transition->tsk.treeview;
				str_p = transition->tsk.task[index].name;
			} else {
				view = transition->treeview;
				str_p = (gchar *)get_domain_last_name(index);
			}
			break;
		case CCS_SCREEN_ACL_LIST :
			view = transition->acl.listview;
			str_p = g_strdup_printf("%s %s",
					ccs_directives[transition->acl.list
						[index].directive].alias,
					transition->acl.list[index].operand);
			break;
		case CCS_SCREEN_EXCEPTION_LIST :
			view = transition->exp.listview;
			str_p = g_strdup_printf("%s %s",
					ccs_directives[transition->exp.list
						[index].directive].alias,
					transition->exp.list[index].operand);
			break;
		case CCS_SCREEN_PROFILE_LIST :
			view = transition->prf.listview;
			str_p = g_strdup_printf("%u-%s",
					  transition->prf.list[index].directive,
					  transition->prf.list[index].operand);
			break;
		case CCS_SCREEN_MANAGER_LIST :
		default :
			return;
			break;
		}

		if(g_strrstr(str_p, S_entry)) {
			GtkTreeModel	*model;
			put_locate_index(transition, index);
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
			gtk_tree_model_foreach(model,
			  (GtkTreeModelForeachFunc)search_pos_list, transition);
			break;
		}
	}

	if (transition->current_page != CCS_SCREEN_DOMAIN_LIST)
		g_free(str_p);

	gtk_action_set_sensitive(gtk_action_group_get_action(
				transition->actions, "SearchBack"), TRUE);
	gtk_action_set_sensitive(gtk_action_group_get_action(
				transition->actions, "SearchFoward"), TRUE);
}

/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void cb_combo_changed(GtkComboBox *combobox, gpointer nothing)
{
	GtkTreeModel	*model;
	GtkTreeIter	iter;
	gchar		*text;

	model = gtk_combo_box_get_model(combobox);
	if (gtk_combo_box_get_active_iter(combobox, &iter)) {
		gtk_tree_model_get(model, &iter, 0, &text, -1);
		g_free(S_entry);
		S_entry = text;
//		g_print("cb_combo_changed[%s]\n", text);
	}
}

static void cb_combo_entry_activate(GtkEntry *entry, search_t *srch)
{
	GtkComboBox	*combobox;
	gchar		*new_text;
	GList		*list;
	gboolean	exist_flag = FALSE;

	combobox = GTK_COMBO_BOX(srch->combo);
	new_text = gtk_combo_box_get_active_text(combobox);
	if(!new_text || strcmp(new_text, "") == 0)
		return;

	g_free(S_entry);
	S_entry = g_strdup(new_text);
//	g_print("cb_combo_entry_activate[%s]\n", new_text);

	for (list = combolist; list; list = g_list_next(list)) {
		if (strcmp(new_text, (gchar *)list->data) == 0) {
			exist_flag = TRUE;
			break;
		}
	}

	if (!exist_flag) {
		guint len;
		combolist = insert_item(combobox, combolist, new_text, 0);
		len = g_list_length(combolist);
		if (len > COMBO_LIST_LIMIT) {
			combolist = remove_item(combobox, combolist, len - 1);
		}
	}

	if (entry)
		search(srch->tran, TRUE);
}

static void cb_btn_prev_clicked(GtkButton *widget , search_t *srch)
{
	cb_combo_entry_activate(NULL, srch);
	search(srch->tran, FALSE);
}

static void cb_btn_next_clicked(GtkButton *widget , search_t *srch)
{
	cb_combo_entry_activate(NULL, srch);
	search(srch->tran, TRUE);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
GList *insert_item(GtkComboBox	*combobox,
			GList *cmblist, const gchar *item_label, gint index)
{
	gtk_combo_box_insert_text(combobox, index, g_strdup(item_label));
	return g_list_insert(cmblist, g_strdup(item_label), index);
}

GList *remove_item(GtkComboBox *combobox, GList *cmblist, gint index)
{
	gtk_combo_box_remove_text(combobox, index);
	g_free(g_list_nth_data(cmblist, index));
	return g_list_delete_link(cmblist, g_list_nth(cmblist, index));
}

static gchar *search_title(transition_t *transition)
{
	gchar		*title;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		if (transition->task_flag) {
			title = _("Process Search for :");
		} else {
			title = _("Domain Search for :");
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		title = _("ACL Search for :");
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		title = _("Exception Search for :");
		break;
	case CCS_SCREEN_PROFILE_LIST :
		title = _("Profile Search for :");
		break;
	case CCS_SCREEN_MANAGER_LIST :
	default :
		title = _("Search for :");
		break;
	}
	return title;
}

void search_input(GtkAction *action, transition_t *transition)
{
	GtkWidget		*dialog, *parent;
	GtkWidget		*hbox, *vbox_l, *vbox_r;
	GtkWidget		*label;
	GtkWidget		*entry;
	GtkWidget		*btn_next, *btn_prev;
	GList			*list;
	search_t		srch;
	gint			response;

	parent = (transition->acl_detached &&
		(int)transition->current_page == CCS_SCREEN_ACL_LIST) ?
			transition->acl_window : transition->window;

	dialog = gtk_dialog_new_with_buttons(_("Find"),
			GTK_WINDOW(parent),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
						hbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

	vbox_l = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_l, TRUE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), search_title(transition));
	gtk_box_pack_start(GTK_BOX(vbox_l), label, FALSE, FALSE, 0);

	entry = gtk_combo_box_entry_new_text();
	srch.tran = transition;
	srch.combo = entry;
	g_signal_connect(G_OBJECT(entry) , "changed",
			G_CALLBACK(cb_combo_changed), NULL);
	g_signal_connect(G_OBJECT(GTK_BIN(entry)->child) , "activate" ,
			G_CALLBACK(cb_combo_entry_activate), &srch);
	gtk_box_pack_start(GTK_BOX(vbox_l), entry, FALSE, FALSE, 0);

	for (list = combolist; list; list = g_list_next(list)) {
		gtk_combo_box_append_text(
			GTK_COMBO_BOX(entry), (gchar *)list->data);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(entry), 0);

	vbox_r = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_r, FALSE, FALSE, 0);

	btn_prev = gtk_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_box_pack_start(GTK_BOX(vbox_r), btn_prev, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(btn_prev) , "clicked" ,
			G_CALLBACK (cb_btn_prev_clicked) , &srch);

	btn_next = gtk_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_box_pack_start(GTK_BOX(vbox_r), btn_next, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(btn_next) , "clicked" ,
			G_CALLBACK (cb_btn_next_clicked) , &srch);

	gtk_widget_set_size_request(dialog, 400, -1);
	put_locate_index(transition, get_current_index(transition));
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void search_back(GtkAction *action, transition_t *transition)
{
	search(transition, FALSE);
}

void search_forward(GtkAction *action, transition_t *transition)
{
	search(transition, TRUE);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
