#include "sys.hpp"
#include "dimsimilarity.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <iostream>

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

	// calc entropies
	data_t entropyX = 0.0;
	for (std::size_t i = 0; i < binsA->getSize(); ++i) {
		std::size_t c = (*binsA)[i];

		if (c > 0) {
			data_t px = static_cast<data_t>(c) / static_cast<data_t>(n);
			entropyX -= px * log2(px);
		}
	}

	data_t entropyY = 0.0;
	for (std::size_t j = 0; j < binsB->getSize(); ++j) {
		std::size_t c = (*binsB)[j];

		if (c > 0) {
			data_t py = static_cast<data_t>(c) / static_cast<data_t>(n);
			entropyY -= py * log2(py);
		}
	}

	data_t entropyXY = 0.0;
	for (std::size_t i = 0; i < binsA->getSize(); ++i) {
		for (std::size_t j = 0; j < binsB->getSize(); ++j) {
			std::size_t c = grid[i][j];

			if (c > 0) {
				data_t pxy = static_cast<data_t>(c) / static_cast<data_t>(n);
				entropyXY -= pxy * log2(pxy);
			}
		}
	}

	// calc mutual information
	data_t mi = entropyX + entropyY - entropyXY;

	// return normalized form
	return mi / fmin(entropyX, entropyY);
}

