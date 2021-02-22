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
#include "hiro.h"
#include "log.h"
#include "util.h"

#include "err.h"
#include "queue.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>

#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*cmd_handlefn)(int, struct cmd*);

static void	handle_cmd_restart(int, struct cmd*);
static void	handle_cmd_send(int, struct cmd*);
static void	handle_cmd_recv(int, struct cmd*);
static void	handle_cmd_ping(int, struct cmd*);

struct cmd_handlers {
	enum cmd_type	type;
	cmd_handlefn	fn;
} handlers[] = {
	{ CMD_RESTART,	handle_cmd_restart },
 	{ CMD_SEND,	handle_cmd_send },
	{ CMD_RECV,	handle_cmd_recv },
	{ CMD_PING,	handle_cmd_ping },
	{ -1,		NULL },
};

LIST_HEAD(clientshead, client) clients;
struct client {
	int			 fd;
	int			 busy;
	size_t			 off;
	size_t			 len;
	struct shstr		*buf;
	/* todo: enqueue msgs? */
	struct event		 ev;
	LIST_ENTRY(client)	 clients;
};

static void
handle_client_write(int fd, short ev, void *d)
{
	struct client *c = d;
	ssize_t r;

	if ((r = write(fd, c->buf->str + c->off, c->len - c->off)) == -1) {
		log_debug("failed write for a client, deleting it");
		LIST_REMOVE(c, clients);
		free_shstr(c->buf);
		close(c->fd);
		event_del(&c->ev);
		free(c);
		return;
	}

	c->off += r;

	if (c->off == c->len) {
		c->busy = 0;
		free_shstr(c->buf);
		event_del(&c->ev);
	}
}

static void
handle_cmd_restart(int fd, struct cmd *cmd)
{
	dprintf(fd, "unimplemented handle_cmd_restart\n");
	close(fd);
}

static void
handle_cmd_send(int fd, struct cmd *cmd)
{
	struct client *c;
	struct shstr *s;

	if (cmd->argc != 2) {
		log_warn("SEND command with improper arg number (%d)",
		    cmd->argc);
		goto end;
	}

	if ((s = make_shstr(cmd->argv[1])) == NULL) {
		log_warn("failed allocation of struct shstr");
		goto end;
	}

	log_warn("TODO: send %s to %s",
	    cmd->argv[1], cmd->argv[0]);

	LIST_FOREACH(c, &clients, clients) {
		/* XXX: enqueue? */
		if (c->busy)
			continue;

		c->len = strlen(s->str);
		c->off = 0;
		c->buf = shstr_inc(s);
		event_set(&c->ev, c->fd, EV_WRITE | EV_PERSIST, handle_client_write, c);
		event_add(&c->ev, NULL);
	}

end:
	close(fd);
}

static void
handle_cmd_recv(int fd, struct cmd *cmd)
{
	struct client *c;

	if ((c = calloc(1, sizeof(*c))) == NULL) {
		log_warn("handle_cmd_recv: failed calloc");
		close(fd);
		return;
	}

	c->fd = fd;
	LIST_INSERT_HEAD(&clients, c, clients);
}

static void
handle_cmd_ping(int fd, struct cmd *cmd)
{
	dprintf(fd, "PONG\n");
	close(fd);
}

static void
usage(const char *me)
{
	fprintf(stderr, "USAGE: %s [-P sock_path] [-p port]\n",
	    me);
}

static int
make_socket(int port, int family)
{
	int sock, v;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	struct sockaddr *addr;
	socklen_t len;

        switch (family) {
	case AF_INET:
		bzero(&addr4, sizeof(addr4));
		addr4.sin_family = family;
		addr4.sin_port = htons(port);
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr = (struct sockaddr*)&addr4;
		len = sizeof(addr4);
		break;

	case AF_INET6:
		bzero(&addr6, sizeof(addr6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
		addr6.sin6_addr = in6addr_any;
		addr = (struct sockaddr*)&addr6;
		len = sizeof(addr6);
		break;

	default:
		/* unreachable */
		abort();
	}

	if ((sock = socket(family, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	v = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) == -1)
		err(1, "setsockopt(SO_REUSEADDR)");

	v = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v)) == -1)
		err(1, "setsockopt(SO_REUSEPORT)");

	/* mark_nonblock(sock); */

	if (bind(sock, addr, len) == -1)
		err(1, "bind");

	if (listen(sock, 16) == -1)
		err(1, "listen");

	return sock;
}

static int
make_ctl_socket(const char *path)
{
	struct sockaddr_un addr;
	int fd;

	unlink(path);
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strlcpy(addr.sun_path, path, sizeof(addr.sun_path));

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		err(1, "bind");

	if (listen(fd, 5) == -1)
		err(1, "listen");

	return fd;
}

static void
handle_cmd(int fd, short events, void *d)
{
	struct cmd cmd;
	struct cmd_handlers *hs;

	if (recv_cmd(fd, &cmd) == -1) {
		log_warn("failed recv_cmd");
		return;
	}

	log_debug("got command: %s", cmd_name(cmd.type));

	for (hs = handlers; hs->fn != NULL; ++hs) {
		if (hs->type == cmd.type) {
			hs->fn(fd, &cmd);
			goto end;
		}
	}

	warnx("unknown command %d", cmd.type);
	close(fd);

end:
	if (cmd.argv != 0)
		free(cmd.argv[0]);
}

static void
handle_ctl_conn(int fd, short events, void *d)
{
	int cfd;

	if ((cfd = accept(fd, NULL, NULL)) == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		err(1, "accept");
	}

	event_once(cfd, EV_READ, handle_cmd, NULL, NULL);
}

static void
handle_conn(int fd, short events, void *d)
{
	puts("got a connection on the socket!");
	return;
}

int
main(int argc, char **argv) 
{
	struct event ctlev, sockev;
	int ch, port, ctl, sock;
	const char *path;

	port = 2103;
	path = NULL;

	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt(argc, argv, "P:p:v")) != -1) {
		switch (ch) {
		case 'p':
			port = parse_portno(optarg);
			break;
		case 'P':
			path = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage(*argv);
			return 1;
		}
	}

	/* XXX: temporary */
	verbose = 3;

	LIST_INIT(&clients);

	if (path == NULL)
		path = default_socket_path();

	if ((ctl = make_ctl_socket(path)) == -1)
		err(1, "make_ctl_socket");

	if ((sock = make_socket(port, AF_INET)) == -1)
		err(1, "make_socket");

	log_debug("starting...");

	event_init();

	event_set(&ctlev, ctl, EV_READ | EV_PERSIST, &handle_ctl_conn, NULL);
	event_add(&ctlev, NULL);
	log_debug("ready to accept commands over the ctl socket");

	event_set(&sockev, sock, EV_READ | EV_PERSIST, &handle_conn, NULL);
	event_add(&sockev, NULL);
	log_debug("ready to accept network connections");

	event_dispatch();

	close(ctl);
	close(sock);

	return 0;
}
