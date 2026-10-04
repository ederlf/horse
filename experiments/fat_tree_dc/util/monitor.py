import re
from subprocess import PIPE, Popen
from time import sleep, time

default_dir = "."


def monitor_qlen(iface, interval_sec=0.01, fname=f"{default_dir}/qlen.txt"):
    pat_queued = re.compile(r"backlog\s[^\s]+\s([\d]+)p")
    cmd = f"tc -s qdisc show dev {iface}"
    ret = []
    open(fname, "w").write("")
    while 1:
        p = Popen(cmd, shell=True, stdout=PIPE)
        output = p.stdout.read()
        # Not quite right, but will do for now
        matches = pat_queued.findall(output)
        if matches and len(matches) > 1:
            ret.append(matches[1])
            t = f"{time():f}"
            open(fname, "a").write(t + "," + matches[1] + "\n")
        sleep(interval_sec)
    # open('qlen.txt', 'w').write('\n'.join(ret))
    return


def monitor_count(
    ipt_args="--src 10.0.0.0/8",
    interval_sec=0.01,
    fname=f"{default_dir}/bytes_sent.txt",
    chain="OUTPUT",
):
    cmd = f"iptables -I {chain} 1 {ipt_args} -j RETURN"
    # We always erase the first rule; will fix this later
    Popen(f"iptables -D {chain} 1", shell=True).wait()
    # Add our rule
    Popen(cmd, shell=True).wait()
    open(fname, "w").write("")
    cmd = f"iptables -vnL {chain} 1 -Z"
    while 1:
        p = Popen(cmd, shell=True, stdout=PIPE)
        output = p.stdout.read().strip()
        values = output.split(" ")
        if len(values) > 2:
            t = f"{time():f}"
            pkts, bytes = values[0], values[1]
            open(fname, "a").write(",".join([t, pkts, bytes]) + "\n")
        sleep(interval_sec)
    return


def monitor_devs(
    dev_pattern="^s", fname=f"{default_dir}/bytes_sent.txt", interval_sec=0.01
):
    """Aggregates (sums) all txed bytes and rate (in Mbps) from
    devices whose name matches @dev_pattern and writes to @fname"""
    pat = re.compile(dev_pattern)
    spaces = re.compile("\s+")
    open(fname, "w").write("")
    prev_tx = {}
    while 1:
        lines = open("/proc/net/dev").read().split("\n")
        t = str(time())
        total = 0
        for line in lines:
            line = spaces.split(line.strip())
            iface = line[0]
            if pat.match(iface) and len(line) > 9:
                tx_bytes = int(line[9])
                total += tx_bytes - prev_tx.get(iface, tx_bytes)
                prev_tx[iface] = tx_bytes
        open(fname, "a").write(
            ",".join([t, str(total * 8 / interval_sec / 1e6), str(total)]) + "\n"
        )
        sleep(interval_sec)
    return


def monitor_devs_ng(fname=f"{default_dir}/txrate.txt", interval_sec=0.01):
    """Uses bwm-ng tool to collect iface tx rate stats.  Very reliable."""
    cmd = (
        f"sleep 1; bwm-ng -t {interval_sec * 1000} -o csv "
        f"-u bits -T rate -C ',' > {fname}"
    )
    Popen(cmd, shell=True).wait()


def monitor_cpu(fname=f"{default_dir}/cpu.txt"):
    cmd = f'(top -b -p 1 -d 1 | grep --line-buffered "^Cpu") > {fname}'
    # BL: Disabling until we reinstantiate attachment using setns.
    # if container is not None:
    #    cmd = ("(top -b -p 1 -d 1 | "
    #           "grep --line-buffered \\\"^Cpu\\\") > %s" % fname)
    #    cmd = "lxc-execute -n %s -- bash -c \"%s\"" % (container, cmd)
    Popen(cmd, shell=True).wait()
