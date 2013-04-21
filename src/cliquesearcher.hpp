#ifndef CLIQUESEARCHER_HPP
#define CLIQUESEARCHER_HPP

#include <list>
#include <set>

#include "graph.hpp"
#include "sys.hpp"

std::list<std::set<bigid_t>> bronKerboschDegeneracy(graph_t data);

#endif

