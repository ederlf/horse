#include "server.h"
#include "lib/util.h"
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <pthread.h>

#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

static void event_cb(struct bufferevent *bev, short events, void *ctx);

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                           struct sockaddr *address, int socklen, void *ctx);
static void accept_error_cb(struct evconnlistener *listener, void *ctx);

// static int evpthread_on = 0;

static int conn_id = 0;

struct server *
server_new(char* address, uint16_t port)
{
    struct server *s = xmalloc(sizeof(struct server));
    size_t addr_size = strlen(address) + 1;
    s->address = xmalloc(addr_size);
    strcpy(s->address, address);
    s->port = port;
    s->event_cb = event_cb;
    s->connections = NULL;
    // if (!evpthread_on) { 
    //     evthread_use_pthreads();
    //     evpthread_on = 1;
    // }
    return s;
}

void 
server_destroy(struct server *s)
{
    struct conn *c, *ctmp;
    pthread_cancel(s->server_thr);
    HASH_ITER(hh, s->connections, c, ctmp) {
        HASH_DEL(s->connections, c);
        bufferevent_free(c->bev);
        free(c);
    }
    free(s->address);
    free(s);
}

static void
event_cb(struct bufferevent *bev, short events, void *ctx)
{
    UNUSED(ctx);
    if (events & BEV_EVENT_ERROR)
            perror("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
    }
}

static void
accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
        UNUSED(address);
        UNUSED(socklen);
        struct server *s = (struct server*) ctx;
        struct conn *c =  xmalloc(sizeof(struct conn));
        /* We got a new connection! Set up a bufferevent for it. */
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(
                base, fd, BEV_OPT_CLOSE_ON_FREE);
        c->id = conn_id;
        c->bev = bev;
        c->owner = s;
        HASH_ADD(hh, s->connections, id, sizeof(int), c);
        bufferevent_setcb(bev, s->read_cb, NULL, s->event_cb, (void*)c);
        bufferevent_enable(bev, EV_READ|EV_WRITE);        
        conn_id++;
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        UNUSED(ctx);
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Got an error %d (%s) on the listener. "
                "Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(base, NULL);
}

static void *
server_listen(void *arg){
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;

    struct server *s = (struct server*) arg;
    base = event_base_new();
    if (!base) {
        /* Needs to check where it goes to clean in case of error */
        fprintf(stderr, "Couldn't open event base");
        return NULL;
    }

    /* Clear the sockaddr before using it, in case there are extra
     * platform-specific fields that can mess us up. */
    memset(&sin, 0, sizeof(sin));
    /* This is an INET address */
    sin.sin_family = AF_INET;
    /* Listen to 172.20.254.254 */
    sin.sin_addr.s_addr = htonl(2887057150);
    // sin.sin_addr.s_addr = htonl(0);
    /* Listen on the given port. */
    sin.sin_port = htons(s->port);

    listener = evconnlistener_new_bind(base, accept_conn_cb, (void*)s,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
            perror("Couldn't create listener");
            return NULL;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    event_base_dispatch(base); 
    return NULL;
}

void server_send(struct server *s, int conn_id, uint8_t *data, size_t len)
{   
    struct conn *c;
    HASH_FIND(hh, s->connections, &conn_id, sizeof(int), c);
    if (c != NULL){
        bufferevent_write(c->bev, data, len);
    }
}

void server_start(struct server *s)
{   
    s->event_cb = event_cb;
    pthread_create(&s->server_thr, (pthread_attr_t*)0, server_listen, (void*)s);
}