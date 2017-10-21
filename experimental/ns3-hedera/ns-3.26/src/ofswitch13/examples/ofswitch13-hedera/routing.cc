#include "routing.h"
#include "hederautil.h"
#include <algorithm>
#include <cstring>
#include <random>
#include <zlib.h>
#include <arpa/inet.h>

namespace ns3 {

Routing::Routing(FatTreeTopo topo)
{
    this->topo = topo;
}

StructuredRouting::StructuredRouting(FatTreeTopo topo, _path_choice get_path): Routing(topo)
{
    this->path_choice = get_path;
}

Path StructuredRouting::get_route(std::string src, std::string dst, 
                                  uint8_t* data){
    Path path;
    if (src.compare(dst) == 0){
        path.push_back(src);
        return path;
    }

    Paths paths_found = this->get_all_route(src, dst);
    // std::cout << paths_found.size();
    if (paths_found.size()){
        return this->path_choice(paths_found, src, dst, data);
    }
    else{
        return path;
    }
}

Paths StructuredRouting::_extend_reachable(uint32_t frontier_layer)
{
    Paths complete_paths;

    if (this->src_path_layer > frontier_layer){
        NodePaths src_paths_next;
        for (auto &node : this->src_paths) {
           Paths src_path_list = this->src_paths[node.first];
           size_t src_path_list_size = src_path_list.size();
           if (src_path_list_size == 0) {
                continue;
           }
           std::string last = src_path_list[0].back();
           std::vector<std::pair<std::string, std::string>>  up_edges = this->topo.up_edges(last);
           if (up_edges.size() == 0) {
                continue;
           }
           std::vector<std::string> up_nodes = this->topo.up_nodes(last);
           std::sort(up_edges.begin(), up_edges.end(), pairCompare);

           for (auto &edge : up_edges) {
                std::string frontier_node = edge.second;
                NodePaths::const_iterator got = this->dst_paths.find (frontier_node);
                if ( got != this->dst_paths.end() ) {
                    Paths dst_path_list = this->dst_paths[frontier_node];
                    for (auto &dst_path: dst_path_list){
                        Path dst_path_rev = dst_path;
                        std::reverse(std::begin(dst_path_rev), std::end(dst_path_rev));
                        for (auto &src_path : src_path_list) {
                            Path new_path = src_path;
                            new_path.insert(new_path.end(), dst_path_rev.begin(), dst_path_rev.end());
                            complete_paths.push_back(new_path); 
                        }
                    }
                }
                else {
                    NodePaths::const_iterator got = src_paths_next.find (frontier_node);
                    if (got == src_paths_next.end()){
                        src_paths_next[frontier_node] = {};  
                    }
                    for (auto &src_path : src_path_list) {
                        Path extended_path = src_path; 
                        extended_path.push_back(frontier_node);
                        src_paths_next[frontier_node].push_back(extended_path);
                    }
                }
           }
        }
        this->src_paths = src_paths_next;
        this->src_path_layer -= 1;
    }

    if (this->dst_path_layer > frontier_layer) {
        NodePaths dst_paths_next;
        for (auto &node : this->dst_paths) {
            Paths dst_path_list = this->dst_paths[node.first]; 
            std::string last = dst_path_list[0].back();
            std::vector<std::pair<std::string, std::string>>  up_edges = this->topo.up_edges(last);
            if (up_edges.size() == 0) {
                continue;
            }
            std::vector<std::string> up_nodes = this->topo.up_nodes(last);
            for (auto &edge : up_edges) {
                std::string frontier_node = edge.second;
                NodePaths::const_iterator got = this->src_paths.find (frontier_node);
                if ( got != this->src_paths.end() ) {
                    Paths src_path_list = this->src_paths[frontier_node];
                    for (auto &src_path: src_path_list){
                        for (auto &dst_path : dst_path_list) {
                            Path dst_path_rev = dst_path;
                            std::reverse(std::begin(dst_path_rev), std::end(dst_path_rev));
                            Path new_path = src_path;
                            new_path.insert(new_path.end(), dst_path_rev.begin(), dst_path_rev.end());
                            complete_paths.push_back(new_path); 
                        }
                    }
                }
                else {
                    NodePaths::const_iterator got = dst_paths_next.find (frontier_node);
                    if (got == dst_paths_next.end()){
                        dst_paths_next[frontier_node] = {};  
                    }
                    for (auto &dst_path : dst_path_list) {
                        Path extended_path = dst_path; 
                        extended_path.push_back(frontier_node);
                        dst_paths_next[frontier_node].push_back(extended_path);
                    }
                }
           }
        }
        this->dst_paths = dst_paths_next;
        this->dst_path_layer -= 1;
    }
    return complete_paths;
}

Paths StructuredRouting::get_all_route(std::string src, std::string dst)
{
    Paths paths;

    if (src.compare(dst) == 0){
        Path p = {src};
        paths.push_back(p);
        return paths;
    }

    this->src_paths[src] = {{src}}; 
    this->dst_paths[dst] = {{dst}}; 

    std::string src_layer = this->topo.layer(src);
    std::string dst_layer = this->topo.layer(dst);

    // # use later in extend_reachable
    this->src_path_layer =  std::stoul(src_layer);
    this->dst_path_layer =  std::stoul(dst_layer);

    // # the lowest layer is the one closest to hosts, with the highest value
    uint32_t lowest_starting_layer =  this->src_path_layer;
    if (this->dst_path_layer > lowest_starting_layer) {
        lowest_starting_layer = this->dst_path_layer;
    }
    for(int depth = lowest_starting_layer - 1; depth > -1; --depth) {
        paths = this->_extend_reachable(depth);
        if (paths.size()){
            return paths;
        }
    }
    return paths;
}

void StructuredRouting::print_route(Path path)
{
    int i = 0;
    for (auto &node: path){
        char str[] = "%d - %s";
        std::cout << format(str, i, node.c_str()) << "\n"; 
        i+=1;
    }
}

RandomStructuredRouting::RandomStructuredRouting(FatTreeTopo topo): StructuredRouting(topo, RandomStructuredRouting::choose_random)
{
}

Path RandomStructuredRouting::choose_random(Paths paths, std::string src, std::string dst, uint8_t *data)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, paths.size()-1);
    return paths[dis(gen)];
}

