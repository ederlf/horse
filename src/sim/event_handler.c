#include "event_handler.h"
#include "net/datapath.h"
#include "net/host.h"
#include <uthash/utlist.h>

static void 
next_flow_event(struct scheduler *sch, struct topology *topo,
                struct event_hdr **events,
                struct event_flow *cur_flow, uint32_t exit_port) {
    uint64_t dst_uuid;
    uint32_t dst_port, latency;

    // if (exit_port == CONTROLLER){
        
    //     scheduler_insert(sch, new_ev);  
    // }
    if (topology_next_hop(topo, cur_flow->node_id, exit_port, 
                           &dst_uuid, &dst_port, &latency)) {
        /* Set completion time and in_port*/
        cur_flow->flow.end_time += latency * 1000000000;
        cur_flow->flow.match.in_port = dst_port;
        /* Create new flow event */
        struct event_flow *new_flow = event_flow_new(cur_flow->flow.start_time                                          , dst_uuid);
        memcpy(&new_flow->flow, &cur_flow->flow, sizeof(struct netflow));
        HASH_ADD(hh, *events, id, sizeof(uint64_t),
                 (struct event_hdr *)new_flow);
        struct event *new_ev = event_new(new_flow->hdr.time, 
                                         new_flow->hdr.id);
        scheduler_insert(sch, new_ev);
    }
}

static void 
handle_netflow(struct scheduler *sch, struct topology *topo,
               struct event_hdr **events, struct event_hdr *ev) {
    struct out_port *out_ports = NULL;
    struct out_port *op;
    struct event_flow *ev_flow = (struct event_flow *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node) {
        printf("Node type %d\n", node->type);
        out_ports = node->recv_netflow(node, &ev_flow->flow);
        // if (node->type == DATAPATH) {
        //     struct datapath *dp = (struct datapath *)node;
            // out_ports = dp_recv_netflow(dp, &ev_flow->flow);
            /* Schedule next event using the output ports*/
        LL_FOREACH(out_ports, op) {
            printf("Handling node Id:%ld Port:%d ev_time:%ld global_t:%ld\n", ev_flow->node_id, op->port, ev->time, sch->clock);
            node->send_netflow(node, &ev_flow->flow, op->port);
            printf("Will create new event\n");
            next_flow_event(sch, topo, events, ev_flow, op->port);
            // }
        }
        // } else if (node->type == ROUTER) {
        // }
    }
    clean_out_ports(out_ports);
}

static void 
handle_instruction(struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev)
{
    struct event_instruction *ev_inst = (struct event_instruction *) ev;
    printf("%p\n", topo);
    printf("%d\n", ev_inst->hdr.type);
    UNUSED(sch);
    UNUSED(events);
}

static void 
handle_packet(struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev)
{
    printf("%p\n", topo);
    printf("%d\n", ev->type);
    UNUSED(sch);
    UNUSED(events);
}

static void 
handle_port(struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev)
{
    struct event_port *ev_port = (struct event_port *) ev;
    printf("%p\n", topo);
    printf("%d\n", ev_port->hdr.type);
    UNUSED(sch);
    UNUSED(events);
}

static void (*event_handler[EVENTS_NUM]) (struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev) = {
    [EVENT_FLOW] = handle_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_INSTRUCTION] = handle_instruction,
    [EVENT_PORT] = handle_port
};

void handle_event(struct scheduler *sch, struct topology *topo, struct event_hdr **events, struct event_hdr *ev)
{
    (*event_handler[ev->type]) (sch, topo, events, ev);
}