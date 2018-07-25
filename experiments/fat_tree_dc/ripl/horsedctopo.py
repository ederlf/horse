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
# path = "/home/vagrant/horse/python"
# if path not in sys.path:
#     sys.path.append(path)

from horse.horse import *

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

    def switches(self):
        return [ n for n in self.nodes if isinstance(self.nodes[n], SDNSwitch)]

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


class FatTreeTopo(StructuredTopo):
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
        super(FatTreeTopo, self).__init__(node_specs, edge_specs)

        self.k = k
        self.id_gen = FatTreeTopo.FatTreeNodeID
        self.numPods = k
        self.aggPerPod = k / 2

        pods = range(0, k)
        core_sws = range(1, k / 2 + 1)
        agg_sws = range(k / 2, k)
        edge_sws = range(0, k / 2)
        hosts = range(2, k / 2 + 2)
        for p in pods:
            agg_objs = {}
            for e in edge_sws:
                edge_id = self.id_gen(p, e, 1).name_str()
                edge_opts = self.def_nopts(self.LAYER_EDGE, edge_id)
                sw = SDNSwitch(edge_id, long(edge_opts["dpid"], 16))
                self.g[edge_id] = []
                self.add_node(sw, **edge_opts)

                for h in hosts:
                    host_id = self.id_gen(p, e, h).name_str()
                    host_opts = self.def_nopts(self.LAYER_HOST, host_id)
                    host = Host(host_id)
                    self.add_node(host, **host_opts)
                    self.g[host_id] = []
                    self.g[edge_id].append(host_id)
                    self.g[host_id].append(edge_id)
                    src, dst = self.port(edge_id, host_id)
                    sw.add_port(port = src, 
                                eth_addr = "00:00:00:00:00:00")
                    host.add_port(port = dst, eth_addr = host_opts["mac"],
                                  ip = host_opts["ip"], netmask = "255.0.0.0")
                    self.add_link(sw, host, src, dst, latency = 0)

                for a in agg_sws:
                    agg_id = self.id_gen(p, a, 1).name_str()
                    agg_opts = self.def_nopts(self.LAYER_AGG, agg_id)
                    
                    if agg_id in self.switches():
                        agg_sw = self.nodes[agg_id]
                    else:
                        agg_sw = SDNSwitch(agg_id, long(agg_opts["dpid"], 16))
                        self.add_node(agg_sw, **agg_opts)
                        self.g[agg_id] = []
                    self.g[edge_id].append(agg_id)
                    self.g[agg_id].append(edge_id)
                    src, dst = self.port(edge_id, agg_id)
                    sw.add_port(port = src, 
                                eth_addr = "00:00:00:00:00:00")
                    agg_sw.add_port(port = dst, 
                                        eth_addr = "00:00:00:00:00:00")
                    self.add_link(sw, agg_sw, src, dst)

            for a in agg_sws:
                agg_id = self.id_gen(p, a, 1).name_str()
                c_index = a - k / 2 + 1
                for c in core_sws:
                    core_id = self.id_gen(k, c_index, c).name_str()
                    core_opts = self.def_nopts(self.LAYER_CORE, core_id)
                    if core_id not in self.switches():
                        core_sw = SDNSwitch(core_id, long(core_opts["dpid"], 16))
                        self.add_node(core_sw, **core_opts)
                        self.g[core_id] = []
                    else:
                        core_sw = self.nodes[core_id]
                    self.g[agg_id].append(core_id)
                    self.g[core_id].append(agg_id)  
                    agg_sw = self.nodes[agg_id]
                    src, dst = self.port(core_id, agg_id)
                    core_sw.add_port(port = src, eth_addr = "00:00:00:00:00:00")
                    agg_sw.add_port(port = dst, 
                                    eth_addr = "00:00:00:00:00:00")
                    self.add_link(core_sw, agg_sw, src, dst)
                    
    def port(self, src, dst):
        '''Get port number (optional)

        Note that the topological significance of DPIDs in FatTreeTopo enables
        this function to be implemented statelessly.

        @param src source switch DPID
        @param dst destination switch DPID
        @return tuple (src_port, dst_port):
            src_port: port on source switch leading to the destination switch
            dst_port: port on destination switch leading to the source switch
        '''
        src_layer = self.layer(src)
        dst_layer = self.layer(dst)

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

# topo = FatTreeTopo(k=int(sys.argv[1]))

# hosts = topo.hosts()
# print len(topo.layer_nodes(topo.LAYER_EDGE))
# print len(topo.switches())
# print len(hosts)
#edge = topo.up_nodes(h)

#print edge[0], h, topo.port(edge[0], h)
#print topo.node_info[h]["mac"]
# time = 10000000
# n = 0
# for i, h in enumerate(hosts):
#     host1 = topo.nodes[h]
#     for h2 in hosts[1:]:
#         if h != h2:
#             h2_ip = topo.node_info[h2]["ip"]
#             host1.udp(h2_ip, time, rate= 10)
#             n += 1
            # time += 1000000
            # host1.ping(h2_ip, time)
    #         host1.ping(h2_ip, time)
    #         time += 1000000
    #         host1.ping(h2_ip, time)
    #         time += 1000000
    #         host1.ping(h2_ip, time)
            # break
    # break
# print "Nr of flow %s" % n
# end_time = time + 60 * 1000000  
# # print end_time
# sim = Sim(topo, ctrl_interval = 300000, end_time = end_time)
# sim.start()
