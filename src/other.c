/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * other.c
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

#define PRINT_WARN(err) {g_warning("%s", err->message);g_error_free(err);err = NULL;}

typedef struct _other_t {
	GtkWidget		*dialog;
	GtkActionGroup 	*actions;
	GtkWidget		*popup;
	generic_list_t	manager;
	generic_list_t	memory;
} other_t;

static void edit_memory(GtkAction *action, other_t *data);
static void append_manager(GtkAction *action, other_t *data);
static void delete_manager(GtkAction *action, other_t *data);

static GtkActionEntry entries[] = {
  {"Edit", GTK_STOCK_EDIT, N_("_Edit"), "<control>E",
	N_("Edit the selected line"), G_CALLBACK(edit_memory)},
  {"Add", GTK_STOCK_ADD, N_("_Add"), "<control>I",
	N_("Append line"), G_CALLBACK(append_manager)},
  {"Delete", GTK_STOCK_DELETE, N_("_Delete"), "<control>D",
	N_("Delete the selected line"), G_CALLBACK(delete_manager)},
};

static guint n_entries = G_N_ELEMENTS(entries);

static const gchar *ui_info = 
"<ui>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='Edit'/>"
"    <toolitem action='Add'/>"
"    <toolitem action='Delete'/>"
"  </toolbar>"

"  <popup name='PopUp'>"
"      <menuitem action='Edit'/>"
"      <menuitem action='Add'/>"
"      <menuitem action='Delete'/>"
"  </popup>"
"</ui>";

static GtkWidget *create_dialog_menu(GtkWidget *parent, other_t *data)
{
	GtkUIManager		*ui;
	GtkActionGroup 	*actions;
	GtkWidget		*toolbar;
	GError			*error = NULL;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_set_translation_domain(actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(actions, entries, n_entries, data);

	ui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui, actions, 0);

	gtk_action_set_sensitive(gtk_action_group_get_action(
						actions, "Edit"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(
						actions, "Add"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(
						actions, "Delete"), FALSE);
	data->actions = actions;

	gtk_window_add_accel_group(GTK_WINDOW(parent), 
				gtk_ui_manager_get_accel_group(ui));
	if (!gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, NULL)) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
	}

	toolbar = gtk_ui_manager_get_widget(ui, "/ToolBar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	data->popup = gtk_ui_manager_get_widget(ui, "/PopUp");

	return toolbar;
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/

enum list_column_pos {
	LIST_NUMBER,		// n
	LIST_COLON,		// :
	LIST_MANAGER,		// display
	N_COLUMNS_LIST
};
static GtkWidget *create_list_manager(void)
{
	GtkWidget		*treeview;
	GtkListStore		*liststore;
	GtkCellRenderer	*renderer;
	GtkTreeViewColumn	*column;

	liststore = gtk_list_store_new(N_COLUMNS_LIST,
				G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	g_object_unref(liststore);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0, "ypad", 0, NULL);
	column = gtk_tree_view_column_new_with_attributes(
			"No.", renderer, "text", LIST_NUMBER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_sort_column_id(column, LIST_NUMBER);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(" ", renderer,
					      "text", LIST_COLON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("operand",
			renderer, "text", LIST_MANAGER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_sort_column_id(column, LIST_MANAGER);

	// ヘッダ表示
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);

	return treeview;
}

static void add_list_manager(generic_list_t *man)
{
	GtkTreePath		*path = NULL;
	GtkTreeViewColumn	*column = NULL;
	GtkListStore		*store;
	GtkTreeIter		iter;
	gint			i;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(
			man->listview), &path, &column);

	get_manager(&(man->list), &(man->count));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(
					GTK_TREE_VIEW(man->listview)));

	gtk_list_store_clear(store);
	for(i = 0; i < man->count; i++){
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
					LIST_NUMBER, i,
					LIST_COLON,  ":",
					LIST_MANAGER, man->list[i].operand,
					-1);
	}

	view_cursor_set(man->listview, path, column);
	gtk_widget_grab_focus(man->listview);
}

static void create_manager_view(GtkWidget *dialog, GtkWidget *listview)
{
	GtkWidget	*scrolledwin;

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(scrolledwin), GTK_SHADOW_IN);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
					scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolledwin), listview);

	// 複数行選択可
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
		GTK_TREE_VIEW(listview)), GTK_SELECTION_MULTIPLE);
	// マウス複数行選択
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(listview), TRUE);
}

