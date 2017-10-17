#ifndef UTIL_H
#define UTIL_H 1

#include <iostream>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstring>
#include <algorithm>
#include <iterator>
 
inline std::string format(const std::string& format, ...)
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

#endif