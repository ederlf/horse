AUTOMAKE_OPTIONS = foreign -Wno-syntax subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I./src -I./vendor -Wstrict-prototypes -Wall -Wextra -Werror     

noinst_LIBRARIES =

include vendor/json/automake.mk
include src/automake.mk

bin_PROGRAMS = horse
horse_SOURCES = src/horse.c
horse_LDADD = src/libhorse.a


# Unit tests
TESTS = flow_table_test scheduler_test instruction_test
unit_test_CFLAGS = -DUNIT_TESTING=1 -I./src -I./vendor

noinst_PROGRAMS = flow_table_test scheduler_test instruction_test

flow_table_test_SOURCES = tests/flow_table_test.c
flow_table_test_CFLAGS = $(unit_test_CFLAGS)
flow_table_test_LDADD = -lcmockery src/libhorse.a

scheduler_test_SOURCES = tests/scheduler_test.c
scheduler_test_CFLAGS = $(unit_test_CFLAGS)
scheduler_test_LDADD = -lcmockery src/libhorse.a

instruction_test_SOURCES = tests/instruction_test.c
instruction_test_CFLAGS = $(unit_test_CFLAGS)
instruction_test_LDADD = -lcmockery src/libhorse.a

