#include "net/node.h"
#include "cmockery_horse.h"
#include <string.h>

static void name_round_trips(void **state)
{
    (void)state;
    struct node n = {0};
    const char *names[] = {"short", "", "1234567890123456789012345678901",
                           "\xc3\xa9\xe7\x8c\xab"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        /* Exact-sized allocations expose the old fixed-width overread to ASan. */
        size_t size = strlen(names[i]) + 1;
        char *input = malloc(size);
        memcpy(input, names[i], size);
        assert_true(node_set_name(&n, input));
        free(input);
        assert_string_equal(n.name, names[i]);
    }
    assert_true(node_set_name(&n, n.name));
}

static void invalid_names_preserve_previous(void **state)
{
    (void)state;
    struct node n = {0};
    assert_true(node_set_name(&n, "original"));
    const char *names[] = {NULL, "12345678901234567890123456789012",
                           "1234567890123456789012345678901234567890"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        assert_int_equal(node_set_name(&n, names[i]), 0);
        assert_string_equal(n.name, "original");
    }
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(name_round_trips),
        unit_test(invalid_names_preserve_previous),
    };
    return run_tests(tests);
}
