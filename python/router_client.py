#!/usr/bin/env python
import os
import sys
from threading import Thread, Lock
import Queue
import syslog
import socket
import struct
import errno
import time

# neighbor 127.0.0.1 announce route 1.0.0.0/24 next-hop 101.1.101.1

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if path not in sys.path:
    sys.path.append(path)

from bgp_peer.peer import BGPPeer
    
def _recv(conn, q):
    readlen = 0
    pos = 0
    size = 0
    buf = None
    while True:
        # It just started, read header
        if pos == 0:
            try:
                readlen = 8
                msg = conn.recv(readlen)
                pos += 8
                msg_type, size = struct.unpack("!HH",msg[:4])
                buf = msg
            except socket.error, e:
                err = e.args[0]
                if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
                    syslog.syslog( 'No data available' )
                    continue
        # Message started, read the rest after the header
        else:
            try:
                readlen = size - 8
                msg = conn.recv(readlen)
                pos = size
                q.put( (msg_type, buf + msg) )
            except socket.error, e:
                err = e.args[0]
                if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
                    syslog.syslog('No data available')
                    continue
        if pos == size:
            pos = 0
            size = 0
            buf = None


def _send(q, stdin):
    counter = 0
    message = None
    
    while True:
        try:
            line = stdin.readline().strip()
            # syslog.syslog("GOT %s" % line)
            # When the parent dies we are seeing continual newlines, so we only access so many before stopping
            if line == "":
                counter += 1
                if counter > 100:
                    break
                continue
            counter = 0
            q.put(line)
        except KeyboardInterrupt:
            pass
        except IOError:
            # most likely a signal during readline
            pass


def _process_sim(sim_queue, peer):
    while True:
        msg_type, buf = sim_queue.get()
        # syslog.syslog("%s" %(str(peer)))
        peer.process_message(msg_type, buf)   
        # if cmds:
        #     for cmd in cmds:
        #         stdout.write(cmd + '\n')
        #         stdout.flush()  

def _process_exa(exabgp_queue, peer):
    while True:
        line = exabgp_queue.get()
        # syslog.syslog("%s" %(str(peer)))
        if line != "done" and line != "error":
            # syslog.syslog(line)
            peer.process_bgp_message(line)

''' main '''

if __name__ == '__main__':

    address = ('172.20.254.254', 6000)
    rname = sys.argv[1]
    mutex = Lock()
    try:
        pass
        # exabgp_queue = Queue.Queue()
        # sim_queue = Queue.Queue()
        # sender = Thread(target=_send, args=(exabgp_queue, sys.stdin))
        # sender.start()
        # conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # # s.setblocking(0)
        # conn.connect(address)
        # peer = BGPPeer(rname, conn, sys.stdout, mutex)
        # receiver = Thread(target=_recv, args=(conn, sim_queue))
        # process_exa = Thread(target =_process_exa, args=(exabgp_queue, peer))
        # process_sim = Thread(target =_process_sim, args=(sim_queue, peer))
        # process_exa.daemon = True
        # process_sim.daemon = True
        # receiver.start()
        # process_exa.start()
        # process_sim.start()
        # sender.join()
        # # receiver.join()
        # conn.close()
    except OSError as msg:
        print msg

