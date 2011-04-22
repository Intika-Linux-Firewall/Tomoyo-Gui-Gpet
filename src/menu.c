/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * menu.c
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gpet.h"

static void terminate(GtkAction *action, transition_t *transition);
static void copy_line(GtkAction *action, transition_t *transition);
static void optimize_acl(GtkAction *action, transition_t *transition);
static void delete_transition(GtkAction *action, transition_t *transition);
static void insert_history_buffer(GtkWidget *view, gchar *entry);
static void append_transition(GtkAction *action, transition_t *transition);
static void set_transition(GtkAction *action, transition_t *transition);
static void manager_transition(GtkAction *action, transition_t *transition);
static void memory_transition(GtkAction *action, transition_t *transition);
static void show_about_dialog(void);
static void Process_state(GtkAction *action, transition_t *transition);
static void Detach_acl(GtkAction *action, transition_t *transition);

static GtkActionEntry entries[] = {
  {"FileMenu", NULL, N_("_File")},
  {"EditMenu", NULL, N_("_Edit")},
  {"ViewMenu", NULL, N_("_View")},
  {"HelpMenu", NULL, N_("_Help")},

  {"Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	N_("Quit a program"), G_CALLBACK(terminate)},

  {"Edit", GTK_STOCK_EDIT, N_("S_et"), "<control>E",
	N_("Set or Edit the selected line"), G_CALLBACK(set_transition)},
  {"Add", GTK_STOCK_ADD, N_("_Add"), "<control>I",
	N_("Append line"), G_CALLBACK(append_transition)},
  {"Delete", GTK_STOCK_DELETE, N_("_Delete"), "<control>D",
	N_("Delete the selected line"), G_CALLBACK(delete_transition)},
  {"Copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C",
	N_("Copy an entry at the cursor position to history buffer"),
	G_CALLBACK(copy_line)},
  {"OptimizationSupport", GTK_STOCK_CONVERT,
	N_("_OptimizationSupport"), "<control>O",
	N_("Extraction of redundant ACL entries"), G_CALLBACK(optimize_acl)},

  {"Search", GTK_STOCK_FIND, N_("_Search"), "<control>F",
	N_("Search for text"), G_CALLBACK(search_input)},
  {"SearchBack", GTK_STOCK_GO_BACK, N_("Search_Backwards"), "<control><shift>G",
	N_("Search backwards for the same text"), G_CALLBACK(search_back)},
  {"SearchFoward", GTK_STOCK_GO_FORWARD, N_("SearchFor_wards"), "<control>G",
	N_("Search forwards for the same text"), G_CALLBACK(search_forward)},

  {"Refresh", GTK_STOCK_REFRESH, N_("_Refresh"), "<control>R",
	N_("Refresh to the latest information"), G_CALLBACK(refresh_transition)},
  {"Manager", GTK_STOCK_DND, N_("_Manager"), "<control>M",
	N_("Manager Profile Editor"), G_CALLBACK(manager_transition)},
  {"Memory", GTK_STOCK_DND, N_("_Statistics"), "<control>S",
	N_("Statistics"), G_CALLBACK(memory_transition)},

  {"About", GTK_STOCK_ABOUT, N_("_About"), "<alt>A",
	N_("About a program"), show_about_dialog}
};
static guint n_entries = G_N_ELEMENTS(entries);

static GtkToggleActionEntry toggle_entries[] = {
  { "Process", GTK_STOCK_MEDIA_RECORD, N_("Process"), "<control>at",
	N_("Process State Viewer"), G_CALLBACK(Process_state), FALSE},
  { "ACL", "", N_("Detach ACL"), "",
	N_("Detach ACL window"), G_CALLBACK(Detach_acl), FALSE},
};
static guint n_toggle_entries = G_N_ELEMENTS(toggle_entries);

static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Quit'/>"
"    </menu>"

"    <menu action='EditMenu'>"
"      <menuitem action='Edit'/>"
"      <menuitem action='Add'/>"
"      <menuitem action='Delete'/>"
"      <separator/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='OptimizationSupport'/>"
"      <separator/>"
"      <menuitem action='Search'/>"
"      <menuitem action='SearchBack'/>"
"      <menuitem action='SearchFoward'/>"
"    </menu>"

"    <menu action='ViewMenu'>"
"      <menuitem action='Refresh'/>"
"      <separator/>"
"      <menuitem action='Manager'/>"
"      <menuitem action='Memory'/>"
"      <separator/>"
"      <menuitem action='Process'/>"
"      <separator/>"
"      <menuitem action='ACL'/>"
"    </menu>"

"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"

"  <toolbar name='ToolBar'>"
"    <toolitem action='Quit'/>"
"    <separator/>"
"    <toolitem action='Edit'/>"
"    <toolitem action='Add'/>"
"    <toolitem action='Delete'/>"
"    <separator/>"
"    <toolitem action='Copy'/>"
"    <toolitem action='OptimizationSupport'/>"
"    <separator/>"
"    <toolitem action='Search'/>"
"    <toolitem action='SearchBack'/>"
"    <toolitem action='SearchFoward'/>"
"    <separator/>"
"    <toolitem action='Refresh'/>"
"    <separator/>"
"    <toolitem action='Process'/>"
"    <separator/>"
"    <toolitem action='About'/>"
"  </toolbar>"

"  <popup name='PopUp'>"
"      <menuitem action='Edit'/>"
"      <menuitem action='Add'/>"
"      <menuitem action='Delete'/>"
"      <separator/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='OptimizationSupport'/>"
"  </popup>"
"</ui>";

GtkWidget *create_menu(GtkWidget *parent, transition_t *transition,
				GtkWidget **toolbar)
{
	GtkUIManager		*ui;
	GtkActionGroup		*actions;
	GtkAccelGroup		*accel_group;
	GError			*error = NULL;
	GtkWidget		*popup;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_set_translation_domain(actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(actions,
			entries, n_entries, transition);
	gtk_action_group_add_toggle_actions(actions,
			toggle_entries, n_toggle_entries, transition);

	ui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui, actions, 0);

	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "Search"), TRUE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "SearchBack"), FALSE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "SearchFoward"), FALSE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "Edit"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(
				actions, "OptimizationSupport"), FALSE);

	if (is_offline()) {
		gtk_action_set_sensitive(
			gtk_action_group_get_action(actions, "Process"), FALSE);
		gtk_action_set_sensitive(
			gtk_action_group_get_action(actions, "Memory"), FALSE);
	}

	transition->actions = actions;

