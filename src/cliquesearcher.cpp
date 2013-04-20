#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "cliquesearcher.hpp"

using namespace std;

struct vertex_t {
	bigid_t id;
	unordered_set<bigid_t> neighbors;
};

unordered_map<bigid_t, bigid_t> sortGraph(graph_t input, graph_t output) {
	assert(output->getSize() == 0);
	cout << "Sort graph: " << flush;
	unordered_map<bigid_t, bigid_t> idMap;
	unordered_map<bigid_t, bigid_t> idMapRev;

	// detect maximum number of neighbors
	long maxNeighbors = 0;
	for (bigid_t i = 0; i < input->getSize(); ++i) {
		maxNeighbors = max(maxNeighbors, static_cast<long>(input->get(i).size()));
	}

	// build initial bins
	vector<vector<vertex_t*>> bins(maxNeighbors + 1);
	for (bigid_t i = 0; i < input->getSize(); ++i) {
		list<bigid_t> tmp = input->get(i);

		bins[tmp.size()].push_back(new vertex_t({
				i,
				unordered_set<bigid_t>(tmp.begin(), tmp.end())})
			);
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
		auto iter = --bins[binMarker].end();
		bigid_t element = (*iter)->id;
		delete (*iter);
		bins[binMarker].erase(iter);

		// process element
		idMap[element] =  counter;
		idMapRev[counter] = element;
		++counter;
		d = max(d, binMarker);
		binMarker = max(0, binMarker - 1);

		// create new bins
		vector<vector<vertex_t*>> binsNext(maxNeighbors + 1);
		for (auto bin : bins) {
			for (auto v : bin) {
				// delete edges to current element
				v->neighbors.erase(element);

				// add to right bin
				binsNext[v->neighbors.size()].push_back(v);
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
		bigid_t iOld = idMapRev[i];

		// lookup new neighbor ids
		list<bigid_t> neighborsNew;
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

	return idMapRev;
}

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
			result.splice(result.begin(), bronKerboschPivot(pNew, rNew, xNew, data));

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

