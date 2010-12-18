/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * menu.c
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

static void terminate(GtkAction *action, transition_t *transition);
static void delete_transition(GtkAction *action, transition_t *transition);
static void append_transition(GtkAction *action, transition_t *transition);
static void set_transition(GtkAction *action, transition_t *transition);
static void manager_transition(GtkAction *action, transition_t *transition);
static void memory_transition(GtkAction *action, transition_t *transition);
static void show_about_dialog(void);
static void Process_state(GtkAction *action, transition_t *transition);

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

  {"Search", GTK_STOCK_FIND, N_("_Search"), "<control>F",
	N_("Search for text"), NULL},
  {"SearchBack", GTK_STOCK_GO_BACK, N_("Search_Backwards"), "<control>P",
	N_("Search backwards for the same text"), NULL},
  {"SearchFoward", GTK_STOCK_GO_FORWARD, N_("SearchFor_wards"), "<control>N",
	N_("Search forwards for the same text"), NULL},

  {"Refresh", GTK_STOCK_REFRESH, N_("_Refresh"), "<control>R",
	N_("Refresh to the latest information"), G_CALLBACK(refresh_transition)},
  {"Manager", GTK_STOCK_DND, N_("_Manager"), "<control>M",
	N_("Manager Profile Editor"), G_CALLBACK(manager_transition)},
  {"Memory", GTK_STOCK_DND, N_("Memory_Usage"), "<control>U",
	N_("Memory Usage"), G_CALLBACK(memory_transition)},

  {"About", GTK_STOCK_ABOUT, N_("_About"), "<alt>A",
	N_("About a program"), show_about_dialog}
};
static guint n_entries = G_N_ELEMENTS(entries);

static GtkToggleActionEntry toggle_entries[] = {
  { "Process", GTK_STOCK_MEDIA_RECORD, N_("Pr_ocess"), "<control>O",
	N_("Process State Viewer"), G_CALLBACK(Process_state), FALSE},
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
"  </popup>"
"</ui>";

GtkWidget *create_menu(GtkWidget *parent, transition_t *transition,
				GtkWidget **toolbar)
{
	GtkUIManager		*ui;
	GtkActionGroup 	*actions;
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
		gtk_action_group_get_action(actions, "Search"), FALSE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "SearchBack"), FALSE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "SearchFoward"), FALSE);
	gtk_action_set_sensitive(
		gtk_action_group_get_action(actions, "Edit"), FALSE);

	transition->actions = actions;

//	gtk_ui_manager_set_add_tearoffs(ui, TRUE);
	gtk_window_add_accel_group(GTK_WINDOW(parent), 
				gtk_ui_manager_get_accel_group(ui));
	if (!gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, NULL)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	*toolbar = gtk_ui_manager_get_widget(ui, "/ToolBar");
	gtk_toolbar_set_style(GTK_TOOLBAR(*toolbar), GTK_TOOLBAR_ICONS);

	/* to save gpet.c popup_menu() */
	popup = gtk_ui_manager_get_widget(ui, "/PopUp");
	g_object_set_data(G_OBJECT(transition->window), "popup", popup);

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
static void Process_state(GtkAction *action, transition_t *transition)
{
	GtkWidget	*view;
	GtkWidget	*notebook;
	GtkWidget	*vbox2;
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
	vbox2 = g_object_get_data(
				G_OBJECT(transition->window), "vbox2");
	gtk_notebook_set_tab_label_text(
				GTK_NOTEBOOK(notebook), vbox2, label_str);
	gtk_notebook_set_menu_label_text(
				GTK_NOTEBOOK(notebook), vbox2, label_str);

	gtk_container_add(transition->container, view);

	transition->current_page = CCS_SCREEN_DOMAIN_LIST;
	refresh_transition(NULL, transition);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
void view_cursor_set(GtkWidget *view,
			GtkTreePath *path, GtkTreeViewColumn *column)
{
	if (!path) {
		path = gtk_tree_path_new_from_indices(0, -1);
	}
	gtk_tree_view_set_cursor(
			GTK_TREE_VIEW(view), path, column, FALSE);
	gtk_tree_view_row_activated(GTK_TREE_VIEW(view), path, column);
/*
	{
		gchar *path_str = gtk_tree_path_to_string(path);
		g_debug("TreePath[%s]", path_str);
		g_free(path_str);
	}
*/
	gtk_tree_path_free(path);
}

void refresh_transition(GtkAction *action, transition_t *transition)
{
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;
	GtkWidget		*view;

	switch((int)transition->current_page) {
	case CCS_SCREEN_EXCEPTION_LIST :
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			transition->exp.listview), &path, &column);
		if (get_exception_policy(
		    &(transition->exp.list), &(transition->exp.count)))
			break;
		add_list_data(&(transition->exp), TRUE);
		disp_statusbar(transition, CCS_SCREEN_EXCEPTION_LIST);
		view_cursor_set(transition->exp.listview, path, column);
//		gtk_widget_set_can_focus(transition->exp.listview, TRUE);
		gtk_widget_grab_focus(transition->exp.listview);
//		gtk_widget_set_state(transition->exp.listview, GTK_STATE_SELECTED);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			transition->prf.listview), &path, &column);
		if (get_profile(
		    &(transition->prf.list), &(transition->prf.count)))
			break;
		add_list_data(&(transition->prf), FALSE);
		disp_statusbar(transition, CCS_SCREEN_PROFILE_LIST);
		view_cursor_set(transition->prf.listview, path, column);
		gtk_widget_grab_focus(transition->prf.listview);
		break;
	case CCS_SCREEN_DOMAIN_LIST :
	case CCS_MAXSCREEN :
		view = transition->task_flag ?
			transition->tsk.treeview : transition->treeview;
		gtk_tree_view_get_cursor(
				GTK_TREE_VIEW(view), &path, &column);

		if (transition->task_flag) {
			if (get_task_list(&(transition->tsk.task),
						&(transition->tsk.count)))
				break;
			add_task_tree_data(GTK_TREE_VIEW(view), &(transition->tsk));
		} else {
			if (get_domain_policy(
			    transition->dp, &(transition->domain_count)))
				break;
			add_tree_data(GTK_TREE_VIEW(view), transition->dp);
		}

		gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
		view_cursor_set(view, path, column);
		gtk_widget_grab_focus(view);
		gtk_widget_show(view);
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
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static gint message_dialog(GtkWidget *parent, gchar *message)
{
	GtkWidget	*dialog;
	gint		result;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent) ,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s", message);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return result;
}

