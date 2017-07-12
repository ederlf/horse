cimport horse
from libc.stdint cimport uint64_t

# class Intf(object):
#     # IPv4 and IPv6 
#     def __init__(self, port_id, eth_addr, ipv4_addr = None, ipv6_addr = None, max_speed = 1000000, cur_speed = 1000000):
#         self.port_id = port_id
#         self.eth_addr = eth_addr
#         self.max_speed = max_speed
#         self.cur_speed = cur_speed
#         self.ipv4_addr = None
#         self.ipv4_mask = None
#         self.ipv6_addr = None
#         self.ipv6_mask = None
#         if ipv4_addr != None:
#             self.set_ipv4(ipv4_addr)
#         if ipv6_addr != None:
#             self.set_ipv6(ipv6_addr)

#     def set_ipv4(self, ipv4_addr):
#         # Convert string to number
#         ip_mask = ipv4_addr.split('/')
#         ip_parts = ip_mask[0].split('.')
#         self.ipv4_addr = (int(ip_parts[0]) << 24) + (int(ip_parts[1]) << 16) + (int(ip_parts[2]) << 8) + int(ip_parts[3])
#         if len(ip_mask) == 2:
#             cidr = int(ip_mask[1])
#             self.ipv4_mask = ((2**cidr) - 1) << (32 - cidr)

#     def set_ipv6(self, ipv6_addr):
#         # Convert string to hex number        
#         pass

cdef class SDNSwitch:
    cdef datapath* _dp_ptr

    # Default ip to connect to a controller is the localhost
    # Default port is the IANA number allocated for OpenFlow  
    def __cinit__(self, uint64_t dp_id, ctrl_ip = "127.0.0.1", 
                  ctrl_port = 6653):
        self._dp_ptr = dp_new(dp_id, ctrl_ip, ctrl_port)

    # def add_port(self, intf):
    #     mac = intf.eth_addr.replace(':', '').decode('hex')
    #     cdef uint8_t *c_eth_addr = mac
    #     dp_add_port(self._dp_ptr, intf.port_id, c_eth_addr, intf.max_speed, intf.cur_speed) 

    def add_port(self, port, eth_addr, max_speed = 1000000, cur_speed = 1000000):
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        dp_add_port(self._dp_ptr, port, c_eth_addr, max_speed, cur_speed)      
    @property
    def dp_id(self):
        return dp_id(self._dp_ptr) 

    @property
    def uuid(self):
        return dp_uuid(self._dp_ptr)     


cdef class Host:
    cdef host* _host_ptr

    def __cinit__(self):
        self._host_ptr = host_new()
        # Add ping to the  
        host_add_app(self._host_ptr, 1)

    def add_port(self, port, eth_addr, ip = None, netmask = None, max_speed = 1000000, cur_speed = 1000000):
        # TODO: Add mac converstion to utils...
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        host_add_port(self._host_ptr, port, c_eth_addr, max_speed, cur_speed)
        if ip != None and netmask != None:
            ip_parts = ip.split('.')
            int_ip = (int(ip_parts[0]) << 24) + (int(ip_parts[1]) << 16) + (int(ip_parts[2]) << 8) + int(ip_parts[3])
            nm_parts = netmask.split('.')
            int_nm = (int(nm_parts[0]) << 24) + (int(nm_parts[1]) << 16) + (int(nm_parts[2]) << 8) + int(nm_parts[3])
            host_set_intf_ipv4(self._host_ptr, port, int_ip, int_nm)

    def ping(self, dst, start_time = 0):
        cdef int ip
        ip_parts = dst.split('.')
        ip = (int(ip_parts[0]) << 24) + (int(ip_parts[1]) << 16) + (int(ip_parts[2]) << 8) + int(ip_parts[3])
        host_start_app(self._host_ptr, 1, start_time, <void*> &ip)

    @property
    def uuid(self):
        return host_uuid(self._host_ptr)

cdef class Topology:
    cdef topology *_topo_ptr
    cdef object dps

    def __cinit__(self):
        self._topo_ptr = topology_new()
        self.dps = []

    # def __dealloc__(self):
    #     if self._topo_ptr != NULL:
    #         topology_destroy(self._topo_ptr)

    def start(self):
        start(self._topo_ptr)

    def add_node(self, Node):
        if isinstance(Node, SDNSwitch):
            dp = <SDNSwitch> Node
            topology_add_datapath(self._topo_ptr, dp._dp_ptr)
        elif isinstance (Node, Host):
            h = <Host> Node
            topology_add_host(self._topo_ptr, h._host_ptr)

    def add_link(self, node1, node2, port1, port2, latency = 1):
        topology_add_link(self._topo_ptr, node1.uuid, node2.uuid, port1, port2, 10, latency, False)

    @property
    def dp(self):
        return "We don't have: %s" % self.dps

    @property
    def dps_num(self):
        return topology_dps_num(self._topo_ptr)

    @property
    def links_num(self):
        return topology_links_num(self._topo_ptr)
