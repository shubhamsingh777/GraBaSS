#ifndef HASH_HPP
#define HASH_HPP

#include <functional>
#include <set>
#include <vector>

template<>
struct std::hash<std::vector<std::size_t>> {
	std::size_t operator()(const std::vector<std::size_t>& obj) const;
};

template<>
struct std::hash<std::set<std::size_t>> {
	std::size_t operator()(const std::set<std::size_t>& obj) const;
};

#endif

