from horse import *

#Create OpenFlow switch
sw1 = SDNSwitch(42)
sw1.add_port(port = 1, "00:00:00:00:01:00")
sw1.add_port(port = 2, "00:00:00:00:02:00")

# Create hosts
h1 = Host()
h2 = Host()
h1.add_port(1, "00:00:00:00:00:01", ip = "10.0.0.1", 
            netmask = "255.255.255.0" )
h2.add_port(1, "00:00:00:00:00:02", ip = "10.0.0.2", 
            netmask = "255.255.255.0")

#Specify application
h1.ping("10.0.0.2", 500000)

# Create and add nodes to Topology
topo = Topology()
topo.add_node(sw1)
topo.add_node(h1)
topo.add_node(h2)

#Connect hosts to the switch
topo.add_link(sw1, h1, 1, 1)
topo.add_link(sw1, h2, 2, 1)

# Start the experiment
topo.start()
