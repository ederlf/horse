#ifndef HEDERAUTIL_H
#define HEDERAUTIL_H 1

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <inttypes.h>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

inline
std::string format(const std::string& format, ...)
{
    va_list args;
    va_start (args, format);
    size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
    va_end (args);
    std::vector<char> vec(len + 1);
    va_start (args, format);
    std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
    va_end (args);
    return &vec[0];
}

template <class Container>
inline void split(const std::string& str, Container& cont,
              char delim = ' ')
{
    std::size_t current, previous = 0;
    current = str.find(delim);
    while (current != std::string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim, previous);
    }
    cont.push_back(str.substr(previous, current - previous));
}

inline
bool pairCompare(const std::pair<std::string, std::string>& firstElem, const std::pair<std::string, std::string>& secondElem) {
  return firstElem.first.compare(secondElem.first) < 0;
}

inline
bool lexiCompare(const std::vector<std::string>& firstElem, const std::vector<std::string>& secondElem) {
  return  std::lexicographical_compare(firstElem.begin(), firstElem.end(),
                                        firstElem.begin(), firstElem.end());
}

inline
// # Host index is in [0, k**3/4)
std::string get_host_name(uint32_t k, uint32_t host_index){
    uint32_t pod_index = host_index / ((k*k) / 4);
    uint32_t edge_index = (host_index % ((k*k) / 4)) / (k / 2);
    uint32_t link_index = host_index % (k / 2) + 2;
    char id[] = "%d_%d_%d";
    return format(id, pod_index, edge_index, link_index);
}

inline
uint32_t get_host_index(uint32_t k, uint32_t pod, uint32_t edge, 
                        uint32_t link) {
    return (pod * ((k*k) / 4)) + edge * (k/2) + link - 2;
}

inline 
double random_real_distr(void)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

inline 
int random_int(int lower_bound, int upper_bound)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(lower_bound, upper_bound);
    return dis(gen);
}

// # Begin traffic pattern #######################

inline std::vector<uint32_t> 
compute_stride(uint32_t k, uint32_t stride)
{
    std::vector<uint32_t> matrix = {};
    for (uint32_t src_index = 0; src_index < (k*k*k)/4; ++src_index )
    {
        matrix.push_back( (src_index + stride) % ((k*k*k) / 4) );
    } 
    return matrix;  
}


inline std::vector<uint32_t> 
compute_stagger_prob(uint32_t k, double sameEdgeProb, double samePodProb )
{
    std::vector<uint32_t> matrix = {};
    if (k == 2){
        matrix.push_back(1);
        matrix.push_back(0);
        return matrix;
    }
    for (uint32_t host_index = 0; host_index < (k*k*k)/4; ++host_index){
        uint32_t p = host_index / ((k*k) / 4);
        uint32_t e = (host_index % ((k*k) / 4)) / (k / 2);
        uint32_t l = host_index % (k / 2) + 2; 
        if (random_real_distr() < sameEdgeProb) {
            uint32_t nl =  2 + random_int(0, k/2 - 1);
            while (nl == l){
                nl = 2 + random_int(0, k/2 - 1);
            }
            matrix.push_back(get_host_index(k, p, e , nl));
        }
        else if (random_real_distr() < samePodProb){
            l = 2 + random_int(0, k/2 - 1);
            uint32_t ne = random_int(0, k/2 - 1);
            while (ne == e) {
                ne = random_int(0, k/2 - 1);
            }
            matrix.push_back(get_host_index(k, p, ne, l));
        }
        else {
            uint32_t np = random_int(0, k-1);
            while (np == p){
               np = random_int(0, k-1); 
            }
          l = 2 + random_int(0, k/2 - 1);        
          e = random_int(0, k/2 - 1);
          matrix.push_back(get_host_index(k, np, e, l));    
        }
    }
    return matrix;
}

inline std::vector<uint32_t> 
compute_random(uint32_t k)
{
    std::vector<uint32_t> matrix = {};
    uint32_t nHosts = (k*k*k)/4 - 1;
    for(uint32_t ind = 0; ind < nHosts +1; ++ind){
        uint32_t dst = random_int(0, nHosts);
        while (dst == ind) {
           dst = random_int(0, nHosts); 
        }
        matrix.push_back(dst);
    }
    return matrix;
}

inline std::vector<uint32_t> 
compute_randbij(uint32_t k)
{
    std::vector<uint32_t> matrix = {};
    for (uint32_t i = 0; i < (k*k*k)/4; ++i){
        matrix.push_back(i);
    } 
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(matrix.begin(), matrix.end(), g);
    return matrix;
}

#endif