cimport horse
from libc.stdint cimport uint64_t

cdef class SDNSwitch:
    cdef datapath* _dp_ptr

    def __cinit__(self, uint64_t dp_id):
        self._dp_ptr = dp_new(dp_id)

    def add_port(self, port, eth_addr):
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        dp_add_port(self._dp_ptr, port, c_eth_addr)      

    @property
    def dp_id(self):
        return dp_id(self._dp_ptr) 

    @property
    def uuid(self):
        return uuid(self._dp_ptr)     


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

    def add_switch(self, SDNSwitch dp):
        # self.dps.append(dp)
        topology_add_datapath(self._topo_ptr, dp._dp_ptr)

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
