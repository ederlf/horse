AUTOMAKE_OPTIONS = foreign -Wno-syntax subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I./src -I./vendor -L./vendor/loci -Wstrict-prototypes -Wall -D_GNU_SOURCE -Wextra -Werror  -fPIC -pthread -levent -lcfluid_base -loci 

noinst_LIBRARIES =
noinst_LTLIBRARIES =

include vendor/json/automake.mk
include vendor/log/automake.mk
include vendor/netemu/automake.mk
include vendor/patricia/automake.mk
include src/automake.mk

bin_PROGRAMS = horse
horse_SOURCES = src/horse.c
horse_LDADD = .libs/libhorse.la -lcfluid_base -loci -levent


# Unit tests
TESTS = flow_table_test scheduler_test instruction_test netflow_test
unit_test_CFLAGS = -DUNIT_TESTING=1 -I./src -I./vendor

noinst_PROGRAMS = horse_daemon flow_table_test scheduler_test instruction_test netflow_test

horse_daemon_SOURCES = src/horse_daemon.c
horse_daemon_CFLAGS = $(AM_CFLAGS) -lpcap
horse_daemon_LDADD = .libs/libroutingmsg.la

flow_table_test_SOURCES = tests/flow_table_test.c
flow_table_test_CFLAGS = $(unit_test_CFLAGS)
flow_table_test_LDADD = -lcmockery .libs/libhorse.a

scheduler_test_SOURCES = tests/scheduler_test.c
scheduler_test_CFLAGS = $(unit_test_CFLAGS)
scheduler_test_LDADD = -lcmockery .libs/libhorse.a

instruction_test_SOURCES = tests/instruction_test.c
instruction_test_CFLAGS = $(unit_test_CFLAGS)
instruction_test_LDADD = -lcmockery .libs/libhorse.a

netflow_test_SOURCES = tests/netflow_test.c
netflow_test_CFLAGS = $(unit_test_CFLAGS)
netflow_test_LDADD = -lcmockery .libs/libhorse.a
