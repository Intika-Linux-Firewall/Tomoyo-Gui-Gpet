/*
 * editpolicy_offline.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2011  NTT DATA CORPORATION
 *
 * Version: 1.8.2+   2011/07/13
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include "ccstools.h"
#include "editpolicy.h"
#include <poll.h>

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

#ifndef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#endif
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member);      \
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

static inline void __list_add(struct list_head *new, struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

/* Enumeration definition for internal use. */

/* Index numbers for Access Controls. */
enum ccs_acl_entry_type_index {
	CCS_TYPE_PATH_ACL,
	CCS_TYPE_PATH2_ACL,
	CCS_TYPE_PATH_NUMBER_ACL,
	CCS_TYPE_MKDEV_ACL,
	CCS_TYPE_MOUNT_ACL,
	CCS_TYPE_ENV_ACL,
	CCS_TYPE_CAPABILITY_ACL,
	CCS_TYPE_INET_ACL,
	CCS_TYPE_UNIX_ACL,
	CCS_TYPE_SIGNAL_ACL,
	CCS_TYPE_AUTO_EXECUTE_HANDLER,
	CCS_TYPE_DENIED_EXECUTE_HANDLER,
	CCS_TYPE_AUTO_TASK_ACL,
	CCS_TYPE_MANUAL_TASK_ACL,
};

/* Index numbers for Capability Controls. */
enum ccs_capability_acl_index {
	/* socket(PF_ROUTE, *, *)                                      */
	CCS_USE_ROUTE_SOCKET,
	/* socket(PF_PACKET, *, *)                                     */
	CCS_USE_PACKET_SOCKET,
	/* sys_reboot()                                                */
	CCS_SYS_REBOOT,
	/* sys_vhangup()                                               */
	CCS_SYS_VHANGUP,
	/* do_settimeofday(), sys_adjtimex()                           */
	CCS_SYS_SETTIME,
	/* sys_nice(), sys_setpriority()                               */
	CCS_SYS_NICE,
	/* sys_sethostname(), sys_setdomainname()                      */
	CCS_SYS_SETHOSTNAME,
	/* sys_create_module(), sys_init_module(), sys_delete_module() */
	CCS_USE_KERNEL_MODULE,
	/* sys_kexec_load()                                            */
	CCS_SYS_KEXEC_LOAD,
	/* sys_ptrace()                                                */
	CCS_SYS_PTRACE,
	CCS_MAX_CAPABILITY_INDEX
};

/* Index numbers for "struct ccs_condition". */
enum ccs_conditions_index {
	CCS_TASK_UID,             /* current_uid()   */
	CCS_TASK_EUID,            /* current_euid()  */
	CCS_TASK_SUID,            /* current_suid()  */
	CCS_TASK_FSUID,           /* current_fsuid() */
	CCS_TASK_GID,             /* current_gid()   */
	CCS_TASK_EGID,            /* current_egid()  */
	CCS_TASK_SGID,            /* current_sgid()  */
	CCS_TASK_FSGID,           /* current_fsgid() */
	CCS_TASK_PID,             /* sys_getpid()   */
	CCS_TASK_PPID,            /* sys_getppid()  */
	CCS_EXEC_ARGC,            /* "struct linux_binprm *"->argc */
	CCS_EXEC_ENVC,            /* "struct linux_binprm *"->envc */
	CCS_TYPE_IS_SOCKET,       /* S_IFSOCK */
	CCS_TYPE_IS_SYMLINK,      /* S_IFLNK */
	CCS_TYPE_IS_FILE,         /* S_IFREG */
	CCS_TYPE_IS_BLOCK_DEV,    /* S_IFBLK */
	CCS_TYPE_IS_DIRECTORY,    /* S_IFDIR */
	CCS_TYPE_IS_CHAR_DEV,     /* S_IFCHR */
	CCS_TYPE_IS_FIFO,         /* S_IFIFO */
	CCS_MODE_SETUID,          /* S_ISUID */
	CCS_MODE_SETGID,          /* S_ISGID */
	CCS_MODE_STICKY,          /* S_ISVTX */
	CCS_MODE_OWNER_READ,      /* S_IRUSR */
	CCS_MODE_OWNER_WRITE,     /* S_IWUSR */
	CCS_MODE_OWNER_EXECUTE,   /* S_IXUSR */
	CCS_MODE_GROUP_READ,      /* S_IRGRP */
	CCS_MODE_GROUP_WRITE,     /* S_IWGRP */
	CCS_MODE_GROUP_EXECUTE,   /* S_IXGRP */
	CCS_MODE_OTHERS_READ,     /* S_IROTH */
	CCS_MODE_OTHERS_WRITE,    /* S_IWOTH */
	CCS_MODE_OTHERS_EXECUTE,  /* S_IXOTH */
	CCS_TASK_TYPE,            /* ((u8) task->ccs_flags) &
				     CCS_TASK_IS_EXECUTE_HANDLER */
	CCS_TASK_EXECUTE_HANDLER, /* CCS_TASK_IS_EXECUTE_HANDLER */
	CCS_EXEC_REALPATH,
	CCS_SYMLINK_TARGET,
	CCS_PATH1_UID,
	CCS_PATH1_GID,
	CCS_PATH1_INO,
	CCS_PATH1_MAJOR,
	CCS_PATH1_MINOR,
	CCS_PATH1_PERM,
	CCS_PATH1_TYPE,
	CCS_PATH1_DEV_MAJOR,
	CCS_PATH1_DEV_MINOR,
	CCS_PATH2_UID,
	CCS_PATH2_GID,
	CCS_PATH2_INO,
	CCS_PATH2_MAJOR,
	CCS_PATH2_MINOR,
	CCS_PATH2_PERM,
	CCS_PATH2_TYPE,
	CCS_PATH2_DEV_MAJOR,
	CCS_PATH2_DEV_MINOR,
	CCS_PATH1_PARENT_UID,
	CCS_PATH1_PARENT_GID,
	CCS_PATH1_PARENT_INO,
	CCS_PATH1_PARENT_PERM,
	CCS_PATH2_PARENT_UID,
	CCS_PATH2_PARENT_GID,
	CCS_PATH2_PARENT_INO,
	CCS_PATH2_PARENT_PERM,
	CCS_MAX_CONDITION_KEYWORD,
	CCS_NUMBER_UNION,
	CCS_NAME_UNION,
	CCS_ARGV_ENTRY,
	CCS_ENVP_ENTRY,
};

/* Index numbers for domain's attributes. */
enum ccs_domain_info_flags_index {
	/* Quota warnning flag.   */
	CCS_DIF_QUOTA_WARNED,
	/*
	 * This domain was unable to create a new domain at
	 * ccs_find_next_domain() because the name of the domain to be created
	 * was too long or it could not allocate memory.
	 * More than one process continued execve() without domain transition.
	 */
	CCS_DIF_TRANSITION_FAILED,
	CCS_MAX_DOMAIN_INFO_FLAGS
};

/* Index numbers for audit type. */
enum ccs_grant_log {
	/* Follow profile's configuration. */
	CCS_GRANTLOG_AUTO,
	/* Do not generate grant log. */
	CCS_GRANTLOG_NO,
	/* Generate grant_log. */
	CCS_GRANTLOG_YES,
};

/* Index numbers for group entries. */
enum ccs_group_id {
	CCS_PATH_GROUP,
	CCS_NUMBER_GROUP,
	CCS_ADDRESS_GROUP,
	CCS_MAX_GROUP
};

/* Index numbers for category of functionality. */
enum ccs_mac_category_index {
	CCS_MAC_CATEGORY_FILE,
	CCS_MAC_CATEGORY_NETWORK,
	CCS_MAC_CATEGORY_MISC,
	CCS_MAC_CATEGORY_IPC,
	CCS_MAC_CATEGORY_CAPABILITY,
	CCS_MAX_MAC_CATEGORY_INDEX
};

/* Index numbers for functionality. */
enum ccs_mac_index {
	CCS_MAC_FILE_EXECUTE,
	CCS_MAC_FILE_OPEN,
	CCS_MAC_FILE_CREATE,
	CCS_MAC_FILE_UNLINK,
	CCS_MAC_FILE_GETATTR,
	CCS_MAC_FILE_MKDIR,
	CCS_MAC_FILE_RMDIR,
	CCS_MAC_FILE_MKFIFO,
	CCS_MAC_FILE_MKSOCK,
	CCS_MAC_FILE_TRUNCATE,
	CCS_MAC_FILE_SYMLINK,
	CCS_MAC_FILE_MKBLOCK,
	CCS_MAC_FILE_MKCHAR,
	CCS_MAC_FILE_LINK,
	CCS_MAC_FILE_RENAME,
	CCS_MAC_FILE_CHMOD,
	CCS_MAC_FILE_CHOWN,
	CCS_MAC_FILE_CHGRP,
	CCS_MAC_FILE_IOCTL,
	CCS_MAC_FILE_CHROOT,
	CCS_MAC_FILE_MOUNT,
	CCS_MAC_FILE_UMOUNT,
	CCS_MAC_FILE_PIVOT_ROOT,
	CCS_MAC_NETWORK_INET_STREAM_BIND,
	CCS_MAC_NETWORK_INET_STREAM_LISTEN,
	CCS_MAC_NETWORK_INET_STREAM_CONNECT,
	CCS_MAC_NETWORK_INET_STREAM_ACCEPT,
	CCS_MAC_NETWORK_INET_DGRAM_BIND,
	CCS_MAC_NETWORK_INET_DGRAM_SEND,
	CCS_MAC_NETWORK_INET_DGRAM_RECV,
	CCS_MAC_NETWORK_INET_RAW_BIND,
	CCS_MAC_NETWORK_INET_RAW_SEND,
	CCS_MAC_NETWORK_INET_RAW_RECV,
	CCS_MAC_NETWORK_UNIX_STREAM_BIND,
	CCS_MAC_NETWORK_UNIX_STREAM_LISTEN,
	CCS_MAC_NETWORK_UNIX_STREAM_CONNECT,
	CCS_MAC_NETWORK_UNIX_STREAM_ACCEPT,
	CCS_MAC_NETWORK_UNIX_DGRAM_BIND,
	CCS_MAC_NETWORK_UNIX_DGRAM_SEND,
	CCS_MAC_NETWORK_UNIX_DGRAM_RECV,
	CCS_MAC_NETWORK_UNIX_SEQPACKET_BIND,
	CCS_MAC_NETWORK_UNIX_SEQPACKET_LISTEN,
	CCS_MAC_NETWORK_UNIX_SEQPACKET_CONNECT,
	CCS_MAC_NETWORK_UNIX_SEQPACKET_ACCEPT,
	CCS_MAC_ENVIRON,
	CCS_MAC_SIGNAL,
	CCS_MAC_CAPABILITY_USE_ROUTE_SOCKET,
	CCS_MAC_CAPABILITY_USE_PACKET_SOCKET,
	CCS_MAC_CAPABILITY_SYS_REBOOT,
	CCS_MAC_CAPABILITY_SYS_VHANGUP,
	CCS_MAC_CAPABILITY_SYS_SETTIME,
	CCS_MAC_CAPABILITY_SYS_NICE,
	CCS_MAC_CAPABILITY_SYS_SETHOSTNAME,
	CCS_MAC_CAPABILITY_USE_KERNEL_MODULE,
	CCS_MAC_CAPABILITY_SYS_KEXEC_LOAD,
	CCS_MAC_CAPABILITY_SYS_PTRACE,
	CCS_MAX_MAC_INDEX
};

/* Index numbers for /proc/ccs/stat interface. */
enum ccs_memory_stat_type {
	CCS_MEMORY_POLICY,
	CCS_MEMORY_AUDIT,
	CCS_MEMORY_QUERY,
	CCS_MAX_MEMORY_STAT
};

/* Index numbers for access controls with one pathname and three numbers. */
enum ccs_mkdev_acl_index {
	CCS_TYPE_MKBLOCK,
	CCS_TYPE_MKCHAR,
	CCS_MAX_MKDEV_OPERATION
};

/* Index numbers for operation mode. */
enum ccs_mode_value {
	CCS_CONFIG_DISABLED,
	CCS_CONFIG_LEARNING,
	CCS_CONFIG_PERMISSIVE,
	CCS_CONFIG_ENFORCING,
	CCS_CONFIG_MAX_MODE,
	CCS_CONFIG_WANT_REJECT_LOG =  64,
	CCS_CONFIG_WANT_GRANT_LOG  = 128,
	CCS_CONFIG_USE_DEFAULT     = 255,
};

/* Index numbers for socket operations. */
enum ccs_network_acl_index {
	CCS_NETWORK_BIND,    /* bind() operation. */
	CCS_NETWORK_LISTEN,  /* listen() operation. */
	CCS_NETWORK_CONNECT, /* connect() operation. */
	CCS_NETWORK_ACCEPT,  /* accept() operation. */
	CCS_NETWORK_SEND,    /* send() operation. */
	CCS_NETWORK_RECV,    /* recv() operation. */
	CCS_MAX_NETWORK_OPERATION
};

/* Index numbers for access controls with two pathnames. */
enum ccs_path2_acl_index {
	CCS_TYPE_LINK,
	CCS_TYPE_RENAME,
	CCS_TYPE_PIVOT_ROOT,
	CCS_MAX_PATH2_OPERATION
};

/* Index numbers for access controls with one pathname. */
enum ccs_path_acl_index {
	CCS_TYPE_EXECUTE,
	CCS_TYPE_READ,
	CCS_TYPE_WRITE,
	CCS_TYPE_APPEND,
	CCS_TYPE_UNLINK,
	CCS_TYPE_GETATTR,
	CCS_TYPE_RMDIR,
	CCS_TYPE_TRUNCATE,
	CCS_TYPE_SYMLINK,
	CCS_TYPE_CHROOT,
	CCS_TYPE_UMOUNT,
	CCS_MAX_PATH_OPERATION
};

/* Index numbers for access controls with one pathname and one number. */
enum ccs_path_number_acl_index {
	CCS_TYPE_CREATE,
	CCS_TYPE_MKDIR,
	CCS_TYPE_MKFIFO,
	CCS_TYPE_MKSOCK,
	CCS_TYPE_IOCTL,
	CCS_TYPE_CHMOD,
	CCS_TYPE_CHOWN,
	CCS_TYPE_CHGRP,
	CCS_MAX_PATH_NUMBER_OPERATION
};

/* Index numbers for stat(). */
enum ccs_path_stat_index {
	/* Do not change this order. */
	CCS_PATH1,
	CCS_PATH1_PARENT,
	CCS_PATH2,
	CCS_PATH2_PARENT,
	CCS_MAX_PATH_STAT
};

/* Index numbers for /proc/ccs/stat interface. */
enum ccs_policy_stat_type {
	/* Do not change this order. */
	CCS_STAT_POLICY_UPDATES,
	CCS_STAT_POLICY_LEARNING,   /* == CCS_CONFIG_LEARNING */
	CCS_STAT_POLICY_PERMISSIVE, /* == CCS_CONFIG_PERMISSIVE */
	CCS_STAT_POLICY_ENFORCING,  /* == CCS_CONFIG_ENFORCING */
	CCS_MAX_POLICY_STAT
};

/* Index numbers for profile's PREFERENCE values. */
enum ccs_pref_index {
	CCS_PREF_MAX_AUDIT_LOG,
	CCS_PREF_MAX_LEARNING_ENTRY,
	CCS_PREF_ENFORCING_PENALTY,
	CCS_MAX_PREF
};

/* Index numbers for /proc/ccs/ interfaces. */
enum ccs_proc_interface_index {
	CCS_DOMAINPOLICY,
	CCS_EXCEPTIONPOLICY,
	CCS_PROCESS_STATUS,
	CCS_STAT,
	CCS_AUDIT,
	CCS_VERSION,
	CCS_PROFILE,
	CCS_QUERY,
	CCS_MANAGER,
	CCS_EXECUTE_HANDLER,
};

/* Index numbers for special mount operations. */
enum ccs_special_mount {
	CCS_MOUNT_BIND,            /* mount --bind /source /dest   */
	CCS_MOUNT_MOVE,            /* mount --move /old /new       */
	CCS_MOUNT_REMOUNT,         /* mount -o remount /dir        */
	CCS_MOUNT_MAKE_UNBINDABLE, /* mount --make-unbindable /dir */
	CCS_MOUNT_MAKE_PRIVATE,    /* mount --make-private /dir    */
	CCS_MOUNT_MAKE_SLAVE,      /* mount --make-slave /dir      */
	CCS_MOUNT_MAKE_SHARED,     /* mount --make-shared /dir     */
	CCS_MAX_SPECIAL_MOUNT
};

/* Index numbers for type of numeric values. */
enum ccs_value_type {
	CCS_VALUE_TYPE_INVALID,
	CCS_VALUE_TYPE_DECIMAL,
	CCS_VALUE_TYPE_OCTAL,
	CCS_VALUE_TYPE_HEXADECIMAL,
};

/* Constants definition for internal use. */

/*
 * TOMOYO uses this hash only when appending a string into the string table.
 * Frequency of appending strings is very low. So we don't need large (e.g.
 * 64k) hash size. 256 will be sufficient.
 */
#define CCS_HASH_BITS 8
#define CCS_MAX_HASH (1u << CCS_HASH_BITS)

/*
 * TOMOYO checks only SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_SEQPACKET.
 * Therefore, we don't need SOCK_MAX.
 */
#define CCS_SOCK_MAX 6

/* Size of temporary buffer for execve() operation. */
#define CCS_EXEC_TMPSIZE     4096

/* Profile number is an integer between 0 and 255. */
#define CCS_MAX_PROFILES 256

/* Group number is an integer between 0 and 255. */
#define CCS_MAX_ACL_GROUPS 256

/* Structure definition for internal use. */

struct ccs_policy_namespace;

/* Common header for holding ACL entries. */
struct ccs_acl_head {
	struct list_head list;
	bool is_deleted;
} __attribute__((__packed__));

/* Common header for shared entries. */
struct ccs_shared_acl_head {
	struct list_head list;
	unsigned int users;
} __attribute__((__packed__));

/* Common header for individual entries. */
struct ccs_acl_info {
	struct list_head list;
	struct ccs_condition *cond; /* Maybe NULL. */
	bool is_deleted;
	u8 type; /* One of values in "enum ccs_acl_entry_type_index". */
} __attribute__((__packed__));

/* Structure for holding a word. */
struct ccs_name_union {
	/* Either @filename or @group is NULL. */
	const struct ccs_path_info *filename;
	struct ccs_group *group;
};

/* Structure for holding a number. */
struct ccs_number_union {
	unsigned long values[2];
	struct ccs_group *group; /* Maybe NULL. */
	/* One of values in "enum ccs_value_type". */
	u8 value_type[2];
};

/* Structure for holding an IP address. */
struct ccs_ipaddr_union {
	struct in6_addr ip[2]; /* Big endian. */
	struct ccs_group *group; /* Pointer to address group. */
	bool is_ipv6; /* Valid only if @group == NULL. */
};

/* Structure for "path_group"/"number_group"/"address_group" directive. */
struct ccs_group {
	struct ccs_shared_acl_head head;
	struct ccs_policy_namespace *ns;
	/* Name of group (without leading '@'). */
	const struct ccs_path_info *group_name;
	/*
	 * List of "struct ccs_path_group" or "struct ccs_number_group" or
	 * "struct ccs_address_group".
	 */
	struct list_head member_list;
};

/* Structure for "path_group" directive. */
struct ccs_path_group {
	struct ccs_acl_head head;
	const struct ccs_path_info *member_name;
};

/* Structure for "number_group" directive. */
struct ccs_number_group {
	struct ccs_acl_head head;
	struct ccs_number_union number;
};

/* Structure for "address_group" directive. */
struct ccs_address_group {
	struct ccs_acl_head head;
	/* Structure for holding an IP address. */
	struct ccs_ipaddr_union address;
};

