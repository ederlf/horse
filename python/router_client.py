#!/usr/bin/env python
import os
import sys
from threading import Thread
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
    
def _recv(conn, stdout, peer):
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
                # syslog.syslog(  )
                # syslog.syslog(str(size))
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
                cmds = peer.process_message(msg_type, buf + msg)
                if cmds:
                    for cmd in cmds:
                        stdout.write(cmd + '\n')
                        stdout.flush()  
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


def _send(conn, stdin, peer):
    counter = 0
    message = None
    
    while True:
        try:
            line = stdin.readline().strip()
            # When the parent dies we are seeing continual newlines, so we only access so many before stopping
            if line == "":
                counter += 1
                if counter > 100:
                    break
                continue
            counter = 0
            # Parse message, and if it's the correct type, store in the database
            message = peer.process_bgp_message(line)

        except KeyboardInterrupt:
            pass
        except IOError:
            # most likely a signal during readline
            pass


''' main '''
if __name__ == '__main__':

    # address = ('127.0.0.1', 6000)
    address = ('172.20.254.254', 6000)
    # syslog.syslog("Creating peer")
    # peer = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # s.setblocking(0)
        s.connect(address)
        peer = BGPPeer(conn=s)
        sender = Thread(target=_send, args=(s, sys.stdin, peer))
        receiver = Thread(target=_recv, args=(s, sys.stdout, peer))
        sender.start()
        receiver.start()
        sender.join()
        s.close()
    except OSError as msg:
        print msg

