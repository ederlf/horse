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

_Noreturn void out_of_memory(void)
{
    fprintf(stderr, "No memory available. Aborting.\n");
    abort();
}

_Noreturn void file_read_error(void)
{
    fprintf(stderr, "Failed to read the file. Aborting.\n");
    abort();
}

void *xmalloc(size_t size)
{
    void *v = malloc(size ? size : 1);
    if (v == NULL) {
        out_of_memory();
    }
    return v;
}

void *xrealloc(void *v, size_t size)
{
    void *result = realloc(v, size ? size : 1);
    if (result == NULL) {
        out_of_memory();
    }
    return result;
}

/* Counts the number of trailing zeros in a word.
 * Extracted from the book Hacker's Delight. */
static int pop(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x << 8);
    x = x + (x << 16);
    return (int)(x >> 24);
}

int nlz(uint32_t x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return pop(~x);
}

int ntz(unsigned x) { return pop(~x & (x - 1)); }

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
uint32_t random_at_most(uint32_t max)
{
    uint32_t num_bins = (uint32_t)max + 1;
    uint32_t num_rand = (uint32_t)RAND_MAX + 1;
    uint32_t bin_size = num_rand / num_bins;
    uint32_t defect = num_rand % num_bins;

    uint32_t x;
    do {
        x = rand();
    }
    // This is carefully written not to overflow
    while (num_rand - defect <= (uint32_t)x);

    // Truncated division is intentional
    return x / bin_size;
}

/* Return the size of the string for reuse */
char *file_to_string(const char *file_name, size_t *size)
{
    *size = 0;
    FILE *fh = fopen(file_name, "r");
    if (fh == NULL) {
        return NULL;
    }
    if (fseek(fh, 0, SEEK_END) != 0) {
        fclose(fh);
        file_read_error();
    }
    long length = ftell(fh);
    if (length < 0 || fseek(fh, 0, SEEK_SET) != 0) {
        fclose(fh);
        file_read_error();
    }
    if ((uintmax_t)length >= SIZE_MAX) {
        fclose(fh);
        file_read_error();
    }
    size_t count = (size_t)length;
    char *contents = xmalloc(count + 1);
    if (fread(contents, 1, count, fh) != count) {
        fclose(fh);
        free(contents);
        file_read_error();
    }
    fclose(fh);
    // count + 1 bytes were allocated after the SIZE_MAX overflow check.
    // NOLINTNEXTLINE(clang-analyzer-security.ArrayBound)
    contents[count] = '\0';
    *size = count;
    return contents;
}

uint8_t get_ip_family(char *ip)
{
    if (strchr(ip, '.')) {
        return AF_INET;
    } else if (strchr(ip, ':')) {
        return AF_INET6;
    } else {
        fprintf(stderr, "Address %s is from unknown family", ip);
        return AF_MAX;
    }
}

int ip_str_addr_compare(char *ip1, char *ip2, uint8_t addr_family)
{
    if (addr_family == AF_INET) {
        struct sockaddr_in sa1, sa2;
        inet_pton(AF_INET, ip1, &(sa1.sin_addr));
        inet_pton(AF_INET, ip2, &(sa2.sin_addr));
        return memcmp(&sa1.sin_addr.s_addr, &sa2.sin_addr.s_addr, sizeof(uint32_t));
    } else {
        struct sockaddr_in6 sa1, sa2;
        inet_pton(AF_INET6, ip1, &(sa1.sin6_addr));
        inet_pton(AF_INET6, ip2, &(sa2.sin6_addr));
        return memcmp(sa1.sin6_addr.s6_addr, sa2.sin6_addr.s6_addr,
                      sizeof(sa1.sin6_addr.s6_addr));
    }
}

void get_ip_str(void *addr, char *str, uint8_t addr_family)
{
    inet_ntop(addr_family, addr, str,
              addr_family == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
}

void get_ip_net(char *ip, void *net_ip, uint8_t addr_family)
{
    inet_pton(addr_family, ip, net_ip);
}
