AUTOMAKE_OPTIONS = foreign -Wno-syntax subdir-objects
ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I./src -I./third-party -Wstrict-prototypes  -Wall -Wextra -Werror     

bin_PROGRAMS = horse

horse_SOURCES = src/lib/flow.h \
		src/lib/flow.c \
		src/lib/heap.h \
		src/lib/heap.c \
		src/lib/util.h \
		src/lib/util.c \
		src/net/datapath.h \
		src/net/datapath.c \
		src/net/flow_table.h \ 
		src/net/flow_table.c \
		src/net/port.h \
		src/net/port.c \
		src/net/topology.h \
		src/net/topology.c \
		src/horse.c

noinst_LIBRARIES = libhorse.a
libhorse_a_SOURCES = src/lib/action.h \
		src/lib/action.c \
		src/lib/flow.h \
		src/lib/flow.c \
		src/lib/heap.h \
		src/lib/heap.c \
		src/lib/util.h \
		src/lib/util.c \
		src/net/datapath.h \
		src/net/datapath.c \
		src/net/flow_table.h \ 
		src/net/flow_table.c \
		src/net/port.h \
		src/net/port.c \
		src/net/topology.h \
		src/net/topology.c

TESTS = flow_table_test

# Unit tests

unit_test_CFLAGS = -DUNIT_TESTING=1 -I./src -I./third-party

noinst_PROGRAMS = flow_table_test

flow_table_test_SOURCES = tests/flow_table_test.c
flow_table_test_CFLAGS = $(unit_test_CFLAGS)
flow_table_test_LDADD = -lcmockery libhorse.a
