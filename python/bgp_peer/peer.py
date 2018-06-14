#!/usr/bin/env python
#  Author:
#  Muhammad Shahbaz (muhammad.shahbaz@gatech.edu)
#  Rudiger Birkner (Networked Systems Group ETH Zurich)
#  Arpit Gupta (Princeton)
#  Eder Leao Fernandes (Queen Mary University of London)


import time
import os
import sys
import json
import socket
import struct
import syslog
np = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if np not in sys.path:
    sys.path.append(np)

from decision_process import best_path_selection, lowest_router_id
from rib import adj_rib_in, rib, RibTuple
import msg as rmsg

TIMING = True

class BGPPeer(object):

    def __init__(self, rname, conn, stdout, mutex):
        self.rname = rname
        self.rib = {"input": adj_rib_in(),
                    "local": rib(),
                    "output": rib()}
        self.neighbors_up = {} # neighbor ip is the key/ ASN is the value
        self.conn = conn # Speak to the simulator
        self.stdout = stdout
        self.mutex = mutex
        with file("/tmp/conf-bgp.%s" % rname) as f:
            self.conf = json.load(f)
            self.router_id = self.conf['router_id']
            self.asn = int(self.conf['asn'])

    def __str__(self):
        peer = "router-id:%s, asn:%s" % (self.router_id, self.asn)
        return peer

    def update(self,route):
        updates = []
        key = {}

        origin = None
        as_path = None
        med = None
        atomic_aggregate = None
        community = None

        route_list = []
        # Extract out neighbor information in the given BGP update
        # { "exabgp": "4.0.1", "time": 1528450259.79, "host" : "horse", 
        # "pid" : 9879, "ppid" : 1, "counter": 3, "type": "update",
        # "neighbor": 
        # { "address": 
        # { "local": "10.0.0.1", "peer": "10.0.0.2" }, 
        # "asn": { "local": 1, "peer": 2 } , 
        # "direction": "receive", 
        # "message": 
        # { "update": 
        # { "attribute": 
        # { "origin": "igp", "as-path": [ 2 ], "confederation-path": [] }, 
        # "announce": 
        # { "ipv4 unicast": 
        # { "10.0.0.2": [ { "nlri": "130.0.0.0/16" } ] } } } } } }

        # if ('state' in route['neighbor'] and route['neighbor']['state']=='down'):
        #     #TODO WHY NOT COMPLETELY DELETE LOCAL?
        #     routes = self.rib['input'].get_all()

        #     for route_item in routes:
        #         self.rib['local'].delete(prefix=route_item.prefix)

        #     self.rib["input"].delete_all()

        if ('update' in route['neighbor']['message']):
            neighbor = route["neighbor"]["address"]["peer"]
            asn = int(route["neighbor"]["asn"]["peer"])
            if ('attribute' in route['neighbor']['message']['update']):
                attribute = route['neighbor']['message']['update']['attribute']

                origin = attribute['origin'] if 'origin' in attribute else ''

                as_path = attribute['as-path'] if 'as-path' in attribute else []
                # Loop detected
                if not self.conf['allowas_in'] and self.asn in as_path:
                    return []

                med = attribute['med'] if 'med' in attribute else ''

                community = attribute['community'] if 'community' in attribute else ''
                communities = ''
                for c in community:
                    communities += ':'.join(map(str,c)) + " "

                atomic_aggregate = attribute['atomic-aggregate'] if 'atomic-aggregate' in attribute else ''

            if ('announce' in route['neighbor']['message']['update']):
                announce = route['neighbor']['message']['update']['announce']
                if ('ipv4 unicast' in announce):
                    for next_hop in announce['ipv4 unicast'].keys():
                        for prefix in announce['ipv4 unicast'][next_hop]:
                            attributes = RibTuple(prefix['nlri'], neighbor, asn, next_hop, origin, as_path,
                                         communities, med, atomic_aggregate)
                            self.add_route("input", attributes)
                            announce_route = self.get_route_with_neighbor("input", prefix['nlri'], neighbor)
                            # if announce_route is None:
                            #     self.logger.debug('-------------- announce_route is None')
                            #     self.rib['input'].dump(logger)
                            #     self.logger.debug('--------------')
                            #     self.logger.debug(str(prefix)+' '+str(neighbor))
                            #     assert(announce_route is not None)
                            if announce_route:
                                route_list.append({'announce': announce_route})

            elif ('withdraw' in route['neighbor']['message']['update']):
                withdraw = route['neighbor']['message']['update']['withdraw']
                if ('ipv4 unicast' in withdraw):
                    for prefix in withdraw['ipv4 unicast'].keys():
                        deleted_route = self.get_route_with_neighbor("input", prefix, neighbor)
                        if deleted_route != None:
                            self.delete_route_with_neighbor("input", prefix, neighbor)
                            route_list.append({'withdraw': deleted_route})

        # print route_list
        return route_list


    def decision_process_local(self, update):
        'Update the local rib with new best path'
        if ('announce' in update):

            # TODO: Make sure this logic is sane.
            '''Goal here is to get all the routes in participant's input
            rib for this prefix. '''

            # NOTES:
            # Currently the logic is that we push the new update in input rib and then
            # make the best path decision. This is very efficient. This is how it should be
            # done:
            # (1) For announcement: We need to compare between the entry for that
            # prefix in the local rib and new announcement. There is not need to scan
            # the entire input rib. The compare the new path with output rib and make
            # deicision whether to announce a new path or not.
            # (2) For withdraw: Check if withdraw withdrawn route is same as
            # best path in local, if yes, then delete it and run the best path
            # selection over entire input rib, else just ignore this withdraw
            # message.
            announce_route = update['announce']
            #self.logger.debug("decision process local:: "+str(announce_route))
            prefix = announce_route.prefix
            # self.logger.debug(" Peer Object for: "+str(self.id)+" --- processing update for prefix: "+str(prefix))
            current_best_routes = self.get_route('local', prefix)
            tie = False
            new_best_routes = None
            if current_best_routes != None:
                # This is what should be feed to the decision process
                current_best_num = len(current_best_routes)
                current_best_route = current_best_routes[0] 
                routes = [announce_route, current_best_route]
                new_best_routes, tie = best_path_selection(routes)
            else:
                # This is the first time for this prefix
                new_best_route = announce_route
                self.update_route('local', new_best_route)
                return [new_best_route]

            # There was a previous route, but no tie until MED.
            if not tie: 
                new_best_route = new_best_routes.pop()
                if not bgp_routes_are_equal(new_best_route, current_best_route):
                    self.update_route('local', new_best_route)
                    # Check
                    updated_best_path = self.get_route('local', prefix)
                    return [updated_best_path]
            # If max_paths is configured and there is a tie, install multiple paths
            else:
                # Decide which route should be taken
                max_paths = self.conf["max_paths"]
                new_best_route = lowest_router_id(new_best_routes)
                # Announced is preferred
                if not bgp_routes_are_equal(new_best_route, current_best_route):
                    if not max_paths:
                        self.update_route('local', new_best_route)    
                    elif current_best_num + 1 <= max_paths:

                        self.update_route('local', new_best_route, new = False,
                                          best = True)
                    else:
                        # Add the best and drop exceeding old route 
                        self.update_route('local', new_best_route, new = False,
                                          best = True)
                        self.drop_route('local', prefix)

                    return [new_best_route]
                # Best route did not change. Insert old in the back if max_paths enabled.
                elif current_best_num + 1 <= max_paths:
                    # Append the route
                    self.update_route('local', announce_route, new = False, best = False)
                    return [announce_route]
                else:
                    return []
                # # Do not send if ases are different from best route
                # if not self.conf["relax"] and current_best_route.asn != announce_route.asn:    
                #     return None
                # Old route should be already in the FIB, just return the announced
                
        elif('withdraw' in update):
            deleted_route = update['withdraw']
            prefix = deleted_route.prefix
            # self.logger.debug(" Peer Object for: "+str(self.id)+" ---processing withdraw for prefix: "+str(prefix))
            if deleted_route is not None:
                # delete route if being used
                current_best_route = self.get_route('local',prefix)
                if current_best_route:
                    # Check if current_best_route and deleted_route are advertised by the same guy
                    if deleted_route['neighbor'] == current_best_route['neighbor']:
                        # TODO: Make sure this logic is sane.
                        '''Goal here is to get all the routes in participant's input
                        rib for this prefix. '''
                        self.delete_route('local',prefix)
                        routes = self.get_routes('input',prefix)
                        if routes:
                            #self.logger.debug('decision_process_local: withdraw: best_route: '+str(type(best_route))+' '+str(best_route))
                            best_route = best_path_selection(routes)
                            self.update_route('local', best_route)
                            return best_route

        return None

    def bgp_update_peers(self, updates):
        # TODO: Verify if the new logic makes sense
        announcements = []
        for update in updates:
            if 'announce' in update:
                prefix = update['announce'].prefix
            else:
                prefix = update['withdraw'].prefix
            prev_routes = self.get_route("output", prefix)
            best_routes = self.get_route("local", prefix)
            if best_routes:
                best_route = best_routes[0]
                if prev_routes:
                    prev_route = prev_routes[0]
                else:
                    prev_route = None
                if ('announce' in update):
                    # Check if best path has changed for this prefix
                    if not bgp_routes_are_equal(best_route, prev_route):
                        # store announcement in output rib
                        self.update_route("output", best_route)
                        if best_route:
                            # announce the route to each router of the participant
                            # neighbors = neighbors_up - 
                            for neighbor in self.neighbors_up:
                                neighbor_sender = update['announce'].neighbor
                                if neighbor != neighbor_sender:
                                    # Get the interface the neighbor is connected
                                    next_hop = self.neighbors_up[neighbor][1]

                                    self.announce_route(neighbor, prefix,
                                                    next_hop, 
                                                    [self.asn] + best_route.as_path)
                        else:
                            continue

                elif ('withdraw' in update):
                    # A new announcement is only needed if the best path has changed
                    if best_route:
                        "There is a best path available for this prefix"
                        if not bgp_routes_are_equal(best_route, prev_route):
                            "There is a new best path available now"
                            # store announcement in output rib
                            self.update_route("output", best_route)
                            for neighbor in neighbors_up:
                                if neighbor != update["neighbor"]["ip"]:
                                    self.announce_route(neighbor, prefix,
                                                    best_route.next_hop, 
                                                    [self.asn] + best_route.as_path)
            # else:
            #     "Currently there is no best route to this prefix"
            #     if prev_route:
            #         # Clear this entry from the output rib
            #         if prefix in prefix_2_VNH:
            #             self.delete_route("output", prefix)
            #             for port in self.ports:
            #                 # TODO: Create a sender queue and import the announce_route function
            #                 announcements.append(withdraw_route(port["IP"],
            #                     prefix,
            #                     prefix_2_VNH[prefix]))

        # return announcements


    def process_bgp_route(self, route):
        "Process each incoming BGP advertisement"
        tstart = time.time()

        reply = ''
        # Map to update for each prefix in the route advertisement.  
        updates = self.update(route)
        #self.logger.debug("process_bgp_route:: "+str(updates))
        # TODO: This step should be parallelized
        # TODO: The decision process for these prefixes is going to be same, we
        # should think about getting rid of such redundant computations.
        best_routes = []
        for update in updates:
            best_route = self.decision_process_local(update)
            if best_route:
                best_routes += best_route
        if len(best_routes):
            # syslog.syslog("%s SENDING BEST ROUTES=============" % self.rname)
            msg = rmsg.BGPFIBMsg(local_id = rmsg.ip2int(self.router_id),
                                   routes = best_routes)
            # syslog.syslog("%s" % str(best_routes))
            self.conn.send(msg.pack())
            # with file("/tmp/rib%s" % self.rname, "a") as f:
            #     for route in best_routes:
            #         f.write("%s %s %s\n" % (route.prefix, route.next_hop, str(route.as_path)))
            #     f.write("\n") 

        if TIMING:
            elapsed = time.time() - tstart
            tstart = time.time()
        # Propagate
        announcements = self.bgp_update_peers(updates)

        if TIMING:
            elapsed = time.time() - tstart
            # print elapsed
            tstart = time.time()

        return announcements

    def process_bgp_message(self, line):
        temp_message = json.loads(line)
        # # Convert Unix timestamp to python datetime
        bgp_msg_type = temp_message['type']

        if bgp_msg_type == 'state':
            state = temp_message['neighbor']['state']
            s = 0
            
            if state == "down":
                s = rmsg.BGPStateMsg.BGP_STATE_DOWN
            elif state == "connected":
                s = rmsg.BGPStateMsg.BGP_STATE_CONNECTED
            else:
                s = rmsg.BGPStateMsg.BGP_STATE_UP
            
            peer = temp_message['neighbor']['address']["peer"]
            peer_asn = temp_message['neighbor']['asn']['peer']

            msg = rmsg.BGPStateMsg(local_id = rmsg.ip2int(self.router_id),
                                   peer_id = rmsg.ip2int(peer),
                                   state = s)
            if state == 'up':
                local_ip =  temp_message['neighbor']['address']['local']
                self.neighbors_up[peer] = (peer_asn, local_ip)
            elif state == 'down':
                self.neighbors_up.pop(peer, 'None')
            self.conn.send(msg.pack())

        elif bgp_msg_type == 'update':
            msg = rmsg.BGPActivity(local_id = rmsg.ip2int(self.router_id))
            self.conn.send(msg.pack())
            self.process_bgp_route(temp_message)
        # No reply to be sent

    def process_message(self, msg_type, data):
        if msg_type == rmsg.MsgType.BGP_ANNOUNCE:
            msg = rmsg.BGPAnnounce(msg=data)
            prefixes = self.conf["prefixes"]
            neighbor = rmsg.int2ip(msg.peer_id)
            routes = self.rib['output'].get_all_routes()
            # Announce routes that should be propagated, but peer was not connected.
            for prefix in routes:
                route = routes[prefix][0]
                next_hop = self.neighbors_up[neighbor][1]
                if neighbor != route.neighbor:
                    pass
                    self.announce_route(neighbor, prefix, next_hop ,
                                    [self.asn] + route.as_path)
            for prefix in prefixes:
                as_path = [self.asn]
                # Just a guarantee, because the session may flap
                if self.neighbors_up[neighbor]:
                    next_hop = self.neighbors_up[neighbor][1]                
                self.announce_route(neighbor, prefix, next_hop, as_path)
            
                # announcements.append(a)
            # return announcements
        # return None

    def announce_route(self, neighbor, prefix, next_hop, as_path):
        msg = "neighbor " + neighbor + " announce route " + prefix + " next-hop " + str(next_hop)
        msg += " as-path [  " + ' '.join(str(ap) for ap in as_path) + "  ]"
        self.mutex.acquire()
        try:
            self.stdout.write(msg + '\n')
            self.stdout.flush()
            # time.sleep(0.0001)
        finally:
            self.mutex.release()

    def add_route(self, rib_name, attributes):
        self.rib[rib_name].update(attributes)

    # Only for local and adj-rib out
    def get_route(self, rib_name, prefix):
        assert(isinstance(self.rib[rib_name], rib))
        return self.rib[rib_name].get(prefix=prefix)

    # Should always be used with adj-rib in type
    def get_route_with_neighbor(self, rib_name, prefix, neighbor):
        assert(isinstance(self.rib[rib_name], adj_rib_in))
        return self.rib[rib_name].get(prefix, neighbor)

    # Only used for rib-in
    def get_routes(self, rib_name,prefix):
        assert(isinstance(self.rib[rib_name], adj_rib_in))
        return self.rib[rib_name].get_all(prefix)

    # Only for rib
    def delete_route(self, rib_name,prefix):
        assert(isinstance(self.rib[rib_name], rib))
        self.rib[rib_name].delete(prefix=prefix)

    def drop_route(self, rib_name, prefix):
        ssert(isinstance(self.rib[rib_name], rib))
        self.rib[rib_name].drop(prefix=prefix)

    #Only for rib in
    def delete_route_with_neighbor(self, rib_name, prefix, neighbor):
        assert(isinstance(self.rib[rib_name], adj_rib_in))
        self.rib[rib_name].delete(prefix=prefix, neighbor=neighbor)

    def filter_route(self, rib_name, item,value):
        return self.rib[rib_name].get_all(**{item:value})

    def update_route(self, rib_name, attributes, new =True, best = True):
        self.rib[rib_name].update(attributes, new, best)


