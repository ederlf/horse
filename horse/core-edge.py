from horse import *

topo = Topology()

edge1 = SDNSwitch(1)

topo.add_node(edge1)
edge1.add_port(5, "00:00:00:00:00:01")

for i in range(0, 4):
    sw = SDNSwitch(2 * (i**i))
    edge1.add_port(i+1, "00:00:00:00:00:01")
    sw.add_port(1, "00:00:00:00:00:01")
    topo.add_node(sw)
    topo.add_link(edge1, sw, i+1, 1)

topo.start()


