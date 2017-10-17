#ifndef ROUTING_H
#define ROUTING_H 1

#include <vector>
#include <string>
#include <map>
#include "dctopo.h"

namespace ns3 {

typedef std::vector<std::string> Path;
typedef std::vector<Path> Paths;
typedef std::map<std::string, Paths > NodePaths;
typedef Path (*_path_choice)(Paths, std::string, std::string, uint8_t *pkt);

class Routing {
/* TBD */
protected:
    FatTreeTopo topo;    

public:
    Routing(FatTreeTopo topo);
    virtual ~Routing(){};
    virtual Path get_route(std::string src, std::string dst, uint8_t* data) = 0;
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
private:
    Paths _extend_reachable(int frontier_layer);

public:
    NodePaths src_paths;
    NodePaths dst_paths;
    uint32_t src_path_layer;
    uint32_t dst_path_layer;
    _path_choice path_choice;
    StructuredRouting(FatTreeTopo topo, _path_choice get_path);
    ~StructuredRouting(){};
    Path get_route(std::string src, std::string dst, uint8_t* data);
    Paths get_all_route(std::string src, std::string dst);
    void print_route(Path path);
};

class RandomStructuredRouting: public StructuredRouting {
    // '''Random Structured Routing.'''
public:
    RandomStructuredRouting(FatTreeTopo topo);
    static Path choose_random(Paths paths, std::string src, std::string dst, uint8_t *data);
};

class STStructuredRouting: public StructuredRouting 
{
public:
    STStructuredRouting(FatTreeTopo topo);
    static Path choose_random(Paths paths, std::string src, std::string dst, uint8_t *data);
};

class HashedStructuredRouting: public StructuredRouting 
{
public:
    HashedStructuredRouting(FatTreeTopo topo);
    static Path choose_random(Paths paths, std::string src, std::string dst, uint8_t *data);   
};

}; // namespace ns3
#endif