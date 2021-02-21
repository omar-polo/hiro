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

#include "util.h"

#include "err.h"

#include "strtonum.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *
default_socket_path(void)
{
	char *v;

	if ((v = getenv("HIRO_SOCKET")) != NULL)
		return v;

	return "/tmp/hirod.1000";
}

int
parse_portno(const char *p)
{
	const char *errstr;
	int n;

	n = strtonum(p, 0, UINT16_MAX, &errstr);
	if (errstr != NULL)
		err(1, "port number is %s: %s", errstr, p);
	return n;
}

struct shstr *
make_shstr(const char *s)
{
	struct shstr *ss;
	char *dup;

	if ((dup = strdup(s)) == NULL)
		return NULL;

	if ((ss = calloc(1, sizeof(*ss))) == NULL) {
		free(dup);
		return NULL;
	}

	ss->str = dup;
	return ss;
}

struct shstr *
shstr_inc(struct shstr *s)
{
	s->rc++;
	return s;
}

void
free_shstr(struct shstr *s)
{
	if (s->rc != 0) {
		s->rc--;
		return;
	}

	free(s->str);
	free(s);
}
