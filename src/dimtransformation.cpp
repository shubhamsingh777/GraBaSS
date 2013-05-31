#include "sys.hpp"
#include "dimtransformation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

struct SortHelper {
	data_t valueData;
	std::size_t valueDiscrete;
	std::size_t pos;
};

void discretizeDim(datadim_t input, discretedim_t output, discretedim_t bins) {
	assert(output->getSize() == 0);
	assert(bins->getSize() == 0);

	// build helper data
	std::vector<SortHelper> tmp;
	std::size_t pos = 0;
	for (std::size_t s = 0; s < input->getSegmentCount(); ++s) {
		datadimObj_t::segment_t* sPtr = input->getSegment(s);
		for (std::size_t i = 0; i < input->getSegmentFillSize(s); ++i) {
			tmp.push_back({(*sPtr)[i], 0, pos++});
		}
	}

	// sort
	std::sort(tmp.begin(), tmp.end(), [](const SortHelper& a, const SortHelper& b){
				return a.valueData < b.valueData;
			});

	// discretize
	data_t step = sqrt(static_cast<data_t>(input->getSize()));
	std::size_t maxDiscrete = static_cast<std::size_t>(floor(static_cast<data_t>(input->getSize()) / step)) - 1; // do not simplify this until the final step calculation is known
	std::size_t discrete = 0;
	data_t jumper = 0.0;
	data_t lastValue = std::numeric_limits<data_t>::signaling_NaN();
	std::size_t bin = 0;
	for (std::size_t i = 0; i < tmp.size(); ++i) {
		if (jumper < step) {
			jumper += 1.0;
		} else if (tmp[i].valueData != lastValue) {
			jumper -= step;
			discrete = std::min(maxDiscrete, discrete + 1); // handle some special cases
			bins->add(bin);
			bin = 0;
		}

		tmp[i].valueDiscrete = discrete;
		++bin;
		lastValue = tmp[i].valueData;
	}
	if (bin != 0) {
		bins->add(bin);
	}

	// sort again
	std::sort(tmp.begin(), tmp.end(), [](const SortHelper& a, const SortHelper& b){
				return a.pos < b.pos;
			});

	// write output
	for (const auto& helper : tmp) {
		output->add(helper.valueDiscrete);
	}
}

