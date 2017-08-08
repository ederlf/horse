#include "event_handler.h"
#include "lib/of_pack.h"
#include "lib/openflow.h"
#include "net/datapath.h"
#include "net/dp_control.h"
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
            /* Schedule a packet in message */
            if (op->port == CONTROLLER){
                size_t len;
                /* node can only be a datapath */
                struct datapath *dp = (struct datapath*) node;
                ev_flow->flow.metadata.reason = OFPR_ACTION;
                uint8_t *data = of_packet_in(&ev_flow->flow, &len);
                /* Create an of out event */
                struct sim_event_of *ev_of;
                ev_of = sim_event_of_msg_out_new(ev->time, 
                                                 dp_id(dp), data, 
                                                 len);
                scheduler_insert(ev_hdl->sch, (struct sim_event*) ev_of);
            }
            else {
                next_flow_event(ev_hdl, ev_flow, op->port);
            }
        }
        netflow_clean_out_ports(&ev_flow->flow);
    }
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
handle_of_in(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    /* Finds the datapath of the event
    *  handle_control_msg(dp, msg); 
    */
    of_object_t *ret;
    struct sim_event_of *ev_of = (struct sim_event_of*) ev;
    struct datapath *dp = topology_datapath_by_dpid(ev_hdl->topo,
                                                    ev_of->dp_id);
    ret = dp_control_handle_control_msg(dp, ev_of->data, 
                                        ev_of->len, ev->time);
    /* In case there is a reply */
    if (ret != NULL){
        struct sim_event_of *ev_of;
        uint8_t *data;
        size_t len = ret->length;
        of_object_wire_buffer_steal(ret, &data);
        ev_of = sim_event_of_msg_out_new(ev->time, 
                                        dp_id(dp), data, 
                                        len);
        scheduler_insert(ev_hdl->sch, (struct sim_event*) ev_of); 
    }
    free(ev_of->data);
}

static void 
handle_of_out(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    struct of_manager *om = ev_hdl->om;
    struct sim_event_of *ev_of = (struct sim_event_of*) ev;
    printf("Will send message to controller\n");
    of_manager_send(om, ev_of->dp_id, ev_of->data, ev_of->len);
}

static void (*event_handler[EVENTS_NUM]) (struct ev_handler *ev_hdl, 
                                          struct sim_event *ev) = {
    [EVENT_FLOW] = handle_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_PORT] = handle_port,
    [EVENT_OF_MSG_IN] = handle_of_in,
    [EVENT_OF_MSG_OUT] = handle_of_out
};

void handle_event(struct ev_handler *ev_hdl,
                  struct sim_event *ev)
{
    (*event_handler[ev->type]) (ev_hdl, ev);
}