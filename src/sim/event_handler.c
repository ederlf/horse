#include "event_handler.h"
#include <uthash/utlist.h>

static void 
handle_traffic(struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev)
{
    struct event_flow *ev_flow = (struct event_flow *) ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node){
        if (node->type == DATAPATH){
            struct datapath *dp = (struct datapath*) node;
            struct out_port *out_ports;
            struct out_port *op;
            out_ports = dp_recv_netflow(dp, &ev_flow->flow);
            /* Schedule next event using the output ports*/

            LL_FOREACH(out_ports, op){
               uint64_t dst_uuid;
               uint32_t dst_port, latency;
               dp_send_netflow(dp, &ev_flow->flow, op->port);
               /* Get the next hop */ 
               topology_next_hop(topo, node->uuid, op->port, &dst_uuid, &dst_port, &latency);
               /* Set completion time and in_port*/ 
               ev_flow->flow.end_time += latency * 1000000000;
               ev_flow->flow.match.in_port = op->port;
               /* This is just here for a quick test */
               // struct event_flow *new_flow = malloc(sizeof(struct event_flow));
               // memcpy(new_flow, ev_flow, sizeof(struct event_flow));
               // new_flow->node_id = dst_uuid;
               // new_flow->hdr.id +=  10;
               // new_flow->hdr.time = ev_flow->flow.start_time;
               // HASH_ADD(hh, events, id, sizeof(uint64_t), 
               //   (struct event_hdr*) new_flow);
               // struct event* new_ev = malloc(sizeof(struct event));
               // init_event(new_ev, new_flow->hdr.time , new_flow->hdr.id); 
               // scheduler_insert(sch, new_ev);
               UNUSED(sch);
               UNUSED(events);
            }
            clean_out_ports(out_ports);
        }
        else if (node->type == ROUTER){

        }  
    }
}

static void 
handle_instruction(struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev)
{
    struct event_instruction *ev_inst = (struct event_instruction *) ev;
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev_inst->hdr.type);
    UNUSED(sch);
    UNUSED(events);
}

static void 
handle_packet(struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev)
{
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev->type);
    UNUSED(sch);
    UNUSED(events);
}

static void 
handle_port(struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev)
{
    struct event_port *ev_port = (struct event_port *) ev;
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev_port->hdr.type);
    UNUSED(sch);
    UNUSED(events);
}

static void (*event_handler[EVENTS_NUM]) (struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev) = {
    [EVENT_FLOW] = handle_traffic,
    [EVENT_PACKET] = handle_packet,
    [EVENT_INSTRUCTION] = handle_instruction,
    [EVENT_PORT] = handle_port
};

void handle_event(struct scheduler *sch, struct topology *topo, struct event_hdr *events, struct event_hdr *ev)
{
    (*event_handler[ev->type]) (sch, topo, events, ev);
}