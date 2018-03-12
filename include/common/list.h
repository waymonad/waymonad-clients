#ifndef _COMMON_LIST_H_
#define _COMMON_LIST_H_

typedef struct {
	unsigned capacity;
	unsigned length;
	void **items;
} list_t;

list_t *create_list(void);
void list_init(list_t *list);
void list_free(list_t *list);
void list_foreach(list_t *list, void (*callback)(void* item));
void list_add(list_t *restrict list, void *item);
void list_del(list_t *restrict list, unsigned index);
void list_append(list_t *restrict dst, const list_t *restrict src);

#endif /* _COMMON_LIST_H_ */
