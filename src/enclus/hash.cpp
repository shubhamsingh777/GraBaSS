#include "hash.hpp"

std::hash<std::size_t> myHasher;

std::size_t std::hash<std::vector<std::size_t>>::operator()(const std::vector<std::size_t>& obj) const {
	std::size_t result = 0;

	for (const auto& element : obj) {
		result ^= myHasher(element);
	}

	return result;
}

std::size_t std::hash<std::set<std::size_t>>::operator()(const std::set<std::size_t>& obj) const {
	std::size_t result = 0;

	for (const auto& element : obj) {
		result ^= myHasher(element);
	}

	return result;
}

