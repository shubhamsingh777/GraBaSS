#ifndef SYS_HPP
#define SYS_HPP

#include <cstddef>
#include <list>
#include <memory>

#include "greycore/dim.hpp"
#include "greycore/wrapper/flatmap.hpp"

#include "hash.hpp"

typedef double data_t;
typedef int mdId_t;
typedef greycore::Dim<data_t> datadimObj_t;
typedef std::shared_ptr<datadimObj_t> datadim_t;
typedef greycore::Dim<std::size_t> discretedimObj_t;
typedef std::shared_ptr<discretedimObj_t> discretedim_t;
typedef greycore::Flatmap<mdId_t, data_t, 8> mdMapObj_t;
typedef std::shared_ptr<mdMapObj_t> mdMap_t;
typedef std::list<std::size_t> subspace_t;

constexpr std::size_t PAGE_SIZE = 4096;

bool checkConfig();

#endif