static void delete_transition(GtkAction *action, transition_t *transition)
{
	GtkTreeSelection	*selection;
	gint			count;
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
		 g_strdup_printf(_("Delete the %d selected exception policies?"), count) :
		 g_strdup_printf(_("Delete the selected exception policy?"));
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
		 g_strdup_printf(_("Delete the %d selected domains?"), count) :
		 g_strdup_printf(_("Delete the selected domain?"));
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
		message = count > 1 ? 
		 g_strdup_printf(_("Delete the %d selected policies?"), count) :
		 g_strdup_printf(_("Delete the selected policy?"));
		if (GTK_RESPONSE_YES ==
		    message_dialog(transition->window, message)) {
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
static GList* append_item(GtkComboBox	*combobox,
				GList *cmblist, const gchar *item_label)
{
	gtk_combo_box_append_text(combobox, g_strdup(item_label));
	return g_list_append(cmblist, g_strdup(item_label));
}
static GList* remove_item(GtkComboBox *combobox,
				GList *cmblist, gint index)
{
	gtk_combo_box_remove_text(combobox, index);
	g_free (g_list_nth_data(cmblist, index));
	return g_list_remove_link(cmblist,
					g_list_nth(cmblist, index));
}

#define COMBO_LIST_LIMIT	20
static GList	*combolist = NULL;
static gint combo_entry_activate(GtkWidget *combo, gchar **new_text)
{
	GtkComboBox	*combobox;
	GList		*list;
	gboolean	exist_flag = FALSE;

	combobox = GTK_COMBO_BOX(combo);
	(*new_text) = gtk_combo_box_get_active_text(combobox);
	if(!(*new_text) || strcmp(*new_text, "") == 0)
		return 1;

	for (list = combolist; list; list = g_list_next(list)) {
		if (strcmp(*new_text, (gchar *)list->data) == 0) {
			exist_flag = TRUE;
			break;
		}
	}

	if (!exist_flag) {
		combolist = append_item(combobox, combolist, *new_text);
		if (g_list_length(combolist) > COMBO_LIST_LIMIT) {
			combolist = remove_item(combobox, combolist, 0);
		}
		DEBUG_PRINT("Append '%s'.\n", *new_text);
	}

	return exist_flag ? 1 : 0;
}

static gint append_dialog(transition_t *transition, 
				gchar *title, gchar **input)
{
	GtkWidget		*dialog;
	GtkWidget		*combo;
	GList			*list;
	gint			response, result = 1;
  
	dialog = gtk_dialog_new_with_buttons(title,
			GTK_WINDOW(transition->window),
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
		result = combo_entry_activate(combo, input);
	}
	gtk_widget_destroy(dialog);

	return result;
}

static void append_transition(GtkAction *action, transition_t *transition)
{
	gchar		*input;
	gint		index, result = 1;
	char		*err_buff = NULL;

	switch((int)transition->current_page) {
	case CCS_SCREEN_DOMAIN_LIST :
		DEBUG_PRINT("append domain\n");
		result = append_dialog(transition,
				 _("Add Domain"), &input);
		if (!result)
			result = add_domain(input, &err_buff);
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
		break;
	case CCS_SCREEN_EXCEPTION_LIST :
		DEBUG_PRINT("append exception\n");
		result = append_dialog(transition,
				 _("Add Exception"), &input);
		if (!result)
			result = add_exception_policy(input, &err_buff);
		break;
	case CCS_SCREEN_PROFILE_LIST :
		DEBUG_PRINT("append profile\n");
		result = append_dialog(transition,
				 _("Add Profile"), &input);
		if (!result)
			result = add_profile(input, &err_buff);
		break;
	default :
		DEBUG_PRINT("append ???\n");
		break;
	}

	if (result && err_buff) {
		g_warning("%s", err_buff);
		free(err_buff);
	} else {
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
	gtk_container_add(
		GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), listview);
	gtk_widget_set_size_request(dialog, 400, 300);  
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
  
	if ((index = select_profile_line(&(transition->prf))) < 0)
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
				"Copyright(C) 2010 TOMOYO Linux Project");
	gtk_about_dialog_set_comments(about,
				"Gui Policy Editor for TOMOYO Linux 1.8"
				"\n(based on ccs-editpolicy:ccstools)");
	gtk_about_dialog_set_website(about, "http://gpet.sourceforge.jp/");
//	gtk_about_dialog_set_website_label(about, "http://tomoyo.sourceforge.jp/");

	pixbuf = get_png_file();
	gtk_about_dialog_set_logo(about, pixbuf);
	g_object_unref(pixbuf);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(GTK_WIDGET(about));
}
