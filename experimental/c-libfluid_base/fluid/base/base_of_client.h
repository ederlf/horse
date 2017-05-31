#ifndef BASE_OF_CLIENT
#define BASE_OF_CLIENT 1

#include "base_of_conn.h"
#include <pthread.h>

struct base_of_client {
    struct base_of_handler ofh;
    int id;
    int blocking;
    struct ev_loop *evloop;
    pthread_t t;
    pthread_t conn_t;
    char address[12];
    int port;
};

void base_of_client_init(struct base_of_client *oc, 
                         int id, const char *address, int port);
int base_of_client_start(struct base_of_client *conn, int block);
void base_of_client_stop(struct base_of_client *conn);

/* Need to initialize  these 3 to ofhandle */
void base_of_client_start_conn(struct base_of_client *conn);
void base_of_client_base_connection_callback(struct base_of_conn *conn, enum conn_event event_type);
void base_of_client_free_data(void *data);

#endif