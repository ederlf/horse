#ifndef ACTION_SET_H
#define ACTION_SET_H 1

#include "action.h"
#include <stdbool.h>

struct action_set {
    struct action *actions;
};

void action_set_init(struct action_set *as);
void action_set_clean(struct action_set *as);
void action_set_add(struct action_set *as, struct action act);
void action_set_merge(struct action_set *as_orig, struct action_set *as_merge);

#endif