/* Structure for entries which follows "struct ccs_condition". */
struct ccs_condition_element {
	/*
	 * Left hand operand. A "struct ccs_argv" for CCS_ARGV_ENTRY, a
	 * "struct ccs_envp" for CCS_ENVP_ENTRY is attached to the tail
	 * of the array of this struct.
	 */
	u8 left;
	/*
	 * Right hand operand. A "struct ccs_number_union" for
	 * CCS_NUMBER_UNION, a "struct ccs_name_union" for CCS_NAME_UNION is
	 * attached to the tail of the array of this struct.
	 */
	u8 right;
	/* Equation operator. True if equals or overlaps, false otherwise. */
	bool equals;
};

/* Structure for optional arguments. */
struct ccs_condition {
	struct ccs_shared_acl_head head;
	u32 size; /* Memory size allocated for this entry. */
	u16 condc; /* Number of conditions in this struct. */
	u16 numbers_count; /* Number of "struct ccs_number_union values". */
	u16 names_count; /* Number of "struct ccs_name_union names". */
	u16 argc; /* Number of "struct ccs_argv". */
	u16 envc; /* Number of "struct ccs_envp". */
	u8 grant_log; /* One of values in "enum ccs_grant_log". */
	const struct ccs_path_info *transit; /* Maybe NULL. */
	/*
	 * struct ccs_condition_element condition[condc];
	 * struct ccs_number_union values[numbers_count];
	 * struct ccs_name_union names[names_count];
	 * struct ccs_argv argv[argc];
	 * struct ccs_envp envp[envc];
	 */
};

/*
 * Structure for "reset_domain"/"no_reset_domain"/"initialize_domain"/
 * "no_initialize_domain"/"keep_domain"/"no_keep_domain" keyword.
 */
struct ccs_transition_control {
	struct ccs_acl_head head;
	u8 type; /* One of values in "enum ccs_transition_type" */
	bool is_last_name; /* True if the domainname is ccs_last_word(). */
	const struct ccs_path_info *domainname; /* Maybe NULL */
	const struct ccs_path_info *program;    /* Maybe NULL */
	struct ccs_policy_namespace *ns;
};

/* Structure for "aggregator" keyword. */
struct ccs_aggregator {
	struct ccs_acl_head head;
	const struct ccs_path_info *original_name;
	const struct ccs_path_info *aggregated_name;
	struct ccs_policy_namespace *ns;
};

/* Structure for "deny_autobind" keyword. */
struct ccs_reserved {
	struct ccs_acl_head head;
	struct ccs_number_union port;
	struct ccs_policy_namespace *ns;
};

/* Structure for policy manager. */
struct ccs_manager {
	struct ccs_acl_head head;
	bool is_domain;  /* True if manager is a domainname. */
	/* A path to program or a domainname. */
	const struct ccs_path_info *manager;
};

/* Structure for argv[]. */
struct ccs_argv {
	unsigned long index;
	const struct ccs_path_info *value;
	bool is_not;
};

/* Structure for envp[]. */
struct ccs_envp {
	const struct ccs_path_info *name;
	const struct ccs_path_info *value;
	bool is_not;
};

/*
 * Structure for "task auto_execute_handler" and "task denied_execute_handler"
 * directive.
 *
 * If "task auto_execute_handler" directive exists and the current process is
 * not an execute handler, all execve() requests are replaced by execve()
 * requests of a program specified by "task auto_execute_handler" directive.
 * If the current process is an execute handler, "task auto_execute_handler"
 * and "task denied_execute_handler" directives are ignored.
 * The program specified by "task execute_handler" validates execve()
 * parameters and executes the original execve() requests if appropriate.
 *
 * "task denied_execute_handler" directive is used only when execve() request
 * was rejected in enforcing mode (i.e. CONFIG::file::execute={ mode=enforcing
 * }). The program specified by "task denied_execute_handler" does whatever it
 * wants to do (e.g. silently terminate, change firewall settings, redirect the
 * user to honey pot etc.).
 */
struct ccs_handler_acl {
	struct ccs_acl_info head;       /* type = CCS_TYPE_*_EXECUTE_HANDLER */
	const struct ccs_path_info *handler; /* Pointer to single pathname.  */
};

/*
 * Structure for "task auto_domain_transition" and
 * "task manual_domain_transition" directive.
 */
struct ccs_task_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_*_TASK_ACL */
	/* Pointer to domainname. */
	const struct ccs_path_info *domainname;
};

/*
 * Structure for "file execute", "file read", "file write", "file append",
 * "file unlink", "file getattr", "file rmdir", "file truncate",
 * "file symlink", "file chroot" and "file unmount" directive.
 */
struct ccs_path_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_PATH_ACL */
	u16 perm; /* Bitmask of values in "enum ccs_path_acl_index". */
	struct ccs_name_union name;
};

/*
 * Structure for "file rename", "file link" and "file pivot_root" directive.
 */
struct ccs_path2_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_PATH2_ACL */
	u8 perm; /* Bitmask of values in "enum ccs_path2_acl_index". */
	struct ccs_name_union name1;
	struct ccs_name_union name2;
};

/*
 * Structure for "file create", "file mkdir", "file mkfifo", "file mksock",
 * "file ioctl", "file chmod", "file chown" and "file chgrp" directive.
 */
struct ccs_path_number_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_PATH_NUMBER_ACL */
	u8 perm; /* Bitmask of values in "enum ccs_path_number_acl_index". */
	struct ccs_name_union name;
	struct ccs_number_union number;
};

/* Structure for "file mkblock" and "file mkchar" directive. */
struct ccs_mkdev_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_MKDEV_ACL */
	u8 perm; /* Bitmask of values in "enum ccs_mkdev_acl_index". */
	struct ccs_name_union name;
	struct ccs_number_union mode;
	struct ccs_number_union major;
	struct ccs_number_union minor;
};

/* Structure for "file mount" directive. */
struct ccs_mount_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_MOUNT_ACL */
	struct ccs_name_union dev_name;
	struct ccs_name_union dir_name;
	struct ccs_name_union fs_type;
	struct ccs_number_union flags;
};

/* Structure for "misc env" directive in domain policy. */
struct ccs_env_acl {
	struct ccs_acl_info head;        /* type = CCS_TYPE_ENV_ACL  */
	const struct ccs_path_info *env; /* environment variable */
};

/* Structure for "capability" directive. */
struct ccs_capability_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_CAPABILITY_ACL */
	u8 operation; /* One of values in "enum ccs_capability_acl_index". */
};

/* Structure for "ipc signal" directive. */
struct ccs_signal_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_SIGNAL_ACL */
	struct ccs_number_union sig;
	/* Pointer to destination pattern. */
	const struct ccs_path_info *domainname;
};

/* Structure for "network inet" directive. */
struct ccs_inet_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_INET_ACL */
	u8 protocol;
	u8 perm; /* Bitmask of values in "enum ccs_network_acl_index" */
	struct ccs_ipaddr_union address;
	struct ccs_number_union port;
};

/* Structure for "network unix" directive. */
struct ccs_unix_acl {
	struct ccs_acl_info head; /* type = CCS_TYPE_UNIX_ACL */
	u8 protocol;
	u8 perm; /* Bitmask of values in "enum ccs_network_acl_index" */
	struct ccs_name_union name;
};

/* Structure for holding string data. */
struct ccs_name {
	struct ccs_shared_acl_head head;
	int size; /* Memory size allocated for this entry. */
	struct ccs_path_info entry;
};

/* Structure for holding a line from /proc/ccs/ interface. */
struct ccs_acl_param {
	char *namespace;
	char *data; /* Unprocessed data. */
	struct list_head *list; /* List to add or remove. */
	struct ccs_policy_namespace *ns; /* Namespace to use. */
	bool is_delete; /* True if it is a delete request. */
};

/* Structure for /proc/ccs/profile interface. */
struct ccs_profile {
	const struct ccs_path_info *comment;
	u8 default_config;
	u8 config[CCS_MAX_MAC_INDEX + CCS_MAX_MAC_CATEGORY_INDEX];
	unsigned int pref[CCS_MAX_PREF];
};

/* Structure for representing YYYY/MM/DD hh/mm/ss. */
struct ccs_time {
	u16 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 min;
	u8 sec;
};

/* Structure for policy namespace. */
struct ccs_policy_namespace {
	/* Profile table. Memory is allocated as needed. */
	struct ccs_profile *profile_ptr[CCS_MAX_PROFILES];
	/* The global ACL referred by "use_group" keyword. */
	struct list_head acl_group[CCS_MAX_ACL_GROUPS];
	/* List for connecting to ccs_namespace_list list. */
	struct list_head namespace_list;
	/* Profile version. Currently only 20100903 is defined. */
	unsigned int profile_version;
	/* Name of this namespace (e.g. "<kernel>", "</usr/sbin/httpd>" ). */
	const char *name;
};

struct ccs_domain2_info {
	struct list_head list;
	struct list_head acl_info_list;
	/* Name of this domain. Never NULL.          */
	const struct ccs_path_info *domainname;
	u8 profile;        /* Profile number to use. */
	u8 group;          /* Group number to use.   */
	bool is_deleted;   /* Delete flag.           */
	bool flags[CCS_MAX_DOMAIN_INFO_FLAGS];
};

struct ccs_io_buffer {
	char *data;
	struct ccs_policy_namespace *ns;
	struct ccs_domain2_info *domain;
	struct ccs_domain2_info *print_this_domain_only;
	bool is_delete;
	bool print_transition_related_only;
	bool eof;
	bool reset;
	u8 type;
	u8 acl_group_index;
};

/* String table for operation mode. */
static const char * const ccs_mode[CCS_CONFIG_MAX_MODE] = {
	[CCS_CONFIG_DISABLED]   = "disabled",
	[CCS_CONFIG_LEARNING]   = "learning",
	[CCS_CONFIG_PERMISSIVE] = "permissive",
	[CCS_CONFIG_ENFORCING]  = "enforcing"
};

/* String table for /proc/ccs/profile interface. */
static const char * const ccs_mac_keywords[CCS_MAX_MAC_INDEX
					   + CCS_MAX_MAC_CATEGORY_INDEX] = {
	/* CONFIG::file group */
	[CCS_MAC_FILE_EXECUTE]    = "execute",
	[CCS_MAC_FILE_OPEN]       = "open",
	[CCS_MAC_FILE_CREATE]     = "create",
	[CCS_MAC_FILE_UNLINK]     = "unlink",
	[CCS_MAC_FILE_GETATTR]    = "getattr",
	[CCS_MAC_FILE_MKDIR]      = "mkdir",
	[CCS_MAC_FILE_RMDIR]      = "rmdir",
	[CCS_MAC_FILE_MKFIFO]     = "mkfifo",
	[CCS_MAC_FILE_MKSOCK]     = "mksock",
	[CCS_MAC_FILE_TRUNCATE]   = "truncate",
	[CCS_MAC_FILE_SYMLINK]    = "symlink",
	[CCS_MAC_FILE_MKBLOCK]    = "mkblock",
	[CCS_MAC_FILE_MKCHAR]     = "mkchar",
	[CCS_MAC_FILE_LINK]       = "link",
	[CCS_MAC_FILE_RENAME]     = "rename",
	[CCS_MAC_FILE_CHMOD]      = "chmod",
	[CCS_MAC_FILE_CHOWN]      = "chown",
	[CCS_MAC_FILE_CHGRP]      = "chgrp",
	[CCS_MAC_FILE_IOCTL]      = "ioctl",
	[CCS_MAC_FILE_CHROOT]     = "chroot",
	[CCS_MAC_FILE_MOUNT]      = "mount",
	[CCS_MAC_FILE_UMOUNT]     = "unmount",
	[CCS_MAC_FILE_PIVOT_ROOT] = "pivot_root",
	/* CONFIG::misc group */
	[CCS_MAC_ENVIRON] = "env",
	/* CONFIG::network group */
	[CCS_MAC_NETWORK_INET_STREAM_BIND]       = "inet_stream_bind",
	[CCS_MAC_NETWORK_INET_STREAM_LISTEN]     = "inet_stream_listen",
	[CCS_MAC_NETWORK_INET_STREAM_CONNECT]    = "inet_stream_connect",
	[CCS_MAC_NETWORK_INET_STREAM_ACCEPT]     = "inet_stream_accept",
	[CCS_MAC_NETWORK_INET_DGRAM_BIND]        = "inet_dgram_bind",
	[CCS_MAC_NETWORK_INET_DGRAM_SEND]        = "inet_dgram_send",
	[CCS_MAC_NETWORK_INET_DGRAM_RECV]        = "inet_dgram_recv",
	[CCS_MAC_NETWORK_INET_RAW_BIND]          = "inet_raw_bind",
	[CCS_MAC_NETWORK_INET_RAW_SEND]          = "inet_raw_send",
	[CCS_MAC_NETWORK_INET_RAW_RECV]          = "inet_raw_recv",
	[CCS_MAC_NETWORK_UNIX_STREAM_BIND]       = "unix_stream_bind",
	[CCS_MAC_NETWORK_UNIX_STREAM_LISTEN]     = "unix_stream_listen",
	[CCS_MAC_NETWORK_UNIX_STREAM_CONNECT]    = "unix_stream_connect",
	[CCS_MAC_NETWORK_UNIX_STREAM_ACCEPT]     = "unix_stream_accept",
	[CCS_MAC_NETWORK_UNIX_DGRAM_BIND]        = "unix_dgram_bind",
	[CCS_MAC_NETWORK_UNIX_DGRAM_SEND]        = "unix_dgram_send",
	[CCS_MAC_NETWORK_UNIX_DGRAM_RECV]        = "unix_dgram_recv",
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_BIND]    = "unix_seqpacket_bind",
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_LISTEN]  = "unix_seqpacket_listen",
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_CONNECT] = "unix_seqpacket_connect",
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_ACCEPT]  = "unix_seqpacket_accept",
	/* CONFIG::ipc group */
	[CCS_MAC_SIGNAL] = "signal",
	/* CONFIG::capability group */
	[CCS_MAC_CAPABILITY_USE_ROUTE_SOCKET]  = "use_route",
	[CCS_MAC_CAPABILITY_USE_PACKET_SOCKET] = "use_packet",
	[CCS_MAC_CAPABILITY_SYS_REBOOT]        = "SYS_REBOOT",
	[CCS_MAC_CAPABILITY_SYS_VHANGUP]       = "SYS_VHANGUP",
	[CCS_MAC_CAPABILITY_SYS_SETTIME]       = "SYS_TIME",
	[CCS_MAC_CAPABILITY_SYS_NICE]          = "SYS_NICE",
	[CCS_MAC_CAPABILITY_SYS_SETHOSTNAME]   = "SYS_SETHOSTNAME",
	[CCS_MAC_CAPABILITY_USE_KERNEL_MODULE] = "use_kernel_module",
	[CCS_MAC_CAPABILITY_SYS_KEXEC_LOAD]    = "SYS_KEXEC_LOAD",
	[CCS_MAC_CAPABILITY_SYS_PTRACE]        = "SYS_PTRACE",
	/* CONFIG group */
	[CCS_MAX_MAC_INDEX + CCS_MAC_CATEGORY_FILE]       = "file",
	[CCS_MAX_MAC_INDEX + CCS_MAC_CATEGORY_NETWORK]    = "network",
	[CCS_MAX_MAC_INDEX + CCS_MAC_CATEGORY_MISC]       = "misc",
	[CCS_MAX_MAC_INDEX + CCS_MAC_CATEGORY_IPC]        = "ipc",
	[CCS_MAX_MAC_INDEX + CCS_MAC_CATEGORY_CAPABILITY] = "capability",
};

/* String table for path operation. */
static const char * const ccs_path_keyword[CCS_MAX_PATH_OPERATION] = {
	[CCS_TYPE_EXECUTE]    = "execute",
	[CCS_TYPE_READ]       = "read",
	[CCS_TYPE_WRITE]      = "write",
	[CCS_TYPE_APPEND]     = "append",
	[CCS_TYPE_UNLINK]     = "unlink",
	[CCS_TYPE_GETATTR]    = "getattr",
	[CCS_TYPE_RMDIR]      = "rmdir",
	[CCS_TYPE_TRUNCATE]   = "truncate",
	[CCS_TYPE_SYMLINK]    = "symlink",
	[CCS_TYPE_CHROOT]     = "chroot",
	[CCS_TYPE_UMOUNT]     = "unmount",
};

/* String table for socket's operation. */
static const char * const ccs_socket_keyword[CCS_MAX_NETWORK_OPERATION] = {
	[CCS_NETWORK_BIND]    = "bind",
	[CCS_NETWORK_LISTEN]  = "listen",
	[CCS_NETWORK_CONNECT] = "connect",
	[CCS_NETWORK_ACCEPT]  = "accept",
	[CCS_NETWORK_SEND]    = "send",
	[CCS_NETWORK_RECV]    = "recv",
};

/* String table for categories. */
static const char * const ccs_category_keywords[CCS_MAX_MAC_CATEGORY_INDEX] = {
	[CCS_MAC_CATEGORY_FILE]       = "file",
	[CCS_MAC_CATEGORY_NETWORK]    = "network",
	[CCS_MAC_CATEGORY_MISC]       = "misc",
	[CCS_MAC_CATEGORY_IPC]        = "ipc",
	[CCS_MAC_CATEGORY_CAPABILITY] = "capability",
};

/* String table for conditions. */
static const char * const ccs_condition_keyword[CCS_MAX_CONDITION_KEYWORD] = {
	[CCS_TASK_UID]             = "task.uid",
	[CCS_TASK_EUID]            = "task.euid",
	[CCS_TASK_SUID]            = "task.suid",
	[CCS_TASK_FSUID]           = "task.fsuid",
	[CCS_TASK_GID]             = "task.gid",
	[CCS_TASK_EGID]            = "task.egid",
	[CCS_TASK_SGID]            = "task.sgid",
	[CCS_TASK_FSGID]           = "task.fsgid",
	[CCS_TASK_PID]             = "task.pid",
	[CCS_TASK_PPID]            = "task.ppid",
	[CCS_EXEC_ARGC]            = "exec.argc",
	[CCS_EXEC_ENVC]            = "exec.envc",
	[CCS_TYPE_IS_SOCKET]       = "socket",
	[CCS_TYPE_IS_SYMLINK]      = "symlink",
	[CCS_TYPE_IS_FILE]         = "file",
	[CCS_TYPE_IS_BLOCK_DEV]    = "block",
	[CCS_TYPE_IS_DIRECTORY]    = "directory",
	[CCS_TYPE_IS_CHAR_DEV]     = "char",
	[CCS_TYPE_IS_FIFO]         = "fifo",
	[CCS_MODE_SETUID]          = "setuid",
	[CCS_MODE_SETGID]          = "setgid",
	[CCS_MODE_STICKY]          = "sticky",
	[CCS_MODE_OWNER_READ]      = "owner_read",
	[CCS_MODE_OWNER_WRITE]     = "owner_write",
	[CCS_MODE_OWNER_EXECUTE]   = "owner_execute",
	[CCS_MODE_GROUP_READ]      = "group_read",
	[CCS_MODE_GROUP_WRITE]     = "group_write",
	[CCS_MODE_GROUP_EXECUTE]   = "group_execute",
	[CCS_MODE_OTHERS_READ]     = "others_read",
	[CCS_MODE_OTHERS_WRITE]    = "others_write",
	[CCS_MODE_OTHERS_EXECUTE]  = "others_execute",
	[CCS_TASK_TYPE]            = "task.type",
	[CCS_TASK_EXECUTE_HANDLER] = "execute_handler",
	[CCS_EXEC_REALPATH]        = "exec.realpath",
	[CCS_SYMLINK_TARGET]       = "symlink.target",
	[CCS_PATH1_UID]            = "path1.uid",
	[CCS_PATH1_GID]            = "path1.gid",
	[CCS_PATH1_INO]            = "path1.ino",
	[CCS_PATH1_MAJOR]          = "path1.major",
	[CCS_PATH1_MINOR]          = "path1.minor",
	[CCS_PATH1_PERM]           = "path1.perm",
	[CCS_PATH1_TYPE]           = "path1.type",
	[CCS_PATH1_DEV_MAJOR]      = "path1.dev_major",
	[CCS_PATH1_DEV_MINOR]      = "path1.dev_minor",
	[CCS_PATH2_UID]            = "path2.uid",
	[CCS_PATH2_GID]            = "path2.gid",
	[CCS_PATH2_INO]            = "path2.ino",
	[CCS_PATH2_MAJOR]          = "path2.major",
	[CCS_PATH2_MINOR]          = "path2.minor",
	[CCS_PATH2_PERM]           = "path2.perm",
	[CCS_PATH2_TYPE]           = "path2.type",
	[CCS_PATH2_DEV_MAJOR]      = "path2.dev_major",
	[CCS_PATH2_DEV_MINOR]      = "path2.dev_minor",
	[CCS_PATH1_PARENT_UID]     = "path1.parent.uid",
	[CCS_PATH1_PARENT_GID]     = "path1.parent.gid",
	[CCS_PATH1_PARENT_INO]     = "path1.parent.ino",
	[CCS_PATH1_PARENT_PERM]    = "path1.parent.perm",
	[CCS_PATH2_PARENT_UID]     = "path2.parent.uid",
	[CCS_PATH2_PARENT_GID]     = "path2.parent.gid",
	[CCS_PATH2_PARENT_INO]     = "path2.parent.ino",
	[CCS_PATH2_PARENT_PERM]    = "path2.parent.perm",
};

