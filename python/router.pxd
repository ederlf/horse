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