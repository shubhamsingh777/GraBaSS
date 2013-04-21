#include <unordered_set>
#include <utility>

#include "graphtransformation.hpp"

using namespace std;

void lookupNeighbors(graph_t input, graph_t output) {
	assert(output->getSize() == 0);

	for (bigid_t v = 0; v < input->getSize(); ++v) {
		unordered_set<bigid_t> neighborsNew;

		for (auto w : input->get(v)) {
			auto tmp = input->get(w);
			neighborsNew.insert(tmp.begin(), tmp.end());
		}

		list<bigid_t> l(neighborsNew.begin(), neighborsNew.end());
		output->add(move(l));

		// report progress
		if (v % 1000 == 0) {
			cout << v << flush;
		} else if (v % 100 == 0) {
			cout << "." << flush;
		}
	}
}

void joinEdges(vector<graph_t> input, graph_t output) {
	assert(output->getSize() == 0);
	assert(!input.empty());

	bigid_t size = (*input.begin())->getSize();
	for (bigid_t v = 0; v < size; ++v) {
		unordered_set<bigid_t> neighborsNew;

		for (auto g : input) {
			auto tmp = g->get(v);
			neighborsNew.insert(tmp.begin(), tmp.end());
		}

		// remove self reference
		neighborsNew.erase(v);

		list<bigid_t>l (neighborsNew.begin(), neighborsNew.end());
		output->add(move(l));

		// report progress
		if (v % 1000 == 0) {
			cout << v << flush;
		} else if (v % 100 == 0) {
			cout << "." << flush;
		}
	}
}

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

