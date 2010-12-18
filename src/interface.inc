/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * interface.inc
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

int get_domain_policy(struct ccs_domain_policy *dp, int *count)
{
	int	result = 1;

	ccs_domain_sort_type = 0;
	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	result = ccs_generic_list_loop(dp);
	*count = dp->list_len - ccs_unnumbered_domain_count;
	return result;
}

int add_domain(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	gpet_line = input;
	ccs_add_entry(NULL);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int set_profile(struct ccs_domain_policy *dp,
				char *profile, char **err_buff)
{
	int	dummy = 0, result = 0;

	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	gpet_line = profile;
	ccs_set_profile(dp, dummy);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int get_task_list(struct ccs_task_entry **tsk, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	int	result;

	ccs_domain_sort_type = 1;
	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	result = ccs_generic_list_loop(dp);
	*tsk = ccs_task_list;
	*count = ccs_task_list_len;
	return result;
}

static void generic_acl_copy(struct ccs_generic_acl **src, int src_cnt, 
				struct ccs_generic_acl **dst, int dst_cnt)
{
	int	i;

	while (dst_cnt)
		free((void *)((*dst)[--dst_cnt].operand));

	*dst = realloc(*dst, src_cnt * sizeof(struct ccs_generic_acl));
	if (!(*dst))
		ccs_out_of_memory();

	memcpy(*dst, ccs_generic_acl_list,
			src_cnt * sizeof(struct ccs_generic_acl));
	for (i = 0; i < src_cnt; i++) {
		(*dst)[i].directive = (*src)[i].directive;
		(*dst)[i].selected = (*src)[i].selected;
		(*dst)[i].operand = strdup((*src)[i].operand);
		if (!((*dst)[i].operand))
			ccs_out_of_memory();
	}
} 

int get_acl_list(struct ccs_domain_policy *dp, int current,
			struct ccs_generic_acl **ga, int *count)
{
	int	result = 0;

	if (ccs_initializer_source(dp, current)) {
		*count = 0;
	} else if (ccs_deleted_domain(dp, current)) {
		*count = 0;
	} else {
		free(ccs_current_domain);
		ccs_current_domain = strdup(ccs_domain_name(dp, current));
		if (!ccs_current_domain)
			ccs_out_of_memory();
		else {
			ccs_domain_sort_type = 0;
			ccs_current_screen = CCS_SCREEN_ACL_LIST;
			result = ccs_generic_list_loop(dp);
			generic_acl_copy(&ccs_generic_acl_list,
						ccs_generic_acl_list_count,
						ga, *count);
			*count = ccs_generic_acl_list_count;
		}
	}
	return result;
}

int get_process_acl_list(int current,
				struct ccs_generic_acl **ga, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	int	result = 0;

	free(ccs_current_domain);
	ccs_current_pid = ccs_task_list[current].pid;
	ccs_current_domain = strdup(ccs_task_list[current].domain);
	if (!ccs_current_domain)
		ccs_out_of_memory();
	else {
		ccs_domain_sort_type = 1;
		ccs_current_screen = CCS_SCREEN_ACL_LIST;
		result = ccs_generic_list_loop(dp);
		generic_acl_copy(&ccs_generic_acl_list,
					ccs_generic_acl_list_count,
					ga, *count);
		*count = ccs_generic_acl_list_count;
	}
	return result;
}

int add_acl_list(struct ccs_domain_policy *dp, int current,
			char *input, char **err_buff)
{
	int	result = 0;

	if (ccs_initializer_source(dp, current) ||
		ccs_deleted_domain(dp, current))
		return 1;

	ccs_current_screen = CCS_SCREEN_ACL_LIST;
	gpet_line = input;
	ccs_add_entry(NULL);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

const char *get_transition_name(enum ccs_transition_type type)
{
    return ccs_transition_type[type];
}

int get_exception_policy(struct ccs_generic_acl **ga, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	int	result;

	ccs_current_screen = CCS_SCREEN_EXCEPTION_LIST;
	result = ccs_generic_list_loop(dp);
	*ga = ccs_generic_acl_list;
	*count = ccs_generic_acl_list_count;
	return result;
}

int add_exception_policy(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_EXCEPTION_LIST;
	gpet_line = input;
	ccs_add_entry(NULL);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int get_profile(struct ccs_generic_acl **ga, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	int	result;

	while (*count)
		free((void *)((*ga)[--(*count)].operand));

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	result = ccs_generic_list_loop(dp);

	generic_acl_copy(&ccs_generic_acl_list,
				ccs_generic_acl_list_count,
				ga, *count);
	*count = ccs_generic_acl_list_count;

	return result;
}

int add_profile(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	gpet_line = input;
	ccs_add_entry(NULL);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int set_profile_level(int index, const char *input, char **err_buff)
{
	struct ccs_domain_policy *dp = NULL;
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	ccs_generic_acl_list[index].selected = 1;
	gpet_line = (char *)input;
	ccs_set_level(dp, 0);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int get_manager(struct ccs_generic_acl **ga, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	struct ccs_generic_acl *orig_generic_acl_list = NULL;
	int		orig_generic_acl_list_count = 0;
	int		result;

	generic_acl_copy(&ccs_generic_acl_list,
				ccs_generic_acl_list_count,
			&orig_generic_acl_list,
				orig_generic_acl_list_count);
	orig_generic_acl_list_count = ccs_generic_acl_list_count;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	result = ccs_generic_list_loop(dp);

	generic_acl_copy(&ccs_generic_acl_list,
				ccs_generic_acl_list_count,
				ga, *count);
	*count = ccs_generic_acl_list_count;

	generic_acl_copy(&orig_generic_acl_list,
				orig_generic_acl_list_count,
			&ccs_generic_acl_list,
				 ccs_generic_acl_list_count);
	ccs_generic_acl_list_count = orig_generic_acl_list_count;

	return result;
}

int add_manager(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	gpet_line = input;
	ccs_add_entry(NULL);
	gpet_line = NULL;

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int get_memory(struct ccs_generic_acl **ga, int *count)
{
	struct ccs_domain_policy *dp = NULL;
	struct ccs_generic_acl *orig_generic_acl_list = NULL;
	int		orig_generic_acl_list_count = 0;
	int		result;

	generic_acl_copy(&ccs_generic_acl_list,
				ccs_generic_acl_list_count,
			&orig_generic_acl_list,
				orig_generic_acl_list_count);
	orig_generic_acl_list_count = ccs_generic_acl_list_count;

	ccs_current_screen = CCS_SCREEN_MEMINFO_LIST;
	result = ccs_generic_list_loop(dp);

	generic_acl_copy(&ccs_generic_acl_list,
				ccs_generic_acl_list_count,
				ga, *count);
	*count = ccs_generic_acl_list_count;

	generic_acl_copy(&orig_generic_acl_list,
				orig_generic_acl_list_count,
			&ccs_generic_acl_list,
				 ccs_generic_acl_list_count);
	ccs_generic_acl_list_count = orig_generic_acl_list_count;

	return result;
}

int set_memory(struct ccs_generic_acl *ga, int count, char **err_buff)
{
	struct ccs_domain_policy *dp = NULL;
	int	i, result = 0;

	ccs_current_screen = CCS_SCREEN_MEMINFO_LIST;
	ccs_policy_file = CCS_PROC_POLICY_MEMINFO;
	generic_acl_copy(&ga, count,
		&ccs_generic_acl_list, ccs_generic_acl_list_count);
	ccs_generic_acl_list_count = count;

	for (i = 0; i < count; i++) {
		ccs_generic_acl_list[i].selected = 0;
	}	

	for (i = 0; i < count; i++) {
		char	*cp;
		cp = strrchr(ccs_generic_acl_list[i].operand, ':');
		if (cp && ga[i].selected) {
			gpet_line = strdup(++cp);
fprintf(stderr, "mem:%2d[%s][%s]\n", i, ccs_generic_acl_list[i].operand, cp);
			ccs_generic_acl_list[i].selected = 1;
			ccs_set_quota(dp, -1);
			ccs_generic_acl_list[i].selected = 0;
		}
	}

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int delete_domain_policy(struct ccs_domain_policy *dp, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	ccs_policy_file = CCS_PROC_POLICY_DOMAIN_POLICY;
	ccs_delete_entry(dp, -1);

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int delete_acl_policy(struct ccs_domain_policy *dp, char **err_buff,
				struct ccs_generic_acl *ga, int count)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_ACL_LIST;
	ccs_policy_file = CCS_PROC_POLICY_DOMAIN_POLICY;

	generic_acl_copy(&ga, count,
		&ccs_generic_acl_list, ccs_generic_acl_list_count);
	ccs_generic_acl_list_count = count;
	ccs_delete_entry(dp, -1);

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int delete_exp_policy(struct ccs_domain_policy *dp, char **err_buff,
				struct ccs_generic_acl *ga, int count)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_EXCEPTION_LIST;
	ccs_policy_file = CCS_PROC_POLICY_EXCEPTION_POLICY;
	ccs_generic_acl_list = ga;
	ccs_generic_acl_list_count = count;
	ccs_delete_entry(dp, -1);

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int delete_manager_policy(
		struct ccs_generic_acl *ga, int count, char **err_buff)
{
	struct ccs_domain_policy *dp = NULL;
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	ccs_policy_file = CCS_PROC_POLICY_MANAGER;
	generic_acl_copy(&ga, count,
		&ccs_generic_acl_list, ccs_generic_acl_list_count);
	ccs_generic_acl_list_count = count;
	ccs_delete_entry(dp, -1);

	if (ccs_last_error) {
		(*err_buff) = strdup(ccs_last_error);
		free(ccs_last_error);
		ccs_last_error = NULL;
		result = 1;
	}

	return result;
}

int is_offline(void)
{
	return ccs_offline_mode;
}
