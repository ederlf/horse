/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifdef NS3_OFSWITCH13

#include "ripl.h"
#include "hederautil.h"

NS_LOG_COMPONENT_DEFINE ("RIPLController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RIPLController);

/********** Public methods ***********/
RIPLController::RIPLController ()
{
  NS_LOG_FUNCTION (this);
}

RIPLController::RIPLController (std::string routing, FatTreeTopo topo)
{
  this->all_switches_up = false;
  this->topo = topo;
  this->routing = get_routing(routing, topo);
  NS_ASSERT_MSG (this->routing != NULL,
                             "Invalid routing type");

  /* Fill macTable in advance, so no need to broadcast avoiding buffering problem */
  std::vector<std::string> hosts = this->topo.layer_nodes(LAYER_HOST);
  for(auto h: hosts) {
    std::vector<std::string> edge = this->topo.up_nodes(h);
    Mac48Address src = Mac48Address(this->topo.id_gen(h).MACStr().c_str());
    std::pair<uint32_t, uint32_t> ports = this->topo.port(edge[0], h);
    uint64_t dpid = this->topo.id_gen(edge[0]).dpid;
    this->macTable[src] = std::make_pair(dpid, ports.first); 
    Ipv4Address ip = Ipv4Address(this->topo.id_gen(h).IPStr().c_str());
    this->ipTable[ip] = src;
  }
  NS_LOG_FUNCTION (this);
}

RIPLController::~RIPLController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RIPLController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RIPLController")
    .SetParent<OFSwitch13Controller> ()
    .SetGroupName ("OFSwitch13")
    .AddConstructor<RIPLController> ()
  ;
  return tid;
}

void
RIPLController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  OFSwitch13Controller::DoDispose ();
}

void 
RIPLController::add_flow(Ptr<const RemoteSwitch> sw, uint32_t buffer_id,
                          uint32_t prio, std::string match, std::string instructions)
{
  uint64_t idle_timeout = 120;
  uint64_t hard_timeout = 60;
  char cmd[] =  "flow-mod cmd=add,table=0,idle=%ld,buffer=%d,hard=%ld,prio=%d %s %s";
  DpctlExecute (sw, format(cmd, idle_timeout, buffer_id, hard_timeout, prio, match.c_str(), instructions.c_str()));           
}

std::vector<uint64_t>
RIPLController::_raw_dpids(std::vector<std::string> nodes)
{
  std::vector<uint64_t> dpids;
  for (auto n: nodes){
    dpids.push_back(FatTreeTopo::id_gen(n).dpid);
  }
  return dpids;
}

