from horse import *
from random import randint

topo = Topology()

k = 101

h1 = Host()
h2 = Host()

h1.add_port(1, "00:00:00:00:00:01", ip = "10.0.0.1", 
            netmask = "255.255.255.0" )
h2.add_port(1, "00:00:00:00:00:02", ip = "10.0.0.2", 
            netmask = "255.255.255.0")

topo.add_node(h1)
topo.add_node(h2)

last_switch = None
for i in range(1, k):
    sw = SDNSwitch(i)
    sw.add_port(1, "00:00:00:00:01:00")
    sw.add_port(2, "00:00:00:00:02:00")
    topo.add_node(sw)
    if i == 1:
        topo.add_link(sw, h1, 1, 1, 0) #latency=randint(0,9))
    if i == k - 1:
        topo.add_link(sw, h2, 2, 1, 0) #latency=randint(0,9))
    if last_switch:
        topo.add_link(last_switch, sw, 2, 1)
    last_switch = sw

h1.ping("10.0.0.2", 5000000)
h2.ping("10.0.0.1", 10000000)
topo.start()


