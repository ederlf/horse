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
RIPLController::add_flow(Ptr<const RemoteSwitch> sw, 
                          uint32_t prio, std::string match, std::string instructions)
{
  uint64_t idle_timeout = 120;
  uint64_t hard_timeout = 10;
  char cmd[] =  "flow-mod cmd=add,table=0,idle=%ld,hard=%ld, prio=%d %s %s";
  DpctlExecute (sw, format(cmd, idle_timeout, hard_timeout, prio, match.c_str(), instructions.c_str()));           
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

  char cmd[] = "eth_type=%d, eth_src=%s, eth_dst=%s";
  std::string cur_cmd = format(cmd, ntohs(type), eth_src48, eth_dst48);
  ptr += 12;
  if (ntohs(type) == 0x0800) {
      uint32_t ip_src;
      uint32_t ip_dst;
      uint8_t ip_proto;
      std::memcpy(&ip_proto, ptr + 9, 1);
      std::memcpy(&ip_src, ptr + 12, 4);
      std::memcpy(&ip_dst, ptr + 16, 4);

      char new_cmd[] = "ip_proto=%d, ip_src=%d, ip_dst=%d, %s";
      cur_cmd = format(new_cmd, ip_proto, ip_src, ip_dst, cur_cmd.c_str());
      ptr += 20;
      if (ip_proto == 6){
        uint16_t tcp_src;
        uint16_t tcp_dst;
        std::memcpy(&tcp_src, ptr , 2);
        std::memcpy(&tcp_dst, ptr + 2, 2);
        char new_cmd[] = "tcp_src=%d, tcp_dst=%d, %s";
        cur_cmd = format(new_cmd, tcp_src, tcp_dst, cur_cmd.c_str());

      }
      else if (ip_proto == 17){
        uint16_t udp_src;
        uint16_t udp_dst;
        std::memcpy(&udp_src, ptr , 2);
        std::memcpy(&udp_dst, ptr + 2, 2);
        char new_cmd[] = "udp_src=%d, udp_dst=%d, %s";
        cur_cmd = format(new_cmd, udp_src, udp_dst, cur_cmd.c_str());
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

      char new_cmd[] = "arp_op=%d, arp_spa=%d, arp_tpa=%d, arp_tha=%s, arp_sha= %s, %s";
      
      cur_cmd = format(new_cmd, arp_op, arp_spa, arp_tpa, arp_tha48, arp_sha48, cur_cmd.c_str());
  }
  return cur_cmd;
}

void 
RIPLController::_install_path(Ptr<const RemoteSwitch> sw, 
                              uint64_t out_dpid, uint32_t final_out_port, uint8_t *pkt)
{
    uint64_t dpId = sw->GetDpId ();
    std::string in_name = FatTreeTopo::id_gen(dpId).NameStr();
    std::string out_name = FatTreeTopo::id_gen(out_dpid).NameStr();
    Path route = this->routing->get_route(in_name, out_name, pkt);
    size_t route_size = route.size(); 
    if (route_size > 0){
        std::string match = this->match_from_packet(pkt);
        size_t i = 0;
        for (auto node: route){
          uint64_t node_dpid = FatTreeTopo::id_gen(node).dpid;
          uint32_t out_port;
          if (i < (route_size - 1)) {
            std::string next_node = route[i+1];  
            std::pair<uint32_t, uint32_t> ports = this->topo.port(node, next_node);
            out_port = ports.first;
          }
          else {
            out_port = final_out_port;
          }
          char cmd[] = "apply:output=%d";
          std::string actions = format(cmd, out_port);
          this->add_flow(this->switches[node_dpid], 10, match, actions);
        }
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

  // if (reason == OFPR_NO_MATCH)
  //   {
  //     // Let's get necessary information (input port and mac address)
  //     uint32_t inPort;
  //     size_t portLen = OXM_LENGTH (OXM_OF_IN_PORT); // (Always 4 bytes)
  //     struct ofl_match_tlv *input =
  //       oxm_match_lookup (OXM_OF_IN_PORT, (struct ofl_match*)msg->match);
  //     memcpy (&inPort, input->value, portLen);

  //     Mac48Address src48;
  //     struct ofl_match_tlv *ethSrc =
  //       oxm_match_lookup (OXM_OF_ETH_SRC, (struct ofl_match*)msg->match);
  //     src48.CopyFrom (ethSrc->value);

  //     Mac48Address dst48;
  //     struct ofl_match_tlv *ethDst =
  //       oxm_match_lookup (OXM_OF_ETH_DST, (struct ofl_match*)msg->match);
  //     dst48.CopyFrom (ethDst->value);

  //     // Get L2Table for this datapath
  //     DatapathMap_t::iterator it = m_learnedInfo.find (dpId);
  //     if (it != m_learnedInfo.end ())
  //       {
  //         L2Table_t *l2Table = &it->second;

  //         // Looking for out port based on dst address (except for broadcast)
  //         if (!dst48.IsBroadcast ())
  //           {
  //             L2Table_t::iterator itDst = l2Table->find (dst48);
  //             if (itDst != l2Table->end ())
  //               {
  //                 outPort = itDst->second;
  //               }
  //             else
  //               {
  //                 NS_LOG_DEBUG ("No L2 info for mac " << dst48 << ". Flood.");
  //               }
  //           }

  //         // Learning port from source address
  //         NS_ASSERT_MSG (!src48.IsBroadcast (), "Invalid src broadcast addr");
  //         L2Table_t::iterator itSrc = l2Table->find (src48);
  //         if (itSrc == l2Table->end ())
  //           {
  //             std::pair <L2Table_t::iterator, bool> ret;
  //             ret = l2Table->insert (
  //                 std::pair<Mac48Address, uint32_t> (src48, inPort));
  //             if (ret.second == false)
  //               {
  //                 NS_LOG_ERROR ("Can't insert mac48address / port pair");
  //               }
  //             else
  //               {
  //                 NS_LOG_DEBUG ("Learning that mac " << src48 <<
  //                               " can be found at port " << inPort << " In DP " << dpId);

  //                 // Send a flow-mod to switch creating this flow. Let's
  //                 // configure the flow entry to 10s idle timeout and to
  //                 // notify the controller when flow expires. (flags=0x0001)
  //                 std::ostringstream cmd;
  //                 cmd << "flow-mod cmd=add,table=0,idle=10,flags=0x0001"
  //                     << ",prio=" << ++prio << " eth_dst=" << src48
  //                     << " apply:output=" << inPort;
  //                 DpctlExecute (swtch, cmd.str ());
  //               }
  //           }
  //         else
  //           {
  //             std::cout << itSrc->second << " " << inPort << "\n";
  //             NS_ASSERT_MSG (itSrc->second == inPort,
  //                            "Inconsistent L2 switching table");
  //           }
  //       }
  //     else
  //       {
  //         NS_LOG_ERROR ("No L2 table for this datapath id " << dpId);
  //       }

  //     // Lets send the packet out to switch.
  //     struct ofl_msg_packet_out reply;
  //     reply.header.type = OFPT_PACKET_OUT;
  //     reply.buffer_id = msg->buffer_id;
  //     reply.in_port = inPort;
  //     reply.data_length = 0;
  //     reply.data = 0;

  //     if (msg->buffer_id == NO_BUFFER)
  //       {
  //         // No packet buffer. Send data back to switch
  //         reply.data_length = msg->data_length;
  //         reply.data = msg->data;
  //       }

  //     // Create output action
  //     struct ofl_action_output *a =
  //       (struct ofl_action_output*)xmalloc (sizeof (struct ofl_action_output));
  //     a->header.type = OFPAT_OUTPUT;
  //     a->port = outPort;
  //     a->max_len = 0;

  //     reply.actions_num = 1;
  //     reply.actions = (struct ofl_action_header**)&a;

  //     SendToSwitch (swtch, (struct ofl_msg_header*)&reply, xid);
  //     free (a);
  //   }
  // else
  //   {
  //     NS_LOG_WARN ("This controller can't handle the packet. Unkwnon reason.");
  //   }

  // // All handlers must free the message when everything is ok
  // ofl_msg_free ((struct ofl_msg_header*)msg, 0);
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
                "apply:output=ctrl:128");

  // Configure te switch to buffer packets and send only the first 128 bytes of
  // each packet sent to the controller when not using an output action to the
  // OFPP_CONTROLLER logical port.
  DpctlExecute (sw, "set-config miss=128");

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
