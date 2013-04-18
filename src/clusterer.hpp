#ifndef CLUSTERER_HPP
#define CLUSTERER_HPP

#include <list>
#include <memory>
#include <set>

#include "graph.hpp"

void sortGraph(std::shared_ptr<Graph> input, std::shared_ptr<Graph> output);
std::list<std::set<long>> bronKerboschDegeneracy(std::shared_ptr<Graph> data);

#endif

