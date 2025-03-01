/*
 * Copyright (c) 2011, Vicent Marti
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

#ifndef UPSKIRT_AUTOLINK_H
#define UPSKIRT_AUTOLINK_H

#include "buffer.h"

extern int
sd_autolink_issafe(const uint8_t *link, size_t link_len);

extern size_t
sd_autolink__www(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size);

extern size_t
sd_autolink__email(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size);

extern size_t
sd_autolink__url(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size);

extern size_t
sd_autolink__subreddit(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size);

extern size_t
sd_autolink__username(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size, uint8_t *rndr_del);

#endif

/* vim: set filetype=c: */
