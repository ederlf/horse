import sys

# path = "/home/vagrant/horse/python/"
# if path not in sys.path:
#     sys.path.append(path)

from horse.horse import *
from horse.router import BGPNeighbor, BGP
from random import randint

def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255)
)

topo = Topology()
#Create OpenFlow switch
bgp1 = [BGP(asn = 100,
            neighbors = [BGPNeighbor(200,"10.0.0.2",local_ip="10.0.0.1")],
            networks = ["140.0.0.0/16"])]
r1 = Router("r1", daemon = "exabgp", *bgp1)
r1.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.0.0.1", netmask = "255.255.0.0")
r1.add_port(port = 2,  eth_addr = rand_mac(), ip = "140.0.0.2", netmask = "255.255.0.0")

bgp2 = [BGP(asn=200, neighbors=[BGPNeighbor(100,"10.0.0.1",local_ip="10.0.0.2")],
            networks=["130.0.0.0/16"])]

r2 = Router("r2", daemon= "exabgp", *bgp2)
r2.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.0.0.2", netmask = "255.255.0.0")
r2.add_port(port = 2,  eth_addr = rand_mac(), ip = "130.0.0.2", netmask = "255.255.0.0")

topo.add_node(r1)
topo.add_node(r2)

h1 = Host('h1')
h1.add_port(port = 1,  eth_addr = rand_mac(), ip = "140.0.0.1", netmask = "255.255.0.0")
h1.set_default_gw("140.0.0.2", 1)

h2 = Host('h2')
h2.add_port(port = 1,  eth_addr = rand_mac(), ip = "130.0.0.1", netmask = "255.255.0.0")
h2.set_default_gw("130.0.0.2", 1)

topo.add_node(h1)
topo.add_node(h2)

# topo.add_node(r3)
topo.add_link(r1, r2, 1, 1)
topo.add_link(r1, h1, 2, 1)
topo.add_link(r2, h2, 2, 1)

h1.ping("130.0.0.1", 5000000)
h2.ping("140.0.0.1", 5000000)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 2000000, end_time = 10000000,
        log_level = LogLevels.LOG_INFO)
sim.start()
