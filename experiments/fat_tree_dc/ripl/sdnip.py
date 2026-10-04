#!/usr/bin/python

# Libraries for creating SDN-IP networks

import socket
import time

from mininet.log import debug
from mininet.node import Host, OVSSwitch

# Import the ONOS classes from onos.py in the ONOS repository
# if not 'ONOS_ROOT' in os.environ:
#    print 'ONOS_ROOT is not set.'
#    print 'Try running the script with \'sudo -E\' to pass your environment in.'
#    sys.exit(1)

# onos_path = os.path.join(os.path.abspath(os.environ['ONOS_ROOT']), 'tools/test/topos/onos.py')
# onos = imp.load_source('onos', onos_path)
# from onos import ONOS


class L2OVSSwitch(OVSSwitch):
    "An OVS switch that acts like a legacy L2 learning switch"

    def __init__(self, name, **params):
        OVSSwitch.__init__(self, name, failMode="standalone", **params)

    def start(self, controllers):
        # This switch should always have no controllers
        OVSSwitch.start(self, [])


class SdnipHost(Host):
    def __init__(self, name, ips, gateway, *args, **kwargs):
        super().__init__(name, *args, **kwargs)

        self.ips = ips
        self.gateway = gateway

    def config(self, **kwargs):
        Host.config(self, **kwargs)

        debug(f"configuring route {self.gateway}")

        self.cmd(f"ip addr flush dev {self.defaultIntf()}")
        for ip in self.ips:
            self.cmd(f"ip addr add {ip} dev {self.defaultIntf()}")

        self.cmd(f"ip route add default via {self.gateway}")


class Router(Host):
    def __init__(self, name, intfDict, *args, **kwargs):
        super().__init__(name, **kwargs)

        self.intfDict = intfDict

    def config(self, **kwargs):
        super(Host, self).config(**kwargs)

        self.cmd("sysctl net.ipv4.ip_forward=1")

        for intf, configs in list(self.intfDict.items()):
            self.cmd(f"ip addr flush dev {intf}")
            self.cmd(f"sysctl net.ipv4.conf.{intf}.rp_filter=0")
            if not isinstance(configs, list):
                configs = [configs]

            for attrs in configs:
                # Configure the vlan if there is one
                if "vlan" in attrs:
                    vlanName = "{}.{}".format(intf, attrs["vlan"])
                    self.cmd(
                        "ip link add link {} name {} type vlan id {}".format(
                            intf, vlanName, attrs["vlan"]
                        )
                    )
                    addrIntf = vlanName
                    self.cmd(
                        "sysctl net.ipv4.conf.{}/{}.rp_filter=0".format(
                            intf, attrs["vlan"]
                        )
                    )
                else:
                    addrIntf = intf

                # Now configure the addresses on the vlan/native interface
                if "mac" in attrs:
                    self.cmd(f"ip link set {addrIntf} down")
                    self.cmd("ip link set {} address {}".format(addrIntf, attrs["mac"]))
                    self.cmd(f"ip link set {addrIntf} up")
                for addr in attrs["ipAddrs"]:
                    self.cmd(f"ip addr add {addr} dev {addrIntf}")


