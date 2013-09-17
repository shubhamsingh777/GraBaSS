#ifndef ENTROPY_HPP
#define ENTROPY_HPP

#include <vector>

#include "sys.hpp"

data_t calcEntropy(const subspace_t& subspace, const std::vector<discretedim_t>& data);

#endif

