#include "action_set.h"
#include "util.h"
#include <stdbool.h>
#include <uthash/utlist.h>

static bool 
active(struct action_set *as, uint16_t type){
    return (as->active & type);
}

void action_set_init(struct action_set *as){
    as->actions = NULL;
    as->active = 0; 
}

void 
action_set_add(struct action_set *as, struct action act){
    struct action_set_elem *elem;
    if (active(as, act.type)){
        /* TODO: return error code */
        return;
    }
    elem = xmalloc(sizeof(struct action_set_elem));
    elem->act = act;
    as->active |= act.type;
    LL_APPEND(as->actions, elem);
}
