from horse import *

sw1 = SDNSwitch(24)
sw2 = SDNSwitch(42)
sw3 = SDNSwitch(42)

sw1.add_port(1, "00:00:00:00:00:01")
sw1.add_port(1, "00:00:00:00:00:02")

sw2.add_port(1, "00:00:00:00:01:00")
sw2.add_port(2, "00:00:00:00:02:00")

sw3.add_port(1, "00:00:00:01:00:00")
sw3.add_port(2, "00:00:00:02:00:00")

topo = Topology()

# print topo.dp

topo.add_switch(sw1)
topo.add_switch(sw2)
topo.add_switch(sw3)

topo.add_link(sw1, sw2, 2, 1)
topo.add_link(sw2, sw3, 2, 2)

topo.start()

# # print topo.dp

# # topo.dp = dp2
# # print topo.dp

# print topo.dp
