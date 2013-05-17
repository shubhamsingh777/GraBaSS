#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "graphtransformation.hpp"

namespace gc = greycore;

bool checkBidir(std::shared_ptr<gc::Graph> graph) {
	for (std::size_t v = 0; v < graph->getSize(); ++v) {
		for (auto w : graph->get(v)) {
			bool ok = false;

			for (auto z : graph->get(w)) {
				if (v == z) {
					ok = true;
					break;
				}
			}

			if (!ok) {
				return false;
			}
		}
	}

	return true;
}

inline double getConnectionRate(const std::vector<std::unordered_set<std::size_t>>& data, std::size_t v, std::size_t w) {
	std::size_t count = 0;
	auto neighbors = data.at(v);

	for (auto x : neighbors) {
		auto& tmp = data.at(x);
		if (tmp.find(w) != tmp.end()) {
			++count;
		}
	}

	return static_cast<double>(count) / static_cast<double>(neighbors.size());
}

void bidirLookup(std::shared_ptr<gc::Graph> input, std::shared_ptr<gc::Graph> output, double threshold) {
	assert(output->getSize() == 0);

	// build lookup table
	std::vector<std::unordered_set<std::size_t>> lookupTable(input->getSize());
	for (std::size_t v = 0; v < input->getSize(); ++v) {
		auto neighbors = input->get(v);
		lookupTable[v].insert(neighbors.begin(), neighbors.end());
	}

	// setup cache
	std::vector<std::unordered_set<std::size_t>> cache(input->getSize());

	// for all vertices
	for (std::size_t v = 0; v < input->getSize(); ++v) {
		auto& neighborsNew = cache[v];
		auto neighbors = input->get(v);
		neighborsNew.insert(neighbors.begin(), neighbors.end());

		// for every neighbor
		for (auto w : input->get(v)) {
			// check every 2nd generation neighbor
			for (auto x : input->get(w)) {
				// a new neighbor?
				if (neighborsNew.find(x) == neighborsNew.end()) {
					// can we use the cache?
					if (x < v) {
						auto& cachePart = cache[x];
						if (cachePart.find(v) != cachePart.end()) {
							neighborsNew.insert(x);
						}
					} else {
						// insert non equal, well connencted neighbors
						if ((v != x) && (getConnectionRate(lookupTable, v, x) >= threshold) && (getConnectionRate(lookupTable, x, v) >= threshold)) {
							neighborsNew.insert(x);
						}
					}
				}
			}
		}

		std::list<std::size_t> l(neighborsNew.begin(), neighborsNew.end());
		output->add(std::move(l));

		// report progress
		if (v % 1000 == 0) {
			std::cout << v << std::flush;
		} else if (v % 100 == 0) {
			std::cout << "." << std::flush;
		}
	}
}

void lookupNeighbors(std::shared_ptr<gc::Graph> input, std::shared_ptr<gc::Graph> output) {
	assert(output->getSize() == 0);

	for (std::size_t v = 0; v < input->getSize(); ++v) {
		std::unordered_set<std::size_t> neighborsNew;

		for (auto w : input->get(v)) {
			auto tmp = input->get(w);
			neighborsNew.insert(tmp.begin(), tmp.end());
		}

		std::list<std::size_t> l(neighborsNew.begin(), neighborsNew.end());
		output->add(std::move(l));

		// report progress
		if (v % 1000 == 0) {
			std::cout << v << std::flush;
		} else if (v % 100 == 0) {
			std::cout << "." << std::flush;
		}
	}
}

void joinEdges(std::vector<std::shared_ptr<gc::Graph>> input, std::shared_ptr<gc::Graph> output) {
	assert(output->getSize() == 0);
	assert(!input.empty());

	std::size_t size = (*input.begin())->getSize();
	for (std::size_t v = 0; v < size; ++v) {
		std::unordered_set<std::size_t> neighborsNew;

		for (auto g : input) {
			auto tmp = g->get(v);
			neighborsNew.insert(tmp.begin(), tmp.end());
		}

		// remove self reference
		neighborsNew.erase(v);

		std::list<std::size_t> l(neighborsNew.begin(), neighborsNew.end());
		output->add(move(l));

		// report progress
		if (v % 1000 == 0) {
			std::cout << v << std::flush;
		} else if (v % 100 == 0) {
			std::cout << "." << std::flush;
		}
	}
}

struct vertex_t {
	std::size_t id;
	std::unordered_set<std::size_t> neighbors;
};

std::unordered_map<std::size_t, std::size_t> sortGraph(std::shared_ptr<gc::Graph> input, std::shared_ptr<gc::Graph> output) {
	assert(output->getSize() == 0);
	std::cout << "Sort graph: " << std::flush;
	std::unordered_map<std::size_t, std::size_t> idMap;
	std::unordered_map<std::size_t, std::size_t> idMapRev;

	// detect maximum number of neighbors
	long maxNeighbors = 0;
	for (std::size_t i = 0; i < input->getSize(); ++i) {
		maxNeighbors = std::max(maxNeighbors, static_cast<long>(input->get(i).size()));
	}

	// build initial bins
	std::vector<std::vector<vertex_t*>> bins(maxNeighbors + 1);
	for (std::size_t i = 0; i < input->getSize(); ++i) {
		std::list<std::size_t> tmp = input->get(i);

		bins[tmp.size()].push_back(new vertex_t({
				i,
				std::unordered_set<std::size_t>(tmp.begin(), tmp.end())})
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
		std::size_t element = (*iter)->id;
		delete (*iter);
		bins[binMarker].erase(iter);

		// process element
		idMap[element] =  counter;
		idMapRev[counter] = element;
		++counter;
		d = std::max(d, binMarker);
		binMarker = std::max(0, binMarker - 1);

		// create new bins
		std::vector<std::vector<vertex_t*>> binsNext(maxNeighbors + 1);
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
			std::cout << counter << std::flush;
		} else if (counter % 100 == 0) {
			std::cout << "." << std::flush;
		}
	}

	// write output
	for (std::size_t i = 0; i < input->getSize(); ++i) {
		// lookup old element id
		std::size_t iOld = idMapRev[i];

		// lookup new neighbor ids
		std::list<std::size_t> neighborsNew;
		for (auto neighbor : input->get(iOld)) {
			neighborsNew.push_back(idMap[neighbor]);
		}

		// sort neighbors
		neighborsNew.sort();

		// store new neighbors with new id
		output->add(neighborsNew);
	}

	// done
	std::cout << "done (maxNeighbors=" << maxNeighbors << ", d=" << d << ")" << std::endl;

	return idMapRev;
}