/* String table for PREFERENCE keyword. */
static const char * const ccs_pref_keywords[CCS_MAX_PREF] = {
	[CCS_PREF_MAX_AUDIT_LOG]      = "max_audit_log",
	[CCS_PREF_MAX_LEARNING_ENTRY] = "max_learning_entry",
	[CCS_PREF_ENFORCING_PENALTY]  = "enforcing_penalty",
};

/* Mapping table from "enum ccs_path_acl_index" to "enum ccs_mac_index". */
static const u8 ccs_p2mac[CCS_MAX_PATH_OPERATION] = {
	[CCS_TYPE_EXECUTE]    = CCS_MAC_FILE_EXECUTE,
	[CCS_TYPE_READ]       = CCS_MAC_FILE_OPEN,
	[CCS_TYPE_WRITE]      = CCS_MAC_FILE_OPEN,
	[CCS_TYPE_APPEND]     = CCS_MAC_FILE_OPEN,
	[CCS_TYPE_UNLINK]     = CCS_MAC_FILE_UNLINK,
	[CCS_TYPE_GETATTR]    = CCS_MAC_FILE_GETATTR,
	[CCS_TYPE_RMDIR]      = CCS_MAC_FILE_RMDIR,
	[CCS_TYPE_TRUNCATE]   = CCS_MAC_FILE_TRUNCATE,
	[CCS_TYPE_SYMLINK]    = CCS_MAC_FILE_SYMLINK,
	[CCS_TYPE_CHROOT]     = CCS_MAC_FILE_CHROOT,
	[CCS_TYPE_UMOUNT]     = CCS_MAC_FILE_UMOUNT,
};

/* Mapping table from "enum ccs_mkdev_acl_index" to "enum ccs_mac_index". */
static const u8 ccs_pnnn2mac[CCS_MAX_MKDEV_OPERATION] = {
	[CCS_TYPE_MKBLOCK] = CCS_MAC_FILE_MKBLOCK,
	[CCS_TYPE_MKCHAR]  = CCS_MAC_FILE_MKCHAR,
};

/* Mapping table from "enum ccs_path2_acl_index" to "enum ccs_mac_index". */
static const u8 ccs_pp2mac[CCS_MAX_PATH2_OPERATION] = {
	[CCS_TYPE_LINK]       = CCS_MAC_FILE_LINK,
	[CCS_TYPE_RENAME]     = CCS_MAC_FILE_RENAME,
	[CCS_TYPE_PIVOT_ROOT] = CCS_MAC_FILE_PIVOT_ROOT,
};

/*
 * Mapping table from "enum ccs_path_number_acl_index" to "enum ccs_mac_index".
 */
static const u8 ccs_pn2mac[CCS_MAX_PATH_NUMBER_OPERATION] = {
	[CCS_TYPE_CREATE] = CCS_MAC_FILE_CREATE,
	[CCS_TYPE_MKDIR]  = CCS_MAC_FILE_MKDIR,
	[CCS_TYPE_MKFIFO] = CCS_MAC_FILE_MKFIFO,
	[CCS_TYPE_MKSOCK] = CCS_MAC_FILE_MKSOCK,
	[CCS_TYPE_IOCTL]  = CCS_MAC_FILE_IOCTL,
	[CCS_TYPE_CHMOD]  = CCS_MAC_FILE_CHMOD,
	[CCS_TYPE_CHOWN]  = CCS_MAC_FILE_CHOWN,
	[CCS_TYPE_CHGRP]  = CCS_MAC_FILE_CHGRP,
};

/*
 * Mapping table from "enum ccs_network_acl_index" to "enum ccs_mac_index" for
 * inet domain socket.
 */
static const u8 ccs_inet2mac[CCS_SOCK_MAX][CCS_MAX_NETWORK_OPERATION] = {
	[SOCK_STREAM] = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_INET_STREAM_BIND,
		[CCS_NETWORK_LISTEN]  = CCS_MAC_NETWORK_INET_STREAM_LISTEN,
		[CCS_NETWORK_CONNECT] = CCS_MAC_NETWORK_INET_STREAM_CONNECT,
		[CCS_NETWORK_ACCEPT]  = CCS_MAC_NETWORK_INET_STREAM_ACCEPT,
	},
	[SOCK_DGRAM] = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_INET_DGRAM_BIND,
		[CCS_NETWORK_SEND]    = CCS_MAC_NETWORK_INET_DGRAM_SEND,
		[CCS_NETWORK_RECV]    = CCS_MAC_NETWORK_INET_DGRAM_RECV,
	},
	[SOCK_RAW]    = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_INET_RAW_BIND,
		[CCS_NETWORK_SEND]    = CCS_MAC_NETWORK_INET_RAW_SEND,
		[CCS_NETWORK_RECV]    = CCS_MAC_NETWORK_INET_RAW_RECV,
	},
};

/*
 * Mapping table from "enum ccs_network_acl_index" to "enum ccs_mac_index" for
 * unix domain socket.
 */
static const u8 ccs_unix2mac[CCS_SOCK_MAX][CCS_MAX_NETWORK_OPERATION] = {
	[SOCK_STREAM] = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_UNIX_STREAM_BIND,
		[CCS_NETWORK_LISTEN]  = CCS_MAC_NETWORK_UNIX_STREAM_LISTEN,
		[CCS_NETWORK_CONNECT] = CCS_MAC_NETWORK_UNIX_STREAM_CONNECT,
		[CCS_NETWORK_ACCEPT]  = CCS_MAC_NETWORK_UNIX_STREAM_ACCEPT,
	},
	[SOCK_DGRAM] = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_UNIX_DGRAM_BIND,
		[CCS_NETWORK_SEND]    = CCS_MAC_NETWORK_UNIX_DGRAM_SEND,
		[CCS_NETWORK_RECV]    = CCS_MAC_NETWORK_UNIX_DGRAM_RECV,
	},
	[SOCK_SEQPACKET] = {
		[CCS_NETWORK_BIND]    = CCS_MAC_NETWORK_UNIX_SEQPACKET_BIND,
		[CCS_NETWORK_LISTEN]  = CCS_MAC_NETWORK_UNIX_SEQPACKET_LISTEN,
		[CCS_NETWORK_CONNECT] = CCS_MAC_NETWORK_UNIX_SEQPACKET_CONNECT,
		[CCS_NETWORK_ACCEPT]  = CCS_MAC_NETWORK_UNIX_SEQPACKET_ACCEPT,
	},
};

/* String table for socket's protocols. */
static const char * const ccs_proto_keyword[CCS_SOCK_MAX] = {
	[SOCK_STREAM]    = "stream",
	[SOCK_DGRAM]     = "dgram",
	[SOCK_RAW]       = "raw",
	[SOCK_SEQPACKET] = "seqpacket",
	[0] = " ", /* Dummy for avoiding NULL pointer dereference. */
	[4] = " ", /* Dummy for avoiding NULL pointer dereference. */
};

/*
 * Mapping table from "enum ccs_capability_acl_index" to "enum ccs_mac_index".
 */
static const u8 ccs_c2mac[CCS_MAX_CAPABILITY_INDEX] = {
	[CCS_USE_ROUTE_SOCKET]  = CCS_MAC_CAPABILITY_USE_ROUTE_SOCKET,
	[CCS_USE_PACKET_SOCKET] = CCS_MAC_CAPABILITY_USE_PACKET_SOCKET,
	[CCS_SYS_REBOOT]        = CCS_MAC_CAPABILITY_SYS_REBOOT,
	[CCS_SYS_VHANGUP]       = CCS_MAC_CAPABILITY_SYS_VHANGUP,
	[CCS_SYS_SETTIME]       = CCS_MAC_CAPABILITY_SYS_SETTIME,
	[CCS_SYS_NICE]          = CCS_MAC_CAPABILITY_SYS_NICE,
	[CCS_SYS_SETHOSTNAME]   = CCS_MAC_CAPABILITY_SYS_SETHOSTNAME,
	[CCS_USE_KERNEL_MODULE] = CCS_MAC_CAPABILITY_USE_KERNEL_MODULE,
	[CCS_SYS_KEXEC_LOAD]    = CCS_MAC_CAPABILITY_SYS_KEXEC_LOAD,
	[CCS_SYS_PTRACE]        = CCS_MAC_CAPABILITY_SYS_PTRACE,
};

/* String table for /proc/ccs/stat interface. */
static const char * const ccs_memory_headers[CCS_MAX_MEMORY_STAT] = {
	[CCS_MEMORY_POLICY]     = "policy:",
	[CCS_MEMORY_AUDIT]      = "audit log:",
	[CCS_MEMORY_QUERY]      = "query message:",
};

/* String table for domain transition control keywords. */
static const char * const ccs_transition_type[CCS_MAX_TRANSITION_TYPE] = {
	[CCS_TRANSITION_CONTROL_NO_RESET]      = "no_reset_domain ",
	[CCS_TRANSITION_CONTROL_RESET]         = "reset_domain ",
	[CCS_TRANSITION_CONTROL_NO_INITIALIZE] = "no_initialize_domain ",
	[CCS_TRANSITION_CONTROL_INITIALIZE]    = "initialize_domain ",
	[CCS_TRANSITION_CONTROL_NO_KEEP]       = "no_keep_domain ",
	[CCS_TRANSITION_CONTROL_KEEP]          = "keep_domain ",
};

/* String table for grouping keywords. */
static const char * const ccs_group_name[CCS_MAX_GROUP] = {
	[CCS_PATH_GROUP]    = "path_group ",
	[CCS_NUMBER_GROUP]  = "number_group ",
	[CCS_ADDRESS_GROUP] = "address_group ",
};

/* String table for domain flags. */
static const char * const ccs_dif[CCS_MAX_DOMAIN_INFO_FLAGS] = {
	[CCS_DIF_QUOTA_WARNED]      = "quota_exceeded\n",
	[CCS_DIF_TRANSITION_FAILED] = "transition_failed\n",
};

/* Mapping table from "enum ccs_mac_index" to "enum ccs_mac_category_index". */
static const u8 ccs_index2category[CCS_MAX_MAC_INDEX] = {
	/* CONFIG::file group */
	[CCS_MAC_FILE_EXECUTE]    = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_OPEN]       = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_CREATE]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_UNLINK]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_GETATTR]    = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MKDIR]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_RMDIR]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MKFIFO]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MKSOCK]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_TRUNCATE]   = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_SYMLINK]    = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MKBLOCK]    = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MKCHAR]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_LINK]       = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_RENAME]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_CHMOD]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_CHOWN]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_CHGRP]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_IOCTL]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_CHROOT]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_MOUNT]      = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_UMOUNT]     = CCS_MAC_CATEGORY_FILE,
	[CCS_MAC_FILE_PIVOT_ROOT] = CCS_MAC_CATEGORY_FILE,
	/* CONFIG::misc group */
	[CCS_MAC_ENVIRON]         = CCS_MAC_CATEGORY_MISC,
	/* CONFIG::network group */
	[CCS_MAC_NETWORK_INET_STREAM_BIND]       = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_STREAM_LISTEN]     = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_STREAM_CONNECT]    = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_STREAM_ACCEPT]     = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_DGRAM_BIND]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_DGRAM_SEND]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_DGRAM_RECV]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_RAW_BIND]          = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_RAW_SEND]          = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_INET_RAW_RECV]          = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_STREAM_BIND]       = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_STREAM_LISTEN]     = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_STREAM_CONNECT]    = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_STREAM_ACCEPT]     = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_DGRAM_BIND]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_DGRAM_SEND]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_DGRAM_RECV]        = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_BIND]    = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_LISTEN]  = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_CONNECT] = CCS_MAC_CATEGORY_NETWORK,
	[CCS_MAC_NETWORK_UNIX_SEQPACKET_ACCEPT]  = CCS_MAC_CATEGORY_NETWORK,
	/* CONFIG::ipc group */
	[CCS_MAC_SIGNAL]          = CCS_MAC_CATEGORY_IPC,
	/* CONFIG::capability group */
	[CCS_MAC_CAPABILITY_USE_ROUTE_SOCKET]  = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_USE_PACKET_SOCKET] = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_REBOOT]        = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_VHANGUP]       = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_SETTIME]       = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_NICE]          = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_SETHOSTNAME]   = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_USE_KERNEL_MODULE] = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_KEXEC_LOAD]    = CCS_MAC_CATEGORY_CAPABILITY,
	[CCS_MAC_CAPABILITY_SYS_PTRACE]        = CCS_MAC_CATEGORY_CAPABILITY,
};

static struct ccs_io_buffer head;
static struct ccs_domain2_info ccs_kernel_domain;
static struct ccs_policy_namespace ccs_kernel_namespace;
static LIST_HEAD(ccs_domain_list);
static LIST_HEAD(ccs_manager_list);
static LIST_HEAD(ccs_path_group);
static LIST_HEAD(ccs_number_group);
static LIST_HEAD(ccs_address_group);
static LIST_HEAD(ccs_transition_list);
static LIST_HEAD(ccs_aggregator_list);
static LIST_HEAD(ccs_reserved_list);
static LIST_HEAD(ccs_namespace_list);
static bool ccs_namespace_enabled;
static struct list_head ccs_name_list[CCS_MAX_HASH];
static unsigned int ccs_memory_quota[CCS_MAX_MEMORY_STAT];

/**
 * ccs_put_condition - Drop reference on "struct ccs_condition".
 *
 * @cond: Pointer to "struct ccs_condition". Maybe NULL.
 *
 * Returns nothing.
 */
static inline void ccs_put_condition(struct ccs_condition *cond)
{
	if (cond)
		cond->head.users--;
}

/**
 * ccs_put_group - Drop reference on "struct ccs_group".
 *
 * @group: Pointer to "struct ccs_group". Maybe NULL.
 *
 * Returns nothing.
 */
static inline void ccs_put_group(struct ccs_group *group)
{
	if (group)
		group->head.users--;
}

/**
 * ccs_put_name - Drop reference on "struct ccs_name".
 *
 * @name: Pointer to "struct ccs_path_info". Maybe NULL.
 *
 * Returns nothing.
 */
static inline void ccs_put_name(const struct ccs_path_info *name)
{
	if (name)
		container_of(name, struct ccs_name, entry)->head.users--;
}

/**
 * ccs_put_name_union - Drop reference on "struct ccs_name_union".
 *
 * @ptr: Pointer to "struct ccs_name_union".
 *
 * Returns nothing.
 */
static void ccs_put_name_union(struct ccs_name_union *ptr)
{
	ccs_put_group(ptr->group);
	ccs_put_name(ptr->filename);
}

/**
 * ccs_put_number_union - Drop reference on "struct ccs_number_union".
 *
 * @ptr: Pointer to "struct ccs_number_union".
 *
 * Returns nothing.
 */
static void ccs_put_number_union(struct ccs_number_union *ptr)
{
	ccs_put_group(ptr->group);
}

/**
 * ccs_del_condition - Delete members in "struct ccs_condition".
 *
 * @element: Pointer to "struct list_head".
 *
 * Returns nothing.
 */
static void ccs_del_condition(struct list_head *element)
{
	struct ccs_condition *cond = container_of(element, typeof(*cond),
						  head.list);
	const u16 condc = cond->condc;
	const u16 numbers_count = cond->numbers_count;
	const u16 names_count = cond->names_count;
	const u16 argc = cond->argc;
	const u16 envc = cond->envc;
	unsigned int i;
	const struct ccs_condition_element *condp
		= (const struct ccs_condition_element *) (cond + 1);
	struct ccs_number_union *numbers_p
		= (struct ccs_number_union *) (condp + condc);
	struct ccs_name_union *names_p
		= (struct ccs_name_union *) (numbers_p + numbers_count);
	const struct ccs_argv *argv
		= (const struct ccs_argv *) (names_p + names_count);
	const struct ccs_envp *envp
		= (const struct ccs_envp *) (argv + argc);
	for (i = 0; i < numbers_count; i++)
		ccs_put_number_union(numbers_p++);
	for (i = 0; i < names_count; i++)
		ccs_put_name_union(names_p++);
	for (i = 0; i < argc; argv++, i++)
		ccs_put_name(argv->value);
	for (i = 0; i < envc; envp++, i++) {
		ccs_put_name(envp->name);
		ccs_put_name(envp->value);
	}
	ccs_put_name(cond->transit);
}

/**
 * ccs_yesno - Return "yes" or "no".
 *
 * @value: Bool value.
 *
 * Returns "yes" if @value is not 0, "no" otherwise.
 */
static const char *ccs_yesno(const unsigned int value)
{
	return value ? "yes" : "no";
}

/**
 * ccs_same_name_union - Check for duplicated "struct ccs_name_union" entry.
 *
 * @a: Pointer to "struct ccs_name_union".
 * @b: Pointer to "struct ccs_name_union".
 *
 * Returns true if @a == @b, false otherwise.
 */
static inline bool ccs_same_name_union(const struct ccs_name_union *a,
				       const struct ccs_name_union *b)
{
	return a->filename == b->filename && a->group == b->group;
}

/**
 * ccs_same_number_union - Check for duplicated "struct ccs_number_union" entry.
 *
 * @a: Pointer to "struct ccs_number_union".
 * @b: Pointer to "struct ccs_number_union".
 *
 * Returns true if @a == @b, false otherwise.
 */
static inline bool ccs_same_number_union(const struct ccs_number_union *a,
					 const struct ccs_number_union *b)
{
	return a->values[0] == b->values[0] && a->values[1] == b->values[1] &&
		a->group == b->group && a->value_type[0] == b->value_type[0] &&
		a->value_type[1] == b->value_type[1];
}

/**
 * ccs_same_ipaddr_union - Check for duplicated "struct ccs_ipaddr_union" entry.
 *
 * @a: Pointer to "struct ccs_ipaddr_union".
 * @b: Pointer to "struct ccs_ipaddr_union".
 *
 * Returns true if @a == @b, false otherwise.
 */
static inline bool ccs_same_ipaddr_union(const struct ccs_ipaddr_union *a,
					 const struct ccs_ipaddr_union *b)
{
	return !memcmp(a->ip, b->ip, sizeof(a->ip)) && a->group == b->group &&
		a->is_ipv6 == b->is_ipv6;
}