static gboolean cb_select_list(GtkTreeView *listview,
				GdkEventButton *event, other_t *data)
{
	if (event->button == 3) {
		gtk_menu_popup(GTK_MENU(data->popup),
					NULL, NULL, NULL, NULL,
					0, gtk_get_current_event_time());
		return TRUE;
	}

	return FALSE;
}

/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void cb_button(GtkButton *button, gpointer data)
{
	GtkWidget		*dialog;
	GtkWidget		*parent;
	GtkWidget		*entry;
	static gchar		*folder = NULL;
	gint			response;

	
	parent = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "parent"));
	entry  = GTK_WIDGET(data);

	dialog = gtk_file_chooser_dialog_new(_("File Selection Dialog"),
				GTK_WINDOW(parent),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	if (folder) {
		gtk_file_chooser_set_current_folder(
				GTK_FILE_CHOOSER(dialog), folder);
		g_free(folder);
	}
	gtk_widget_show(dialog);

	response = gtk_dialog_run (GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_ACCEPT) {
		gchar	*filename;

		filename = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER(dialog));
		folder = gtk_file_chooser_get_current_folder(
						GTK_FILE_CHOOSER(dialog));
		DEBUG_PRINT("%s\n", folder);
		gtk_entry_set_text(GTK_ENTRY(entry), filename);
		g_free(filename);
	} else {
		DEBUG_PRINT("Another response was recieved.\n");    
	}
	gtk_widget_destroy(dialog);
}

static void append_manager(GtkAction *action, other_t *data)
{
	GtkWidget		*dialog;
	GtkWidget		*hbox;
	GtkWidget		*entry;
	GtkWidget		*button;
	gint			response;

	DEBUG_PRINT("append manager\n");

	dialog = gtk_dialog_new_with_buttons(_("Manager Add"),
			GTK_WINDOW(data->dialog),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			NULL);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(
		GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(entry), "parent", (gpointer)dialog);

	button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(cb_button), (gpointer)entry);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);  

	gtk_widget_set_size_request(dialog, 600, -1);  
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY) {
		DEBUG_PRINT("append path\n");
		const gchar	*input;
		char		*err_buff = NULL;
		gint		result;
		input = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		result = add_manager((char *)input, &err_buff);
		if (result) {
			g_warning("%s", err_buff);
			free(err_buff);
		} else {
			add_list_manager(&(data->manager));
		}
	}
	gtk_widget_destroy(dialog);
}

/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void set_delete_flag(gpointer data, generic_list_t *gen)
{
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	gint			number;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gen->listview));
	if (!model || !gtk_tree_model_get_iter(model, &iter, data)) {
		g_warning("ERROR: %s(%d)", __FILE__, __LINE__);
		return;
	}
	gtk_tree_model_get(model, &iter, LIST_NUMBER, &number, -1);

	DEBUG_PRINT(" index[%d]\n", number);
	gen->list[number].selected = 1;
}

static void delete_manager(GtkAction *action, other_t *data)
{
	GtkWidget		*dialog;
	GtkTreeSelection	*selection;
	gint			count;
	gchar			*message = NULL;
	GList			*list;
	char			*err_buff = NULL;
	int			result = 0;

	DEBUG_PRINT("delete manager\n");
	selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(data->manager.listview));
	if (!selection || !(count = 
		    gtk_tree_selection_count_selected_rows(selection)))
		return;

	DEBUG_PRINT("count[%d]\n", count);
	message = count > 1 ? 
	 g_strdup_printf(_("Delete the %d selected managers?"), count) :
	 g_strdup_printf(_("Delete the selected manager?"));
	dialog = gtk_message_dialog_new(GTK_WINDOW(data->dialog),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s", message);

	if (GTK_RESPONSE_YES == gtk_dialog_run(GTK_DIALOG(dialog))) {
		list = gtk_tree_selection_get_selected_rows(selection, NULL);
		g_list_foreach(list, (GFunc)set_delete_flag, &(data->manager));
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);

		result = delete_manager_policy(data->manager.list,
					data->manager.count, &err_buff);
		if (result) {
			g_warning("%s", err_buff);
			free(err_buff);
		} else {
			add_list_manager(&(data->manager));
		}
	}
	gtk_widget_destroy(dialog);
	g_free(message);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
