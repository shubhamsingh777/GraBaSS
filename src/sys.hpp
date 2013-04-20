#ifndef SYS_HPP
#define SYS_HPP

#include <cstddef>

typedef double data_t;
typedef long bigid_t;

constexpr std::size_t PAGE_SIZE = 4096;

bool checkConfig();

#endif

