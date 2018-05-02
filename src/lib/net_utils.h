#ifndef NET_UTILS
#define NET_UTILS 1

void add_veth_pair(char *iface1, char *iface2);
void set_intf_name(char *rname, char* new_intf_name, char *old_intf_name);
void set_intf_up(char *rname, char *intf);
void set_intf_to_ns(char *rname, char* intf);
void set_intf_ip(char* rname, char* intf, char *ip, char* mask);
void set_intf_to_bridge(char *intf, char *bridge);
void delete_intf(char *iface);
void create_bridge(char *bridge);
void setup_veth(char *rname, char *intf1, char *intf2, char* bridge);
#endif