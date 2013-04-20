#ifndef GRAPHBUILDER_HPP
#define GRAPHBUILDER_HPP

#include <memory>
#include <vector>

#include "dim.hpp"
#include "graph.hpp"

void buildGraph(std::vector<std::shared_ptr<Dim<double, double>>> data, std::shared_ptr<Graph> graph, double threshold);

#endif

