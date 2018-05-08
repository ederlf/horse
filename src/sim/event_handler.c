#include "event_handler.h"
#include "lib/of_pack.h"
#include "lib/openflow.h"
#include "net/datapath.h"
#include "net/dp_control.h"
#include "net/host.h"
#include <log/log.h>
#include <uthash/utlist.h>


/* Static functions */


static void handle_after_flow_recv(struct ev_handler *ev_hdl, struct node* node, 
                       struct netflow *nf, uint64_t flow_id);

static void handle_recv_netflow(struct ev_handler *ev_hdl, struct sim_event *ev);

static void 
handle_send_netflow(struct ev_handler *ev_hdl, struct sim_event *ev); 

static void
handle_after_flow_send(struct ev_handler *ev_hdl, struct node *node, 
                       uint32_t out_port, struct netflow *nf, uint64_t flow_id);

static void 
handle_packet(struct ev_handler *ev_hdl, struct sim_event *ev);

static void 
handle_port(struct ev_handler *ev_hdl, struct sim_event *ev);

static void
handle_of_in(struct ev_handler *ev_hdl, struct sim_event_fti *ev);

static void
handle_of_out(struct ev_handler *ev_hdl, struct sim_event_fti *ev);

static void
handle_start_app(struct ev_handler *ev_hdl, struct sim_event *ev);

static void handle_fti(struct ev_handler *ev_hdl, struct sim_event *ev);

static void
add_live_flow(struct ev_handler *evh, struct live_flow *lf);

static void
del_live_flow(struct ev_handler *evh, struct live_flow *lf);

static struct live_flow*
find_live_flow(struct ev_handler *evh, uint64_t flow_id);

/* Handler for the events that cause FTI */
static void (*event_handler_fti[2]) (struct ev_handler *ev_hdl,
                                    struct sim_event_fti *ev) = {
    [EVENT_OF_MSG_IN] = handle_of_in,
    [EVENT_OF_MSG_OUT] = handle_of_out
};

/* General handler for events */
static void (*event_handler[EVENTS_NUM]) (struct ev_handler *ev_hdl, 
                                          struct sim_event *ev) = {
    [EVENT_FLOW_RECV] = handle_recv_netflow,
    [EVENT_FLOW_SEND] = handle_send_netflow,
    [EVENT_PACKET] = handle_packet,
    [EVENT_PORT] = handle_port,
    [EVENT_FTI] = handle_fti,
    [EVENT_APP_START] = handle_start_app
};
/* Implementation */

static void
handle_after_flow_recv(struct ev_handler *ev_hdl, struct node* node, 
                       struct netflow *nf, uint64_t flow_id){
    
    struct out_port *op, *tmp;
    struct scheduler *sch = ev_hdl->sch;

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
            struct sim_event_fti *ev_of;
            ev_of = sim_event_of_msg_out_new(nf->start_time,
                                             dp_id(dp), data, len);
            scheduler_insert(ev_hdl->sch, (struct sim_event*) ev_of);
            of_object_delete(msg);
        }
        else if (op->port == OFPP_FLOOD){
            /* Convert a flood to more ports */
            struct datapath *dp = (struct datapath*) node;
            LL_DELETE(nf->out_ports, op);
            free(op);
            dp_create_flood(dp, nf);
            /* Get back to the loop and process all the ports added*/
            op = tmp = nf->out_ports;
            continue;
        }
        else {
            /* Create new send event */
            struct sim_event_flow_send *send; 
            // struct node *src_node = topology_node(topo, ev->src_id);
            /* New send flow event */
            /* Need to copy because of multicast 
               Is there a way to avoid that?
            */
            
            // Update sender and receiver buffers
            int rate = (nf->byte_cnt * 8) / 1000;
            node_update_port_capacity(node, rate, op->port);
            /* Calculate the time to transmit. The flow send event will happen then */
            node_add_tx_time(node, op->port, nf);
            log_debug("Scheduling to send %x %ld %d %ld\n", nf->match.eth_type,
                      nf->start_time, op->port, sch->clock);
            send = sim_event_flow_send_new(nf->start_time, node->uuid,
                                           op->port);
            send->flow = nf;
            send->flow_id = flow_id;
            scheduler_insert(sch, (struct sim_event*) send);
        }
        LL_DELETE(nf->out_ports, op);
        free(op);
    }
}

