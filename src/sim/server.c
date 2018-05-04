#include "server.h"
#include "lib/util.h"
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <pthread.h>

#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void event_cb(struct bufferevent *bev, short events, void *ctx);

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                           struct sockaddr *address, int socklen, void *ctx);
static void accept_error_cb(struct evconnlistener *listener, void *ctx);

struct server *
server_new(char* address, uint16_t port)
{
    struct server *s = xmalloc(sizeof(struct server));
    size_t addr_size = strlen(address) + 1;
    s->address = xmalloc(addr_size);
    strncpy(s->address, address, addr_size);
    s->port = port;
    s->event_cb = event_cb;
    return s;
}

void 
server_destroy(struct server *s)
{
    pthread_cancel(s->server_thr);
    free(s->address);
    free(s);
}

static void
read_cb(struct bufferevent *bev, void *ctx)
{
        char data[8192];
        UNUSED(ctx);
        printf("Calling read cb\n");
        size_t n;
        /* This callback is invoked when there is data to read on bev. */
        for (;;) {
            n = bufferevent_read(bev, data, sizeof(data));
            if (n <= 0) {
                /* Done. */
                break;
            }
        } 
        printf("%s\n", data);
        // struct evbuffer *input = bufferevent_get_input(bev);
        // struct evbuffer *output = bufferevent_get_output(bev);
        /* Copy all the data from the input buffer to the output buffer. */
        // evbuffer_add_buffer(output, input);
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
        UNUSED(ctx);
        UNUSED(address);
        UNUSED(socklen);
        /* We got a new connection! Set up a bufferevent for it. */
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(
                base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
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
    /* Listen on 172.20.254.254 */
    sin.sin_addr.s_addr = htonl(2887057150);
    // sin.sin_addr.s_addr = htonl(0);
    /* Listen on the given port. */
    sin.sin_port = htons(s->port);

    listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
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

void server_start(struct server *s)
{
    pthread_create(&s->server_thr, (pthread_attr_t*)0, server_listen, (void*)s);
}