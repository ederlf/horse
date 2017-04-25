#include "app.h"
#include "ping.h"

struct app *app_creator(uint16_t type)
{
    struct app *a = xmalloc(sizeof(struct app));
    a->type = type;
    switch (type){
        case PINGV4: {
            a->handle_netflow = ping_handle_netflow;
            a->start = ping_start;
            break;
        }
    }
    return a;
}




