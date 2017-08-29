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

k = int(sys.argv[1]) + 1
hosts = []

topo = Topology()
# "00:00:00:00:00:0%s"% i
last_switch = None
for i in range(1, k):
    sw = SDNSwitch(i)
    h = Host()
    h.add_port(port = 1, eth_addr = rand_mac(), ip = "10.0.0.%s" % (i), 
               netmask = "255.255.255.0")
    sw.add_port(port = 1, eth_addr = "00:00:00:00:01:00")
    sw.add_port(port = 2, eth_addr = "00:00:00:00:02:00")
    sw.add_port(port = 3, eth_addr = "00:00:00:00:03:00")
    hosts.append(h)
    topo.add_node(h)
    topo.add_node(sw)
    topo.add_link(sw, h, 1, 1, latency = 0) #latency=randint(0,9))
    if last_switch:
        topo.add_link(last_switch, sw, 2, 3)
    last_switch = sw

time = 5000000
# hosts[0].ping("10.0.0.2", 5000000)
# hosts[0].ping("10.0.0.3", 6000000)
# hosts[1].ping("10.0.0.1", 7000000)
# hosts[1].ping("10.0.0.3", 8000000)
# hosts[2].ping("10.0.0.1", 9000000)
# hosts[2].ping("10.0.0.2", 10000000)
for i, h in enumerate(hosts):
    for z in range(1, k):
      if z != i + 1:
        h.ping("10.0.0.%s" % (z), time)
        time += 1000000



topo.start()

