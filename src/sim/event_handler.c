#include "event_handler.h"
#include "net/datapath.h"
#include "net/host.h"
#include <uthash/utlist.h>

static void 
next_flow_event(struct scheduler *sch, struct topology *topo,
                struct event_flow *cur_flow, uint32_t exit_port) {
    uint64_t dst_uuid;
    uint32_t dst_port, latency;

    // if (exit_port == CONTROLLER){
        
    //     scheduler_insert(sch, new_ev);  
    // }
    if (topology_next_hop(topo, cur_flow->node_id, exit_port, 
                           &dst_uuid, &dst_port, &latency)) {
        /* Set completion time and in_port*/
        cur_flow->flow.end_time += latency;
        cur_flow->flow.match.in_port = dst_port;
        /* Create new flow event */
        struct event_flow *new_flow = event_flow_new(cur_flow->flow.start_time                                          , dst_uuid);
        memcpy(&new_flow->flow, &cur_flow->flow, sizeof(struct netflow));
        new_flow->flow.out_ports = NULL;
        
        scheduler_insert(sch, (struct event*) new_flow);
        printf("Will create new event dst:%ld size %ld exit_port %d\n", dst_uuid, sch->ev_queue->size, dst_port);
    }
}

static void 
next_ctrl_ev(struct scheduler *sch, struct event_flow *cur_flow)
{
    struct event_flow *new_flow = event_flow_new(cur_flow->flow.start_time                                          , cur_flow->node_id);
    new_flow->hdr.type = EVENT_CTRL;
    memcpy(&new_flow->flow, &cur_flow->flow, sizeof(struct netflow));
    scheduler_insert(sch, (struct event*) new_flow);
    printf("Will create new event to controller from:%ld size %ld\n", cur_flow->node_id, sch->ev_queue->size);
}

/**
 * create just a single function to handle flows.
 * Static methods for send and receive.
 * If there are out_ports just send
 * Else receive and send.
 */

static void 
handle_netflow(struct scheduler *sch, struct topology *topo, 
               struct event *ev) {
    struct out_port *op, *ports;
    struct event_flow *ev_flow = (struct event_flow *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node) {
        printf("Node type %s and ID:%ld InPORT %d IP %x\n", node->type? "HOST": "DATAPATH", ev_flow->node_id, ev_flow->flow.match.in_port, ev_flow->flow.match.ipv4_dst);
        // struct netflow *f = &ev_flow->flow;
        // printf("POOORT %d\n", f->match.in_port);
        node->handle_netflow(node, &ev_flow->flow);
        ports = ev_flow->flow.out_ports;
        /* May have or not ports to send the flow */
        LL_FOREACH(ports, op) {
            printf("PORT %x\n", op->port);
            if (op->port == CONTROLLER){
                next_ctrl_ev(sch, ev_flow);
            }
            else {
                next_flow_event(sch, topo, ev_flow, op->port);
            }
        }
        netflow_clean_out_ports(&ev_flow->flow);
    }
}

static void 
handle_instruction(struct scheduler *sch, struct topology *topo,
                     struct event *ev)
{
    struct event_instruction *ev_inst = (struct event_instruction *) ev;
    printf("%p\n", topo);
    printf("%d\n", ev_inst->hdr.type);
    UNUSED(sch);
}

static void 
handle_packet(struct scheduler *sch, struct topology *topo, 
              struct event *ev)
{
    printf("%p\n", topo);
    printf("%d\n", ev->type);
    UNUSED(sch);
}

static void 
handle_port(struct scheduler *sch, struct topology *topo, struct event *ev)
{
    struct event_port *ev_port = (struct event_port *) ev;
    printf("%p\n", topo);
    printf("%d\n", ev_port->hdr.type);
    UNUSED(sch);
}

static void (*event_handler[EVENTS_NUM]) (struct scheduler *sch, struct topology *topo, struct event *ev) = {
    [EVENT_FLOW] = handle_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_INSTRUCTION] = handle_instruction,
    [EVENT_PORT] = handle_port
};

void handle_event(struct scheduler *sch, struct topology *topo, 
                  struct event *ev)
{
    (*event_handler[ev->type]) (sch, topo, ev);
}