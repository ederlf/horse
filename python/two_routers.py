from horse import *

topo = Topology()
#Create OpenFlow switch
r1 = Router("r1")
r1.add_port(port = 1,  eth_addr = "00:00:00:00:01:00", ip = "10.0.0.1", netmask = "255.255.0.0")
r1.add_port(port = 2,  eth_addr = "00:00:00:00:11:00", ip = "10.0.1.1", netmask = "255.255.0.0")

bgp1 = BGP(config_file = "/home/vagrant/horse/python/topos/config/conf.ini1")
bgp1.add_advertised_prefix("140.0.0.0/16")
r1.add_protocol(bgp1)

r2 = Router("r2")
r2.add_port(port = 1,  eth_addr = "00:00:00:00:02:00", ip = "10.0.0.2", netmask = "255.255.0.0")

bgp2 = BGP(config_file = "/home/vagrant/horse/python/topos/config/conf.ini2")
bgp2.add_advertised_prefix("130.0.0.0/16")
r2.add_protocol(bgp2)

# r3 = Router("r3")
# r3.add_port(port = 1,  eth_addr = "00:00:00:00:03:00", ip = "10.0.0.3", netmask = "255.255.0.0")
# r3.add_protocol(type = 179, config_file = "/home/vagrant/horse/python/topos/conf.ini3")


topo.add_node(r1)
topo.add_node(r2)
# topo.add_node(r3)
topo.add_link(r1, r2, 1, 1)
# topo.add_link(r1, r3, 2, 1)

# Create hosts
# h1 = Host()
# h2 = Host()
# h1.add_port(1, "00:00:00:00:00:01", ip = "10.0.0.1", 
#             netmask = "255.255.255.0" )
# h2.add_port(1, "00:00:00:00:00:02", ip = "10.0.0.2", 
#             netmask = "255.255.255.0")

# #Specify application
# h1.ping("10.0.0.2", 500000)

# # Create and add nodes to Topology
# 
# topo.add_node(sw1)
# topo.add_node(h1)
# topo.add_node(h2)

# #Connect hosts to the switch
# topo.add_link(sw1, h1, 1, 1)
# topo.add_link(sw1, h2, 2, 1)

# # Start the experiment
sim = Sim(topo, ctrl_interval = 100000, end_time = 5000000,
        log_level = LogLevels.LOG_INFO)
sim.start()
