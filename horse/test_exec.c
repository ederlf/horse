#include <unistd.h>
#include<stdio.h>
#include <stdio.h>
#include <string.h>

#define SIZE 1000

char *erase_idx(char *p, int idx){
    memmove(&p[idx], &p[idx + 1], strlen(p) - idx);
}

char *erase_c(char *p, char ch)
{

    char *ptr;

    while (ptr = strchr(p, ch)){
        int idx = ptr - p;
        erase_idx(p, idx);
    }

    return p;
}

int main()
{
    char str[] = "\"10.0.0.1 \" exabgp.log.destination=syslog exabgp /home/vagrant/horse/python/topos/config/new/conf.ini1";
    // char str[] =  ".a man, =\"a plan\", a canal Panama";
    puts(str);  
    erase_c(str, '"');

    puts(str);

    return 0;
}

//int main(int *argc, char **argv)
//{
 //   char *cmd = "env";
 //   char *args[5];
 //   args[0] = "env";
 //   //args[1] = "exabgp.daemon.daemonize=true";
 //   args[1] = "exabgp.tcp.bind=  10.0.0.2 10.0.1.2";
 //   args[2] = "exabgp";
 //   args[3] = "/home/vagrant/horse/python/topos/config/new/conf.ini2";
 //   args[4] = NULL;
//
 //   execvp(cmd, args); //This will run "ls -la" as if it were a command
//}
