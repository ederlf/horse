AUTOMAKE_OPTIONS = foreign -Wno-syntax subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I./src -I./third-party -Wstrict-prototypes  -Wall -Wextra -Werror     

bin_PROGRAMS = horse

horse_SOURCES = src/lib/flow.h \
		src/lib/flow.c \
		src/net/datapath.h \
		src/net/datapath.c \
		src/net/flow_table.h \ 
		src/net/flow_table.c \
		src/net/port.h \
		src/net/port.c \
		src/net/topology.h \
		src/net/topology.c \
		src/horse.c

TESTS = simple_test

# Unit tests

unit_test_CFLAGS = -DUNIT_TESTING=1

noinst_PROGRAMS = simple_test

simple_test_SOURCES = tests/simple_test.c
simple_test_CFLAGS = $(unit_test_CFLAGS)
simple_test_LDADD = -lcmockery
