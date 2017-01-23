#include "event_handler.h"

static void 
handle_flow(struct topology *topo, struct event_flow *ev)
{
    /* Datapath to handle the flow */
    struct datapath *dp = topology_datapath(topo, ev->dpid);
    if (dp){
        // uint32_t *out_ports; 
        // out_ports = dp_handle_flow(dp, ev->pkt_cnt, ev->byte_cnt, ev->match);
          
    }
}

static void 
handle_instruction(struct topology *topo, struct event_instruction *ev)
{
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev->hdr.type);
}

static void 
handle_packet(struct topology *topo, struct event_hdr *ev)
{
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev->type);
}

static void 
handle_port(struct topology *topo, struct event_port *ev)
{
    printf("%d\n", topology_dps_num(topo));
    printf("%d\n", ev->hdr.type);
}

void handle_event(struct topology *topo, struct event_hdr *ev)
{
    switch (ev->type){
        case EVENT_FLOW: {
            handle_flow(topo, (struct event_flow *) ev);
            break;
        }
        case EVENT_PACKET: {
            handle_packet(topo, ev);
            break;
        }
        case EVENT_INSTRUCTION :{
            handle_instruction(topo, (struct event_instruction *) ev);
            break;
        }
        case EVENT_PORT: {
            handle_port(topo, (struct event_port *) ev);
            break;
        }
        default:{
            break;
        }
    }
}