#ifndef ENCLUS_HPP
#define ENCLUS_HPP

#include <list>
#include <unordered_set>
#include <vector>

#include "sys.hpp"
#include "hash.hpp"

typedef std::unordered_map<std::vector<std::size_t>, data_t> density_t;
typedef std::list<std::size_t> subspace_t;
typedef std::unordered_set<subspace_t> subspaces_t;
typedef std::vector<data_t> entropyCache_t;

#endif

