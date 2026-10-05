#include "routing_msg/routing_msg.h"
#include "cmockery_horse.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

/* Native codecs allocate with libc; release their results with libc too. */
#undef malloc
#undef free

static uint32_t read_u32(const uint8_t *data)
{
    uint32_t value;
    memcpy(&value, data, sizeof(value));
    return ntohl(value);
}

static void shared_wire_fixtures(void **state)
{
    (void)state;
    FILE *fixtures = fopen(ROUTING_WIRE_FIXTURES, "r");
    if (fixtures == NULL) {
        fail();
        return;
    }
    char name[32], hex[129];
    size_t count = 0;
    while (fscanf(fixtures, "%31s %128s", name, hex) == 2) {
        size_t length = strlen(hex) / 2;
        /* malloc provides the alignment required by the existing C decoder. */
        uint8_t *wire = malloc(length);
        if (wire == NULL) {
            fclose(fixtures);
            fail();
            return;
        }
        for (size_t i = 0; i < length; ++i) {
            char byte_text[] = {hex[2 * i], hex[2 * i + 1], '\0'};
            char *end;
            unsigned long byte = strtoul(byte_text, &end, 16);
            assert_true(end == byte_text + 2);
            wire[i] = (uint8_t)byte;
        }
        struct routing_msg *message = NULL;
        if (strcmp(name, "announce") == 0) {
            /* Announce decoding is not implemented in C yet (plan item 03). */
            message = (struct routing_msg *)routing_msg_bgp_announce_new(0x0a000001,
                                                                         0x0a000002);
        } else {
            routing_msg_unpack(wire, &message);
            assert_int_equal(message->router_id, 0x0a000001);
            assert_int_equal(length, (message->size + 7u) / 8u * 8u);
        }
        if (strcmp(name, "state") == 0) {
            assert_int_equal(message->type, BGP_STATE);
            assert_int_equal(message->size, BGP_STATE_LEN);
            struct bgp_state *bgp = (struct bgp_state *)message;
            assert_int_equal(bgp->peer_rid, 0x0a000002);
            assert_int_equal(bgp->state, BGP_STATE_UP);
        } else if (strncmp(name, "fib_", 4) == 0) {
            assert_int_equal(message->type, ROUTER_FIB);
            assert_int_equal((message->size - HEADER_LEN) % 12, 0);
            size_t entries = (message->size - HEADER_LEN) / 12;
            const uint32_t prefixes[] = {0x0a010000, 0xc0000200, 0};
            const uint32_t masks[] = {0xffff0000, 0xffffff00, 0};
            assert_true(entries <= 3);
            for (size_t i = 0; i < entries && i < 3; ++i) {
                /* Match fib_add's interpretation of the logical payload. */
                const uint8_t *entry = wire + HEADER_LEN + i * 12;
                assert_int_equal(read_u32(entry), prefixes[i]);
                assert_int_equal(read_u32(entry + 4), masks[i]);
                assert_int_equal(read_u32(entry + 8), 0x0a000002 + i);
            }
        } else if (strcmp(name, "activity") == 0) {
            assert_int_equal(message->type, ROUTER_ACTIVITY);
            assert_int_equal(message->size, HEADER_LEN);
        }
        if (message->type != ROUTER_FIB) {
            uint8_t *packed = routing_msg_pack(message);
            assert_true(packed != NULL);
            assert_memory_equal(packed, wire, length);
            free(packed);
        }
        free(message);
        free(wire);
        ++count;
    }
    fclose(fixtures);
    assert_int_equal(count, 7);
}

int main(void)
{
    const UnitTest tests[] = {unit_test(shared_wire_fixtures)};
    return run_tests(tests);
}
