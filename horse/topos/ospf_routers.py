import sys

# path = "/home/vagrant/horse/python/"
# if path not in sys.path:
#     sys.path.append(path)

from horse.horse import *
from horse.router import OSPF, OSPFInterface, OSPFNet
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

DAEMON = "quagga"

topo = Topology()
#Create OpenFlow switch


linknet = OSPFNet(network = '10.10.10.0/30')
net = OSPFNet(network = '192.168.1.0/24')
ifaces = [OSPFInterface(name = 'r1-eth1', hello_interval=1)]
proto = [OSPF(router_id='10.10.10.1', networks=[net, linknet], interfaces=ifaces)]
r1 = Router("r1", daemon = DAEMON, *proto)
r1.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.10.10.1", netmask = "255.255.255.252")
r1.add_port(port = 2,  eth_addr = rand_mac(), ip = "192.168.1.2", netmask = "255.255.255.0")

net = OSPFNet(network = '172.17.1.0/24')
ifaces = [OSPFInterface(name = 'r2-eth1', hello_interval=1)]
proto = [OSPF(router_id='10.10.10.2', networks=[net, linknet], interfaces=ifaces)]
r2 = Router("r2", daemon= DAEMON, *proto)
r2.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.10.10.2", netmask = "255.255.255.252")
r2.add_port(port = 2,  eth_addr = rand_mac(), ip = "172.17.1.2", netmask = "255.255.255.0")

topo.add_node(r1)
topo.add_node(r2)

h1 = Host('h1')
h1.add_port(port = 1,  eth_addr = rand_mac(), ip = "192.168.1.1", netmask = "255.255.255.0")
h1.set_default_gw("192.168.1.2", 1)

h2 = Host('h2')
h2.add_port(port = 1,  eth_addr = rand_mac(), ip = "172.17.1.1", netmask = "255.255.255.0")
h2.set_default_gw("172.17.1.2", 1)

topo.add_node(h1)
topo.add_node(h2)

# topo.add_node(r3)
topo.add_link(r1, r2, 1, 1)
topo.add_link(r1, h1, 2, 1)
topo.add_link(r2, h2, 2, 1)

h1.ping("172.17.1.1", 10000000)
h2.ping("192.168.1.1", 10000000)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 5000000, end_time = 20000000,
        log_level = LogLevels.LOG_DEBUG)
sim.start()