void manager_main(transition_t *transition)
{
	other_t		data;
	GtkWidget		*dialog;
	GtkWidget		*listview;
	gint			response;

	transition->current_page = CCS_SCREEN_MANAGER_LIST;

	dialog = gtk_dialog_new_with_buttons(_("Manager Policy"),
			GTK_WINDOW(transition->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);

	create_dialog_menu(dialog, &data);
	listview = create_list_manager();
	create_manager_view(dialog, listview);

	data.dialog = dialog;
	data.manager.listview = listview;
	data.manager.count = 0;
	data.manager.list = NULL;
	add_list_manager(&(data.manager));

	gtk_action_set_sensitive(gtk_action_group_get_action(
					data.actions, "Add"), TRUE);
	gtk_action_set_sensitive(gtk_action_group_get_action(
					data.actions, "Delete"), TRUE);
	g_signal_connect(G_OBJECT(listview), "button-press-event", 
					G_CALLBACK(cb_select_list), &data);

	gtk_widget_set_size_request(dialog, 640, 480);
	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_CLOSE) {
		DEBUG_PRINT("Close button was pressed.\n");
	} else {
		DEBUG_PRINT("Another response was recieved.\n");
	}
	gtk_widget_destroy(dialog);
}
/*-------+---------+---------+---------+---------+---------+---------+--------*/
static void get_disp_column(const char *data, gchar **head,
				gchar **now_str, gchar **quota_str)
{
	int		cnt;	
	guint		now, quota;
	gboolean	bNow = TRUE, bQuota = TRUE;	

	if ((cnt = sscanf(data, "Policy: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  Policy";
	} else if ((cnt = sscanf(data, "Audit logs: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  Audit logs";
	} else if ((cnt = sscanf(data, "Query lists: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  Query lists";
	} else if ((cnt = sscanf(data, "Total: %u", &now)) == 1) {
		*head = "Total memory in use";
	} else if ((cnt = sscanf(data, "Shared: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  String data";
	} else if ((cnt = sscanf(data, "Private: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  Numeric data";
	} else if ((cnt = sscanf(data, "Dynamic: %u (Quota: %u)", &now, &quota)) >= 1) {
		*head = "  Temporary data";
	} else {
		*head = (gchar *)data;
		bNow = FALSE;
		bQuota = FALSE;
		DEBUG_PRINT("Not found!\n");
	}
	if (cnt == 1)
		bQuota = FALSE;

	*now_str = bNow ? g_strdup_printf("%u", now) : g_strdup("");
	*quota_str = bQuota ? g_strdup_printf("%u", quota) : g_strdup("");
}

enum mem_column_pos {
	LIST_HEAD,		// 
	LIST_NOW,		// 
	LIST_QUOTA,		// 
	N_COLUMNS_MEM_LIST
};

static void add_list_memory(generic_list_t *mem)
{
	GtkListStore		*store;
	GtkTreeIter		iter;
	gint			i;
	gchar			*head, *now, *quota;

	get_memory(&(mem->list), &(mem->count));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(
					GTK_TREE_VIEW(mem->listview)));

	gtk_list_store_clear(store);
	for(i = 0; i < mem->count; i++){
		gtk_list_store_append(store, &iter);
		get_disp_column(mem->list[i].operand,
						&head, &now, &quota);
		gtk_list_store_set(store, &iter,
					LIST_HEAD, head,
					LIST_NOW, now,
					LIST_QUOTA, quota,
					-1);
		g_free(now);
		g_free(quota);
	}

	view_cursor_set(mem->listview, NULL, NULL);
	gtk_widget_grab_focus(mem->listview);
}

static void cb_cell_edited(GtkCellRendererText *cell,
		const gchar *path_string, const gchar *new_text,
		generic_list_t *mem)
{
	GtkTreeModel	*model;
	GtkTreePath	*path;
	GtkTreeIter	iter;
	gint		index;
	gchar		*old_text, *cp, *start;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mem->listview));
	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, LIST_QUOTA, &old_text, -1);

	index = gtk_tree_path_get_indices(path)[0];
//(2.16)	if (g_strcmp0(old_text, new_text))
	if (strcmp(old_text, new_text))
		mem->list[index].selected = 1;
	g_free(old_text);

	start = (gchar *)mem->list[index].operand;
	cp = strchr(mem->list[index].operand, ':');
	if (cp)
		*cp = '\0';
	mem->list[index].operand = g_strdup_printf("%s:%s", start, new_text);
	free(start);
	DEBUG_PRINT(" Input mem:%2d[%s]\n", index, mem->list[index].operand);

	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					LIST_QUOTA, new_text, -1);

	gtk_tree_path_free(path);
}

static GtkWidget *create_list_memory(generic_list_t *mem)
{
	GtkWidget		*treeview;
	GtkListStore		*liststore;
	GtkCellRenderer	*renderer;

	liststore = gtk_list_store_new(N_COLUMNS_MEM_LIST,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	g_object_unref(liststore);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(
			GTK_TREE_VIEW(treeview), -1, _("Memory used     "),
			renderer, "text", LIST_HEAD, NULL);
	gtk_cell_renderer_set_fixed_size(renderer, 200, -1);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0, "ypad", 0, NULL);
	gtk_tree_view_insert_column_with_attributes(
			GTK_TREE_VIEW(treeview), -1, _(" Now (bytes) "),
			renderer, "text", LIST_NOW, NULL);
	gtk_cell_renderer_set_fixed_size(renderer, 100, -1);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "xalign", 1.0, "ypad", 0, NULL);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited",
				G_CALLBACK(cb_cell_edited), mem);
	gtk_tree_view_insert_column_with_attributes(
			GTK_TREE_VIEW(treeview), -1, _(" Quota (bytes) "),
			renderer, "text", LIST_QUOTA, NULL);
	gtk_cell_renderer_set_fixed_size(renderer, 100, -1);

	// ヘッダ表示
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);

	return treeview;
}

static void create_memory_view(GtkWidget *dialog, GtkWidget *listview)
{
	GtkWidget	*scrolledwin;

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(scrolledwin), GTK_SHADOW_IN);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
					scrolledwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolledwin), listview);
}

