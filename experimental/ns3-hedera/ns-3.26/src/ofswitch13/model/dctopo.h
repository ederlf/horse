#ifndef FATTREETOPO
#define FATTREETOPO 1

#include <string>
#include <inttypes.h>
#include <vector>
#include <unordered_map>


namespace ns3 {

const uint32_t PORT_BASE = 1;
const std::string LAYER_CORE = "0";
const std::string LAYER_AGG = "1";
const std::string LAYER_EDGE = "2";
const std::string LAYER_HOST = "3";

class NodeId {
public:
    uint64_t dpid;

    NodeId(uint64_t dpid);
    ~NodeId(){};

    std::string IPStr(void);
};

class FatTreeNodeID: public NodeId
{
public:
    uint32_t pod;
    uint32_t sw;
    uint32_t host;
    std::string name;

    FatTreeNodeID(uint8_t pod = 0, uint8_t sw = 0, uint8_t host = 0, 
                  uint64_t dpid = 0, std::string name = "None");
    ~FatTreeNodeID(){};

    std::string NameStr(void);
    std::string MACStr(void);
    std::string IPStr(void);
};


class StructuredNodeSpec {
public:
        uint32_t up_total;
        uint32_t down_total; 
        uint32_t up_speed;
        uint32_t down_speed;
        std::string type_str;
        // @param up_total number of up links
        // @param down_total number of down links
        // @param up_speed speed in Gbps of up links
        // @param down_speed speed in Gbps of down links
        // @param type_str string; model of switch or server
        
        StructuredNodeSpec(uint32_t up_total, uint32_t down_total, 
                           uint32_t up_speed, uint32_t down_speed, std::string type_str = "None");
        ~StructuredNodeSpec(){};
};

class StructuredEdgeSpec{
public:
    uint32_t speed;
    StructuredEdgeSpec(uint32_t speed);
    ~StructuredEdgeSpec(){};
};

typedef std::unordered_map<std::string, std::string> InfoDict;
typedef std::pair<std::pair<std::string, uint32_t>, std::pair<std::string, uint32_t>> Link;

class StructuredTopo {
private:
    
    bool is_layer(std::string node, std::string layer) {
        return this->layer(node).compare(layer) == 0;
    }

public:

    uint32_t k;
    uint32_t speed;

    std::vector<StructuredNodeSpec> node_specs;
    std::vector<StructuredEdgeSpec> edge_specs;
    std::unordered_map<std::string, InfoDict> node_info;
    std::vector<std::string> nodes;
    std::unordered_map<std::string, std::vector<std::string>> links;
    /* To ease creation on ns3 later */
    std::vector<Link> bidirectional_links;

    StructuredTopo(std::vector<StructuredNodeSpec> node_specs, 
                   std::vector<StructuredEdgeSpec> edge_specs);
    ~StructuredTopo(){};
    
    std::unordered_map<std::string, std::string > def_nopts(std::string layer);
    InfoDict nodeInfo(std::string name);
    std::string layer(std::string name);
    bool isPortUp(uint32_t port); 
    std::vector<std::string> layer_nodes(std::string layer);
    std::vector<std::string> up_nodes(std::string name);
    std::vector<std::string> down_nodes(std::string name);
    std::vector<std::pair<std::string, std::string>> up_edges(std::string name);
    std::vector<std::pair<std::string, std::string>> down_edges(std::string name);

    void addNode(std::string name, InfoDict info);
    void addLink(std::string src, uint32_t src_port, std::string dst, uint32_t dst_port);

    void printLinks();
};

class FatTreeTopo: public StructuredTopo 
{
public:

    uint32_t numPods;
    uint32_t aggPerPod;

    static FatTreeNodeID id_gen(std::string name) {
        return FatTreeNodeID(0, 0, 0, 0, name);
    }

    static FatTreeNodeID id_gen(uint32_t pod, uint32_t sw, uint32_t host) {
        return FatTreeNodeID(pod, sw, host, 0, "None");
    }

    FatTreeTopo(uint32_t k, uint32_t speed, std::vector<StructuredNodeSpec> node_specs, std::vector<StructuredEdgeSpec> edge_specs);
    ~FatTreeTopo(){};

    std::unordered_map<std::string, std::string> def_nopts(std::string layer, std::string name);
    std::pair<uint32_t, uint32_t> port(std::string src, std::string dst);
    static FatTreeTopo build(uint32_t k, uint32_t speed);
};

}; // namespace ns3
#endif