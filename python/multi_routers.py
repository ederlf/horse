import time
from horse import *

import sys
NUM = int(sys.argv[1])

topo = Topology()
#Create OpenFlow switch
routers = []
for i in range(0, NUM):
    r = Router("r%s" % (i+1))
    r.add_port(port = 1,  eth_addr = "00:00:00:00:01:00", ip = "10.0.0.%s" % (i+1),
               netmask = "255.255.0.0")
    bgp = BGP(config_file = "/home/vagrant/horse/python/topos/config/conf.ini%s" % (i+1))
    bgp.add_advertised_prefix("1%s0.0.0.0/16" % i)
    r.add_protocol(bgp)
    routers.append(r)

# r3 = Router("r3")
# r3.add_port(port = 1,  eth_addr = "00:00:00:00:03:00", ip = "10.0.0.3", netmask = "255.255.0.0")
# r3.add_protocol(type = 179, config_file = "/home/vagrant/horse/python/topos/conf.ini3")

for i in range(0, len(routers), 2):
    topo.add_node(routers[i])
    topo.add_node(routers[i+1])
    topo.add_link(routers[i], routers[i+1], 1, 1)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 3000000, end_time = 20000000,
        log_level = LogLevels.LOG_INFO)

sim.start()

