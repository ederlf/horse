cimport horse
from libc.stdint cimport uint64_t

cdef class SDNSwitch:
    cdef datapath* _dp_ptr

    def __cinit__(self, uint64_t dp_id):
        self._dp_ptr = dp_new(dp_id)

    @property
    def dp_id(self):
        return dp_id(self._dp_ptr) 


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

    @property
    def dp(self):
        return "We don't have: %s" % self.dps

    # @dp.setter
    # def dp(self, dp):
    #     self.dps.append(dp)

    # @dp.deleter
    # def dp(self):
    #     del self.dps[:]   



