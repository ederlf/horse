#ifndef REDIS_IPC
#define REDIS_IPC 1

#include "hiredis/hiredis.h"

#define TIME_CHANNEL "time"

struct redis_ipc {
    int port;
    char *hostname;     
    redisContext *c;
};


struct redis_ipc *redis_ipc_new(const char *hostname, int port);
void redis_ipc_destroy(struct redis_ipc *ri); 
void publish_time(struct redis_ipc *ri, uint64_t time);


#endif