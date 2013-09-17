#include "entropy.hpp"

#include <cmath>
#include <unordered_map>

typedef std::unordered_map<std::vector<std::size_t>, data_t> density_t;

density_t calcDensity(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density;
	std::size_t nSegments = data.at(0)->getSegmentCount();
	std::size_t n = data.at(0)->getSize();
	data_t step = 1.0 / static_cast<data_t>(n);
	std::vector<size_t> pos(subspace.size());

	for (std::size_t segment = 0; segment < nSegments; ++segment) {
		std::size_t size = data.at(0)->getSegmentFillSize(segment);

		typename std::vector<discretedimObj_t::segment_t*> segmentPtrs(subspace.size());
		std::size_t ptrIter = 0;
		for (std::size_t s : subspace) {
			const discretedim_t& dim = data.at(s);
			segmentPtrs[ptrIter++] = dim->getSegment(segment);
		}

		for (std::size_t i = 0; i < size; ++i) {
			std::size_t posIter = 0;

			for (const auto& segmentPtr : segmentPtrs) {
				pos[posIter++] = (*segmentPtr)[i];
			}

			density[pos] += step;
		}
	}

	return density;
}

data_t calcEntropy(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density = calcDensity(subspace, data);
	data_t result = 0.0;

	for (const auto& kv : density) {
		data_t p = kv.second;
		result -= p * log2(p);
	}

	return result;
}