//	gtk_ui_manager_set_add_tearoffs(ui, TRUE);
	accel_group = gtk_ui_manager_get_accel_group(ui);
	gtk_window_add_accel_group(GTK_WINDOW(parent), accel_group);
	if (!gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, NULL)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	*toolbar = gtk_ui_manager_get_widget(ui, "/ToolBar");
	gtk_toolbar_set_style(GTK_TOOLBAR(*toolbar), GTK_TOOLBAR_ICONS);

	/* to save gpet.c popup_menu() */
	popup = gtk_ui_manager_get_widget(ui, "/PopUp");
	g_object_set_data(G_OBJECT(transition->window), "popup", popup);

	/* to save gpet.c gpet_main() */
	g_object_set_data(G_OBJECT(transition->window),
						"AccelGroup", accel_group);

	return gtk_ui_manager_get_widget(ui, "/MenuBar");
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
void disp_statusbar(transition_t *transition, int scr)
{
	gchar	*status_str = NULL;

	switch (scr) {
	case CCS_SCREEN_EXCEPTION_LIST :
		status_str = g_strdup_printf("Entry[%d]",
			transition->exp.count);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		status_str = g_strdup_printf("Entry[%d]",
			transition->prf.count);
		break;
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_SCREEN_ACL_LIST :
		if (transition->task_flag)
			status_str = g_strdup_printf("Process[%d] Policy[%d]",
				transition->tsk.count, transition->acl.count);
		else
			status_str = g_strdup_printf("Domain[%d] Policy[%d]",
				transition->domain_count, transition->acl.count);
		break;
	case CCS_MAXSCREEN :
		status_str = g_strdup_printf("Domain[%d]",
			transition->domain_count);
		break;
	default :
		g_warning("BUG: screen [%d]  file(%s) line(%d)",
				scr, __FILE__, __LINE__);
		break;
	}
	gtk_statusbar_pop(GTK_STATUSBAR(transition->statusbar),
				transition->contextid);
	gtk_statusbar_push(GTK_STATUSBAR(transition->statusbar),
				transition->contextid, status_str);
	g_free(status_str);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void terminate(GtkAction *action, transition_t *transition)
{
	write_config(transition);
	g_object_unref(transition->actions);
	gtk_main_quit();
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static gulong handler_id = 0UL;

static gboolean cb_destroy_acl(GtkWidget *widget, GdkEvent *event,
					transition_t *transition)
{
	GtkWidget	*acl_container;
	GtkPaned	*paned;

	if (handler_id == 0UL)
		return FALSE;

	/* get gpet.c main()*/
	acl_container = g_object_get_data(G_OBJECT(transition->window), "acl_container");
	paned = g_object_get_data(G_OBJECT(transition->window), "pane");

	g_signal_handler_disconnect(
			G_OBJECT(transition->acl_window), handler_id);
	handler_id = 0UL;

	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
		gtk_action_group_get_action(transition->actions, "ACL")), FALSE);
	transition->acl_detached = FALSE;

	gtk_widget_reparent(GTK_WIDGET(transition->container),
					GTK_WIDGET(paned));

	gtk_widget_reparent(GTK_WIDGET(acl_container),
					GTK_WIDGET(paned));

	transition->current_page = CCS_SCREEN_DOMAIN_LIST;
	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	refresh_transition(NULL, transition);

	return gtk_widget_hide_on_delete(transition->acl_window);
}

