#include "node.h"
#include <stdio.h>

/* Starts from 1, zero is used to indicate None */
static uint64_t current_uuid = 0; 

void 
node_init(struct node* n, uint16_t type)
{
    n->uuid = ++current_uuid;
    memset(n->name, 0x0, MAX_NODE_NAME);
    n->ports_num = 0;
    n->ports = NULL;
    n->type = type;
    n->flow_buff.tail = 0;
    buffer_state_init(&n->buffer_state);
}

void 
node_destroy_ports(struct node *n)
{
    /* Free ports */
    struct port *cur_port, *tmp;
    HASH_ITER(hh, n->ports, cur_port, tmp) {
        HASH_DEL(n->ports, cur_port);  
        free(cur_port->ipv4_addr);
        free(cur_port->ipv6_addr);
        free(cur_port);
    }
}

void 
node_add_port(struct node *n, uint32_t port_id, uint8_t eth_addr[ETH_LEN],
              uint32_t speed, uint32_t curr_speed)
{
    struct port *p = port_new(port_id, eth_addr, speed, curr_speed);
    /* TODO, allow to give a name to the interface in the python binding */
    char port_name[16];
    sprintf(port_name, "%s-eth%"PRIu32"", n->name, port_id);
    memcpy(p->name, port_name, strlen(port_name));    
    HASH_ADD(hh, n->ports, port_id, sizeof(uint32_t), p);
    n->ports_num++;
}

/* Retrieve a datapath port */
struct port* 
node_port(const struct node *n, uint32_t port)
{
    struct port *p;
    HASH_FIND(hh, n->ports, &port, sizeof(uint32_t), p);
    return p;
}

void 
node_add_tx_time(struct node *n, uint32_t out_port, struct netflow *flow)
{
    struct port *p = node_port(n, out_port);
    if (p != NULL) {
        netflow_update_send_time(flow, p->curr_speed);
    }
}

void 
node_update_port_stats(struct node *n, struct netflow *flow, uint32_t out_port)
{
    struct port *p = node_port(n, out_port);
    if (p != NULL) {
        uint8_t upnlive = (p->config & PORT_UP) && (p->state & PORT_LIVE);
        if (upnlive) {
            p->stats.tx_packets += flow->pkt_cnt;
            p->stats.tx_bytes += flow->byte_cnt; 
        }
    }
}

bool 
node_is_buffer_empty(struct node *n)
{
    return !n->flow_buff.tail;
}

bool 
node_flow_push(struct node *n, struct netflow *flow){
    if (!(n->flow_buff.tail - (BUFFER_MAX-1))){
        return 0;
    }
    n->flow_buff.tail++;
    n->flow_buff.flows[n->flow_buff.tail] = flow;
    return 1;
}

struct netflow*
node_flow_pop(struct node *n){
    struct netflow *f = n->flow_buff.flows[n->flow_buff.tail];
    n->flow_buff.tail--;
    return f;
}

void node_calculate_loss(struct node *n, struct netflow *nf, uint32_t out_port)
{
    struct port *p = node_port(n, out_port);
    if (p) {
        buffer_state_calculate_loss(&n->buffer_state, p->curr_speed);
    }
    UNUSED(nf);
}

void
node_update_port_capacity(struct node *n, int bits, uint32_t out_port)
{
    struct port *p = node_port(n, out_port);
    if (p) {
        buffer_state_update_capacity(&p->buffer_state, bits);
    }
}

int node_calculate_port_loss(struct node *n, struct netflow *nf,
                             uint32_t out_port)
{
    struct port *p = node_port(n, out_port);
    if (p) {
        return buffer_state_calculate_loss(&p->buffer_state, p->curr_speed);
    }
    return 0;
    UNUSED(nf);
}

void node_write_stats(const struct node *n, uint64_t time, FILE *fp)
{
    struct port *p, *tmp;
    uint64_t total_tx = 0; 
    uint64_t total_rx = 0;
    uint64_t t = time / 1000000;
    HASH_ITER(hh, n->ports, p, tmp) {
        uint64_t tx_rate, rx_rate;
        tx_rate = p->stats.tx_bytes - p->prev_stats.tx_bytes;
        rx_rate = p->stats.rx_bytes - p->prev_stats.rx_bytes;
        total_tx += tx_rate;
        total_rx += rx_rate;
        fprintf (fp, "%"PRIu64",%s,%"PRIu64",%"PRIu64"\n", t, p->name, tx_rate, rx_rate);
        p->prev_stats.tx_bytes = p->stats.tx_bytes;
        p->prev_stats.rx_bytes = p->stats.rx_bytes;
    }
}