cimport router
import sys
import re
from msg import ip2int, int2ip, netmask2cidr
from horse import Router
import os
import json
from util import namedtuple_with_defaults 

def writeLine(configFile, indent, line):
    intentStr = ''
    for _ in range(0, indent):
        intentStr += '  '
    configFile.write('%s%s\n' % (intentStr, line))
    
def getRouterId(interfaces):
    for intfAttributesList in interfaces.itervalues():
        if not isinstance(intfAttributesList, list):
            continue
        # Try use the first set of attributes, but if using vlans they might not have addresses
        intfAttributes = intfAttributesList[1] if not intfAttributesList[0]['ipAddrs'] else intfAttributesList[0]
        return intfAttributes['ipAddrs'][0].split('/')[0]

# Specific configuration to a neighbor
# Except for the ASN and IP, all the other fields are here for future usage
cdef class BGPNeighbor:
    cdef asn
    cdef ip
    cdef local_ip # ExaBGP needs the local IP
    cdef ebgp_multihop
    cdef next_hop_self
    cdef peer_weight
    cdef maximum_prefix
    cdef distribute_list
    cdef prefix_list
    cdef filter_list
    cdef route_map
    cdef route_reflector_client
    cdef port


    def __cinit__(self, asn, ip, local_ip = None, ebgp_multihop = False, next_hop_self = False,
                  peer_weight = None, maximum_prefix = None, distribute_list = 
                  None, prefix_list = None, filter_list = None, 
                  route_map = None, route_reflector_client = False, port = None):
        self.asn = asn
        self.ip = ip
        self.local_ip = local_ip
        self.ebgp_multihop = ebgp_multihop
        self.next_hop_self = next_hop_self
        self.peer_weight = peer_weight
        self.maximum_prefix = maximum_prefix
        self.distribute_list = distribute_list
        self.prefix_list = filter_list
        self.route_map = route_map
        self.route_reflector_client = route_reflector_client
        self.port = port

    def neighbor_to_dict(self):
        d = {}
        d["asn"] = self.asn
        d["ip"] = self.ip
        d["local_ip"] = self.local_ip
        return d

    property asn:
        def __get__(self):
            return self.asn

    property ip:
        def __get__(self):
            return self.ip

    property local_ip:
        def __get__(self):
            return self.local_ip

    property port:
        def __get__(self):
            return self.port

        # def __set__(self, asn):
        #     self.router_id = router_id

# Values for redistribute
RKERNEL = 1
RSTATIC = 2
RCONNECTED = 4
RRIB = 8
ROSPF = 16