static void Detach_acl(GtkAction *action, transition_t *transition)
{
	GtkWidget	*acl_container;
	GtkPaned	*paned;
	/* get gpet.c main()*/
	acl_container = g_object_get_data(G_OBJECT(transition->window), "acl_container");
	paned = g_object_get_data(G_OBJECT(transition->window), "pane");

	if (transition->acl_detached) {
		cb_destroy_acl(NULL, NULL, transition);
	} else {
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
			gtk_action_group_get_action(transition->actions, "ACL")), TRUE);
		transition->acl_detached = TRUE;
		handler_id = g_signal_connect(
			G_OBJECT(transition->acl_window), "delete_event",
			G_CALLBACK(cb_destroy_acl), transition);
		gtk_widget_reparent(GTK_WIDGET(acl_container),
					transition->acl_window);
		gtk_widget_show_all(transition->acl_window);

		gtk_widget_reparent(GTK_WIDGET(transition->container),
					GTK_WIDGET(paned));
		transition->current_page = CCS_SCREEN_ACL_LIST;
		set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
		refresh_transition(NULL, transition);
	}
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void Process_state(GtkAction *action, transition_t *transition)
{
	GtkWidget	*view;
	GtkWidget	*notebook;
	GtkWidget	*tab1;
	gchar		*label_str;

	view = transition->task_flag ?
			transition->tsk.treeview : transition->treeview;
	gtk_container_remove(transition->container, view);
	g_object_ref(view);

	transition->task_flag = ~transition->task_flag & 1;

	if (transition->task_flag) {
		label_str = _("Process State");
		view = transition->tsk.treeview;
	} else {
		label_str = _("Domain Transition");
		view = transition->treeview;
	}

	/* from gpet.c:gpet_main()*/
	notebook = g_object_get_data(
				G_OBJECT(transition->window), "notebook");
	tab1 = g_object_get_data(
				G_OBJECT(transition->window), "tab1");
	gtk_notebook_set_tab_label_text(
				GTK_NOTEBOOK(notebook), tab1, label_str);
	gtk_notebook_set_menu_label_text(
				GTK_NOTEBOOK(notebook), tab1, label_str);

	gtk_container_add(transition->container, view);

	transition->current_page = CCS_SCREEN_DOMAIN_LIST;
	set_sensitive(transition->actions, transition->task_flag,
						transition->current_page);
	if (transition->acl_detached)
		g_object_set(G_OBJECT(notebook), "can-focus", FALSE, NULL);
	else
		g_object_set(G_OBJECT(notebook), "can-focus", TRUE, NULL);
	refresh_transition(action, transition);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
void view_cursor_set(GtkWidget *view,
			GtkTreePath *path, GtkTreeViewColumn *column)
{
	if (!path) {
		path = gtk_tree_path_new_from_indices(0, -1);
	}

	{
		gchar *path_str = gtk_tree_path_to_string(path);
		DEBUG_PRINT("<%s> TreePath[%s]\n",
			g_type_name(G_OBJECT_TYPE(view)), path_str);
		g_free(path_str);
	}
	gtk_tree_view_set_cursor(
			GTK_TREE_VIEW(view), path, column, FALSE);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(view), path, NULL,
						TRUE, 0.5, 0.0);	// ksn
	gtk_tree_path_free(path);
}

