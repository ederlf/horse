# Horse: A hybrid tool for network reproduction

[![Build Status](https://travis-ci.org/ederlf/horse.svg?branch=master)](https://travis-ci.org/ederlf/horse)

Horse is a hybrid simulation tool to reproduce network experiments. 
It employs emulation for the control plane and simulation for the data plane.

For now, only the SDN version is available in this repository. It supports controllers
running [OpenFlow 1.3](https://www.opennetworking.org/images/stories/downloads/sdn-resources/onf-specifications/openflow/openflow-spec-v1.3.1.pdf) applications. 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Prerequisites

Horse is implemented in C with Python binding implemented in Cython.

The current version of this code is compiled on Ubuntu 14.0.4.3 with the following gcc version:
```
gcc version 4.8.4 (Ubuntu 4.8.4-2ubuntu1~14.04.3)
```

Installing Cython:

```bash
$ sudo apt-get install python-pip
$ pip install cython
```

The connection with OpenFlow controllers needs the installation of a C version from the base part of [Libfluid](http://opennetworkingfoundation.github.io/libfluid/). Consequently, libfluid dependencies are required:

```bash
$ sudo apt-get install autoconf libtool build-essential pkg-config
$ sudo apt-get install libevent-dev 

```

### Installing

The first step is to install the C version of libfluid base from the multi-client branch.

```bash
$ git clone https://github.com/ederlf/libcfluid_base.git
$ cd libcfluid_base
$ git checkout multi-client
$ ./autogen
$ ./configure
$ make
$ sudo make install
```

Then compile Horse:

```bash
$ cd horse
$ ./boot.sh
$ ./configure
$ make
```

Finally, generate the cython code:

```bash
$ cd horse/python
$ python setup.py build_ext --inplace
```

# Creating a Topology

The code shows how to create a linear topology, composed of N OpenFlow switches and hosts, and schedule pings between all the hosts. 

```python
from horse import *
from random import randint
import sys

def rand_mac():
    return "%02x:%02x:%02x:%02x:%02x:%02x" % (
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255),
        randint(0, 255)
)

k = int(sys.argv[1]) + 1
hosts = []

topo = Topology()
last_switch = None
for i in range(1, k):
    sw = SDNSwitch("s%s" %i, i)
    h = Host("h%s" % i)
    h.add_port(port = 1, eth_addr = rand_mac(), ip = "10.0.0.%s" % (i), 
               netmask = "255.255.255.0")
    sw.add_port(port = 1, eth_addr = "00:00:00:00:01:00")
    sw.add_port(port = 2, eth_addr = "00:00:00:00:02:00")
    sw.add_port(port = 3, eth_addr = "00:00:00:00:03:00")
    hosts.append(h)
    topo.add_node(h)
    topo.add_node(sw)
    topo.add_link(sw, h, 1, 1, latency = 0) #latency=randint(0,9))
    if last_switch:
        topo.add_link(last_switch, sw, 2, 3)
    last_switch = sw

# Start time for the pings in microseconds
time = 5000000
for i, h in enumerate(hosts):
    for z in range(1, k):
      if z != i + 1:
        # print "10.0.0.%s" % (z)
        h.ping("10.0.0.%s" % (z), time)
        time += 1000000
end_time = 5000000 + (len(hosts) * len(hosts)) * 1000000  
sim = Sim(topo, ctrl_interval = 100000, end_time = end_time, log_level = LogLevels.LOG_INFO)
sim.start()
```

## Running an example

In a future version, the simulator may have capabilities to start a controller, but for now, start the OpenFlow controller of your preference. The application needs to run **OpenFlow 1.3**. 
Example using [Ryu](https://github.com/osrg/ryu) controller.

```bash
$ git clone https://github.com/osrg/ryu.git
$ cd ryu; pip install .
$ ryu-manager ryu/ryu/app/simple_switch_13.py
```

Now, in another window, start the example of a linear topology with 2 hosts and 2 switches:

```bash
$ cd horse
$ python python/linear.py 2
```

The code executes ping between all hosts. You should see some informational logs about the start and conclusion of pings:

```
17:16:31 INFO  src/net/host.c:334: Flow Start time APP 5000000 Execs 1

17:16:31 INFO  src/net/app/ping.c:15: ECHO REQUEST from:a000001 to:a000002

17:16:31 INFO  src/net/app/ping.c:27: ECHO_REPLY ms:19.266 Src:a000001 Dst:a000002
```

## Built With

* [uthash](https://troydhanson.github.io/uthash/) - Hash table for C structures
* [loxigen](https://github.com/floodlight/loxigen) - For generation of OpenFlow structs
* [patricia](https://github.com/jsommers/pytricia) - Used for the routing tables
* [log](https://github.com/rxi/log.c) - Simple logging library
* [json.h](https://github.com/sheredom/json.h) - JSON parser for C and C++

## Authors

* **Eder Leao Fernandes** - *Initial work* - [Personal Page](http://www.eecs.qmul.ac.uk/~eleao/)

See also the list of [contributors](https://github.com/ederlf/horse/contributors) who participated in this project.

## License

This project is licensed under the BSD License - see the [LICENSE](https://github.com/ederlf/horse/docs/LICENSE) file for details

