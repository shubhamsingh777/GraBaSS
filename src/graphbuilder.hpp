#ifndef GRAPHBUILDER_HPP
#define GRAPHBUILDER_HPP

#include <memory>
#include <utility>
#include <vector>

#include "greycore/dim.hpp"
#include "greycore/wrapper/graph.hpp"
#include "sys.hpp"

void buildGraph(std::vector<std::pair<discretedim_t, discretedim_t>> data, std::shared_ptr<greycore::Graph> graph, data_t threshold);

#endif

