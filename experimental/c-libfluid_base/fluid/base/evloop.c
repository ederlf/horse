#include "evloop.h"
#include <stdlib.h>

struct ev_loop *ev_loop_new(int id)
{
    struct ev_loop *evl = malloc(sizeof(struct ev_loop));
    evl->id = id;
    evl->base = event_base_new();
    evl->stopped = 0;
    event_base_add_virtual(evl->base);
    return evl;
}

void ev_loop_destroy(struct ev_loop *ev)
{
    event_base_free(ev->base);
    free(ev);
}

void ev_loop_run(struct ev_loop *ev)
{
    if (ev->stopped) return;

    event_base_dispatch(ev->base);
    // See note in EventLoop::EventLoop. Here we disable the virtual event
    // to guarantee that nothing blocks.
    event_base_del_virtual(ev->base);
    event_base_loop(ev->base, EVLOOP_NONBLOCK);
}

void ev_loop_stop(struct ev_loop *ev)
{
    ev->stopped = 1;
    event_base_loopbreak(ev->base);
}

void* thread_adapter(void* arg)
{
    struct ev_loop *ev = (struct ev_loop*) arg;
    ev_loop_run(ev);
    return NULL;
}