static void 
handle_recv_netflow(struct ev_handler *ev_hdl, struct sim_event *ev) {
    struct topology *topo = ev_hdl->topo;
    struct sim_event_flow_recv *ev_flow = (struct sim_event_flow_recv *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    struct live_flow *lf = find_live_flow(ev_hdl, ev_flow->flow_id);
    if (node != NULL && lf != NULL) {
        struct netflow *nf = node->recv_netflow(node, lf->flow);
        if (nf) {
            log_debug("NF eth_type %x %p", nf->match.eth_type, nf);
            handle_after_flow_recv(ev_hdl, node, nf, ev_flow->flow_id);
            lf->flow = nf;
        }
        else {
            /* No more actions, just remove the volume from sending side */
            del_live_flow(ev_hdl, lf);
        }
    }
}


static void
handle_after_flow_send(struct ev_handler *ev_hdl, struct node *node, 
                       uint32_t out_port, struct netflow *nf, uint64_t flow_id){
    uint64_t dst_uuid;
    uint32_t dst_port, latency;
    struct scheduler *sch = ev_hdl->sch;
    struct topology *topo = ev_hdl->topo;
    /* May have or not ports to send the flow */
    /* Set completion time and in_port*/
    uint64_t exec_id = nf->exec_id;
    struct exec *exec;
    /* Does the host has more to send? */
    if (node->type == HOST) {
        HASH_FIND(hh, host_execs((struct host*) node), &exec_id,
                  sizeof(uint64_t), exec); 
        if ( exec && exec->exec_cnt ){
            /* Schedule 1 microsecond after the time the next flow will 
               start on the next hop .Otherwise the app risks to start before 
               sending the flow, which may cause a non existent congestion. 
            */
            exec->start_time = nf->start_time + 1000000;
            log_debug("Scheduling next app exec %ld\n", nf->start_time );
            struct sim_event_app_start *ev = 
                    sim_event_app_start_new(exec->start_time, node->uuid, exec);
            scheduler_insert(sch, (struct sim_event*) ev);
        } 
    }

    if (topology_next_hop(topo, node->uuid, out_port, 
                           &dst_uuid, &dst_port, &latency)){
        struct sim_event_flow_recv *new_flow;   
        int rate = (nf->byte_cnt * 8) / 1000;
        // /* Calculate loss */
        // node_calculate_port_loss(node, nf, out_port);
        // // struct node *dst_node = topology_node(topo, dst_uuid);
        
        // node_update_port_capacity(node, -rate, out_port);
        // node_add_tx_time(node, out_port, nf);
        /* Creates new event */
        nf->start_time += latency;
        nf->match.in_port = dst_port;
        new_flow = sim_event_flow_recv_new(nf->start_time, dst_uuid, rate);
        new_flow->flow = nf;
        new_flow->flow_id = flow_id;
        log_debug("Scheduling to receive %x %ld %d\n", nf->match.eth_type,
                  nf->start_time, dst_port);
        scheduler_insert(sch, (struct sim_event*) new_flow);
    }
    else {
        netflow_destroy(nf);
    }
}

static void 
handle_send_netflow(struct ev_handler *ev_hdl, struct sim_event *ev) {
    struct topology *topo = ev_hdl->topo;
    struct sim_event_flow_send *ev_flow = (struct sim_event_flow_send *)ev;
    struct node *node = topology_node(topo, ev_flow->node_id);
    struct live_flow *lf =  find_live_flow(ev_hdl, ev_flow->flow_id);
    if (node != NULL && lf != NULL) {
        struct netflow *nf = lf->flow;
        // handle_before_flow_send();
        int rate = (nf->byte_cnt * 8) / 1000;
        int loss = node_calculate_port_loss(node, nf, ev_flow->out_port);
        /* Calculate loss */
        if (loss){
           /* Remove capacity from buffer and drop whole flow for now */
           node_update_port_capacity(node, -rate, ev_flow->out_port);
           /*TODO: Calculate loss */
        }
        // struct node *dst_node = topology_node(topo, dst_uuid);
        else {
            node_update_port_capacity(node, -rate, ev_flow->out_port);
            /* Calculate loss here */
            node->send_netflow(node, nf, ev_flow->out_port);
            handle_after_flow_send(ev_hdl, node, ev_flow->out_port, nf,
                                   ev_flow->flow_id);
        }
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
handle_fti(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    struct sim_event_fti *ev_fti = (struct sim_event_fti*) ev;
    (*event_handler_fti[ev_fti->subtype]) (ev_hdl, ev_fti);
}

static void
handle_of_in(struct ev_handler *ev_hdl, struct sim_event_fti *ev)
{
    /* Finds the datapath of the event
    *  handle_control_msg(dp, msg); 
    */
    of_object_t *ret;
    struct netflow *nf = netflow_new();
    struct scheduler *sch = ev_hdl->sch;
    struct out_port *op,  *tmp;
    struct fti_event_of *ev_of = (struct fti_event_of *) ev;
    struct datapath *dp = topology_datapath_by_dpid(ev_hdl->topo,
                                                    ev_of->dp_id);
    
    ret = dp_control_handle_control_msg(dp, ev_of->base.data, 
                                        nf, ev_of->base.len, ev->hdr.time);

    
    /* A packet out may have added ports to send the flow */
    LL_FOREACH_SAFE(nf->out_ports, op, tmp){
        /* Use the datapath directly, since it is the only possible here*/
        if (op->port == OFPP_FLOOD){
             /* Convert a flood to more ports */
            LL_DELETE(nf->out_ports, op);
            free(op);
            dp_create_flood(dp, nf);
            /* Get back to the loop and process all the ports added*/
            op = tmp = nf->out_ports;
            continue;
        }
        struct sim_event_flow_send *send; 
        struct live_flow *lf;
        struct netflow *new = netflow_new_from_netflow(nf);
        
        lf = live_flow_new(dp_uuid(dp), new);
        add_live_flow(ev_hdl, lf);

        log_debug("Scheduling new send from packet out %ld %x %d\n",
                  nf->start_time, nf->match.eth_type, op->port);
        send = sim_event_flow_send_new(nf->start_time, 
                                       dp_uuid(dp), op->port);
        /* Need to copy because of broadcasting 
           Is there a way to avoid that?
        */
        node_add_tx_time((struct node*) dp, op->port, new);
        new->out_ports = NULL;
        send->flow = new;
        send->flow_id = lf->flow_id;
        scheduler_insert(sch, (struct sim_event*) send);
        LL_DELETE(nf->out_ports, op);
        free(op);
    }

    /* In case there is a reply */
    if (ret != NULL){
        struct sim_event_fti *of_out;
        uint8_t *msg = NULL;
        size_t len = ret->length;
        of_object_wire_buffer_steal(ret, &msg);
        of_out = sim_event_of_msg_out_new(ev->hdr.time, 
                                        dp_id(dp), msg, 
                                        len);
        scheduler_insert(ev_hdl->sch, (struct sim_event*) of_out);
        of_object_delete(ret);
    }
    netflow_destroy(nf);
    // ev_of->data = NULL;
}

static void 
handle_of_out(struct ev_handler *ev_hdl, struct sim_event_fti *ev)
{
    struct conn_manager *cm = ev_hdl->cm;
    struct fti_event_of *ev_of = (struct fti_event_of *) ev;
    conn_manager_send_of(cm, ev_of->dp_id, ev_of->base.data, ev_of->base.len);
}

static void
handle_start_app(struct ev_handler *ev_hdl, struct sim_event *ev)
{   
    struct netflow *nf;
    struct out_port *op,  *tmp;
    struct topology *topo = ev_hdl->topo;
    struct scheduler *sch =  ev_hdl->sch;
    struct sim_event_app_start *ev_app = (struct sim_event_app_start *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_app->node_id);
    if (node) {
        nf = host_execute_app((struct host*) node, ev_app->exec);
        /* Sends if application started with success */
        if (nf) {            
            LL_FOREACH_SAFE(nf->out_ports, op, tmp){
                struct sim_event_flow_send *send; 
                struct live_flow *lf;
                struct netflow *new = netflow_new_from_netflow(nf);
                
                lf = live_flow_new(node->uuid, new);
                add_live_flow(ev_hdl, lf);

                node_add_tx_time(node, op->port, new);
                log_debug("Scheduling send from app start %ld %x\n",
                          nf->start_time, nf->match.eth_type );
                send = sim_event_flow_send_new(new->start_time, 
                                               node->uuid, op->port);
                /* Need to copy because of multicast 
                */
                new->out_ports = NULL;
                send->flow = new;
                send->flow_id = lf->flow_id;
                scheduler_insert(sch, (struct sim_event*) send);
            }
        }
        netflow_destroy(nf);
    }
}


static void
add_live_flow(struct ev_handler *evh, struct live_flow *lf)
{
    HASH_ADD(hh, evh->live_flows, flow_id, sizeof(uint64_t), lf);
}

static void
del_live_flow(struct ev_handler *evh, struct live_flow *lf)
{
    HASH_DEL(evh->live_flows, lf);
    live_flow_destroy(lf);
}

static struct live_flow*
find_live_flow(struct ev_handler *evh, uint64_t flow_id)
{
    struct live_flow *f = NULL;
    HASH_FIND(hh, evh->live_flows, &flow_id, sizeof(uint64_t), f);
    return f;
}

void handle_event(struct ev_handler *ev_hdl,
                  struct sim_event *ev)
{
    (*event_handler[ev->type]) (ev_hdl, ev);
}


