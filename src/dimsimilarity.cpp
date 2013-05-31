#include "sys.hpp"
#include "dimsimilarity.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <iostream>

struct BestCaseHelper {
	std::size_t capacity;
	std::size_t pos;

	bool operator<(const BestCaseHelper& obj) const {
		return this->capacity < obj.capacity;
	}
};

inline data_t calcDelta(discretedim_t& binsA, discretedim_t& binsB, std::vector<std::vector<std::size_t>>& grid, std::size_t n) {
	data_t delta = 0.0;

	for (std::size_t i = 0; i < binsA->getSize(); ++i) {
		data_t dA = static_cast<data_t>((*binsA)[i]) / static_cast<data_t>(n);
		for (std::size_t j = 0; j < binsB->getSize(); ++j) {
			data_t dB = static_cast<data_t>((*binsB)[j]) / static_cast<data_t>(n);
			data_t e = dA * dB * static_cast<data_t>(n);

			delta += fabs(static_cast<data_t>(grid[i][j]) - e);
		}
	}

	return delta;
}

data_t dimsimilarity(discretedim_t dimA, discretedim_t binsA, discretedim_t dimB, discretedim_t binsB) {
	assert(dimA->getSize() == dimB->getSize());
	std::size_t n = dimA->getSize();

	// build grid
	std::vector<std::vector<std::size_t>> grid(binsA->getSize(), std::vector<std::size_t>(binsB->getSize(), 0));
	for (std::size_t s = 0; s < dimA->getSegmentCount(); ++s) {
		discretedimObj_t::segment_t* sPtr1 = dimA->getSegment(s);
		discretedimObj_t::segment_t* sPtr2 = dimB->getSegment(s);

		for (std::size_t i = 0; i < dimA->getSegmentFillSize(s); ++i) {
			++grid[(*sPtr1)[i]][(*sPtr2)[i]];
		}
	}

	// find best case (yep, greedy stuff)
	std::vector<std::vector<std::size_t>> gridBest(binsA->getSize(), std::vector<std::size_t>(binsB->getSize(), 0));
	std::vector<BestCaseHelper> tmpA;
	for (std::size_t i = 0; i < binsA->getSize(); ++i) {
		assert((*binsA)[i] > 0);
		tmpA.push_back({(*binsA)[i], i});
	}
	std::make_heap(tmpA.begin(), tmpA.end());
	std::vector<BestCaseHelper> tmpB;
	for (std::size_t j = 0; j < binsB->getSize(); ++j) {
		assert((*binsB)[j] > 0);
		tmpB.push_back({(*binsB)[j], j});
	}
	std::make_heap(tmpB.begin(), tmpB.end());
	while ((!tmpA.empty()) && (!tmpB.empty())) {
		auto bA = tmpA.front();
		auto bB = tmpB.front();
		std::pop_heap(tmpA.begin(), tmpA.end()); tmpA.pop_back();
		std::pop_heap(tmpB.begin(), tmpB.end()); tmpB.pop_back();

		std::size_t x = std::min(bA.capacity, bB.capacity);
		bA.capacity -= x;
		bB.capacity -= x;

		if (bA.capacity > 0) {
			tmpA.push_back(bA);
			std::push_heap(tmpA.begin(), tmpA.end());
		}
		if (bB.capacity > 0) {
			tmpB.push_back(bB);
			std::push_heap(tmpB.begin(), tmpB.end());
		}

		gridBest[bA.pos][bB.pos] = x;
	}

	// calculate result
	data_t delta = calcDelta(binsA, binsB, grid, n);
	data_t deltaBest = calcDelta(binsA, binsB, gridBest, n);

	return std::min(1.0, std::max(0.0, delta / deltaBest)); // fix numeric errors
}

