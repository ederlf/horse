#ifndef OF_CONN_H
#define OF_CONN_H 1

#include "base/base_of_conn.h"
#include <inttypes.h>

struct of_handler;

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
    struct of_handler *ofhandler;
    void* application_data;
};

struct of_handler {
    /**
    Callback for connection events.
    
    This method blocks the event loop on which the connection is running, 
    preventing other connection's events from being handled. If you want to 
    perform a very long operation, try to queue it and return.

    The implementation must be thread-safe, because it will possibly be called
    from several threads (on which connection events are being created).

    @param conn the OFConnection on which the message was received
    @param event_type the event type (see #Event)
    */
    void (*connection_callback) (struct of_conn* conn, enum ofconn_event event_type);

    /**
    Callback for new messages.
    
    This method blocks the event loop on which the connection is running, 
    preventing other connection's events from being handled. If you want to 
    perform a very long operation, try to queue it and return.

    The implementation must be thread-safe, because it will possibly be called
    from several threads (on which message events are being created).

    By default, the message data will managed (freed) for you. If you want a 
    zero-copy behavior, see OFServerSettings::keep_data_ownership.

    @param conn the OFConnection on which the message was received
    @param type OpenFlow message type
    @param data binary message data
    @param len message length
    */
    void (*message_callback)(struct of_conn* conn, uint8_t type, void* data, size_t len);
    
    /**
    Free the data passed to OFHandler::message_callback.
    */
    void (*free_data)(void* data);

};

struct of_conn *of_conn_new(struct base_of_conn *bconn);
void of_conn_send(struct of_conn *c, void* data, size_t len);
void of_conn_add_timed_callback(struct of_conn *oc, void* (*cb)(void*),
                                      int interval,
                                      void* arg); 
void of_conn_close(struct of_conn *oc);

#endif