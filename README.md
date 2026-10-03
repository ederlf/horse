# Horse: A hybrid tool for network reproduction

[![Build Status](https://travis-ci.org/ederlf/horse.svg?branch=master)](https://travis-ci.org/ederlf/horse)

Horse is a hybrid simulation tool to reproduce network experiments. 
It employs emulation for the control plane and simulation for the data plane.

For now, only the SDN version is available in this repository. It supports controllers
running [OpenFlow 1.3](https://www.opennetworking.org/images/stories/downloads/sdn-resources/onf-specifications/openflow/openflow-spec-v1.3.1.pdf) applications. 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Prerequisites

Horse is implemented in C with Python bindings implemented in Cython.
The modern build requires Linux, a C11 compiler, CMake 3.20+, Git, pkg-config,
libevent with pthread support, libpcap, and Python 3.10+ with development headers.

On Debian/Ubuntu:

```bash
$ sudo apt-get install build-essential cmake git pkg-config libevent-dev libpcap-dev python3-dev python3-venv
```

On Fedora:

```bash
$ sudo dnf install gcc cmake git pkgconf-pkg-config libevent-devel libpcap-devel python3-devel
```

### Installing

CMake builds LOCI from the bundled sources and downloads the libcfluid_base
fork and cmockery at pinned commits. No manually installed libfluid or
precompiled LOCI library is required.

```bash
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
$ cmake --build build --parallel 2
$ ctest --test-dir build --output-on-failure
```

Build the Python bindings in a virtual environment:

```bash
$ python3 -m venv .venv
$ . .venv/bin/activate
$ python -m pip install -e .
```

The Python build uses the pinned tools in `pyproject.toml` and bundles
`libhorse.so` beside the extensions, so `LD_LIBRARY_PATH` is unnecessary.
For offline builds, supply local dependency checkouts using
`-DFETCHCONTENT_SOURCE_DIR_CFLUID=/path/to/libcfluid_base` and
`-DFETCHCONTENT_SOURCE_DIR_CMOCKERY=/path/to/cmockery` when configuring CMake.
The Python build accepts `FETCHCONTENT_SOURCE_DIR_CFLUID` as an environment
variable. Optional native installation uses
`cmake --install build --prefix /path/to/install`; Quagga finds `horse_daemon`
on PATH or through `HORSE_ROUTING_HELPER`.

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

The bundled learning-switch application was also validated with OS-Ken 4.2.0
in a separate Python 3.12 environment:

```bash
$ python3.12 -m venv /tmp/horse-controller
$ /tmp/horse-controller/bin/python -m pip install -r requirements-controller.txt
$ /tmp/horse-controller/bin/python tools/run_controller.py
```


Now, in another window, start the example of a linear topology with 2 hosts and 2 switches:

```bash
$ cd horse
$ python -m horse.linear 2
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

* **Eder Leao Moosmann** - main author [Personal Page](http://www.ederlm.de)

See also the list of [contributors](https://github.com/ederlf/horse/contributors) who participated in this project.

## License

This project is licensed under the BSD License - see the [LICENSE](https://github.com/ederlf/horse/docs/LICENSE) file for details

