import time
from horse import *

from random import randint

import sys
NUM = int(sys.argv[1])

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
#Create OpenFlow switch
routers = []
hosts = []
for i in range(0, NUM):
    r = Router("r%s" % (i+1))
    r.add_port(port = 1,  eth_addr = rand_mac(), ip = "10.0.0.%s" % (i+1),
               netmask = "255.255.0.0")
    # Add BGP
    bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/conf.ini%s" % (i+1))
    # bgp.add_advertised_prefix("1%s0.0.0.0/16" % i)
    bgp.add_advertised_prefixes(prefixes = gen_prefixes(i))

    bgp.set_maximum_paths(2)
    r.add_protocol(bgp)
    routers.append(r)
    # Connect to host
    h = Host("h%s" % (i+1))
    h.add_port(port = 1, eth_addr = rand_mac(), ip = "1%s0.0.0.1" % (i), 
               netmask = "255.255.255.0")
    r.add_port(port = 2, eth_addr = rand_mac(), ip = "1%s0.0.0.2" % (i),
               netmask = "255.255.255.0")
    h.set_default_gw("1%s0.0.0.2" % (i), 1)
    topo.add_node(r);
    topo.add_node(h)
    topo.add_link(r, h, 2, 1)
    hosts.append(h)

# r3 = Router("r3")
# r3.add_port(port = 1,  eth_addr = "00:00:00:00:03:00", ip = "10.0.0.3", netmask = "255.255.0.0")
# r3.add_protocol(type = 179, config_file = "/home/vagrant/horse/python/topos/conf.ini3")

time = 10000000

for i in range(0, len(routers), 2):
    topo.add_link(routers[i], routers[i+1], 1, 1)
    hosts[i].ping("1%s0.0.0.1" % (i+1), time)
    hosts[i+1].ping("1%s0.0.0.1" % (i), time)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 2000000, end_time = 60000000,
        log_level = LogLevels.LOG_INFO)

sim.start()

