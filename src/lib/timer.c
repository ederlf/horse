/* Written by Godmar Back <gback@cs.vt.edu>, February 2006.*/

#include "timer.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

static sem_t timer_sem;         /* signaled if timer signal arrived*/

/* Timer signal handler.
 * On each timer signal, the signal handler will signal a semaphore.
 */
static void
timersignalhandler(int signum) 
{
    signum = signum;
    /* called in signal handler context, we can only call 
     * async-signal-safe functions now!
     */
    sem_post(&timer_sem);   // the only async-signal-safe function pthreads defines
}

/* Timer thread.
 * This dedicated thread waits for posts on the timer semaphore.
 * For each post, timer_func() is called once.
 * 
 * This ensures that the timer_func() is not called in a signal context.
 */
static void *
timerthread(void *args) 
{
    struct timer *t = (struct timer*) args;
    while (!t->timer_stopped) {
        int rc = sem_wait(&timer_sem);      // retry on EINTR
        if (rc == -1 && errno == EINTR)
            continue;
        if (rc == -1) {
            perror("sem_wait");
            exit(-1);
        }
        t->exec(t->arg);   // user-specific, can do anything it wants
    }
    return 0;
}

/* Initialize timer */
void
init_timer(struct timer t, void* arg)
{
    t.timer_stopped = false;
    t.arg = arg;
    /* One time set up */
    sem_init(&timer_sem, /*not shared*/ 0, /*initial value*/0);
    /* Starts thread timer_thread */
    pthread_create(&t.timer_thread, (pthread_attr_t*)0, timerthread, (void*)&t);
    signal(SIGALRM, timersignalhandler);
}

/* Shut timer down */
void
shutdown_timer(struct timer t) 
{
    t.timer_stopped = true;
    sem_post(&timer_sem);
    pthread_join(t.timer_thread, 0);
}

/* Set a periodic timer.  You may need to modify this function. */
void
set_periodic_timer(long delay) 
{
    struct itimerval tval = { 
        /* subsequent firings */ .it_interval = { .tv_sec = 0, .tv_usec = delay }, 
        /* first firing */       .it_value = { .tv_sec = 0, .tv_usec = delay }};

    setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}