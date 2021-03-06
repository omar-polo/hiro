/*
 * Copyright (c) 2021 Omar Polo <op@omarpolo.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "cmd.h"
#include "util.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define dead_attr __attribute__((__noreturn__))

int		 cmd_restart(int, char**);
void		 cmd_restart_usage(void) dead_attr;

int		 cmd_send(int, char**);
void		 cmd_send_usage(void) dead_attr;

int		 cmd_recv(int, char**);
void		 cmd_recv_usage(void) dead_attr;

int		 cmd_ping(int, char**);
void		 cmd_ping_usage(void) dead_attr;

typedef int(*cmdmainfn)(int, char**);

struct cmddef {
	const char	*cmd;
	cmdmainfn	 fn;
} cmds[] = {
	{ "restart",	cmd_restart },
	{ "send",	cmd_send },
	{ "recv",	cmd_recv },
	{ "ping",	cmd_ping },
	{ NULL,		NULL },
};

const char *sockpath;
int fd;

/* XXX: replace every instance with `getprogname'. */
char *me;

static void
usage(const char *me)
{
	fprintf(stderr, "USAGE: %s [-P sock_path] cmd <args..>\n",
	    me);
}

static void
io_copy(int from, int to)
{
	char buf[64];
	ssize_t len;

	for (;;) {
		if ((len = read(from, buf, sizeof(buf))) <= 0)
			break;
		write(to, buf, len);
	}
}

int
cmd_restart(int argc, char **argv)
{
	struct cmd cmd = {
		.type = CMD_RESTART,
	};

	if (send_cmd(fd, &cmd) == -1)
		err(1, "cmd_restart");

	io_copy(fd, 1);
	return 0;
}

void dead_attr
cmd_restart_usage(void)
{
	fprintf(stderr, "USAGE: %s restart\n", me);
	exit(1);
}

int
cmd_send(int argc, char **argv)
{
	struct cmd cmd = {
		.type = CMD_SEND,
	};

	if (getopt(argc, argv, "") != -1)
		cmd_send_usage();

	/* argc -= optind; */
	/* argv += optind; */

	if (argc != 2)
		cmd_send_usage();

	cmd.argc = argc;
	cmd.argv = argv;

	if (send_cmd(fd, &cmd) == -1)
		err(1, "cmd_send");

	io_copy(fd, 1);

	return 0;
}

void dead_attr
cmd_send_usage(void)
{
	fprintf(stderr, "USAGE: %s send <to> <what>\n", me);
	exit(1);
}

int
cmd_recv(int argc, char **argv)
{
	struct cmd cmd = {
		.type = CMD_RECV,
	};

	optind = 0;

	if (getopt(argc, argv, "") != -1)
		cmd_recv_usage();
	argc -= optind;
	argv += optind;

	/* XXX: investigate this.  Basically optind is at least 1, so
	 * we go under 0... */
	if (argc >= 0)
		cmd_recv_usage();

	if (send_cmd(fd, &cmd) == -1)
		err(1, "cmd_send");

	io_copy(fd, 1);

	return 0;
}

void dead_attr
cmd_recv_usage(void)
{
	fprintf(stderr, "USAGE: %s recv\n", me);
	exit(1);
}

int
cmd_ping(int argc, char **argv)
{
	struct cmd cmd = {
		.type = CMD_PING,
	};

	if (argc != 0)
		cmd_ping_usage();

	if (send_cmd(fd, &cmd) == -1)
		err(1, "send_cmd");

	io_copy(fd, 1);

	return 0;
}

void dead_attr
cmd_ping_usage(void)
{
	fprintf(stderr, "USAGE: %s [...] ping\n", me);
	exit(1);
}

static int
open_ctl_sock(const char *path)
{
	struct sockaddr_un addr;
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strlcpy(addr.sun_path, path, sizeof(addr.sun_path));

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		err(1, "connect");

	return fd;
}

int
main(int argc, char **argv)
{
	struct cmddef *cmd;
	int ch, ret;
	const char *sub;

	me = *argv;

	while ((ch = getopt(argc, argv, "P:")) != -1) {
		switch (ch) {
		case 'P':
			sockpath = optarg;
			break;
		case '?':
			usage(me);
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (sockpath == NULL)
		sockpath = default_socket_path();

	if (argc == 0) {
		usage(me);
		return 1;
	}

	sub = argv[0];

	argv++;
	argc--;

	ret = 1;
	for (cmd = cmds; cmd->cmd != NULL; ++cmd) {
		if (!strcmp(sub, cmd->cmd)) {
			if ((fd = open_ctl_sock(sockpath)) == -1)
				err(1, "open_ctl_sock: %s", sockpath);

			ret = cmd->fn(argc, argv);
			goto end;
		}
	}

	usage(me);

end:
	close(fd);
	return ret;
}
