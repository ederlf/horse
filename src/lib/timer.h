#ifndef TIMER_H
#define TIMER_H 1

#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

struct timer {
    void* (*exec)(void*);     /* Function to be executed inside the timer */
    pthread_t timer_thread;  /* thread in which user timer functions execute */
    void *arg; /*Argument passed to the executing thread */
    bool timer_stopped;
};

void init_timer(struct timer *t, void* arg);
void shutdown_timer(struct timer *t);  
void set_periodic_timer(long delay); 

#endif