# This class serves as a configuration for the BGP protocol
cdef class BGP:
    cdef asn
    cdef router_id
    cdef object networks
    cdef med 
    cdef cmp_med
    cdef object neighbors
    cdef redistribute # Bitmap of possible values to redistribute
    cdef multiple_instances # Complex as it contains neighbors per view
    cdef maximum_paths # Maximum number of allowed paths. All attributes should be equal.
    cdef relax   # Allows load balancing among different ASes. Only AS path needs to match.
    cdef allowas_in

    def __cinit__(self, asn = None, router_id = None, neighbors = None,
                  networks=None, med = None, always_compare_med = False, 
                  redistribute = None, multiple_instances = None, 
                  maximum_paths = None, relax = False, allowas_in = False):
        self.asn = asn
        self.router_id = router_id
        self.networks = networks
        self.neighbors = neighbors
        self.med = med 
        self.cmp_med = always_compare_med
        self.redistribute = redistribute
        self.multiple_instances = multiple_instances
        self.maximum_paths = maximum_paths
        self.relax = relax
        self.allowas_in = allowas_in

    def add_neighbor(self, neighbor):
        if not self.neighbors:
            self.neighbors = []
        self.neighbors.append(neighbor)

    # def set_router_id(self, router_id):
    #     if isinstance(router_id, basestring):
    #         bgp_set_router_id(self._bgp_ptr, ip2int(router_id))
    #     else:
    #         bgp_set_router_id(self._bgp_ptr, router_id)

    def set_allowas_in(self, value):
        self.allowas_in = value

    def add_advertised_prefix(self, prefix, 
                              as_path = None, communities = None):
        self.networks[prefix] = {}
        if communities:
            self.networks[prefix]["COMM"] = communities
        if as_path:
            self.networks[prefix]["AS-PATH"] = as_path
    
    def add_advertised_prefixes(self, prefixes, as_path = None,
                                communities = None):
        for prefix in prefixes:
            self.add_advertised_prefix(prefix, as_path, communities)

    def set_maximum_paths(self, value):
        self.maximum_paths = value

    def set_relaxed_maximum_paths(self, value = True):
        self.relax = value 

    def add_advertised_prefix_list(self, prefixes, next_hop= None, as_path = None, communities = None):
        for prefix in prefixes:
            self.add_advertised_prefix(prefix, next_hop, as_path, communities)

    # This file is needed by the simulated side of the routers.
    def write_auxiliary_config_file(self, runDir, rname):
        conf = {}
        neighbors = {}
        conf["prefixes"] = self.networks
        for n in self.neighbors:
            neighbors[n.ip] = n.neighbor_to_dict()
        conf["neighbors"] = neighbors
        conf["asn"] = self.asn
        conf["router_id"] = self.router_id
        conf["max_paths"] = self.maximum_paths
        conf["relax"] = self.relax
        conf["allowas_in"] = self.allowas_in
        with file("%s/conf-bgp.%s" % (runDir, rname), "w") as f:
            json.dump(conf, f)

    property router_id:
        def __get__(self):
            return self.router_id

        def __set__(self, router_id):
            self.router_id = router_id

    property neighbors:
        def __get__(self):
            return self.neighbors

        def __set__(self, neighbors):
            self.neighbors = neighbors

    property maximum_paths:
        def __get__(self):
            return self.maximum_paths

        def __set__(self, maximum_paths):
            self.maximum_paths = maximum_paths

netparams = ('network', 'area')
OSPFNet = namedtuple_with_defaults('OSPFNet', netparams, ('0.0.0.0', 0))
ifaceparams = ('name', 'cost', 'type', 'priority', 'hello_interval')
# Default priority is the same as Quagga.
OSPFInterface = namedtuple_with_defaults('OSPFInterface', ifaceparams,
                                         (None, 1, 'point-to-point', 0, 10))
cdef class OSPF:
    cdef router_id
    cdef interfaces
    cdef networks
    cdef hello_interval
    # @networks: list of announced networks 
    # @interfaces: list of OSPFInterface"
    def __cinit__(self, router_id = None, networks = [], interfaces = [],
                  debug= False):
        self.router_id = router_id
        self.networks = networks
        self.interfaces = interfaces
        self.debug = false

    property router_id:
        def __get__(self):
            return self.router_id

        def __set__(self, router_id):
            self.router_id = router_id

    property interfaces:
        def __get__(self):
            return self.interfaces

        def __set__(self, interfaces):
            self.interfaces = interfaces

    property networks:
        def __get__(self):
            return self.networks

        def __set__(self, networks):
            self.networks = networks

    property debug:
        def __get__(self):
            return self.debug

        def __set__(self, debug):
            self.debug = debug

cdef class Daemon:
    # cdef namespace
    # cdef router_id
    # cdef runDir
    # cdef ecmp_enabled

    def __init__(self, namespace, runDir, *protocols, **kwargs):
        self.namespace = namespace
        self.runDir = runDir

    def __cinit__(self, namespace, runDir, *protocols, **kwargs):
        self.namespace = None
        self.runDir = None
        self.router_id = None

    def check_ecmp(self, proto):
        if proto.maximum_paths > 1:
            self.ecmp_enabled =  True 

    def get_ecmp_enabled(self):
        return self.ecmp_enabled

    property router_id:
        def __get__(self):
            return self.router_id

        def __set__(self, router_id):
            self.router_id = router_id


