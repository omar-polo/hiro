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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* todo: add checks for write errors */
int
send_cmd(int fd, struct cmd *cmd)
{
	size_t len;
	int i;

	write(fd, &cmd->type, sizeof(cmd->type));
	write(fd, &cmd->argc, sizeof(cmd->argc));

	if (cmd->argc == 0)
		return 0;

	len = 0;
	for (i = 0; i < cmd->argc; ++i)
		len += strlen(cmd->argv[i]) + 1;

	write(fd, &len, sizeof(len));
	for (i = 0; i < cmd->argc; ++i)
		write(fd, cmd->argv[i], strlen(cmd->argv[i]) +1);
	return 0;
}

/* todo: add checks for read errors */
int
recv_cmd(int fd, struct cmd *cmd)
{
	size_t len;
	int i;
	char *args;

	cmd->argv = NULL;

	if (read(fd, &cmd->type, sizeof(cmd->type)) <= 0)
		return -1;

	read(fd, &cmd->argc, sizeof(cmd->argc));

	if (cmd->argc == 0)
		return 0;

	read(fd, &len, sizeof(len));

	if ((args = calloc(1, len)) == NULL)
		return -1;
	if ((cmd->argv = calloc(1, cmd->argc+1)) == NULL)
		return -1;

	read(fd, args, len);

	for (i = 0; i < cmd->argc; ++i) {
		cmd->argv[i] = args;
		args = strchr(args, '\0');

		/* if we don't find a NUL terminator... we have
		 * already have been killed for a SIGSEV,
		 * probably */
		if (args == NULL)
			return -1;
		args++;
	}

	return 0;
}

const char *
cmd_name(enum cmd_type type)
{
	switch (type) {
	case CMD_RESTART:
		return "restart";
	case CMD_SEND:
		return "send";
	case CMD_RECV:
		return "recv";
	case CMD_PING:
		return "ping";
	default:
		return "unknown command";
	}
}
