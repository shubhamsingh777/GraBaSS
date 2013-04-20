#include <iostream>
#include <limits>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#include "d2ops.hpp"
#include "graphbuilder.hpp"

using namespace std;

class TBBEdgeHelper {
	public:
		data_t xMax;
		list<bigid_t> refs;

		TBBEdgeHelper(datadim_t _d1, vector<datadim_t> *_data, data_t _threshold) :
			xMax(numeric_limits<data_t>::lowest()),
			refs(),
			d1(_d1),
			data(_data),
			threshold(_threshold) {}

		TBBEdgeHelper(TBBEdgeHelper& obj, tbb::split) :
			xMax(numeric_limits<data_t>::lowest()),
			refs(),
			d1(obj.d1),
			data(obj.data),
			threshold(obj.threshold) {}

		void operator()(const tbb::blocked_range<bigid_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				auto d2 = (*data)[i];
				data_t x = D2Ops<data_t, data_t>::Pearson::calc(d1, d2);
				xMax = max(xMax, x);
				if (x >= threshold) {
					refs.push_back(i);
				}
			}
		}

		void join(TBBEdgeHelper& obj) {
			this->xMax = max(this->xMax, obj.xMax);
			this->refs.splice(this->refs.end(), obj.refs);
		}

	private:
		datadim_t d1;
		vector<datadim_t> *data;
		data_t threshold;
};

class TBBSearchHelper {
	public:
		list<bigid_t> refs;

		TBBSearchHelper(bigid_t _id, graph_t _graph) :
			refs(),
			id(_id),
			graph(_graph) {}

		TBBSearchHelper(TBBSearchHelper& obj, tbb::split) :
			refs(),
			id(obj.id),
			graph(obj.graph) {}

		void operator()(const tbb::blocked_range<bigid_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				list<bigid_t> reverse = graph->get(i);
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
		bigid_t id;
		graph_t graph;
};

void buildGraph(vector<datadim_t> data, graph_t graph, data_t threshold) {
		cout << "Build initial graph: " << flush;

		typedef D2Ops<data_t, data_t>::Pearson op2;
		long edgeCount = 0;
		data_t xMax = numeric_limits<data_t>::lowest();

		for (bigid_t i = 0; i < static_cast<bigid_t>(data.size()); i++) {
			list<bigid_t> refs;

			// reverse search existing vertices
			TBBSearchHelper helper1(i, graph);
			parallel_reduce(tbb::blocked_range<bigid_t>(0, i), helper1);
			refs.splice(refs.end(), helper1.refs);

			// do not add self reference (=i)

			// test edges too non exisiting vertices
			TBBEdgeHelper helper2(data[i], &data, threshold);
			parallel_reduce(tbb::blocked_range<bigid_t>(i + 1, data.size()), helper2);
			xMax = max(xMax, helper2.xMax);
			refs.splice(refs.end(), helper2.refs);

			// store
			graph->add(refs);
			edgeCount += refs.size();

			// report progress
			if (i % 100 == 0) {
				cout << i << flush;
			} else if (i % 10 == 0) {
				cout << "." << flush;
			}
		}

		cout << "done (" << (edgeCount / 2) << " edges, max="<< xMax << ")" << endl;
}

