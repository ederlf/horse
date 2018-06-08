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
// void crc32(const void *data, size_t n_bytes, uint32_t *crc);
int nlz(uint32_t x);
int ntz(uint32_t x);
uint8_t get_ip_family(char *ip);
int ip_str_addr_compare(char *ip1, char *ip2, uint8_t addr_family);
void get_ip_str(const struct in_addr ip, char *str, uint8_t addr_family);
void get_ip_net(char *ip, uint32_t *net_ip, uint8_t addr_family);

#define ROUND_UP(X, Y) (((X) + ((Y) - 1)) / (Y) * (Y))

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

/* Simple public domain implementation of the standard CRC32 checksum.*/
static inline uint32_t 
crc32_for_byte(uint32_t r) {
  int j;
  for(j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

static inline void 
crc32(const void *data, size_t n_bytes, uint32_t *crc) {
    static uint32_t table[0x100];
    size_t i;
    if(!*table) {
        for(i = 0; i < 0x100; ++i) {
            table[i] = crc32_for_byte(i);
        }
    }
    for(i = 0; i < n_bytes; ++i){
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
    }
}

#endif
