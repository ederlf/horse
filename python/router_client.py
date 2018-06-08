#!/usr/bin/env python
import os
import sys
from threading import Thread
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
    
def _recv(conn, q, peer):
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
                # cmds = peer.process_message(msg_type, buf + msg)
                # if cmds:
                #     for cmd in cmds:
                #         stdout.write(cmd + '\n')
                #         stdout.flush()  
                        # time.sleep(1)
            except socket.error, e:
                err = e.args[0]
                if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
                    syslog.syslog('No data available')
                    continue
        if pos == size:
            readlen = 0
            pos = 0
            size = 0
            buf = None


def _send(q, peer, stdin):
    counter = 0
    message = None
    
    while True:
        try:
            line = stdin.readline().strip()
            # syslog.syslog(line)
            # When the parent dies we are seeing continual newlines, so we only access so many before stopping
            if line == "":
                counter += 1
                if counter > 100:
                    break
                continue
            counter = 0
            # Parse message, and if it's the correct type, store in the database
            # import syslog
            # syslog.syslog("GOT %s" % line)
            q.put(line)
            # message = peer.process_bgp_message(line)

        except KeyboardInterrupt:
            pass
        except IOError:
            # most likely a signal during readline
            pass


def _process_sim(sim_queue, peer, stdout):
    while True:
        msg_type, buf = sim_queue.get()
        # cmds = peer.process_message(msg_type, buf)   
        # if cmds:
        #     for cmd in cmds:
        #         stdout.write(cmd + '\n')
        #         stdout.flush()  

def _process_exa(exabgp_queue, peer):
    while True:
        line = exabgp_queue.get()
        syslog.syslog("WILL PROCESS %s" % line)
        peer.process_bgp_message(line)

''' main '''
if __name__ == '__main__':

    address = ('172.20.254.254', 6000)
    try:
        exabgp_queue = Queue.Queue()
        sim_queue = Queue.Queue()
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # s.setblocking(0)
        s.connect(address)
        peer = BGPPeer(conn=s)
        
        sender = Thread(target=_send, args=(exabgp_queue, peer, sys.stdin))
        process_exa = Thread(target =_process_exa, args=(exabgp_queue, peer))
        
        receiver = Thread(target=_recv, args=(s, sim_queue, peer))
        process_sim = Thread(target =_process_sim, args=(sim_queue, peer, sys.stdout))
        
        process_exa.daemon = True
        process_sim.daemon = True
        sender.start()
        # receiver.start()
        process_exa.start()
        process_sim.start()
        sender.join()
        s.close()
    except OSError as msg:
        print msg