/**
 * ccs_partial_name_hash - Hash name.
 *
 * @c:        A unsigned long value.
 * @prevhash: A previous hash value.
 *
 * Returns new hash value.
 *
 * This function is copied from partial_name_hash() in the kernel source.
 */
static inline unsigned long ccs_partial_name_hash(unsigned long c,
						  unsigned long prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/**
 * ccs_full_name_hash - Hash full name.
 *
 * @name: Pointer to "const unsigned char".
 * @len:  Length of @name in byte.
 *
 * Returns hash value.
 *
 * This function is copied from full_name_hash() in the kernel source.
 */
static inline unsigned int ccs_full_name_hash(const unsigned char *name,
					      unsigned int len)
{
	unsigned long hash = 0;
	while (len--)
		hash = ccs_partial_name_hash(*name++, hash);
	return (unsigned int) hash;
}

/**
 * ccs_get_name - Allocate memory for string data.
 *
 * @name: The string to store into the permernent memory.
 *
 * Returns pointer to "struct ccs_path_info" on success, abort otherwise.
 */
static const struct ccs_path_info *ccs_get_name(const char *name)
{
	struct ccs_name *ptr;
	unsigned int hash;
	int len;
	int allocated_len;
	struct list_head *head;

	if (!name)
		name = "";
	len = strlen(name) + 1;
	hash = ccs_full_name_hash((const unsigned char *) name, len - 1);
	head = &ccs_name_list[hash % CCS_MAX_HASH];
	list_for_each_entry(ptr, head, head.list) {
		if (hash != ptr->entry.hash || strcmp(name, ptr->entry.name))
			continue;
		ptr->head.users++;
		goto out;
	}
	allocated_len = sizeof(*ptr) + len;
	ptr = ccs_malloc(allocated_len);
	ptr->entry.name = ((char *) ptr) + sizeof(*ptr);
	memmove((char *) ptr->entry.name, name, len);
	ptr->head.users = 1;
	ccs_fill_path_info(&ptr->entry);
	ptr->size = allocated_len;
	list_add_tail(&ptr->head.list, head);
out:
	return &ptr->entry;
}


/**
 * ccs_commit_ok - Allocate memory and check memory quota.
 *
 * @data: Data to copy from.
 * @size: Size in byte.
 *
 * Returns pointer to allocated memory on success, abort otherwise.
 * @data is zero-cleared on success.
 */
static void *ccs_commit_ok(void *data, const unsigned int size)
{
	void *ptr = ccs_malloc(size);
	memmove(ptr, data, size);
	memset(data, 0, size);
	return ptr;
}

/**
 * ccs_permstr - Find permission keywords.
 *
 * @string: String representation for permissions in foo/bar/buz format.
 * @keyword: Keyword to find from @string/
 *
 * Returns ture if @keyword was found in @string, false otherwise.
 *
 * This function assumes that strncmp(w1, w2, strlen(w1)) != 0 if w1 != w2.
 */
static bool ccs_permstr(const char *string, const char *keyword)
{
	const char *cp = strstr(string, keyword);
	if (cp)
		return cp == string || *(cp - 1) == '/';
	return false;
}

/**
 * ccs_read_token - Read a word from a line.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns a word on success, "" otherwise.
 *
 * To allow the caller to skip NULL check, this function returns "" rather than
 * NULL if there is no more words to read.
 */
static char *ccs_read_token(struct ccs_acl_param *param)
{
	char *pos = param->data;
	char *del = strchr(pos, ' ');
	if (del)
		*del++ = '\0';
	else
		del = pos + strlen(pos);
	param->data = del;
	return pos;
}

/**
 * ccs_get_domainname - Read a domainname from a line.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns a domainname on success, NULL otherwise.
 */
static const struct ccs_path_info *ccs_get_domainname
(struct ccs_acl_param *param)
{
	char *start = param->data;
	char *pos = start;
	while (*pos) {
		if (*pos++ != ' ' || *pos++ == '/')
			continue;
		pos -= 2;
		*pos++ = '\0';
		break;
	}
	param->data = pos;
	if (ccs_correct_domain(start))
		return ccs_get_name(start);
	return NULL;
}

/**
 * ccs_get_group - Allocate memory for "struct ccs_path_group"/"struct ccs_number_group"/"struct ccs_address_group".
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @list:  List to use.
 *
 * Returns pointer to "struct ccs_group" on success, NULL otherwise.
 */
static struct ccs_group *ccs_get_group(struct ccs_acl_param *param,
				       struct list_head *list)
{
	struct ccs_group e = { };
	struct ccs_group *group = NULL;
	const char *group_name = ccs_read_token(param);
	bool found = false;
	if (!ccs_correct_word(group_name))
		return NULL;
	e.ns = param->ns;
	e.group_name = ccs_get_name(group_name);
	list_for_each_entry(group, list, head.list) {
		if (e.ns != group->ns || e.group_name != group->group_name)
			continue;
		group->head.users++;
		found = true;
		break;
	}
	if (!found) {
		struct ccs_group *entry = ccs_commit_ok(&e, sizeof(e));
		INIT_LIST_HEAD(&entry->member_list);
		entry->head.users = 1;
		list_add_tail(&entry->head.list, list);
		group = entry;
		found = true;
	}
	ccs_put_name(e.group_name);
	return found ? group : NULL;
}

/**
 * ccs_parse_ulong - Parse an "unsigned long" value.
 *
 * @result: Pointer to "unsigned long".
 * @str:    Pointer to string to parse.
 *
 * Returns one of values in "enum ccs_value_type".
 *
 * The @src is updated to point the first character after the value
 * on success.
 */
static u8 ccs_parse_ulong(unsigned long *result, char **str)
{
	const char *cp = *str;
	char *ep;
	int base = 10;
	if (*cp == '0') {
		char c = *(cp + 1);
		if (c == 'x' || c == 'X') {
			base = 16;
			cp += 2;
		} else if (c >= '0' && c <= '7') {
			base = 8;
			cp++;
		}
	}
	*result = strtoul(cp, &ep, base);
	if (cp == ep)
		return CCS_VALUE_TYPE_INVALID;
	*str = ep;
	switch (base) {
	case 16:
		return CCS_VALUE_TYPE_HEXADECIMAL;
	case 8:
		return CCS_VALUE_TYPE_OCTAL;
	default:
		return CCS_VALUE_TYPE_DECIMAL;
	}
}

/**
 * ccs_parse_name_union - Parse a ccs_name_union.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @ptr:   Pointer to "struct ccs_name_union".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_name_union(struct ccs_acl_param *param,
				 struct ccs_name_union *ptr)
{
	char *filename;
	if (param->data[0] == '@') {
		param->data++;
		ptr->group = ccs_get_group(param, &ccs_path_group);
		return ptr->group != NULL;
	}
	filename = ccs_read_token(param);
	if (!ccs_correct_word(filename))
		return false;
	ptr->filename = ccs_get_name(filename);
	return true;
}

/**
 * ccs_parse_number_union - Parse a ccs_number_union.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @ptr:   Pointer to "struct ccs_number_union".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_number_union(struct ccs_acl_param *param,
				   struct ccs_number_union *ptr)
{
	char *data;
	u8 type;
	unsigned long v;
	memset(ptr, 0, sizeof(*ptr));
	if (param->data[0] == '@') {
		param->data++;
		ptr->group = ccs_get_group(param, &ccs_number_group);
		return ptr->group != NULL;
	}
	data = ccs_read_token(param);
	type = ccs_parse_ulong(&v, &data);
	if (type == CCS_VALUE_TYPE_INVALID)
		return false;
	ptr->values[0] = v;
	ptr->value_type[0] = type;
	if (!*data) {
		ptr->values[1] = v;
		ptr->value_type[1] = type;
		return true;
	}
	if (*data++ != '-')
		return false;
	type = ccs_parse_ulong(&v, &data);
	if (type == CCS_VALUE_TYPE_INVALID || *data || ptr->values[0] > v)
		return false;
	ptr->values[1] = v;
	ptr->value_type[1] = type;
	return true;
}

/**
 * ccs_find_domain2 - Find a domain by the given name.
 *
 * @domainname: The domainname to find.
 *
 * Returns pointer to "struct ccs_domain2_info" if found, NULL otherwise.
 */
static struct ccs_domain2_info *ccs_find_domain2(const char *domainname)
{
	struct ccs_domain2_info *domain;
	struct ccs_path_info name;
	name.name = domainname;
	ccs_fill_path_info(&name);
	list_for_each_entry(domain, &ccs_domain_list, list) {
		if (!domain->is_deleted &&
		    !ccs_pathcmp(&name, domain->domainname))
			return domain;
	}
	return NULL;
}

static int client_fd = EOF;

static void cprintf(const char *fmt, ...)
	__attribute__ ((format(printf, 1, 2)));

/**
 * cprintf - printf() over socket.
 *
 * @fmt: The printf()'s format string, followed by parameters.
 *
 * Returns nothing.
 */
static void cprintf(const char *fmt, ...)
{
	va_list args;
	static char *buffer = NULL;
	static unsigned int buffer_len = 0;
	static unsigned int buffer_pos = 0;
	int len;
	if (head.reset) {
		head.reset = false;
		buffer_pos = 0;
	}
	while (1) {
		va_start(args, fmt);
		len = vsnprintf(buffer + buffer_pos, buffer_len - buffer_pos,
				fmt, args);
		va_end(args);
		if (len < 0)
			_exit(1);
		if (buffer_pos + len < buffer_len) {
			buffer_pos += len;
			break;
		}
		buffer_len = buffer_pos + len + 4096;
		buffer = ccs_realloc(buffer, buffer_len);
	}
	if (len && buffer_pos < 1048576)
		return;
	/*
	 * Reader might close connection without reading until EOF.
	 * In that case, we should not call _exit() because offline daemon does
	 * not call fork() for each accept()ed socket connection.
	 */
	if (write(client_fd, buffer, buffer_pos) != buffer_pos) {
		close(client_fd);
		client_fd = EOF;
	}
	buffer_pos = 0;
}

/**
 * ccs_update_policy - Update an entry for exception policy.
 *
 * @new_entry:       Pointer to "struct ccs_acl_info".
 * @size:            Size of @new_entry in bytes.
 * @param:           Pointer to "struct ccs_acl_param".
 * @check_duplicate: Callback function to find duplicated entry.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_policy(struct ccs_acl_head *new_entry, const int size,
			     struct ccs_acl_param *param,
			     bool (*check_duplicate)
			     (const struct ccs_acl_head *,
			      const struct ccs_acl_head *))
{
	int error = param->is_delete ? -ENOENT : -ENOMEM;
	struct ccs_acl_head *entry;
	struct list_head *list = param->list;
	list_for_each_entry(entry, list, list) {
		if (!check_duplicate(entry, new_entry))
			continue;
		entry->is_deleted = param->is_delete;
		error = 0;
		break;
	}
	if (error && !param->is_delete) {
		entry = ccs_commit_ok(new_entry, size);
		list_add_tail(&entry->list, list);
		error = 0;
	}
	return error;
}

/* List of "struct ccs_condition". */
static LIST_HEAD(ccs_condition_list);

/**
 * ccs_get_dqword - ccs_get_name() for a quoted string.
 *
 * @start: String to save.
 *
 * Returns pointer to "struct ccs_path_info" on success, NULL otherwise.
 */
static const struct ccs_path_info *ccs_get_dqword(char *start)
{
	char *cp = start + strlen(start) - 1;
	if (cp == start || *start++ != '"' || *cp != '"')
		return NULL;
	*cp = '\0';
	if (*start && !ccs_correct_word(start))
		return NULL;
	return ccs_get_name(start);
}

/**
 * ccs_parse_name_union_quoted - Parse a quoted word.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @ptr:   Pointer to "struct ccs_name_union".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_name_union_quoted(struct ccs_acl_param *param,
					struct ccs_name_union *ptr)
{
	char *filename = param->data;
	if (*filename == '@')
		return ccs_parse_name_union(param, ptr);
	ptr->filename = ccs_get_dqword(filename);
	return ptr->filename != NULL;
}

/**
 * ccs_parse_argv - Parse an argv[] condition part.
 *
 * @left:  Lefthand value.
 * @right: Righthand value.
 * @argv:  Pointer to "struct ccs_argv".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_argv(char *left, char *right, struct ccs_argv *argv)
{
	if (ccs_parse_ulong(&argv->index, &left) != CCS_VALUE_TYPE_DECIMAL ||
	    *left++ != ']' || *left)
		return false;
	argv->value = ccs_get_dqword(right);
	return argv->value != NULL;
}

/**
 * ccs_parse_envp - Parse an envp[] condition part.
 *
 * @left:  Lefthand value.
 * @right: Righthand value.
 * @envp:  Pointer to "struct ccs_envp".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_envp(char *left, char *right, struct ccs_envp *envp)
{
	const struct ccs_path_info *name;
	const struct ccs_path_info *value;
	char *cp = left + strlen(left) - 1;
	if (*cp-- != ']' || *cp != '"')
		goto out;
	*cp = '\0';
	if (!ccs_correct_word(left))
		goto out;
	name = ccs_get_name(left);
	if (!strcmp(right, "NULL")) {
		value = NULL;
	} else {
		value = ccs_get_dqword(right);
		if (!value) {
			ccs_put_name(name);
			goto out;
		}
	}
	envp->name = name;
	envp->value = value;
	return true;
out:
	return false;
}

/**
 * ccs_same_condition - Check for duplicated "struct ccs_condition" entry.
 *
 * @a: Pointer to "struct ccs_condition".
 * @b: Pointer to "struct ccs_condition".
 *
 * Returns true if @a == @b, false otherwise.
 */
static inline bool ccs_same_condition(const struct ccs_condition *a,
				      const struct ccs_condition *b)
{
	return a->size == b->size && a->condc == b->condc &&
		a->numbers_count == b->numbers_count &&
		a->names_count == b->names_count &&
		a->argc == b->argc && a->envc == b->envc &&
		a->grant_log == b->grant_log && a->transit == b->transit &&
		!memcmp(a + 1, b + 1, a->size - sizeof(*a));
}

/**
 * ccs_condition_type - Get condition type.
 *
 * @word: Keyword string.
 *
 * Returns one of values in "enum ccs_conditions_index" on success,
 * CCS_MAX_CONDITION_KEYWORD otherwise.
 */
static u8 ccs_condition_type(const char *word)
{
	u8 i;
	for (i = 0; i < CCS_MAX_CONDITION_KEYWORD; i++) {
		if (!strcmp(word, ccs_condition_keyword[i]))
			break;
	}
	return i;
}

/* Define this to enable debug mode. */
/* #define DEBUG_CONDITION */

#ifdef DEBUG_CONDITION
#define dprintk printk
#else
#define dprintk(...) do { } while (0)
#endif

/**
 * ccs_commit_condition - Commit "struct ccs_condition".
 *
 * @entry: Pointer to "struct ccs_condition".
 *
 * Returns pointer to "struct ccs_condition" on success, NULL otherwise.
 *
 * This function merges duplicated entries. This function returns NULL if
 * @entry is not duplicated but memory quota for policy has exceeded.
 */
static struct ccs_condition *ccs_commit_condition(struct ccs_condition *entry)
{
	struct ccs_condition *ptr;
	bool found = false;
	list_for_each_entry(ptr, &ccs_condition_list, head.list) {
		if (!ccs_same_condition(ptr, entry))
			continue;
		/* Same entry found. Share this entry. */
		ptr->head.users++;
		found = true;
		break;
	}
	if (!found) {
		if (entry) {
			entry->head.users = 1;
			list_add(&entry->head.list, &ccs_condition_list);
		} else {
			found = true;
			ptr = NULL;
		}
	}
	if (found) {
		ccs_del_condition(&entry->head.list);
		free(entry);
		entry = ptr;
	}
	return entry;
}

/**
 * ccs_get_condition - Parse condition part.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns pointer to "struct ccs_condition" on success, NULL otherwise.
 */
static struct ccs_condition *ccs_get_condition(struct ccs_acl_param *param)
{
	struct ccs_condition *entry = NULL;
	struct ccs_condition_element *condp = NULL;
	struct ccs_number_union *numbers_p = NULL;
	struct ccs_name_union *names_p = NULL;
	struct ccs_argv *argv = NULL;
	struct ccs_envp *envp = NULL;
	struct ccs_condition e = { };
	char * const start_of_string = param->data;
	char * const end_of_string = start_of_string + strlen(start_of_string);
	char *pos;
rerun:
	pos = start_of_string;
	while (1) {
		u8 left = -1;
		u8 right = -1;
		char *left_word = pos;
		char *cp;
		char *right_word;
		bool is_not;
		if (!*left_word)
			break;
		/*
		 * Since left-hand condition does not allow use of "path_group"
		 * or "number_group" and environment variable's names do not
		 * accept '=', it is guaranteed that the original line consists
		 * of one or more repetition of $left$operator$right blocks
		 * where "$left is free from '=' and ' '" and "$operator is
		 * either '=' or '!='" and "$right is free from ' '".
		 * Therefore, we can reconstruct the original line at the end
		 * of dry run even if we overwrite $operator with '\0'.
		 */
		cp = strchr(pos, ' ');
		if (cp) {
			*cp = '\0'; /* Will restore later. */
			pos = cp + 1;
		} else {
			pos = "";
		}
		right_word = strchr(left_word, '=');
		if (!right_word || right_word == left_word)
			goto out;
		is_not = *(right_word - 1) == '!';
		if (is_not)
			*(right_word++ - 1) = '\0'; /* Will restore later. */
		else if (*(right_word + 1) != '=')
			*right_word++ = '\0'; /* Will restore later. */
		else
			goto out;
		dprintk(KERN_WARNING "%u: <%s>%s=<%s>\n", __LINE__, left_word,
			is_not ? "!" : "", right_word);
		if (!strcmp(left_word, "grant_log")) {
			if (entry) {
				if (is_not ||
				    entry->grant_log != CCS_GRANTLOG_AUTO)
					goto out;
				else if (!strcmp(right_word, "yes"))
					entry->grant_log = CCS_GRANTLOG_YES;
				else if (!strcmp(right_word, "no"))
					entry->grant_log = CCS_GRANTLOG_NO;
				else
					goto out;
			}
			continue;
		}
		if (!strcmp(left_word, "auto_domain_transition")) {
			if (entry) {
				if (is_not || entry->transit)
					goto out;
				entry->transit = ccs_get_dqword(right_word);
				if (!entry->transit ||
				    (entry->transit->name[0] != '/' &&
				     !ccs_domain_def(entry->transit->name)))
					goto out;
			}
			continue;
		}
		if (!strncmp(left_word, "exec.argv[", 10)) {
			if (!argv) {
				e.argc++;
				e.condc++;
			} else {
				e.argc--;
				e.condc--;
				left = CCS_ARGV_ENTRY;
				argv->is_not = is_not;
				if (!ccs_parse_argv(left_word + 10,
						    right_word, argv++))
					goto out;
			}
			goto store_value;
		}
		if (!strncmp(left_word, "exec.envp[\"", 11)) {
			if (!envp) {
				e.envc++;
				e.condc++;
			} else {
				e.envc--;
				e.condc--;
				left = CCS_ENVP_ENTRY;
				envp->is_not = is_not;
				if (!ccs_parse_envp(left_word + 11,
						    right_word, envp++))
					goto out;
			}
			goto store_value;
		}
		left = ccs_condition_type(left_word);
		dprintk(KERN_WARNING "%u: <%s> left=%u\n", __LINE__, left_word,
			left);
		if (left == CCS_MAX_CONDITION_KEYWORD) {
			if (!numbers_p) {
				e.numbers_count++;
			} else {
				e.numbers_count--;
				left = CCS_NUMBER_UNION;
				param->data = left_word;
				if (*left_word == '@' ||
				    !ccs_parse_number_union(param,
							    numbers_p++))
					goto out;
			}
		}
		if (!condp)
			e.condc++;
		else
			e.condc--;
		if (left == CCS_EXEC_REALPATH || left == CCS_SYMLINK_TARGET) {
			if (!names_p) {
				e.names_count++;
			} else {
				e.names_count--;
				right = CCS_NAME_UNION;
				param->data = right_word;
				if (!ccs_parse_name_union_quoted(param,
								 names_p++))
					goto out;
			}
			goto store_value;
		}
		right = ccs_condition_type(right_word);
		if (right == CCS_MAX_CONDITION_KEYWORD) {
			if (!numbers_p) {
				e.numbers_count++;
			} else {
				e.numbers_count--;
				right = CCS_NUMBER_UNION;
				param->data = right_word;
				if (!ccs_parse_number_union(param,
							    numbers_p++))
					goto out;
			}
		}
store_value:
		if (!condp) {
			dprintk(KERN_WARNING "%u: dry_run left=%u right=%u "
				"match=%u\n", __LINE__, left, right, !is_not);
			continue;
		}
		condp->left = left;
		condp->right = right;
		condp->equals = !is_not;
		dprintk(KERN_WARNING "%u: left=%u right=%u match=%u\n",
			__LINE__, condp->left, condp->right,
			condp->equals);
		condp++;
	}
	dprintk(KERN_INFO "%u: cond=%u numbers=%u names=%u ac=%u ec=%u\n",
		__LINE__, e.condc, e.numbers_count, e.names_count, e.argc,
		e.envc);
	if (entry)
		return ccs_commit_condition(entry);
	e.size = sizeof(*entry)
		+ e.condc * sizeof(struct ccs_condition_element)
		+ e.numbers_count * sizeof(struct ccs_number_union)
		+ e.names_count * sizeof(struct ccs_name_union)
		+ e.argc * sizeof(struct ccs_argv)
		+ e.envc * sizeof(struct ccs_envp);
	entry = ccs_malloc(e.size);
	*entry = e;
	condp = (struct ccs_condition_element *) (entry + 1);
	numbers_p = (struct ccs_number_union *) (condp + e.condc);
	names_p = (struct ccs_name_union *) (numbers_p + e.numbers_count);
	argv = (struct ccs_argv *) (names_p + e.names_count);
	envp = (struct ccs_envp *) (argv + e.argc);
	{
		bool flag = false;
		for (pos = start_of_string; pos < end_of_string; pos++) {
			if (*pos)
				continue;
			if (flag) /* Restore " ". */
				*pos = ' ';
			else if (*(pos + 1) == '=') /* Restore "!=". */
				*pos = '!';
			else /* Restore "=". */
				*pos = '=';
			flag = !flag;
		}
	}
	goto rerun;
out:
	dprintk(KERN_WARNING "%u: %s failed\n", __LINE__, __func__);
	if (entry) {
		ccs_del_condition(&entry->head.list);
		free(entry);
	}
	return NULL;
}

/**
 * ccs_same_acl_head - Check for duplicated "struct ccs_acl_info" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static inline bool ccs_same_acl_head(const struct ccs_acl_info *a,
				     const struct ccs_acl_info *b)
{
	return a->type == b->type && a->cond == b->cond;
}

/**
 * ccs_update_domain - Update an entry for domain policy.
 *
 * @new_entry:       Pointer to "struct ccs_acl_info".
 * @size:            Size of @new_entry in bytes.
 * @param:           Pointer to "struct ccs_acl_param".
 * @check_duplicate: Callback function to find duplicated entry.
 * @merge_duplicate: Callback function to merge duplicated entry. Maybe NULL.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_domain(struct ccs_acl_info *new_entry, const int size,
			     struct ccs_acl_param *param,
			     bool (*check_duplicate)
			     (const struct ccs_acl_info *,
			      const struct ccs_acl_info *),
			     bool (*merge_duplicate)
			     (struct ccs_acl_info *, struct ccs_acl_info *,
			      const bool))
{
	const bool is_delete = param->is_delete;
	int error = is_delete ? -ENOENT : -ENOMEM;
	struct ccs_acl_info *entry;
	struct list_head * const list = param->list;
	if (param->data[0]) {
		new_entry->cond = ccs_get_condition(param);
		if (!new_entry->cond)
			return -EINVAL;
	}
	list_for_each_entry(entry, list, list) {
		if (!ccs_same_acl_head(entry, new_entry) ||
		    !check_duplicate(entry, new_entry))
			continue;
		if (merge_duplicate)
			entry->is_deleted = merge_duplicate(entry, new_entry,
							    is_delete);
		else
			entry->is_deleted = is_delete;
		error = 0;
		break;
	}
	if (error && !is_delete) {
		entry = ccs_commit_ok(new_entry, size);
		list_add_tail(&entry->list, list);
		error = 0;
	}
	ccs_put_condition(new_entry->cond);
	return error;
}

/**
 * ccs_same_transition_control - Check for duplicated "struct ccs_transition_control" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_transition_control(const struct ccs_acl_head *a,
					const struct ccs_acl_head *b)
{
	const struct ccs_transition_control *p1 = container_of(a, typeof(*p1),
							       head);
	const struct ccs_transition_control *p2 = container_of(b, typeof(*p2),
							       head);
	return p1->type == p2->type && p1->is_last_name == p2->is_last_name
		&& p1->domainname == p2->domainname
		&& p1->program == p2->program && p1->ns == p2->ns;
}

/**
 * ccs_write_transition_control - Write "struct ccs_transition_control" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @type:  Type of this entry.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_transition_control(struct ccs_acl_param *param,
					const u8 type)
{
	struct ccs_transition_control e = { .type = type, .ns = param->ns };
	int error = param->is_delete ? -ENOENT : -ENOMEM;
	char *program = param->data;
	char *domainname = strstr(program, " from ");
	if (domainname) {
		*domainname = '\0';
		domainname += 6;
	} else if (type == CCS_TRANSITION_CONTROL_NO_KEEP ||
		   type == CCS_TRANSITION_CONTROL_KEEP) {
		domainname = program;
		program = NULL;
	}
	if (program && strcmp(program, "any")) {
		if (!ccs_correct_path(program))
			return -EINVAL;
		e.program = ccs_get_name(program);
	}
	if (domainname && strcmp(domainname, "any")) {
		if (!ccs_correct_domain(domainname)) {
			if (!ccs_correct_path(domainname))
				goto out;
			e.is_last_name = true;
		}
		e.domainname = ccs_get_name(domainname);
	}
	param->list = &ccs_transition_list;
	error = ccs_update_policy(&e.head, sizeof(e), param,
				  ccs_same_transition_control);
out:
	ccs_put_name(e.domainname);
	ccs_put_name(e.program);
	return error;
}

/**
 * ccs_same_aggregator - Check for duplicated "struct ccs_aggregator" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_aggregator(const struct ccs_acl_head *a,
				const struct ccs_acl_head *b)
{
	const struct ccs_aggregator *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_aggregator *p2 = container_of(b, typeof(*p2), head);
	return p1->original_name == p2->original_name &&
		p1->aggregated_name == p2->aggregated_name && p1->ns == p2->ns;
}

/**
 * ccs_write_aggregator - Write "struct ccs_aggregator" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_aggregator(struct ccs_acl_param *param)
{
	struct ccs_aggregator e = { .ns = param->ns };
	int error = param->is_delete ? -ENOENT : -ENOMEM;
	const char *original_name = ccs_read_token(param);
	const char *aggregated_name = ccs_read_token(param);
	if (!ccs_correct_word(original_name) ||
	    !ccs_correct_path(aggregated_name))
		return -EINVAL;
	e.original_name = ccs_get_name(original_name);
	e.aggregated_name = ccs_get_name(aggregated_name);
	if (e.aggregated_name->is_patterned) /* No patterns allowed. */
		goto out;
	param->list = &ccs_aggregator_list;
	error = ccs_update_policy(&e.head, sizeof(e), param,
				  ccs_same_aggregator);
out:
	ccs_put_name(e.original_name);
	ccs_put_name(e.aggregated_name);
	return error;
}

