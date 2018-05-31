#!/usr/bin/env python
import json
import os
import datetime
import sys
from threading import Thread
import msg as rmsg
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

# 'neighbor 10.0.0.3 announce route 200.20.0.0/24 next-hop self',
    

announcements = [
    'neighbor 10.0.0.1 announce route 100.10.0.0/24 next-hop 10.0.0.2',
    'neighbor 10.0.0.1 announce route 100.20.0.0/24 next-hop 10.0.0.2',
]

def message_parser(line, peer):
    # Parse JSON string  to dictionary
    temp_message = json.loads(line)
    # syslog.syslog(line)
    # Convert Unix timestamp to python datetime
    if temp_message['type'] == 'state':
        state = temp_message['neighbor']['state']
        s = 0
        if state == "down":
            s = rmsg.BGPStateMsg.BGP_STATE_DOWN
        elif state == "connected":
            s = rmsg.BGPStateMsg.BGP_STATE_CONNECTED
        else:
            s = rmsg.BGPStateMsg.BGP_STATE_UP
        # return message
        peer_id = struct.unpack("!L", socket.inet_aton(temp_message['neighbor']['ip']))[0] 
        router_id =  temp_message['neighbor']['address']['local']
        
        msg = rmsg.BGPStateMsg(local_id = rmsg.ip2int(router_id), peer_id = peer_id,
                              state = s)
        
        if state == 'up':
            # syslog.syslog("SENDING STATE %s %s" % (state, peer.router_id == None))
            if peer.router_id == None:
                peer.router_id = router_id
                syslog.syslog(router_id)
                with file("/tmp/conf-%s" % router_id) as f:
                    peer.conf = json.load(f)
                    # syslog.syslog(str(peer.conf))

            if peer.asn == None:
                peer.asn = temp_message['neighbor']['asn']['local']

            peer.neighbors_up[temp_message['neighbor']['ip']] = temp_message['neighbor']['asn']['peer']
            # syslog.syslog(str(peer.neighbors_up))

            # if temp_message['neighbor']['address']['peer'] == "10.0.0.1":
            #     for announce in announcements:
            #         syslog.syslog("ANNOUNCE %s" % announce)
            #         sys.stdout.write(announce + '\n')
            #         sys.stdout.flush()
            #         time.sleep(1)
        elif state == 'down' and peer.router_id:
            peer.neighbors_up.pop(temp_message['neighbor']['ip'], 'None')
        return msg.pack()

    if temp_message['type'] == 'keepalive':
        message = {
            'type': 'keepalive',
            'time': timestamp,
            'peer': temp_message['neighbor']['ip'],
        }

        return None

    if temp_message['type'] == 'update':
        message = {
            'type': 'update',
            'origin': temp_message['neighbor']['address']['local'],
            'peer': temp_message['neighbor']['ip'],
        }
        return None

    # If message is a different type, ignore
    return None

def process_message(msg, stdout):
    msg_type, size = struct.unpack("!BH", msg[:3])
    print "Received"
    print msg_type, size

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
                # syslog.syslog(str(len(msg)))
                msg_type, size = struct.unpack("!BH",msg[:3])
                # syslog.syslog( str(msg_type) )
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
                process_message(buf + msg, stdout)
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
            # syslog.syslog(line)
            # When the parent dies we are seeing continual newlines, so we only access so many before stopping
            if line == "":
                counter += 1
                if counter > 100:
                    break
                continue
            counter = 0
            # Parse message, and if it's the correct type, store in the database
            message = message_parser(line, peer)
            if message:
                # conn.send(message)
                # m = json.dumps(message)
                conn.send(message)

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
    peer = BGPPeer()
    # peer = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # s.setblocking(0)
        s.connect(address)
        sender = Thread(target=_send, args=(s, sys.stdin, peer))
        receiver = Thread(target=_recv, args=(s, sys.stdout, peer))
        sender.start()
        receiver.start()
        #Iterate through messages
        sender.join()
        # receiver2.join()
        s.close()
    except OSError as msg:
        print msg

