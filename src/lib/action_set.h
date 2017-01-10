#ifndef ACTION_SET_H
#define ACTION_SET_H 1

#include "action.h"

struct action_set_elem {
   struct action act;
   struct action_set_elem *next; 
};

struct action_set {
    struct action_set_elem *actions;
    uint16_t active;
};

void action_set_init(struct action_set *as);
void action_set_clean(struct action_set *as);
void action_set_add(struct action_set *as, struct action act);


#endif