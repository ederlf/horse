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
np = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
if np not in sys.path:
    sys.path.append(np)

from decision_process import best_path_selection
from rib import adj_rib_in, rib, RibTuple
import msg as rmsg

TIMING = True

class BGPPeer(object):

    def __init__(self, router_id = None, asn = None):
        self.router_id = router_id
        self.asn = asn
        self.rib = {"input": adj_rib_in(),
                    "local": rib(),
                    "output": rib()}
        self.neighbors_up = {} # neighbor ip is the key/ ASN is the value
        self.conf = None

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
    
        neighbor = route["neighbor"]["ip"]

        if ('state' in route['neighbor'] and route['neighbor']['state']=='down'):
            #TODO WHY NOT COMPLETELY DELETE LOCAL?
            routes = self.rib['input'].get_all()

            for route_item in routes:
                self.rib['local'].delete(prefix=route_item.prefix)

            self.rib["input"].delete_all()

        if ('update' in route['neighbor']['message']):
            if ('attribute' in route['neighbor']['message']['update']):
                attribute = route['neighbor']['message']['update']['attribute']

                origin = attribute['origin'] if 'origin' in attribute else ''

                as_path = attribute['as-path'] if 'as-path' in attribute else []

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
                        for prefix in announce['ipv4 unicast'][next_hop].keys():
                            attributes = RibTuple(prefix, neighbor, next_hop, origin, as_path,
                                         communities, med, atomic_aggregate)
                            self.add_route("input", attributes)
                            announce_route = self.get_route_with_neighbor("input", prefix, neighbor)
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
            current_best_route = self.get_route('local', prefix)
            if current_best_route:
                # This is what should be feed to the decision process
                routes = [announce_route, current_best_route]
                new_best_route = best_path_selection(routes)
            else:
                # This is the first time for this prefix
                new_best_route = announce_route

            # self.logger.debug(" Peer Object for: "+str(self.id)+" ---Best Route after Selection: "+str(prefix)+' '+str(new_best_route))
            if not bgp_routes_are_equal(new_best_route, current_best_route):
            #     self.logger.debug(" Peer Object for: "+str(self.id)+" --- No change in Best Path...move on "+str(prefix))
            # else:
                #self.logger.debug('decision_process_local: announce: new_best_route: '+str(type(new_best_route))+' '+str(new_best_route))
                self.update_route('local', new_best_route)
                # Check
                updated_best_path = self.get_route('local', prefix)
                # self.logger.debug(" Peer Object for: "+str(self.id)+" Pushed: "+str(new_best_route)+" Observing: "+str(updated_best_path))
                # self.logger.debug(" Peer Object for: "+str(self.id)+" --- Best Path changed: "+str(prefix)+' '+str(new_best_route)+" Older best route: "+str(current_best_route))

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
                #         else:
                #             self.logger.debug(" Peer Object for: "+str(self.id)+" ---No best route available for prefix "+str(prefix)+" after receiving withdraw message.")
                #     else:
                #         self.logger.debug(" Peer Object for: "+str(self.id)+" ---BGP withdraw for prefix "+str(prefix)+" has no impact on best path")
                # else:
                #     self.logger.debug(" Peer Object for: "+str(self.id)+" --- This is weird. How can we not have any delete object in this function")


    def bgp_update_peers(self, updates):
        # TODO: Verify if the new logic makes sense
        announcements = []
        for update in updates:
            if 'announce' in update:
                prefix = update['announce'].prefix
            else:
                prefix = update['withdraw'].prefix

            prev_route = self.get_route("output", prefix)
            best_route = self.get_route("local", prefix)
    
            if best_route:
                if ('announce' in update):
                    # Check if best path has changed for this prefix
                    if not bgp_routes_are_equal(best_route, prev_route):
                        # store announcement in output rib
                        # self.logger.debug(str(best_route)+' '+str(prev_route))
                        self.update_route("output", best_route)
                        if best_route:
                            pass
                            # announce the route to each router of the participant
                            # neighbors = neighbors_up - 
                            for neighbor in self.neighbors_up:
                                if neighbor != update[neighbor]:
                                    announcements.append(neighbor, prefix,
                                                         prefix, best_route.as_path)
                        else:
                            # self.logger.debug("Race condition problem for prefix: "+str(prefix))
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
                                if neighbor != update[neighbor]:
                                    announcements.append(neighbor, prefix,
                                                         prefix, best_route.as_path)
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

        return announcements


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
        for update in updates:
            self.decision_process_local(update)

        import syslog
        syslog.syslog("Decided %s" % str(self.rib['local'].table))
        if TIMING:
            elapsed = time.time() - tstart
            print str(elapsed)
            # self.logger.debug("Time taken for decision process: "+str(elapsed))
            tstart = time.time()

        # Propagate
        announcements = self.bgp_update_peers(updates)
        # Tell Route Server that it needs to announce these routes
        for announcement in announcements:
            # TODO: Complete the logic for this function
            self.send_announcement(announcement)

        if TIMING:
            elapsed = time.time() - tstart
            print elapsed
            # self.logger.debug("Time taken to send garps/announcements: "+str(elapsed))
            tstart = time.time()

    def message_parser(self, line):
        temp_message = json.loads(line)
        # Convert Unix timestamp to python datetime
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
            # return message
            peer_id = struct.unpack("!L", socket.inet_aton(temp_message['neighbor']['ip']))[0] 
            router_id =  temp_message['neighbor']['address']['local']
            
            msg = rmsg.BGPStateMsg(local_id = rmsg.ip2int(router_id), peer_id = peer_id,
                                  state = s)
            if state == 'up':
                if self.router_id == None:
                    self.router_id = router_id
                    with file("/tmp/conf-%s" % router_id) as f:
                        self.conf = json.load(f)

                if self.asn == None:
                    self.asn = temp_message['neighbor']['asn']['local']

                self.neighbors_up[temp_message['neighbor']['ip']] = temp_message['neighbor']['asn']['peer']
            elif state == 'down' and self.router_id:
                self.neighbors_up.pop(temp_message['neighbor']['ip'], 'None')
            return msg.pack()

        elif bgp_msg_type == 'update':
            self.process_bgp_route(temp_message)
        # No reply to be sent
        return None

    def process_message(self, msg_type, data):
        if msg_type == rmsg.MsgType.BGP_ANNOUNCE:
            msg = rmsg.BGPAnnounce(msg=data)
            prefixes = self.conf["prefixes"]
            announcements = []
            for prefix in prefixes:
                as_path = [self.asn]
                neighbor = rmsg.int2ip(msg.peer_id)
                a = announce_route(neighbor, prefix, prefixes[prefix]['NH'], as_path)
                announcements.append(a)
            return announcements
        return None

    def send_announcement(self, announcement):
        "Send the announcements to XRS"
        announcement.as_path = [self.asn] + announcement.as_path
        print announcement
        # self.logger.debug("Sending announcements to XRS. "+str(type(announcement)))

        # self.xrs_client.send(json.dumps(announcement))


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

    #Only for rib in
    def delete_route_with_neighbor(self, rib_name, prefix, neighbor):
        assert(isinstance(self.rib[rib_name], adj_rib_in))
        self.rib[rib_name].delete(prefix=prefix, neighbor=neighbor)

    def filter_route(self, rib_name, item,value):
        return self.rib[rib_name].get_all(**{item:value})


    def update_route(self, rib_name, attributes):
        self.rib[rib_name].update(attributes)


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


def announce_route(neighbor, prefix, next_hop, as_path):

    msg = "neighbor " + neighbor + " announce route " + prefix + " next-hop " + str(next_hop)
    msg += " as-path [ ( " + ' '.join(str(ap) for ap in as_path) + " ) ]"

    return msg


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
