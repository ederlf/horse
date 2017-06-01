#include "of_conn.h"

struct of_conn *of_conn_new(struct base_of_conn *bconn)
{
    struct of_conn *c = malloc(sizeof(struct of_conn));
    c->conn = bconn;
    c->conn->manager = c;
    c->id = bconn->id;
    c->state = STATE_HANDSHAKE;
    c->alive = 1;
    c->version = 0;
    c->application_data = NULL;
    return c;
}

void of_conn_destroy(struct of_conn *c)
{
    free(c);
}

void of_conn_send(struct of_conn *c, void* data, size_t len)
{
    if (c->conn != NULL)
        base_of_conn_send(c->conn, (uint8_t*) data, len);
}

void of_conn_add_timed_callback(struct of_conn *oc, void* (*cb)(void*),
                                      int interval,
                                      void* arg) {
    if (oc->conn != NULL)
        base_of_conn_add_timed_callback(oc->conn, cb, interval, arg);
}

void of_conn_close(struct of_conn *oc) {
    // Don't close twice
    if (oc->conn == NULL)
        return;

    oc->state = STATE_DOWN;
    // Close the BaseOFConnection. This will trigger
    // BaseOFHandler::base_connection_callback. Then BaseOFServer will take
    // care of freeing it for us, so we can lose track of it.
    base_of_conn_close(oc->conn);
    oc->conn = NULL;
}