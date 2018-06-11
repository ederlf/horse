#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "netns.h"

#define NETNS_ETC_DIR "/etc/netns"
#define NETNS_RUN_DIR "/var/run/netns"

static void bind_etc(const char *name)
{
	char etc_netns_path[PATH_MAX];
	char netns_name[PATH_MAX];
	char etc_name[PATH_MAX];
	struct dirent *entry;
	DIR *dir;

	snprintf(etc_netns_path, sizeof(etc_netns_path),
			"%s/%s", NETNS_ETC_DIR, name);
	dir = opendir(etc_netns_path);
	if (!dir)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		snprintf(netns_name, sizeof(netns_name), "%s/%s",
				etc_netns_path, entry->d_name);
		snprintf(etc_name, sizeof(etc_name), "/etc/%s", entry->d_name);
		if (mount(netns_name, etc_name, "none", MS_BIND, NULL) < 0) {
			fprintf(stderr, "Bind %s -> %s failed: %s",
				netns_name, etc_name, strerror(errno));
		}
	}
	closedir(dir);
}

int netns_enter(const char *name)
{
	char net_path[PATH_MAX];
	int netns;

	snprintf(net_path, sizeof(net_path), "%s/%s", NETNS_RUN_DIR, name);
	netns = open(net_path, O_RDONLY | O_CLOEXEC);
	if (netns < 0) {
		perror(net_path);
		return -1;
	}

	if (setns(netns, CLONE_NEWNET) < 0) {
		perror("setns");
		close(netns);
		return -1;
	}
	close(netns);

	if (unshare(CLONE_NEWNS) < 0) {
		perror("unshare");
		return -1;
	}

	/* Don't let any mounts propagate back to the parent */
	if (mount("", "/", "none", MS_SLAVE | MS_REC, NULL)) {
		perror("\"mount --make-rslave /\" failed");
		return -1;
	}

	/* Mount a version of /sys that describes the network namespace */
	if (umount2("/sys", MNT_DETACH) < 0) {
		perror("umount of /sys failed");
		return -1;
	}

	if (mount(name, "/sys", "sysfs", 0, NULL) < 0) {
		perror("mount of /sys failed");
		return -1;
	}

	/* Setup bind mounts for config files in /etc */
	bind_etc(name);
	return 0;
}

int netns_delete(const char *name)
{
	char netns_path[PATH_MAX];

	snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR, name);
	umount2(netns_path, MNT_DETACH);
	if (unlink(netns_path) < 0) {
		perror(netns_path);
		return -1;
	}
	return 0;
}

/* This function creates a new network namespace and
 * a new mount namespace and bind them into a well known
 * location in the filesystem based on the name provided.
 *
 * The mount namespace is created so that any necessary
 * userspace tweaks like remounting /sys, or bind mounting
 * a new /etc/resolv.conf can be shared between users.
 */
