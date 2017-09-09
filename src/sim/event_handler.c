#include "event_handler.h"
#include "lib/of_pack.h"
#include "lib/openflow.h"
#include "net/datapath.h"
#include "net/dp_control.h"
#include "net/host.h"
#include <uthash/utlist.h>

static void
next_flow_event(struct ev_handler *ev_hdl, struct node* node, 
                struct netflow *nf){
    uint64_t dst_uuid;
    uint32_t dst_port, latency;
    struct out_port *op, *tmp;
    struct scheduler *sch = ev_hdl->sch;
    struct topology *topo = ev_hdl->topo;
    /* May have or not ports to send the flow */
    LL_FOREACH_SAFE(nf->out_ports, op, tmp) {
        /* Schedule a packet in message */
        if (op->port == OFPP_CONTROLLER) {
            size_t len;
            uint8_t *data = NULL;
            /* node can only be a datapath */
            struct datapath *dp = (struct datapath*) node;
            nf->metadata.reason = OFPR_ACTION;
            of_object_t *msg = pack_packet_in(nf, &len);
            of_object_wire_buffer_steal(msg, &data);
            /* Create an of out event */
            struct sim_event_of *ev_of;
            // printf("Packet in start time %ld\n", nf->start_time);
            ev_of = sim_event_of_msg_out_new(nf->start_time, 
                                             dp_id(dp), data, 
                                             len);
            scheduler_insert(ev_hdl->sch, (struct sim_event*) ev_of);
            of_object_delete(msg);
        }
        else if (topology_next_hop(topo, node->uuid, op->port, 
                           &dst_uuid, &dst_port, &latency)) {
            /* Set completion time and in_port*/
            nf->end_time += latency;
            nf->match.in_port = dst_port;
            /* Create new flow event */
            struct sim_event_flow *new_flow; 
            new_flow = sim_event_flow_new(nf->start_time, dst_uuid);
            memcpy(&new_flow->flow, nf, sizeof(struct netflow));
            new_flow->flow.out_ports = NULL;
            scheduler_insert(sch, (struct sim_event*) new_flow);
        }
        LL_DELETE(nf->out_ports, op);
        free(op);
    }
    // netflow_clean_out_ports(nf);
}

/**
 * create just a single function to handle flows.
 * Static methods for send and receive.
 * If there are out_ports just send
 * Else receive and send.
 */

static void 
handle_netflow(struct ev_handler *ev_hdl, struct sim_event *ev) {
    struct topology *topo = ev_hdl->topo;
    struct sim_event_flow *ev_flow = (struct sim_event_flow *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    if (node) {
        node->handle_netflow(node, &ev_flow->flow);
        next_flow_event(ev_hdl, node, &ev_flow->flow);
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
    struct netflow nf;
    netflow_init(&nf);
    struct sim_event_of *ev_of = (struct sim_event_of*) ev;
    struct datapath *dp = topology_datapath_by_dpid(ev_hdl->topo,
                                                    ev_of->dp_id);
    
    ret = dp_control_handle_control_msg(dp, ev_of->data, 
                                        &nf, ev_of->len, ev->time);
    if (nf.out_ports != NULL) {
        next_flow_event(ev_hdl, (struct node*) dp, &nf);
    }
    /* In case there is a reply */
    if (ret != NULL){
        struct sim_event_of *of_out;
        uint8_t *msg = NULL;
        size_t len = ret->length;
        of_object_wire_buffer_steal(ret, &msg);
        of_out = sim_event_of_msg_out_new(ev->time, 
                                        dp_id(dp), msg, 
                                        len);
        scheduler_insert(ev_hdl->sch, (struct sim_event*) of_out);
        of_object_delete(ret);
    }
    // ev_of->data = NULL;
}

static void 
handle_of_out(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    struct of_manager *om = ev_hdl->om;
    struct sim_event_of *ev_of = (struct sim_event_of*) ev;
    // printf("Will send message to controller\n");
    of_manager_send(om, ev_of->dp_id, ev_of->data, ev_of->len);
}

static void
handle_start_app(struct ev_handler *ev_hdl, struct sim_event *ev)
{   
    struct netflow nf;
    struct topology *topo = ev_hdl->topo;
    struct sim_event_app_start *ev_app = (struct sim_event_app_start *)ev;
    /* Retrieve node to handle the flow */
    // printf("App start %p %ld %x\n", ev, ev_app->node_id, *((uint32_t*) ev_app->exec->args));
    struct node *node = topology_node(topo, ev_app->node_id);
    if (node) {
        nf = host_execute_app((struct host*) node, ev_app->exec);
        next_flow_event(ev_hdl, node, &nf);
    }
}

static void (*event_handler[EVENTS_NUM]) (struct ev_handler *ev_hdl, 
                                          struct sim_event *ev) = {
    [EVENT_FLOW] = handle_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_PORT] = handle_port,
    [EVENT_OF_MSG_IN] = handle_of_in,
    [EVENT_OF_MSG_OUT] = handle_of_out,
    [EVENT_APP_START] = handle_start_app
};

void handle_event(struct ev_handler *ev_hdl,
                  struct sim_event *ev)
{
    (*event_handler[ev->type]) (ev_hdl, ev);
}