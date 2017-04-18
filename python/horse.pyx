cimport horse
from libc.stdint cimport uint64_t

cdef class SDNSwitch:
    cdef datapath* _dp_ptr

    def __cinit__(self, uint64_t dp_id):
        self._dp_ptr = dp_new(dp_id)

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

    def add_port(self, port, eth_addr, max_speed = 1000000, cur_speed = 1000000):
        # TODO: Add mac converstion to utils...
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        host_add_port(self._host_ptr, port, c_eth_addr, max_speed, cur_speed)

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

    def add_link(self, node1, node2, port1, port2):
        topology_add_link(self._topo_ptr, node1.uuid, node2.uuid, port1, port2, 10, 20, False)

    @property
    def dp(self):
        return "We don't have: %s" % self.dps

    @property
    def dps_num(self):
        return topology_dps_num(self._topo_ptr)

    @property
    def links_num(self):
        return topology_links_num(self._topo_ptr)