int netns_add(const char *name)
{
	int ret, fd, made_netns_run_dir_mount = 0;
	char netns_path[PATH_MAX];
	pid_t pid;

	/* create a child process that manipulates it's own net namespace */
	pid = fork();
	if (pid == -1) {
		perror("fork");
		return 0;
	}
	if (pid != 0) {
		waitpid(pid, &ret, 0);
		return ret == EXIT_SUCCESS ? 0 : -1;
	}

	if (mkdir(NETNS_RUN_DIR, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) {
		if (errno != EEXIST) {
			perror(NETNS_RUN_DIR);
			exit(EXIT_FAILURE);
		}
	}

	/* Make it possible for network namespace mounts to propagate between
	 * mount namespaces.  This makes it likely that a unmounting a network
	 * namespace file in one namespace will unmount the network namespace
	 * file in all namespaces allowing the network namespace to be freed
	 * sooner.
	 */
	while (mount("", NETNS_RUN_DIR, "none", MS_SHARED | MS_REC, NULL)) {
		/* Fail unless we need to make the mount point */
		if (errno != EINVAL || made_netns_run_dir_mount) {
			perror("mount --make-shared "NETNS_RUN_DIR" failed");
			exit(EXIT_FAILURE);
		}

		/* Upgrade NETNS_RUN_DIR to a mount point */
		if (mount(NETNS_RUN_DIR, NETNS_RUN_DIR, "none", MS_BIND, NULL)) {
			perror("mount --bind "NETNS_RUN_DIR" "
					NETNS_RUN_DIR" failed");
			exit(EXIT_FAILURE);
		}
		made_netns_run_dir_mount = 1;
	}

	/* Create the filesystem state */
	snprintf(netns_path, sizeof(netns_path), "%s/%s", NETNS_RUN_DIR, name);

	fd = open(netns_path, O_RDONLY|O_CREAT|O_EXCL, 0);
	if (fd < 0) {
		perror(netns_path);
		exit(EXIT_FAILURE);
	}
	close(fd);
	if (unshare(CLONE_NEWNET) < 0) {
		perror("clone(CLONE_NEWNET)");
		goto out_delete;
	}

	/* Bind the netns last so I can watch for it */
	if (mount("/proc/self/ns/net", netns_path, "none", MS_BIND, NULL) < 0) {
		perror("Bind mount /proc/self/ns/net failed");
		goto out_delete;
	}
	exit(EXIT_SUCCESS);
out_delete:
	netns_delete(name);
	exit(EXIT_FAILURE);
}


static int count_args(const char *cmd)
{
	const char *ptr;
	int argc = 1;
    int open = 0;
	for (ptr = cmd; *ptr; ++ptr) {
		if (*ptr == '"' && !open){
            open = 1;
            ++ptr;
        }
        else if (*ptr == '"' && open){
            open = 0;
            ++ptr;
        }
        if (isspace(*ptr) && !open) {
			++argc;
			while (isspace(ptr[1]))
				++ptr;
		}
	}
	return argc;
}

static void split_args(char **argv, char *cmd)
{
	int i = 1;
	char *ptr;

	argv[0] = cmd;
    int open = 0;
	for (ptr = cmd; *ptr; ++ptr) {
        if (*ptr == '"' && !open){
            open = 1;
            memmove(&ptr[0], &ptr[1], strlen(ptr));
        }
        else if (*ptr == '"' && open){
            open = 0;
            memmove(&ptr[0], &ptr[1], strlen(ptr));
        }
        if (isspace(*ptr) && !open) {
			*ptr = 0;

			while (isspace(ptr[1]))
				++ptr;

			argv[i++] = ptr + 1;
		}
	}

	argv[i] = NULL;
}

pid_t netns_launch(const char *name, const char *fmt, ...)
{
	char *temp, **argv;
	int argc, nullfd;
	va_list ap;
	pid_t pid;

	pid = fork();

	if (pid == -1) {
		perror("fork");
		return -1;
	}
	if (pid != 0)
		return pid;

	/* TODO: use pipe here, process output on failure */
	nullfd = open("/dev/null", O_WRONLY);
	if (nullfd == -1) {
		perror("open /dev/null");
	} else {
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
		close(nullfd);
	}

	va_start(ap, fmt);
	if (vasprintf(&temp, fmt, ap) == -1)
        exit(EXIT_FAILURE);
	va_end(ap);

	argc = count_args(temp);
	argv = alloca(sizeof(char*) * (argc + 1));
	split_args(argv, temp);

	if (name && netns_enter(name) != 0)
		exit(EXIT_FAILURE);

	execvp(argv[0], argv);
	perror(argv[0]);
	exit(EXIT_FAILURE);
}

int netns_run(const char *name, const char *fmt, ...)
{
	int status, argc, nullfd;
	char *temp, **argv;
	va_list ap;
	pid_t pid;

	pid = fork();

	if (pid == -1) {
		perror("fork");
		return -1;
	}
	if (pid != 0) {
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status) == EXIT_SUCCESS ? 0 : -1;
	}

	/* TODO: use pipe here, process output on failure */
	nullfd = open("/dev/null", O_WRONLY);
	if (nullfd == -1) {
		perror("open /dev/null");
	} else {
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
        close(nullfd);
	}
	va_start(ap, fmt);
	if (vasprintf(&temp, fmt, ap) == -1)
		exit(EXIT_FAILURE);
	va_end(ap);

	argc = count_args(temp);
	argv = alloca(sizeof(char*) * (argc + 1));
	split_args(argv, temp);

	if (name && netns_enter(name) != 0)
        exit(EXIT_FAILURE);
	execvp(argv[0], argv);
	perror(argv[0]);
	exit(EXIT_FAILURE);
}

int netns_run_argv(const char *name, const char *cmd, int argc, char **argv)
{
	int i, status, nullfd, cmd_argc;
	char *temp, **new_argv;
	pid_t pid;

	pid = fork();

	if (pid == -1) {
		perror("fork");
		return -1;
	}
	if (pid != 0) {
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status) == EXIT_SUCCESS ? 0 : -1;
	}

	/* TODO: use pipe here, process output on failure */
	nullfd = open("/dev/null", O_WRONLY);
	if (nullfd == -1) {
		perror("open /dev/null");
	} else {
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
		close(nullfd);
	}

	if (cmd) {
		temp = strdup(cmd);
		if (!temp)
			exit(EXIT_FAILURE);
		cmd_argc = count_args(temp);
		new_argv = alloca(sizeof(char*) * (cmd_argc + argc + 1));
		split_args(new_argv, temp);
	} else {
		cmd_argc = 0;
		new_argv = alloca(sizeof(char*) * (argc + 1));
	}

	for (i = 0; i < argc; ++i)
		new_argv[cmd_argc + i] = argv[i];

	new_argv[cmd_argc + i] = 0;

	if (name && netns_enter(name) != 0)
		exit(EXIT_FAILURE);

	execvp(new_argv[0], new_argv);
	perror(new_argv[0]);
	exit(EXIT_FAILURE);
}
