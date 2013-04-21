#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#include "cliquesearcher.hpp"

using namespace std;

inline void testDifference(long& winnerValue, list<bigid_t>& recursionElements, const set<bigid_t>& p, const list<bigid_t>&& neighbors) {
	list<bigid_t> test;
	set_difference(p.begin(), p.end(), neighbors.begin(), neighbors.end(), back_inserter(test));

	long s = static_cast<long>(test.size());
	if (s < winnerValue) {
		winnerValue = s;
		recursionElements = move(test);
	}
}

list<list<bigid_t>> bronKerboschPivot(set<bigid_t>&& p, list<bigid_t>&& r, set<bigid_t>&& x, graph_t data) {
	list<list<bigid_t>> result;

	if (p.empty() && x.empty()) {
		result.push_back(r);
	} else {
		// choose pivot
		long winnerValue = p.size() + 1;
		list<bigid_t> recursionElements;
		for (auto u : p) {
			testDifference(winnerValue, recursionElements, p, data->get(u));
		}
		for (auto u : x) {
			testDifference(winnerValue, recursionElements, p, data->get(u));
		}

		// recursion loop
		for (auto v : recursionElements) {
			list<bigid_t> neighbors = data->get(v);

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

list<list<bigid_t>> bronKerboschDegeneracy(graph_t data) {
	list<list<bigid_t>> result;

	cout << "Search cliques: " << flush;
	for (bigid_t v = 0; v < data->getSize(); ++v) {
		list<bigid_t> neighbors = data->get(v);

		// split neighbors
		auto splitPoint = find_if(neighbors.begin(), neighbors.end(), [v](bigid_t i){return i>v;});
		set<bigid_t> p(splitPoint, neighbors.end());
		set<bigid_t> x(neighbors.begin(), splitPoint);

		// prepare r
		list<bigid_t> r({v});

		// shoot and merge
		result.splice(result.end(), bronKerboschPivot(move(p), move(r), move(x), data));

		// report progress
		if (v % 100 == 0) {
			cout << v << flush;
		} else if (v % 10 == 0) {
			cout << "." << flush;
		}
	}

	result.reverse();
	cout << "done (found " << result.size() << " cliques)" << endl;

	return result;
}