static void edit_memory(GtkAction *action, other_t *data)
{
	GtkTreeSelection	*selection;
	GList			*list;
	GtkTreeViewColumn	*column;

	DEBUG_PRINT("edit memory\n");

	selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(data->memory.listview));
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	column = gtk_tree_view_get_column(
		GTK_TREE_VIEW(data->memory.listview), LIST_QUOTA);
	gtk_tree_view_set_cursor_on_cell(
		GTK_TREE_VIEW(data->memory.listview),
		g_list_first(list)->data, column, NULL, TRUE);
}

void memory_main(transition_t *transition)
{
	other_t		data;
	GtkWidget		*dialog;
	GtkWidget		*listview;
	gint			response;

	transition->current_page = CCS_SCREEN_MEMINFO_LIST;

	dialog = gtk_dialog_new_with_buttons(_("Memory Usage"),
			GTK_WINDOW(transition->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			NULL);

	data.dialog = dialog;
	data.memory.count = 0;
	data.memory.list = NULL;
	create_dialog_menu(dialog, &data);
	listview = create_list_memory(&(data.memory));
	data.memory.listview = listview;
	create_memory_view(dialog, listview);

	add_list_memory(&(data.memory));

	gtk_action_set_sensitive(gtk_action_group_get_action(
					data.actions, "Edit"), TRUE);
	g_signal_connect(G_OBJECT(listview), "button-press-event", 
					G_CALLBACK(cb_select_list), &data);

	gtk_widget_set_size_request(dialog, 450, 300);
	gtk_widget_show_all(dialog);
  
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY) {
		char	*err_buff = NULL;
		DEBUG_PRINT("Apply button was pressed.\n");
		if (set_memory(data.memory.list,
				data.memory.count, &err_buff)) {
			g_warning("%s", err_buff);
			free(err_buff);
		}
	} else {
		DEBUG_PRINT("Another response was recieved.\n");
	}
	gtk_widget_destroy(dialog);
}