void refresh_transition(GtkAction *action, transition_t *transition)
{
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;
	GtkWidget		*view = NULL;

	DEBUG_PRINT("In  Refresh Page[%d]\n", (int)transition->current_page);
	switch((int)transition->current_page) {
	case CCS_SCREEN_EXCEPTION_LIST :
		view = transition->exp.listview;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(view),
							&path, &column);
		if (get_exception_policy(
		    &(transition->exp.list), &(transition->exp.count)))
			break;
		add_list_data(&(transition->exp), TRUE);
		set_position_addentry(transition, &path);
		disp_statusbar(transition, CCS_SCREEN_EXCEPTION_LIST);
		view_cursor_set(view, path, column);
		gtk_widget_grab_focus(view);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		view = transition->prf.listview;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(view),
							&path, &column);
		if (get_profile(
		    &(transition->prf.list), &(transition->prf.count)))
			break;
		add_list_data(&(transition->prf), FALSE);
		set_position_addentry(transition, &path);
		disp_statusbar(transition, CCS_SCREEN_PROFILE_LIST);
		view_cursor_set(view, path, column);
		gtk_widget_grab_focus(view);
		break;
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_MAXSCREEN :
		view = transition->task_flag ?
			transition->tsk.treeview : transition->treeview;
		gtk_tree_view_get_cursor(
				GTK_TREE_VIEW(view), &path, &column);

		gtk_widget_hide(view);
		if (transition->task_flag) {
			if (get_task_list(&(transition->tsk.task),
						&(transition->tsk.count)))
				break;
			add_task_tree_data(GTK_TREE_VIEW(view), &(transition->tsk));
			gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
		} else {
			if (get_domain_policy(
			    transition->dp, &(transition->domain_count)))
				break;
			add_tree_data(GTK_TREE_VIEW(view), transition->dp);
			gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
			set_position_addentry(transition, &path);
		}

		view_cursor_set(view, path, column);
		gtk_widget_show(view);
#if 0
		if (transition->acl_detached) {	// get focus
			gtk_widget_grab_focus(transition->acl.listview);
			DEBUG_PRINT("☆Transition[%d] Acl[%d]\n",
				gtk_window_has_toplevel_focus(GTK_WINDOW(transition->window)),
				gtk_window_has_toplevel_focus(GTK_WINDOW(transition->acl_window)));
		} else {
			gtk_widget_grab_focus(view);
		}
#endif
		gtk_widget_grab_focus(view);
		break;
	case CCS_SCREEN_ACL_LIST :
		view = transition->task_flag ?
			transition->tsk.treeview : transition->treeview;
		gtk_tree_view_get_cursor(
				GTK_TREE_VIEW(view), &path, &column);
		view_cursor_set(view, path, column);
		gtk_widget_grab_focus(transition->acl.listview);
		break;
	}

	if (transition->acl_detached) {
		DEBUG_PRINT("★Transition[%d] Acl[%d]\n",
		gtk_window_has_toplevel_focus(GTK_WINDOW(transition->window)),
		gtk_window_has_toplevel_focus(GTK_WINDOW(transition->acl_window)));
	}
	DEBUG_PRINT("Out  Refresh Page[%d]\n", (int)transition->current_page);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void copy_line(GtkAction *action, transition_t *transition)
{
	gint		index;
	GtkWidget	*view;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		index = get_current_domain_index(transition);
		view = transition->treeview;
		if (index >= 0)
			insert_history_buffer(view, g_strdup(
				ccs_domain_name(transition->dp, index)));
		break;
	case CCS_SCREEN_ACL_LIST :
		view = transition->acl.listview;
		insert_history_buffer(view, get_alias_and_operand(view));
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		view = transition->exp.listview;
		insert_history_buffer(view, get_alias_and_operand(view));
		break;
	default :
		break;
	}
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void optimize_acl(GtkAction *action, transition_t *transition)
{
	GtkTreeSelection	*selection;
	GtkTreeModel		*model;
	gint			count, current;

	if ((int)transition->current_page != CCS_SCREEN_ACL_LIST)
		return;

	selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->acl.listview));
	if (!selection || !(count =
	    gtk_tree_selection_count_selected_rows(selection)))
		return;
	DEBUG_PRINT("count[%d]\n", count);

	// current位置を取得：
	current = select_list_line(&(transition->acl));
	if (current < 0)
		return;

	get_optimize_acl_list(current,
			&(transition->acl.list), transition->acl.count);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(transition->acl.listview));
	gtk_tree_model_foreach(model,
		(GtkTreeModelForeachFunc)disp_acl_line, &(transition->acl));