STStructuredRouting::STStructuredRouting(FatTreeTopo topo): StructuredRouting(topo, STStructuredRouting::choose_random)
{
}

Path STStructuredRouting::choose_random(Paths paths, std::string src, std::string dst, uint8_t *data)
{
    return paths[0];
}

HashedStructuredRouting::HashedStructuredRouting(FatTreeTopo topo): StructuredRouting(topo, HashedStructuredRouting::choose_random)
{

}

Path HashedStructuredRouting::choose_random(Paths paths, std::string src, std::string dst, uint8_t *data)
{
    uint8_t bytes[14] = {};
    uint16_t offset = 12;
    uint16_t type;

    Path path;
    type = *((uint16_t*)(data + offset));
    if (ntohs(type) == 0x800) {
        /* Advance to IP fields */
        offset += 11;
        uint8_t ip_proto = *((uint8_t*)(data + offset));
        offset += 3;
        uint64_t ip_src_dst = *((uint64_t*)(data + offset));
        std::memcpy(&bytes[0], &ip_src_dst, sizeof(uint64_t));
        std::memcpy(&bytes[8], &ip_proto, sizeof(uint8_t));
        if (ip_proto == 17 || ip_proto == 6) {
            offset += 8;
            uint32_t tp_src_dst = *((uint32_t*)(data + offset));
            std::memcpy(&bytes[9], &tp_src_dst, sizeof(uint32_t));
        }
        uint32_t crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, bytes, 14);
        int choice = crc % paths.size();
        std::sort (paths.begin(), paths.end(), lexiCompare);
        path = paths[choice];
    } 
    return path;
} 

StructuredRouting *get_routing(std::string routing, FatTreeTopo topo)
{
    if (routing.compare("random") == 0){
        return new RandomStructuredRouting(topo);
    }
    else if (routing.compare("st") == 0) {
        return new STStructuredRouting(topo);  
    }
    else if (routing.compare("hashed") == 0) {
        return new HashedStructuredRouting(topo);
    } 
    return NULL;
}

}; // End namespace ns3


// int main(int argc, char const *argv[])
// {
//     /* code */
//     ns3::FatTreeTopo topo = ns3::FatTreeTopo::build(4, 1);
//     ns3::STStructuredRouting r = ns3::STStructuredRouting(topo);
//     ns3::Path p = r.get_route("0_0_2", "0_1_3", NULL);
//     r.print_route(p);
//     return 0;
// }

