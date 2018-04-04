#ifndef BGP_H
#define BGP_H 1

#include "routing.h"

#define MAX_FILE_NAME_SIZE 256

struct bgp {
    struct routing base;
    char config_file[MAX_FILE_NAME_SIZE]; /* Will be removed when config is done in Python */
};

void bgp_init(struct bgp *p, char *config_file);

#endif