#if 0
	gint			index;
	GtkTreePath		*start_path, *end_path;
	gint			start_index, end_index;
	// 表示処理
	start_path = end_path = gtk_tree_path_new_from_indices(current, -1);
	gtk_tree_selection_unselect_range(selection, start_path, end_path);

	for (index = 0; index < transition->acl.count; index++) {
		if (transition->acl.list[index].selected)
			break;
	}
	end_path = NULL;
	start_path = gtk_tree_path_new_from_indices(index, -1);
	start_index = end_index = index;
	count = -1;
	for ( ; index < transition->acl.count; index++) {
		count++;
		// 対象データ：ccs_gacl_list[index].selected == 1;
		if (!transition->acl.list[index].selected)
			continue;

		//パス取得：
		if (index == (start_index + count)) {
			end_path = gtk_tree_path_new_from_indices(index, -1);
			end_index = index;
		} else {
			if (!end_path)
				end_path = start_path;

			// カーソルセット：
g_print("start[%d] end[%d]\n", start_index, end_index);
			gtk_tree_selection_select_range(
					selection, start_path, end_path);

			start_path = gtk_tree_path_new_from_indices(index, -1);
			start_index = index;
			count = 0;
			end_path = NULL;
		}
	}

	if (end_path) {
g_print("start[%d] end[%d]\n", start_index, end_index);
		gtk_tree_selection_select_range(selection, start_path, end_path);
	}
#endif
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static gint message_dialog(GtkWidget *parent, gchar *message)
{
	GtkWidget	*dialog;
	gint		result;

	dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(parent) ,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), message);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return result;
}

static void delete_transition(GtkAction *action, transition_t *transition)
{
	GtkTreeSelection	*selection;
	gint			count;
	GtkWidget		*parent;
	gchar			*message = NULL;
	gint			ret = 1;

	DEBUG_PRINT("delete : current page[%d]\n", transition->current_page);

	switch((int)transition->current_page) {
	case CCS_SCREEN_EXCEPTION_LIST :
		selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->exp.listview));
		if (!selection || !(count =
		    gtk_tree_selection_count_selected_rows(selection)))
			break;
		DEBUG_PRINT("count[%d]\n", count);
		message = count > 1 ?
		 g_strdup_printf(_(
			"<span foreground='red' size='x-large'>"
			"<b>Delete</b> the %d selected exception policies?</span>"), count) :
		 g_strdup_printf(_(
			"<span foreground='red' size='x-large'>"
			"<b>Delete</b> the selected exception policy?</span>"));
		if (GTK_RESPONSE_YES ==
		    message_dialog(transition->window, message)) {
			ret = delete_exp(transition, selection, count);
		}
		break;
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_MAXSCREEN :
		selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->treeview));
		if (!selection || !(count =
		    gtk_tree_selection_count_selected_rows(selection)))
			break;
		DEBUG_PRINT("count[%d]\n", count);
		message = count > 1 ?
		 g_strdup_printf(_(
			"<span foreground='red' size='x-large'>"
			"<b>Delete</b> the %d selected domains?</span>"), count) :
		 g_strdup_printf(_(
			"<span foreground='red' size='x-large'>"
			"<b>Delete</b> the selected domain?</span>"));
		if (GTK_RESPONSE_YES ==
		    message_dialog(transition->window, message)) {
			ret = delete_domain(transition, selection, count);
		}
		break;
	case CCS_SCREEN_ACL_LIST :
		DEBUG_PRINT("Delete ACL\n");
		selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->acl.listview));
		if (!selection || !(count =
		    gtk_tree_selection_count_selected_rows(selection)))
			break;
		DEBUG_PRINT("count[%d]\n", count);
		parent = transition->acl_detached ?
			transition->acl_window : transition->window;
		message = count > 1 ?
		 g_strdup_printf(_(
			"<span foreground='blue' size='x-large'>"
			"<b>Delete</b> the %d selected policies?</span>"), count) :
		 g_strdup_printf(_(
			"<span foreground='blue' size='x-large'>"
			"<b>Delete</b> the selected policy?</span>"));
  		if (GTK_RESPONSE_YES ==
		    message_dialog(parent, message)) {
			ret = delete_acl(transition, selection, count);
		}
		break;
	case CCS_SCREEN_MANAGER_LIST :
		DEBUG_PRINT("Delete manager\n");
		break;
	}

	g_free(message);

	if (!ret)
		refresh_transition(NULL, transition);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
