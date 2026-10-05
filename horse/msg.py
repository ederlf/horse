"""Routing messages using the native daemon's network-byte-order wire contract."""

import os
import socket
import struct
import sys
from enum import IntEnum
from typing import NamedTuple

HEADER = struct.Struct("!HHI")
STATE_BODY = struct.Struct("!IB3x")
ANNOUNCE_BODY = struct.Struct("!I4x")
FIB_ENTRY_STRUCT = struct.Struct("!III")
HEADER_LEN = HEADER.size
BGP_STATE_LEN = HEADER_LEN + STATE_BODY.size
BGP_ANNOUNCE_LEN = HEADER_LEN + ANNOUNCE_BODY.size
WIRE_ALIGNMENT = 8
MAX_MESSAGE_SIZE = 0xFFFF
MAX_FIB_ENTRIES = (MAX_MESSAGE_SIZE - HEADER_LEN) // FIB_ENTRY_STRUCT.size

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if path not in sys.path:
    sys.path.append(path)


def ip2int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


def int2ip(addr):
    return socket.inet_ntoa(struct.pack("!I", addr))


def netmask2cidr(netmask):
    return sum([bin(int(x)).count("1") for x in netmask.split(".")])


def cidr_to_netmask(cidr):
    cidr = int(cidr)
    if not 0 <= cidr <= 32:
        raise ValueError("IPv4 prefix length must be between 0 and 32")
    return (0xFFFFFFFF >> (32 - cidr)) << (32 - cidr)


class MsgType(IntEnum):
    BGP_ACTIVITY = 0
    BGP_FIB = 1
    BGP_STATE = 2
    BGP_ANNOUNCE = 3


class BGPState(IntEnum):
    DOWN = 0
    CONNECTED = 1
    UP = 2


class FIBRoute(NamedTuple):
    """The prefix and next hop carried on the wire; BGP attributes are absent."""

    prefix: str
    next_hop: str


def wire_size(size):
    """Round a logical message length up to the native 8-byte wire alignment."""
    if not HEADER_LEN <= size <= MAX_MESSAGE_SIZE:
        raise ValueError("Routing message length must be between 8 and 65535 bytes")
    return (size + WIRE_ALIGNMENT - 1) // WIRE_ALIGNMENT * WIRE_ALIGNMENT


class RouterMsg:
    HEADER_FMT = HEADER.format

    def __init__(
        self, msg_type=MsgType.BGP_ACTIVITY, size=HEADER_LEN, router_id=0, msg=None
    ):
        if msg is not None:
            self.unpack(msg)
        else:
            self.type = MsgType(msg_type)
            wire_size(size)
            self.size = size
            self.router_id = router_id

    def pack(self):
        wire_size(self.size)
        return HEADER.pack(self.type, self.size, self.router_id)

    def unpack(self, msg):
        # Header-only parsing remains useful for stream readers.
        if len(msg) < HEADER_LEN:
            raise ValueError("Truncated routing message header")
        msg_type, self.size, self.router_id = HEADER.unpack_from(msg)
        self.type = MsgType(msg_type)
        wire_size(self.size)

    def _unpack_frame(self, msg, expected_type, expected_size=None):
        RouterMsg.unpack(self, msg)
        if self.type != expected_type:
            raise ValueError(f"Expected {expected_type.name}, got {self.type.name}")
        if expected_size is not None and self.size != expected_size:
            raise ValueError("Invalid logical length for routing message type")
        if len(msg) != wire_size(self.size):
            raise ValueError(
                "Routing frame must include exactly its padded wire length"
            )


class BGPStateMsg(RouterMsg):
    BGPSTATE_FMT = STATE_BODY.format
    BGP_STATE_DOWN = BGPState.DOWN
    BGP_STATE_CONNECTED = BGPState.CONNECTED
    BGP_STATE_UP = BGPState.UP

    def __init__(self, local_id=0, peer_id=0, state=0, msg=None):
        if msg is not None:
            self.unpack(msg)
        else:
            super().__init__(
                msg_type=MsgType.BGP_STATE, size=BGP_STATE_LEN, router_id=local_id
            )
            self.peer_id = peer_id
            self.state = BGPState(state)

    def pack(self):
        return super().pack() + STATE_BODY.pack(self.peer_id, BGPState(self.state))

    def unpack(self, msg):
        self._unpack_frame(msg, MsgType.BGP_STATE, BGP_STATE_LEN)
        self.peer_id, state = STATE_BODY.unpack_from(msg, HEADER_LEN)
        self.state = BGPState(state)


class BGPAnnounce(RouterMsg):
    BGPANNOUNCE_FMT = ANNOUNCE_BODY.format

    def __init__(self, local_id=0, peer_id=0, state=0, msg=None):
        if msg is not None:
            self.unpack(msg)
        else:
            super().__init__(
                msg_type=MsgType.BGP_ANNOUNCE, size=BGP_ANNOUNCE_LEN, router_id=local_id
            )
            self.peer_id = peer_id

    def pack(self):
        return super().pack() + ANNOUNCE_BODY.pack(self.peer_id)

    def unpack(self, msg):
        self._unpack_frame(msg, MsgType.BGP_ANNOUNCE, BGP_ANNOUNCE_LEN)
        (self.peer_id,) = ANNOUNCE_BODY.unpack_from(msg, HEADER_LEN)


class BGPFIBMsg(RouterMsg):
    FIB_ENTRY = FIB_ENTRY_STRUCT.format
    FIB_ENTRY_LEN = FIB_ENTRY_STRUCT.size

    def __init__(self, local_id=0, routes=None, msg=None):
        if msg is not None:
            self.unpack(msg)
        else:
            self.routes = [] if routes is None else routes
            super().__init__(
                msg_type=MsgType.BGP_FIB,
                size=HEADER_LEN + len(self.routes) * self.FIB_ENTRY_LEN,
                router_id=local_id,
            )

    def unpack(self, msg):
        self._unpack_frame(msg, MsgType.BGP_FIB)
        if (self.size - HEADER_LEN) % self.FIB_ENTRY_LEN:
            raise ValueError("FIB payload must contain complete 12-byte entries")
        self.routes = []
        for ip, mask, next_hop in FIB_ENTRY_STRUCT.iter_unpack(
            msg[HEADER_LEN : self.size]
        ):
            cidr = mask.bit_count()
            if mask != cidr_to_netmask(cidr):
                raise ValueError("FIB netmask must be a contiguous IPv4 prefix mask")
            self.routes.append(FIBRoute(f"{int2ip(ip)}/{cidr}", int2ip(next_hop)))

    def pack(self):
        # Routes are public and may have changed since construction.
        self.size = HEADER_LEN + len(self.routes) * self.FIB_ENTRY_LEN
        padded_size = wire_size(self.size)
        parts = [super().pack()]
        for route in self.routes:
            ip, mask = route.prefix.split("/")
            parts.append(
                FIB_ENTRY_STRUCT.pack(
                    ip2int(ip), cidr_to_netmask(mask), ip2int(route.next_hop)
                )
            )
        parts.append(bytes(padded_size - self.size))
        return b"".join(parts)


class BGPActivity(RouterMsg):
    """Inform the simulator of routing activity, keeping it in FTI mode."""

    def __init__(self, local_id=0, msg=None):
        if msg is not None:
            self.unpack(msg)
        else:
            super().__init__(
                msg_type=MsgType.BGP_ACTIVITY, size=HEADER_LEN, router_id=local_id
            )

    def unpack(self, msg):
        self._unpack_frame(msg, MsgType.BGP_ACTIVITY, HEADER_LEN)
