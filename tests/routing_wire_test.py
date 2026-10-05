"""Verify routing codecs against the same wire fixtures used by native tests."""

import importlib.util
import unittest
from pathlib import Path

# Load this pure Python codec without requiring compiled topology bindings.
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("horse_msg", ROOT / "horse/msg.py")
msg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(msg)
FIXTURES = {
    name: bytes.fromhex(hex_bytes)
    for name, hex_bytes in (
        line.split()
        for line in (ROOT / "tests/fixtures/routing_wire.txt").read_text().splitlines()
    )
}
RID = msg.ip2int("10.0.0.1")
PEER = msg.ip2int("10.0.0.2")
ROUTES = [
    msg.FIBRoute("10.1.0.0/16", "10.0.0.2"),
    msg.FIBRoute("192.0.2.0/24", "10.0.0.3"),
    msg.FIBRoute("0.0.0.0/0", "10.0.0.4"),
]


class RoutingWireTests(unittest.TestCase):
    def test_native_message_ids(self):
        self.assertEqual([int(t) for t in msg.MsgType], [0, 1, 2, 3])
        self.assertEqual(msg.MsgType.BGP_ACTIVITY, 0)
        self.assertEqual(msg.MsgType.BGP_FIB, 1)
        self.assertEqual(msg.MsgType.BGP_STATE, 2)
        self.assertEqual(msg.MsgType.BGP_ANNOUNCE, 3)

    def test_shared_fixtures(self):
        objects = {
            "activity": msg.BGPActivity(local_id=RID),
            "state": msg.BGPStateMsg(local_id=RID, peer_id=PEER, state=2),
            "announce": msg.BGPAnnounce(local_id=RID, peer_id=PEER),
            "fib_empty": msg.BGPFIBMsg(local_id=RID),
            "fib_one": msg.BGPFIBMsg(local_id=RID, routes=ROUTES[:1]),
            "fib_two": msg.BGPFIBMsg(local_id=RID, routes=ROUTES[:2]),
            "fib_three": msg.BGPFIBMsg(local_id=RID, routes=ROUTES),
        }
        for name, original in objects.items():
            with self.subTest(name=name):
                wire = FIXTURES[name]
                self.assertEqual(original.pack(), wire)
                decoded = type(original)(msg=wire)
                self.assertEqual(decoded.router_id, RID)
                self.assertEqual(decoded.type, original.type)
                self.assertEqual(decoded.size, original.size)
                self.assertEqual(decoded.pack(), wire)
                if name.startswith("fib_"):
                    self.assertEqual(decoded.routes, original.routes)
                elif name in ("state", "announce"):
                    self.assertEqual(decoded.peer_id, PEER)

    def test_state_codes(self):
        for state in msg.BGPState:
            original = msg.BGPStateMsg(state=state)
            self.assertEqual(msg.BGPStateMsg(msg=original.pack()).state, state)
        with self.assertRaises(ValueError):
            msg.BGPStateMsg(state=3)

    def test_header_and_wire_length_limits(self):
        header = msg.RouterMsg(msg=FIXTURES["fib_one"][:8])
        self.assertEqual(header.size, 20)
        self.assertEqual(header.pack(), FIXTURES["fib_one"][:8])
        self.assertEqual(msg.wire_size(65535), 65536)
        for size in (0, 7, 65536):
            with self.subTest(size=size), self.assertRaises(ValueError):
                msg.RouterMsg(size=size)

    def test_truncated_or_extra_bytes(self):
        classes = {
            "activity": msg.BGPActivity,
            "state": msg.BGPStateMsg,
            "announce": msg.BGPAnnounce,
            "fib_one": msg.BGPFIBMsg,
        }
        for name, cls in classes.items():
            wire = FIXTURES[name]
            for length in range(len(wire)):
                with (
                    self.subTest(name=name, length=length),
                    self.assertRaises(ValueError),
                ):
                    cls(msg=wire[:length])
            with self.assertRaises(ValueError):
                cls(msg=wire + b"\0")

    def test_wrong_type_and_lengths(self):
        with self.assertRaises(ValueError):
            msg.BGPStateMsg(msg=FIXTURES["announce"])
        with self.assertRaises(ValueError):
            msg.RouterMsg(msg=msg.HEADER.pack(99, 8, RID))
        for size in (0, 7, 9, 12, 20):
            wire = msg.HEADER.pack(msg.MsgType.BGP_STATE, size, RID) + bytes(8)
            with self.subTest(size=size), self.assertRaises(ValueError):
                msg.BGPStateMsg(msg=wire)
        # An aligned wire buffer still requires whole logical FIB entries.
        with self.assertRaises(ValueError):
            msg.BGPFIBMsg(msg=msg.HEADER.pack(msg.MsgType.BGP_FIB, 16, RID) + bytes(8))

    def test_fib_length_limit_and_route_mutation(self):
        route = ROUTES[0]
        original = msg.BGPFIBMsg(routes=[route] * msg.MAX_FIB_ENTRIES)
        wire = original.pack()
        self.assertEqual(original.size, 65528)
        self.assertEqual(len(wire), 65528)
        self.assertEqual(len(msg.BGPFIBMsg(msg=wire).routes), msg.MAX_FIB_ENTRIES)
        with self.assertRaises(ValueError):
            msg.BGPFIBMsg(routes=[route] * (msg.MAX_FIB_ENTRIES + 1))
        original.routes.append(route)
        with self.assertRaises(ValueError):
            original.pack()
        original.routes = [route]
        self.assertEqual(len(original.pack()), 24)
        self.assertEqual(original.size, 20)

    def test_masks_and_prefix_bits(self):
        routes = [
            msg.FIBRoute("192.0.2.1/32", "10.0.0.2"),
            msg.FIBRoute("192.0.2.1/24", "10.0.0.3"),
        ]
        wire = msg.BGPFIBMsg(routes=routes).pack()
        self.assertEqual(msg.BGPFIBMsg(msg=wire).routes, routes)
        invalid = bytearray(FIXTURES["fib_one"])
        invalid[12:16] = bytes.fromhex("ff00ff00")
        with self.assertRaises(ValueError):
            msg.BGPFIBMsg(msg=invalid)
        for prefix in (-1, 33):
            with self.assertRaises(ValueError):
                msg.cidr_to_netmask(prefix)

    def test_padding_is_ignored_and_emitted_as_zero(self):
        for name, cls, index in (
            ("state", msg.BGPStateMsg, 13),
            ("announce", msg.BGPAnnounce, 12),
            ("fib_one", msg.BGPFIBMsg, 20),
        ):
            wire = bytearray(FIXTURES[name])
            wire[index:] = b"\xff" * (len(wire) - index)
            self.assertEqual(cls(msg=wire).pack(), FIXTURES[name])


if __name__ == "__main__":
    unittest.main()
