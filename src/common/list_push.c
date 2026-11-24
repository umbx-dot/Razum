#include "common/list.h"

#include <stdlib.h>

#include "common/diagnostics.h"

void list_push(PointerList *list, void *item) {
    if (list->count == list->capacity) {
        int capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        void **items = realloc(list->items, (size_t)capacity * sizeof *items);
        if (items == NULL) {
            fatal_error("out of memory");
        }
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count++] = item;
}
