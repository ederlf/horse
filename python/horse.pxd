from libc.stdint cimport uint64_t
from libc.stdint cimport uint32_t
from libc.stdint cimport uint16_t
from libc.stdint cimport uint8_t

cdef extern from "sim/sim.h":

    cdef struct datapath:
        pass

    cdef struct host:
        pass

    cdef struct topology:
        pass

    cdef struct sim_config:
        pass

    # Datapath .h
    datapath* dp_new(uint64_t, char*, int)
    void dp_destroy(datapath* dp)
    void dp_add_port(datapath *dp, uint32_t port_id, uint8_t *eth_addr, uint32_t speed, uint32_t cur_speed)
    uint64_t dp_id(const datapath* dp)
    uint64_t dp_uuid(const datapath* dp)

    # Host.h

    host* host_new()
    void host_destroy(host* h)
    void host_add_port(host *h, uint32_t port_id, 
                        uint8_t *eth_addr, uint32_t speed, uint32_t cur_speed)
    void host_set_intf_ipv4(host *h, uint32_t port_id, 
                            uint32_t addr, uint32_t netmask)
    uint64_t host_uuid(const host* h)
    void host_add_app(host *h, uint16_t type)
    void host_add_app_exec(host *h, uint64_t id, uint16_t type, uint64_t 
                        start_time, void* args, size_t arg_len)
    void host_start_app(host *h, uint64_t id)

   

    # Topology.h
    topology* topology_new()
    void topology_destroy(topology *topo)
    void topology_add_datapath(topology *topo, datapath *dp)
    void topology_add_host(topology *topo, host *h)
    uint32_t topology_dps_num(const topology *topo)
    uint32_t topology_links_num(const topology *topo)
    void topology_add_link(topology *t, uint64_t uuidA, uint64_t uuidB, uint32_t portA, uint32_t portB, uint32_t bw, uint32_t latency, bint directed)
    
    # Sim.h
    # TODO: Create an object for the simulator to ease adding config?
    void start(topology *topo, sim_config *config)
    
    # Config.h
    sim_config *sim_config_new()
    void sim_config_set_mode(sim_config *conf, int mode)
    int sim_config_get_mode(sim_config *conf)
    void sim_config_set_end_time(sim_config *conf, uint64_t end_time)
    uint64_t sim_config_get_end_time(sim_config *conf)
    void sim_config_set_ctrl_idle_interval(sim_config *conf, 
                                       uint64_t interval)
    uint64_t sim_config_get_ctrl_idle_interval(sim_config *conf)
