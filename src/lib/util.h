/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef UTIL_H
#define UTIL_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

#define UNUSED(x) (void)x;

void out_of_memory(void);
void file_read_error(void);
void *xmalloc(size_t size);
void *xrealloc(void *v, int size);
uint32_t random_at_most(uint32_t max);
char* file_to_string(const char * file_name, size_t *size);
int nlz(uint32_t x);
int ntz(uint32_t x);
uint8_t get_ip_family(char *ip);
int ip_str_addr_compare(char *ip1, char *ip2, uint8_t addr_family);
void get_ip_str(const struct in_addr ip, char *str, uint8_t addr_family);
void get_ip_net(char *ip, uint32_t *net_ip, uint8_t addr_family);

static inline uint64_t
hton64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

static inline uint64_t
ntoh64(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
#endif
}

#endif
