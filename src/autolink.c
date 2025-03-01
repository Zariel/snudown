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

#include "buffer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int
sd_autolink_issafe(const uint8_t *link, size_t link_len)
{
	static const size_t valid_uris_count = 13;
	static const char *valid_uris[] = {
		"http://", "https://", "ftp://", "mailto://",
		"/", "git://", "steam://", "irc://", "news://", "mumble://",
		"ssh://", "ircs://", "#"
	};

	size_t i;

	for (i = 0; i < valid_uris_count; ++i) {
		size_t len = strlen(valid_uris[i]);

		if (link_len > len &&
			strncasecmp((char *)link, valid_uris[i], len) == 0 &&
			(isalnum(link[len]) || link[len] == '#' || link[len] == '/' || link[len] == '?'))
			return 1;
	}

	return 0;
}

static size_t
autolink_delim(uint8_t *data, size_t link_end, size_t offset, size_t size)
{
	uint8_t cclose, copen = 0;
	size_t i;

	for (i = 0; i < link_end; ++i)
		if (data[i] == '<') {
			link_end = i;
			break;
		}

	while (link_end > 0) {
		if (strchr("?!.,", data[link_end - 1]) != NULL)
			link_end--;

		else if (data[link_end - 1] == ';') {
			size_t new_end = link_end - 2;

			while (new_end > 0 && isalpha(data[new_end]))
				new_end--;

			if (new_end < link_end - 2 && data[new_end] == '&')
				link_end = new_end;
			else
				link_end--;
		}
		else break;
	}

	if (link_end == 0)
		return 0;

	cclose = data[link_end - 1];

	switch (cclose) {
	case '"':	copen = '"'; break;
	case '\'':	copen = '\''; break;
	case ')':	copen = '('; break;
	case ']':	copen = '['; break;
	case '}':	copen = '{'; break;
	}

	if (copen != 0) {
		size_t closing = 0;
		size_t opening = 0;
		size_t i = 0;

		/* Try to close the final punctuation sign in this same line;
		 * if we managed to close it outside of the URL, that means that it's
		 * not part of the URL. If it closes inside the URL, that means it
		 * is part of the URL.
		 *
		 * Examples:
		 *
		 *	foo http://www.pokemon.com/Pikachu_(Electric) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric)
		 *
		 *	foo (http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric)
		 *
		 *	foo http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric))
		 *
		 *	(foo http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> foo http://www.pokemon.com/Pikachu_(Electric)
		 */

		while (i < link_end) {
			if (data[i] == copen)
				opening++;
			else if (data[i] == cclose)
				closing++;

			i++;
		}

		if (closing != opening)
			link_end--;
	}

	return link_end;
}

static size_t
check_domain(uint8_t *data, size_t size)
{
	size_t i, np = 0;

	if (!isalnum(data[0]))
		return 0;

	for (i = 1; i < size - 1; ++i) {
		if (data[i] == '.') np++;
		else if (!isalnum(data[i]) && data[i] != '-') break;
	}

	/* a valid domain needs to have at least a dot.
	 * that's as far as we get */
	return np ? i : 0;
}

size_t
sd_autolink__www(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size)
{
	size_t link_end;

	if (offset > 0 && !ispunct(data[-1]) && !isspace(data[-1]))
		return 0;

	if (size < 4 || memcmp(data, "www.", strlen("www.")) != 0)
		return 0;

	link_end = check_domain(data, size);

	if (link_end == 0)
		return 0;

	while (link_end < size && !isspace(data[link_end]))
		link_end++;

	link_end = autolink_delim(data, link_end, offset, size);

	if (link_end == 0)
		return 0;

	bufput(link, data, link_end);
	*rewind_p = 0;

	return (int)link_end;
}

size_t
sd_autolink__email(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size)
{
	size_t link_end, rewind;
	int nb = 0, np = 0;

	for (rewind = 0; rewind < offset; ++rewind) {
		uint8_t c = data[-rewind - 1];

		if (isalnum(c))
			continue;

		if (strchr(".+-_", c) != NULL)
			continue;

		break;
	}

	if (rewind == 0)
		return 0;

	for (link_end = 0; link_end < size; ++link_end) {
		uint8_t c = data[link_end];

		if (isalnum(c))
			continue;

		if (c == '@')
			nb++;
		else if (c == '.' && link_end < size - 1)
			np++;
		else if (c != '-' && c != '_')
			break;
	}

	if (link_end < 2 || nb != 1 || np == 0)
		return 0;

	link_end = autolink_delim(data, link_end, offset, size);

	if (link_end == 0)
		return 0;

	bufput(link, data - rewind, link_end + rewind);
	*rewind_p = rewind;

	return link_end;
}

size_t
sd_autolink__url(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size)
{
	size_t link_end, rewind = 0, domain_len;

	if (size < 4 || data[1] != '/' || data[2] != '/')
		return 0;

	while (rewind < offset && isalpha(data[-rewind - 1]))
		rewind++;

	if (!sd_autolink_issafe(data - rewind, size + rewind))
		return 0;
	link_end = strlen("://");

	domain_len = check_domain(data + link_end, size - link_end);
	if (domain_len == 0)
		return 0;

	link_end += domain_len;
	while (link_end < size && !isspace(data[link_end]))
		link_end++;

	link_end = autolink_delim(data, link_end, offset, size);

	if (link_end == 0)
		return 0;

	bufput(link, data - rewind, link_end + rewind);
	*rewind_p = rewind;

	return link_end;
}

size_t
sd_autolink__subreddit(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size)
{
	size_t link_end;

	if (size < 3)
		return 0;

	/* make sure this / is part of /r/ */
	if (strncasecmp((char*)data, "/r/", 3) != 0)
		return 0;

	/* the first character of a subreddit name must be a letter or digit */
	link_end = strlen("/r/");
	if (!isalnum(data[link_end]))
		return 0;
	link_end += 1;

	/* consume valid characters ([A-Za-z0-9_]) until we run out */
	while (link_end < size && (isalnum(data[link_end]) ||
								data[link_end] == '_' ||
								data[link_end] == '+'))
		link_end++;

	/* make the link */
	bufput(link, data, link_end);
	*rewind_p = 0;

	return link_end;
}

size_t
sd_autolink__username(size_t *rewind_p, struct buf *link, uint8_t *data, size_t offset, size_t size, uint8_t *rndr_del)
{
	size_t link_end;
	uint8_t tilde_count = 0;
	
	if (size < 1)
		return 0;

	/* make sure it starts with ~ */
	if (data[0] != '~')
		return 0;

	link_end = 1;
	
	/* consume valid characters ([A-Za-z0-9_-]) until we run out */
	while (link_end < size && (isalnum(data[link_end]) ||
								data[link_end] == '_' ||
								data[link_end] == '-' ||
								data[link_end] == '~')) {
		if(data[link_end] == '~') {
			tilde_count++;
		}
		link_end++;
	}
	
	if(tilde_count && (tilde_count != 4)) {
		return 0;
	}
	
	if(link_end > 5) {
		if(data[link_end-1] == '~' && data[link_end-2] == '~') {
			if(data[1] == '~' && data[2] == '~') {
				*rndr_del = 1;
			}
		}
	}
	
	if(tilde_count && !*rndr_del) {
		return 0;
	}

	/* make the link */
	if(*rndr_del) {
		bufput(link, data+2, link_end-4);
	} else {
		bufput(link, data, link_end);
	}
	*rewind_p = 0;

	return link_end;
}
