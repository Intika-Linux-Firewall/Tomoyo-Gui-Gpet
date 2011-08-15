/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * interface.inc
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

int get_domain_policy(struct ccs_domain_policy *dp, int *count)
{
	int	result = 1;

	ccs_domain_sort_type = 0;
	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	ccs_dp = *dp;
	result = ccs_generic_list_loop();
	*dp = ccs_dp;
	*count = dp->list_len - ccs_unnumbered_domain_count;
	return result;
}

int add_domain(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	gpet_line = input;
	ccs_add_entry();
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
	ccs_dp = *dp;
	ccs_set_profile(dummy);
//	*dp = ccs_dp;
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
	int	result;

	ccs_domain_sort_type = 1;
	ccs_current_screen = CCS_SCREEN_DOMAIN_LIST;
	result = ccs_generic_list_loop();
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

	if (src_cnt < 1)
		return;

	*dst = realloc(*dst, src_cnt * sizeof(struct ccs_generic_acl));
	if (!(*dst))
		ccs_out_of_memory();

	memcpy(*dst, ccs_gacl_list,
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

	ccs_dp = *dp;
	if (ccs_initializer_source(current)) {
		*count = 0;
	} else if (ccs_deleted_domain(current)) {
		*count = 0;
	} else {
		free(ccs_current_domain);
		ccs_current_domain = strdup(ccs_domain_name(dp, current));
		if (!ccs_current_domain)
			ccs_out_of_memory();
		else {
			ccs_domain_sort_type = 0;
			ccs_current_screen = CCS_SCREEN_ACL_LIST;
			result = ccs_generic_list_loop();
			generic_acl_copy(&ccs_gacl_list,
						ccs_gacl_list_count,
						ga, *count);
			*count = ccs_gacl_list_count;
		}
	}
	*dp = ccs_dp;
	return result;
}

int get_process_acl_list(int current,
				struct ccs_generic_acl **ga, int *count)
{
	int	result = 0;

	free(ccs_current_domain);
	ccs_current_pid = ccs_task_list[current].pid;
	ccs_current_domain = strdup(ccs_task_list[current].domain);
	if (!ccs_current_domain)
		ccs_out_of_memory();
	else {
		ccs_domain_sort_type = 1;
		ccs_current_screen = CCS_SCREEN_ACL_LIST;
		result = ccs_generic_list_loop();
		generic_acl_copy(&ccs_gacl_list,
					ccs_gacl_list_count,
					ga, *count);
		*count = ccs_gacl_list_count;
	}
	return result;
}

int get_optimize_acl_list(int current, struct ccs_generic_acl **ga, int count)
{
	ccs_current_screen = CCS_SCREEN_ACL_LIST;
	ccs_policy_file = CCS_PROC_POLICY_DOMAIN_POLICY;

	generic_acl_copy(ga, count, &ccs_gacl_list, ccs_gacl_list_count);

	ccs_list_item_count = ccs_gacl_list_count = count;
	ccs_editpolicy_optimize(current);

	generic_acl_copy(&ccs_gacl_list, ccs_gacl_list_count, ga, count);

	return 0;
}

int add_acl_list(struct ccs_domain_policy *dp, int current,
			char *input, char **err_buff)
{
	int	result = 0;

	ccs_dp = *dp;
	if (ccs_initializer_source(current) ||
		ccs_deleted_domain(current)) {
		*dp = ccs_dp;
		return 1;
	}

	ccs_current_screen = CCS_SCREEN_ACL_LIST;
	gpet_line = input;
	ccs_add_entry();
	gpet_line = NULL;
	*dp = ccs_dp;

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
	int	result;

	ccs_current_screen = CCS_SCREEN_EXCEPTION_LIST;
	result = ccs_generic_list_loop();
	*ga = ccs_gacl_list;
	*count = ccs_gacl_list_count;
	return result;
}

int add_exception_policy(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_EXCEPTION_LIST;
	gpet_line = input;
	ccs_add_entry();
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
	int	result;

	while (*count)
		free((void *)((*ga)[--(*count)].operand));

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	result = ccs_generic_list_loop();

	generic_acl_copy(&ccs_gacl_list,
				ccs_gacl_list_count,
				ga, *count);
	*count = ccs_gacl_list_count;

	return result;
}

int add_profile(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	gpet_line = input;
	ccs_add_entry();
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
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_PROFILE_LIST;
	ccs_gacl_list[index].selected = 1;
	gpet_line = (char *)input;
	ccs_set_level(0);
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
	struct ccs_generic_acl *orig_generic_acl_list = NULL;
	int		orig_generic_acl_list_count = 0;
	int		result;

	generic_acl_copy(&ccs_gacl_list,
				ccs_gacl_list_count,
			&orig_generic_acl_list,
				orig_generic_acl_list_count);
	orig_generic_acl_list_count = ccs_gacl_list_count;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	result = ccs_generic_list_loop();

	generic_acl_copy(&ccs_gacl_list,
				ccs_gacl_list_count,
				ga, *count);
	*count = ccs_gacl_list_count;

	generic_acl_copy(&orig_generic_acl_list,
				orig_generic_acl_list_count,
			&ccs_gacl_list,
				 ccs_gacl_list_count);
	ccs_gacl_list_count = orig_generic_acl_list_count;

	return result;
}

int add_manager(char *input, char **err_buff)
{
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	gpet_line = input;
	ccs_add_entry();
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
	struct ccs_generic_acl *orig_generic_acl_list = NULL;
	int		orig_generic_acl_list_count = 0;
	int		result;

	generic_acl_copy(&ccs_gacl_list,
				ccs_gacl_list_count,
			&orig_generic_acl_list,
				orig_generic_acl_list_count);
	orig_generic_acl_list_count = ccs_gacl_list_count;

	ccs_current_screen = CCS_SCREEN_STAT_LIST;
	result = ccs_generic_list_loop();

	generic_acl_copy(&ccs_gacl_list,
				ccs_gacl_list_count,
				ga, *count);
	*count = ccs_gacl_list_count;

	generic_acl_copy(&orig_generic_acl_list,
				orig_generic_acl_list_count,
			&ccs_gacl_list,
				 ccs_gacl_list_count);
	ccs_gacl_list_count = orig_generic_acl_list_count;

	return result;
}

int set_memory(struct ccs_generic_acl *ga, int count, char **err_buff)
{
	int	i, result = 0;

	ccs_current_screen = CCS_SCREEN_STAT_LIST;
	ccs_policy_file = CCS_PROC_POLICY_STAT;
	generic_acl_copy(&ga, count,
		&ccs_gacl_list, ccs_gacl_list_count);
	ccs_gacl_list_count = count;

	for (i = 0; i < count; i++) {
		ccs_gacl_list[i].selected = 0;
	}

	for (i = 0; i < count; i++) {
		char	*cp;
		cp = strrchr(ccs_gacl_list[i].operand, ':');
		if (cp && ga[i].selected) {
			gpet_line = strdup(++cp);
// fprintf(stderr, "mem:%2d[%s][%s]\n", i, ccs_gacl_list[i].operand, cp);
			ccs_gacl_list[i].selected = 1;
			ccs_set_quota(-1);
			ccs_gacl_list[i].selected = 0;
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
	ccs_dp = *dp;
	ccs_delete_entry(-1);
	*dp = ccs_dp;

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
		&ccs_gacl_list, ccs_gacl_list_count);
	ccs_gacl_list_count = count;
	ccs_dp = *dp;
	ccs_delete_entry(-1);
	*dp = ccs_dp;

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
	ccs_gacl_list = ga;
	ccs_gacl_list_count = count;
	ccs_dp = *dp;
	ccs_delete_entry(-1);
	*dp = ccs_dp;

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
	int	result = 0;

	ccs_current_screen = CCS_SCREEN_MANAGER_LIST;
	ccs_policy_file = CCS_PROC_POLICY_MANAGER;
	generic_acl_copy(&ga, count,
		&ccs_gacl_list, ccs_gacl_list_count);
	ccs_gacl_list_count = count;
	ccs_delete_entry(-1);

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

int is_network(void)
{
	return ccs_network_mode;
}

char *get_remote_ip(char *str_ip)
{
	const u32 ip = ntohl(ccs_network_ip);
	sprintf(str_ip, "%u.%u.%u.%u:%u",
			(u8) (ip >> 24), (u8) (ip >> 16),
			(u8) (ip >> 8), (u8) ip, ntohs(ccs_network_port));
	return str_ip;
}

const char *get_policy_dir(void)
{
	return ccs_policy_dir;
}

const char *get_domain_last_name(const int index)
{
	return ccs_get_last_name(index);
}
