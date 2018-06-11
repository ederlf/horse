#!usr/bin/env python
#  Author:
# Eder Leao Fernandes

from collections import namedtuple

# have all the rib implementations return a consistent interface
# TODO: Add local pref
labels = ('prefix',  'neighbor', 'neighbor_as', 'next_hop', 'origin', 'as_path', 'communities', 'med', 'atomic_aggregate')
RibTuple = namedtuple('RibTuple', labels)

class rib(object):

    def __init__(self):
        self.table = {} # key is prefix, tuple is value 

    def __del__(self):
        #self.cluster.shutdown()
        pass

class rib(rib):
    def __init__(self):
        super(rib, self).__init__()

    def update(self, item, new = True, best = True):
        assert(isinstance(item, RibTuple))
        if new:
            self.table[item.prefix] = []
        if best:
            self.table[item.prefix].insert(0, item) 
        else:
            self.table[item.prefix].append(item)

    def drop(self, prefix):
        if prefix in self.table:
            if (len(self.table[prefix])):
                self.table[prefix].pop()

    def get(self, prefix):
        if prefix in self.table:
            return self.table[prefix]
        return None

    def get_all_routes(self):
        return self.table

    def get_prefixes(self):
        return sorted(self.table.keys())

    def delete(self, prefix):
        if prefix in self.table:
            self.table.pop(prefix, 'None')

class adj_rib_in(rib):
    def __init__(self):
        super(adj_rib_in, self).__init__()

    def update(self, item):
        assert(isinstance(item, RibTuple)) 
        if item.neighbor not in self.table:
            self.table[item.neighbor] = {}
        self.table[item.neighbor][item.prefix] = item
        
    def get(self, prefix, neighbor):
        if neighbor in self.table:
            if prefix in self.table[neighbor]:
                return self.table[neighbor][prefix]
        return None

    def get_all(self, prefix):
        routes = []
        for neighbor in self.table:
            if prefix in self.table[neighbor]:
                routes.append(self.table[neighbor][prefix])
        return routes

    def delete(self, prefix, neighbor):
        if neighbor in self.table:
            if prefix in self.table[neighbor]:
                self.table[neighbor].pop(prefix, 'None')
            

''' main '''
if __name__ == '__main__':
    #TODO Update test

    local_rib = rib()
    route =  RibTuple('10.0.0.1', '172.0.0.2', 100, '172.0.0.2', '10', 'igp', '100, 200, 300', '0', 0,'false')
    local_rib.update(route)
    print local_rib.get('10.0.0.1')
    local_rib.delete('10.0.0.1')
    print local_rib.get('10.0.0.1')


    adj_rib = adj_rib_in()
    adj_rib.update(route)
    print adj_rib.get_all('10.0.0.1')
    print adj_rib.get('10.0.0.1', '172.0.0.2')

    adj_rib.delete('10.0.0.1', '172.0.0.2')
