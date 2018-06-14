"""
RipL+Ryu.  As simple a data center controller as possible.
"""

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.controller import dpset
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet
from ryu.lib.packet import arp
from ryu.lib.packet import ipv4
from ryu.lib.packet import tcp
from ryu.lib.packet import udp
from ryu.lib.packet import ether_types
from ripl.mn import topos
from riplryu.util import buildTopo, getRouting, dpidToStr
from ryu import cfg
CONF = cfg.CONF
topo = CONF['ripl']['topo']
rt = CONF['ripl']['routing']

class RipLRyu(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    _CONTEXTS = {
        'dpset': dpset.DPSet,
    }


    def init(self):
        self.pktin = 0
        self.switches = {}  # Switches seen: [dpid] -> Switch
        # self.t = t  # Master Topo object, passed in and never modified.
        # self.r = r  # Master Routing object, passed in and reused.
        self.macTable = {}  # [mac] -> (dpid, port)
        self.ip_mac = {}
        self.topo = buildTopo(topo, topos)
        self.routing = getRouting(rt, self.topo)
        self.all_switches_up = False 
        hosts = self.topo.hosts()
        self.paths = []
        for h in hosts:
            edge = self.topo.up_nodes(h)
            src = self.topo.id_gen(name = h).mac_str()
            print "Registering %s" % src
            port = self.topo.port(edge[0], h)
            dpid = self.topo.id_gen(name = edge[0]).dpid
            self.macTable[src] = (dpid, port[0])
            ip = self.topo.id_gen(name = h).ip_str()
            self.ip_mac[ip] = src
        print self.macTable

    def __init__(self, *args, **kwargs):
        super(RipLRyu, self).__init__(*args, **kwargs)
        self.init()
        self.dpset = kwargs['dpset']

    @set_ev_cls(dpset.EventDP, dpset.DPSET_EV_DISPATCHER)
    def handler_datapath(self, ev):
        if not ev.enter:
            self.init()

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        sw = self.switches.get(datapath.id)
        sw_str = dpidToStr(datapath.id)
        self.logger.info("Saw switch come up: %s", sw_str)
        name_str = self.topo.id_gen(dpid = datapath.id).name_str()
        if name_str not in self.topo.switches():
          self.logger.warn("Ignoring unknown switch %s" % sw_str)
          return
        if sw is None:
          self.logger.info("Added fresh switch %s" % sw_str)
          self.switches[datapath.id] = datapath
        else:
          self.logger.info("Odd - already saw switch %s come up" % sw_str)  

        # install table-miss flow entry
        #
        # We specify NO BUFFER to max_len of the output action due to
        # OVS bug. At this moment, if we specify a lesser number, e.g.,
        # 128, OVS will send Packet-In with invalid buffer_id and
        # truncated packet data. In that case, we cannot output packets
        # correctly.  The bug has been fixed in OVS v2.1.0.
        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions, default = True)
        if len(self.switches) == len(self.topo.switches()):
          self.logger.info("Woo!  All switches up")
          self.all_switches_up = True

    def add_flow(self, datapath, priority, match, actions, buffer_id=None, default = False):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]
        idle_timeout = 0 #10
        hard_timeout = 0 #120
        if default:
            priority = 0
            idle_timeout = 0
            hard_timeout = 0

        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                    priority=priority, match=match, 
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority, 
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    match=match, instructions=inst)
        # print mod
        datapath.send_msg(mod)

    def _raw_dpids(self, arr):
        "Convert a list of name strings (from Topo object) to numbers."
        return [self.topo.id_gen(name = a).dpid for a in arr]

    def _match_from_packet(self, datapath, pkt, in_port = None):
        fields = {} 
        
        if in_port is not None:
            fields["in_port"] = in_port
        eth = pkt.get_protocols(ethernet.ethernet)[0]
        fields["eth_type"] = eth.ethertype
        fields["eth_src"] = eth.src
        fields["eth_dst"] = eth.dst
        parp = pkt.get_protocol(arp.arp)
        if parp:
          fields["arp_op"] = parp.opcode
          fields["arp_spa"] = parp.src_ip
          fields["arp_tpa"] = parp.dst_ip
          fields["arp_tha"] = parp.dst_mac
          fields["arp_sha"] = parp.src_mac
          return datapath.ofproto_parser.OFPMatch(**fields)
        ip = pkt.get_protocol(ipv4.ipv4)
        if ip:
            fields["ip_proto"] = ip.proto
            fields["ipv4_src"] = ip.src
            fields["ipv4_dst"] = ip.dst
            pkt_udp = pkt.get_protocol(udp.udp)
            pkt_tcp = pkt.get_protocol(tcp.tcp)
            if pkt_udp:
                fields["udp_src"] = pkt_udp.src_port
                fields["udp_dst"] = pkt_udp.dst_port
            elif pkt_tcp:
                fields["tcp_src"] = pkt_tcp.src_port
                fields["tcp_dst"] = pkt_tcp.dst_port
        if fields["eth_dst"] == "00:00:00:02:02:03":
            print fields
        return datapath.ofproto_parser.OFPMatch(**fields)

    def _install_path(self, dp, out_dpid, final_out_port, pkt):
        "Install entries on route between two switches."
        ofproto = dp.ofproto
        parser = dp.ofproto_parser
        in_name = self.topo.id_gen(dpid = dp.id).name_str()
        out_name = self.topo.id_gen(dpid = out_dpid).name_str()
        route = self.routing.get_route(in_name, out_name, pkt)
        # self.logger.info("route: %s" % route)
        if route :
            # print route
            self.paths.append(route)
            match = self._match_from_packet(dp, pkt)
            for i, node in enumerate(route):
                node_dpid = self.topo.id_gen(name = node).dpid
                if i < len(route) - 1:
                    next_node = route[i + 1]
                    out_port, next_in_port = self.topo.port(node, next_node)
                else:
                    out_port = final_out_port
                actions = [parser.OFPActionOutput(out_port)]
                self.add_flow(self.switches[node_dpid], 10, match, actions)


    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        if not self.all_switches_up:
            self.logger.info("Saw PacketIn %s before all switches were up - ignoring." % ev.msg)
            return
        msg = ev.msg
        datapath = msg.datapath
        dpid = datapath.id
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']
        topo = self.topo

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocols(ethernet.ethernet)[0]

        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            # ignore lldp packet
            return
        dst = eth.dst
        src = eth.src

        # Learn MAC address of the sender on every packet-in.
        if dst == "00:00:00:02:02:03":
            sw_name = topo.id_gen(dpid = dpid).name_str()
            print hex(dpid), in_port, sw_name 
        #self.macTable[src] = (dpid, in_port)
        # self.logger.info("mactable: %s" % self.macTable)

        if dst == "ff:ff:ff:ff:ff:ff":
            parp = pkt.get_protocol(arp.arp)
            dst = self.ip_mac[parp.dst_ip]

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data
          # Insert flow, deliver packet directly to destination.
        if dst in self.macTable:
            out_dpid, out_port = self.macTable[dst]
            print "Installing path"
            self._install_path(datapath, out_dpid, out_port, pkt)
            if dst == "00:00:00:02:02:03":
                self.logger.info("sending to entry in mactable: %s %s dst %s src %s" % (hex(out_dpid), out_port, dst, src))
            actions = [parser.OFPActionOutput(out_port)]
            out = parser.OFPPacketOut(datapath=self.switches[out_dpid], buffer_id=msg.buffer_id,in_port=ofproto.OFPP_CONTROLLER, actions=actions, data=data)
            self.switches[out_dpid].send_msg(out)
        elif src in self.macTable:
            print "Not found src %s dst %s" % (src, dst)
            # print self.macTable
        #       # Broadcast to every other edge output except the input on the input edge.
        #     # Hub behavior, baby!
        #     for sw in self._raw_dpids(topo.layer_nodes(topo.LAYER_EDGE)):
        #         print "Flooding " + dst
        #         ports = []
        #         sw_name = topo.id_gen(dpid = sw).name_str()
        #         for host in topo.down_nodes(sw_name):
        #             sw_port, host_port = topo.port(sw_name, host)
        #             if sw != dpid or (sw == dpid and in_port != sw_port):
        #                 ports.append(sw_port)
        #         # Send packet out each non-input host port
        #         # TODO: send one packet only.
        #         for port in ports:
        #             actions = [parser.OFPActionOutput(port)]
        #             out = parser.OFPPacketOut(datapath=self.switches[sw], buffer_id=msg.buffer_id,in_port=ofproto.OFPP_CONTROLLER, actions=actions, data=data)
        #             self.switches[sw].send_msg(out)