#define COMBO_LIST_LIMIT	20
static GList	*combolist = NULL;
static gchar	*Last_entry_string = NULL;

gchar *get_combo_entry_last(void)
{
	return Last_entry_string;
}

static void insert_history_buffer(GtkWidget *view, gchar *entry)
{
	GtkClipboard	*clipboard;
	GList		*list;
	guint		len;

	if (!entry)
		return;

	/* to system clipboard */
	clipboard = gtk_widget_get_clipboard(view, GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, entry, -1);

	for (list = combolist; list; list = g_list_next(list)) {
		if (strcmp(entry, (gchar *)list->data) == 0)
			return;
	}

	combolist = g_list_insert(combolist, entry, 0);
	len = g_list_length(combolist);
	if (len > COMBO_LIST_LIMIT) {
		g_free(g_list_nth_data(combolist, len - 1));
		combolist = g_list_delete_link(combolist,
					g_list_nth(combolist, len - 1));
	}
}

static gint combo_entry_activate(GtkWidget *combo, gchar **new_text)
{
	GtkComboBox	*combobox;
	GList		*list;
	gboolean	exist_flag = FALSE;

	combobox = GTK_COMBO_BOX(combo);
	(*new_text) = gtk_combo_box_get_active_text(combobox);
	if(!(*new_text) || strcmp(*new_text, "") == 0)
		return 1;

	g_free(Last_entry_string);
	Last_entry_string = g_strdup(*new_text);

	for (list = combolist; list; list = g_list_next(list)) {
		if (strcmp(*new_text, (gchar *)list->data) == 0) {
			exist_flag = TRUE;
			break;
		}
	}

	if (!exist_flag) {
		guint len;
		combolist = insert_item(combobox, combolist, *new_text, 0);
		len = g_list_length(combolist);
		if (len > COMBO_LIST_LIMIT) {
			combolist = remove_item(combobox, combolist, len - 1);
		}
		DEBUG_PRINT("Append '%s'.\n", *new_text);
	}

	return exist_flag ? 1 : 0;
}

static gint append_dialog(transition_t *transition,
				gchar *title, gchar **input)
{
	GtkWidget		*dialog, *parent;
	GtkWidget		*combo;
	GList			*list;
	gint			response, result = 1;

	parent = (transition->acl_detached &&
		(int)transition->current_page == CCS_SCREEN_ACL_LIST) ?
			transition->acl_window : transition->window;

	dialog = gtk_dialog_new_with_buttons(title,
			GTK_WINDOW(parent),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			NULL);

	combo = gtk_combo_box_entry_new_text();
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	for (list = combolist; list; list = g_list_next(list)) {
		gtk_combo_box_append_text(
			GTK_COMBO_BOX(combo), (gchar *)list->data);
	}

	gtk_container_add(
		GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), combo);
	gtk_widget_set_size_request(dialog, 600, -1);
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY) {
		combo_entry_activate(combo, input);
		result = 0;
	}
	gtk_widget_destroy(dialog);

	return result;
}

