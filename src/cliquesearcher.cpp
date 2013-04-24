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

using namespace std;

typedef vector<vector<bigid_t>> cs_workdataobj_t;
typedef cs_workdataobj_t* cs_workdata_t;
typedef atomic<bigid_t> cs_progressobj_t;
typedef cs_progressobj_t* cs_progress_t;

cs_workdata_t createWorkdata(graph_t graph) {
	auto result = new cs_workdataobj_t(graph->getSize());

	for (bigid_t i = 0; i < graph->getSize(); ++i) {
		auto tmp = graph->get(i);
		vector<bigid_t> v(tmp.begin(), tmp.end());
		v.shrink_to_fit();
		(*result)[i] = move(v);
	}

	return result;
}

inline void testDifference(long& winnerValue, list<bigid_t>& recursionElements, const set<bigid_t>& p, vector<bigid_t>& neighbors) {
	list<bigid_t> test;
	set_difference(p.begin(), p.end(), neighbors.begin(), neighbors.end(), back_inserter(test));

	long s = static_cast<long>(test.size());
	if (s < winnerValue) {
		winnerValue = s;
		recursionElements = move(test);
	}
}

list<list<bigid_t>> bronKerboschPivot(set<bigid_t>&& p, list<bigid_t>&& r, set<bigid_t>&& x, cs_workdata_t data) {
	list<list<bigid_t>> result;

	if (p.empty() && x.empty()) {
		result.push_back(r);
	} else {
		// choose pivot
		long winnerValue = p.size() + 1;
		list<bigid_t> recursionElements;
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
			set<bigid_t> pNew;
			set_intersection(p.begin(), p.end(), neighbors.begin(), neighbors.end(), inserter(pNew, pNew.end()));
			list<bigid_t> rNew(r);
			rNew.push_back(v);
			set<bigid_t> xNew;
			set_intersection(x.begin(), x.end(), neighbors.begin(), neighbors.end(), inserter(xNew, xNew.end()));

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
		list<list<bigid_t>> result;

		TBBBKHelper(cs_workdata_t _data, cs_progress_t _progress) :
			data(_data),
			progress(_progress) {}

		TBBBKHelper(TBBBKHelper& obj, tbb::split) :
			data(obj.data),
			progress(obj.progress) {}

		void operator()(const tbb::blocked_range<bigid_t>& range) {
			for (auto v = range.begin(); v != range.end(); ++v) {
				auto& neighbors = (*data)[v];

				// split neighbors
				auto splitPoint = find_if(neighbors.begin(), neighbors.end(), [v](bigid_t i){return i>v;});
				set<bigid_t> p(splitPoint, neighbors.end());
				set<bigid_t> x(neighbors.begin(), splitPoint);

				// prepare r
				list<bigid_t> r({v});

				// shoot and merge
				result.splice(result.end(), bronKerboschPivot(move(p), move(r), move(x), data));

				// report progress
				bigid_t pr = (*this->progress)++;
				if (pr % 100 == 0) {
					cout << pr << flush;
				} else if (pr % 10 == 0) {
					cout << "." << flush;
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

list<list<bigid_t>> bronKerboschDegeneracy(graph_t data) {
	cout << "Search cliques: " << flush;
	unique_ptr<cs_workdataobj_t> workdata(createWorkdata(data));
	unique_ptr<cs_progressobj_t> progress(new cs_progressobj_t(0));

	TBBBKHelper helper(workdata.get(), progress.get());
	parallel_reduce(tbb::blocked_range<bigid_t>(0, static_cast<bigid_t>(workdata->size())), helper);

	cout << "done (found " << helper.result.size() << " cliques)" << endl;

	return helper.result;
}

