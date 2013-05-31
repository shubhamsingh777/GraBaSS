#include <iostream>
#include <limits>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#include "graphbuilder.hpp"
#include "dimsimilarity.hpp"

namespace gc = greycore;

class TBBEdgeHelper {
	public:
		data_t xMax;
		std::list<std::size_t> refs;

		TBBEdgeHelper(discretedim_t _d1, discretedim_t _bins1, std::vector<std::pair<discretedim_t, discretedim_t>> *_data, data_t _threshold) :
			xMax(std::numeric_limits<data_t>::lowest()),
			refs(),
			d1(_d1),
			bins1(_bins1),
			data(_data),
			threshold(_threshold) {}

		TBBEdgeHelper(TBBEdgeHelper& obj, tbb::split) :
			xMax(std::numeric_limits<data_t>::lowest()),
			refs(),
			d1(obj.d1),
			bins1(obj.bins1),
			data(obj.data),
			threshold(obj.threshold) {}

		void operator()(const tbb::blocked_range<std::size_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				auto d2 = (*data)[i].first;
				auto bins2 = (*data)[i].second;
				data_t x = dimsimilarity(d1, bins1, d2, bins2);
				xMax = std::max(xMax, x);
				if (x >= threshold) {
					refs.push_back(i);
				}
			}
		}

		void join(TBBEdgeHelper& obj) {
			this->xMax = std::max(this->xMax, obj.xMax);
			this->refs.splice(this->refs.end(), obj.refs);
		}

	private:
		discretedim_t d1;
		discretedim_t bins1;
		std::vector<std::pair<discretedim_t, discretedim_t>> *data;
		data_t threshold;
};

class TBBSearchHelper {
	public:
		std::list<std::size_t> refs;

		TBBSearchHelper(std::size_t _id, std::shared_ptr<gc::Graph> _graph) :
			refs(),
			id(_id),
			graph(_graph) {}

		TBBSearchHelper(TBBSearchHelper& obj, tbb::split) :
			refs(),
			id(obj.id),
			graph(obj.graph) {}

		void operator()(const tbb::blocked_range<std::size_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				std::list<std::size_t> reverse = graph->get(i);
				for (auto r : reverse) {
					if (r == id) {
						refs.push_back(i);
					}
				}
			}
		}

		void join(TBBSearchHelper& obj) {
			this->refs.splice(this->refs.end(), obj.refs);
		}

	private:
		std::size_t id;
		std::shared_ptr<gc::Graph> graph;
};

void buildGraph(std::vector<std::pair<discretedim_t, discretedim_t>> data, std::shared_ptr<gc::Graph> graph, data_t threshold) {
	std::cout << "Build initial graph: " << std::flush;

	long edgeCount = 0;
	data_t xMax = std::numeric_limits<data_t>::lowest();

	for (std::size_t i = 0; i < data.size(); i++) {
		std::list<std::size_t> refs;

		// reverse search existing vertices
		TBBSearchHelper helper1(i, graph);
		parallel_reduce(tbb::blocked_range<std::size_t>(0, i), helper1);
		refs.splice(refs.end(), helper1.refs);

		// do not add self reference (=i)

		// test edges too non exisiting vertices
		TBBEdgeHelper helper2(data[i].first, data[i].second, &data, threshold);
		parallel_reduce(tbb::blocked_range<std::size_t>(i + 1, data.size()), helper2);
		xMax = std::max(xMax, helper2.xMax);
		refs.splice(refs.end(), helper2.refs);

		// store
		graph->add(refs);
		edgeCount += refs.size();

		// report progress
		if (i % 100 == 0) {
			std::cout << i << std::flush;
		} else if (i % 10 == 0) {
			std::cout << "." << std::flush;
		}
	}

	std::cout << "done (" << (edgeCount / 2) << " edges, max="<< xMax << ")" << std::endl;
}

