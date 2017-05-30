#ifndef BASE_OF_CONN
#define BASE_OF_CONN 1

#include "ofp_buffer.h"
#include "evloop.h"

struct leventbaseconn;
struct base_of_handler;

enum conn_event {
        /** The connection has been successfully established */
        EVENT_UP,
        /** The other end has ended the connection */
        EVENT_DOWN,
        /** The connection resources have been released and freed */
        EVENT_CLOSED
    };

struct timed_callback {
        void* (*cb)(void*);
        void* cb_arg;
        void* data;
    };

typedef struct timed_callback * tc_ptr;

struct base_of_conn {
    int id;
    struct ev_loop *evl;
    struct ofp_buffer *ofb;
    struct base_of_handler *ofh;
    void *manager;
    int running;
    tc_ptr *timed_callbacks;
    struct leventbaseconn *lev_base; 
};

struct base_of_handler {
    void (*base_connection_callback) (struct base_of_conn *conn,
                                    enum conn_event event_type);
    void (*base_message_callback) (struct base_of_conn* conn,
                                       void* data,
                                       size_t len);
    void (*free_data) (void* data);                            
};

struct base_of_conn *base_of_conn_new(int id,
                        struct base_of_handler *ofhandler,
                        struct ev_loop* evloop,
                        int fd);
void base_of_conn_destroy(struct base_of_conn *conn);
void base_of_conn_send(struct base_of_conn *conn, void* data, size_t len);
void base_of_conn_add_timed_callback(struct base_of_conn *conn, void* (*cb)(void*), int interval, void* arg);
void base_of_conn_close(struct base_of_conn *conn);

#endif