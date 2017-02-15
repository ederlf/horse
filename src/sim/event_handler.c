#include "event_handler.h"

static void 
handle_traffic(struct topology *topo, struct event_hdr *ev)
{
    struct event_flow *ev_flow = (struct event_flow *) ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node){
        if (node->type == DATAPATH){
            struct datapath *dp = (struct datapath*) node;
            struct out_port *op;
            op = dp_handle_flow(dp, &ev_flow->flow);
            /* Schedule next event using the next ports*/
            UNUSED(op);
        }
        else if (node->type == ROUTER){

        }  
    }
}

static void 
handle_instruction(struct topology *topo, struct event_hdr *ev)
{
    struct event_instruction *ev_inst = (struct event_instruction *) ev;
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev_inst->hdr.type);
}

static void 
handle_packet(struct topology *topo, struct event_hdr *ev)
{
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev->type);
}

static void 
handle_port(struct topology *topo, struct event_hdr *ev)
{
    struct event_port *ev_port = (struct event_port *) ev;
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev_port->hdr.type);
}

static void (*event_handler[EVENTS_NUM]) (struct topology *topo, struct event_hdr *ev) = {
    [EVENT_FLOW] = handle_traffic,
    [EVENT_PACKET] = handle_packet,
    [EVENT_INSTRUCTION] = handle_instruction,
    [EVENT_PORT] = handle_port
};

void handle_event(struct topology *topo, struct event_hdr *ev)
{
    (*event_handler[ev->type]) (topo, ev);
}