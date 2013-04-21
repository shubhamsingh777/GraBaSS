#ifndef GRAPHTRANSFORMATION_HPP
#define GRAPHTRANSFORMATION_HPP

#include <unordered_map>
#include <vector>

#include "graph.hpp"
#include "sys.hpp"

void lookupNeighbors(graph_t input, graph_t output);
void joinEdges(std::vector<graph_t> input, graph_t output);
std::unordered_map<bigid_t, bigid_t> sortGraph(graph_t input, graph_t output);

#endif

