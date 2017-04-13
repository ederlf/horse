#ifndef HOST_H
#define HOST_H 1

struct host; 

struct host *host_new(void);
void host_destroy(struct host *h);

#endif

