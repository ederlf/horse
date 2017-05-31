#include "base_of_conn.h"
#include "vector.h"
#include <event2/event.h>
#include <event2/bufferevent.h>

struct leventbaseconn{
    struct bufferevent* bev;
    struct event* close_event; 
};

static void notify_msg_cb(struct base_of_conn *conn, 
                          void* data, size_t n);
static void notify_conn_cb(struct base_of_conn *conn, 
                           enum conn_event event_type);
static void do_close(struct base_of_conn *conn);


static void event_cb(struct bufferevent *bev, short events, void* arg)
{
    struct base_of_conn* c = (struct base_of_conn*) arg;

    if (events & BEV_EVENT_ERROR)
        perror("Connection error");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        notify_conn_cb(c, EVENT_DOWN);
    }
}

static void timer_callback(evutil_socket_t fd, short what, void *arg)
{
    struct timed_callback* tc = (struct timed_callback*) arg;
    tc->cb(tc->cb_arg);
}

static void read_cb(struct bufferevent *bev, void* arg)
{
    struct base_of_conn* c = (struct base_of_conn*) arg;
    
    uint16_t len;
    struct ofp_buffer *ofbuf = c->ofb;

    while (1) {
        // Decide how much we should read
        len = get_read_len(ofbuf);
        if (len <= 0) break;

        // Read the data and put it in the buffer
        size_t read = bufferevent_read(bev, get_read_pos(ofbuf), len);
        if (read <= 0) break; else read_notify(ofbuf, read);

        // Check if the message is fully received and dispatch
        if (is_ready(ofbuf)) {
            void* data = get_data(ofbuf);
            size_t len = get_len(ofbuf);
            clear(ofbuf, 0);
            notify_msg_cb(c, data, len);
        }
    }
}

static void close_cb(int fd, short which, void *arg) {
    struct base_of_conn *c = (struct base_of_conn *) arg;
    do_close(c);
}

struct base_of_conn *base_of_conn_new(int id,
                        struct base_of_handler* ofhandler,
                        struct ev_loop* evloop,
                        void *owner,
                        int fd)
{
    struct base_of_conn *conn = malloc(sizeof(struct base_of_conn));
    conn->id = id;
    // TODO: move event_base to BaseOFConnection::LibEventBaseOFConnection so
    // we don't need to store this here
    conn->evl = evloop;
    conn->ofb = ofp_buffer_new();
    conn->manager = NULL;
    conn->owner = owner;
    conn->ofh = ofhandler;
    conn->timed_callbacks = NULL;
    conn->lev_base = malloc(sizeof(struct leventbaseconn));

    struct event_base* base = evloop->base;
    conn->lev_base->close_event = event_new(base,-1,EV_PERSIST,
                                            close_cb, conn);
    event_add(conn->lev_base->close_event, NULL);

    
    conn->lev_base->bev = bufferevent_socket_new(base,
                                       fd,
                                       BEV_OPT_CLOSE_ON_FREE |
                                       BEV_OPT_THREADSAFE);

    notify_conn_cb(conn, EVENT_UP);

    bufferevent_setcb(conn->lev_base->bev,
                      read_cb,
                      NULL,
                      event_cb,
                      conn);
    bufferevent_enable(conn->lev_base->bev, EV_READ|EV_WRITE);
}

void base_of_conn_destroy(struct base_of_conn *conn)
{
    free(conn->lev_base);
}

static void notify_msg_cb(struct base_of_conn *conn, void* data, size_t n)
{
    conn->ofh->base_message_callback(conn, data, n);
}

static void notify_conn_cb(struct base_of_conn *conn, enum conn_event event_type)
{
    conn->ofh->base_connection_callback(conn, event_type);
}

static void do_close(struct base_of_conn *conn)
{
    // Stop all timed callbacks
    tc_ptr *v = conn->timed_callbacks;
    tc_ptr tc;
    if(conn->timed_callbacks) {
        tc_ptr *it;
        for(it = vector_begin(v); it != vector_end(v); ++it) {
            tc = *it;
            event_del((struct event*) tc->data);
            event_free((struct event*) tc->data);
            free(tc); 
        }
    }

    // Stop the events and delete the buffers
    event_del(conn->lev_base->close_event);
    event_free(conn->lev_base->close_event);

    bufferevent_free(conn->lev_base->bev);
    ofp_buffer_destroy(conn->ofb);
    conn->ofb = NULL;

    notify_conn_cb(conn, EVENT_CLOSED);
}

void base_of_conn_close(struct base_of_conn *conn) {
    event_active(conn->lev_base->close_event, EV_READ, 0);
}

void base_of_conn_send(struct base_of_conn *conn, void* data, size_t len) {
    bufferevent_write(conn->lev_base->bev, data, len);
}

void base_of_conn_add_timed_callback(struct base_of_conn *conn, void* (*cb)(void*), int interval, void* arg) {
    struct timeval tv = { interval / 1000, (interval % 1000) * 1000 };
    struct timed_callback* tc = malloc(sizeof(struct timed_callback));
    tc->cb = cb;
    tc->cb_arg = arg;
    struct event_base* base = (struct event_base*) conn->evl->base;
    struct event* ev = event_new(base,
                                 -1,
                                 EV_PERSIST,
                                 timer_callback,
                                 tc);
    tc->data = ev;
    vector_push_back(conn->timed_callbacks, tc);
    event_add(ev, &tv);
}