cdef class QuaggaDaemon(Daemon):
    # cdef quagga_daemon* _qd_ptr
    # cdef router_id
    # cdef runDir

    # Can receive either a configuration file or a protocol object
    def __cinit__(self, namespace, runDir, protocols, kwargs):
        
        super(QuaggaDaemon, self).__init__(namespace, runDir)
        self._qd_ptr = quagga_daemon_new(namespace)
        if not "zebra_conf" in kwargs:
            # Generate Basic zebra file.
            self.zebraConfFile = runDir + "/zebra%s.conf" % namespace
            self.generateZebra()
        set_quagga_daemon_zebra_file(self._qd_ptr, self.zebraConfFile)
        if "bgpd_conf" in kwargs:
            self.bgpd_conf = kwargs["bgpd_conf"]
            set_quagga_daemon_bgpd_file(self._qd_ptr, self.bgpd_conf)
            bgp = self.parse_bgpd_file()
            if bgp.router_id:
                self.router_id = bgp.router_id
            self.check_ecmp(bgp)
        if "ospfd_conf" in kwargs:
            self.ospfd_conf = kwargs["ospfd_conf"]
            set_quagga_daemon_ospfd_file(self._qd_ptr, self.ospfd_conf)
        if "ospf6d_conf" in kwargs:
            self.ospf6d_conf = kwargs["ospf6d_conf"]
            set_quagga_daemon_ospf6d_file(self._qd_ptr, self.ospf6d_conf)
        if "ripd_conf" in kwargs:
            self.ripd_conf = kwargs["ripd_conf"]
            set_quagga_daemon_ripd_file(self._qd_ptr, self.ripd_conf)
        if "ripngd_conf"  in kwargs:
            self.ripngd_conf = kwargs["ripngd_conf"]
            set_quagga_daemon_ripngd_file(self._qd_ptr, self.ripngd_conf)
        for p in protocols:
            if isinstance(p, BGP):
                bgp = <BGP> p
                self.bgpd_conf = "%s/bgpd%s.conf" % (runDir, self.namespace)
                self.generateBgpd(bgp)
                set_quagga_daemon_bgpd_file(self._qd_ptr, self.bgpd_conf)   
                self.check_ecmp(bgp)
                if bgp.router_id:
                    self.router_id = bgp.router_id
            elif isinstance(p, OSPF):
                ospf = <OSPF> p
                self.ospfd_conf = "%s/ospfd%s.conf" % (runDir, self.namespace)
                self.generateOspfd(ospf)
                set_quagga_daemon_ospfd_file(self._qd_ptr, self.ospfd_conf)
                if ospf.router_id:
                    self.router_id = ospf.router_id

    cdef quagga_daemon* get_quagga_ptr(self):
        return self._qd_ptr

    def generateZebra(self):
        configFile = open(self.zebraConfFile, 'w+')
        configFile.write('hostname %s\n' % self.namespace)
        configFile.write('password %s\n' % "horse")
        configFile.write('log file %s/z_%s debugging\n' % (self.runDir,
                                                           self.namespace))
        configFile.close()

    def generateBgpd(self, BGP bgp):
        configFile = open(self.bgpd_conf, 'w+')
        writeLine(configFile, 0, 'hostname %s' % self.namespace);
        writeLine(configFile, 0, 'password %s' % 'horse')
        writeLine(configFile, 0, 'log file %s/qbgp_%s' % (self.runDir, self.namespace))
        writeLine(configFile, 0, 'debug bgp')
        writeLine(configFile, 0, '!')
        writeLine(configFile, 0, 'router bgp %s' % bgp.asn)
        if bgp.router_id:
            writeLine(configFile, 1, 'bgp router-id %s' % bgp.router_id)
        if bgp.relax:
            writeLine(configFile, 1, 'bgp bestpath as-path multipath-relax')    
        
        # writeLine(1, 'timers bgp %s' % '3 9')
        if bgp.maximum_paths:
            writeLine(configFile, 1, 'maximum-paths %s' % bgp.maximum_paths)
        writeLine(configFile, 1, '!')
        
        for neighbor in bgp.neighbors:
            writeLine(configFile, 1, 'neighbor %s remote-as %s' % (neighbor.ip, neighbor.asn))
            writeLine(configFile, 1, 'neighbor %s ebgp-multihop' % neighbor.ip)
            writeLine(configFile, 1, 'neighbor %s timers connect %s' % (neighbor.ip, '5'))
            writeLine(configFile, 1, 'neighbor %s advertisement-interval %s' % (neighbor.ip, '1'))
            if neighbor.port:
                writeLine(configFile, 1, 'neighbor %s port %s' % (neighbor.ip, neighbor.port))
            writeLine(configFile, 1, '!')
            
        for route in bgp.networks:
            writeLine(configFile, 1, 'network %s' % route)
        configFile.close()

    def parse_bgpd_file(self):
        bgp = BGP()
        with file(self.config_file, "r") as f:
            conf = f.read()
            router_id = re.findall( r'bgp router-id [0-9]+(?:\.[0-9]+){3}', conf )
            if router_id:
                bgp.router_id = router_id.split()[2]
            asn =  re.findall( r'router bgp [0-9]*', conf)
            if asn:
                bgp.asn = asn.split()[2]
            neighbors = re.findall( r'neighbor [0-9]+(?:\.[0-9]+){3} remote-as [0-9]*', conf )
            if neighbors:
                # TODO parse other neighbor fields
                for n in neighbors:
                    nfields = n.split()
                    bgp.add_neighbor(BGPNeighbor(nfields[1], nfields[3]))
            relax =  re.findall( r' bgp bestpath as-path multipath-relax', conf)
            if relax:
                bgp.relax =  True
            maximum_paths = re.findall( r'maximum-paths [0-9]*', conf)
            if maximum_paths:
                bgp.maximum_paths = maximum_paths.split()[1]
        return bgp

    def generateOspfd(self, OSPF ospf):
        configFile = open(self.ospfd_conf, 'w+')
        writeLine(configFile, 0, 'hostname %s' % self.namespace);
        writeLine(configFile, 0, 'password %s' % 'horse')
        writeLine(configFile, 0, 'log file %s/qospf_%s' % (self.runDir,
                                                           self.namespace))
        if (self.debug):
            writeLine(configFile, 0, 'debug ospf event')
            writeLine(configFile, 0, 'debug ospf zebra')
            writeLine(configFile, 0, 'debug ospf packet all')
            writeLine(configFile, 0, 'debug ospf lsa')
        for intf in ospf.interfaces:
            writeLine(configFile, 0, '!')
            writeLine(configFile, 0, "interface %s" % intf.name)
            writeLine(configFile, 2, "ip ospf cost %s" % intf.cost)
            writeLine(configFile, 2, "ip ospf network %s" % intf.type)
            writeLine(configFile, 2, "ip ospf priority %s" % intf.priority)
            writeLine(configFile, 2, "ip ospf hello-interval %s" % intf.hello_interval)
            writeLine(configFile, 0, '!')
        # writeLine(configFile, 0, "interface %s-inet" % self.namespace)
        writeLine(configFile, 0, 'router ospf')
        writeLine(configFile, 2, 'passive-interface default')
        writeLine(configFile, 2, 'no passive-interface %s' % intf.name)
        if ospf.router_id:
            writeLine(configFile, 2, 'ospf router-id %s' % ospf.router_id)
        for net in ospf.networks:
            writeLine(configFile, 2, 'network %s area %s' % (net.network,
                                                             net.area))
        writeLine(configFile, 0, '!')


        configFile.close()

