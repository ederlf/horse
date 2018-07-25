cdef class Daemon:
    cdef namespace
    cdef router_id
    cdef runDir
    cdef ecmp_enabled

cdef class QuaggaDaemon(Daemon):
    cdef quagga_daemon* _qd_ptr

cdef class ExaBGPDaemon(Daemon):
    cdef exabgp_daemon* _exa_ptr
    cdef config_file
    cdef object local_ips
    cdef exabgp_daemon* get_exabgp_ptr(self)

cdef extern from "net/routing/quagga_daemon.h":
    
    cdef struct quagga_daemon:
        pass

    quagga_daemon* quagga_daemon_new(char *namespace)
    void set_quagga_daemon_zebra_file(quagga_daemon *d, char *fname)
    void set_quagga_daemon_bgpd_file(quagga_daemon *d, char *fname)
    void set_quagga_daemon_ospfd_file(quagga_daemon *d, char *fname)
    void set_quagga_daemon_ospf6d_file(quagga_daemon *d, char *fname)
    void set_quagga_daemon_ripd_file(quagga_daemon *d, char *fname)
    void set_quagga_daemon_ripngd_file(quagga_daemon *d, char *fname)

cdef extern from "net/routing/exabgp_daemon.h":
    
    cdef struct exabgp_daemon:
        pass

    exabgp_daemon* exabgp_daemon_new(char *namespace);
    void set_exabgp_daemon_config_file(exabgp_daemon *d, char *fname);
    void exabgp_daemon_add_port_ip(exabgp_daemon *d, char *ip);




