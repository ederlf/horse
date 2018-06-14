#include "signal_handler.h"
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h>
#include <errno.h>

static void handle_sigchld(int sig) 
{
  (void) sig;
  int saved_errno = errno;
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
}

void register_handle_sigchild(void)
{
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror(0);
        exit(1);
    }
}