#ifndef CLIQUESEARCHER_HPP
#define CLIQUESEARCHER_HPP

#include <list>
#include <memory>
#include <set>
#include <unordered_map>

#include "graph.hpp"

std::unordered_map<long, long> sortGraph(std::shared_ptr<Graph> input, std::shared_ptr<Graph> output);
std::list<std::set<long>> bronKerboschDegeneracy(std::shared_ptr<Graph> data);

#endif