/* Domain create handler. */

/**
 * ccs_find_namespace - Find specified namespace.
 *
 * @name: Name of namespace to find.
 * @len:  Length of @name.
 *
 * Returns pointer to "struct ccs_policy_namespace" if found, NULL otherwise.
 */
static struct ccs_policy_namespace *ccs_find_namespace(const char *name,
						       const unsigned int len)
{
	struct ccs_policy_namespace *ns;
	list_for_each_entry(ns, &ccs_namespace_list, namespace_list) {
		if (strncmp(name, ns->name, len) ||
		    (name[len] && name[len] != ' '))
			continue;
		return ns;
	}
	return NULL;
}

/**
 * ccs_assign_namespace - Create a new namespace.
 *
 * @domainname: Name of namespace to create.
 *
 * Returns pointer to "struct ccs_policy_namespace" on success, NULL otherwise.
 */
static struct ccs_policy_namespace *ccs_assign_namespace(const char *domainname)
{
	struct ccs_policy_namespace *ptr;
	struct ccs_policy_namespace *entry;
	const char *cp = domainname;
	unsigned int len = 0;
	while (*cp && *cp++ != ' ')
		len++;
	ptr = ccs_find_namespace(domainname, len);
	if (ptr)
		return ptr;
	if (len >= CCS_EXEC_TMPSIZE - 10 || !ccs_domain_def(domainname))
		return NULL;
	entry = ccs_malloc(sizeof(*entry) + len + 1);
	{
		char *name = (char *) (entry + 1);
		memmove(name, domainname, len);
		name[len] = '\0';
		entry->name = name;
	}
	entry->profile_version = 20100903;
	for (len = 0; len < CCS_MAX_ACL_GROUPS; len++)
		INIT_LIST_HEAD(&entry->acl_group[len]);
	ccs_namespace_enabled = !list_empty(&ccs_namespace_list);
	list_add_tail(&entry->namespace_list, &ccs_namespace_list);
	return entry;
}

/**
 * ccs_assign_domain2 - Create a domain or a namespace.
 *
 * @domainname: The name of domain.
 *
 * Returns pointer to "struct ccs_domain2_info" on success, NULL otherwise.
 */
static struct ccs_domain2_info *ccs_assign_domain2(const char *domainname)
{
	struct ccs_domain2_info e = { };
	struct ccs_domain2_info *entry = ccs_find_domain2(domainname);
	if (entry)
		return entry;
	/* Requested domain does not exist. */
	/* Don't create requested domain if domainname is invalid. */
	if (strlen(domainname) >= CCS_EXEC_TMPSIZE - 10 ||
	    !ccs_correct_domain(domainname))
		return NULL;
	e.domainname = ccs_get_name(domainname);
	entry = ccs_commit_ok(&e, sizeof(e));
	INIT_LIST_HEAD(&entry->acl_info_list);
	list_add_tail(&entry->list, &ccs_domain_list);
	ccs_put_name(e.domainname);
	return entry;
}

/**
 * ccs_same_path_acl - Check for duplicated "struct ccs_path_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_path_acl(const struct ccs_acl_info *a,
			      const struct ccs_acl_info *b)
{
	const struct ccs_path_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_path_acl *p2 = container_of(b, typeof(*p2), head);
	return ccs_same_name_union(&p1->name, &p2->name);
}

/**
 * ccs_merge_path_acl - Merge duplicated "struct ccs_path_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_path_acl(struct ccs_acl_info *a, struct ccs_acl_info *b,
			       const bool is_delete)
{
	u16 * const a_perm = &container_of(a, struct ccs_path_acl, head)->perm;
	u16 perm = *a_perm;
	const u16 b_perm = container_of(b, struct ccs_path_acl, head)->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_update_path_acl - Update "struct ccs_path_acl" list.
 *
 * @perm:  Permission.
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_path_acl(const u16 perm, struct ccs_acl_param *param)
{
	struct ccs_path_acl e = {
		.head.type = CCS_TYPE_PATH_ACL,
		.perm = perm
	};
	int error;
	if (!ccs_parse_name_union(param, &e.name))
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_path_acl,
					  ccs_merge_path_acl);
	ccs_put_name_union(&e.name);
	return error;
}

/**
 * ccs_same_mkdev_acl - Check for duplicated "struct ccs_mkdev_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_mkdev_acl(const struct ccs_acl_info *a,
			       const struct ccs_acl_info *b)
{
	const struct ccs_mkdev_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_mkdev_acl *p2 = container_of(b, typeof(*p2), head);
	return ccs_same_name_union(&p1->name, &p2->name) &&
		ccs_same_number_union(&p1->mode, &p2->mode) &&
		ccs_same_number_union(&p1->major, &p2->major) &&
		ccs_same_number_union(&p1->minor, &p2->minor);
}

/**
 * ccs_merge_mkdev_acl - Merge duplicated "struct ccs_mkdev_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_mkdev_acl(struct ccs_acl_info *a, struct ccs_acl_info *b,
				const bool is_delete)
{
	u8 *const a_perm = &container_of(a, struct ccs_mkdev_acl, head)->perm;
	u8 perm = *a_perm;
	const u8 b_perm = container_of(b, struct ccs_mkdev_acl, head)->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_update_mkdev_acl - Update "struct ccs_mkdev_acl" list.
 *
 * @perm:  Permission.
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_mkdev_acl(const u8 perm, struct ccs_acl_param *param)
{
	struct ccs_mkdev_acl e = {
		.head.type = CCS_TYPE_MKDEV_ACL,
		.perm = perm
	};
	int error;
	if (!ccs_parse_name_union(param, &e.name) ||
	    !ccs_parse_number_union(param, &e.mode) ||
	    !ccs_parse_number_union(param, &e.major) ||
	    !ccs_parse_number_union(param, &e.minor))
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_mkdev_acl,
					  ccs_merge_mkdev_acl);
	ccs_put_name_union(&e.name);
	ccs_put_number_union(&e.mode);
	ccs_put_number_union(&e.major);
	ccs_put_number_union(&e.minor);
	return error;
}

/**
 * ccs_same_path2_acl - Check for duplicated "struct ccs_path2_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_path2_acl(const struct ccs_acl_info *a,
			       const struct ccs_acl_info *b)
{
	const struct ccs_path2_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_path2_acl *p2 = container_of(b, typeof(*p2), head);
	return ccs_same_name_union(&p1->name1, &p2->name1) &&
		ccs_same_name_union(&p1->name2, &p2->name2);
}

/**
 * ccs_merge_path2_acl - Merge duplicated "struct ccs_path2_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_path2_acl(struct ccs_acl_info *a, struct ccs_acl_info *b,
				const bool is_delete)
{
	u8 * const a_perm = &container_of(a, struct ccs_path2_acl, head)->perm;
	u8 perm = *a_perm;
	const u8 b_perm = container_of(b, struct ccs_path2_acl, head)->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_update_path2_acl - Update "struct ccs_path2_acl" list.
 *
 * @perm:  Permission.
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_path2_acl(const u8 perm, struct ccs_acl_param *param)
{
	struct ccs_path2_acl e = {
		.head.type = CCS_TYPE_PATH2_ACL,
		.perm = perm
	};
	int error;
	if (!ccs_parse_name_union(param, &e.name1) ||
	    !ccs_parse_name_union(param, &e.name2))
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_path2_acl,
					  ccs_merge_path2_acl);
	ccs_put_name_union(&e.name1);
	ccs_put_name_union(&e.name2);
	return error;
}

/**
 * ccs_same_path_number_acl - Check for duplicated "struct ccs_path_number_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_path_number_acl(const struct ccs_acl_info *a,
				     const struct ccs_acl_info *b)
{
	const struct ccs_path_number_acl *p1 = container_of(a, typeof(*p1),
							    head);
	const struct ccs_path_number_acl *p2 = container_of(b, typeof(*p2),
							    head);
	return ccs_same_name_union(&p1->name, &p2->name) &&
		ccs_same_number_union(&p1->number, &p2->number);
}

/**
 * ccs_merge_path_number_acl - Merge duplicated "struct ccs_path_number_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_path_number_acl(struct ccs_acl_info *a,
				      struct ccs_acl_info *b,
				      const bool is_delete)
{
	u8 * const a_perm = &container_of(a, struct ccs_path_number_acl, head)
		->perm;
	u8 perm = *a_perm;
	const u8 b_perm = container_of(b, struct ccs_path_number_acl, head)
		->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_update_path_number_acl - Update create/mkdir/mkfifo/mksock/ioctl/chmod/chown/chgrp ACL.
 *
 * @perm:  Permission.
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_path_number_acl(const u8 perm,
				      struct ccs_acl_param *param)
{
	struct ccs_path_number_acl e = {
		.head.type = CCS_TYPE_PATH_NUMBER_ACL,
		.perm = perm
	};
	int error;
	if (!ccs_parse_name_union(param, &e.name) ||
	    !ccs_parse_number_union(param, &e.number))
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_path_number_acl,
					  ccs_merge_path_number_acl);
	ccs_put_name_union(&e.name);
	ccs_put_number_union(&e.number);
	return error;
}

/**
 * ccs_same_mount_acl - Check for duplicated "struct ccs_mount_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_mount_acl(const struct ccs_acl_info *a,
			       const struct ccs_acl_info *b)
{
	const struct ccs_mount_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_mount_acl *p2 = container_of(b, typeof(*p2), head);
	return ccs_same_name_union(&p1->dev_name, &p2->dev_name) &&
		ccs_same_name_union(&p1->dir_name, &p2->dir_name) &&
		ccs_same_name_union(&p1->fs_type, &p2->fs_type) &&
		ccs_same_number_union(&p1->flags, &p2->flags);
}

/**
 * ccs_update_mount_acl - Write "struct ccs_mount_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_update_mount_acl(struct ccs_acl_param *param)
{
	struct ccs_mount_acl e = { .head.type = CCS_TYPE_MOUNT_ACL };
	int error;
	if (!ccs_parse_name_union(param, &e.dev_name) ||
	    !ccs_parse_name_union(param, &e.dir_name) ||
	    !ccs_parse_name_union(param, &e.fs_type) ||
	    !ccs_parse_number_union(param, &e.flags))
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_mount_acl, NULL);
	ccs_put_name_union(&e.dev_name);
	ccs_put_name_union(&e.dir_name);
	ccs_put_name_union(&e.fs_type);
	ccs_put_number_union(&e.flags);
	return error;
}

/**
 * ccs_write_file - Update file related list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_file(struct ccs_acl_param *param)
{
	u16 perm = 0;
	u8 type;
	const char *operation = ccs_read_token(param);
	for (type = 0; type < CCS_MAX_PATH_OPERATION; type++)
		if (ccs_permstr(operation, ccs_path_keyword[type]))
			perm |= 1 << type;
	if (perm)
		return ccs_update_path_acl(perm, param);
	for (type = 0; type < CCS_MAX_PATH2_OPERATION; type++)
		if (ccs_permstr(operation, ccs_mac_keywords[ccs_pp2mac[type]]))
			perm |= 1 << type;
	if (perm)
		return ccs_update_path2_acl(perm, param);
	for (type = 0; type < CCS_MAX_PATH_NUMBER_OPERATION; type++)
		if (ccs_permstr(operation, ccs_mac_keywords[ccs_pn2mac[type]]))
			perm |= 1 << type;
	if (perm)
		return ccs_update_path_number_acl(perm, param);
	for (type = 0; type < CCS_MAX_MKDEV_OPERATION; type++)
		if (ccs_permstr(operation,
				ccs_mac_keywords[ccs_pnnn2mac[type]]))
			perm |= 1 << type;
	if (perm)
		return ccs_update_mkdev_acl(perm, param);
	if (ccs_permstr(operation, ccs_mac_keywords[CCS_MAC_FILE_MOUNT]))
		return ccs_update_mount_acl(param);
	return -EINVAL;
}

/**
 * ccs_parse_ipaddr_union - Parse an IP address.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @ptr:   Pointer to "struct ccs_ipaddr_union".
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_parse_ipaddr_union(struct ccs_acl_param *param,
				   struct ccs_ipaddr_union *ptr)
{
	struct ccs_ip_address_entry e;
	memset(ptr, 0, sizeof(ptr));
	if (ccs_parse_ip(ccs_read_token(param), &e) == 0) {
		memmove(&ptr->ip[0], e.min, sizeof(ptr->ip[0]));
		memmove(&ptr->ip[1], e.max, sizeof(ptr->ip[1]));
		ptr->is_ipv6 = e.is_ipv6;
		return true;
	}
	return false;
}

/*
 * Routines for printing IPv4 or IPv6 address.
 * These are copied from include/linux/kernel.h include/net/ipv6.h
 * include/net/addrconf.h lib/hexdump.c lib/vsprintf.c and simplified.
 */
