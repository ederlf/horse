import time
from horse import *
from random import randint
import sys


def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255)
)

# Creates 254 prefixes
def gen_prefixes(host):
    prefixes = []
    for i in range(0, 200):
        prefixes.append("1%s0.%s.0.0/16" % (host,i))
    for i in range(0, 200):
        prefixes.append("1%s0.0.%s.0/24" % (host,i))
    return prefixes

topo = Topology()

#R1
r1 = Router("r1")
r1.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.1.12.1",
               netmask = "255.255.255.0")
r1.add_port(port = 2,  eth_addr = rand_mac(), ip = "10.1.13.1",
               netmask = "255.255.255.0")
r1.add_port(port = 3,  eth_addr = rand_mac(), ip = "192.168.3.3",
               netmask = "255.255.255.0")
r1.add_port(port = 4,  eth_addr = rand_mac(), ip = "192.168.3.4",
               netmask = "255.255.255.0")
bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/load_balance/conf.as100")
bgp.add_advertised_prefix("192.168.3.0/24")
bgp.set_maximum_paths(2)
bgp.set_relaxed_maximum_paths()
r1.add_protocol(bgp)


#R2
r2 = Router("r2")
r2.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.1.12.2",
               netmask = "255.255.255.0")
r2.add_port(port = 2,  eth_addr = rand_mac(), ip = "10.1.24.3",
               netmask = "255.255.255.0")
bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/load_balance/conf.as200")
r2.add_protocol(bgp)

#R3
r3 = Router("r3")
bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/load_balance/conf.as300")
r3.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.1.13.3",
               netmask = "255.255.255.0")
r3.add_port(port = 2,  eth_addr = rand_mac(), ip = "10.1.34.4",
               netmask = "255.255.255.0")
r3.add_protocol(bgp)

#R4
r4 = Router("r4")
r4.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.1.24.1",
               netmask = "255.255.255.0")
r4.add_port(port = 2,  eth_addr = rand_mac(), ip = "10.1.34.1",
               netmask = "255.255.255.0")
r4.add_port(port = 3,  eth_addr = rand_mac(), ip = "192.168.1.3",
               netmask = "255.255.255.0")
r4.add_port(port = 4,  eth_addr = rand_mac(), ip = "192.168.2.4",
               netmask = "255.255.255.0")

bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/load_balance/conf.as400")
bgp.add_advertised_prefix("192.168.1.0/24")
r4.add_protocol(bgp)

#Host 1
h1 = Host("h1")
h1.add_port(port = 1, eth_addr = rand_mac(), ip = "192.168.3.1",
           netmask = "255.255.255.0")
h1.set_default_gw("192.168.3.3", 1)

#Host 2
h2 = Host("h2")
h2.add_port(port = 1, eth_addr = rand_mac(), ip = "192.168.3.2",
           netmask = "255.255.255.0")
h2.set_default_gw("192.168.4.4", 1)

#Host 3
h3 = Host("h3")
h3.add_port(port = 1, eth_addr = rand_mac(), ip = "192.168.1.1",
           netmask = "255.255.255.0")
h3.set_default_gw("192.168.1.3", 1)

#Host 4
h4 = Host("h4")
h4.add_port(port = 1, eth_addr = rand_mac(), ip = "192.168.2.1",
           netmask = "255.255.255.0")
h4.set_default_gw("192.168.2.4", 1)

topo.add_node(r1);
topo.add_node(r2);
topo.add_node(r3);
topo.add_node(r4);
topo.add_node(h1)
topo.add_node(h2)
topo.add_node(h3)
topo.add_node(h4)

#Links
topo.add_link(r1, r2, 1, 1)
topo.add_link(r1, r3, 2, 1)
topo.add_link(r1, h1, 3, 1)
topo.add_link(r1, h2, 4, 1)
topo.add_link(r4, r2, 1, 2)
topo.add_link(r4, r3, 2, 2)
topo.add_link(r4, h3, 3, 1)
topo.add_link(r4, h4, 4, 1)

time = 10000000
h1.ping("192.168.1.1", time)
h1.udp("192.168.1.1", 11000000, duration = 5, rate = 1,
       dst_port=randint(5000, 60000), src_port=randint(5000, 60000))
h1.udp("192.168.1.1", 11000000, duration = 5,
       rate = 1, dst_port=randint(5000, 60000), src_port=randint(5000, 60000))
# h2.ping("192.168.5.1", time)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 2000000, end_time = 30000000,
        log_level = LogLevels.LOG_INFO)

sim.start()

