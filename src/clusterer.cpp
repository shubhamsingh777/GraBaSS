#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "clusterer.hpp"

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

		// store new neighbors with new id
		output->add(neighborsNew);
	}

	// done
	cout << "done (d=" << d << ")" << endl;
}

