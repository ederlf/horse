#ifndef OFCLIENT_H
#define OFCLIENT_H 1

#include "base/base_of_client.h"
#include "of_conn.h"

struct of_client {
    struct base_of_client base;
    struct of_conn *conn;

};

struct of_client *of_client_new(int id, char* address, int port
             /*const struct OFServerSettings ofsc*/);
int of_client_start(struct of_client *oc, int block);
void of_client_start_conn(struct of_client *oc);
void of_client_stop_conn(struct of_client *oc);
void of_client_stop(struct of_client *oc);
void base_message_callback(struct base_of_conn* c, void* data, size_t len);
void base_connection_callback(struct base_of_conn* c, 
                              enum conn_event event_type);

#endif