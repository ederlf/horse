#include "event_handler.h"
#include "lib/of_pack.h"
#include "lib/openflow.h"
#include "net/datapath.h"
#include "net/dp_control.h"
#include "net/host.h"
#include <uthash/utlist.h>

static void
handle_after_flow_recv(struct ev_handler *ev_hdl, struct node* node, 
                       struct netflow *nf, struct sim_event_flow_recv *ev){
    
    struct out_port *op, *tmp;
    struct scheduler *sch = ev_hdl->sch;
    // struct topology *topo = ev_hdl->topo;
    // struct topology *topo = ev_hdl->topo;
    UNUSED(ev);
    /* Update the sender and receiver. Flow is over */
    // struct node *src_node = topology_node(topo, ev->src_id);
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
            struct netflow *new = netflow_new_from_netflow(nf);
            printf("Scheduling to send %x %ld %d %ld\n", nf->match.eth_type, nf->start_time, op->port, sch->clock);
            send = sim_event_flow_send_new(nf->start_time, 
                                            node->uuid, op->port);
           
            new->out_ports = NULL;
            send->flow = new;
            // node_add_buffer_state_egress_flow(node, &new, op->port);
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
    if (node) {
        if (node->recv_netflow(node, &ev_flow->flow)) {
            handle_after_flow_recv(ev_hdl, node, ev_flow->flow, ev_flow);
        }
        /* No more actions, just remove the volume from sending side */
        
        netflow_destroy(ev_flow->flow);
    }
}


static void
handle_after_flow_send(struct ev_handler *ev_hdl, struct node *node, 
                       uint32_t out_port, struct netflow *nf){
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
        HASH_FIND(hh, host_execs((struct host*) node), &exec_id, sizeof(uint64_t), exec); 
        if ( exec && exec->exec_cnt ){
            printf("Scheduling next app exec %ld\n", nf->start_time );
            /* Schedule 1 microsecond after the time the next flow will start on the next hop
               Otherwise the app risks to start before sending the flow, which may cause a non
               existent congestion. 
            */
            exec->start_time = nf->start_time + 1;
            struct sim_event_app_start *ev = sim_event_app_start_new(nf->start_time, node->uuid, exec);
            scheduler_insert(sch, (struct sim_event*) ev);
        } 
    }

    if (topology_next_hop(topo, node->uuid, out_port, 
                           &dst_uuid, &dst_port, &latency)){
        // printf("%ld %d %d\n", dst_uuid, dst_port, out_port);
        struct sim_event_flow_recv *new_flow;   
        uint32_t in_port = nf->match.in_port;

        int rate = (nf->byte_cnt * 8) / 1000;
        // /* Calculate loss */
        // node_calculate_port_loss(node, nf, out_port);
        // // struct node *dst_node = topology_node(topo, dst_uuid);
        
        // node_update_port_capacity(node, -rate, out_port);
        // node_add_tx_time(node, out_port, nf);
        /* Creates new event */
        nf->start_time += latency;
        new_flow = sim_event_flow_recv_new(nf->start_time, dst_uuid, 
                                            node->uuid, rate, in_port);
        
        new_flow->flow = nf;
        new_flow->flow->match.in_port = dst_port;
        printf("Scheduling to receive %x %ld\n", nf->match.eth_type, nf->start_time);
        scheduler_insert(sch, (struct sim_event*) new_flow);
    }
    else {
        printf("Could not find next hop\n");
        netflow_destroy(nf);  
    }
}

