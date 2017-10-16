#ifndef ROUTING_H
#define ROUTING_H 1

#include <vector>
#include <string>
#include "dctopo.h"

namespace ns3 {

class Routing {
/* TBD */
protected:
    FatTreeTopo topo;    

public:
    Routing();
    virtual ~Routing();
    virtual <vector> get_route(string src, string dst, void* data) = 0;
};


/* '''Route flow through a StructuredTopo and return one path.

    Optionally accepts a function to choose among the set of valid paths.  For
    example, this could be based on a random choice, hash value, or
    always-leftmost path (yielding spanning-tree routing).

    Completely stupid!  Think of it as a topology-aware Dijstra's, that either
    extends the frontier until paths are found, or quits when it has looked for
    path all the way up to the core.  It simply enumerates all valid paths and
    chooses one.  Alternately, think of it as a bidrectional DFS.

    This is in no way optimized, and may be the slowest routing engine you've
    ever seen.  Still, it works with both VL2 and FatTree topos, and should
    help to bootstrap hardware testing and policy choices.

    The main data structures are the path dicts, one each for the src and dst.
    Each path dict has node ids as its keys.  The values are lists of routes,
    where each route records the list of dpids to get from the starting point
    (src or dst) to the key.

    Invariant: the last element in each route must be equal to the key 
*/

class StructuredRouting: public Routing
{
    StructuredRouting(FatTreeTopo topo, )
};

}; // namespace ns3
#endif