static const char hex_asc[] = "0123456789abcdef";
#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]

static inline char *pack_hex_byte(char *buf, u8 byte)
{
	*buf++ = hex_asc_hi(byte);
	*buf++ = hex_asc_lo(byte);
	return buf;
}

static inline int ipv6_addr_v4mapped(const struct in6_addr *a)
{
	return (a->s6_addr32[0] | a->s6_addr32[1] |
		(a->s6_addr32[2] ^ htonl(0x0000ffff))) == 0;
}

static inline int ipv6_addr_is_isatap(const struct in6_addr *addr)
{
	return (addr->s6_addr32[2] | htonl(0x02000000)) == htonl(0x02005EFE);
}

static char *ip4_string(char *p, const u8 *addr)
{
	/*
	 * Since this function is called outside vsnprintf(), I can use
	 * sprintf() here.
	 */
	return p +
		sprintf(p, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
}

static char *ip6_compressed_string(char *p, const char *addr)
{
	int i, j, range;
	unsigned char zerolength[8];
	int longest = 1;
	int colonpos = -1;
	u16 word;
	u8 hi, lo;
	bool needcolon = false;
	bool useIPv4;
	struct in6_addr in6;

	memcpy(&in6, addr, sizeof(struct in6_addr));

	useIPv4 = ipv6_addr_v4mapped(&in6) || ipv6_addr_is_isatap(&in6);

	memset(zerolength, 0, sizeof(zerolength));

	if (useIPv4)
		range = 6;
	else
		range = 8;

	/* find position of longest 0 run */
	for (i = 0; i < range; i++) {
		for (j = i; j < range; j++) {
			if (in6.s6_addr16[j] != 0)
				break;
			zerolength[i]++;
		}
	}
	for (i = 0; i < range; i++) {
		if (zerolength[i] > longest) {
			longest = zerolength[i];
			colonpos = i;
		}
	}
	if (longest == 1)		/* don't compress a single 0 */
		colonpos = -1;

	/* emit address */
	for (i = 0; i < range; i++) {
		if (i == colonpos) {
			if (needcolon || i == 0)
				*p++ = ':';
			*p++ = ':';
			needcolon = false;
			i += longest - 1;
			continue;
		}
		if (needcolon) {
			*p++ = ':';
			needcolon = false;
		}
		/* hex u16 without leading 0s */
		word = ntohs(in6.s6_addr16[i]);
		hi = word >> 8;
		lo = word & 0xff;
		if (hi) {
			if (hi > 0x0f)
				p = pack_hex_byte(p, hi);
			else
				*p++ = hex_asc_lo(hi);
			p = pack_hex_byte(p, lo);
		}
		else if (lo > 0x0f)
			p = pack_hex_byte(p, lo);
		else
			*p++ = hex_asc_lo(lo);
		needcolon = true;
	}

	if (useIPv4) {
		if (needcolon)
			*p++ = ':';
		p = ip4_string(p, &in6.s6_addr[12]);
	}
	*p = '\0';

	return p;
}

/**
 * ccs_print_ipv4 - Print an IPv4 address.
 *
 * @min_ip: Pointer to "u32 in network byte order".
 * @max_ip: Pointer to "u32 in network byte order".
 *
 * Returns nothing.
 */
static void ccs_print_ipv4(const u32 *min_ip, const u32 *max_ip)
{
	char addr[sizeof("255.255.255.255")];
	ip4_string(addr, (const u8 *) min_ip);
	cprintf("%s", addr);
	if (*min_ip == *max_ip)
		return;
	ip4_string(addr, (const u8 *) max_ip);
	cprintf("-%s", addr);
}

/**
 * ccs_print_ipv6 - Print an IPv6 address.
 *
 * @min_ip: Pointer to "struct in6_addr".
 * @max_ip: Pointer to "struct in6_addr".
 *
 * Returns nothing.
 */
static void ccs_print_ipv6(const struct in6_addr *min_ip,
			   const struct in6_addr *max_ip)
{
	char addr[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255")];
	ip6_compressed_string(addr, (const char *) min_ip);
	cprintf("%s", addr);
	if (!memcmp(min_ip, max_ip, 16))
		return;
	ip6_compressed_string(addr, (const char *) max_ip);
	cprintf("-%s", addr);
}

/**
 * ccs_print_ip - Print an IP address.
 *
 * @ptr: Pointer to "struct ccs_ipaddr_union".
 *
 * Returns nothing.
 */
static void ccs_print_ip(const struct ccs_ipaddr_union *ptr)
{
	if (ptr->is_ipv6)
		ccs_print_ipv6(&ptr->ip[0], &ptr->ip[1]);
	else
		ccs_print_ipv4(&ptr->ip[0].s6_addr32[0],
			       &ptr->ip[1].s6_addr32[0]);
}

/**
 * ccs_same_inet_acl - Check for duplicated "struct ccs_inet_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_inet_acl(const struct ccs_acl_info *a,
			      const struct ccs_acl_info *b)
{
	const struct ccs_inet_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_inet_acl *p2 = container_of(b, typeof(*p2), head);
	return p1->protocol == p2->protocol &&
		ccs_same_ipaddr_union(&p1->address, &p2->address) &&
		ccs_same_number_union(&p1->port, &p2->port);
}

/**
 * ccs_same_unix_acl - Check for duplicated "struct ccs_unix_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b except permission bits, false otherwise.
 */
static bool ccs_same_unix_acl(const struct ccs_acl_info *a,
			      const struct ccs_acl_info *b)
{
	const struct ccs_unix_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_unix_acl *p2 = container_of(b, typeof(*p2), head);
	return p1->protocol == p2->protocol &&
		ccs_same_name_union(&p1->name, &p2->name);
}

/**
 * ccs_merge_inet_acl - Merge duplicated "struct ccs_inet_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_inet_acl(struct ccs_acl_info *a, struct ccs_acl_info *b,
			       const bool is_delete)
{
	u8 * const a_perm = &container_of(a, struct ccs_inet_acl, head)->perm;
	u8 perm = *a_perm;
	const u8 b_perm = container_of(b, struct ccs_inet_acl, head)->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_merge_unix_acl - Merge duplicated "struct ccs_unix_acl" entry.
 *
 * @a:         Pointer to "struct ccs_acl_info".
 * @b:         Pointer to "struct ccs_acl_info".
 * @is_delete: True for @a &= ~@b, false for @a |= @b.
 *
 * Returns true if @a is empty, false otherwise.
 */
static bool ccs_merge_unix_acl(struct ccs_acl_info *a, struct ccs_acl_info *b,
			       const bool is_delete)
{
	u8 * const a_perm = &container_of(a, struct ccs_unix_acl, head)->perm;
	u8 perm = *a_perm;
	const u8 b_perm = container_of(b, struct ccs_unix_acl, head)->perm;
	if (is_delete)
		perm &= ~b_perm;
	else
		perm |= b_perm;
	*a_perm = perm;
	return !perm;
}

/**
 * ccs_write_inet_network - Write "struct ccs_inet_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_inet_network(struct ccs_acl_param *param)
{
	struct ccs_inet_acl e = { .head.type = CCS_TYPE_INET_ACL };
	int error = -EINVAL;
	u8 type;
	const char *protocol = ccs_read_token(param);
	const char *operation = ccs_read_token(param);
	for (e.protocol = 0; e.protocol < CCS_SOCK_MAX; e.protocol++)
		if (!strcmp(protocol, ccs_proto_keyword[e.protocol]))
			break;
	for (type = 0; type < CCS_MAX_NETWORK_OPERATION; type++)
		if (ccs_permstr(operation, ccs_socket_keyword[type]))
			e.perm |= 1 << type;
	if (e.protocol == CCS_SOCK_MAX || !e.perm)
		return -EINVAL;
	if (param->data[0] == '@') {
		param->data++;
		e.address.group = ccs_get_group(param, &ccs_address_group);
		if (!e.address.group)
			return -ENOMEM;
	} else {
		if (!ccs_parse_ipaddr_union(param, &e.address))
			goto out;
	}
	if (!ccs_parse_number_union(param, &e.port) ||
	    e.port.values[1] > 65535)
		goto out;
	error = ccs_update_domain(&e.head, sizeof(e), param, ccs_same_inet_acl,
				  ccs_merge_inet_acl);
out:
	ccs_put_group(e.address.group);
	ccs_put_number_union(&e.port);
	return error;
}

/**
 * ccs_write_unix_network - Write "struct ccs_unix_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_unix_network(struct ccs_acl_param *param)
{
	struct ccs_unix_acl e = { .head.type = CCS_TYPE_UNIX_ACL };
	int error;
	u8 type;
	const char *protocol = ccs_read_token(param);
	const char *operation = ccs_read_token(param);
	for (e.protocol = 0; e.protocol < CCS_SOCK_MAX; e.protocol++)
		if (!strcmp(protocol, ccs_proto_keyword[e.protocol]))
			break;
	for (type = 0; type < CCS_MAX_NETWORK_OPERATION; type++)
		if (ccs_permstr(operation, ccs_socket_keyword[type]))
			e.perm |= 1 << type;
	if (e.protocol == CCS_SOCK_MAX || !e.perm)
		return -EINVAL;
	if (!ccs_parse_name_union(param, &e.name))
		return -EINVAL;
	error = ccs_update_domain(&e.head, sizeof(e), param, ccs_same_unix_acl,
				  ccs_merge_unix_acl);
	ccs_put_name_union(&e.name);
	return error;
}

/**
 * ccs_same_capability_acl - Check for duplicated "struct ccs_capability_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_capability_acl(const struct ccs_acl_info *a,
				    const struct ccs_acl_info *b)
{
	const struct ccs_capability_acl *p1 = container_of(a, typeof(*p1),
							   head);
	const struct ccs_capability_acl *p2 = container_of(b, typeof(*p2),
							   head);
	return p1->operation == p2->operation;
}

/**
 * ccs_write_capability - Write "struct ccs_capability_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_capability(struct ccs_acl_param *param)
{
	struct ccs_capability_acl e = { .head.type = CCS_TYPE_CAPABILITY_ACL };
	const char *operation = ccs_read_token(param);
	for (e.operation = 0; e.operation < CCS_MAX_CAPABILITY_INDEX;
	     e.operation++) {
		if (strcmp(operation,
			   ccs_mac_keywords[ccs_c2mac[e.operation]]))
			continue;
		return ccs_update_domain(&e.head, sizeof(e), param,
					 ccs_same_capability_acl, NULL);
	}
	return -EINVAL;
}

/**
 * ccs_same_env_acl - Check for duplicated "struct ccs_env_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_env_acl(const struct ccs_acl_info *a,
			     const struct ccs_acl_info *b)
{
	const struct ccs_env_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_env_acl *p2 = container_of(b, typeof(*p2), head);
	return p1->env == p2->env;
}

/**
 * ccs_write_env - Write "struct ccs_env_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_env(struct ccs_acl_param *param)
{
	struct ccs_env_acl e = { .head.type = CCS_TYPE_ENV_ACL };
	int error = -ENOMEM;
	const char *data = ccs_read_token(param);
	if (!ccs_correct_word(data) || strchr(data, '='))
		return -EINVAL;
	e.env = ccs_get_name(data);
	error = ccs_update_domain(&e.head, sizeof(e), param,
				  ccs_same_env_acl, NULL);
	ccs_put_name(e.env);
	return error;
}

/**
 * ccs_write_misc - Update environment variable list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_misc(struct ccs_acl_param *param)
{
	if (ccs_str_starts(param->data, "env "))
		return ccs_write_env(param);
	return -EINVAL;
}

/**
 * ccs_same_signal_acl - Check for duplicated "struct ccs_signal_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_signal_acl(const struct ccs_acl_info *a,
				const struct ccs_acl_info *b)
{
	const struct ccs_signal_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_signal_acl *p2 = container_of(b, typeof(*p2), head);
	return ccs_same_number_union(&p1->sig, &p2->sig) &&
		p1->domainname == p2->domainname;
}

/**
 * ccs_write_ipc - Update "struct ccs_signal_acl" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_ipc(struct ccs_acl_param *param)
{
	struct ccs_signal_acl e = { .head.type = CCS_TYPE_SIGNAL_ACL };
	int error;
	if (!ccs_parse_number_union(param, &e.sig))
		return -EINVAL;
	e.domainname = ccs_get_domainname(param);
	if (!e.domainname)
		error = -EINVAL;
	else
		error = ccs_update_domain(&e.head, sizeof(e), param,
					  ccs_same_signal_acl, NULL);
	ccs_put_name(e.domainname);
	ccs_put_number_union(&e.sig);
	return error;
}


/**
 * ccs_same_reserved - Check for duplicated "struct ccs_reserved" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_reserved(const struct ccs_acl_head *a,
			      const struct ccs_acl_head *b)
{
	const struct ccs_reserved *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_reserved *p2 = container_of(b, typeof(*p2), head);
	return p1->ns == p2->ns && ccs_same_number_union(&p1->port, &p2->port);
}

/**
 * ccs_write_reserved_port - Update "struct ccs_reserved" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_reserved_port(struct ccs_acl_param *param)
{
	struct ccs_reserved e = { .ns = param->ns };
	int error;
	if (param->data[0] == '@' || !ccs_parse_number_union(param, &e.port) ||
	    e.port.values[1] > 65535 || param->data[0])
		return -EINVAL;
	param->list = &ccs_reserved_list;
	error = ccs_update_policy(&e.head, sizeof(e), param,
				  ccs_same_reserved);
	/*
	 * ccs_put_number_union() is not needed because param->data[0] != '@'.
	 */
	return error;
}

/**
 * ccs_print_namespace - Print namespace header.
 *
 * @ns: Pointer to "struct ccs_policy_namespace".
 *
 * Returns nothing.
 */
static void ccs_print_namespace(const struct ccs_policy_namespace *ns)
{
	if (!ccs_namespace_enabled)
		return;
	cprintf("%s ", ns->name);
}

/**
 * ccs_assign_profile - Create a new profile.
 *
 * @ns:      Pointer to "struct ccs_policy_namespace".
 * @profile: Profile number to create.
 *
 * Returns pointer to "struct ccs_profile" on success, NULL otherwise.
 */
static struct ccs_profile *ccs_assign_profile(struct ccs_policy_namespace *ns,
					      const unsigned int profile)
{
	struct ccs_profile *ptr;
	if (profile >= CCS_MAX_PROFILES)
		return NULL;
	ptr = ns->profile_ptr[profile];
	if (ptr)
		return ptr;
	ptr = ccs_malloc(sizeof(*ptr));
	ptr->default_config = CCS_CONFIG_DISABLED |
		CCS_CONFIG_WANT_GRANT_LOG | CCS_CONFIG_WANT_REJECT_LOG;
	memset(ptr->config, CCS_CONFIG_USE_DEFAULT,
	       sizeof(ptr->config));
	ptr->pref[CCS_PREF_MAX_AUDIT_LOG] = 1024;
	ptr->pref[CCS_PREF_MAX_LEARNING_ENTRY] = 2048;
	ns->profile_ptr[profile] = ptr;
	return ptr;
}

/**
 * ccs_find_yesno - Find values for specified keyword.
 *
 * @string: String to check.
 * @find:   Name of keyword.
 *
 * Returns 1 if "@find=yes" was found, 0 if "@find=no" was found, -1 otherwise.
 */
static s8 ccs_find_yesno(const char *string, const char *find)
{
	const char *cp = strstr(string, find);
	if (cp) {
		cp += strlen(find);
		if (!strncmp(cp, "=yes", 4))
			return 1;
		else if (!strncmp(cp, "=no", 3))
			return 0;
	}
	return -1;
}

/**
 * ccs_set_uint - Set value for specified preference.
 *
 * @i:      Pointer to "unsigned int".
 * @string: String to check.
 * @find:   Name of keyword.
 *
 * Returns nothing.
 */
static void ccs_set_uint(unsigned int *i, const char *string, const char *find)
{
	const char *cp = strstr(string, find);
	if (cp)
		sscanf(cp + strlen(find), "=%u", i);
}

/**
 * ccs_set_mode - Set mode for specified profile.
 *
 * @name:    Name of functionality.
 * @value:   Mode for @name.
 * @profile: Pointer to "struct ccs_profile".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_set_mode(char *name, const char *value,
			struct ccs_profile *profile)
{
	u8 i;
	u8 config;
	if (!strcmp(name, "CONFIG")) {
		i = CCS_MAX_MAC_INDEX + CCS_MAX_MAC_CATEGORY_INDEX;
		config = profile->default_config;
	} else if (ccs_str_starts(name, "CONFIG::")) {
		config = 0;
		for (i = 0; i < CCS_MAX_MAC_INDEX + CCS_MAX_MAC_CATEGORY_INDEX;
		     i++) {
			int len = 0;
			if (i < CCS_MAX_MAC_INDEX) {
				const u8 c = ccs_index2category[i];
				const char *category =
					ccs_category_keywords[c];
				len = strlen(category);
				if (strncmp(name, category, len) ||
				    name[len++] != ':' || name[len++] != ':')
					continue;
			}
			if (strcmp(name + len, ccs_mac_keywords[i]))
				continue;
			config = profile->config[i];
			break;
		}
		if (i == CCS_MAX_MAC_INDEX + CCS_MAX_MAC_CATEGORY_INDEX)
			return -EINVAL;
	} else {
		return -EINVAL;
	}
	if (strstr(value, "use_default")) {
		config = CCS_CONFIG_USE_DEFAULT;
	} else {
		u8 mode;
		for (mode = 0; mode < CCS_CONFIG_MAX_MODE; mode++)
			if (strstr(value, ccs_mode[mode]))
				/*
				 * Update lower 3 bits in order to distinguish
				 * 'config' from 'CCS_CONFIG_USE_DEAFULT'.
				 */
				config = (config & ~7) | mode;
		if (config != CCS_CONFIG_USE_DEFAULT) {
			switch (ccs_find_yesno(value, "grant_log")) {
			case 1:
				config |= CCS_CONFIG_WANT_GRANT_LOG;
				break;
			case 0:
				config &= ~CCS_CONFIG_WANT_GRANT_LOG;
				break;
			}
			switch (ccs_find_yesno(value, "reject_log")) {
			case 1:
				config |= CCS_CONFIG_WANT_REJECT_LOG;
				break;
			case 0:
				config &= ~CCS_CONFIG_WANT_REJECT_LOG;
				break;
			}
		}
	}
	if (i < CCS_MAX_MAC_INDEX + CCS_MAX_MAC_CATEGORY_INDEX)
		profile->config[i] = config;
	else if (config != CCS_CONFIG_USE_DEFAULT)
		profile->default_config = config;
	return 0;
}

