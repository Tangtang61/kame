/* $NetBSD: linux_syscallargs.h,v 1.44.4.1 2003/10/22 04:03:00 jmc Exp $ */

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.63 2002/04/10 18:18:27 christos Exp 
 */

#ifndef _LINUX_SYS__SYSCALLARGS_H_
#define	_LINUX_SYS__SYSCALLARGS_H_

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register_t pad;						\
		struct { x datum; } le;					\
		struct { /* LINTED zero array dimension */		\
			int8_t pad[  /* CONSTCOND */			\
				(sizeof (register_t) < sizeof (x))	\
				? 0					\
				: sizeof (register_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct linux_sys_open_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct linux_sys_waitpid_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
};

struct linux_sys_creat_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_link_args {
	syscallarg(const char *) path;
	syscallarg(const char *) link;
};

struct linux_sys_unlink_args {
	syscallarg(const char *) path;
};

struct linux_sys_execve_args {
	syscallarg(const char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct linux_sys_chdir_args {
	syscallarg(const char *) path;
};

struct linux_sys_time_args {
	syscallarg(linux_time_t *) t;
};

struct linux_sys_mknod_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct linux_sys_chmod_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_lchown16_args {
	syscallarg(const char *) path;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_break_args {
	syscallarg(char *) nsize;
};

struct linux_sys_stime_args {
	syscallarg(linux_time_t *) t;
};

struct linux_sys_ptrace_args {
	syscallarg(int) request;
	syscallarg(int) pid;
	syscallarg(int) addr;
	syscallarg(int) data;
};

struct linux_sys_alarm_args {
	syscallarg(unsigned int) secs;
};

struct linux_sys_utime_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_utimbuf *) times;
};

struct linux_sys_access_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
};

struct linux_sys_nice_args {
	syscallarg(int) incr;
};

struct linux_sys_kill_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
};

struct linux_sys_rename_args {
	syscallarg(const char *) from;
	syscallarg(const char *) to;
};

struct linux_sys_mkdir_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
};

struct linux_sys_rmdir_args {
	syscallarg(const char *) path;
};

struct linux_sys_pipe_args {
	syscallarg(int *) pfds;
};

struct linux_sys_times_args {
	syscallarg(struct times *) tms;
};

struct linux_sys_brk_args {
	syscallarg(char *) nsize;
};

struct linux_sys_signal_args {
	syscallarg(int) signum;
	syscallarg(linux_handler_t) handler;
};

struct linux_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(u_long) com;
	syscallarg(caddr_t) data;
};

struct linux_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct linux_sys_oldolduname_args {
	syscallarg(struct linux_oldold_utsname *) up;
};

struct linux_sys_sigaction_args {
	syscallarg(int) signum;
	syscallarg(const struct linux_old_sigaction *) nsa;
	syscallarg(struct linux_old_sigaction *) osa;
};

struct linux_sys_sigsetmask_args {
	syscallarg(linux_old_sigset_t) mask;
};

struct linux_sys_setreuid16_args {
	syscallarg(int) ruid;
	syscallarg(int) euid;
};

struct linux_sys_setregid16_args {
	syscallarg(int) rgid;
	syscallarg(int) egid;
};

struct linux_sys_sigsuspend_args {
	syscallarg(caddr_t) restart;
	syscallarg(int) oldmask;
	syscallarg(int) mask;
};

struct linux_sys_sigpending_args {
	syscallarg(linux_old_sigset_t *) set;
};

struct linux_sys_setrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct orlimit *) rlp;
};

struct linux_sys_getrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct orlimit *) rlp;
};

struct linux_sys_gettimeofday_args {
	syscallarg(struct timeval *) tp;
	syscallarg(struct timezone *) tzp;
};

struct linux_sys_settimeofday_args {
	syscallarg(struct timeval *) tp;
	syscallarg(struct timezone *) tzp;
};

struct linux_sys_getgroups16_args {
	syscallarg(int) gidsetsize;
	syscallarg(linux_gid_t *) gidset;
};