cdef class ExaBGPDaemon(Daemon):
    # cdef exabgp_daemon* _exa_ptr

    def __cinit__(self, namespace, runDir, protocols,
                  config_files):

        super(ExaBGPDaemon, self).__init__(namespace, runDir)
        self._exa_ptr = exabgp_daemon_new(namespace)
        self.local_ips = []
        if "exabgp_conf" in config_files: 
            self.config_file = config_files["exabgp_conf"]
            if os.path.isfile(self.config_file):
                set_exabgp_daemon_config_file(self._exa_ptr, self.config_file)
                bgp = self.parse_config_file()
                bgp.write_auxiliary_config_file(runDir, self.namespace)
                if bgp.router_id:
                    self.router_id = bgp.router_id
                self.check_ecmp(bgp)
        for p in protocols:
            if isinstance(p, BGP):
                bgp = <BGP> p
                self.config_file = "%s/exabgp%s.conf" % (runDir, namespace)
                if bgp.router_id:
                    self.router_id = bgp.router_id
                self.check_ecmp(bgp)
                for i in range(0, len(bgp.neighbors)): 
                    neigh = bgp.neighbors[i]
                    self.local_ips.append(neigh.local_ip)
                    self.add_ip(neigh.local_ip)
                self.router_id = bgp.router_id = max(self.local_ips)
                self.generateExaBGPconfig(bgp)
                set_exabgp_daemon_config_file(self._exa_ptr, self.config_file)
                bgp.write_auxiliary_config_file(runDir, self.namespace)

    cdef exabgp_daemon* get_exabgp_ptr(self):
        return self._exa_ptr

    def parse_config_file(self):
        bgp = BGP()
        with file(self.config_file, "r") as f:
            conf = f.read()
            # neighbor = []
            router_id = re.findall( r'router-id [0-9]+(?:\.[0-9]+){3}', conf )
            asn =  re.findall( r'local-as [0-9]*', conf)
            ips = re.findall( r'neighbor [0-9]+(?:\.[0-9]+){3}', conf )
            asys = re.findall( r'peer-as [0-9]*', conf)
            local_ips = re.findall( r'local-address [0-9]+(?:\.[0-9]+){3}', conf)
            self.local_ips = [x[14:] for x in local_ips]
            # Router id is in the configuration
            if router_id and bgp.router_id == None:
                bgp.router_id = router_id[0][10:]
            for ip in self.local_ips:
                self.add_ip(ip)
            if asn:
                bgp.asn = asn[0][9:]
            for neigh_ip, asn, local_ip in zip(ips, asys, local_ips):
                bgp.add_neighbor(BGPNeighbor(neigh_ip[9:], asn[8:],
                                             local_ip = local_ip))
        return bgp


    def generateExaBGPconfig(self, BGP bgp):
        configFile = open(self.config_file, 'w+')

        writeLine(configFile, 0, 'process client{')
        writeLine(configFile, 2, 'run /usr/bin/python /home/vagrant/horse/horse/router_client.py %s;' % self.namespace)
        writeLine(configFile, 2, 'encoder json;\n}\n')

        writeLine(configFile, 0, 'template {')
        writeLine(configFile, 2, 'neighbor router {')
        writeLine(configFile, 4, 'family {')
        writeLine(configFile, 6, 'ipv4 unicast;')
        writeLine(configFile, 4, '}')
        writeLine(configFile, 4, 'api speaking {')
        writeLine(configFile, 6, 'processes [client];')
        writeLine(configFile, 6, 'neighbor-changes;')
        writeLine(configFile, 6, 'receive {')
        writeLine(configFile, 8, 'parsed;')
        writeLine(configFile, 8, 'update;')
        writeLine(configFile, 6, '}')
        writeLine(configFile, 4, '}')
        writeLine(configFile, 4, 'local-as %s;' % bgp.asn)
        writeLine(configFile, 4, 'manual-eor true;')
        writeLine(configFile, 2, '}')
        writeLine(configFile, 0, '}\n')

        for neighbor in bgp.neighbors:
            writeLine(configFile, 0, 'neighbor %s {' % neighbor.ip)
            writeLine(configFile, 2, 'inherit router;')
            writeLine(configFile, 2, 'peer-as %s;' % neighbor.asn)
            writeLine(configFile, 2, 'router-id %s;' % bgp.router_id)
            writeLine(configFile, 2, 'local-address %s;' % neighbor.local_ip)
            writeLine(configFile, 2, 'group-updates false;')
            writeLine(configFile, 0, '}\n')

        configFile.close()

    def add_ip(self, ip):
        exabgp_daemon_add_port_ip(self._exa_ptr, ip)
    