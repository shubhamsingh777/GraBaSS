#ifndef GRAPHBUILDER_HPP
#define GRAPHBUILDER_HPP

#include <memory>
#include <vector>

#include "dim.hpp"
#include "graph.hpp"
#include "sys.hpp"

void buildGraph(std::vector<datadim_t> data, std::shared_ptr<Graph> graph, data_t threshold);

#endif

