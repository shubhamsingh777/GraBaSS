#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cliquesearcher.hpp"

using namespace std;

void sortGraph(shared_ptr<Graph> input, shared_ptr<Graph> output) {
	assert(output->getSize() == 0);
	cout << "Sort graph: " << flush;
	typedef unordered_set<long> idSet_t;
	unordered_map<long, long> idMap;
	unordered_map<long, long> idMapRev;
	idSet_t blacklist;

	// detect maximum number of neighbors
	long maxNeighbors = 0;
	for (int i = 0; i < input->getSize(); ++i) {
		maxNeighbors = max(maxNeighbors, static_cast<long>(input->get(i).size()));
	}

	// build initial bins
	vector<idSet_t> bins(maxNeighbors + 1, idSet_t());
	for (int i = 0; i < input->getSize(); ++i) {
		bins[input->get(i).size()].insert(i);
	}
	int binMarker = 0;

	// iterate throw all bins
	long counter = 0;
	int d = -1;
	while (binMarker <= maxNeighbors) {
		// check if current bin contains elements
		if (bins[binMarker].empty()) {
			++binMarker;
			continue;
		}

		// get+remove first element from bin
		auto iter = bins[binMarker].begin();
		long element = *iter;
		bins[binMarker].erase(iter);

		// process element
		idMap[element] =  counter;
		idMapRev[counter] = element;
		++counter;
		blacklist.insert(element);
		d = max(d, binMarker);
		binMarker = max(0, binMarker - 1);

		// create new bins
		vector<idSet_t> binsNext(maxNeighbors + 1, idSet_t());
		for (auto bin : bins) {
			for (auto i : bin) {
				// count non blacklisted neighbors
				int nCount = 0;
				for (auto neighbor : input->get(i)) {
					if (blacklist.find(neighbor) != blacklist.end()) {
						++nCount;
					}
				}

				// add to right bin
				binsNext[nCount].insert(i);
			}
		}
		swap(bins, binsNext);

		// report progress
		if (counter % 1000 == 0) {
			cout << counter << flush;
		} else if (counter % 100 == 0) {
			cout << "." << flush;
		}
	}

	// write output
	for (int i = 0; i < input->getSize(); ++i) {
		// lookup old element id
		long iOld = idMapRev[i];

		// lookup new neighbor ids
		list<long> neighborsNew;
		for (auto neighbor : input->get(iOld)) {
			neighborsNew.push_back(idMap[neighbor]);
		}

		// sort neighbors
		neighborsNew.sort();

		// store new neighbors with new id
		output->add(neighborsNew);
	}

	// done
	cout << "done (maxNeighbors=" << maxNeighbors << ", d=" << d << ")" << endl;
}

list<set<long>> bronKerboschPivot(set<long> p, set<long> r, set<long> x, shared_ptr<Graph> data) {
	list<set<long>> result;
	set<long> all;
	set_union(p.begin(), p.end(), x.begin(), x.end(), inserter(all, all.end()));

	if (all.empty()) {
		result.push_back(r);
	} else {
		// choose pivot
		long winner = -1;
		long winnerValue = -1;
		list<long> winnerNeighbors;
		for (auto u : all) {
			list<long> neighbors = data->get(u);
			set<long> test;
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
		set<long> recursionElements;
		set_difference(p.begin(), p.end(), winnerNeighbors.begin(), winnerNeighbors.end(), inserter(recursionElements, recursionElements.end()));

		// recursion loop
		for (auto v : recursionElements) {
			list<long> neighbors = data->get(v);

			// build p, r, x
			set<long> pNew;
			set_intersection(p.begin(), p.end(), neighbors.begin(), neighbors.end(), inserter(pNew, pNew.end()));
			set<long> rNew(r);
			rNew.insert(v);
			set<long> xNew;
			set_intersection(x.begin(), x.end(), neighbors.begin(), neighbors.end(), inserter(xNew, xNew.end()));

			// call recursive function and store results
			result.splice(result.begin(), bronKerboschPivot(pNew, rNew, xNew, data));

			// prepare next round
			p.erase(v);
			x.insert(v);
		}
	}

	return result;
}

list<set<long>> bronKerboschDegeneracy(shared_ptr<Graph> data) {
	list<set<long>> result;

	cout << "Build cluster: " << flush;
	for (long v = 0; v < data->getSize(); ++v) {
		list<long> neighbors = data->get(v);

		// split neighbors
		auto splitPoint = find_if(neighbors.begin(), neighbors.end(), [v](long i){return i>v;});
		set<long> p(splitPoint, neighbors.end());
		set<long> x(neighbors.begin(), splitPoint);

		// prepare r
		set<long> r;
		r.insert(v);

		// shoot and merge
		result.splice(result.begin(), bronKerboschPivot(p, r, x, data));

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

