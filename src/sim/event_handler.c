#include "event_handler.h"
#include "net/datapath.h"
#include "net/host.h"
#include <uthash/utlist.h>

static void 
next_flow_event(struct ev_handler *ev_hdl, 
                struct sim_event_flow *cur_flow, uint32_t exit_port) {
    uint64_t dst_uuid;
    uint32_t dst_port, latency;

    struct scheduler *sch = ev_hdl->sch;
    struct topology *topo = ev_hdl->topo;
    // if (exit_port == CONTROLLER){
        
    //     scheduler_insert(sch, new_ev);  
    // }
    if (topology_next_hop(topo, cur_flow->node_id, exit_port, 
                           &dst_uuid, &dst_port, &latency)) {
        /* Set completion time and in_port*/
        cur_flow->flow.end_time += latency;
        cur_flow->flow.match.in_port = dst_port;
        /* Create new flow event */
        struct sim_event_flow *new_flow = sim_event_flow_new(cur_flow->flow.start_time                                          , dst_uuid);
        memcpy(&new_flow->flow, &cur_flow->flow, sizeof(struct netflow));
        new_flow->flow.out_ports = NULL;
        
        scheduler_insert(sch, (struct sim_event*) new_flow);
        // printf("Will create new event dst:%ld size %ld dst_port %d\n", dst_uuid, sch->ev_queue->size, dst_port);
    }
}

static void 
next_ctrl_ev(struct ev_handler *ev_hdl, struct sim_event_flow *cur_flow)
{
    struct scheduler *sch = ev_hdl->sch;
    struct sim_event_flow *new_flow = sim_event_flow_new(cur_flow->flow.start_time                                          , cur_flow->node_id);
    new_flow->hdr.type = EVENT_PACKET_IN;
    memcpy(&new_flow->flow, &cur_flow->flow, sizeof(struct netflow));
    scheduler_insert(sch, (struct sim_event*) new_flow);
    // printf("Will create new event to controller from:%ld size %ld\n", cur_flow->node_id, sch->ev_queue->size);
}

/**
 * create just a single function to handle flows.
 * Static methods for send and receive.
 * If there are out_ports just send
 * Else receive and send.
 */

static void 
handle_netflow(struct ev_handler *ev_hdl, struct sim_event *ev) {
    struct out_port *op, *ports;
    struct topology *topo = ev_hdl->topo;
    struct sim_event_flow *ev_flow = (struct sim_event_flow *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node) {
        // printf("Node type %s and ID:%ld InPORT %d IP %x time %ld\n", node->type? "HOST": "DATAPATH", ev_flow->node_id, ev_flow->flow.match.in_port, ev_flow->flow.match.ipv4_dst, ev_flow->flow.start_time);
        // struct netflow *f = &ev_flow->flow;
        // printf("POOORT %d\n", f->match.in_port);
        node->handle_netflow(node, &ev_flow->flow);
        ports = ev_flow->flow.out_ports;
        /* May have or not ports to send the flow */
        LL_FOREACH(ports, op) {
            if (op->port == CONTROLLER){
                next_ctrl_ev(ev_hdl, ev_flow);
            }
            else {
                next_flow_event(ev_hdl, ev_flow, op->port);
            }
        }
        netflow_clean_out_ports(&ev_flow->flow);
    }
}

static void 
handle_instruction(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    UNUSED(ev_hdl);
    UNUSED(ev);
}

static void 
handle_packet(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    UNUSED(ev_hdl);
    UNUSED(ev);
}

static void 
handle_port(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    UNUSED(ev_hdl);
    UNUSED(ev);
}

static void 
handle_packet_in(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    // struct of_manager *ofm = ev_hdl
    struct sim_event_pkt_in *sim_pkt_in = (struct sim_event_pkt_in*) ev;

}

static void (*event_handler[EVENTS_NUM]) (struct ev_handler *ev_hdl, 
                                          struct sim_event *ev) = {
    [EVENT_FLOW] = handle_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_INSTRUCTION] = handle_instruction,
    [EVENT_PORT] = handle_port,
    [EVENT_PACKET_IN] = handle_packet_in

};

void handle_event(struct ev_handler *ev_hdl,
                  struct sim_event *ev)
{
    (*event_handler[ev->type]) (ev_hdl, ev);
}