#include "app.h"
#include "ping.h"
#include "raw_udp.h"
#include <uthash/utlist.h>

struct app *app_creator(uint16_t type)
{
    struct app *a = xmalloc(sizeof(struct app));
    a->type = type;
    switch (type){
        case UDP: {
            a->handle_netflow = raw_udp_handle_netflow;
            a->start = raw_udp_start;
            break;
        }
        case PINGV4: {
            a->handle_netflow = ping_handle_netflow;
            a->start = ping_start;
            break;
        }
    }
    return a;
}

struct exec *app_new_exec(uint64_t id, uint32_t type, uint32_t execs_num, 
                        uint64_t start_time, void *args, size_t arg_size)
{
    struct exec *exec = xmalloc(sizeof(struct exec));
    exec->id = id;
    exec->type = type;
    exec->exec_cnt = execs_num;
    exec->start_time = start_time;
    exec->args = xmalloc(arg_size);
    memcpy(exec->args, args, arg_size);
    return exec;
}

void app_destroy(struct app *app)
{
    free(app);
}

void app_destroy_exec(struct exec *exec)
{
    free(exec->args);
    free(exec);
}