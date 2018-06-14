"""
RipL+Ryu.  As simple a data center controller as possible.
"""

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.controller import dpset
from ryu.ofproto import ofproto_v1_3
from ryu.lib import hub
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
topo = CONF['hedera']['topo']
rt = CONF['hedera']['routing']
bw = CONF['hedera']['bw']

from demand import estimate_demands

class HederaRyu(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def init(self):
        self.mac_to_port = {}
        self.pktin = 0
        self.switches = {}  # Switches seen: [dpid] -> Switch
        # self.t = t  # Master Topo object, passed in and never modified.
        # self.r = r  # Master Routing object, passed in and reused.
        self.macTable = {}  # [mac] -> (dpid, port)
        self.topo = buildTopo(topo, topos)
        self.routing = getRouting(rt, self.topo)
        if bw is None:
            self.bw= 0.0 # Default 10 Mbps link
        else:
            #Gbps
            self.bw = float(bw) / 1000
        self.all_switches_up = False

        self.ethMapper = {}
        self.flowReserve = {} # flow['id'] -> reserveAmount (demand, path)
        self.paths = {} # flow['id'] -> (route, match, final_out_port)
        # For GlobalFirstFit
        self.bwReservation = {}

    def __init__(self, *args, **kwargs):
        super(HederaRyu, self).__init__(*args, **kwargs)
        self.init()
        # self.monitor_thread = hub.spawn(self._monitor)

    @set_ev_cls(dpset.EventDP, dpset.DPSET_EV_DISPATCHER)
    def handler_datapath(self, ev):
        if not ev.enter:
            self.init()

    def _reflow(self, flow_id, new_route):
        old_route, match, final_out_port = self.paths[flow_id]
        if new_route != old_route:
            print "Rerouting", flow_id, old_route, new_route
            for i in range(len(new_route) - 1, -1, -1):
                node = new_route[i]
                node_dpid = self.topo.id_gen(name = node).dpid
                if i < len(new_route) - 1:
                    next_node = new_route[i + 1]
                    out_port, next_in_port = self.topo.port(node, next_node)
                else:
                    out_port = final_out_port
                dp = self.switches[node_dpid]
                parser = dp.ofproto_parser
                ofproto = dp.ofproto
                actions = [parser.OFPActionOutput(out_port)]
                if i == (len(new_route) - 1):
                    self.add_flow(dp, 10, match, actions, 
                              cmd = ofproto.OFPFC_MODIFY_STRICT)
                else:
                    self.add_flow(dp, 10, match, actions) 
         
        self.paths[flow_id] = (new_route, match, final_out_port)


    def _status_requester(self):
        if self.all_switches_up:
            # Clear all
            self.flows = []
            self.flowQueryMsg = {} #set() # Store response to be received
            # Ask for outgoing flow from edge switches to hosts
            for sw_name in self.topo.layer_nodes(self.topo.LAYER_EDGE):
                connected_hosts = self.topo.down_nodes(sw_name)
                sw_dpid = self.topo.id_gen(name=sw_name).dpid
                self.flowQueryMsg[sw_dpid] = 0
                for host_name in connected_hosts:
                    sw_port, host_port = self.topo.port(sw_name, host_name)
                    self._request_stats(self.switches[sw_dpid], sw_port)
                    # print "request (sw, src_port) = (" + str(sw_dpid) + ", " + str(sw_port) + ")"
                    #self.flowQueryMsg.add((sw_dpid, sw_port))
                    # NOTE: sw_port doesn't match what we get back from openflow.
                    # So, the best we can do is to count it...
                    self.flowQueryMsg[sw_dpid] += 1

    def _request_stats(self, datapath, out_port):
        self.logger.debug('send stats request: %016x', datapath.id)
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        match = parser.OFPMatch(eth_type = 0x800, ip_proto = 6)
        req = parser.OFPFlowStatsRequest(datapath, match = match, out_port = out_port)
        datapath.send_msg(req)

    def _monitor(self):
        while True:
            self._status_requester()
            # 2 seconds might be too demanding...
            hub.sleep(10)


    @set_ev_cls(ofp_event.EventOFPExperimenter, MAIN_DISPATCHER)
    def experimenter_message_handler(self,ev):
        msg = ev.msg
        datapath = msg.datapath
        experimenter = msg.experimenter
        exp_type = msg.exp_type
        if experimenter == 0xF0A1F0A1:
            if exp_type == 0:
                 self._status_requester()

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

    def add_flow(self, datapath, priority, match, actions, buffer_id=None, default = False, cmd = ofproto_v1_3.OFPFC_ADD):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]
        idle_timeout = 0
        hard_timeout = 0 
        if default:
            priority = 0
            idle_timeout = 0
            hard_timeout = 0

        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                    priority=priority, match=match, command = cmd,
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority, 
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    match=match, command = cmd, instructions=inst)
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
        return datapath.ofproto_parser.OFPMatch(**fields)

    def _install_path(self, dp, out_dpid, final_out_port, pkt):
        "Install entries on route between two switches."
        ofproto = dp.ofproto
        parser = dp.ofproto_parser
        in_name = self.topo.id_gen(dpid = dp.id).name_str()
        out_name = self.topo.id_gen(dpid = out_dpid).name_str()
        route = self.routing.get_route(in_name, out_name, pkt)
        # self.logger.info("route: %s" % route)
        print route
        if route:
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
            # Parsing again, could be better if match fields could be used.
            ip = pkt.get_protocol(ipv4.ipv4)
            pkt_tcp = pkt.get_protocol(tcp.tcp)
            pkt_udp = pkt.get_protocol(udp.udp)
            if ip: 
                if pkt_tcp:
                    self.paths[(ip.src, ip.dst,
                    pkt_tcp.src_port, pkt_tcp.dst_port)] = (route, match, final_out_port)
                elif pkt_udp:
                    self.paths[(ip.src, ip.dst,
                    pkt_udp.src_port, pkt_udp.dst_port)] = (route, match, final_out_port)

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
        # print pkt
        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            # ignore lldp packet
            return
        dst = eth.dst
        src = eth.src

        # Learn MAC address of the sender on every packet-in.
        self.macTable[src] = (dpid, in_port)
        # self.logger.info("mactable: %s" % self.macTable)

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

          # Insert flow, deliver packet directly to destination.
        if dst in self.macTable:
            out_dpid, out_port = self.macTable[dst]
            self._install_path(datapath, out_dpid, out_port, pkt)
            # self.logger.info("sending to entry in mactable: %s %s" % (hex(out_dpid), out_port))
            actions = [parser.OFPActionOutput(out_port)]
            out = parser.OFPPacketOut(datapath=self.switches[out_dpid], buffer_id=msg.buffer_id,in_port=ofproto.OFPP_CONTROLLER, actions=actions, data=data)
            self.switches[out_dpid].send_msg(out)
        else:
              # Broadcast to every other edge output except the input on the input edge.
            # Hub behavior, baby!
            for sw in self._raw_dpids(topo.layer_nodes(topo.LAYER_EDGE)):
                ports = []
                sw_name = topo.id_gen(dpid = sw).name_str()
                for host in topo.down_nodes(sw_name):
                    sw_port, host_port = topo.port(sw_name, host)
                    if sw != dpid or (sw == dpid and in_port != sw_port):
                        ports.append(sw_port)
                # Send packet out each non-input host port
                # TODO: send one packet only.
                for port in ports:
                    actions = [parser.OFPActionOutput(port)]
                    out = parser.OFPPacketOut(datapath=self.switches[sw], buffer_id=msg.buffer_id,in_port=ofproto.OFPP_CONTROLLER, actions=actions, data=data)
                    self.switches[sw].send_msg(out)

    def _buildMACToHostDict(self):
        self.ethMapper = {}
        hosts = self.topo.hosts()
        for h in hosts:
          info = self.topo.nodeInfo(h)
          self.ethMapper[info['mac']] = h

    @set_ev_cls(ofp_event.EventOFPFlowStatsReply, MAIN_DISPATCHER)
    def _flow_stats_reply_handler(self, ev):
        msg = ev.msg
        body = ev.msg.body
        datapath = msg.datapath
        sw_id = datapath.id
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        # Check for response validity
        if not sw_id in self.flowQueryMsg or self.flowQueryMsg[sw_id] <= 0:
            return # Bad response from switches we didn't ask
        self.flowQueryMsg[sw_id] -= 1
        if self.flowQueryMsg[sw_id] == 0:
            self.flowQueryMsg.pop(sw_id)
        for stat in body:
            # Do things
            duration = stat.duration_sec * 1e9 + stat.duration_nsec
            if duration < 1:
                duration = 1
            # only if there are tcp packets
            # print stat.match
            if "udp_src" in stat.match:
                s = stat.match["eth_src"] 
                t = stat.match["eth_dst"] 
                if not self.ethMapper.has_key(s) or not self.ethMapper.has_key(t):
                    self._buildMACToHostDict()
                src = self.ethMapper[s]
                dst = self.ethMapper[t]
                demand = 8 * float(stat.byte_count) / duration / self.bw
                # #print "receive (sw,port) = (" + str(sw_id) + ", " + str(stat.match.tp_src) + ") ", stat.byte_count, duration, ",", demand
                
                flowSignature = (stat.match["ipv4_src"], stat.match["ipv4_dst"], stat.match["udp_src"], stat.match["udp_dst"]) 
                self.flows.append({'converged': False, 'demand': demand, 'src': src, 'dest': dst, 'receiver_limited': False, 'id': flowSignature})
                # If we got all reponse do demand estimation
                if len(self.flowQueryMsg) == 0:
                    print 'Executing demand estimation.'
                    # Clear Reservation from dead flow
                    # TODO: this process hasn't happended yet. So never tested...
                    newFlowList = []
                    for flow in self.flows:
                        newFlowList.append(flow['id'])
                    flowIDToRemove = []
                    for flowSignature in self.flowReserve:
                        if flowSignature not in newFlowList:
                            #That flow dies
                            flowIDToRemove.append(flowSignature)
                            prev = None
                            demand, path = self.flowReserve[flowSignature]
                            for node in path:
                                if prev is not None:
                                    self.bwReservation[(prev, node)] -= demand
                                prev = node
                    for id in flowIDToRemove:
                        del self.flowReserve[id]

                # Demand Estimation
                cmptable = {}
                for flow in self.flows:
                    cmptable[(flow['src'], flow['dest'])] = {'orig_demand': flow['demand'],'new_demand' : None}
                m, norm_flows = estimate_demands(self.flows, self.topo.hosts())
                for flow in norm_flows:
                    cmptable[(flow['src'], flow['dest'])]['new_demand'] = flow['demand']
                for key in sorted(cmptable.iterkeys()):
                    src, dst = key
                    # print src, dst, cmptable[key]['orig_demand'], cmptable[key]['new_demand'] 
                for flow in norm_flows:
                    demand = flow['demand']
                    # Some threshold we use... all flows are big anyway
                    if demand > 0.1: 
                        #Global first fit:
                        paths = self.routing.get_all_route(flow['src'], flow['dest'])
                        for path in paths:
                            prev = None
                            isFitHere = True
                            for node in path:
                                if prev is not None:
                                    k = (prev, node)
                                    prev = node
                                    if not self.bwReservation.has_key(k):
                                        self.bwReservation[k] = 0
                                    if self.bwReservation[k] + demand > 1:
                                        isFitHere = False
                                        break
                                prev = node
                            if isFitHere:
                                prev = None
                                for node in path:
                                    if prev is not None:
                                        k = (prev, node)
                                        self.bwReservation[k] += demand
                                    prev = node
                                self.flowReserve[flow['id']] = (demand, path)
                                self._reflow(flow['id'], path[1:-1])
                                break