/**
 * ccs_write_profile - Write profile table.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_profile(void)
{
	char *data = head.data;
	unsigned int i;
	char *cp;
	struct ccs_profile *profile;
	if (sscanf(data, "PROFILE_VERSION=%u", &head.ns->profile_version)
	    == 1)
		return 0;
	i = strtoul(data, &cp, 10);
	if (*cp != '-')
		return -EINVAL;
	data = cp + 1;
	profile = ccs_assign_profile(head.ns, i);
	if (!profile)
		return -EINVAL;
	cp = strchr(data, '=');
	if (!cp)
		return -EINVAL;
	*cp++ = '\0';
	if (!strcmp(data, "COMMENT")) {
		const struct ccs_path_info *new_comment = ccs_get_name(cp);
		const struct ccs_path_info *old_comment = profile->comment;
		profile->comment = new_comment;
		ccs_put_name(old_comment);
		return 0;
	}
	if (!strcmp(data, "PREFERENCE")) {
		for (i = 0; i < CCS_MAX_PREF; i++)
			ccs_set_uint(&profile->pref[i], cp,
				     ccs_pref_keywords[i]);
		return 0;
	}
	return ccs_set_mode(data, cp, profile);
}

/**
 * ccs_print_config - Print mode for specified functionality.
 *
 * @config: Mode for that functionality.
 *
 * Returns nothing.
 *
 * Caller prints functionality's name.
 */
static void ccs_print_config(const u8 config)
{
	cprintf("={ mode=%s grant_log=%s reject_log=%s }\n",
		ccs_mode[config & 3],
		ccs_yesno(config & CCS_CONFIG_WANT_GRANT_LOG),
		ccs_yesno(config & CCS_CONFIG_WANT_REJECT_LOG));
}

/**
 * ccs_read_profile - Read profile table.
 *
 * Returns nothing.
 */
static void ccs_read_profile(void)
{
	struct ccs_policy_namespace *ns;
	if (head.eof)
		return;
	list_for_each_entry(ns, &ccs_namespace_list, namespace_list) {
		u16 index;
		ccs_print_namespace(ns);
		cprintf("PROFILE_VERSION=%u\n", ns->profile_version);
		for (index = 0; index < CCS_MAX_PROFILES; index++) {
			u8 i;
			const struct ccs_profile *profile =
				ns->profile_ptr[index];
			if (!profile)
				continue;
			ccs_print_namespace(ns);
			cprintf("%u-COMMENT=%s\n", index, profile->comment ?
				profile->comment->name : "");
			ccs_print_namespace(ns);
			cprintf("%u-PREFERENCE={ ", index);
			for (i = 0; i < CCS_MAX_PREF; i++)
				cprintf("%s=%u ", ccs_pref_keywords[i],
					profile->pref[i]);
			cprintf("}\n");
			ccs_print_namespace(ns);
			cprintf("%u-CONFIG", index);
			ccs_print_config(profile->default_config);
			for (i = 0; i < CCS_MAX_MAC_INDEX
				     + CCS_MAX_MAC_CATEGORY_INDEX; i++) {
				const u8 config = profile->config[i];
				if (config == CCS_CONFIG_USE_DEFAULT)
					continue;
				ccs_print_namespace(ns);
				if (i < CCS_MAX_MAC_INDEX)
					cprintf("%u-CONFIG::%s::%s", index,
						ccs_category_keywords
						[ccs_index2category[i]],
						ccs_mac_keywords[i]);
				else
					cprintf("%u-CONFIG::%s", index,
						ccs_mac_keywords[i]);
				ccs_print_config(config);
			}
		}
	}
	head.eof = true;
}

/**
 * ccs_same_manager - Check for duplicated "struct ccs_manager" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_manager(const struct ccs_acl_head *a,
			     const struct ccs_acl_head *b)
{
	return container_of(a, struct ccs_manager, head)->manager
		== container_of(b, struct ccs_manager, head)->manager;
}

/**
 * ccs_update_manager_entry - Add a manager entry.
 *
 * @manager:   The path to manager or the domainnamme.
 * @is_delete: True if it is a delete request.
 *
 * Returns 0 on success, negative value otherwise.
 */
static inline int ccs_update_manager_entry(const char *manager,
					   const bool is_delete)
{
	struct ccs_manager e = { };
	struct ccs_acl_param param = {
		.is_delete = is_delete,
		.list = &ccs_manager_list,
	};
	int error = is_delete ? -ENOENT : -ENOMEM;
	if (ccs_domain_def(manager)) {
		if (!ccs_correct_domain(manager))
			return -EINVAL;
		e.is_domain = true;
	} else {
		if (!ccs_correct_path(manager))
			return -EINVAL;
	}
	e.manager = ccs_get_name(manager);
	error = ccs_update_policy(&e.head, sizeof(e), &param,
				  ccs_same_manager);
	ccs_put_name(e.manager);
	return error;
}

/**
 * ccs_write_manager - Write manager policy.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_manager(void)
{
	return ccs_update_manager_entry(head.data, head.is_delete);
}

/**
 * ccs_read_manager - Read manager policy.
 *
 * Returns nothing.
 */
static void ccs_read_manager(void)
{
	struct ccs_manager *ptr;
	if (head.eof)
		return;
	list_for_each_entry(ptr, &ccs_manager_list, head.list)
		if (!ptr->head.is_deleted)
			cprintf("%s\n", ptr->manager->name);
	head.eof = true;
}

/**
 * ccs_select_domain - Parse select command.
 *
 * @data: String to parse.
 *
 * Returns true on success, false otherwise.
 */
static bool ccs_select_domain(const char *data)
{
	struct ccs_domain2_info *domain = NULL;
	if (strncmp(data, "select ", 7))
		return false;
	data += 7;
	if (!strncmp(data, "domain=", 7)) {
		if (*(data + 7) == '<')
			domain = ccs_find_domain2(data + 7);
	} else
		return false;
	if (domain) {
		head.domain = domain;
		head.print_this_domain_only = domain;
	} else
		head.eof = true;
	cprintf("# select %s\n", data);
	return true;
}

/**
 * ccs_same_handler_acl - Check for duplicated "struct ccs_handler_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_handler_acl(const struct ccs_acl_info *a,
				 const struct ccs_acl_info *b)
{
	const struct ccs_handler_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_handler_acl *p2 = container_of(b, typeof(*p2), head);
	return p1->handler == p2->handler;
}

/**
 * ccs_same_task_acl - Check for duplicated "struct ccs_task_acl" entry.
 *
 * @a: Pointer to "struct ccs_acl_info".
 * @b: Pointer to "struct ccs_acl_info".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_task_acl(const struct ccs_acl_info *a,
			      const struct ccs_acl_info *b)
{
	const struct ccs_task_acl *p1 = container_of(a, typeof(*p1), head);
	const struct ccs_task_acl *p2 = container_of(b, typeof(*p2), head);
	return p1->domainname == p2->domainname;
}

/**
 * ccs_write_task - Update task related list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_task(struct ccs_acl_param *param)
{
	int error;
	const bool is_auto = ccs_str_starts(param->data,
					    "auto_domain_transition ");
	if (!is_auto && !ccs_str_starts(param->data,
					"manual_domain_transition ")) {
		struct ccs_handler_acl e = { };
		char *handler;
		if (ccs_str_starts(param->data, "auto_execute_handler "))
			e.head.type = CCS_TYPE_AUTO_EXECUTE_HANDLER;
		else if (ccs_str_starts(param->data,
					"denied_execute_handler "))
			e.head.type = CCS_TYPE_DENIED_EXECUTE_HANDLER;
		else
			return -EINVAL;
		handler = ccs_read_token(param);
		if (!ccs_correct_path(handler))
			return -EINVAL;
		e.handler = ccs_get_name(handler);
		if (e.handler->is_patterned)
			error = -EINVAL; /* No patterns allowed. */
		else
			error = ccs_update_domain(&e.head, sizeof(e), param,
						  ccs_same_handler_acl, NULL);
		ccs_put_name(e.handler);
	} else {
		struct ccs_task_acl e = {
			.head.type = is_auto ?
			CCS_TYPE_AUTO_TASK_ACL : CCS_TYPE_MANUAL_TASK_ACL,
			.domainname = ccs_get_domainname(param),
		};
		if (!e.domainname)
			error = -EINVAL;
		else
			error = ccs_update_domain(&e.head, sizeof(e), param,
						  ccs_same_task_acl, NULL);
		ccs_put_name(e.domainname);
	}
	return error;
}

/**
 * ccs_write_domain2 - Write domain policy.
 *
 * @ns:        Pointer to "struct ccs_policy_namespace".
 * @list:      Pointer to "struct list_head".
 * @data:      Policy to be interpreted.
 * @is_delete: True if it is a delete request.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_domain2(struct ccs_policy_namespace *ns,
			     struct list_head *list, char *data,
			     const bool is_delete)
{
	struct ccs_acl_param param = {
		.ns = ns,
		.list = list,
		.data = data,
		.is_delete = is_delete,
	};
	static const struct {
		const char *keyword;
		int (*write) (struct ccs_acl_param *);
	} ccs_callback[7] = {
		{ "file ", ccs_write_file },
		{ "network inet ", ccs_write_inet_network },
		{ "network unix ", ccs_write_unix_network },
		{ "misc ", ccs_write_misc },
		{ "capability ", ccs_write_capability },
		{ "ipc signal ", ccs_write_ipc },
		{ "task ", ccs_write_task },
	};
	u8 i;
	for (i = 0; i < 7; i++) {
		if (!ccs_str_starts(param.data, ccs_callback[i].keyword))
			continue;
		return ccs_callback[i].write(&param);
	}
	return -EINVAL;
}

/**
 * ccs_delete_domain2 - Delete a domain from domain policy.
 *
 * @domainname: Name of domain.
 *
 * Returns nothing.
 */
static void ccs_delete_domain2(const char *domainname)
{
	struct ccs_domain2_info *domain;
	list_for_each_entry(domain, &ccs_domain_list, list) {
		if (domain == &ccs_kernel_domain)
			continue;
		if (strcmp(domain->domainname->name, domainname))
			continue;
		domain->is_deleted = true;
	}
}

/**
 * ccs_write_domain - Write domain policy.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_domain(void)
{
	char *data = head.data;
	struct ccs_policy_namespace *ns;
	struct ccs_domain2_info *domain = head.domain;
	const bool is_delete = head.is_delete;
	const bool is_select = !is_delete && ccs_str_starts(data, "select ");
	unsigned int profile;
	if (*data == '<') {
		domain = NULL;
		if (is_delete)
			ccs_delete_domain2(data);
		else if (is_select)
			domain = ccs_find_domain2(data);
		else
			domain = ccs_assign_domain2(data);
		head.domain = domain;
		return 0;
	}
	if (!domain)
		return -EINVAL;
	ns = ccs_assign_namespace(domain->domainname->name);
	if (!ns)
		return -EINVAL;
	if (sscanf(data, "use_profile %u\n", &profile) == 1
	    && profile < CCS_MAX_PROFILES) {
		if (ns->profile_ptr[(u8) profile])
			if (!is_delete)
				domain->profile = (u8) profile;
		return 0;
	}
	if (sscanf(data, "use_group %u\n", &profile) == 1
	    && profile < CCS_MAX_ACL_GROUPS) {
		if (!is_delete)
			domain->group = (u8) profile;
		return 0;
	}
	for (profile = 0; profile < CCS_MAX_DOMAIN_INFO_FLAGS; profile++) {
		const char *cp = ccs_dif[profile];
		if (strncmp(data, cp, strlen(cp) - 1))
			continue;
		domain->flags[profile] = !is_delete;
		return 0;
	}
	return ccs_write_domain2(ns, &domain->acl_info_list, data, is_delete);
}

/**
 * ccs_print_name_union - Print a ccs_name_union.
 *
 * @ptr: Pointer to "struct ccs_name_union".
 *
 * Returns nothing.
 */
static void ccs_print_name_union(const struct ccs_name_union *ptr)
{
	if (ptr->group)
		cprintf(" @%s", ptr->group->group_name->name);
	else
		cprintf(" %s", ptr->filename->name);
}

/**
 * ccs_print_name_union_quoted - Print a ccs_name_union with a quote.
 *
 * @ptr: Pointer to "struct ccs_name_union".
 *
 * Returns nothing.
 */
static void ccs_print_name_union_quoted(const struct ccs_name_union *ptr)
{
	if (ptr->group)
		cprintf("@%s", ptr->group->group_name->name);
	else
		cprintf("\"%s\"", ptr->filename->name);
}

/**
 * ccs_print_number_union_nospace - Print a ccs_number_union without a space.
 *
 * @ptr: Pointer to "struct ccs_number_union".
 *
 * Returns nothing.
 */
static void ccs_print_number_union_nospace(const struct ccs_number_union *ptr)
{
	if (ptr->group) {
		cprintf("@%s", ptr->group->group_name->name);
	} else {
		int i;
		unsigned long min = ptr->values[0];
		const unsigned long max = ptr->values[1];
		u8 min_type = ptr->value_type[0];
		const u8 max_type = ptr->value_type[1];
		for (i = 0; i < 2; i++) {
			switch (min_type) {
			case CCS_VALUE_TYPE_HEXADECIMAL:
				cprintf("0x%lX", min);
				break;
			case CCS_VALUE_TYPE_OCTAL:
				cprintf("0%lo", min);
				break;
			default:
				cprintf("%lu", min);
				break;
			}
			if (min == max && min_type == max_type)
				break;
			cprintf("-");
			min_type = max_type;
			min = max;
		}
	}
}

/**
 * ccs_print_number_union - Print a ccs_number_union.
 *
 * @ptr: Pointer to "struct ccs_number_union".
 *
 * Returns nothing.
 */
static void ccs_print_number_union(const struct ccs_number_union *ptr)
{
	cprintf(" ");
	ccs_print_number_union_nospace(ptr);
}

/**
 * ccs_print_condition - Print condition part.
 *
 * @cond: Pointer to "struct ccs_condition".
 *
 * Returns true on success, false otherwise.
 */
static void ccs_print_condition(const struct ccs_condition *cond)
{
	const u16 condc = cond->condc;
	const struct ccs_condition_element *condp = (typeof(condp)) (cond + 1);
	const struct ccs_number_union *numbers_p =
		(typeof(numbers_p)) (condp + condc);
	const struct ccs_name_union *names_p =
		(typeof(names_p)) (numbers_p + cond->numbers_count);
	const struct ccs_argv *argv =
		(typeof(argv)) (names_p + cond->names_count);
	const struct ccs_envp *envp = (typeof(envp)) (argv + cond->argc);
	u16 i;
	for (i = 0; i < condc; i++) {
		const u8 match = condp->equals;
		const u8 left = condp->left;
		const u8 right = condp->right;
		condp++;
		cprintf(" ");
		switch (left) {
		case CCS_ARGV_ENTRY:
			cprintf("exec.argv[%lu]%s=\"%s\"", argv->index,
				argv->is_not ? "!" : "", argv->value->name);
			argv++;
			continue;
		case CCS_ENVP_ENTRY:
			cprintf("exec.envp[\"%s\"]%s=",
				envp->name->name, envp->is_not ? "!" : "");
			if (envp->value)
				cprintf("\"%s\"", envp->value->name);
			else
				cprintf("NULL");
			envp++;
			continue;
		case CCS_NUMBER_UNION:
			ccs_print_number_union_nospace(numbers_p++);
			break;
		default:
			cprintf("%s", ccs_condition_keyword[left]);
			break;
		}
		cprintf("%s", match ? "=" : "!=");
		switch (right) {
		case CCS_NAME_UNION:
			ccs_print_name_union_quoted(names_p++);
			break;
		case CCS_NUMBER_UNION:
			ccs_print_number_union_nospace(numbers_p++);
			break;
		default:
			cprintf("%s", ccs_condition_keyword[right]);
			break;
		}
	}
	if (cond->grant_log != CCS_GRANTLOG_AUTO)
		cprintf(" grant_log=%s",
			ccs_yesno(cond->grant_log == CCS_GRANTLOG_YES));
	if (cond->transit)
		cprintf(" auto_domain_transition=\"%s\"",
			cond->transit->name);
}

/**
 * ccs_set_group - Print "acl_group " header keyword and category name.
 *
 * @category: Category name.
 *
 * Returns nothing.
 */
static void ccs_set_group(const char *category)
{
	if (head.type == CCS_EXCEPTIONPOLICY) {
		ccs_print_namespace(head.ns);
		cprintf("acl_group %u ", head.acl_group_index);
	}
	cprintf("%s", category);
}

/**
 * ccs_print_entry - Print an ACL entry.
 *
 * @acl: Pointer to an ACL entry.
 *
 * Returns nothing.
 */
static void ccs_print_entry(const struct ccs_acl_info *acl)
{
	const u8 acl_type = acl->type;
	const bool may_trigger_transition = acl->cond && acl->cond->transit;
	bool first = true;
	u8 bit;
	if (acl->is_deleted)
		return;
	if (acl_type == CCS_TYPE_PATH_ACL) {
		struct ccs_path_acl *ptr
			= container_of(acl, typeof(*ptr), head);
		const u16 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_PATH_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (head.print_transition_related_only &&
			    bit != CCS_TYPE_EXECUTE && !may_trigger_transition)
				continue;
			if (first) {
				ccs_set_group("file ");
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_path_keyword[bit]);
		}
		if (first)
			return;
		ccs_print_name_union(&ptr->name);
	} else if (acl_type == CCS_TYPE_AUTO_EXECUTE_HANDLER ||
		   acl_type == CCS_TYPE_DENIED_EXECUTE_HANDLER) {
		struct ccs_handler_acl *ptr
			= container_of(acl, typeof(*ptr), head);
		ccs_set_group("task ");
		cprintf(acl_type == CCS_TYPE_AUTO_EXECUTE_HANDLER ?
			"auto_execute_handler " : "denied_execute_handler ");
		cprintf("%s", ptr->handler->name);
	} else if (acl_type == CCS_TYPE_AUTO_TASK_ACL ||
		   acl_type == CCS_TYPE_MANUAL_TASK_ACL) {
		struct ccs_task_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		ccs_set_group("task ");
		cprintf(acl_type == CCS_TYPE_AUTO_TASK_ACL ?
			"auto_domain_transition " :
			"manual_domain_transition ");
		cprintf("%s", ptr->domainname->name);
	} else if (head.print_transition_related_only &&
		   !may_trigger_transition) {
		return;
	} else if (acl_type == CCS_TYPE_MKDEV_ACL) {
		struct ccs_mkdev_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		const u8 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_MKDEV_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (first) {
				ccs_set_group("file ");
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_mac_keywords[ccs_pnnn2mac[bit]]);
		}
		if (first)
			return;
		ccs_print_name_union(&ptr->name);
		ccs_print_number_union(&ptr->mode);
		ccs_print_number_union(&ptr->major);
		ccs_print_number_union(&ptr->minor);
	} else if (acl_type == CCS_TYPE_PATH2_ACL) {
		struct ccs_path2_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		const u8 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_PATH2_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (first) {
				ccs_set_group("file ");
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_mac_keywords[ccs_pp2mac[bit]]);
		}
		if (first)
			return;
		ccs_print_name_union(&ptr->name1);
		ccs_print_name_union(&ptr->name2);
	} else if (acl_type == CCS_TYPE_PATH_NUMBER_ACL) {
		struct ccs_path_number_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		const u8 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_PATH_NUMBER_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (first) {
				ccs_set_group("file ");
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_mac_keywords[ccs_pn2mac[bit]]);
		}
		if (first)
			return;
		ccs_print_name_union(&ptr->name);
		ccs_print_number_union(&ptr->number);
	} else if (acl_type == CCS_TYPE_ENV_ACL) {
		struct ccs_env_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		ccs_set_group("misc env ");
		cprintf("%s", ptr->env->name);
	} else if (acl_type == CCS_TYPE_CAPABILITY_ACL) {
		struct ccs_capability_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		ccs_set_group("capability ");
		cprintf("%s", ccs_mac_keywords[ccs_c2mac[ptr->operation]]);
	} else if (acl_type == CCS_TYPE_INET_ACL) {
		struct ccs_inet_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		const u8 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_NETWORK_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (first) {
				ccs_set_group("network inet ");
				cprintf("%s ",
					ccs_proto_keyword[ptr->protocol]);
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_socket_keyword[bit]);
		}
		if (first)
			return;
		cprintf(" ");
		if (ptr->address.group)
			cprintf("@%s", ptr->address.group->group_name->name);
		else
			ccs_print_ip(&ptr->address);
		ccs_print_number_union(&ptr->port);
	} else if (acl_type == CCS_TYPE_UNIX_ACL) {
		struct ccs_unix_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		const u8 perm = ptr->perm;
		for (bit = 0; bit < CCS_MAX_NETWORK_OPERATION; bit++) {
			if (!(perm & (1 << bit)))
				continue;
			if (first) {
				ccs_set_group("network unix ");
				cprintf("%s ",
					ccs_proto_keyword[ptr->protocol]);
				first = false;
			} else {
				cprintf("/");
			}
			cprintf("%s", ccs_socket_keyword[bit]);
		}
		if (first)
			return;
		ccs_print_name_union(&ptr->name);
	} else if (acl_type == CCS_TYPE_SIGNAL_ACL) {
		struct ccs_signal_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		ccs_set_group("ipc signal ");
		ccs_print_number_union_nospace(&ptr->sig);
		cprintf(" %s", ptr->domainname->name);
	} else if (acl_type == CCS_TYPE_MOUNT_ACL) {
		struct ccs_mount_acl *ptr =
			container_of(acl, typeof(*ptr), head);
		ccs_set_group("file mount");
		ccs_print_name_union(&ptr->dev_name);
		ccs_print_name_union(&ptr->dir_name);
		ccs_print_name_union(&ptr->fs_type);
		ccs_print_number_union(&ptr->flags);
	}
	if (acl->cond)
		ccs_print_condition(acl->cond);
	cprintf("\n");
}

