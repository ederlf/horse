from libc.stdint cimport uint64_t

cdef extern from "sim/sim.h":
    cdef struct datapath:
        pass

    cdef struct topology:
        pass

    datapath* dp_new(uint64_t)
    void dp_destroy(datapath* dp)
    uint64_t dp_id(datapath* dp)
    topology* topology_new()
    void topology_destroy(topology *topo)
    void topology_add_datapath(topology *topo, datapath *dp)
    void start(topology *topo)
    


    

    