struct linux_sys_setgroups16_args {
	syscallarg(int) gidsetsize;
	syscallarg(linux_gid_t *) gidset;
};

struct linux_sys_oldselect_args {
	syscallarg(struct linux_oldselect *) lsp;
};

struct linux_sys_symlink_args {
	syscallarg(const char *) path;
	syscallarg(const char *) to;
};

struct linux_sys_readlink_args {
	syscallarg(const char *) name;
	syscallarg(char *) buf;
	syscallarg(int) count;
};

struct linux_sys_uselib_args {
	syscallarg(const char *) path;
};

struct linux_sys_swapon_args {
	syscallarg(char *) name;
};

struct linux_sys_reboot_args {
	syscallarg(int) magic1;
	syscallarg(int) magic2;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct linux_sys_readdir_args {
	syscallarg(int) fd;
	syscallarg(caddr_t) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_old_mmap_args {
	syscallarg(struct linux_oldmmap *) lmp;
};

struct linux_sys_truncate_args {
	syscallarg(const char *) path;
	syscallarg(long) length;
};

struct linux_sys_fchown16_args {
	syscallarg(int) fd;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_statfs_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_fstatfs_args {
	syscallarg(int) fd;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_ioperm_args {
	syscallarg(unsigned int) lo;
	syscallarg(unsigned int) hi;
	syscallarg(int) val;
};

struct linux_sys_socketcall_args {
	syscallarg(int) what;
	syscallarg(void *) args;
};

struct linux_sys_stat_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_lstat_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_olduname_args {
	syscallarg(struct linux_old_utsname *) up;
};

struct linux_sys_iopl_args {
	syscallarg(int) level;
};

struct linux_sys_wait4_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
};

struct linux_sys_swapoff_args {
	syscallarg(const char *) path;
};

struct linux_sys_sysinfo_args {
	syscallarg(struct linux_sysinfo *) arg;
};

struct linux_sys_ipc_args {
	syscallarg(int) what;
	syscallarg(int) a1;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(caddr_t) ptr;
};

struct linux_sys_sigreturn_args {
	syscallarg(struct linux_sigcontext *) scp;
};

struct linux_sys_clone_args {
	syscallarg(int) flags;
	syscallarg(void *) stack;
};

struct linux_sys_setdomainname_args {
	syscallarg(char *) domainname;
	syscallarg(int) len;
};

struct linux_sys_uname_args {
	syscallarg(struct linux_utsname *) up;
};

struct linux_sys_modify_ldt_args {
	syscallarg(int) func;
	syscallarg(void *) ptr;
	syscallarg(size_t) bytecount;
};

struct linux_sys_mprotect_args {
	syscallarg(const void *) start;
	syscallarg(unsigned long) len;
	syscallarg(int) prot;
};

struct linux_sys_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(const linux_old_sigset_t *) set;
	syscallarg(linux_old_sigset_t *) oset;
};

struct linux_sys_getpgid_args {
	syscallarg(int) pid;
};

struct linux_sys_personality_args {
	syscallarg(int) per;
};

struct linux_sys_llseek_args {
	syscallarg(int) fd;
	syscallarg(u_int32_t) ohigh;
	syscallarg(u_int32_t) olow;
	syscallarg(caddr_t) res;
	syscallarg(int) whence;
};

struct linux_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(struct linux_dirent *) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_select_args {
	syscallarg(int) nfds;
	syscallarg(fd_set *) readfds;
	syscallarg(fd_set *) writefds;
	syscallarg(fd_set *) exceptfds;
	syscallarg(struct timeval *) timeout;
};

struct linux_sys_msync_args {
	syscallarg(caddr_t) addr;
	syscallarg(int) len;
	syscallarg(int) fl;
};

struct linux_sys_fdatasync_args {
	syscallarg(int) fd;
};

struct linux_sys___sysctl_args {
	syscallarg(struct linux___sysctl *) lsp;
};

struct linux_sys_sched_setparam_args {
	syscallarg(pid_t) pid;
	syscallarg(const struct linux_sched_param *) sp;
};

struct linux_sys_sched_getparam_args {
	syscallarg(pid_t) pid;
	syscallarg(struct linux_sched_param *) sp;
};

struct linux_sys_sched_setscheduler_args {
	syscallarg(pid_t) pid;
	syscallarg(int) policy;
	syscallarg(const struct linux_sched_param *) sp;
};

struct linux_sys_sched_getscheduler_args {
	syscallarg(pid_t) pid;
};

struct linux_sys_sched_get_priority_max_args {
	syscallarg(int) policy;
};

struct linux_sys_sched_get_priority_min_args {
	syscallarg(int) policy;
};

struct linux_sys_mremap_args {
	syscallarg(void *) old_address;
	syscallarg(size_t) old_size;
	syscallarg(size_t) new_size;
	syscallarg(u_long) flags;
};

struct linux_sys_setresuid16_args {
	syscallarg(uid_t) ruid;
	syscallarg(uid_t) euid;
	syscallarg(uid_t) suid;
};

struct linux_sys_setresgid16_args {
	syscallarg(gid_t) rgid;
	syscallarg(gid_t) egid;
	syscallarg(gid_t) sgid;
};

struct linux_sys_rt_sigreturn_args {
	syscallarg(struct linux_rt_sigframe *) sfp;
};

struct linux_sys_rt_sigaction_args {
	syscallarg(int) signum;
	syscallarg(const struct linux_sigaction *) nsa;
	syscallarg(struct linux_sigaction *) osa;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(const linux_sigset_t *) set;
	syscallarg(linux_sigset_t *) oset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigpending_args {
	syscallarg(linux_sigset_t *) set;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_queueinfo_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
	syscallarg(void *) uinfo;
};

struct linux_sys_rt_sigsuspend_args {
	syscallarg(linux_sigset_t *) unewset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_pread_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_pwrite_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_chown16_args {
	syscallarg(const char *) path;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_sigaltstack_args {
	syscallarg(const struct linux_sigaltstack *) ss;
	syscallarg(struct linux_sigaltstack *) oss;
};

struct linux_sys_ugetrlimit_args {
	syscallarg(int) which;
	syscallarg(struct orlimit *) rlp;
};

struct linux_sys_truncate64_args {
	syscallarg(const char *) path;
	syscallarg(off_t) length;
};

struct linux_sys_stat64_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat64 *) sp;
};

struct linux_sys_lstat64_args {
	syscallarg(const char *) path;
	syscallarg(struct linux_stat64 *) sp;
};

struct linux_sys_fstat64_args {
	syscallarg(int) fd;
	syscallarg(struct linux_stat64 *) sp;
};

struct linux_sys_lchown_args {
	syscallarg(const char *) path;
	syscallarg(uid_t) uid;
	syscallarg(gid_t) gid;
};

struct linux_sys_setresuid_args {
	syscallarg(uid_t) ruid;
	syscallarg(uid_t) euid;
	syscallarg(uid_t) suid;
};

struct linux_sys_getresuid_args {
	syscallarg(uid_t *) ruid;
	syscallarg(uid_t *) euid;
	syscallarg(uid_t *) suid;
};

struct linux_sys_setresgid_args {
	syscallarg(gid_t) rgid;
	syscallarg(gid_t) egid;
	syscallarg(gid_t) sgid;
};

struct linux_sys_getresgid_args {
	syscallarg(gid_t *) rgid;
	syscallarg(gid_t *) egid;
	syscallarg(gid_t *) sgid;
};

struct linux_sys_chown_args {
	syscallarg(const char *) path;
	syscallarg(uid_t) uid;
	syscallarg(gid_t) gid;
};

struct linux_sys_setfsuid_args {
	syscallarg(uid_t) uid;
};

struct linux_sys_getdents64_args {
	syscallarg(int) fd;
	syscallarg(struct linux_dirent64 *) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_fcntl64_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

/*
 * System call prototypes.
 */

int	linux_sys_nosys(struct proc *, void *, register_t *);
int	sys_exit(struct proc *, void *, register_t *);
int	sys_fork(struct proc *, void *, register_t *);
int	sys_read(struct proc *, void *, register_t *);
int	sys_write(struct proc *, void *, register_t *);
int	linux_sys_open(struct proc *, void *, register_t *);
int	sys_close(struct proc *, void *, register_t *);
int	linux_sys_waitpid(struct proc *, void *, register_t *);
int	linux_sys_creat(struct proc *, void *, register_t *);
int	linux_sys_link(struct proc *, void *, register_t *);
int	linux_sys_unlink(struct proc *, void *, register_t *);
int	linux_sys_execve(struct proc *, void *, register_t *);
int	linux_sys_chdir(struct proc *, void *, register_t *);
int	linux_sys_time(struct proc *, void *, register_t *);
int	linux_sys_mknod(struct proc *, void *, register_t *);
int	linux_sys_chmod(struct proc *, void *, register_t *);
int	linux_sys_lchown16(struct proc *, void *, register_t *);
int	linux_sys_break(struct proc *, void *, register_t *);
int	compat_43_sys_lseek(struct proc *, void *, register_t *);
int	sys_getpid(struct proc *, void *, register_t *);
int	sys_setuid(struct proc *, void *, register_t *);
int	sys_getuid(struct proc *, void *, register_t *);
int	linux_sys_stime(struct proc *, void *, register_t *);
int	linux_sys_ptrace(struct proc *, void *, register_t *);
int	linux_sys_alarm(struct proc *, void *, register_t *);
int	linux_sys_pause(struct proc *, void *, register_t *);
int	linux_sys_utime(struct proc *, void *, register_t *);
int	linux_sys_access(struct proc *, void *, register_t *);
int	linux_sys_nice(struct proc *, void *, register_t *);
int	sys_sync(struct proc *, void *, register_t *);
int	linux_sys_kill(struct proc *, void *, register_t *);
int	linux_sys_rename(struct proc *, void *, register_t *);
int	linux_sys_mkdir(struct proc *, void *, register_t *);
int	linux_sys_rmdir(struct proc *, void *, register_t *);
int	sys_dup(struct proc *, void *, register_t *);
int	linux_sys_pipe(struct proc *, void *, register_t *);
int	linux_sys_times(struct proc *, void *, register_t *);
int	linux_sys_brk(struct proc *, void *, register_t *);
int	sys_setgid(struct proc *, void *, register_t *);
int	sys_getgid(struct proc *, void *, register_t *);
int	linux_sys_signal(struct proc *, void *, register_t *);
int	sys_geteuid(struct proc *, void *, register_t *);
int	sys_getegid(struct proc *, void *, register_t *);
int	sys_acct(struct proc *, void *, register_t *);
int	linux_sys_ioctl(struct proc *, void *, register_t *);
int	linux_sys_fcntl(struct proc *, void *, register_t *);
int	sys_setpgid(struct proc *, void *, register_t *);
int	linux_sys_oldolduname(struct proc *, void *, register_t *);
int	sys_umask(struct proc *, void *, register_t *);
int	sys_chroot(struct proc *, void *, register_t *);
int	sys_dup2(struct proc *, void *, register_t *);
int	sys_getppid(struct proc *, void *, register_t *);
int	sys_getpgrp(struct proc *, void *, register_t *);
int	sys_setsid(struct proc *, void *, register_t *);
int	linux_sys_sigaction(struct proc *, void *, register_t *);
int	linux_sys_siggetmask(struct proc *, void *, register_t *);
int	linux_sys_sigsetmask(struct proc *, void *, register_t *);
int	linux_sys_setreuid16(struct proc *, void *, register_t *);
int	linux_sys_setregid16(struct proc *, void *, register_t *);
int	linux_sys_sigsuspend(struct proc *, void *, register_t *);
int	linux_sys_sigpending(struct proc *, void *, register_t *);
int	compat_43_sys_sethostname(struct proc *, void *, register_t *);
int	linux_sys_setrlimit(struct proc *, void *, register_t *);
int	linux_sys_getrlimit(struct proc *, void *, register_t *);
int	sys_getrusage(struct proc *, void *, register_t *);
int	linux_sys_gettimeofday(struct proc *, void *, register_t *);
int	linux_sys_settimeofday(struct proc *, void *, register_t *);
int	linux_sys_getgroups16(struct proc *, void *, register_t *);
int	linux_sys_setgroups16(struct proc *, void *, register_t *);
int	linux_sys_oldselect(struct proc *, void *, register_t *);
int	linux_sys_symlink(struct proc *, void *, register_t *);
int	compat_43_sys_lstat(struct proc *, void *, register_t *);
int	linux_sys_readlink(struct proc *, void *, register_t *);
int	linux_sys_uselib(struct proc *, void *, register_t *);
int	linux_sys_swapon(struct proc *, void *, register_t *);
int	linux_sys_reboot(struct proc *, void *, register_t *);
int	linux_sys_readdir(struct proc *, void *, register_t *);
int	linux_sys_old_mmap(struct proc *, void *, register_t *);
int	sys_munmap(struct proc *, void *, register_t *);
int	linux_sys_truncate(struct proc *, void *, register_t *);
int	compat_43_sys_ftruncate(struct proc *, void *, register_t *);
int	sys_fchmod(struct proc *, void *, register_t *);
int	linux_sys_fchown16(struct proc *, void *, register_t *);
int	sys_getpriority(struct proc *, void *, register_t *);
int	sys_setpriority(struct proc *, void *, register_t *);
int	sys_profil(struct proc *, void *, register_t *);
int	linux_sys_statfs(struct proc *, void *, register_t *);
int	linux_sys_fstatfs(struct proc *, void *, register_t *);
int	linux_sys_ioperm(struct proc *, void *, register_t *);
int	linux_sys_socketcall(struct proc *, void *, register_t *);
int	sys_setitimer(struct proc *, void *, register_t *);
int	sys_getitimer(struct proc *, void *, register_t *);
int	linux_sys_stat(struct proc *, void *, register_t *);
int	linux_sys_lstat(struct proc *, void *, register_t *);
int	linux_sys_fstat(struct proc *, void *, register_t *);
int	linux_sys_olduname(struct proc *, void *, register_t *);
int	linux_sys_iopl(struct proc *, void *, register_t *);
int	linux_sys_wait4(struct proc *, void *, register_t *);
int	linux_sys_swapoff(struct proc *, void *, register_t *);
int	linux_sys_sysinfo(struct proc *, void *, register_t *);
int	linux_sys_ipc(struct proc *, void *, register_t *);
int	sys_fsync(struct proc *, void *, register_t *);
int	linux_sys_sigreturn(struct proc *, void *, register_t *);
int	linux_sys_clone(struct proc *, void *, register_t *);
int	linux_sys_setdomainname(struct proc *, void *, register_t *);
int	linux_sys_uname(struct proc *, void *, register_t *);
int	linux_sys_modify_ldt(struct proc *, void *, register_t *);
int	linux_sys_mprotect(struct proc *, void *, register_t *);
int	linux_sys_sigprocmask(struct proc *, void *, register_t *);
int	linux_sys_getpgid(struct proc *, void *, register_t *);
int	sys_fchdir(struct proc *, void *, register_t *);
int	linux_sys_personality(struct proc *, void *, register_t *);
int	linux_sys_setfsuid(struct proc *, void *, register_t *);
int	linux_sys_getfsuid(struct proc *, void *, register_t *);
int	linux_sys_llseek(struct proc *, void *, register_t *);
int	linux_sys_getdents(struct proc *, void *, register_t *);
int	linux_sys_select(struct proc *, void *, register_t *);
int	sys_flock(struct proc *, void *, register_t *);
int	linux_sys_msync(struct proc *, void *, register_t *);
int	sys_readv(struct proc *, void *, register_t *);
int	sys_writev(struct proc *, void *, register_t *);
int	sys_getsid(struct proc *, void *, register_t *);
int	linux_sys_fdatasync(struct proc *, void *, register_t *);
int	linux_sys___sysctl(struct proc *, void *, register_t *);
int	sys_mlock(struct proc *, void *, register_t *);
int	sys_munlock(struct proc *, void *, register_t *);
int	sys_mlockall(struct proc *, void *, register_t *);
int	sys_munlockall(struct proc *, void *, register_t *);
int	linux_sys_sched_setparam(struct proc *, void *, register_t *);
int	linux_sys_sched_getparam(struct proc *, void *, register_t *);
int	linux_sys_sched_setscheduler(struct proc *, void *, register_t *);
int	linux_sys_sched_getscheduler(struct proc *, void *, register_t *);
int	linux_sys_sched_yield(struct proc *, void *, register_t *);
int	linux_sys_sched_get_priority_max(struct proc *, void *, register_t *);
int	linux_sys_sched_get_priority_min(struct proc *, void *, register_t *);
int	sys_nanosleep(struct proc *, void *, register_t *);
int	linux_sys_mremap(struct proc *, void *, register_t *);
int	linux_sys_setresuid16(struct proc *, void *, register_t *);
int	linux_sys_getresuid(struct proc *, void *, register_t *);
int	sys_poll(struct proc *, void *, register_t *);
int	linux_sys_setresgid16(struct proc *, void *, register_t *);
int	linux_sys_getresgid(struct proc *, void *, register_t *);
int	linux_sys_rt_sigreturn(struct proc *, void *, register_t *);
int	linux_sys_rt_sigaction(struct proc *, void *, register_t *);
int	linux_sys_rt_sigprocmask(struct proc *, void *, register_t *);
int	linux_sys_rt_sigpending(struct proc *, void *, register_t *);
int	linux_sys_rt_queueinfo(struct proc *, void *, register_t *);
int	linux_sys_rt_sigsuspend(struct proc *, void *, register_t *);
int	linux_sys_pread(struct proc *, void *, register_t *);
int	linux_sys_pwrite(struct proc *, void *, register_t *);
int	linux_sys_chown16(struct proc *, void *, register_t *);
int	sys___getcwd(struct proc *, void *, register_t *);
int	linux_sys_sigaltstack(struct proc *, void *, register_t *);
int	sys___vfork14(struct proc *, void *, register_t *);
int	linux_sys_ugetrlimit(struct proc *, void *, register_t *);
int	linux_sys_mmap2(struct proc *, void *, register_t *);
int	linux_sys_truncate64(struct proc *, void *, register_t *);
int	sys_ftruncate(struct proc *, void *, register_t *);
int	linux_sys_stat64(struct proc *, void *, register_t *);
int	linux_sys_lstat64(struct proc *, void *, register_t *);
int	linux_sys_fstat64(struct proc *, void *, register_t *);
int	linux_sys_lchown(struct proc *, void *, register_t *);
int	sys_getuid(struct proc *, void *, register_t *);
int	sys_getgid(struct proc *, void *, register_t *);
int	sys_geteuid(struct proc *, void *, register_t *);
int	sys_getegid(struct proc *, void *, register_t *);
int	sys_setreuid(struct proc *, void *, register_t *);
int	sys_setregid(struct proc *, void *, register_t *);
int	sys_getgroups(struct proc *, void *, register_t *);
int	sys_setgroups(struct proc *, void *, register_t *);
int	sys___posix_fchown(struct proc *, void *, register_t *);
int	linux_sys_setresuid(struct proc *, void *, register_t *);
int	linux_sys_getresuid(struct proc *, void *, register_t *);
int	linux_sys_setresgid(struct proc *, void *, register_t *);
int	linux_sys_getresgid(struct proc *, void *, register_t *);
int	linux_sys_chown(struct proc *, void *, register_t *);
int	sys_setuid(struct proc *, void *, register_t *);
int	sys_setgid(struct proc *, void *, register_t *);
int	linux_sys_setfsuid(struct proc *, void *, register_t *);
int	linux_sys_getfsuid(struct proc *, void *, register_t *);
int	linux_sys_getdents64(struct proc *, void *, register_t *);
int	linux_sys_fcntl64(struct proc *, void *, register_t *);
#endif /* _LINUX_SYS__SYSCALLARGS_H_ */
