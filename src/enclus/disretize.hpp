#ifndef DISCRETIZE_HPP
#define DISCRETIZE_HPP

#include <memory>
#include <vector>

#include "greycore/database.hpp"

#include "sys.hpp"

std::vector<discretedim_t> discretize(const std::vector<datadim_t>& dims, std::shared_ptr<greycore::Database>& dbDiscrete, std::size_t xi);

#endif

