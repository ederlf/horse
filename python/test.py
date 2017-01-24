from horse import *

sw1 = SDNSwitch(24)
sw2 = SDNSwitch(42)
topo = Topology()

# print topo.dp

topo.add_switch(sw1)
topo.add_switch(sw2)

topo.start()

# # print topo.dp

# # topo.dp = dp2
# # print topo.dp

# print topo.dp