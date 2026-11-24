#include "checking/checking.h"

void scopes_push(Scopes *scopes) {
    scopes->depth++;
}

void scopes_pop(Scopes *scopes) {
    while (scopes->entries.count > 0 && LIST_AT(scopes->entries, ScopeEntry, scopes->entries.count - 1)->depth == scopes->depth) {
        scopes->entries.count--;
    }
    scopes->depth--;
}
