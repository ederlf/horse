#ifndef SERVER_H
#define SERVER_H 1

#include <event2/bufferevent.h>

struct server {
    uint16_t port;
    char *address; 
    pthread_t server_thr;
    struct bufferevent *bev;
    void *owner; 
    void (*read_cb) (struct bufferevent *bev, void *ctx);
    void (*event_cb) (struct bufferevent *bev, short events, void *ctx);
};

struct server *server_new(char* address, uint16_t port);
void server_destroy(struct server *s);
void server_start(struct server *s);
void server_send(struct server* s, uint8_t *data, size_t len);

#endif