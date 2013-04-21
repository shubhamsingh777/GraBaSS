#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "cliquesearcher.hpp"

using namespace std;

list<set<bigid_t>> bronKerboschPivot(set<bigid_t> p, set<bigid_t> r, set<bigid_t> x, graph_t data) {
	list<set<bigid_t>> result;
	set<bigid_t> all;
	set_union(p.begin(), p.end(), x.begin(), x.end(), inserter(all, all.end()));

	if (all.empty()) {
		result.push_back(r);
	} else {
		// choose pivot
		bigid_t winner = -1;
		long winnerValue = -1;
		list<bigid_t> winnerNeighbors;
		for (auto u : all) {
			list<bigid_t> neighbors = data->get(u);
			set<bigid_t> test;
			set_intersection(p.begin(), p.end(), neighbors.begin(), neighbors.end(), inserter(test, test.end()));
			long s = static_cast<long>(test.size());
			if (s > winnerValue) {
				winnerValue = s;
				winner = u;
				winnerNeighbors = neighbors;
			}
		}
		assert(winner != -1);

		// build set for recursion
		set<bigid_t> recursionElements;
		set_difference(p.begin(), p.end(), winnerNeighbors.begin(), winnerNeighbors.end(), inserter(recursionElements, recursionElements.end()));

		// recursion loop
		for (auto v : recursionElements) {
			list<bigid_t> neighbors = data->get(v);

			// build p, r, x
			set<bigid_t> pNew;
			set_intersection(p.begin(), p.end(), neighbors.begin(), neighbors.end(), inserter(pNew, pNew.end()));
			set<bigid_t> rNew(r);
			rNew.insert(v);
			set<bigid_t> xNew;
			set_intersection(x.begin(), x.end(), neighbors.begin(), neighbors.end(), inserter(xNew, xNew.end()));

			// call recursive function and store results
			result.splice(result.end(), bronKerboschPivot(pNew, rNew, xNew, data));

			// prepare next round
			p.erase(v);
			x.insert(v);
		}
	}

	return result;
}

list<set<bigid_t>> bronKerboschDegeneracy(graph_t data) {
	list<set<bigid_t>> result;

	cout << "Search cliques: " << flush;
	for (bigid_t v = 0; v < data->getSize(); ++v) {
		list<bigid_t> neighbors = data->get(v);

		// split neighbors
		auto splitPoint = find_if(neighbors.begin(), neighbors.end(), [v](bigid_t i){return i>v;});
		set<bigid_t> p(splitPoint, neighbors.end());
		set<bigid_t> x(neighbors.begin(), splitPoint);

		// prepare r
		set<bigid_t> r({v});

		// shoot and merge
		result.splice(result.end(), bronKerboschPivot(p, r, x, data));

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

