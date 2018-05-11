import struct
import ipaddress

HEADER_LEN = 8
BGP_STATE_LEN = HEADER_LEN + 8

class MsgType:
    BGP_STATE = 0
    BGP_ANNOUNCE = 1

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
                                        msg[0:HEADER_LEN])


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
            self.state = state
            self.peer_id = peer_id

    def pack(self):
        msg = super(BGPStateMsg, self).pack()
        msg += struct.pack(BGPStateMsg.BGPSTATE_FMT, self.peer_id, self.state)
        return msg

    def unpack(self, msg):
        super(BGPStateMsg, self).unpack(msg)
        self.peer_id, self.state = struct.unpack(BGPStateMsg.BGPSTATE_FMT,
                                                 msg[HEADER_LEN:])



# msg = BGPStateMsg(local_id = 1000, peer_id = 2000,
#                   state = BGPStateMsg.BGP_STATE_UP)
# packed = msg.pack()
# msg2 = BGPStateMsg(msg=packed)
# print msg2.router_id, msg2.state

# 255.255.255.255