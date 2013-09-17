#ifndef GRAPHTRANSFORMATION_HPP
#define GRAPHTRANSFORMATION_HPP

#include <memory>
#include <unordered_map>
#include <vector>

#include "greycore/wrapper/graph.hpp"
#include "sys.hpp"

void bidirLookup(std::shared_ptr<greycore::Graph> input, std::shared_ptr<greycore::Graph> output, double threshold);
void lookupNeighbors(std::shared_ptr<greycore::Graph> input, std::shared_ptr<greycore::Graph> output);
void joinEdges(std::vector<std::shared_ptr<greycore::Graph>> input, std::shared_ptr<greycore::Graph> output);
std::unordered_map<std::size_t, std::size_t> sortGraph(std::shared_ptr<greycore::Graph> input, std::shared_ptr<greycore::Graph> output);

#endif

