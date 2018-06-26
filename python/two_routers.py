from horse import *
from router import ExaBGPRouter
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
r1 = ExaBGPRouter("r1")
r1.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.0.0.1", netmask = "255.255.0.0")
r1.add_port(port = 2,  eth_addr = rand_mac(), ip = "140.0.0.2", netmask = "255.255.0.0")
bgp1 = BGP(config_file = "/home/vagrant/horse/python/topos/config/new/conf.ini1")
bgp1.add_advertised_prefix("140.0.0.0/16")
r1.add_protocol(bgp1)

r2 = ExaBGPRouter("r2")
r2.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.0.0.2", netmask = "255.255.0.0")
r2.add_port(port = 2,  eth_addr = rand_mac(), ip = "130.0.0.2", netmask = "255.255.0.0")
bgp2 = BGP(config_file = "/home/vagrant/horse/python/topos/config/new/conf.ini2")
bgp2.add_advertised_prefix("130.0.0.0/16")
r2.add_protocol(bgp2)

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
