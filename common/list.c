#include "common/list.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void list_init(list_t *list)
{
	list->capacity = 0;
	list->length = 0;
	list->items = NULL;
}

list_t *create_list(void)
{
	list_t *ret = calloc(sizeof(*ret), 1);
	list_init(ret);

	return ret;
}

void list_free(list_t *list)
{
	if (!list) {
		return;
	}
	/* Either NULL or alloced. This **has** to work */
	free(list->items);
	free(list);
}

void list_foreach(list_t *list, void(*callback)(void *item))
{
	if (!list) {
		return;
	}

	for (unsigned i = 0; i < list->length; ++i) {
		callback(list->items[i]);
	}
}

void list_add(list_t *list, void *item)
{
	assert(list);

	if (list->capacity <= list->length) {
		if (!list->capacity) {
			list->capacity = 2;
		}

		list->capacity = list->capacity * 2;
		list->items = realloc(list->items, sizeof(void *) * list->capacity);
	}

	list->items[list->length] = item;
	++list->length;
}

void list_del(list_t *list, unsigned index)
{
	assert(list);

	if (index >= list->length) {
		return;
	}

	--list->length;
	list->items[index] = list->items[list->length];

	if (list->length < list->capacity / 2) {
		list->capacity = list->capacity / 2;
		list->items = realloc(list->items, sizeof(void *) * list->capacity);
	}
}

void list_append(list_t *restrict dst, const list_t *restrict src)
{
	assert(dst);
	assert(src);

	if (dst->capacity < src->length + dst->length) {
		dst->capacity = src->length + dst->length;
		dst->items = realloc(dst->items, sizeof(void *) * dst->capacity);
	}

	memcpy(&dst->items[dst->length], src->items, sizeof(void *) * src->length);
	dst->length = src->length + dst->length;
}
