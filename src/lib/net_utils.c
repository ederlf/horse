#include "net_utils.h"
#include <netemu/netns.h>
#include <stdlib.h>

void 
add_veth_pair(char *intf1, char* intf2)
{
    /* Create interfaces and add to namespace*/
    netns_run(NULL, "ip link add %s type veth "
            "peer name %s",
            intf1, intf2);
}

/* Set the interface name. To set up the name of an interface out of a namespace
   pass rname as NULL.
*/
void 
set_intf_name(char *rname, char* new_intf_name, char *old_intf_name)
{
    netns_run(rname, "ip link set %s name %s", old_intf_name, new_intf_name);
}

/* Set interface up */
void 
set_intf_up(char *rname, char *intf)
{
    netns_run(rname, "ip link set dev %s up", intf);  
}

/* Send an interface to the respective namespace rname. */
void 
set_intf_to_ns(char *rname, char* intf)
{
    /* Add to namespace */
    netns_run(NULL, "ip link set %s netns %s", intf, rname);
    /* Turn up */
    set_intf_up(rname, intf);
}

/* Set the interface IP. To set up the IP of an interface out of a namespace
   pass rname as NULL.
*/
void 
set_intf_ip(char* rname, char* intf, char *addr, char* mask)
{
    netns_run(rname, "ip addr add %s/%s dev %s", addr, mask, intf);
}

void 
set_intf_to_bridge(char *intf, char *bridge)
{
    /* Add to bridge */
    netns_run(NULL, "ip link set dev %s master %s", intf, bridge); 
}

void 
delete_intf(char *iface)
{
    netns_run(NULL, "ip link del %s", iface);
}


void 
create_bridge(char *bridge)
{
    netns_run(NULL, "ip link add name %s type bridge", bridge);
    netns_run(NULL, "ip link set dev %s up", bridge);
}

/* This function does all the steps necessaries to create a veth pair, send
   one of the sides to a namespace and connect the other side to a bridge */
void 
setup_veth(char *rname, char *intf1, char *intf2, char* bridge)
{
    /* Create pair */
    add_veth_pair(intf1, intf2);
    /* Send one of the sides to the namespace */
    set_intf_to_ns(rname, intf2);
    /* Set the interfaces up */ 
    set_intf_up(NULL, intf1);
    set_intf_up(rname, intf2);
    /* Add interface to bridge */
    set_intf_to_bridge(intf1, bridge);
}
