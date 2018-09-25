#!/usr/bin/env python
'''@package dctopo

Data center network topology creation.

@author Eder Leao Fernandes (e.leao@qmul.ac.uk)

This package includes code to create and draw networks with a regular,
repeated structure.  The main class is StructuredTopo, which augments the
standard Horse Topology object with layer metadata plus convenience functions 
to enumerate up, down, and layer edges.

Based on the code from @author Brandon Heller (brandonh@stanford.edu)
'''
import sys
from random import randint

# path = "/home/vagrant/horse/horse-python/"
# if path not in sys.path:
#     sys.path.append(path)

from horse.horse import *
from horse.router import BGPNeighbor, BGP

def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255)
)

def replace_ip(network, ip):
    net, subnet = network.split('/')
    gw = net.split('.')
    gw[3] = ip
    gw = '.'.join(gw)
    gw = '/'.join([gw, subnet])
    return gw

PORT_BASE = 1  # starting index for OpenFlow switch ports

class NodeID(object):
    '''Topo node identifier.'''

    def __init__(self, dpid = None):
        '''Init.

        @param dpid dpid
        '''
        # DPID-compatible hashable identifier: opaque 64-bit unsigned int
        self.dpid = dpid

    def __str__(self):
        '''String conversion.

        @return str dpid as string
        '''
        return str(self.dpid)

    def name_str(self):
        '''Name conversion.

        @return name name as string
        '''
        return str(self.dpid)

    def ip_str(self):
        '''Name conversion.

        @return ip ip as string
        '''
        hi = (self.dpid & 0xff0000) >> 16
        mid = (self.dpid & 0xff00) >> 8
        lo = self.dpid & 0xff
        return "10.%i.%i.%i" % (hi, mid, lo)


class StructuredNodeSpec(object):
    '''Layer-specific vertex metadata for a StructuredTopo graph.'''

    def __init__(self, up_total, down_total, up_speed, down_speed,
                 type_str = None):
        '''Init.

        @param up_total number of up links
        @param down_total number of down links
        @param up_speed speed in Gbps of up links
        @param down_speed speed in Gbps of down links
        @param type_str string; model of switch or server
        '''
        self.up_total = up_total
        self.down_total = down_total
        self.up_speed = up_speed
        self.down_speed = down_speed
        self.type_str = type_str


class StructuredEdgeSpec(object):
    '''Static edge metadata for a StructuredTopo graph.'''

    def __init__(self, speed = 1.0):
        '''Init.

        @param speed bandwidth in Gbps
        '''
        self.speed = speed


class StructuredTopo(Topology):
    '''Data center network representation for structured multi-trees.'''

    def __init__(self, node_specs, edge_specs):
        '''Create StructuredTopo object.

        @param node_specs list of StructuredNodeSpec objects, one per layer
        @param edge_specs list of StructuredEdgeSpec objects for down-links,
            one per layer
        '''
        super(StructuredTopo, self).__init__()
        self.node_specs = node_specs
        self.edge_specs = edge_specs
        self.g = {}

    def def_nopts(self, layer):
        '''Return default dict for a structured topo.

        @param layer layer of node
        @return d dict with layer key/val pair, plus anything else (later)
        '''
        return {'layer': layer}

    def routers(self):
        return [ n for n in self.nodes if isinstance(self.nodes[n], Router)]

    def hosts(self):
        return [ n for n in self.nodes if isinstance(self.nodes[n], Host)]

    def layer(self, name):
        '''Return layer of a node

        @param name name of switch
        @return layer layer of switch
        '''
        info = self.node_info[name]
        return info['layer']

    def isPortUp(self, port):
        ''' Returns whether port is facing up or down

        @param port port number
        @return portUp boolean is port facing up?
        '''
        return port % 2 == PORT_BASE

    def layer_nodes(self, layer):
        '''Return nodes at a provided layer.

        @param layer layer
        @return names list of names
        '''
        def is_layer(name):
            '''Returns true if node is at layer.'''
            return self.layer(name) == layer

        nodes = [n for n in self.node_info if is_layer(n)]
        return nodes

    def up_nodes(self, name):
        '''Return edges one layer higher (closer to core).

        @param name name

        @return names list of names
        '''
        layer = self.layer(name) - 1
        # not very efficient but should do it for now
        nodes = [n for n in self.g[name] if self.layer(n) == layer]
        return nodes

    def down_nodes(self, name):
        '''Return edges one layer higher (closer to hosts).

        @param name name
        @return names list of names
        '''
        layer = self.layer(name) + 1
        nodes = [n for n in self.g[name] if self.layer(n) == layer]
        return nodes

    def up_edges(self, name):
        '''Return edges one layer higher (closer to core).

        @param name name
        @return up_edges list of name pairs
        '''
        edges = [(name, n) for n in self.up_nodes(name)]
        return edges

    def down_edges(self, name):
        '''Return edges one layer lower (closer to hosts).

        @param name name
        @return down_edges list of name pairs
        '''
        edges = [(name, n) for n in self.down_nodes(name)]
        return edges