std::string 
RIPLController::match_from_packet(uint8_t *pkt)
{

  uint8_t eth_src[6];
  uint8_t eth_dst[6];
  Mac48Address eth_src48;
  Mac48Address eth_dst48;
  uint16_t type;
  uint8_t *ptr = pkt;

  std::memcpy(eth_dst,ptr, 6);
  std::memcpy(eth_src, ptr + 6, 6);
  std::memcpy(&type, ptr + 12, 2);
  eth_src48.CopyFrom(eth_src);
  eth_dst48.CopyFrom(eth_dst);

  std::ostringstream cmd;
  cmd << "eth_type=" << ntohs(type) << ",eth_src=" << eth_src48 << ",eth_dst=" << eth_dst48;
  std::string cur_cmd = cmd.str();
  ptr += 14;
  if (ntohs(type) == 0x0800) {
      uint32_t ip_src;
      uint32_t ip_dst;
      uint8_t ip_proto;
      std::memcpy(&ip_proto, ptr + 9, 1);
      std::memcpy(&ip_src, ptr + 12, 4);
      std::memcpy(&ip_dst, ptr + 16, 4);

      Ipv4Address v4_src = Ipv4Address(ntohl(ip_src));
      Ipv4Address v4_dst = Ipv4Address(ntohl(ip_dst));
      std::ostringstream str_src;
      std::ostringstream str_dst;

      v4_src.Print(str_src);
      v4_dst.Print(str_dst);

      char new_cmd[] = "ip_proto=%d,ip_src=%s,ip_dst=%s,%s";
      cur_cmd = format(new_cmd, ip_proto, str_src.str().c_str(), str_dst.str().c_str(), cur_cmd.c_str());
      ptr += 20;
      if (ip_proto == 6){
        uint16_t tcp_src;
        uint16_t tcp_dst;
        std::memcpy(&tcp_src, ptr , 2);
        std::memcpy(&tcp_dst, ptr + 2, 2);
        char new_cmd[] = "tcp_src=%d,tcp_dst=%d,%s";
        cur_cmd = format(new_cmd, htons(tcp_src), htons(tcp_dst), cur_cmd.c_str());

      }
      else if (ip_proto == 17){
        uint16_t udp_src;
        uint16_t udp_dst;
        std::memcpy(&udp_src, ptr , 2);
        std::memcpy(&udp_dst, ptr + 2, 2);
        char new_cmd[] = "udp_src=%d,udp_dst=%d,%s";
        cur_cmd = format(new_cmd, htons(udp_src), htons(udp_dst), cur_cmd.c_str());
      }
  }
  else if (ntohs(type) == 0x0806 ){
      uint8_t arp_sha[6];
      uint8_t arp_tha[6];
      Mac48Address arp_sha48;
      Mac48Address arp_tha48;
      uint32_t arp_spa;
      uint32_t arp_tpa;
      uint16_t arp_op;

      std::memcpy(&arp_op, ptr + 6, 2);
      std::memcpy(arp_sha, ptr + 8, 6);
      std::memcpy(&arp_spa, ptr + 14, 4);
      std::memcpy(arp_tha, ptr + 18, 6);
      std::memcpy(&arp_tpa, ptr + 24, 4);
      
      arp_sha48.CopyFrom(arp_sha);
      arp_tha48.CopyFrom(arp_tha);
      Ipv4Address v4_arp_spa = Ipv4Address(ntohl(arp_spa));
      Ipv4Address v4_arp_tpa = Ipv4Address(ntohl(arp_tpa));

      std::ostringstream str_arp_spa;
      std::ostringstream str_arp_tpa;

      v4_arp_spa.Print(str_arp_spa);
      v4_arp_tpa.Print(str_arp_tpa);

      std::ostringstream cmd;
      cmd << "arp_op=" << ntohs(arp_op) << ",arp_spa="<< str_arp_spa.str() << ",arp_tpa=" << str_arp_tpa.str() << ",arp_tha=" << arp_tha48 << ",arp_sha=" << arp_sha48;
      char new_cmd[] = "%s,%s"; 
      cur_cmd = format(new_cmd, cur_cmd.c_str(), cmd.str().c_str());
  }
  return cur_cmd;
}

void 
RIPLController::_install_path(Ptr<const RemoteSwitch> sw, 
                              uint64_t out_dpid,  uint32_t buffer_id, 
                              uint32_t final_out_port, uint8_t *pkt)
{
    uint64_t dpId = sw->GetDpId ();
    std::string in_name = FatTreeTopo::id_gen(dpId).NameStr();
    std::string out_name = FatTreeTopo::id_gen(out_dpid).NameStr();
    Path route = this->routing->get_route(in_name, out_name, pkt);  

    size_t route_size = route.size();
    // NS_LOG_DEBUG("ROUTE SIZE " << route.size() << " IN " << in_name << " out " <<  out_name);
    if (route_size > 0){
        std::string match = this->match_from_packet(pkt);
        size_t i = 0;
        for (auto node: route){
          // NS_LOG_DEBUG("NODE in the path " << node);
          uint64_t node_dpid = FatTreeTopo::id_gen(node).dpid;
          uint32_t out_port;
          if (i < (route_size - 1)) {
            std::string next_node = route[i + 1];  
            std::pair<uint32_t, uint32_t> ports = this->topo.port(node, next_node);
            out_port = ports.first;
          }
          else {
            out_port = final_out_port;
          }
          char cmd[] = "apply:output=%d";
          std::string actions = format(cmd, out_port);
          if (!i){
            this->add_flow(this->switches[node_dpid], buffer_id, 10, match, actions);
          }
          else {
            this->add_flow(this->switches[node_dpid], NO_BUFFER, 10, match, actions);
          }
          i += 1;
        }
    }
}                    
 
