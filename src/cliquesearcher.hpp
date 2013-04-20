#ifndef CLIQUESEARCHER_HPP
#define CLIQUESEARCHER_HPP

#include <list>
#include <memory>
#include <set>
#include <unordered_map>

#include "graph.hpp"
#include "sys.hpp"

std::unordered_map<bigid_t, bigid_t> sortGraph(graph_t input, graph_t output);
std::list<std::set<bigid_t>> bronKerboschDegeneracy(graph_t data);

#endif

