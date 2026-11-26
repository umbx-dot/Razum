#include "project.h"

#include <string.h>

RzTarget *rz_find_target(const RzProject *project, const char *name) {
    for (int index = 0; index < project->targets.count; index++) {
        RzTarget *target = LIST_AT(project->targets, RzTarget, index);
        if (strcmp(target->name, name) == 0) {
            return target;
        }
    }
    return NULL;
}

static void visit(const RzProject *project, RzTarget *target, PointerList *order) {
    if (target->visiting == 1) {
        rz_error("target dependency cycle at '%s'", target->name);
    }
    if (target->visiting == 2) {
        return;
    }
    if (order != NULL && (target->platforms & RZ_HOST) == 0) {
        rz_error("target '%s' is unavailable on %s", target->name, rz_platform());
    }
    target->visiting = 1;
    for (int index = 0; index < target->needs.count; index++) {
        const char *name = LIST_AT(target->needs, const char, index);
        RzTarget *need = rz_find_target(project, name);
        if (need == NULL) {
            rz_error("target '%s' depends on missing target '%s'", target->name, name);
        }
        visit(project, need, order);
    }
    target->visiting = 2;
    if (order != NULL) {
        list_push(order, target);
    }
}

PointerList rz_build_order(const RzProject *project, const char *requested) {
    PointerList order = {0};
    for (int index = 0; index < project->targets.count; index++) {
        visit(project, LIST_AT(project->targets, RzTarget, index), NULL);
    }
    for (int index = 0; index < project->targets.count; index++) {
        LIST_AT(project->targets, RzTarget, index)->visiting = 0;
    }
    if (requested != NULL) {
        RzTarget *target = rz_find_target(project, requested);
        if (target == NULL) {
            rz_error("unknown target '%s'", requested);
        }
        visit(project, target, &order);
    } else {
        for (int index = 0; index < project->targets.count; index++) {
            RzTarget *target = LIST_AT(project->targets, RzTarget, index);
            if ((target->platforms & RZ_HOST) != 0) {
                visit(project, target, &order);
            }
        }
    }
    if (order.count == 0) {
        rz_error("no build targets available for %s", rz_platform());
    }
    return order;
}
