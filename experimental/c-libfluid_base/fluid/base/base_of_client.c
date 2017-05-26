#include "base_of_client.h"

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
                                               sock);
    return NULL;
}

void base_of_client_start_conn(struct base_of_client *conn)
{
    pthread_create(&conn->conn_t, NULL,
                      try_connect,
                       conn);
}

int base_of_client_start(struct base_of_client *conn, int block)
{
    conn->blocking = block;
    base_of_client_start_conn(conn);
    if (!conn->blocking) {
        pthread_create(&conn->t,
                       NULL,
                       thread_adapter,
                       conn->evloop);
    }
    else {
        ev_loop_run(conn->evloop);
    }
    return 1;
}

void base_of_client_stop(struct base_of_client *conn) {
    pthread_cancel(conn->conn_t);
    ev_loop_stop(conn->evloop);
    if (!conn->blocking) {
        pthread_join(conn->t, NULL);
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