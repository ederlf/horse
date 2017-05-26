#include "of_client.h"
#include "base/of.h"

struct of_client *of_client_new(int id, 
                                     char* address, int port
                                    /*const struct OFServerSettings ofsc*/)
{
    struct of_client *oc = malloc(sizeof(struct of_client));
    oc->base.id = id;
    memcpy(oc->base.address, address, strlen(address) + 1);
    oc->base.port = port;
    // this->ofsc = ofsc;
    oc->conn = NULL;
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


void* of_client_send_echo(void* arg) {
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