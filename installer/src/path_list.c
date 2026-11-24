#include "setup.h"

#include <wctype.h>

static size_t without_trailing_slash(const wchar_t *text, size_t length) {
    while (length > 0 && (text[length - 1] == L'\\' || text[length - 1] == L'/')) {
        length--;
    }
    return length;
}

static bool same_entry(const wchar_t *item, size_t item_length, const wchar_t *entry) {
    size_t left = without_trailing_slash(item, item_length);
    size_t right = without_trailing_slash(entry, wcslen(entry));
    if (left != right) {
        return false;
    }
    for (size_t index = 0; index < left; index++) {
        if (towlower(item[index]) != towlower(entry[index])) {
            return false;
        }
    }
    return true;
}

static bool append(wchar_t *out, size_t capacity, size_t *used, const wchar_t *text, size_t length) {
    if (*used + length + 1 > capacity) {
        return false;
    }
    wmemcpy(out + *used, text, length);
    *used += length;
    out[*used] = L'\0';
    return true;
}

bool path_list_contains(const wchar_t *list, const wchar_t *entry) {
    for (const wchar_t *item = list; *item != L'\0';) {
        size_t length = wcscspn(item, L";");
        if (length > 0 && same_entry(item, length, entry)) {
            return true;
        }
        item += length + (item[length] == L';');
    }
    return false;
}

bool path_list_add(const wchar_t *list, const wchar_t *entry, wchar_t *out, size_t capacity) {
    size_t used = 0;
    out[0] = L'\0';
    size_t list_length = wcslen(list);
    if (!append(out, capacity, &used, list, list_length)) {
        return false;
    }
    if (path_list_contains(list, entry)) {
        return true;
    }
    if (list_length > 0 && list[list_length - 1] != L';' && !append(out, capacity, &used, L";", 1)) {
        return false;
    }
    return append(out, capacity, &used, entry, wcslen(entry));
}

bool path_list_remove(const wchar_t *list, const wchar_t *entry, wchar_t *out, size_t capacity) {
    size_t used = 0;
    out[0] = L'\0';
    for (const wchar_t *item = list; *item != L'\0';) {
        size_t length = wcscspn(item, L";");
        if (length > 0 && !same_entry(item, length, entry)) {
            if ((used > 0 && !append(out, capacity, &used, L";", 1)) || !append(out, capacity, &used, item, length)) {
                return false;
            }
        }
        item += length + (item[length] == L';');
    }
    return true;
}
