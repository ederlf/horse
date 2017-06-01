#ifndef OF_CONN_H
#define OF_CONN_H 1

#include "base/base_of_conn.h"
#include <inttypes.h>

enum state {
    /** Sent hello message, waiting for reply */
    STATE_HANDSHAKE,
    /** Version negotiation done, features request received */
    STATE_RUNNING,
    /** Version negotiation failed, connection closed */
    STATE_FAILED,
    /** OFConnection is down, unable to send/receive data (it will be closed
    automatically). It represents a disconnection event at the controller
    level. */
    STATE_DOWN
};

/**
OFConnection events. In the descriptions below, "safe to use" means that
messages can be sent and received normally according to the OpenFlow
specification.
*/
enum ofconn_event {
    /** The connection has been started, but it is still waiting for the
    OpenFlow handshake. It is not safe to use the connection. */
    OF_EVENT_STARTED,

    /** The connection has been established (OpenFlow handshake complete). 
    It is safe to use the connection. */
    OF_EVENT_ESTABLISHED,

    /** The version negotiation has failed because the parts cannot talk in 
    a common OpenFlow version. It is not safe to use the connection. */
    OF_EVENT_FAILED_NEGOTIATION,

    /** The connection has been closed. It is not safe to use the
    connection. */
    OF_EVENT_CLOSED,

    /** The connection has been closed due to inactivity (no response to 
    echo requests). It is not safe to use the connection. */
    OF_EVENT_DEAD,
};

struct of_conn {
    struct base_of_conn* conn;
    int id;
    enum state state;
    uint8_t version;
    int alive;
    void* application_data;
};

struct of_conn *of_conn_new(struct base_of_conn *bconn);
void of_conn_destroy(struct of_conn *c);
void of_conn_send(struct of_conn *c, void* data, size_t len);
void of_conn_add_timed_callback(struct of_conn *oc, void* (*cb)(void*),
                                      int interval,
                                      void* arg); 
void of_conn_close(struct of_conn *oc);

#endif