void RIPLController::send_packet_out(Ptr<const RemoteSwitch> sw, uint32_t xid,
                                   uint32_t in_port, uint32_t out_port, uint8_t* data, 
				   uint32_t data_length)
{
      // Lets send the packet out to switch.
      struct ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = NO_BUFFER;
      reply.in_port = in_port;
      // reply.data_length = 0;
      // reply.data = 0;

      // if (buffer_id == NO_BUFFER)
         // {
          // No packet buffer. Send data back to switch
      // Ptr<Packet> pkt =  Create<Packet>(data, data_length, true);
      // uint8_t pktData[pkt->GetSize ()];
      // pkt->CopyData (pktData, pkt->GetSize ());

      reply.data_length = data_length;
      reply.data = data;
      reply.actions_num = 1;
      if (out_port){
        // Create output actions
        struct ofl_action_output *a =
        (struct ofl_action_output*)xmalloc (sizeof (struct ofl_action_output));
      	a->header.type = OFPAT_OUTPUT;
      	a->port = out_port;
      	a->max_len = 0;
        reply.actions = (struct ofl_action_header**)&a;
        SendToSwitch (sw, (struct ofl_msg_header*)&reply, xid);
        free (a);
      }
      else {
        struct ofl_action_group *a =
          (struct ofl_action_group*) xmalloc (sizeof (struct ofl_action_group) );
        a->header.type = OFPAT_GROUP;
        a->group_id = 1;
        reply.actions = (struct ofl_action_header**)&a;
        SendToSwitch (sw, (struct ofl_msg_header*)&reply, xid);
        free (a);
      }
}

ofl_err
RIPLController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  uint64_t dpId = swtch->GetDpId ();
  // enum ofp_packet_in_reason reason = msg->reason;

  char *msgStr =
    ofl_structs_match_to_string ((struct ofl_match_header*)msg->match, 0);
  NS_LOG_DEBUG ("DP "<< dpId << " Packet in match: " << msgStr);
  free (msgStr);
  uint8_t reason = msg->reason;
  uint8_t *pkt = NULL;

  if (reason == OFPR_NO_MATCH)
  {
      // Let's get necessary information (input port and mac address)
      uint32_t inPort;
      size_t portLen = OXM_LENGTH (OXM_OF_IN_PORT); // (Always 4 bytes)
      struct ofl_match_tlv *input =
        oxm_match_lookup (OXM_OF_IN_PORT, (struct ofl_match*)msg->match);
      memcpy (&inPort, input->value, portLen);

      Mac48Address src48;
      struct ofl_match_tlv *ethSrc =
        oxm_match_lookup (OXM_OF_ETH_SRC, (struct ofl_match*)msg->match);
      src48.CopyFrom (ethSrc->value);

      Mac48Address dst48;
      struct ofl_match_tlv *ethDst =
        oxm_match_lookup (OXM_OF_ETH_DST, (struct ofl_match*)msg->match);
      dst48.CopyFrom (ethDst->value);
      // this->macTable[src48] = std::make_pair(dpId, inPort);
      // self.macTable[src] = (dpid, in_port)
      // NS_LOG_DEBUG("DP "<< dpId << " MAC " << dst48 << " Broadcast? " << dst48.IsBroadcast());
      if(dst48.IsBroadcast()) {
         struct ofl_match_tlv *arp_tpa =
        oxm_match_lookup (OXM_OF_ARP_TPA, (struct ofl_match*)msg->match);
        Ipv4Address ip = Ipv4Address( ntohl(*((uint32_t*) arp_tpa->value)));
        ipMACTable_t::iterator it = this->ipTable.find(ip);
        if (it != ipTable.end()){
            // NS_LOG_DEBUG("DP "<< dpId << " Broadcast " << dst48);
            uint8_t newMac[6];
            this->ipTable[ip].CopyTo(newMac);
            dst48.CopyFrom(newMac);
            // NS_LOG_DEBUG("DP "<< dpId << " Changed " << dst48);
        }
      } 

      // if (msg->buffer_id == NO_BUFFER){
      pkt = msg->data; 
      // NS_LOG_DEBUG ("DP "<< dpId << " Buffer " << msg->buffer_id);
      // printf("Buffer %d\n", msg->data_length);
      // }
      macTable_t::iterator it = this->macTable.find(dst48);
      if (it != this->macTable.end()) {
          uint64_t out_dpid = this->macTable[dst48].first;
          uint32_t out_port = this->macTable[dst48].second;
          char cmd[] = "apply:output=%d";
          std::string actions = format(cmd, out_port);
           NS_LOG_DEBUG ("DP "<< dpId << " Installing Path");
          _install_path(swtch, out_dpid, msg->buffer_id, out_port, pkt);
          // send_packet_out(this->switches[out_dpid], xid, OFPP_CONTROLLER, 
                          // out_port, msg->data, msg->data_length);
      } 
      /* Should never get here */
      else {
          NS_LOG_DEBUG ("Sending packet out I should not");
          std::vector<uint64_t> dps = _raw_dpids(this->topo.layer_nodes(LAYER_EDGE));
          for (auto  dp: dps){
              std::vector<uint32_t> ports;
              std::string sw_name = FatTreeTopo::id_gen(dp).NameStr();
              for (auto host: this->topo.down_nodes(sw_name)){
                  NS_LOG_DEBUG("Switch "<< sw_name << " Host " << host);
                  uint32_t sw_port =  this->topo.port(sw_name, host).first;
                  if (dp != dpId || (dp == dpId && inPort != sw_port)){
                      ports.push_back(sw_port);
                }
              }
              for (auto port: ports){
                send_packet_out(this->switches[dp], xid, 
                             OFPP_CONTROLLER, port, msg->data, 
                            msg->data_length);
              }
         }
      }
  }

  // // All handlers must free the message when everything is ok
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);
  return 0;
}

