#noinst_LIBRARIES += libhorse.a

lib_LTLIBRARIES = libhorse.la
libhorse_la_SOURCES = src/lib/action.h \
                src/lib/action.c \
                src/lib/action_list.h \
                src/lib/action_list.c \
                src/lib/action_set.h \
                src/lib/action_set.c \
                src/lib/event.h \
                src/lib/event.c \
                src/lib/flow.h \
                src/lib/flow.c \
                src/lib/heap.h \
                src/lib/heap.c \
                src/lib/instruction.h \
                src/lib/instruction.c \
                src/lib/instruction_set.h \
                src/lib/instruction_set.c \
                src/lib/json_topology.h \
                src/lib/json_topology.c \
		src/lib/netflow.h \
		src/lib/netflow.c \
                src/lib/packets.h \
                src/lib/openflow.h \
                src/lib/timer.h \
                src/lib/timer.c \
                src/lib/util.h \
                src/lib/util.c \
                src/net/app/app.h \
                src/net/app/app.c \
                src/net/app/ping.h \
                src/net/app/ping.c \
                src/net/arp_table.h \
                src/net/arp_table.c \
                src/net/datapath.h \
                src/net/datapath.c \
                src/net/dp_actions.h \
                src/net/dp_actions.c \
                src/net/flow_table.h \ 
                src/net/flow_table.c \
                src/net/host.h \
                src/net/host.c \
                src/net/legacy_node.h \ 
                src/net/legacy_node.c \
                src/net/node.h \
                src/net/node.c \
                src/net/port.h \
                src/net/port.c \
                src/net/route_table.h \
                src/net/route_table.c \
                src/net/topology.h \
                src/net/topology.c \
                src/sim/event_handler.h \
                src/sim/event_handler.c \
                src/sim/scheduler.h \
                src/sim/scheduler.c \
                src/sim/sim.h \
                src/sim/sim.c

libhorse_la_LIBADD = libjson.la libpatricia.la
