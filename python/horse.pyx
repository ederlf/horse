cimport horse
from libc.stdint cimport uint64_t
from libc.stdint cimport UINT64_MAX
import random
import os.path
import json
import re
from msg import ip2int, int2ip, netmask2cidr
from collections import namedtuple

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
#         self.ipv4_addr = (int(ip_parts[0]) << 24) + (int(ip_parts[1]) << 16) + (int(ip_parts[2]) << 8) + int(BGP[3])
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

    def add_port(self, port, eth_addr, max_speed = 1000000, cur_speed = 10000):
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

cdef class BGP:
    cdef bgp *_bgp_ptr
    cdef neighbors
    cdef local_ips
    cdef prefixes
    cdef asn
    cdef router_id
    cdef max_paths # Maximum number of allowed paths. All attributes should be equal.
    cdef relax   # Allows load balancing among different ASes. Only AS path needs to match.
    cdef allowas_in

    def __cinit__(self, config_file=None):
        self.neighbors = {}
        self.prefixes = {}
        self.max_paths = 0
        self.relax = False
        self.router_id = 0
        self.allowas_in = False
        self.local_ips = []

        if config_file and os.path.isfile(config_file):
            self._bgp_ptr = bgp_new(config_file)
            with file(config_file, "r") as f:
                conf = f.read()
                # neighbor = []
                router_id = re.findall( r'router-id [0-9]+(?:\.[0-9]+){3}', conf )
                asn =  re.findall( r'local-as [0-9]*', conf)
                ip = re.findall( r'neighbor [0-9]+(?:\.[0-9]+){3}', conf )
                asys = re.findall( r'peer-as [0-9]*', conf)
                local_ips = re.findall( r'local-address [0-9]+(?:\.[0-9]+){3}', conf)
                self.local_ips = [x[14:] for x in local_ips]
                if router_id and self.router_id == 0:
                    self.router_id = router_id[0][10:]
                    bgp_set_router_id(self._bgp_ptr, ip2int(self.router_id))
                else:
                    for ip in self.local_ips:
                        int_ip = ip2int(ip)
                        if int_ip > self.router_id:
                            self.router_id = ip
                    bgp_set_router_id(self._bgp_ptr, ip2int(self.router_id))
                if asn:
                    self.asn = asn[0][9:]
                for neighbor, asn in zip(ip, asys):
                    n = neighbor[9:]
                    asn = int(asn[8:])
                    self.neighbors[n] = {}
                    self.neighbors[n]["MED"] = 0
                    self.neighbors[n]["ASN"] = asn
        else:
            print "No config file provided for bgp router"

    def set_router_id(self, router_id):
        if isinstance(router_id, basestring):
            bgp_set_router_id(self._bgp_ptr, ip2int(router_id))
        else:
            bgp_set_router_id(self._bgp_ptr, router_id)

    def set_allowas_in(self, value):
        self.allowas_in = value

    def add_advertised_prefix(self, prefix, 
                              as_path = None, communities = None):
        self.prefixes[prefix] = {}
        if communities:
            self.prefix[prefix]["COMM"] = communities
        if as_path:
            self.prefixes[prefix]["AS-PATH"] = as_path
    
    def add_advertised_prefixes(self, prefixes, as_path = None,
                                communities = None):
        for prefix in prefixes:
            self.add_advertised_prefix(prefix, as_path, communities)

    def set_maximum_paths(self, value):
        self.max_paths = value

    def set_relaxed_maximum_paths(self, value = True):
        self.relax = value 

    def write_config_file(self, rname):
        conf = {}
        conf["prefixes"] = self.prefixes
        conf["neighbors"] = self.neighbors
        conf["asn"] = self.asn
        conf["router_id"] = self.router_id
        conf["max_paths"] = self.max_paths
        conf["relax"] = self.relax
        conf["allowas_in"] = self.allowas_in
        with file("/tmp/conf-bgp.%s" % rname, "w") as f:
            json.dump(conf, f)

    def add_advertised_prefix_list(self, prefixes, next_hop= None, as_path = None, communities = None):
        for prefix in prefixes:
            self.add_advertised_prefix(prefix, next_hop, as_path, communities)

    def add_neighbor(self, neighbor_ip, neighbor_as):
        int_ip = ip2int(neighbor_ip) 
        bgp_add_neighbor(self._bgp_ptr, int_ip, neighbor_as)

    def get_local_ips(self):
        return self.local_ips