ofl_err
RIPLController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // NS_LOG_DEBUG ( "Flow entry expired. Removing from L2 switch table.");
  // uint64_t dpId = swtch->GetDpId ();
  // DatapathMap_t::iterator it = m_learnedInfo.find (dpId);
  // if (it != m_learnedInfo.end ())
  //   {
  //     Mac48Address mac48;
  //     struct ofl_match_tlv *ethSrc =
  //       oxm_match_lookup (OXM_OF_ETH_DST, (struct ofl_match*)msg->stats->match);
  //     mac48.CopyFrom (ethSrc->value);

  //     L2Table_t *l2Table = &it->second;
  //     L2Table_t::iterator itSrc = l2Table->find (mac48);
  //     if (itSrc != l2Table->end ())
  //       {
  //         l2Table->erase (itSrc);
  //       }
  //   }

  // // All handlers must free the message when everything is ok
  // ofl_msg_free_flow_removed (msg, true, 0);
  return 0;
}

ofl_err
  RIPLController::HandleMultipartReply (
    struct ofl_msg_multipart_reply_header *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
 {
  return 0;
 } 

/********** Private methods **********/
void
RIPLController::HandshakeSuccessful (
  Ptr<const RemoteSwitch> sw)
{
  NS_LOG_FUNCTION (this << sw);
  uint64_t dpId = sw->GetDpId ();

  std::string name_str = FatTreeTopo::id_gen(dpId).NameStr();
  std::map<uint64_t, Ptr<const RemoteSwitch>>::iterator it = this->switches.find (dpId);
  NS_LOG_DEBUG("Saw switch come up: " << dpId);
  if (it != this->switches.end ()){
      NS_LOG_DEBUG("Already saw that switch: " << dpId);
  }
  else {
      NS_LOG_DEBUG("Added fresh switch: " << dpId);
      // Ptr<RemoteSwitch> add = sw; 
      this->switches[dpId] = sw;
  }

  // After a successfull handshake, let's install the table-miss entry, setting
  // to 128 bytes the maximum amount of data from a packet that should be sent
  // to the controller.
  DpctlExecute (sw, "flow-mod cmd=add,table=0,prio=0 "
                "apply:output=ctrl:1500");


  // Add a group that broadcast to every host
  // std::vector<uint32_t> ports;
  // std::string sw_name = FatTreeTopo::id_gen(dpId).NameStr();
  // for (auto host: this->topo.down_nodes(sw_name)){
  //     NS_LOG_DEBUG("Switch "<< sw_name << " Host " << host);
  //     uint32_t sw_port =  this->topo.port(sw_name, host).first;
  //     ports.push_back(sw_port);
  // }  
  // std::ostringstream cmd;
  // cmd << "group-mod cmd=add,type=all,group=1 ";
  // for (auto port: ports){
  //   char ext_cmd[] = "weight=0,port=any,group=any output=%d ";
  //   cmd << format(ext_cmd, port);
  // }
  // // std::cout << cmd.str() << "\n";
  // // abort();
  // DpctlExecute (sw, cmd.str());
  // Configure te switch to buffer packets and send only the first 128 bytes of
  // each packet sent to the controller when not using an output action to the
  // OFPP_CONTROLLER logical port.
  DpctlExecute (sw, "set-config miss=1500");

  // Create an empty L2SwitchingTable and insert it into m_learnedInfo
  // L2Table_t l2Table;
  // uint64_t dpId = sw->GetDpId ();

  // std::pair <DatapathMap_t::iterator, bool> ret;
  // ret =  m_learnedInfo.insert (std::pair<uint64_t, L2Table_t> (dpId, l2Table));
  // if (ret.second == false)
  //   {
  //     NS_LOG_ERROR ("Table exists for this datapath.");
  //   }
}

} // namespace ns3
#endif // NS3_OFSWITCH13