static void 
handle_send_netflow(struct ev_handler *ev_hdl, struct sim_event *ev) {
    struct topology *topo = ev_hdl->topo;
    struct sim_event_flow_send *ev_flow = (struct sim_event_flow_send *)ev;
    /* Retrieve node to handle the flow */
    struct node *node = topology_node(topo, ev_flow->node_id);
    printf("Sending %ld %x %ld\n", ev->time, ev_flow->flow->match.eth_type, ev_flow->flow->start_time);
    if (node) {
        // handle_before_flow_send();
        int rate = (ev_flow->flow->byte_cnt * 8) / 1000;
        int loss = node_calculate_port_loss(node, ev_flow->flow, ev_flow->out_port);
        /* Calculate loss */
        if (loss){
           /* Remove capacity from buffer and drop whole flow for now */
           node_update_port_capacity(node, -rate, ev_flow->out_port);
           printf("Dropping %d\n", loss); 
        }
        // struct node *dst_node = topology_node(topo, dst_uuid);
        else {
            node_update_port_capacity(node, -rate, ev_flow->out_port);
            /* Calculate loss here */
            node->send_netflow(node, ev_flow->flow, ev_flow->out_port);
            // printf("Handle after\n");
            handle_after_flow_send(ev_hdl, node, ev_flow->out_port, ev_flow->flow);
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
handle_of_in(struct ev_handler *ev_hdl, struct sim_event *ev)
{
    /* Finds the datapath of the event
    *  handle_control_msg(dp, msg); 
    */
    of_object_t *ret;
    struct netflow *nf = netflow_new();
    struct scheduler *sch = ev_hdl->sch;
    struct out_port *op,  *tmp;
    struct sim_event_of *ev_of = (struct sim_event_of*) ev;
    struct datapath *dp = topology_datapath_by_dpid(ev_hdl->topo,
                                                    ev_of->dp_id);
    
    ret = dp_control_handle_control_msg(dp, ev_of->data, 
                                        nf, ev_of->len, ev->time);

    
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
        struct netflow *new = netflow_new_from_netflow(nf);
        printf("Scheduling new send from packet out %ld %x\n", nf->start_time, nf->match.eth_type );
        send = sim_event_flow_send_new(nf->start_time, 
                                       dp_uuid(dp), op->port);
        // printf("Next flow %d %ld\n", dst_port, dst_uuid);
        /* Need to copy because of broadcasting 
           Is there a way to avoid that?
        */
        node_add_tx_time((struct node*) dp, op->port, new);
        new->out_ports = NULL;
        send->flow = new;
        scheduler_insert(sch, (struct sim_event*) send);
        LL_DELETE(nf->out_ports, op);
        free(op);
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
    netflow_destroy(nf);
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
    struct netflow *nf;
    struct out_port *op,  *tmp;
    struct topology *topo = ev_hdl->topo;
    struct scheduler *sch =  ev_hdl->sch;
    struct sim_event_app_start *ev_app = (struct sim_event_app_start *)ev;
    /* Retrieve node to handle the flow */
    // printf("App start %ld %ld %x\n", ev->time, ev_app->node_id, *((uint32_t*) ev_app->exec->args));
    struct node *node = topology_node(topo, ev_app->node_id);
    if (node) {
        nf = host_execute_app((struct host*) node, ev_app->exec);
        /* Sends if application started with success */
        if (nf) {            
            LL_FOREACH_SAFE(nf->out_ports, op, tmp){
                struct sim_event_flow_send *send; 
                struct netflow *new = netflow_new_from_netflow(nf);
                node_add_tx_time(node, op->port, new);
                printf("Scheduling send from app start %ld %x\n", nf->start_time, nf->match.eth_type );
                send = sim_event_flow_send_new(nf->start_time, 
                                                node->uuid, op->port);
                /* Need to copy because of multicast 
                   Is there a way to avoid that?
                */
                new->out_ports = NULL;
                send->flow = new;
                scheduler_insert(sch, (struct sim_event*) send);
            }
        }
        netflow_destroy(nf);
    }
}

static void (*event_handler[EVENTS_NUM]) (struct ev_handler *ev_hdl, 
                                          struct sim_event *ev) = {
    [EVENT_FLOW_RECV] = handle_recv_netflow,
    [EVENT_FLOW_SEND] = handle_send_netflow,
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