static void append_transition(GtkAction *action, transition_t *transition)
{
	gchar		*input = NULL;
	gint		index, result = 1;
	enum addentry_type	type = ADDENTRY_NON;
	char		*err_buff = NULL;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		DEBUG_PRINT("append domain\n");
		result = append_dialog(transition,
				 _("Add Domain"), &input);
		if (!result)
			result = add_domain(input, &err_buff);
		type = ADDENTRY_DOMAIN_LIST;
		break;
	case CCS_SCREEN_ACL_LIST :
		DEBUG_PRINT("append acl\n");
		result = append_dialog(transition, _("Add Acl"), &input);
		if (!result) {
			index = get_current_domain_index(transition);
			if (index >= 0)
				result = add_acl_list(transition->dp,
						index, input, &err_buff);
		}
		type = ADDENTRY_ACL_LIST;
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		DEBUG_PRINT("append exception\n");
		result = append_dialog(transition,
				 _("Add Exception"), &input);
		if (!result)
			result = add_exception_policy(input, &err_buff);
		type = ADDENTRY_EXCEPTION_LIST;
		break;
	case CCS_SCREEN_PROFILE_LIST :
		DEBUG_PRINT("append profile\n");
		result = append_dialog(transition,
				 _("Add Profile (0 - 255)"), &input);
		if (!result)
			result = add_profile(input, &err_buff);
		type = ADDENTRY_PROFILE_LIST;
		break;
	default :
		DEBUG_PRINT("append ???\n");
		break;
	}

	if (result) {
		if(err_buff) {
			g_warning("%s", err_buff);
			free(err_buff);
		}
		transition->addentry = ADDENTRY_NON;
	} else {
		transition->addentry = type;
		refresh_transition(NULL, transition);
	}
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
enum list_column_pos {
	LIST_PROFILE,		// display
	PROFILE_NO,		// hidden
	N_COLUMNS_LIST
};
static GtkWidget *create_list_profile(void)
{
	GtkWidget		*treeview;
	GtkListStore		*liststore;
	GtkCellRenderer	*renderer;
	GtkTreeViewColumn	*column;

	liststore = gtk_list_store_new(N_COLUMNS_LIST,
					G_TYPE_STRING, G_TYPE_UINT);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	g_object_unref(liststore);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	column = gtk_tree_view_column_new_with_attributes("operand",
				renderer, "text", LIST_PROFILE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	// ヘッダ表示
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

	return treeview;
}

static void add_list_profile(GtkWidget *listview,
				 generic_list_t *generic)
{
	GtkListStore	*store;
	GtkTreeIter	iter;
	int		i;
	gchar		*profile;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(
					GTK_TREE_VIEW(listview)));
	gtk_list_store_clear(store);

	DEBUG_PRINT("Count[%d]\n", generic->count);
	for(i = 0; i < generic->count; i++){
		if (generic->list[i].directive > 255 ||
			!strstr(generic->list[i].operand, "COMMENT="))
			continue;

		DEBUG_PRINT("[%3d]:%3u-%s\n", i,
			generic->list[i].directive, generic->list[i].operand);

		gtk_list_store_append(store, &iter);
		profile = g_strdup_printf("%3u-%s",
					generic->list[i].directive,
					generic->list[i].operand);
		gtk_list_store_set(store, &iter,
				LIST_PROFILE, profile,
				PROFILE_NO, generic->list[i].directive,
				-1);
		g_free(profile);
	}
}

static gboolean apply_profile(transition_t *transition,
				GtkTreeSelection *domain_selection,
				GtkWidget *listview)
{
	GtkTreeSelection	*selection;
	GtkTreeIter		iter;
	GtkTreeModel		*model;
	GList			*list;
	guint			profile_no;
	gint			result;

	selection = gtk_tree_view_get_selection(
						GTK_TREE_VIEW(listview));
	if (selection &&
	    gtk_tree_selection_count_selected_rows(selection) == 1) {
		list = gtk_tree_selection_get_selected_rows(
							selection, NULL);
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(listview));
		gtk_tree_model_get_iter(model, &iter,
					g_list_first(list)->data);
		gtk_tree_model_get(model, &iter,
					PROFILE_NO, &profile_no, -1);
		DEBUG_PRINT("Selct Profile [%u]\n", profile_no);
		result = set_domain_profile(transition,
					domain_selection, profile_no);
		if (!result)
			refresh_transition(NULL, transition);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void set_domain(transition_t *transition)
{
	GtkTreeSelection	*domain_selection;
	GtkWidget		*dialog;
	GtkWidget		*listview;
	gint			count, response;

	if (transition->task_flag)
		domain_selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->tsk.treeview));
	else
		domain_selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(transition->treeview));

	if (!domain_selection || !(count =
	    gtk_tree_selection_count_selected_rows(domain_selection)))
		return;

	dialog = gtk_dialog_new_with_buttons(_("Profile list"),
			GTK_WINDOW(transition->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			NULL);

	listview = create_list_profile();
	add_list_profile(listview, &(transition->prf));
	view_cursor_set(listview, NULL, NULL);
	gtk_container_add(
		GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), listview);
	gtk_widget_set_size_request(dialog, 400, 300);
	gtk_widget_set_name(dialog, "GpetProfileSelectDialog");	// .gpetrc
	gtk_widget_show_all(dialog);

