import signal
import sys
from random import randint

from horse import Host, LogLevels, SDNSwitch, Sim, Topology


def signal_handler(signal, frame):
    print("You pressed Ctrl+C!")
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)


def rand_mac():
    return f"{randint(0, 255):02x}:{randint(0, 255):02x}:{randint(0, 255):02x}:{randint(0, 255):02x}:{randint(0, 255):02x}:{randint(0, 255):02x}"


k = int(sys.argv[1]) + 1
hosts = []

topo = Topology()
# "00:00:00:00:00:0%s"% i
last_switch = None
for i in range(1, k):
    sw = SDNSwitch(f"s{i}", i)
    h = Host(f"h{i}")
    h.add_port(port=1, eth_addr=rand_mac(), ip=f"10.0.0.{i}", netmask="255.255.255.0")
    sw.add_port(port=1, eth_addr="00:00:00:00:01:00")
    sw.add_port(port=2, eth_addr="00:00:00:00:02:00")
    sw.add_port(port=3, eth_addr="00:00:00:00:03:00")
    hosts.append(h)
    topo.add_node(h)
    topo.add_node(sw)
    topo.add_link(sw, h, 1, 1, latency=0)  # latency=randint(0,9))
    if last_switch:
        topo.add_link(last_switch, sw, 2, 3)
    last_switch = sw

time = 5000000
for i, h in enumerate(hosts):
    for z in range(1, k):
        if z != i + 1:
            # print "10.0.0.%s" % (z)
            h.ping(f"10.0.0.{z}", time)
            time += 1000000
end_time = 5000000 + (len(hosts) * len(hosts)) * 1000000
sim = Sim(topo, ctrl_interval=100000, end_time=end_time, log_level=LogLevels.LOG_INFO)
sim.start()