params = ('id',  'eth_addr', 'ip', 'netmask', 'max_speed', 'cur_speed')
Port = namedtuple('Port', params)
cdef class Router:
    cdef router* _router_ptr 
    cdef ports

    def __init__(self, name):
        self.name = name

    def __cinit__(self):
        self._router_ptr = router_new()
        # self.name = name
        self.ports = {}

    def add_port(self, port, eth_addr, ip = None, 
                netmask = None, max_speed = 1000000, cur_speed = 10000):
        # TODO: Add mac conversion to utils...
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        # Add internal for check of configuration
        self.ports[ip] = Port(port, eth_addr, ip, netmask, max_speed, cur_speed)
        router_add_port(self._router_ptr, port, c_eth_addr, max_speed, cur_speed)
        if ip != None and netmask != None:
            int_ip = ip2int(ip)
            int_nm = ip2int(netmask)
            router_set_intf_ipv4(self._router_ptr, port, int_ip, int_nm)

    def add_protocol(self, Proto):
        if isinstance(Proto, BGP):
            proto = <BGP> Proto
            proto.write_config_file(self.name)
            if proto.max_paths:
                router_set_ecmp(self._router_ptr, True)
            for ip in proto.local_ips:
                if ip in self.ports:
                    port = self.ports[ip]
                    ip_conf = '/'.join([ip, str(netmask2cidr(port.netmask))])
                    bgp_add_local_ip(proto._bgp_ptr, ip_conf)
                else:
                    print "BGP local port does not exist"
            router_add_bgp(self._router_ptr, proto._bgp_ptr)

    property name:
        def __get__(self):
            return router_name(self._router_ptr)

        def __set__(self, name):
            router_set_name(self._router_ptr, name)

    @property
    def uuid(self):
        return router_uuid(self._router_ptr)

cdef class BGPRouter(Router):
    # TODO: Should just use the members of the BGP class
    cdef object routes
    cdef object intfDict
    cdef object neighbors
    cdef object bgp
    cdef object exabgpConfFile
    cdef object asNum

    # def __new__(cls, name):
    #     return super().__new__(cls, name)

    def __init__(self, name, intfDict,
                 asNum, neighbors, routes,
                 exabgpConfFile=None,
                 allowas_in = False,
                 relax = False,
                 maximum_paths= 0,
                 runDir='/tmp', *args, **kwargs):
        
        # self.name = name
        super(BGPRouter, self).__init__(name)
        self.routes = routes
        self.intfDict = intfDict
        if exabgpConfFile is not None:
            self.exabgpConfFile = exabgpConfFile
            self.bgp = BGP(config_file =  self.exabgpConfFile)
        else:
            self.exabgpConfFile = '%s/%s.conf' % (runDir, name)      
            self.asNum = asNum
            self.neighbors = neighbors
            self.generateConfig()
            self.bgp = BGP(config_file = self.exabgpConfFile)
            # print self.bgp.get_local_ips()

        self.config()
        self.configBGP(allowas_in, maximum_paths, relax)
        self.add_protocol(self.bgp)

    def config(self):
        for p in self.intfDict:
            port = self.intfDict[p][0]
            self.add_port(port = port['id'], eth_addr = port['mac'], ip = port['ipAddrs'][0],
               netmask = port['netmask'])

    def generateConfig(self):
        self.generateExaBGP()
     
    def generateExaBGP(self):
        configFile = open(self.exabgpConfFile, 'w+')

        def writeLine(indent, line):
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


        writeLine(0, 'process client{')
        writeLine(2, 'run /usr/bin/python /home/vagrant/horse/python/router_client.py %s;' % self.name)
        writeLine(2, 'encoder json;\n}\n')

        writeLine(0, 'template {')
        writeLine(2, 'neighbor router {')
        writeLine(4, 'family {')
        writeLine(6, 'ipv4 unicast;')
        writeLine(4, '}')
        writeLine(4, 'api speaking {')
        writeLine(6, 'processes [client];')
        writeLine(6, 'neighbor-changes;')
        writeLine(6, 'receive {')
        writeLine(8, 'parsed;')
        writeLine(8, 'update;')
        writeLine(6, '}')
        writeLine(4, '}')
        writeLine(4, 'local-as %s;' % self.asNum)
        writeLine(4, 'manual-eor true;')
        writeLine(2, '}')
        writeLine(0, '}\n')

        router_id = getRouterId(self.intfDict)
        for neighbor in self.neighbors:
            writeLine(0, 'neighbor %s {' % neighbor['address'])
            writeLine(2, 'inherit router;')
            writeLine(2, 'peer-as %s;' % neighbor['as'])
            writeLine(2, 'router-id %s;' % router_id)
            writeLine(2, 'local-address %s;' % neighbor['local-address'] )
            writeLine(2, 'group-updates false;')
            writeLine(0, '}\n')

        configFile.close()
        
    def configBGP(self, allowas_in, maximum_paths, relax):
        self.bgp.set_maximum_paths(maximum_paths) 
        self.bgp.set_relaxed_maximum_paths(relax)
        self.bgp.set_allowas_in(allowas_in)

        for route in self.routes:
            self.bgp.add_advertised_prefix(route)

