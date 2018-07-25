from horse import *
from random import randint
import sys

from ixp_config import Config

def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255)
)

class SDXTopo(Topology):

    def hosts(self):
        hosts =  [h for h in self.nodes if isinstance(self.nodes[h], Host)]
        return hosts

    def __init__(self, config, *args, **kwargs):
        self.config = config
        # Describe Code
        # Set up data plane switch - this is the emulated router dataplane
        # Note: The controller needs to be configured with the specific driver that
        # will be attached to this switch.

        # IXP fabric
        # edge switches
        edge_switches = []

        for i in range(1, len(filter(lambda s: 'edge' in str(s), config.dpids))+1):
            dpid = format(config.dpids['edge-%s' % i], '016x')
            sw = SDNSwitch(long(dpid, 16))
            self.add_node(sw, name = 'edge-%s' % i)
            edge_switches.append(sw)
        # core switches
        core_switches = []
        for i in range(1, len(filter(lambda s: 'core' in str(s), config.dpids))+1):
            dpid = format(config.dpids['core-%s' % i], '016x')
            sw = SDNSwitch(long(dpid, 16))
            self.add_node(sw, name = 'core-%s' % i)
            edge_switches.append(sw)

        # connect edge to core links
        core_port = 1
        for edge_switch in edge_switches:
            edge_port = 1
            for core_switch in core_switches:
                edge_switch.add_port(port = edge_port, eth_addr = rand_mac())
                core_switch.add_port(port = core_port, eth_addr = rand_mac())
                self.add_link(edge_switch, core_switch, edge_port, core_port)
                edge_port += 1
            core_port += 1


        # Add Participants to the IXP
        # They will actually be just simple hosts 
        # enough for the load balancing experiment
        host_num = 1
        while host_num <= 800:
            for i, edge in enumerate(edge_switches):
                hname = "h%s" % host_num
                host = Host(name = hname)
                self.add_node(host, name = hname)
                host.add_port(port = 0, eth_addr = rand_mac(), ip = "10.0.0.%s" % (i), netmask = "255.255.255.0")
                edge.add_port(port=edge_port, eth_addr = rand_mac())
                self.add_link(edge, host, edge_port, 0)
                host_num += 1
            edge_port += 1


def main(argv):
    if len(argv) == 2:
        config_file = argv[1]
        config = Config(config_file)
        topo = SDXTopo(config)
        print topo.hosts()
    else:
        print "Missing config file"

if __name__ == "__main__":
    main(sys.argv)

# topo = Topology()
# # "00:00:00:00:00:0%s"% i
# last_switch = None
# for i in range(1, k):
#     sw = SDNSwitch(i)
#     h = Host("h%s" % i)
#     h.add_port(port = 1, eth_addr = rand_mac(), ip = "10.0.0.%s" % (i), 
#                netmask = "255.255.255.0")
#     sw.add_port(port = 1, eth_addr = "00:00:00:00:01:00")
#     sw.add_port(port = 2, eth_addr = "00:00:00:00:02:00")
#     sw.add_port(port = 3, eth_addr = "00:00:00:00:03:00")
#     hosts.append(h)
#     topo.add_node(h)
#     topo.add_node(sw)
#     topo.add_link(sw, h, 1, 1, latency = 0) #latency=randint(0,9))
#     if last_switch:
#         topo.add_link(last_switch, sw, 2, 3)
#     last_switch = sw

# time = 5000000
# for i, h in enumerate(hosts):
#     for z in range(1, k):
#       if z != i + 1:
#         h.ping("10.0.0.%s" % (z), time)
#         time += 1000000
# end_time = 5000000 + (len(hosts) * len(hosts)) * 1000000  
# sim = Sim(topo, ctrl_interval = 100000, end_time = end_time)
# sim.start()
