#include "dctopo.h"
#include "util.h"
#include <utility> 

namespace ns3 {


NodeId::NodeId(uint64_t dpid)
{
    this->dpid = dpid;
}

// @return ip ip as string 
std::string 
NodeId::IPStr(void) 
{
    char str[] = "10.%i.%i.%i"; 
    uint8_t hi = (this->dpid && 0xff0000) >> 16;
    uint8_t mid = (this->dpid & 0xff00) >> 8;
    uint8_t lo = this->dpid & 0xff;
    return format(str, hi, mid, lo);
}

// ========== FatTreeNodeID ==================================================

FatTreeNodeID::FatTreeNodeID(uint8_t pod, uint8_t sw, 
                             uint8_t host, uint64_t dpid, std::string name) : NodeId(dpid)
{
    if (dpid) {
        this->pod = (dpid & 0xff0000) >> 16;
        this->sw = (dpid & 0xff00) >> 8;
        this->host = (dpid & 0xff);;
        this->dpid = dpid;
    }
    else if (name.compare("None")) {
        std::vector<std::string> parts;
        split(name, parts, '_');
        this->pod = std::stoul (parts[0]); 
        this->sw = std::stoul (parts[1]); 
        this->host = std::stoul (parts[2]);
        this->dpid = (this->pod << 16) + (this->sw << 8) + this->host;
    }
    else {
        this->pod = pod;
        this->sw = sw;
        this->host = host;
        this->dpid = (pod << 16) + (sw << 8) + host;
    }
}

std::string FatTreeNodeID::NameStr(void)
{
    char str[] = "%i_%i_%i"; 
    return format(str, this->pod, this->sw, this->host);

}

std::string FatTreeNodeID::MACStr(void)
{
    char str[] = "00:00:00:%02x:%02x:%02x";
    return  format(str, this->pod, this->sw, this->host);
}

std::string FatTreeNodeID::IPStr(void)
{
    char str[] = "10.%i.%i.%i";
    return format(str, this->pod, this->sw, this->host);
}

// ========== StructuredNodeSpec============================================


StructuredNodeSpec::StructuredNodeSpec(uint32_t up_total, 
                                       uint32_t down_total, uint32_t up_speed, uint32_t down_speed, std::string type_str)
{
    this->up_total = up_total;
    this->down_total = down_total;
    this->up_speed = up_speed;
    this->down_speed = down_speed;
    this->type_str = type_str;
}

StructuredEdgeSpec::StructuredEdgeSpec(uint32_t speed)
{
    this->speed = speed;
}

// =========== StructuredTopo =================

StructuredTopo::StructuredTopo(std::vector<StructuredNodeSpec> node_specs, 
                   std::vector<StructuredEdgeSpec> edge_specs)
{
    this->node_specs = node_specs;
    this->edge_specs = edge_specs;
}

std::unordered_map<std::string, std::string > 
StructuredTopo::def_nopts(std::string layer) {
        // '''Return default dict for a structured topo.

        // @param layer layer of node
        // @return d dict with layer key/val pair, plus anything else (later)
        std::unordered_map<std::string, std::string > d = {{"layer", layer}};
        return d;
}

InfoDict
StructuredTopo::nodeInfo(std::string name)
{
    return this->node_info[name];
}

std::string
StructuredTopo::layer(std::string name)
{
        InfoDict info;
        info = this->nodeInfo(name);
        return info["layer"];
}

bool
StructuredTopo::isPortUp(uint32_t port) 
{
        return port % 2 == PORT_BASE;
}

std::vector<std::string>
StructuredTopo::layer_nodes(std::string layer){
    // '''Return nodes at a provided layer.

    // @param layer layer
    // @return names list of names
    // '''
    std::vector<std::string> nodes;
    for (auto &node: this->nodes) {
        if (this->is_layer(node, layer)) {
            nodes.push_back(node);
        }    
    }
    return nodes;
}

std::vector<std::string>
StructuredTopo::up_nodes(std::string name) {
    // '''Return edges one layer higher (closer to core).

    // @param name name

    // @return names list of names
    // '''
    uint32_t up_layer = std::stoul(this->layer(name)) - 1;
    std::string layer = std::to_string(up_layer);
    std::vector<std::string> node_links = this->links[name];
    for (auto &node: node_links) {
        if (this->is_layer(node, layer)) {
            nodes.push_back(node);
        }    
    }
    return nodes;
}

std::vector<std::string>
StructuredTopo::down_nodes(std::string name) {
    // '''Return edges one layer higher (closer to core).

    // @param name name

    // @return names list of names
    // '''
    uint32_t up_layer = std::stoul(this->layer(name)) + 1;
    std::string layer = std::to_string(up_layer);
    std::vector<std::string> node_links = this->links[name];
    for (auto &node: node_links) {
        if (this->is_layer(node, layer)) {
            nodes.push_back(node);
        }    
    }
    return nodes;
}

std::vector<std::pair<std::string, std::string>>
StructuredTopo::up_edges(std::string name){
    // '''Return edges one layer higher (closer to core).

    // @param name name
    // @return up_edges list of name pairs
    // '''
    std::vector<std::pair<std::string, std::string>> edges; 
    std::vector<std::string> up_nodes = this->up_nodes(name);
    for (auto &node: up_nodes) {
        edges.push_back(std::make_pair(name, node));
    }  
    return edges;  
}

std::vector<std::pair<std::string, std::string>>
StructuredTopo::down_edges(std::string name){
    // '''Return edges one layer higher (closer to core).

    // @param name name
    // @return up_edges list of name pairs
    // '''
    std::vector<std::pair<std::string, std::string>> edges; 
    std::vector<std::string> down_nodes = this->down_nodes(name);
    for (auto &node: down_nodes) {
        edges.push_back(std::make_pair(name, node));
    }  
    return edges;  
}

void StructuredTopo::addNode(std::string name, InfoDict info) 
{
    this->nodes.push_back(name);
    this->node_info[name] = info; 
}

void StructuredTopo::addLink(std::string src, uint32_t src_port, std::string dst, uint32_t dst_port) 
{
    this->links[src].push_back(dst);
    this->links[dst].push_back(src);
    Link l = std::make_pair(std::make_pair(src, src_port), std::make_pair(dst, dst_port));
    this->bidirectional_links.push_back(l);
}

void StructuredTopo::printLinks(void)
{
    std::cout << this->bidirectional_links.size() << "\n";
    for(auto &l: this->bidirectional_links){
        std::pair<std::string, uint32_t> src, dst;
        src = l.first;
        dst = l.second;
        char text[] = "src:%s src_port:%u <-> dst:%s dst_port:%u";
        std::cout << format(text, src.first.c_str(), src.second, dst.first.c_str(), dst.second) << "\n";
    }
}

// ========================= FatTreeTopo ================================

FatTreeTopo::FatTreeTopo(uint32_t k, uint32_t speed, std::vector<StructuredNodeSpec> node_specs, std::vector<StructuredEdgeSpec> edge_specs): StructuredTopo(node_specs, edge_specs)
{
    this->k = k;
    this->speed = speed;
}

std::unordered_map<std::string, std::string > 
FatTreeTopo::def_nopts(std::string layer, std::string name) {
    // '''Return default dict for a FatTree topo.

    // @param layer layer of node
    // @param name name of node
    // @return d dict with layer key/val pair, plus anything else (later)
    // '''
    std::unordered_map<std::string, std::string > d =  {{"layer", layer}};
    if ( name.compare("None") ) {
        FatTreeNodeID id = id_gen(name);
        // # For hosts only, set the IP
        if (layer.compare(LAYER_HOST) == 0){
          d["ip"] = id.IPStr(); 
          d["mac"] = id.MACStr();
        }
        char dp_id[] = "%" PRIx64 "";  
        d["dpid"] = format(dp_id, id.dpid);
    }
    return d;
}

static FatTreeTopo build(uint32_t k, uint32_t speed)
{
    // @param k switch degree
    //     @param speed bandwidth in Gbps
        // '''
    StructuredNodeSpec core = StructuredNodeSpec(0, k, 0, 
                                                 speed, "core");
    StructuredNodeSpec agg = StructuredNodeSpec(k / 2, k / 2, speed, speed, "agg");
    StructuredNodeSpec edge = StructuredNodeSpec(k / 2, k / 2, speed, speed, "edge");
    StructuredNodeSpec host = StructuredNodeSpec(1, 0, speed, 0, "host");
    std::vector<StructuredNodeSpec> node_specs = {core, agg, edge, host};
    StructuredEdgeSpec edge_spec = StructuredEdgeSpec(speed);
    std::vector<StructuredEdgeSpec> edge_specs = {edge_spec, edge_spec, edge_spec};
    FatTreeTopo topo = FatTreeTopo(k, speed, node_specs, edge_specs);

    // self.k = k
    // self.id_gen = FatTreeTopo.FatTreeNodeID
    topo.numPods = k;
    topo.aggPerPod = k / 2;

    std::vector<int> pods(k);
    std::iota(std::begin(pods), std::end(pods), 0);

    std::vector<int> core_sws((k / 2 + 1) - 1);
    std::iota(std::begin(core_sws), std::end(core_sws), 1);

    std::vector<int> agg_sws(k/2);
    std::iota(std::begin(agg_sws), std::end(agg_sws), k/2);

    std::vector<int> edge_sws(k/2);
    std::iota(std::begin(edge_sws), std::end(edge_sws), 0);

    std::vector<int> hosts((k / 2 + 2) - 2);
    std::iota(std::begin(hosts), std::end(hosts), 2);

    for (auto &p: pods) {
        for (auto &e: edge_sws) {
            std::string edge_id = topo.id_gen(p, e, 1).NameStr();
            InfoDict edge_opts = topo.def_nopts(LAYER_EDGE, edge_id);
            topo.addNode(edge_id, edge_opts);

            for (auto &h: hosts) {
                std::string host_id = topo.id_gen(p, e, h).NameStr();
                InfoDict host_opts = topo.def_nopts(LAYER_HOST, host_id);
                topo.addNode(host_id, host_opts);
                std::pair<int, int> src_dst = topo.port(edge_id, host_id);
                topo.addLink(edge_id, src_dst.first, host_id, src_dst.second);
                // std::cout << src_dst.first << " " << src_dst.second << "\n";  
            }
            for (auto &a: agg_sws) {
                std::string agg_id = topo.id_gen(p, a, 1).NameStr();
                InfoDict agg_opts = topo.def_nopts(LAYER_AGG, agg_id);
                std::unordered_map<std::string, InfoDict>::const_iterator got = topo.node_info.find (agg_id);
                if ( got == topo.node_info.end() ) {
                    topo.addNode(agg_id, agg_opts);
                }
                std::pair<int, int> src_dst = topo.port(edge_id, agg_id);
                topo.addLink(edge_id, src_dst.first, agg_id, src_dst.second);
            }
        }
        for (auto &a: agg_sws) {
            std::string agg_id = topo.id_gen(p, a, 1).NameStr();
            uint32_t c_index = a - k / 2 + 1;
            for (auto &c: core_sws) {
                std::string core_id = topo.id_gen(k, c_index, c).NameStr();
                InfoDict core_opts = topo.def_nopts(LAYER_CORE, core_id);
                std::unordered_map<std::string, InfoDict>::const_iterator got = topo.node_info.find (core_id);
                if ( got == topo.node_info.end() ) {
                    topo.addNode(core_id, core_opts);
                }
                std::pair<int, int> src_dst = topo.port(core_id, agg_id);
                topo.addLink(core_id, src_dst.first, agg_id, src_dst.second);
            }
        }      
    }
    return topo;
}

std::pair<uint32_t, uint32_t>
FatTreeTopo::port(std::string src, std::string dst){
    // '''Get port number (optional)

    // Note that the topological significance of DPIDs in FatTreeTopo enables
    // this function to be implemented statelessly.

    // @param src source switch DPID
    // @param dst destination switch DPID
    // @return tuple (src_port, dst_port):
    //     src_port: port on source switch leading to the destination switch
    //     dst_port: port on destination switch leading to the source switch
    // '''
    
    uint32_t src_port, dst_port;

    std::string src_layer = this->layer(src);
    std::string dst_layer = this->layer(dst);

    FatTreeNodeID src_id = id_gen(src);
    FatTreeNodeID dst_id = id_gen(dst);

    if (src_layer.compare(LAYER_HOST) == 0 && 
        dst_layer.compare(LAYER_EDGE) == 0 ) {
        src_port = 0;
        dst_port = (src_id.host - 2) * 2 + 1;
    }
    else if (src_layer.compare(LAYER_EDGE) == 0 && 
        dst_layer.compare(LAYER_CORE) == 0) {
        src_port = (dst_id.sw - 2) * 2;
        dst_port = src_id.pod;
    }
    else if (src_layer.compare(LAYER_EDGE) == 0 && 
             dst_layer.compare(LAYER_AGG) == 0) {
        src_port = (dst_id.sw - this->k / 2) * 2;
        dst_port = src_id.sw * 2 + 1;
    }
    else if (src_layer.compare(LAYER_AGG) == 0 && 
             dst_layer.compare(LAYER_CORE) == 0) {
        src_port = (dst_id.host - 1) * 2;
        dst_port = src_id.pod;
    }
    else if (src_layer.compare(LAYER_CORE) == 0 && 
             dst_layer.compare(LAYER_AGG) == 0) {
        src_port = dst_id.pod;
        dst_port = (src_id.host - 1) * 2;
    }
    else if (src_layer.compare(LAYER_AGG) == 0 && 
             dst_layer.compare(LAYER_EDGE) == 0) {
        src_port = dst_id.sw * 2 + 1;
        dst_port = (src_id.sw - this->k / 2) * 2;
    }
    else if (src_layer.compare(LAYER_CORE) == 0 && 
             dst_layer.compare(LAYER_EDGE) == 0) {
        src_port = dst_id.pod;
        dst_port = (src_id.sw - 2) * 2;
    }
    else if (src_layer.compare(LAYER_EDGE) == 0 && 
             dst_layer.compare(LAYER_HOST) == 0){
        src_port = (dst_id.host - 2) * 2 + 1;
        dst_port = 0;
    }
    else {
        std::cout << "Could not find port leading to given dst switch" << "\n";
    }
    // Shift by one; as of v0.9, OpenFlow ports are 1-indexed.
    if (src_layer.compare(LAYER_HOST)) {
        src_port += 1;
    }
    if (dst_layer.compare(LAYER_HOST)){
        dst_port += 1;
    }
    return std::make_pair(src_port, dst_port);
}

}; //namespace ns3

int main(int argc, char const *argv[])
{
    /* code */

    ns3::FatTreeTopo topo = ns3::build(4, 1);
    topo.printLinks();
    return 0;
}