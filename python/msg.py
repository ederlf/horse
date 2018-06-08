import struct
import ipaddress
import os
import sys
import socket

HEADER_LEN = 8
BGP_STATE_LEN = HEADER_LEN + 8
BGP_ANNOUNCE_LEN = HEADER_LEN + 8

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if path not in sys.path:
    sys.path.append(path)

from bgp_peer.rib import RibTuple

def ip2int(addr):                                                               
    return struct.unpack("!I", socket.inet_aton(addr))[0]                       

def int2ip(addr):                                                               
    return socket.inet_ntoa(struct.pack("!I", addr)) 

def netmask2cidr(netmask):
    return sum([bin(int(x)).count("1") for x in netmask.split(".")])

def cidr_to_netmask(cidr):
    cidr = int(cidr)
    mask = (0xffffffff >> (32 - cidr)) << (32 - cidr)
    return mask
    # return (str( (0xff000000 & mask) >> 24)   + '.' +
    #       str( (0x00ff0000 & mask) >> 16)   + '.' +
    #       str( (0x0000ff00 & mask) >> 8)    + '.' +
    #       str( (0x000000ff & mask)))

class MsgType:
    BGP_STATE = 0
    BGP_ANNOUNCE = 1
    BGP_FIB = 2
    BGP_ACTIVITY = 3

class RouterMsg(object):
    
    HEADER_FMT = "!HHI"
    
    def __init__(self, msg_type = 0, size = 0, router_id = 0, msg = None):
        if msg:
            self.unpack(msg)
        else:
            self.type = msg_type
            self.size = size
            self.router_id = router_id
        
    def pack(self):
        msg = struct.pack(RouterMsg.HEADER_FMT, self.type, self.size,
                          self.router_id)
        return msg

    def unpack(self, msg):
        self.type, self.size, self.router_id = struct.unpack( 
                                        RouterMsg.HEADER_FMT, 
                                        msg[:HEADER_LEN])

class BGPStateMsg(RouterMsg):
    
    BGPSTATE_FMT = "!IB3x"
    BGP_STATE_DOWN = 0
    BGP_STATE_CONNECTED = 1
    BGP_STATE_UP = 2

    def __init__(self, local_id = 0, peer_id = 0, state = 0, msg = None):
        if msg:
            self.unpack(msg)
        else:
            super(BGPStateMsg, self).__init__(msg_type=MsgType.BGP_STATE,
                                              size = BGP_STATE_LEN, 
                                              router_id = local_id)
            self.peer_id = peer_id
            self.state = state

    def pack(self):
        msg = super(BGPStateMsg, self).pack()
        msg += struct.pack(BGPStateMsg.BGPSTATE_FMT, self.peer_id, self.state)
        return msg

    def unpack(self, msg):
        super(BGPStateMsg, self).unpack(msg)
        self.peer_id, self.state = struct.unpack(BGPStateMsg.BGPSTATE_FMT,
                                                 msg[HEADER_LEN:])

class BGPAnnounce(RouterMsg):
    BGPANNOUNCE_FMT = "!I4x"

    def __init__(self, local_id = 0, peer_id = 0, state = 0, msg = None):
        if msg:
            self.unpack(msg)
        else:
            super(BGPAnnounce, self).__init__(msg_type=MsgType.BGP_ANNOUNCE,
                                              size = BGP_ANNOUNCE_LEN, 
                                              router_id = local_id)
            self.peer_id = peer_id

    def pack(self):
        msg = super(BGPAnnounce, self).pack()
        msg += struct.pack(BGPAnnounce.BGPANNOUNCE_FMT, self.peer_id)
        return msg

    def unpack(self, msg):
        super(BGPAnnounce, self).unpack(msg)
        peer_id = struct.unpack(BGPAnnounce.BGPANNOUNCE_FMT,
                                     msg[HEADER_LEN:])
        self.peer_id = peer_id[0]
                
class BGPFIBMsg(RouterMsg):
    FIB_ENTRY = "!III"
    FIB_ENTRY_LEN = 12

    def __init__(self, local_id = 0, routes = None, msg = None):
        if msg:
            self.unpack(msg)
        else:
            super(BGPFIBMsg, self).__init__(msg_type=MsgType.BGP_FIB,
                                              size = HEADER_LEN, 
                                              router_id = local_id)
            self.routes = routes
            self.size += len(routes) * 12

    def unpack(self, msg):
        pass

    def pack(self):
        msg = super(BGPFIBMsg, self).pack()
        for route in self.routes:
            ip, mask = route.prefix.split('/')
            next_hop = route.next_hop
            msg += struct.pack(BGPFIBMsg.FIB_ENTRY, ip2int(ip),
                               cidr_to_netmask(mask), ip2int(next_hop))
        rest = self.size % 8
        if rest:
            msg+= struct.pack("!%sx" % rest)
        return msg

# This message is basically to inform the simulator that there is BGP activity
# keeping the simulation mode in the FTI mode.
class BGPActivity(RouterMsg):

    def __init__(self, local_id = 0, msg = None):
        if msg:
            self.unpack(msg)
        else:
            super(BGPActivity, self).__init__(msg_type=MsgType.BGP_ACTIVITY,
                                              size = HEADER_LEN, 
                                              router_id = local_id)
    def pack(self):
        msg = super(BGPActivity, self).pack()
        return msg

    def unpack(self, msg):
        super(BGPActivity, self).unpack(msg)

# routes = []

# for i in range(0, 1):
#     route =  RibTuple('10.0.0.1/24', '172.0.0.2', 10, '172.0.0.2', 'igp', '100, 200, 300', '0', 0,'false')
#     routes.append(route)

# msg = BGPFIBMsg(local_id=1000, routes = routes)
# print msg.size
# data = msg.pack()

# msg = BGPStateMsg(local_id = 1000, peer_id = 2000,
#                   state = BGPStateMsg.BGP_STATE_UP)
# packed = msg.pack()
# msg2 = BGPStateMsg(msg=packed)
# print msg2.router_id, msg2.state

# 255.255.255.255