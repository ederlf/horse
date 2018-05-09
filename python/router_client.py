#!/usr/bin/env python
import json
import os
import datetime
from sys import stdin, stdout
from threading import Thread
import syslog
import socket
import struct
import errno

def message_parser(line):
    # Parse JSON string  to dictionary
    temp_message = json.loads(line)
    # line =  "{ \"exabgp\": \"3.4.8\", \"time\": 1525168159, \"host\" : \"horse\", \"pid\" : \"8571\", \"ppid\" : \"1\", \"counter\": 1, \"type\": \"state\", \"neighbor\": { \"ip\": \"10.0.0.1\", \"address\": { \"local\": \"10.0.0.2\", \"peer\": \"10.0.0.1\"}, \"asn\": { \"local\": \"2\", \"peer\": \"1\"}, \"state\": \"connected\"} }"
    # parsed = json.loads(line)


    # test = {"time": 10000, "state" : "up"}
    # state = test['state']
    # syslog.syslog(test)
    # Convert Unix timestamp to python datetime
    # syslog.syslog(line)
    if temp_message['type'] == 'state':
        message = {
            'origin': temp_message['neighbor']['address']['local'],
            'type': 'state',
            'peer': temp_message['neighbor']['ip'],
            'state': temp_message['neighbor']['state'],
        }
        return message

    if temp_message['type'] == 'keepalive':
        message = {
            'type': 'keepalive',
            'time': timestamp,
            'peer': temp_message['neighbor']['ip'],
        }

        return message

    # If message is a different type, ignore
    return None

def process_message(msg, stdout):
    msg_type, size = struct.unpack("!BH", msg[:3])
    print "Received"
    print msg_type, size

def _recv(conn, stdout):
    readlen = 0
    pos = 0
    size = 0
    buf = None
    syslog.syslog("I am starting")
    while True:
        # It just started
        syslog.syslog( "I am running" )
        print pos
        if pos == 0:
            try:
                syslog.syslog ("Will try the reader yo")
                readlen = 8
                msg = conn.recv(readlen)
                pos += 8
                syslog.syslog(str(len(msg)))
                msg_type, size = struct.unpack("!BH",msg[:3])
                syslog.syslog( str(msg_type) )
                syslog.syslog(str(size))
                buf = msg
                syslog.syslog("Got the header yo") 
            except socket.error, e:
                err = e.args[0]
                if err == errno.EAGAIN or err == errno.EWOULDBLOCK:
                    syslog.syslog( 'No data available' )
                    continue
            # syslog.syslog(msg_type)
            # syslog.syslog(size)
        # Message started to be received
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


def _send(conn, stdin):
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
            message = message_parser(line)
            if message:
                # conn.send(message)
                # m = json.dumps(message)
                conn.send(str(message))
                #parsed = json.dumps(message)
                # syslog.syslog(parsed)

        except KeyboardInterrupt:
            pass
        except IOError:
            # most likely a signal during readline
            pass

''' main '''
if __name__ == '__main__':

    # address = ('127.0.0.1', 6000)
    address = ('172.20.254.254', 6000)
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(address)
        # s.setblocking(0)
        sender = Thread(target=_send, args=(s, stdin))
        receiver = Thread(target=_recv, args=(s, stdout))
        sender.start()
        receiver.start()
        sender.join()
        receiver.join()
        s.close()
    except OSError as msg:
        print msg
