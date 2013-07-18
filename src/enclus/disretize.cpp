#include <iostream>
#include <tuple>

#include "disretize.hpp"

namespace gc = greycore;

std::tuple<data_t, data_t> detectRange(const datadim_t& dim) {
	std::size_t nSegments = dim->getSegmentCount();
	bool first = true;
	data_t min;
	data_t max;

	for (std::size_t segment = 0; segment < nSegments; ++segment) {
		std::size_t size = dim->getSegmentFillSize(segment);
		typename datadimObj_t::segment_t* data = dim->getSegment(segment);

		for (std::size_t i = 0; i < size; ++i) {
			data_t x = (*data)[i];

			if (first) {
				min = x;
				max = x;
				first = false;
			} else {
				min = std::min(min, x);
				max = std::max(max, x);
			}
		}
	}

	return std::make_tuple(min, max);
}

std::vector<discretedim_t> discretize(const std::vector<datadim_t>& dims, std::shared_ptr<gc::Database>& dbDiscrete, std::size_t xi) {
	std::cout << "Discretize: " << std::flush;
	std::vector<discretedim_t> ddims;
	std::size_t dimCounter = 0;

	for (const auto& dim : dims) {
		// prepare
		std::size_t nSegments = dim->getSegmentCount();

		// detect range
		data_t min;
		data_t max;
		std::tie(min, max) = detectRange(dim);
		data_t step = (max - min) / static_cast<data_t>(xi);

		// points => bins
		auto dd = dbDiscrete->createDim<std::size_t>(dim->getName() + ".discrete");
		for (std::size_t segment = 0; segment < nSegments; ++segment) {
			std::size_t size = dim->getSegmentFillSize(segment);
			typename datadimObj_t::segment_t* data = dim->getSegment(segment);

			for (std::size_t i = 0; i < size; ++i) {
				std::size_t pos = static_cast<std::size_t>(floor(((*data)[i] - min) / step));
				dd->add(pos);
			}
		}

		// store
		ddims.push_back(dd);

		// report progress
		++dimCounter;
		if (dimCounter % 1000 == 0) {
			std::cout << dimCounter << std::flush;
		} else if (dimCounter % 100 == 0) {
			std::cout << "." << std::flush;
		}
	}

	std::cout << "done" << std::endl;

	return ddims;
}

