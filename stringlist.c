/**
 *	umsftpd - User-mode SFTP application.
 *	Copyright (C) 2021-2021 Johannes Bauer
 *
 *	This file is part of umsftpd.
 *
 *	umsftpd is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	umsftpd is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with umsftpd; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stringlist.h"

struct stringlist_t* stringlist_new(void) {
	struct stringlist_t *list = calloc(1, sizeof(struct stringlist_t));
	if (!list) {
		return NULL;
	}
	list->sorted = true;
	return list;
}

bool stringlist_insert(struct stringlist_t *list, const char *string) {
	char *str_copy = strdup(string);
	if (!str_copy) {
		return false;
	}

	char **list_realloced = realloc(list->strings, sizeof(char*) * (list->count + 1));
	if (!list_realloced) {
		free(str_copy);
		return false;
	}

	list->strings = list_realloced;
	list->count++;
	list->strings[list->count - 1] = str_copy;
	list->sorted = false;
	return true;
}

static int stringlist_cmp(const void *vstrptr1, const void *vstrptr2) {
	const char **strptr1 = (const char**)vstrptr1;
	const char **strptr2 = (const char**)vstrptr2;
	const char *str1 = *strptr1;
	const char *str2 = *strptr2;
	return strcmp(str1, str2);
}

void stringlist_sort(struct stringlist_t *list) {
	if (list->count) {
		qsort(list->strings, list->count, sizeof(char*), stringlist_cmp);
		list->sorted = true;
	}
}

void stringlist_free(struct stringlist_t *list) {
	for (unsigned int i = 0; i < list->count; i++) {
		free(list->strings[i]);
	}
	free(list->strings);
	free(list);
}
