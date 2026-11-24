#pragma once

typedef struct {
    void **items;
    int count;
    int capacity;
} PointerList;

#define LIST_AT(list, type, index) ((type *)(list).items[(index)])

void list_push(PointerList *list, void *item);
