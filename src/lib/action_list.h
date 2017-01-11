#ifndef ACTION_LIST_H
#define ACTION_LIST_H 1

#include "action.h"

struct action_list_elem {
    struct action act;
    struct action_list_elem *next;
};

struct action_list {
    struct action_list_elem *actions;
};

void action_list_init(struct action_list *al);
void action_list_clean(struct action_list *al);
void action_list_add(struct action_list *al, struct action act);

#endif