def get_prefixes_from_announcements(route):
    prefixes = []
    if ('update' in route['neighbor']):
        if ('announce' in route['neighbor']['update']):
            announce = route['neighbor']['update']['announce']
            if ('ipv4 unicast' in announce):
                for next_hop in announce['ipv4 unicast'].keys():
                    for prefix in announce['ipv4 unicast'][next_hop].keys():
                        print announce['ipv4 unicast']
                        next_hop = announce['ipv4 unicast'][next_hop][prefix]
                        prefixes.append(next_hop)

        elif ('withdraw' in route['neighbor']['update']):
            withdraw = route['neighbor']['update']['withdraw']
            if ('ipv4 unicast' in withdraw):
                for prefix in withdraw['ipv4 unicast'].keys():
                    prefixes.append(prefix)
    return prefixes

def bgp_routes_are_equal(route1, route2):
    if route1 is None:
        return False
    if route2 is None:
        return False
    if (route1.next_hop != route2.next_hop):
        return False
    if (route1.as_path != route2.as_path):
        return False
    return True





def withdraw_route(neighbor, prefix, next_hop):

    msg = "neighbor " + neighbor + " withdraw route " + prefix + " next-hop " + str(next_hop)

    return msg


''' main '''
if __name__ == '__main__':
    import time
    from guppy import hpy 
    h = hpy()
    mypeer = BGPPeer()
    # route =  RibTuple('10.0.0.1', '172.0.0.2','172.0.0.2', 'igp', '100, 200, 300', '0', 0,'false')
    route = '''{ "exabgp": "3.4.8", "time": 1526892856, "host" : "horse", "pid" : "21445", "ppid" : "1", "counter": 6, "type": "update", "neighbor": { "ip": "10.0.0.2", "address": { "local": "10.0.0.1", "peer": "10.0.0.2"}, "asn": { "local": "1", "peer": "2"}, "message": { "update": { "attribute": { "origin": "igp", "as-path": [ 2 ], "med": 100, "confederation-path": [] }, "announce": { "ipv4 unicast": { "10.0.0.2": { "100.20.0.0/24": {  }, "10.10.0.0/16": { } } } } } }} }'''
    

    mypeer.process_bgp_route(json.loads(route))
    # mypeer.process_bgp_route(json.loads(route2))
    # mypeer.process_bgp_route(json.loads(route3))
    # print mypeer.rib['input'].table
    print mypeer.rib['local'].table
    print mypeer.rib['output'].table
    print h.heap()
    time.sleep(5)
    # print mypeer.filter_route('input', 'as_path', '300')