class BgpRouter(Router):
    def __init__(
        self,
        name,
        intfDict,
        asNum,
        neighbors,
        routes=None,
        configFile=None,
        zebraConfFile=None,
        speaker="quagga",
        runDir="/tmp",
        *args,
        **kwargs,
    ):
        super().__init__(name, intfDict, **kwargs)

        self.runDir = runDir
        self.routes = [] if routes is None else routes
        self.speaker = speaker

        self.asNum = asNum
        self.neighbors = neighbors

        if self.speaker == "quagga":
            if configFile is not None:
                self.quaggaConfFile = configFile
                self.zebraConfFile = zebraConfFile
            else:
                self.quaggaConfFile = f"{runDir}/quagga{name}.conf"
                self.zebraConfFile = f"{runDir}/zebra{name}.conf"
            self.socket = f"{self.runDir}/zebra{self.name}.api"
            self.quaggaPidFile = f"{self.runDir}/quagga{self.name}.pid"
            self.zebraPidFile = f"{self.runDir}/zebra{self.name}.pid"
        elif self.speaker == "exabgp":
            print("Will run exabgp")
            if configFile:
                self.exabgpConfFile = configFile
            else:
                self.exabgpConfFile = f"{self.runDir}/{self.name}.conf"
            self.bindports = []
        self.generateConfig()

    def config(self, **kwargs):
        super().config(**kwargs)
        if self.speaker == "quagga":
            self.cmd(
                f"zebra -d -f {self.zebraConfFile} -z {self.socket} -i {self.zebraPidFile}"
            )
            while True:
                try:
                    s = socket.socket(
                        socket.AF_UNIX, socket.SOCK_STREAM
                    )  # @UndefinedVariable
                    s.connect(self.socket)
                    # print 'connected - breaking'
                    break
                except Exception:
                    # print' ERROR: ' + repr(e)
                    time.sleep(0.1)
            # print 'zebra ready'
            self.cmd(
                f"bgpd -d -f {self.quaggaConfFile} -z {self.socket} -i {self.quaggaPidFile}"
            )
        else:
            ports = " ".join(self.bindports)
            cmd = f'env exabgp.daemon.daemonize=true exabgp.tcp.bind="{ports}" exabgp.log.level=INFO exabgp.log.destination=syslog exabgp {self.exabgpConfFile}'
            self.cmd(cmd)

    def generateConfig(self):
        if self.speaker == "quagga":
            self.generateQuagga()
            self.generateZebra()
        elif self.speaker == "exabgp":
            self.generateExaBGP()
        else:
            print("Unknown BGP speaker")

    def generateExaBGP(self):
        configFile = open(self.exabgpConfFile, "w+")

        def writeLine(indent, line):
            intentStr = ""
            for _ in range(0, indent):
                intentStr += "  "
            configFile.write(f"{intentStr}{line}\n")

        def getRouterId(interfaces):
            for intfAttributesList in interfaces.values():
                if not isinstance(intfAttributesList, list):
                    continue
                # Try use the first set of attributes, but if using vlans they might not have addresses
                intfAttributes = (
                    intfAttributesList[1]
                    if not intfAttributesList[0]["ipAddrs"]
                    else intfAttributesList[0]
                )
                return intfAttributes["ipAddrs"][0].split("/")[0]

        # writeLine(0, 'process client{')
        # writeLine(2, 'run /usr/bin/python /home/vagrant/horse/python/test.py %s;' % self.name)
        # writeLine(2, 'encoder json;\n}\n')

        writeLine(0, "template {")
        writeLine(2, "neighbor router {")
        writeLine(4, "family {")
        writeLine(6, "ipv4 unicast;")
        writeLine(4, "}")
        writeLine(4, "api speaking {")
        # writeLine(6, 'processes [client];')
        writeLine(6, "neighbor-changes;")
        writeLine(6, "receive {")
        writeLine(8, "parsed;")
        writeLine(8, "update;")
        writeLine(8, "notification;")
        writeLine(8, "open;")
        writeLine(6, "}")
        writeLine(4, "}")
        writeLine(4, f"local-as {self.asNum};")
        writeLine(4, "manual-eor true;")
        writeLine(2, "}")
        writeLine(0, "}\n")

        router_id = getRouterId(self.intfDict)
        for neighbor in self.neighbors:
            writeLine(0, "neighbor {} {{".format(neighbor["address"]))
            writeLine(2, "inherit router;")
            writeLine(2, "peer-as {};".format(neighbor["as"]))
            writeLine(2, f"router-id {router_id};")
            writeLine(2, "local-address {};".format(neighbor["local-address"]))
            writeLine(2, "group-updates false;")
            writeLine(2, "adj-rib-in false;")
            writeLine(0, "}\n")
            self.bindports.append(neighbor["local-address"])

        configFile.close()

    def generateQuagga(self):
        configFile = open(self.quaggaConfFile, "w+")

        def writeLine(indent, line):
            intentStr = ""
            for _ in range(0, indent):
                intentStr += "  "
            configFile.write(f"{intentStr}{line}\n")

        def getRouterId(interfaces):
            for intfAttributesList in interfaces.values():
                if not isinstance(intfAttributesList, list):
                    continue
                # Try use the first set of attributes, but if using vlans they might not have addresses
                intfAttributes = (
                    intfAttributesList[1]
                    if not intfAttributesList[0]["ipAddrs"]
                    else intfAttributesList[0]
                )
                return intfAttributes["ipAddrs"][0].split("/")[0]

        # print("FDP %s" % getRouterId(self.intfDict))
        # print("  FDP %s" % str(self.intfDict))
        writeLine(0, f"hostname {self.name}")
        writeLine(0, "password {}".format("sdnip"))
        writeLine(0, f"log file /var/run/quagga/q_{self.asNum}")
        writeLine(0, "debug bgp")
        writeLine(0, "!")
        writeLine(0, f"router bgp {self.asNum}")
        writeLine(1, f"bgp router-id {getRouterId(self.intfDict)}")
        writeLine(1, "bgp bestpath as-path multipath-relax")
        writeLine(1, "timers bgp {}".format("3 9"))
        writeLine(1, "!")
        writeLine(1, "maximum-paths 64")

        for neighbor in self.neighbors:
            writeLine(
                1,
                "neighbor {} remote-as {}".format(neighbor["address"], neighbor["as"]),
            )
            writeLine(1, "neighbor {} ebgp-multihop".format(neighbor["address"]))
            writeLine(
                1, "neighbor {} timers connect {}".format(neighbor["address"], "5")
            )
            writeLine(
                1,
                "neighbor {} advertisement-interval {}".format(
                    neighbor["address"], "1"
                ),
            )
            if "port" in neighbor:
                writeLine(
                    1,
                    "neighbor {} port {}".format(neighbor["address"], neighbor["port"]),
                )
            writeLine(1, "!")

        for route in self.routes:
            writeLine(1, f"network {route}")

        configFile.close()

    def generateZebra(self):
        configFile = open(self.zebraConfFile, "w+")
        configFile.write(f"hostname {self.name}\n")
        configFile.write("password {}\n".format("sdnip"))
        configFile.write(f"log file /var/run/quagga/z_{self.asNum} debugging\n")
        configFile.close()

    def terminate(self):
        if self.speaker == "quagga":
            self.cmd(f"ps ax | grep '{self.socket}' | awk '{{print $1}}' | xargs kill")
        elif self.speaker == "exabgp":
            self.cmd("pkill exabgp")

        super().terminate()


# This module provides router classes; runnable topologies live in mn.py.
