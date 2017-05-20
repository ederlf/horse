#include "redis_ipc.h"
#include "util.h"

static void redis_connect(struct redis_ipc *ri);

struct redis_ipc 
*redis_ipc_new(const char *hostname, int port)
{
    struct redis_ipc *ri = xmalloc(sizeof(struct redis_ipc));
    int hlen = strlen(hostname) + 1;
    ri->port = port;
    ri->hostname = xmalloc(hlen);
    strncpy(ri->hostname, hostname, hlen);
    redis_connect(ri);
    return ri;
}

void redis_ipc_destroy(struct redis_ipc *ri)
{
    redisFree(ri->c);
    free(ri);
}

void publish_time(struct redis_ipc *ri, uint64_t time)
{
    redisCommand(ri->c,"PUBLISH %s %b", TIME_CHANNEL, &time, sizeof(time));
}


static void redis_connect(struct redis_ipc *ri)
{
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    ri->c = redisConnectWithTimeout(ri->hostname, ri->port, timeout);
    if (ri->c == NULL || ri->c->err) {
        if (ri->c) {
            printf("Connection error: %s\n", ri->c->errstr);
            redisFree(ri->c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }
}