AUTOMAKE_OPTIONS = foreign -Wno-syntax subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wstrict-prototypes -Werror -Wall -Wextra

TESTS = simple_test

# Unit tests

unit_test_CFLAGS = -DUNIT_TESTING=1

noinst_PROGRAMS = simple_test

simple_test_SOURCES = tests/simple_test.c
simple_test_CFLAGS = $(unit_test_CFLAGS)
simple_test_LDADD = -lcmockery