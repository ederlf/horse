#include "base_of_client.h"

void base_of_client_init(struct base_of_client *oc, 
                         int id, const char *address, int port)
{
    evthread_use_pthreads();
    oc->id = id;
    strncpy(oc->address, address, strlen(address) + 1);
    oc->port = port;
    oc->evloop = ev_loop_new(0);
}

void base_of_client_clean(struct base_of_client *oc)
{
    ev_loop_destroy(oc->evloop);
}

static void* try_connect(void* arg)
{
    int sock;
    struct sockaddr_in echoserver;
    int received = 0;

    struct base_of_client *boc = (struct base_of_client *) arg;

    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Error creating socket");
        return NULL;
    }
    memset(&echoserver, 0, sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_addr.s_addr = inet_addr(boc->address);
    echoserver.sin_port = htons(boc->port);
    while (connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) {
        fprintf(stderr, "Retrying in 5 seconds...\n");
        sleep(5);
    }
    struct base_of_conn* c = base_of_conn_new(0,
                                               (struct base_of_handler*) boc,
                                               boc->evloop,
                                               boc,
                                               sock);
    return NULL;
}

void base_of_client_start_conn(struct base_of_client *bofc)
{
    pthread_create(&bofc->conn_t, NULL,
                      try_connect,
                       bofc);
}

int base_of_client_start(struct base_of_client *bofc, int block)
{
    bofc->blocking = block;
    base_of_client_start_conn(bofc);
    if (!bofc->blocking) {
        pthread_create(&bofc->t,
                       NULL,
                       thread_adapter,
                       bofc->evloop);
    }
    else {
        ev_loop_run(bofc->evloop);
    }
    return 1;
}

void base_of_client_stop(struct base_of_client *bofc) {
    pthread_cancel(bofc->conn_t);
    ev_loop_stop(bofc->evloop);
    if (!bofc->blocking) {
        pthread_join(bofc->t, NULL);
    }
}

void base_of_client_base_connection_callback(struct base_of_conn *conn, enum conn_event event_type) {
    if (event_type == EVENT_CLOSED) {
        base_of_conn_destroy(conn);
    }
}

void base_of_client_free_data(void *data)
{
    free(data);
}