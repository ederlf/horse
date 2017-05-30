#include "of_client.h"
#include "base/of.h"

static void* send_echo(void* arg);
static void base_message_callback(struct of_client *ofc, 
                        struct base_of_conn* c, 
                        void* data, size_t len);
static void base_connection_callback(struct of_client *ofc, 
                              struct base_of_conn* c, 
                              enum conn_event event_type);
static void free_data(void *data);


struct of_client *of_client_new(int id, 
                                     char* address, int port,
                                    struct of_settings *ofsc)
{
    struct of_client *oc = malloc(sizeof(struct of_client));
    oc->base.id = id;
    memcpy(oc->base.address, address, strlen(address) + 1);
    oc->base.port = port;
    oc->ofsc = ofsc;
    oc->conn = NULL;
    oc->base.ofh.base_connection_callback = base_connection_callback;
    oc->base.ofh.base_message_callback = base_message_callback;
    oc->base.ofh.free_data = free_data;
}

int of_client_start(struct of_client *oc, int block)
{
    return base_of_client_start(&oc->base, block);
}

void of_client_start_conn(struct of_client *oc){
    base_of_client_start_conn(&oc->base);
}

void of_client_stop_conn(struct of_client *oc){
    if (oc->conn != NULL)
        of_conn_close(oc->conn);
}

void of_client_stop(struct of_client *oc) {
    of_client_stop_conn(oc);
    base_of_client_stop(&oc->base);
}

void base_message_callback(struct of_client *ofc, 
                        struct base_of_conn* c, 
                        void* data, size_t len) {
    uint8_t type = ((uint8_t*) data)[1];
    struct of_conn *cc = (struct of_conn*) c->manager;
    struct of_settings *ofsc = ofc->ofsc;
    // We trust that the other end is using the negotiated protocol
    // version. Should we?

    if (ofsc->liveness_check && type == OFPT_ECHO_REQUEST) {
        uint8_t msg[8];
        memset((void*) msg, 0, 8);
        msg[0] = ((uint8_t*) data)[0];
        msg[1] = OFPT_ECHO_REPLY;
        ((uint16_t*) msg)[1] = htons(8);
        ((uint32_t*) msg)[1] = ((uint32_t*) data)[1];
        // TODO: copy echo data
        of_conn_send(cc, msg, 8);

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->handshake && type == OFPT_HELLO) {
        uint8_t version = ((uint8_t*) data)[0];
        if (!ofsc->supported_versions & (1 << (version - 1))) {
            uint8_t msg[12];
            memset((void*) msg, 0, 8);
            msg[0] = version;
            msg[1] = OFPT_ERROR;
            ((uint16_t*) msg)[1] = htons(12);
            ((uint32_t*) msg)[1] = ((uint32_t*) data)[1];
            ((uint16_t*) msg)[4] = htons(OFPET_HELLO_FAILED);
            ((uint16_t*) msg)[5] = htons(OFPHFC_INCOMPATIBLE);
            of_conn_send(cc, msg, 12);
            of_conn_close(cc);
            cc->state = STATE_FAILED;
            ofc->connection_callback(cc, OF_EVENT_FAILED_NEGOTIATION);
        } else {
            if (ofsc->is_controller) {
                struct ofp_header msg;
                msg.version = ((uint8_t*) data)[0];
                msg.type = OFPT_FEATURES_REQUEST;
                msg.length = htons(8);
                msg.xid = ((uint32_t*) data)[1];
                of_conn_send(cc, &msg, 8);
            }
        }

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->liveness_check && type == OFPT_ECHO_REPLY) {
        if (ntohl(((uint32_t*) data)[1]) == ECHO_XID) {
            cc->alive = true;
        }

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->handshake && !ofsc->is_controller && type == OFPT_FEATURES_REQUEST) {
        struct ofp_switch_features reply;

        cc->version = (((uint8_t*) data)[0]);
        cc->state = (STATE_RUNNING);
        reply.header.version = ((uint8_t*) data)[0];
        reply.header.type = OFPT_FEATURES_REPLY;
        reply.header.length = htons(sizeof(reply));
        reply.header.xid = ((uint32_t*) data)[1];
        reply.datapath_id = ofsc->datapath_id;
        reply.n_buffers = ofsc->n_buffers;
        reply.n_tables = ofsc->n_tables;
        reply.auxiliary_id = ofsc->auxiliary_id;
        reply.capabilities = ofsc->capabilities;
        of_conn_send(cc, &reply, sizeof(reply));

        if (ofsc->liveness_check)
            of_conn_add_timed_callback(cc, send_echo, ofsc->echo_interval * 1000, cc );
        ofc->connection_callback(cc, OF_EVENT_ESTABLISHED);

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    // Handle feature replies
    if (ofsc->handshake && ofsc->is_controller && type == OFPT_FEATURES_REPLY) {
        cc->version = (((uint8_t*) data)[0]);
        cc->state = (STATE_RUNNING);
        if (ofsc->liveness_check)
            of_conn_add_timed_callback(cc, send_echo, ofsc->echo_interval * 1000, cc);
        ofc->connection_callback(cc, OF_EVENT_ESTABLISHED);
        goto dispatch;
    }


    goto dispatch;

    // Dispatch a message and goto done
    dispatch:
        ofc->message_callback(cc, type, data, len);
        goto done;
    // Free the message and return
    done:
        free(data);
        // c->free_data(data);
        return;
}

void base_connection_callback(struct of_client *ofc,
                              struct base_of_conn* c, 
                              enum conn_event event_type) {
    /* If the connection was closed, destroy it.
    There's no need to notify the user, since a DOWN event already
    means a CLOSED event will happen and nothing should be expected from
    the connection. */
    if (event_type == EVENT_CLOSED) {
        base_of_client_base_connection_callback(c, event_type);
        // TODO: delete the OFConnection?
        return;
    }

    int conn_id = c->id;
    if (event_type == EVENT_UP) {
        if (ofc->ofsc->handshake) {
            struct ofp_hello msg;
            msg.header.version = ofc->ofsc->max_supported_version;
            msg.header.type = OFPT_HELLO;
            msg.header.length = htons(8);
            msg.header.xid = htonl(HELLO_XID);
            base_of_conn_send(c, &msg, 8);
        }
        ofc->conn = of_conn_new(c);
        ofc->connection_callback(ofc->conn, OF_EVENT_STARTED);
    }
    else if (event_type == EVENT_DOWN) {
        ofc->connection_callback(ofc->conn, EVENT_CLOSED);
    }
}


static void* send_echo(void* arg) {
    struct of_conn *cc = (struct of_conn*) arg;

    if (!cc->alive) {
        of_conn_close(cc);
        cc->ofhandler->connection_callback(cc, OF_EVENT_DEAD);
        return NULL;
    }

    uint8_t msg[8];
    memset((void*) msg, 0, 8);
    msg[0] = (uint8_t) cc->version;
    msg[1] = OFPT_ECHO_REQUEST;
    ((uint16_t*) msg)[1] = htons(8);
    ((uint32_t*) msg)[1] = htonl(ECHO_XID);

    cc->alive = 0;
    of_conn_send(cc, msg, 8);

    return NULL;
}