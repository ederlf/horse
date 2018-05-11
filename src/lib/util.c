/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "util.h"

void out_of_memory()
{
    fprintf(stderr, "No memory available. Aborting.\n");
    abort();
}

void file_read_error(){
    fprintf(stderr, "Failed to read the file. Aborting.\n");
    abort();
}

void *
xmalloc(size_t size)
{
    void *v = malloc(size? size : 1);
    if (v == NULL){
        out_of_memory();
    }
    return v;
}

void *
xrealloc(void *v, int size)
{
    v = realloc(v, size? size : 1);
    if (v == NULL){
        out_of_memory();
    }    
    return v;
}

/* Counts the number of trailing zeros in a word.
 * Extracted from the book Hacker's Delight. */ 
static int 
pop(uint32_t x) {
   x = x - ((x >> 1) & 0x55555555);
   x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
   x = (x + (x >> 4)) & 0x0F0F0F0F;
   x = x + (x << 8);
   x = x + (x << 16);
   return x >> 24;
}

int 
nlz(uint32_t x) {
   x = x | (x >> 1);
   x = x | (x >> 2);
   x = x | (x >> 4);
   x = x | (x >> 8);
   x = x | (x >>16);
   return pop(~x);
}

int 
ntz(unsigned x) {
   return pop(~x & (x - 1));
}

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
uint32_t 
random_at_most(uint32_t max) {
    uint32_t num_bins = (uint32_t) max + 1;
    uint32_t num_rand = (uint32_t) RAND_MAX + 1;
    uint32_t bin_size = num_rand / num_bins;
    uint32_t defect   = num_rand % num_bins;

    uint32_t x;
    do {
        x = rand();
    }
    // This is carefully written not to overflow
    while (num_rand - defect <= (uint32_t)x);

    // Truncated division is intentional
    return x/bin_size;
}

/* Return the size of the string for reuse */
char* 
file_to_string(const char * file_name, size_t *size){
    FILE *fh = fopen(file_name, "r");
    char *str_file = NULL; 
    if ( fh != NULL ){
        size_t s;
        fseek(fh, 0L, SEEK_END);
        s = ftell(fh);
        rewind(fh);
        str_file = malloc(s);
        if (str_file != NULL){
            if (fread(str_file, s, 1, fh) < 1){
                /* Abort in case of failure */
                file_read_error();
            }
        }
        fclose(fh);
        *size = s;
    }
    return str_file;
}

uint8_t 
get_ip_family(char *ip)
{
    if ( strchr(ip,'.') ) {
        return AF_INET;
    }
    else if ( strchr(ip,':') ) {
        return AF_INET6;
    }
    else {
        fprintf(stderr, "Address %s is from unknown family", ip);
        return AF_MAX;
    } 
}

int
ip_str_addr_compare(char *ip1, char *ip2, uint8_t addr_family)
{
    if ( addr_family == AF_INET ) {
        struct sockaddr_in sa1, sa2;
        inet_pton(AF_INET, ip1, &(sa1.sin_addr));
        inet_pton(AF_INET, ip2, &(sa2.sin_addr));
        return memcmp(&sa1.sin_addr.s_addr, &sa2.sin_addr.s_addr,
                      sizeof(uint32_t));
    }
    else {
        struct sockaddr_in6 sa1, sa2;
        inet_pton(AF_INET6, ip1, &(sa1.sin6_addr));
        inet_pton(AF_INET6, ip2, &(sa2.sin6_addr));
        return memcmp(sa1.sin6_addr.s6_addr, sa2.sin6_addr.s6_addr,
                      INET6_ADDRSTRLEN);
    }
}

void
get_ip_str(const struct in_addr ip, char *str, uint8_t addr_family)
{
    if ( addr_family == AF_INET ){
        inet_ntop(AF_INET, &(ip), str, INET_ADDRSTRLEN);
    }
    else if ( addr_family == AF_INET6 ) {
        inet_ntop(AF_INET6, &(ip), str, INET6_ADDRSTRLEN);
    }
    else {
        fprintf(stderr, "Unknown family address %u\n", addr_family);
    }
}

void 
get_ip_net(char *ip, uint32_t *net_ip, uint8_t addr_family)
{
    /* IPv4 */
    if ( addr_family == AF_INET ){
        // struct in_addr *in_addr_ip;
        inet_pton(AF_INET, ip, net_ip);
    } 
    /* IPv6 */
    else if ( addr_family == AF_INET6 ) {
        inet_pton(AF_INET6, ip, net_ip);
    }
    else {
        fprintf(stderr, "Unknown family address %u\n", addr_family);
    }
}
