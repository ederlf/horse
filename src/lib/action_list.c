#include "action_list.h"
#include <uthash/utlist.h>

void 
action_list_init(struct action_list *al)
{
    al->actions = NULL;
}

void action_list_clean(struct action_list *al)
{
    struct action_list_elem *nxt_elem, *tmp;
    LL_FOREACH_SAFE(al->actions, nxt_elem, tmp){
        LL_DELETE(al->actions, nxt_elem);
        free(nxt_elem);
    }
}

void 
action_list_add(struct action_list *al, struct action act)
{
    struct action_list_elem *elem;
    elem = xmalloc(sizeof(struct action_list_elem));
    elem->act = act;
    LL_APPEND(al->actions, elem);
}
