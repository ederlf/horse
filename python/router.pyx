cimport router
import sys
import re
from msg import ip2int, int2ip, netmask2cidr
from horse import Router
import os
import json

cdef class QuaggaDaemon:
    cdef quagga_daemon* _qd_ptr

    def __cinit__(self, namespace, zebra_conf=None, bgpd_conf=None,
                  ospfd_conf=None, ospf6d_conf=None, ripd_conf = None, 
                  ripngd_conf = None):
        self._qd_ptr = quagga_daemon_new(namespace)
        if not zebra_conf:
            print "Please provide zebra file"
            sys.exit(0)
        set_quagga_daemon_zebra_file(self._qd_ptr, zebra_conf)
        if bgpd_conf:
            set_quagga_daemon_bgpd_file(self._qd_ptr, bgpd_conf)
        if ospfd_conf:
            set_quagga_daemon_ospfd_file(self._qd_ptr, ospfd_conf)
        if ospf6d_conf:
            set_quagga_daemon_ospf6d_file(self._qd_ptr, ospf6d_conf)
        if ripd_conf:
            set_quagga_daemon_ripd_file(self._qd_ptr, ripd_conf)
        if ripngd_conf:
            set_quagga_daemon_ripngd_file(self._qd_ptr, ripngd_conf)

cdef class ExaBGPDaemon:
    cdef exabgp_daemon* _exa_ptr
    cdef neighbors
    cdef local_ips
    cdef prefixes
    cdef asn
    cdef router_id
    cdef max_paths # Maximum number of allowed paths. All attributes should be equal.
    cdef relax   # Allows load balancing among different ASes. Only AS path needs to match.
    cdef allowas_in

    def __cinit__(self, namespace, exabgp_conf):
        self._exa_ptr = exabgp_daemon_new(namespace)
        self.neighbors = {}
        self.prefixes = {}
        self.max_paths = 0
        self.relax = False
        self.router_id = 0
        self.allowas_in = False
        self.local_ips = []

        if exabgp_conf and os.path.isfile(exabgp_conf):
            set_exabgp_daemon_config_file(self._exa_ptr, exabgp_conf)
        else:
            print "No config file provided for BGP router"

    def add_ip(self, ip):
        exabgp_daemon_add_port_ip(self._exa_ptr, ip)

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

class ExaBGPRouter(Router):
    # TODO: Should just use the members of the BGP class

    # def __new__(cls, name):
    #     return super().__new__(cls, name)

    def __init__(self, name, intfDict,
                 asNum, neighbors, routes,
                 exabgpConfFile=None,
                 allowas_in = False,
                 relax = False,
                 maximum_paths= 0,
                 runDir='/tmp', *args, **kwargs):
        
        super(ExaBGPRouter, self).__init__(name)
        self.routes = routes
        self.intfDict = intfDict
        if exabgpConfFile is not None:
            self.exabgpConfFile = exabgpConfFile
        else:
            self.exabgpConfFile = '%s/%s.conf' % (runDir, name)      
            self.asNum = asNum
            self.neighbors = neighbors
            self.generateExaBGPconfig()
        self.daemon = ExaBGPDaemon(self, self.name, self.exabgpConfFile)
        self.write_auxiliary_config_file()
        self.parse_config_file()
        self.config()

    def parse_config_file(self):
        with file(self.exabgpConfFile, "r") as f:
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
                # bgp_set_router_id(self._bgp_ptr, ip2int(self.router_id))
            else:
                for ip in self.local_ips:
                    self.daemon.add_ip(ip)
                    int_ip = ip2int(ip)
                    if int_ip > self.router_id:
                        self.router_id = ip
                # bgp_set_router_id(self._bgp_ptr, ip2int(self.router_id))
            if asn:
                self.asn = asn[0][9:]
            for neighbor, asn in zip(ip, asys):
                n = neighbor[9:]
                asn = int(asn[8:])
                self.neighbors[n] = {}
                self.neighbors[n]["MED"] = 0
                self.neighbors[n]["ASN"] = asn

    def write_auxiliary_config_file(self):
        conf = {}
        conf["prefixes"] = self.prefixes
        conf["neighbors"] = self.neighbors
        conf["asn"] = self.asn
        conf["router_id"] = self.router_id
        conf["max_paths"] = self.max_paths
        conf["relax"] = self.relax
        conf["allowas_in"] = self.allowas_in
        with file("/tmp/conf-bgp.%s" % self.name, "w") as f:
            json.dump(conf, f)

    def config(self):
        for p in self.intfDict:
            port = self.intfDict[p][0]
            self.add_port(port = port['id'], eth_addr = port['mac'],
                          ip = port['ipAddrs'][0], 
                          netmask = port['netmask'])

    def generateExaBGP_config(self):
        configFile = open(self.exabgpConfFile, 'w+')

        writeLine(configFile, 0, 'process client{')
        writeLine(configFile, 2, 'run /usr/bin/python /home/vagrant/horse/python/router_client.py %s;' % self.name)
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
        writeLine(configFile, 4, 'local-as %s;' % self.asNum)
        writeLine(configFile, 4, 'manual-eor true;')
        writeLine(configFile, 2, '}')
        writeLine(configFile, 0, '}\n')

        router_id = getRouterId(self.intfDict)
        for neighbor in self.neighbors:
            writeLine(configFile, 0, 'neighbor %s {' % neighbor['address'])
            writeLine(configFile, 2, 'inherit router;')
            writeLine(configFile, 2, 'peer-as %s;' % neighbor['as'])
            writeLine(configFile, 2, 'router-id %s;' % router_id)
            writeLine(configFile, 2, 'local-address %s;' % neighbor['local-address'] )
            writeLine(configFile, 2, 'group-updates false;')
            writeLine(configFile, 0, '}\n')

        configFile.close()
        
#     def configBGP(self, allowas_in, maximum_paths, relax):
#         pass
        # self.bgp.set_maximum_paths(maximum_paths) 
        # self.bgp.set_relaxed_maximum_paths(relax)
        # self.bgp.set_allowas_in(allowas_in)

        # for route in self.routes:
        #     self.bgp.add_advertised_prefix(route)