/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * gpet.h
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

#ifndef __GPET_H__
#define __GPET_H__

#include "ccstools.h"
#include "editpolicy.h"

typedef struct _generic_list_t {
	GtkWidget	*listview;
	struct	ccs_generic_acl	*list;
	int	count;
} generic_list_t;

typedef struct _task_list_t {
	GtkWidget	*treeview;
	struct	ccs_task_entry	*task;
	int	count;
} task_list_t;

enum addentry_type {
	ADDENTRY_DOMAIN_LIST,
	ADDENTRY_ACL_LIST,
	ADDENTRY_EXCEPTION_LIST,
	ADDENTRY_PROFILE_LIST,
	ADDENTRY_MANAGER_LIST,
	ADDENTRY_NAMESPACE_LIST,
	ADDENTRY_NON
};

typedef struct _transition_t {
	GtkWidget		*window;
	GtkWidget		*domainbar;
	GtkWidget		*treeview;
	GtkWidget		*statusbar;
	gint			contextid;
	enum ccs_screen_type	current_page;
	GtkActionGroup		*actions;

	GtkContainer		*container;

	struct ccs_domain_policy3	*dp;
	int				domain_count;
	generic_list_t		acl;	// ACL
	GtkWidget			*acl_window;
	gboolean			acl_detached;
	generic_list_t		exp;	// exception
	generic_list_t		prf;	// profile
	generic_list_t		ns;	// namespace
	task_list_t			tsk;	// Process
	int				task_flag;	// 1:process
							// 0:domain
	enum addentry_type	addentry;
} transition_t;


//#define DEBUG

#ifdef DEBUG
	#define DEBUG_PRINT(format, ...) \
	g_print("%s:%d (%s) " format, __FILE__, __LINE__, G_STRFUNC, ##__VA_ARGS__)
#else
	#define DEBUG_PRINT(...)
#endif


#define CCS_DISK_POLICY_DIR			"/policy/current/"
#define CCS_DISK_POLICY_DOMAIN_POLICY		"domain_policy.conf"
#define CCS_DISK_POLICY_EXCEPTION_POLICY	"exception_policy.conf"
#define CCS_DISK_POLICY_MANAGER			"manager.conf"
#define CCS_DISK_POLICY_PROFILE			"profile.conf"


// ccstools  editpolicy.c
int ccs_main(int argc, char *argv[]);

// interface.inc
int get_domain_policy(struct ccs_domain_policy3 *dp, int *count);
int add_domain(char *input, char **err_buff);
int set_profile(struct ccs_domain_policy3 *dp,
				char *profile, char **err_buff);
int get_task_list(struct ccs_task_entry **tsk, int *count);
const char *get_domain_name(const struct ccs_domain_policy3 *dp,
				const int index);
int get_acl_list(struct ccs_domain_policy3 *dp, int current,
			struct ccs_generic_acl **ga, int *count);
int get_process_acl_list(int current,
				struct ccs_generic_acl **ga, int *count);
int get_optimize_acl_list(int current, struct ccs_generic_acl **ga, int count);
int add_acl_list(struct ccs_domain_policy3 *dp, int current,
			char *input, char **err_buff);
const char *get_transition_name(enum ccs_transition_type type);
int get_exception_policy(struct ccs_generic_acl **ga, int *count);
int add_exception_policy(char *input, char **err_buff);
int get_profile(struct ccs_generic_acl **ga, int *count);
int add_profile(char *input, char **err_buff);
int set_profile_level(int index, const char *input, char **err_buff);
int get_namespace(struct ccs_generic_acl **ga, int *count);
int add_namespace(char *input, char **err_buff);
int get_manager(struct ccs_generic_acl **ga, int *count);
int add_manager(char *input, char **err_buff);
int get_memory(struct ccs_generic_acl **ga, int *count);
int set_memory(struct ccs_generic_acl *ga, int count, char **err_buff);
int delete_domain_policy(struct ccs_domain_policy3 *dp, char **err_buff);
int delete_acl_policy(struct ccs_domain_policy3 *dp, char **err_buff,
				struct ccs_generic_acl *ga, int count);
int delete_exp_policy(struct ccs_domain_policy3 *dp, char **err_buff,
				struct ccs_generic_acl *ga, int count);
int delete_manager_policy(
		struct ccs_generic_acl *ga, int count, char **err_buff);
_Bool is_offline(void);
_Bool is_network(void);
char *get_remote_ip(char *str_ip);
const char *get_policy_dir(void);
const char *get_domain_last_name(const int index);
int get_find_target_domain(const int index);
const char *get_ns_name(void);
void put_ns_name(const char *namespace);
_Bool is_ccs(void);

// gpet.c
gchar *decode_from_octal_str(const char *name);
void add_tree_data(GtkTreeView *treeview, struct ccs_domain_policy3 *dp);
void add_list_data(generic_list_t *generic,
				enum ccs_screen_type current_page);
gint get_current_domain_index(transition_t *transition);
gchar *get_alias_and_operand(GtkWidget *view, gboolean alias_flag);
void set_position_addentry(transition_t *transition, GtkTreePath **path);
void set_sensitive(GtkActionGroup *actions, int task_flag,
				enum ccs_screen_type current_page);
gint delete_domain(transition_t *transition,
			GtkTreeSelection *selection, gint count);
void view_setting(GtkWidget *treeview, gint search_column);
gint set_domain_profile(transition_t *transition,
			GtkTreeSelection *selection, guint profile);
gboolean disp_acl_line(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, generic_list_t *acl);
gint select_list_line(generic_list_t *gen);
gint delete_acl(transition_t *transition,
			GtkTreeSelection *selection, gint count);
gint delete_exp(transition_t *transition,
			GtkTreeSelection *selection, gint count);
gchar *disp_window_title(enum ccs_screen_type current_page);
int gpet_main(void);

// menu.c
GtkWidget *create_menu(GtkWidget *parent, transition_t *transition,
                       GtkWidget **toolbar);
void disp_statusbar(transition_t *transition, int scr);
gchar *encode_to_octal_str(const char *str);
void view_cursor_set(GtkWidget *view,
			GtkTreePath *path, GtkTreeViewColumn *column);
void refresh_transition(GtkAction *action, transition_t *transition);
gchar *get_combo_entry_last(void);
GdkPixbuf *get_png_file(void);

// conf.c
void read_config(transition_t *tran);
void write_config(transition_t *tran);

// other.c
gboolean namespace_main(transition_t *transition);
void manager_main(transition_t *transition);
void memory_main(transition_t *transition);

// process.c
void add_task_tree_data(GtkTreeView *treeview, task_list_t *tsk);
void set_select_flag_process(gpointer data, task_list_t *tsk);
GtkWidget *create_task_tree_model(transition_t *transition);
gint get_current_process_index(task_list_t *tsk);

// search.c
GList *insert_item(GtkComboBox	*combobox,
			GList *cmblist, const gchar *item_label, gint index);
GList *remove_item(GtkComboBox *combobox, GList *cmblist, gint index);
void search_input(GtkAction *action, transition_t *transition);
void search_back(GtkAction *action, transition_t *transition);
void search_forward(GtkAction *action, transition_t *transition);

#endif /* __GPET_H__ */
