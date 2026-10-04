"""Exercise native bindings, message encoding, and existing topology generators."""

import os
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from horse.router import BGP, BGPNeighbor

from experiments.fat_tree_dc.ripl.horsebgptopo import HorseBGPFatTreeTopo
from experiments.fat_tree_dc.ripl.horsedctopo import FatTreeTopo
from horse import Host, Router, SDNSwitch, SimConfig, Topology
from horse.bgp_peer.rib import RibTuple
from horse.msg import BGPFIBMsg, BGPStateMsg, int2ip, ip2int


def main():
    host = Host("host")
    host.add_port(1, "00:00:00:00:00:01", ip="10.0.0.1", netmask="255.255.255.0")
    host.ping("10.0.0.2", 1000000)
    host.udp("10.0.0.2", 2000000, duration=2, rate=1)
    switch = SDNSwitch("switch", 42)
    switch.add_port(1, "00:00:00:00:00:02")
    topology = Topology()
    topology.add_node(host)
    topology.add_node(switch)
    topology.add_link(host, switch, 1, 1)
    assert host.name == "host" and switch.name == "switch"
    assert topology.dps_num == 1 and topology.links_num == 1
    config = SimConfig()
    config.set_end_time(3000000)
    assert config.end_time == 3000000
    assert int2ip(ip2int("10.0.0.1")) == "10.0.0.1"
    message = BGPStateMsg(local_id=1, peer_id=2, state=BGPStateMsg.BGP_STATE_UP)
    decoded = BGPStateMsg(msg=message.pack())
    assert decoded.router_id == 1 and decoded.peer_id == 2
    assert decoded.state == BGPStateMsg.BGP_STATE_UP
    route = RibTuple(
        "10.0.0.0/24", "10.0.0.2", 200, "10.0.0.2", "igp", [], [], 0, False
    )
    assert len(BGPFIBMsg(local_id=1, routes=[route]).pack()) == 24
    for k, hosts, switches, links in ((2, 2, 5, 6), (4, 16, 20, 48)):
        tree = FatTreeTopo(k=k)
        assert len(tree.hosts()) == hosts and len(tree.switches()) == switches
        assert tree.links_num == links
    original = os.getcwd()
    with tempfile.TemporaryDirectory(prefix="horse-bindings-") as directory:
        try:
            os.chdir(directory)
            bgp = BGP(
                asn=100,
                router_id="10.0.0.1",
                neighbors=[BGPNeighbor(200, "10.0.0.2", local_ip="10.0.0.1")],
                networks=["140.0.0.0/16"],
            )
            router = Router("r1", bgp, runDir=directory)
            router.add_port(
                1, "00:00:00:00:00:03", ip="10.0.0.1", netmask="255.255.255.0"
            )
            assert router.name == "r1"
            assert "router bgp 100" in Path(directory, "bgpdr1.conf").read_text()
            tree = HorseBGPFatTreeTopo(k=4)
            assert len(tree.nodes) == 36 and tree.links_num == 48
        finally:
            os.chdir(original)
    print("PASS native bindings, message encoding, SDN/BGP topology generation")


if __name__ == "__main__":
    main()
