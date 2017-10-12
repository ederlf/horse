cimport horse
from libc.stdint cimport uint64_t
from libc.stdint cimport UINT64_MAX
import random

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
    cdef object ports

    # Default ip to connect to a controller is the localhost
    # Default port is the IANA number allocated for OpenFlow  
    def __cinit__(self, name, uint64_t dp_id, ctrl_ip = "127.0.0.1", 
                  ctrl_port = 6653):
        self._dp_ptr = dp_new(dp_id, ctrl_ip, ctrl_port)
        self.name = name

    # def add_port(self, intf):
    #     mac = intf.eth_addr.replace(':', '').decode('hex')
    #     cdef uint8_t *c_eth_addr = mac
    #     dp_add_port(self._dp_ptr, intf.port_id, c_eth_addr, intf.max_speed, intf.cur_speed) 

    def add_port(self, port, eth_addr, max_speed = 1000000, cur_speed = 1000000):
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        dp_add_port(self._dp_ptr, port, c_eth_addr, max_speed, cur_speed)

    property name:
        def __get__(self):
            return dp_name(self._dp_ptr)

        def __set__(self, name):
            dp_set_name(self._dp_ptr, name)

    @property
    def dp_id(self):
        return dp_id(self._dp_ptr) 

    @property
    def uuid(self):
        return dp_uuid(self._dp_ptr)     


cdef class Host:
    cdef host* _host_ptr
    cdef int exec_id

    def __cinit__(self, name):
        self._host_ptr = host_new()
        # It needs to be improved when number of apps grow
        host_add_app(self._host_ptr, 1)  #PING
        host_add_app(self._host_ptr, 17) #UDP
        self.exec_id = 1
        self.name = name

    def add_port(self, port, eth_addr, ip = None, 
                netmask = None, max_speed = 1000000, cur_speed = 1000000):
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
        host_add_app_exec(self._host_ptr, self.exec_id, 1, start_time, <void*> &ip, sizeof(int))
        self.exec_id += 1

    # Rate in Mbps, duration and interval in seconds
    def udp(self, dst, start_time, duration = 60, interval = 5, rate = 10, dst_port = 5001, src_port = random.randint(5002, 65000)):
        cdef int ip
        cdef raw_udp_args args
        ip_parts = dst.split('.')
        ip = (int(ip_parts[0]) << 24) + (int(ip_parts[1]) << 16) + (int(ip_parts[2]) << 8) + int(ip_parts[3])
        args.duration = duration
        args.rate = rate * 1000000 #Mbits to bits
        args.ip_dst = ip
        args.dst_port = dst_port
        args.src_port = src_port
        args.interval = interval
        times = duration / interval
        init_time = start_time
        microsec_interval = interval * 1000000
        for i in range(0, times):
            host_add_app_exec(self._host_ptr, self.exec_id, 17, init_time, <void*> &args, sizeof(raw_udp_args))
            init_time = init_time +  microsec_interval
            self.exec_id += 1

    property name:
        def __get__(self):
            return host_name(self._host_ptr)

        def __set__(self, name):
            host_set_name(self._host_ptr, name)

    @property
    def uuid(self):
        return host_uuid(self._host_ptr)

cdef class SimConfig:
    cdef sim_config *_config_ptr
    args = ("mode", "end_time")

    def __cinit__(self):
        self._config_ptr = sim_config_new()

    @property
    def mode(self):
        return sim_config_get_mode(self._config_ptr)
        
    def set_mode(self, value):
        sim_config_set_mode(self._config_ptr, value) 

    @property
    def end_time(self):
        return sim_config_get_end_time(self._config_ptr)
        
    def set_end_time(self, value):
        sim_config_set_end_time(self._config_ptr, value)

    @property
    def ctrl_idle_interval(self):
        return sim_config_get_ctrl_idle_interval(self._config_ptr)
        
    def set_ctrl_idle_interval(self, value):
        sim_config_set_ctrl_idle_interval(self._config_ptr, value)


cdef class Topology:
    cdef topology *_topo_ptr
    cdef object dps
    cdef object node_info
    cdef object nodes

    def __cinit__(self):
        self._topo_ptr = topology_new()
        self.dps = []
        self.node_info = {}
        self.nodes = {}

    # def __dealloc__(self):
    #     if self._topo_ptr != NULL:
    #         topology_destroy(self._topo_ptr)

    # @staticmethod
    cdef topology* topo_ptr(self):
        return self._topo_ptr

    def add_node(self, Node, **kwargs):
        if isinstance(Node, SDNSwitch):
            dp = <SDNSwitch> Node
            if "name" in kwargs:
                self.node_info[kwargs["name"]] = kwargs
                self.nodes[kwargs["name"]] = Node
            topology_add_datapath(self._topo_ptr, dp._dp_ptr)

        elif isinstance (Node, Host):
            h = <Host> Node
            # TODO: Remember to improve it
            if "name" in kwargs:
                self.node_info[kwargs["name"]] = kwargs
                self.nodes[kwargs["name"]] = Node
            topology_add_host(self._topo_ptr, h._host_ptr)

    def add_link(self, node1, node2, port1, port2, bw = 1, latency = 0):
        topology_add_link(self._topo_ptr, node1.uuid, node2.uuid, port1, port2, bw, latency, False)


    property nodes:
        def __get__(self):
            return self.nodes

    property node_info:
        def __get__(self):
            return self.node_info

    @property
    def dp(self):
        return "We don't have: %s" % self.dps

    @property
    def dps_num(self):
        return topology_dps_num(self._topo_ptr)

    @property
    def links_num(self):
        return topology_links_num(self._topo_ptr)

    

cdef class Sim:
    cdef Topology topo
    cdef SimConfig config
    # Default interval is 0.5 seconds
    def __cinit__(self, topo, mode = 1, end_time = UINT64_MAX, 
                  ctrl_interval = 10000):
        self.topo = topo
        # self._topo_ptr = topo._topo_ptr
        # print topo.topo_ptr()
        self.config =  SimConfig()
        self.config.set_mode(mode)
        self.config.set_end_time(end_time)
        self.config.set_ctrl_idle_interval(ctrl_interval)

    def start(self):
        topo = <Topology> self.topo
        config = <SimConfig> self.config    
        start(topo._topo_ptr, config._config_ptr)     