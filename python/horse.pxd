from libc.stdint cimport uint64_t
from libc.stdint cimport uint32_t
from libc.stdint cimport uint8_t

cdef extern from "sim/sim.h":

    cdef struct datapath:
        pass

    cdef struct topology:
        pass

    # Datapath .h
    datapath* dp_new(uint64_t)
    void dp_destroy(datapath* dp)
    void dp_add_port(datapath *dp, uint32_t port_id, uint8_t *eth_addr)
    uint64_t dp_id(const datapath* dp)
    uint64_t dp_uuid(const datapath* dp)
    # Topology.h
    topology* topology_new()
    void topology_destroy(topology *topo)
    void topology_add_datapath(topology *topo, datapath *dp)
    uint32_t topology_dps_num(const topology *topo)
    uint32_t topology_links_num(const topology *topo)
    void topology_add_link(topology *t, uint64_t uuidA, uint64_t uuidB, uint32_t portA, uint32_t portB, uint32_t bw, uint32_t latency, bint directed)
    # Sim.h
    void start(topology *topo)
    


    

    