/**
 * ccs_read_domain2 - Read domain policy.
 *
 * @list: Pointer to "struct list_head".
 *
 * Returns nothing.
 */
static void ccs_read_domain2(struct list_head *list)
{
	struct ccs_acl_info *ptr;
	list_for_each_entry(ptr, list, list)
		ccs_print_entry(ptr);
}

/**
 * ccs_read_domain - Read domain policy.
 *
 * Returns nothing.
 */
static void ccs_read_domain(void)
{
	struct ccs_domain2_info *domain;
	if (head.eof)
		return;
	list_for_each_entry(domain, &ccs_domain_list, list) {
		u8 i;
		if (domain->is_deleted)
			continue;
		if (head.print_this_domain_only &&
		    head.print_this_domain_only != domain)
			continue;
		/* Print domainname and flags. */
		cprintf("%s\n", domain->domainname->name);
		cprintf("use_profile %u\n", domain->profile);
		cprintf("use_group %u\n", domain->group);
		for (i = 0; i < CCS_MAX_DOMAIN_INFO_FLAGS; i++)
			if (domain->flags[i])
				cprintf("%s", ccs_dif[i]);
		cprintf("\n");
		ccs_read_domain2(&domain->acl_info_list);
		cprintf("\n");
	}
	head.eof = true;
}

/**
 * ccs_same_path_group - Check for duplicated "struct ccs_path_group" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_path_group(const struct ccs_acl_head *a,
				const struct ccs_acl_head *b)
{
	return container_of(a, struct ccs_path_group, head)->member_name ==
		container_of(b, struct ccs_path_group, head)->member_name;
}

/**
 * ccs_same_number_group - Check for duplicated "struct ccs_number_group" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_number_group(const struct ccs_acl_head *a,
				  const struct ccs_acl_head *b)
{
	return !memcmp(&container_of(a, struct ccs_number_group, head)->number,
		       &container_of(b, struct ccs_number_group, head)->number,
		       sizeof(container_of(a, struct ccs_number_group, head)
			      ->number));
}

/**
 * ccs_same_address_group - Check for duplicated "struct ccs_address_group" entry.
 *
 * @a: Pointer to "struct ccs_acl_head".
 * @b: Pointer to "struct ccs_acl_head".
 *
 * Returns true if @a == @b, false otherwise.
 */
static bool ccs_same_address_group(const struct ccs_acl_head *a,
				   const struct ccs_acl_head *b)
{
	const struct ccs_address_group *p1 = container_of(a, typeof(*p1),
							  head);
	const struct ccs_address_group *p2 = container_of(b, typeof(*p2),
							  head);
	return ccs_same_ipaddr_union(&p1->address, &p2->address);
}

/**
 * ccs_write_group - Write "struct ccs_path_group"/"struct ccs_number_group"/"struct ccs_address_group" list.
 *
 * @param: Pointer to "struct ccs_acl_param".
 * @type:  Type of this group.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_group(struct ccs_acl_param *param, const u8 type)
{
	struct ccs_group *group = ccs_get_group(param, type == CCS_PATH_GROUP ?
						&ccs_path_group :
						type == CCS_NUMBER_GROUP ?
						&ccs_number_group :
						&ccs_address_group);
	int error = -EINVAL;
	if (!group)
		return -ENOMEM;
	param->list = &group->member_list;
	if (type == CCS_PATH_GROUP) {
		struct ccs_path_group e = { };
		e.member_name = ccs_get_name(ccs_read_token(param));
		error = ccs_update_policy(&e.head, sizeof(e), param,
					  ccs_same_path_group);
		ccs_put_name(e.member_name);
	} else if (type == CCS_NUMBER_GROUP) {
		struct ccs_number_group e = { };
		if (param->data[0] == '@' ||
		    !ccs_parse_number_union(param, &e.number))
			goto out;
		error = ccs_update_policy(&e.head, sizeof(e), param,
					  ccs_same_number_group);
		/*
		 * ccs_put_number_union() is not needed because
		 * param->data[0] != '@'.
		 */
	} else {
		struct ccs_address_group e = { };
		if (param->data[0] == '@' ||
		    !ccs_parse_ipaddr_union(param, &e.address))
			goto out;
		error = ccs_update_policy(&e.head, sizeof(e), param,
					  ccs_same_address_group);
	}
out:
	ccs_put_group(group);
	return error;
}

/**
 * ccs_write_exception - Write exception policy.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_write_exception(void)
{
	const bool is_delete = head.is_delete;
	struct ccs_acl_param param = {
		.ns = head.ns,
		.is_delete = is_delete,
		.data = head.data,
	};
	u8 i;
	if (ccs_str_starts(param.data, "aggregator "))
		return ccs_write_aggregator(&param);
	if (ccs_str_starts(param.data, "deny_autobind "))
		return ccs_write_reserved_port(&param);
	for (i = 0; i < CCS_MAX_TRANSITION_TYPE; i++)
		if (ccs_str_starts(param.data, ccs_transition_type[i]))
			return ccs_write_transition_control(&param, i);
	for (i = 0; i < CCS_MAX_GROUP; i++)
		if (ccs_str_starts(param.data, ccs_group_name[i]))
			return ccs_write_group(&param, i);
	if (ccs_str_starts(param.data, "acl_group ")) {
		unsigned int group;
		char *data;
		group = strtoul(param.data, &data, 10);
		if (group < CCS_MAX_ACL_GROUPS && *data++ == ' ')
			return ccs_write_domain2(head.ns,
						 &head.ns->acl_group[group],
						 data, is_delete);
	}
	return -EINVAL;
}

/**
 * ccs_read_group - Read "struct ccs_path_group"/"struct ccs_number_group"/"struct ccs_address_group" list.
 *
 * @ns: Pointer to "struct ccs_policy_namespace".
 *
 * Returns nothing.
 */
static void ccs_read_group(const struct ccs_policy_namespace *ns)
{
	struct ccs_group *group;
	list_for_each_entry(group, &ccs_path_group, head.list) {
		struct ccs_acl_head *ptr;
		list_for_each_entry(ptr, &group->member_list, list) {
			if (group->ns != ns)
				continue;
			if (ptr->is_deleted)
				continue;
			ccs_print_namespace(group->ns);
			cprintf("%s%s", ccs_group_name[CCS_PATH_GROUP],
				group->group_name->name);
			cprintf(" %s",
				container_of(ptr, struct ccs_path_group,
					     head)->member_name->name);
			cprintf("\n");
		}
	}
	list_for_each_entry(group, &ccs_number_group, head.list) {
		struct ccs_acl_head *ptr;
		list_for_each_entry(ptr, &group->member_list, list) {
			if (group->ns != ns)
				continue;
			if (ptr->is_deleted)
				continue;
			ccs_print_namespace(group->ns);
			cprintf("%s%s", ccs_group_name[CCS_NUMBER_GROUP],
				group->group_name->name);
			ccs_print_number_union(&container_of
					       (ptr, struct ccs_number_group,
						head)->number);
			cprintf("\n");
		}
	}
	list_for_each_entry(group, &ccs_address_group, head.list) {
		struct ccs_acl_head *ptr;
		list_for_each_entry(ptr, &group->member_list, list) {
			if (group->ns != ns)
				continue;
			if (ptr->is_deleted)
				continue;
			ccs_print_namespace(group->ns);
			cprintf("%s%s", ccs_group_name[CCS_ADDRESS_GROUP],
				group->group_name->name);
			cprintf(" ");
			ccs_print_ip(&container_of
				     (ptr, struct ccs_address_group, head)->
				     address);
			cprintf("\n");
		}
	}
}

/**
 * ccs_read_policy - Read "struct ccs_..._entry" list.
 *
 * @ns: Pointer to "struct ccs_policy_namespace".
 *
 * Returns nothing.
 */
static void ccs_read_policy(const struct ccs_policy_namespace *ns)
{
	struct ccs_acl_head *acl;
	if (head.print_transition_related_only)
		goto transition_only;
	list_for_each_entry(acl, &ccs_reserved_list, list) {
		struct ccs_reserved *ptr =
			container_of(acl, typeof(*ptr), head);
		if (acl->is_deleted || ptr->ns != ns)
			continue;
		ccs_print_namespace(ptr->ns);
		cprintf("deny_autobind ");
		ccs_print_number_union_nospace(&ptr->port);
		cprintf("\n");
	}
	list_for_each_entry(acl, &ccs_aggregator_list, list) {
		struct ccs_aggregator *ptr =
			container_of(acl, typeof(*ptr), head);
		if (acl->is_deleted || ptr->ns != ns)
			continue;
		ccs_print_namespace(ptr->ns);
		cprintf("aggregator %s %s\n", ptr->original_name->name,
			ptr->aggregated_name->name);
	}
transition_only:
	list_for_each_entry(acl, &ccs_transition_list, list) {
		struct ccs_transition_control *ptr =
			container_of(acl, typeof(*ptr), head);
		if (acl->is_deleted || ptr->ns != ns)
			continue;
		ccs_print_namespace(ptr->ns);
		cprintf("%s%s from %s\n", ccs_transition_type[ptr->type],
			ptr->program ? ptr->program->name : "any",
			ptr->domainname ? ptr->domainname->name : "any");
	}
}

/**
 * ccs_read_exception - Read exception policy.
 *
 * Returns nothing.
 */
static void ccs_read_exception(void)
{
	struct ccs_policy_namespace *ns;
	if (head.eof)
		return;
	list_for_each_entry(ns, &ccs_namespace_list, namespace_list) {
		unsigned int i;
		head.ns = ns;
		ccs_read_policy(ns);
		ccs_read_group(ns);
		for (i = 0; i < CCS_MAX_ACL_GROUPS; i++)
			ccs_read_domain2(&ns->acl_group[i]);
	}
	head.eof = true;
}

/**
 * ccs_read_stat - Read statistic data.
 *
 * Returns nothing.
 */
static void ccs_read_stat(void)
{
	u8 i;
	if (head.eof)
		return;
	for (i = 0; i < CCS_MAX_MEMORY_STAT; i++)
		cprintf("Memory used by %-22s %10u\n", ccs_memory_headers[i],
			ccs_memory_quota[i]);
	head.eof = true;
}

/**
 * ccs_write_stat - Set memory quota.
 *
 * Returns 0.
 */
static int ccs_write_stat(void)
{
	char *data = head.data;
	u8 i;
	if (ccs_str_starts(data, "Memory used by "))
		for (i = 0; i < CCS_MAX_MEMORY_STAT; i++)
			if (ccs_str_starts(data, ccs_memory_headers[i]))
				ccs_memory_quota[i] = strtoul(data, NULL, 10);
	return 0;
}

/**
 * ccs_parse_policy - Parse a policy line.
 *
 * @line: Line to parse.
 *
 * Returns 0 on success, negative value otherwise.
 */
static int ccs_parse_policy(char *line)
{
	/* Delete request? */
	head.is_delete = !strncmp(line, "delete ", 7);
	if (head.is_delete)
		memmove(line, line + 7, strlen(line + 7) + 1);
	/* Selecting namespace to update. */
	if (head.type == CCS_EXCEPTIONPOLICY || head.type == CCS_PROFILE) {
		if (*line == '<') {
			char *cp = strchr(line, ' ');
			if (cp) {
				*cp++ = '\0';
				head.ns = ccs_assign_namespace(line);
				memmove(line, cp, strlen(cp) + 1);
			} else
				head.ns = NULL;
			/* Don't allow updating if namespace is invalid. */
			if (!head.ns)
				return -ENOENT;
		} else
			head.ns = &ccs_kernel_namespace;
	}
	/* Do the update. */
	switch (head.type) {
	case CCS_DOMAINPOLICY:
		return ccs_write_domain();
	case CCS_EXCEPTIONPOLICY:
		return ccs_write_exception();
	case CCS_STAT:
		return ccs_write_stat();
	case CCS_PROFILE:
		return ccs_write_profile();
	case CCS_MANAGER:
		return ccs_write_manager();
	default:
		return -ENOSYS;
	}
}

/**
 * ccs_write_control - write() for /proc/ccs/ interface.
 *
 * @buffer:     Pointer to buffer to read from.
 * @buffer_len: Size of @buffer.
 *
 * Returns @buffer_len on success, negative value otherwise.
 */
static void ccs_write_control(char *buffer, const size_t buffer_len)
{
	static char *line = NULL;
	static int line_len = 0;
	size_t avail_len = buffer_len;
	while (avail_len > 0) {
		const char c = *buffer++;
		avail_len--;
		line = ccs_realloc(line, line_len + 1);
		line[line_len++] = c;
		if (c != '\n')
			continue;
		line[line_len - 1] = '\0';
		line_len = 0;
		head.data = line;
		ccs_normalize_line(line);
		if (!strcmp(line, "reset")) {
			const u8 type = head.type;
			memset(&head, 0, sizeof(head));
			head.reset = true;
			head.type = type;
			continue;
		}
		/* Don't allow updating policies by non manager programs. */
		switch (head.type) {
		case CCS_DOMAINPOLICY:
			if (ccs_select_domain(line))
				continue;
			/* fall through */
		case CCS_EXCEPTIONPOLICY:
			if (!strcmp(line, "select transition_only")) {
				head.print_transition_related_only = true;
				continue;
			}
		}
		ccs_parse_policy(line);
	}
}

/**
 * ccs_editpolicy_offline_init - Initialize variables for offline daemon.
 *
 * Returns nothing.
 */
static void ccs_editpolicy_offline_init(coid)
{
	static _Bool first = true;
	int i;
	if (!first)
		return;
	first = false;
	memset(&head, 0, sizeof(head));
	memset(&ccs_kernel_domain, 0, sizeof(ccs_kernel_domain));
	memset(&ccs_kernel_namespace, 0, sizeof(ccs_kernel_namespace));
	ccs_namespace_enabled = false;
	ccs_kernel_namespace.name = "<kernel>";
	for (i = 0; i < CCS_MAX_ACL_GROUPS; i++)
		INIT_LIST_HEAD(&ccs_kernel_namespace.acl_group[i]);
	list_add_tail(&ccs_kernel_namespace.namespace_list,
		      &ccs_namespace_list);
	for (i = 0; i < CCS_MAX_HASH; i++)
		INIT_LIST_HEAD(&ccs_name_list[i]);
	INIT_LIST_HEAD(&ccs_kernel_domain.acl_info_list);
	ccs_kernel_domain.domainname = ccs_savename("<kernel>");
	list_add_tail(&ccs_kernel_domain.list, &ccs_domain_list);
	memset(ccs_memory_quota, 0, sizeof(ccs_memory_quota));
}

/**
 * ccs_editpolicy_offline_main - Read request and handle policy I/O.
 *
 * @fd: Socket file descriptor. 
 *
 * Returns nothing.
 */
static void ccs_editpolicy_offline_main(const int fd)
{
	int i;
	static char buffer[4096];
	ccs_editpolicy_offline_init();
	/* Read filename. */
	for (i = 0; i < sizeof(buffer); i++) {
		if (read(fd, buffer + i, 1) != 1)
			return;
		if (!buffer[i])
			break;
	}
	if (!memchr(buffer, '\0', sizeof(buffer)))
		return;
	memset(&head, 0, sizeof(head));
	head.reset = true;
	if (!strcmp(buffer, CCS_PROC_POLICY_DOMAIN_POLICY))
		head.type = CCS_DOMAINPOLICY;
	else if (!strcmp(buffer, CCS_PROC_POLICY_EXCEPTION_POLICY))
		head.type = CCS_EXCEPTIONPOLICY;
	else if (!strcmp(buffer, CCS_PROC_POLICY_PROFILE))
		head.type = CCS_PROFILE;
	else if (!strcmp(buffer, CCS_PROC_POLICY_MANAGER))
		head.type = CCS_MANAGER;
	else if (!strcmp(buffer, CCS_PROC_POLICY_STAT))
		head.type = CCS_STAT;
	else
		return;
	/* Return \0 to indicate success. */
	if (write(fd, "", 1) != 1)
		return;
	client_fd = fd;
	while (1) {
		struct pollfd pfd = { .fd = fd, .events = POLLIN };
		int len;
		int nonzero_len;
		poll(&pfd, 1, -1);
		len = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
		if (len <= 0)
			break;
restart:
		for (nonzero_len = 0 ; nonzero_len < len; nonzero_len++)
			if (!buffer[nonzero_len])
				break;
		if (nonzero_len) {
			ccs_write_control(buffer, nonzero_len);
		} else {
			switch (head.type) {
			case CCS_DOMAINPOLICY:
				ccs_read_domain();
				break;
			case CCS_EXCEPTIONPOLICY:
				ccs_read_exception();
				break;
			case CCS_STAT:
				ccs_read_stat();
				break;
			case CCS_PROFILE:
				ccs_read_profile();
				break;
			case CCS_MANAGER:
				ccs_read_manager();
				break;
			}
			/* Flush data. */
			cprintf("%s", "");
			/* Return \0 to indicate EOF. */
			if (write(fd, "", 1) != 1)
				return;
			nonzero_len = 1;
		}
		len -= nonzero_len;
		memmove(buffer, buffer + nonzero_len, len);
		if (len)
			goto restart;
	}
}

/**
 * ccs_editpolicy_offline_daemon - Emulate /proc/ccs/ interface.
 *
 * @listener: Listener fd. This is a listening PF_INET socket.
 * @notifier: Notifier fd. This is a pipe's reader side.
 *
 * This function does not return.
 */
void ccs_editpolicy_offline_daemon(const int listener, const int notifier)
{
	while (1) {
		struct pollfd pfd[2] = {
			{ .fd = listener, .events = POLLIN },
			{ .fd = notifier, .events = POLLIN }
		};
		struct sockaddr_in addr;
		socklen_t size = sizeof(addr);
		int fd;
		if (poll(pfd, 2, -1) == EOF ||
		    (pfd[1].revents & (POLLIN | POLLHUP)))
			_exit(1);
		fd = accept(listener, (struct sockaddr *) &addr, &size);
		if (fd == EOF)
			continue;
		ccs_editpolicy_offline_main(fd);
		close(fd);
	}
}
