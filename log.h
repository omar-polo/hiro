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

#ifndef HIRO_LOG_H
#define HIRO_LOG_H

extern int verbose;

#define LOG_ATTR_FMT __attribute__((format (printf, 1, 2)))

void		log_debug(const char*, ...) LOG_ATTR_FMT;
void		log_info(const char*, ...) LOG_ATTR_FMT;
void		log_notice(const char*, ...) LOG_ATTR_FMT;
void		log_warn(const char*, ...) LOG_ATTR_FMT;
void		log_err(const char*, ...) LOG_ATTR_FMT;
void		log_fatal(const char*, ...) LOG_ATTR_FMT __attribute__((__noreturn__));

#endif
