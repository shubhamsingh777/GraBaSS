#ifndef GRAPHTRANSFORMATION_HPP
#define GRAPHTRANSFORMATION_HPP

#include <unordered_map>

#include "graph.hpp"
#include "sys.hpp"

std::unordered_map<bigid_t, bigid_t> sortGraph(graph_t input, graph_t output);

#endif

