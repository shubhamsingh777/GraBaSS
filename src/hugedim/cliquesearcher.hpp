#ifndef CLIQUESEARCHER_HPP
#define CLIQUESEARCHER_HPP

#include <list>
#include <memory>
#include <vector>

#include "greycore/wrapper/graph.hpp"
#include "sys.hpp"

std::list<std::vector<std::size_t>> bronKerboschDegeneracy(std::shared_ptr<greycore::Graph> data);

#endif