class HorseBGPFatTreeTopo(StructuredTopo):
    '''Three-layer homogeneous Fat Tree.

    From "A scalable, commodity data center network architecture, M. Fares et
    al. SIGCOMM 2008."
    '''
    LAYER_CORE = 0
    LAYER_AGG = 1
    LAYER_EDGE = 2
    LAYER_HOST = 3

    class FatTreeNodeID(NodeID):
        '''Fat Tree-specific node.'''

        def __init__(self, pod = 0, sw = 0, host = 0, dpid = None, name = None):
            '''Create FatTreeNodeID object from custom params.

            Either (pod, sw, host) or dpid must be passed in.

            @param pod pod ID
            @param sw switch ID
            @param host host ID
            @param dpid optional dpid
            @param name optional name
            '''
            if dpid:
                self.pod = (dpid & 0xff0000) >> 16
                self.sw = (dpid & 0xff00) >> 8
                self.host = (dpid & 0xff)
                self.dpid = dpid
            elif name:
                pod, sw, host = [int(s) for s in name.split('_')]
                self.pod = pod
                self.sw = sw
                self.host = host
                self.dpid = (pod << 16) + (sw << 8) + host
            else:
                self.pod = pod
                self.sw = sw
                self.host = host
                self.dpid = (pod << 16) + (sw << 8) + host

        def __str__(self):
            return "(%i, %i, %i)" % (self.pod, self.sw, self.host)

        def name_str(self):
            '''Return name string'''
            return "%i_%i_%i" % (self.pod, self.sw, self.host)

        def mac_str(self):
            '''Return MAC string'''
            return "00:00:00:%02x:%02x:%02x" % (self.pod, self.sw, self.host)

        def ip_str(self):
            '''Return IP string'''
            return "10.%i.%i.%i" % (self.pod, self.sw, self.host)
    """
    def _add_port(self, src, dst):
        '''Generate port mapping for new edge.

        Since Node IDs are assumed hierarchical and unique, we don't need to
        maintain a port mapping.  Instead, compute port values directly from
        node IDs and topology knowledge, statelessly, for calls to self.port.

        @param src source switch DPID
        @param dst destination switch DPID
        '''
        pass
    """
    def def_nopts(self, layer, name = None):
        '''Return default dict for a FatTree topo.

        @param layer layer of node
        @param name name of node
        @return d dict with layer key/val pair, plus anything else (later)
        '''
        d = {'layer': layer}
        if name:
            id = self.id_gen(name = name)
            d.update({'name': name})
            # For hosts only, set the IP
            if layer == self.LAYER_HOST:
              d.update({'ip': id.ip_str()})
              d.update({'mac': id.mac_str()})
            d.update({'dpid': "%016x" % id.dpid})
        return d

    # Ugly port handling :/
    def __init__(self, k = 4, speed = 1.0):
        '''Init.

        @param k switch degree
        @param speed bandwidth in Gbps
        '''
        print "Creating Horse Topo"
        core = StructuredNodeSpec(0, k, None, speed, type_str = 'core')
        agg = StructuredNodeSpec(k / 2, k / 2, speed, speed, type_str = 'agg')
        edge = StructuredNodeSpec(k / 2, k / 2, speed, speed,
                                  type_str = 'edge')
        host = StructuredNodeSpec(1, 0, speed, None, type_str = 'host')
        node_specs = [core, agg, edge, host]
        edge_specs = [StructuredEdgeSpec(speed)] * 3
        super(HorseBGPFatTreeTopo, self).__init__(node_specs, edge_specs)

        self.k = k
        self.id_gen = HorseBGPFatTreeTopo.FatTreeNodeID
        self.numPods = k
        self.aggPerPod = k / 2

        pods = range(0, k)
        core_sws = range(1, k / 2 + 1)
        agg_sws = range(k / 2, k)
        edge_sws = range(0, k / 2)
        hosts = range(2, k / 2 + 2)

        edge_asn = 65001
        agg_asn = 64601
        core_asn = 65534
        nodes = {}
        net_idx = 1
        layer_edge_ip = 1
        edge_agg_ip = 1
        core_agg_ip  = 1
        agg_sub_net = 0
        edge_aggr_sub = 0
        links = {}
        for p in pods:
            agg_objs = {}
            for e in edge_sws:
                edge_id = self.id_gen(p, e, 1).name_str()
                edge_opts = self.def_nopts(self.LAYER_EDGE, edge_id)
                edge_networks = []
                edge_intfs = {}
                edge_neighbors = []
                nodes[edge_id] = {}
                nodes[edge_id]['asn'] = edge_asn
                nodes[edge_id]['opts'] = edge_opts
                nodes[edge_id]['type'] = self.LAYER_EDGE

                for a in agg_sws:
                    agg_id = self.id_gen(p, a, 1).name_str()
                    if not agg_id in nodes: 
                        nodes[agg_id] = {}
                        nodes[agg_id]['intfs'] = {}
                        nodes[agg_id]['neighbors'] = []
                        nodes[agg_id]['asn'] = agg_asn
                        nodes[agg_id]['networks'] = []
                        nodes[agg_id]['type'] = self.LAYER_AGG
                        agg_opts = self.def_nopts(self.LAYER_AGG, agg_id)
                        nodes[agg_id]['opts'] = agg_opts

                    src, dst = self.port(edge_id, agg_id, self.LAYER_EDGE, self.LAYER_AGG)
                    edge_eth = [{'ipAddrs': ["10.1.%s.%s" % (edge_aggr_sub, 1)], 'id':src, 'netmask':"255.255.255.0", "mac":rand_mac() }]
                    agg_eth = [{'ipAddrs': ["10.1.%s.%s" % (edge_aggr_sub, 2)],'id':dst, 'netmask':"255.255.255.0", "mac":rand_mac() }]
                    edge_intfs[edge_id + '-eth' + str(src)] = edge_eth
                    nodes[agg_id]['intfs'][agg_id + '-eth' + str(dst)] = agg_eth
                    
                    edge_neighbor_ip =  agg_eth[0]['ipAddrs'][0].split('/')[0]
                    agg_neighbor_ip = edge_eth[0]['ipAddrs'][0].split('/')[0]

                    edge_neighbors.append ( BGPNeighbor(agg_asn,
                                            edge_neighbor_ip, 
                                            local_ip=agg_neighbor_ip)) 
                    nodes[agg_id]['neighbors'].append( BGPNeighbor(edge_asn,
                                                    agg_neighbor_ip, 
                                                    local_ip=edge_neighbor_ip) )    
                    edge_aggr_sub += 1
                    # edge_agg_ip += 2
                    links[(edge_id, agg_id)] = (src, dst)

                for h in hosts:
                    edge_ip = 129
                    net = '192.168.%s.0/24' % net_idx
                    edge_networks.append(net)
                    host_id = self.id_gen(p, e, h).name_str()
                    nodes[host_id] = {}
                    nodes[host_id]['ip'] = [replace_ip(net, '1')] 
                    host_opts = self.def_nopts(self.LAYER_HOST, host_id)
                    nodes[host_id]['opts'] = host_opts
                    nodes[host_id]['gateway'] = replace_ip(net, '254').split('/')[0]
                    nodes[host_id]['type'] = self.LAYER_HOST
                    src, dst = self.port(edge_id, host_id, self.LAYER_EDGE, self.LAYER_HOST)
                    ip = replace_ip(net, '254').split('/')[0]
                    eth = [{'ipAddrs': [ip], 'id':src, 'netmask':"255.255.255.0", "mac":rand_mac()}]  # ex.: 100.0.0.254
                    edge_intfs[edge_id + '-eth' + str(src)] = eth
                    net_idx += 1
                    edge_ip += 1
                    links[(edge_id, host_id)] = (src, dst) #self.addLink(host_id, edge_id, port1=dst, port2=src)
                nodes[edge_id]['intfs'] = edge_intfs
                nodes[edge_id]['neighbors'] = edge_neighbors
                nodes[edge_id]['networks'] = edge_networks
                edge_asn += 1
                
            for a in agg_sws:
                core_agg = 1
                agg_id = self.id_gen(p, a, 1).name_str()
                c_index = a - k / 2 + 1
                for c in core_sws:
                    core_id = self.id_gen(k, c_index, c).name_str()
                    if core_id not in nodes:
                        nodes[core_id] = {}
                        nodes[core_id]['intfs'] = {}
                        nodes[core_id]['networks'] = []
                        nodes[core_id]['neighbors'] = []
                        nodes[core_id]['asn'] = core_asn
                        nodes[core_id]['type'] = self.LAYER_CORE
                        core_opts = self.def_nopts(self.LAYER_CORE, core_id)
                        nodes[core_id]['opts'] = core_opts
                    src, dst = self.port(core_id, agg_id, self.LAYER_CORE, self.LAYER_AGG)
                    agg_eth = [{'ipAddrs': ["20.1.%s.%s" % (agg_sub_net, 1)], 'id':dst, 'netmask':"255.255.255.0", "mac":rand_mac() }]
                    core_eth = [{'ipAddrs': ["20.1.%s.%s" % (agg_sub_net, 2)], 'id':src, 'netmask':"255.255.255.0", "mac":rand_mac()}]
                    nodes[agg_id]['intfs'][agg_id + '-eth' + str(dst)] = agg_eth
                    nodes[core_id]['intfs'][core_id + '-eth' + str(src)] = core_eth
                    core_neigh_ip = agg_eth[0]['ipAddrs'][0].split('/')[0]
                    agg_neigh_ip = core_eth[0]['ipAddrs'][0].split('/')[0]
                    nodes[core_id]['neighbors'].append ( 
                                            BGPNeighbor(nodes[agg_id]['asn'],
                                                        core_neigh_ip,
                                                        local_ip=agg_neigh_ip) )
                    nodes[agg_id]['neighbors'].append( 
                                            BGPNeighbor(core_asn,agg_neigh_ip,
                                                    local_ip=core_neigh_ip) )
                    links[(core_id, agg_id)] = (src, dst) # self.addLink(core_id, agg_id, port1=src, port2=dst)
                    # core_agg += 2
                    agg_sub_net += 1
            agg_asn += 1

        node_objects = {}
        for node in nodes:
            created = None
            if 'asn' in nodes[node]:
                router_id = max([nodes[node]['intfs'][x][0]['ipAddrs'][0] for x in nodes[node]['intfs']])
                if nodes[node]['type'] == self.LAYER_EDGE or nodes[node]['type'] == self.LAYER_AGG: 
                    bgp = [BGP(asn = nodes[node]['asn'],
                              neighbors = nodes[node]['neighbors'],
                              networks = nodes[node]['networks'],
                              allowas_in = False,
                              relax = True, maximum_paths = self.k/2, 
                              router_id=router_id)]

                    created = Router(node, daemon = "quagga", *bgp)
                    
                else:
                    bgp = [BGP(asn=nodes[node]['asn'],
                              neighbors=nodes[node]['neighbors'],
                              networks=nodes[node]["networks"],
                              allowas_in=True,
                              router_id=router_id)]
                    created = Router(node, daemon = "quagga", *bgp)
                # for intf in nodes[node]['intfs']:
                for intf in nodes[node]['intfs']:
                    i = nodes[node]['intfs'][intf][0]
                    created.add_port(port=i['id'], eth_addr=i['mac'],
                                     ip=i['ipAddrs'][0], netmask=i['netmask'])
            else:
                created = Host(node)

            if created:
                self.add_node(created)
                node_objects[node] =  created

        for link in links:
            src_name, dst_name = link[0], link[1]
            src_port, dst_port = links[(src_name, dst_name)][0], links[(src_name, dst_name)][1]
            src_obj =  node_objects[src_name]
            dst_obj = node_objects[dst_name]
            if nodes[dst_name]['type'] == self.LAYER_HOST:
                ip = nodes[dst_name]['ip'][0].split('/')[0]
                dst_obj.add_port(port = dst_port, eth_addr = rand_mac(),
                          ip = ip, netmask = "255.255.255.0")
                dst_obj.set_default_gw(nodes[dst_name]['gateway'], dst_port)
            self.add_link(src_obj, dst_obj, src_port, dst_port)

                    
    def port(self, src, dst, src_layer, dst_layer):
        '''Get port number (optional)

        Note that the topological significance of DPIDs in FatTreeTopo enables
        this function to be implemented statelessly.

        @param src source switch DPID
        @param dst destination switch DPID
        @return tuple (src_port, dst_port):
            src_port: port on source switch leading to the destination switch
            dst_port: port on destination switch leading to the source switch
        '''
        # src_layer = self.layer(src)
        # dst_layer = self.layer(dst)

        src_id = self.id_gen(name = src)
        dst_id = self.id_gen(name = dst)

        LAYER_CORE = 0
        LAYER_AGG = 1
        LAYER_EDGE = 2
        LAYER_HOST = 3

        if src_layer == LAYER_HOST and dst_layer == LAYER_EDGE:
            src_port = 0
            dst_port = (src_id.host - 2) * 2 + 1
        elif src_layer == LAYER_EDGE and dst_layer == LAYER_CORE:
            src_port = (dst_id.sw - 2) * 2
            dst_port = src_id.pod
        elif src_layer == LAYER_EDGE and dst_layer == LAYER_AGG:
            src_port = (dst_id.sw - self.k / 2) * 2
            dst_port = src_id.sw * 2 + 1
        elif src_layer == LAYER_AGG and dst_layer == LAYER_CORE:
            src_port = (dst_id.host - 1) * 2
            dst_port = src_id.pod
        elif src_layer == LAYER_CORE and dst_layer == LAYER_AGG:
            src_port = dst_id.pod
            dst_port = (src_id.host - 1) * 2
        elif src_layer == LAYER_AGG and dst_layer == LAYER_EDGE:
            src_port = dst_id.sw * 2 + 1
            dst_port = (src_id.sw - self.k / 2) * 2
        elif src_layer == LAYER_CORE and dst_layer == LAYER_EDGE:
            src_port = dst_id.pod
            dst_port = (src_id.sw - 2) * 2
        elif src_layer == LAYER_EDGE and dst_layer == LAYER_HOST:
            src_port = (dst_id.host - 2) * 2 + 1
            dst_port = 0
        else:
            raise Exception("Could not find port leading to given dst switch")

        # Shift by one; as of v0.9, OpenFlow ports are 1-indexed.
        if src_layer != LAYER_HOST:
            src_port += 1
        if dst_layer != LAYER_HOST:
            dst_port += 1

        return (src_port, dst_port)


if __name__ == '__main__':
    HorseBGPFatTreeTopo(k=2)
