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

#ifndef RIPL_CONTROLLER_H
#define RIPL_CONTROLLER_H

#include <ns3/ofswitch13-module.h>
#include "routing.h"

namespace ns3 {

/**
 * \ingroup ofswitch13
 * \brief The Hedera  OpenFlow 1.3 controller (works for data centers)
 */
class RIPLController : public OFSwitch13Controller
{
public:
  RIPLController ();          //!< Default constructor
  RIPLController (std::string routing, FatTreeTopo topo);
  virtual ~RIPLController (); //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  std::vector<uint64_t>
  _raw_dpids(std::vector<std::string> nodes);

  void 
  add_flow(Ptr<const RemoteSwitch> sw, uint32_t buffer_id,
                          uint32_t prio, std::string match, std::string instructions);

  std::string 
  match_from_packet(uint8_t *pkt);

  void 
  _install_path(Ptr<const RemoteSwitch> sw, uint64_t out_dpid, uint32_t buffer_id,
                uint32_t final_out_port, uint8_t *pkt);

  void 
  send_packet_out(Ptr<const RemoteSwitch> sw, uint32_t xid,
                   uint32_t in_port, uint32_t out_port, 
                  uint8_t* data, uint32_t data_length);

  /**
   * Handle packet-in messages sent from switch to this controller. Look for L2
   * switching information, update the structures and send a packet-out back.
   *
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);

  /**
   * Handle flow removed messages sent from switch to this controller. Look for
   * L2 switching information and removes associated entry.
   *
   * \param msg The flow removed message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);

  ofl_err HandleMultipartReply (
    struct ofl_msg_multipart_reply_header *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid);

protected:
  // Inherited from OFSwitch13Controller
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:

  typedef std::map<Mac48Address, std::pair<uint64_t, uint32_t>> macTable_t;
  typedef std::map<Ipv4Address, Mac48Address> ipMACTable_t;   
  bool all_switches_up;
  std::map<uint64_t, Ptr<const RemoteSwitch>> switches;
  ipMACTable_t ipTable;
  macTable_t macTable;
  FatTreeTopo topo;
  StructuredRouting *routing;

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  IpMacMap_t m_arpTable; //!< ARP resolution table.

};

} // namespace ns3
#endif /* RIPLController */
