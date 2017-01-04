noinst_LIBRARIES += src/libhorse.a

src_libhorse_a_SOURCES = src/lib/action.h \
                src/lib/action.c \
                src/lib/flow.h \
                src/lib/flow.c \
                src/lib/heap.h \
                src/lib/heap.c \
                src/lib/instruction.h \
                src/lib/instruction.c \
                src/lib/json_topology.h \
                src/lib/json_topology.c \
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
                src/sim/event.h \
                src/sim/event.c \
                src/sim/scheduler.h \
                src/sim/scheduler.c \
                src/sim/sim.h \
                src/sim/sim.c

src_libhorse_a_LIBADD = vendor/json/json.o