retry_profile:
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY) {
		DEBUG_PRINT("Apply button was pressed.\n");
		if (!apply_profile(transition,
					domain_selection, listview))
			goto retry_profile;
	} else if (response == GTK_RESPONSE_CANCEL) {
		DEBUG_PRINT("Cancel button was pressed.\n");
	} else {
		DEBUG_PRINT("Another response was recieved.\n");
	}
	gtk_widget_destroy(dialog);
}

static void edit_profile(transition_t *transition)
{
	GtkWidget		*dialog;
	GtkWidget		*hbox;
	GtkWidget		*label;
	GtkWidget		*entry;
	gchar			*profile, *label_str, *ptr;
	gint			index, response;

	if ((index = select_list_line(&(transition->prf))) < 0)
		return;

	entry = gtk_entry_new();

	profile = g_strdup(transition->prf.list[index].operand);
	ptr = strchr(profile, '=');
	if (ptr)
		ptr++;
	if (ptr) {
		gtk_entry_set_text(GTK_ENTRY(entry), ptr);
		*ptr = '\0';
	}

	if (transition->prf.list[index].directive < 256)
		label_str = g_strdup_printf("%3u-%s",
			transition->prf.list[index].directive, profile);
	else
		label_str = g_strdup_printf("%s", profile);
	label = gtk_label_new(label_str);
	g_free(profile);
	g_free(label_str);

	dialog = gtk_dialog_new_with_buttons(_("Profile Edit"),
			GTK_WINDOW(transition->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(
		GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

	gtk_widget_set_size_request(dialog, 600, -1);
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY) {
		const gchar	*input;
		char		*err_buff = NULL;
//		transition->prf.list[index].selected = 1;
		input = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		DEBUG_PRINT("Edit profile [%s] index[%d]\n", input, index);
		if (set_profile_level(index, input, &err_buff)) {
			g_warning("%s", err_buff);
			free(err_buff);
		} else {
			refresh_transition(NULL, transition);
		}
	}
	gtk_widget_destroy(dialog);
}

static void set_transition(GtkAction *action, transition_t *transition)
{
	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		DEBUG_PRINT("set profile\n");
		set_domain(transition);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		DEBUG_PRINT("edit profile\n");
		edit_profile(transition);
		break;
	default :
		DEBUG_PRINT("edit ???\n");
		break;
	}
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void manager_transition(GtkAction *action, transition_t *transition)
{
	manager_main(transition);
}

static void memory_transition(GtkAction *action, transition_t *transition)
{
	memory_main(transition);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
GdkPixbuf *get_png_file(void)
{
	GdkPixbuf	*pixbuf;
	GError		*err = NULL;

	pixbuf = gdk_pixbuf_new_from_file(PACKAGE_ICON_DIR "/tomoyo.png", &err);
	if (!pixbuf) {
		g_warning("%s", err->message);
		g_error_free(err);
	}
	return pixbuf;
}

static void show_about_dialog(void)
{
	GtkWidget		*dialog;
	GtkAboutDialog	*about;
	GdkPixbuf		*pixbuf;

	const gchar	*authors[] = {
		_("Yoshihiro Kusuno <yocto@users.sourceforge.jp>"),
		_("ccstools --- kumaneko san"),
		NULL};
	const gchar	*documenters[] = {_("Yoshihiro Kusuno"), NULL};
//	const gchar 	*translators = "none";

	dialog = gtk_about_dialog_new();
	about = GTK_ABOUT_DIALOG(dialog);
	gtk_about_dialog_set_name(about, "gpet");
	gtk_about_dialog_set_authors(about, authors);
	gtk_about_dialog_set_documenters(about, documenters);
//	gtk_about_dialog_set_translator_credits(about, translators);
	gtk_about_dialog_set_version(about, VERSION);
	gtk_about_dialog_set_copyright(about,
				"Copyright(C) 2010,2011 TOMOYO Linux Project");
	gtk_about_dialog_set_comments(about,
				"Gui Policy Editor for TOMOYO Linux 1.8"
				" or AKARI 1.0"
				"\n(based on ccs-editpolicy:ccstools)");
	gtk_about_dialog_set_website(about, "http://sourceforge.jp/projects/gpet/");
//	gtk_about_dialog_set_website_label(about, "http://tomoyo.sourceforge.jp/");

	pixbuf = get_png_file();
	if (pixbuf) {
		gtk_about_dialog_set_logo(about, pixbuf);
		g_object_unref(pixbuf);
	}
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(GTK_WIDGET(about));
}
