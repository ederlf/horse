#!/usr/bin/python
import argparse
import os
import sys
import json
import socket
import random
import struct
import time


from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.node import RemoteController, OVSSwitch, Node
from sdnip import BgpRouter, SdnipHost

# path_mininet_config = '/home/vagrant/horse/experimental/mininet/mininet.cfg'
# default_isdx_config = '/home/vagrant/endeavour/examples/test-mh/config/sdx_global.cfg'

def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        random.randint(0, 255),
        random.randint(0, 255),
        random.randint(0, 255),
        random.randint(0, 255),
        random.randint(0, 255),
        random.randint(0, 255)
)

class SDXTopo(Topo):

    def __init__(self,  *args, **kwargs):
        Topo.__init__(self, *args, **kwargs)
        # Describe Code
        # Set up data plane switch - this is the emulated router dataplane
        # Note: The controller needs to be configured with the specific driver that
        # will be attached to this switch.

        # IXP fabric
        # edge switches
        edge_switches = []

        # for i in range(1, len(filter(lambda s: 'edge' in str(s), config.dpids))+1):
        #     edge_switches.append(self.addSwitch(
        #         'edge-%s' % i, dpid=format(config.dpids['edge-%s' % i], '016x')))

        # # core switches
        # core_switches = []
        # for i in range(1, len(filter(lambda s: 'core' in str(s), config.dpids))+1):
        #     core_switches.append(self.addSwitch(
        #         'core-%s' % i, dpid=format(config.dpids['core-%s' % i], '016x')))

        # # connect edge to core links
        # for edge_switch in edge_switches:
        #     for core_switch in core_switches:
        #         self.addLink(edge_switch, core_switch)

        # connect arp switch to edge 0 port 6
        #arp_switch = self.addSwitch('s2', dpid=format(config.dpids['arp-switch'], '016x'))
        #self.addLink(edge_switches[0], arp_switch, 6, 1)

        # connect route server to edge 0 port 7
        # route_server = self.addHost('x1', ip=self.config.route_server.ip, mac=self.config.route_server.mac, inNamespace=False)
        # self.addLink(edge_switches[0], route_server, 6)



        # Add node for ARP Proxy"
        # arp_proxy = self.addHost('x2', ip=self.config.arp_proxy.ip, mac=self.config.arp_proxy.mac, inNamespace=False)
        # self.addLink(edge_switches[0], arp_proxy, 7)

        # Add Participants to the IXP
        # Connects one participant to one of the four edge switches
        # Each participant consists of 1 quagga router PLUS
        # 1 host per network advertised behind quagga
 
        name = 'r1'
        peereth0 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.12.1/24"]}]
        peereth1 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.13.1/24"]}]
        intfs = {name + '-eth0': peereth0, name + '-eth1': peereth1}
        networks = ["192.168.3.0/24"]
        neighbors = [{'address': "10.1.12.2", 'as': 200}, {'address': "10.1.13.3", 'as': 300}]
        r1 = self.addParticipant(intfs,
                             name=name,
                            networks=networks,
                            asn=100,
                            neighbors=neighbors)

        name = 'r2'
        peereth0 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.12.2/24"]}]
        peereth1 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.24.3/24"]}]
        intfs = {name + '-eth0': peereth0, name + '-eth1': peereth1}
        neighbors = [{'address': "10.1.12.1", 'as': 100}, {'address': "10.1.24.1", 'as': 400}]
        r2 = self.addParticipant(intfs,
                             name=name,
                            networks=[],
                            asn=200, 
                            neighbors=neighbors)

        name = 'r3'
        peereth0 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.13.3/24"]}]
        peereth1 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.34.4/24"]}]
        intfs = {name + '-eth0': peereth0, name + '-eth1': peereth1}
        neighbors = [{'address': "10.1.13.1", 'as': 100}, {'address': "10.1.34.1", 'as': 400}]
        r3 = self.addParticipant(intfs,
                             name=name,
                            networks=[],
                            asn=300,
                            neighbors=neighbors)

        name = 'r4'
        peereth0 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.24.1/24"]}]
        peereth1 = [{'mac': rand_mac(), 'ipAddrs': ["10.1.34.1/24"]}]
        intfs = {name + '-eth0': peereth0, name + '-eth1': peereth1}
        networks = ["192.168.1.0/24"]
        neighbors = [{'address': "10.1.24.3", 'as': 200}, {'address': "10.1.34.4", 'as': 300}]
        r4 = self.addParticipant(intfs,
                             name=name,
                            networks=networks,
                            asn=400,
                            neighbors=neighbors)

        self.addLink(r1, r2, 0, 0)
        self.addLink(r1, r3, 1, 0)
        self.addLink(r4, r2, 0, 1)
        self.addLink(r4, r3, 1, 1)
        # for i in range(0, 50):
        #     name = "p%s" % (i + 4)
        #     mac = ':'.join(("%12x" % random.randint(0, 0xFFFFFFFFFFFF))[i:i+2] for i in range(0, 12, 2))
        #     ip = socket.inet_ntoa(struct.pack('>I', random.randint(1, 0xffffffff)))
        #     asn = i + 1
        #     networks = [socket.inet_ntoa(struct.pack('>I', random.randint(1, 0xffffffff))) + "/24" for x in range(0,2)]
        #     self.addParticipant(fabric=edge_switches[i%4],
        #                      name=name,
        #                     port=1,
        #                     mac=mac,
        #                     ip=ip,
        #                     networks=networks,
        #                     asn=asn)

    def addParticipant(self, intfs, name, networks, asn, neighbors):
        # Adds the interface to connect the router to the Route server
        # Adds 1 gateway interface for each network connected to the router
        for net in networks:
            eth = {'ipAddrs': [replace_ip(net, '254')]}  # ex.: 100.0.0.254
            i = len(intfs)
            intfs[name + '-eth' + str(i)] = eth

        # Set up the peer router
        print asn, networks, intfs
        peer = self.addHost(name,
                            intfDict=intfs,
                            asNum=asn,
                            neighbors=neighbors,
                            routes=networks,
                            cls=BgpRouter)
        # self.addLink(fabric, peer, port)

        # Adds a host connected to the router via the gateway interface
        i = 0
        for net in networks:
            i += 1
            ips = [replace_ip(net, '1')]  # ex.: 100.0.0.1/24
            hostname = 'h' + str(i) + '_' + name  # ex.: h1_a1
            host = self.addHost(hostname,
                               cls=SdnipHost,
                               ips=ips,
                               gateway=replace_ip(net, '254').split('/')[0])  # ex.: 100.0.0.254
            # Set up data plane connectivity
            self.addLink(peer, host, i+1)
        return peer

def replace_ip(network, ip):
    net, subnet = network.split('/')
    gw = net.split('.')
    gw[3] = ip
    gw = '.'.join(gw)
    gw = '/'.join([gw, subnet])
    return gw

if __name__ == "__main__":
    setLogLevel('info')

    # parser = argparse.ArgumentParser()
    # parser.add_argument('config', help='path of config file', nargs='?', default= default_isdx_config)
    # args = parser.parse_args()

    # locate config file
    # config_file = os.path.abspath(args.config)

    # config = Config(config_file)

    topo = SDXTopo()

    net = Mininet(topo=topo, controller=RemoteController, switch=OVSSwitch)

    net.start()

    CLI(net)
    #net.pingAll()
    # time.sleep(5)

    # h1 = net.get('h1_r1')	
    # h1.sendCmd('ping 192.168.1.1 -c 1')   
    # result = h1.waitOutput()
    # print result
    net.stop()

    info("done\n")
