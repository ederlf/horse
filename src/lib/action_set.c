#include "action_set.h"
#include "util.h"

void action_set_init(struct action_set *as){
    as->actions = NULL;
}

void action_set_clean(struct action_set *as)
{
    struct action *nxt_elem, *tmp;
    HASH_ITER(hh, as->actions, nxt_elem, tmp){
        HASH_DEL(as->actions, nxt_elem);
        free(nxt_elem);
    }
}

void 
action_set_add(struct action_set *as, struct action act){
    
    struct action *orig_action;
    uint16_t type = act.type;
    HASH_FIND(hh, as->actions, &type, sizeof(uint16_t), orig_action);
    if (orig_action != NULL) {
        /* Do not know why i cannot just copy act to orig_action...*/
        struct action *new_action;
        new_action = xmalloc(sizeof(struct action));
        memcpy(new_action, &act, sizeof(struct action));
        HASH_REPLACE(hh, as->actions, type, sizeof(uint16_t), new_action, orig_action);
        free(orig_action);
    } 
    else {
        orig_action = xmalloc(sizeof(struct action));
        memcpy(orig_action, &act, sizeof(struct action));
        HASH_ADD(hh, as->actions, type, sizeof(uint16_t), orig_action);
    }
}

void 
action_set_merge(struct action_set *as_orig, struct action_set *as_merge)
{
    struct action *act, *tmp;
    HASH_ITER(hh, as_merge->actions, act, tmp) {
        action_set_add(as_orig, *act);
    }
}

/* Return an action if it exists in the hash. Returns NULL if not */
struct action* 
action_set_action(struct action_set *as, uint16_t type)
{
    struct action *act;  
    HASH_FIND(hh, as->actions, &type, sizeof(uint16_t), act);
    return act;
}