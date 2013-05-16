#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#include "cliquesearcher.hpp"

namespace gc = greycore;

typedef std::vector<std::vector<std::size_t>> cs_workdataobj_t;
typedef cs_workdataobj_t* cs_workdata_t;
typedef std::atomic<std::size_t> cs_progressobj_t;
typedef cs_progressobj_t* cs_progress_t;

cs_workdata_t createWorkdata(std::shared_ptr<gc::Graph> graph) {
	auto result = new cs_workdataobj_t(graph->getSize());

	for (std::size_t i = 0; i < graph->getSize(); ++i) {
		auto tmp = graph->get(i);
		std::vector<std::size_t> v(tmp.begin(), tmp.end());
		v.shrink_to_fit();
		(*result)[i] = move(v);
	}

	return result;
}

inline void testDifference(std::size_t& winnerValue, std::list<std::size_t>& recursionElements, const std::set<std::size_t>& p, std::vector<std::size_t>& neighbors) {
	std::list<std::size_t> test;
	std::set_difference(p.begin(), p.end(), neighbors.begin(), neighbors.end(), back_inserter(test));

	std::size_t s = test.size();
	if (s < winnerValue) {
		winnerValue = s;
		recursionElements = move(test);
	}
}

std::list<std::vector<std::size_t>> bronKerboschPivot(std::set<std::size_t>&& p, std::vector<std::size_t>&& r, std::set<std::size_t>&& x, cs_workdata_t data) {
	std::list<std::vector<std::size_t>> result;

	if (p.empty() && x.empty()) {
		r.shrink_to_fit();
		result.push_back(r);
	} else {
		// choose pivot
		std::size_t winnerValue = p.size() + 1;
		std::list<std::size_t> recursionElements;
		for (auto u : p) {
			testDifference(winnerValue, recursionElements, p, (*data)[u]);
		}
		for (auto u : x) {
			testDifference(winnerValue, recursionElements, p, (*data)[u]);
		}

		// recursion loop
		for (auto v : recursionElements) {
			auto& neighbors = (*data)[v];

			// build p, r, x
			std::set<std::size_t> pNew;
			std::set_intersection(p.begin(), p.end(), neighbors.begin(), neighbors.end(), inserter(pNew, pNew.end()));
			std::vector<std::size_t> rNew(r);
			rNew.push_back(v);
			std::set<std::size_t> xNew;
			std::set_intersection(x.begin(), x.end(), neighbors.begin(), neighbors.end(), inserter(xNew, xNew.end()));

			// call recursive function and store results
			result.splice(result.end(), bronKerboschPivot(move(pNew), move(rNew), move(xNew), data));

			// prepare next round
			p.erase(v);
			x.insert(v);
		}
	}

	return result;
}

class TBBBKHelper {
	public:
		std::list<std::vector<std::size_t>> result;

		TBBBKHelper(cs_workdata_t _data, cs_progress_t _progress) :
			data(_data),
			progress(_progress) {}

		TBBBKHelper(TBBBKHelper& obj, tbb::split) :
			data(obj.data),
			progress(obj.progress) {}

		void operator()(const tbb::blocked_range<std::size_t>& range) {
			for (auto v = range.begin(); v != range.end(); ++v) {
				auto& neighbors = (*data)[v];

				// split neighbors
				auto splitPoint = std::find_if(neighbors.begin(), neighbors.end(), [v](std::size_t i){return i>v;});
				std::set<std::size_t> p(splitPoint, neighbors.end());
				std::set<std::size_t> x(neighbors.begin(), splitPoint);

				// prepare r
				std::vector<std::size_t> r({v});

				// shoot and merge
				result.splice(result.end(), bronKerboschPivot(move(p), move(r), move(x), data));

				// report progress
				std::size_t pr = (*this->progress)++;
				if (pr % 100 == 0) {
					std::cout << pr << std::flush;
				} else if (pr % 10 == 0) {
					std::cout << "." << std::flush;
				}
			}
		}

		void join(TBBBKHelper& obj) {
			this->result.splice(this->result.end(), obj.result);
		}

	private:
		cs_workdata_t data;
		cs_progress_t progress;
};

std::list<std::vector<std::size_t>> bronKerboschDegeneracy(std::shared_ptr<gc::Graph> data) {
	std::cout << "Search cliques: " << std::flush;
	std::unique_ptr<cs_workdataobj_t> workdata(createWorkdata(data));
	std::unique_ptr<cs_progressobj_t> progress(new cs_progressobj_t(0));

	TBBBKHelper helper(workdata.get(), progress.get());
	parallel_reduce(tbb::blocked_range<std::size_t>(0, workdata->size()), helper);

	std::cout << "done (found " << helper.result.size() << " cliques)" << std::endl;

	return helper.result;
}