cdef class Host:
    cdef host* _host_ptr
    cdef int exec_id
    cdef object ports # Quick workaround to get ips

    def __cinit__(self, name):
        self._host_ptr = host_new()
        # It needs to be improved when number of apps grow
        host_add_app(self._host_ptr, 1)  #PING
        host_add_app(self._host_ptr, 17) #UDP
        self.exec_id = 1
        self.name = name
        self.ports = []

    def add_port(self, port, eth_addr, ip = None, 
                netmask = None, max_speed = 1000000, cur_speed = 10000):
        # TODO: Add mac conversion to utils...
        mac = eth_addr.replace(':', '').decode('hex')
        cdef uint8_t *c_eth_addr = mac
        host_add_port(self._host_ptr, port, c_eth_addr, max_speed, cur_speed)
        self.ports.append(Port(port, eth_addr, ip, netmask, max_speed, cur_speed))
        if ip != None and netmask != None:
            int_ip = ip2int(ip)
            int_nm = ip2int(netmask)
            host_set_intf_ipv4(self._host_ptr, port, int_ip, int_nm)


    def get_ports(self):
        return self.ports

    def set_default_gw(self, ip, port):
        int_ip = ip2int(ip)
        host_set_default_gw(self._host_ptr, int_ip, port)

    def ping(self, dst, start_time = 0):
        cdef uint32_t ip
        ip = ip2int(dst)
        host_add_app_exec(self._host_ptr, self.exec_id, 1, 1, start_time, <void*> &ip, sizeof(int))
        self.exec_id += 1

    # Rate in Mbps, duration and interval in seconds
    def udp(self, dst, start_time, duration = 60, rate = 10, dst_port = 5001, src_port = random.randint(5002, 65000)):
        cdef uint32_t ip
        cdef raw_udp_args args
        ip = ip2int(dst)
        args.rate = rate * 1000000 #Mbits to bits
        args.ip_dst = ip
        args.dst_port = dst_port
        args.src_port = src_port 
        host_add_app_exec(self._host_ptr, self.exec_id, 17, duration, start_time, <void*> &args, sizeof(raw_udp_args))
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

    def set_log_level(self, level):
        sim_config_set_log_level(level)

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
        if "name" in kwargs:
            self.node_info[kwargs["name"]] = kwargs
            self.nodes[kwargs["name"]] = Node
        else:
            self.nodes[Node.name] = Node
    
        if isinstance(Node, SDNSwitch):
            dp = <SDNSwitch> Node
            topology_add_datapath(self._topo_ptr, dp._dp_ptr)
        elif isinstance (Node, Host):
            h = <Host> Node
            topology_add_host(self._topo_ptr, h._host_ptr)
        elif isinstance (Node, Router):
            r = <Router> Node
            topology_add_router(self._topo_ptr, r._router_ptr)

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

cdef class  LogLevels: #logging 
    LOG_TRACE = 0 
    LOG_DEBUG = 1
    LOG_INFO = 2 
    LOG_WARN = 3
    LOG_ERROR = 4
    LOG_FATAL = 5 

cdef class Sim:
    cdef Topology topo
    cdef SimConfig config
    # Default interval is 0.5 seconds
    def __cinit__(self, topo, mode = 1, end_time = UINT64_MAX, 
                  ctrl_interval = 10000, log_level = None):
        self.topo = <Topology> topo
        # self._topo_ptr = topo._topo_ptr
        # print topo.topo_ptr()
        self.config =  SimConfig()
        self.config.set_mode(mode)
        self.config.set_end_time(end_time)
        self.config.set_ctrl_idle_interval(ctrl_interval)
        if log_level != None:
            self.config.set_log_level(log_level)
        else:
            self.config.set_log_level(LogLevels.LOG_FATAL + 1)

    def start(self):
        topo = <Topology> self.topo
        config = <SimConfig> self.config    
        start(topo._topo_ptr, config._config_ptr)     