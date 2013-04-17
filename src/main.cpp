#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#include "sys.hpp"
#include "dbfile.hpp"
#include "dim.hpp"
#include "graph.hpp"
#include "parser.hpp"
#include "d1ops.hpp"
#include "d2ops.hpp"
#include "clusterer.hpp"

using namespace std;

class TBBEdgeHelper {
	public:
		double xMax;
		list<double> refs;

		TBBEdgeHelper(shared_ptr<Dim<double, double>> _d1, vector<shared_ptr<Dim<double, double>>> *_data, double _threshold) :
			xMax(numeric_limits<double>::lowest()),
			refs(),
			d1(_d1),
			data(_data),
			threshold(_threshold) {}

		TBBEdgeHelper(TBBEdgeHelper& obj, tbb::split) :
			xMax(numeric_limits<double>::lowest()),
			refs(),
			d1(obj.d1),
			data(obj.data),
			threshold(obj.threshold) {}

		void operator()(const tbb::blocked_range<size_t>& range) {
			for (size_t i = range.begin(); i != range.end(); ++i) {
				auto d2 = (*data)[i];
				double x = D2Ops<double, double>::Pearson::calc(d1, d2);
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
		shared_ptr<Dim<double, double>> d1;
		vector<shared_ptr<Dim<double, double>>> *data;
		double threshold;
};

class TBBSearchHelper {
	public:
		list<double> refs;

		TBBSearchHelper(long _id, shared_ptr<Graph> _graph) :
			refs(),
			id(_id),
			graph(_graph) {}

		TBBSearchHelper(TBBSearchHelper& obj, tbb::split) :
			refs(),
			id(obj.id),
			graph(obj.graph) {}

		void operator()(const tbb::blocked_range<size_t>& range) {
			for (size_t i = range.begin(); i != range.end(); ++i) {
				list<long> reverse = graph->get(i);
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
		long id;
		shared_ptr<Graph> graph;
};

int main(/*int argc, char **argv*/) {
	cout << "Check system: " << flush;
	cout.precision(numeric_limits<double>::digits10 + 1); // set double precision
	Sys::checkConfig();
	cout << "done" << endl;

	{
		cout << "Open DB: " << flush;
		shared_ptr<DBFile> db(new DBFile("columns.db"));
		shared_ptr<DBFile> graphStorage(new DBFile("graph.db"));
		cout << "done" << endl;

		cout << "Parse: " << flush;
		ParseResult pr = parse(db, "test.input");
		cout << "done (" << pr.dims.size() << " columns, " << pr.nRows << " rows)" << endl;

		typedef D1Ops<double, double> ops1;
		ops1::Exp::calcAndStoreVector(pr.dims);
		ops1::Var::calcAndStoreVector(pr.dims);
		ops1::StdDev::calcAndStoreVector(pr.dims);

		cout << "Build initial graph: " << flush;
		shared_ptr<Graph> graph(new Graph(graphStorage, "phase0"));
		double threshold = 0.4;
		typedef D2Ops<double, double>::Pearson op2;
		long edgeCount = 0;
		double xMax = numeric_limits<double>::lowest();
		for (long i = 0; i < static_cast<long>(pr.dims.size()); i++) {
			list<long> refs;

			// reverse search existing vertices
			TBBSearchHelper helper1(i, graph);
			parallel_reduce(tbb::blocked_range<size_t>(0, i), helper1);
			refs.insert(refs.end(), helper1.refs.begin(), helper1.refs.end()); // no splice because of incompatible allocators

			// do not add self reference (=i)

			// test edges too non exisiting vertices
			TBBEdgeHelper helper2(pr.dims[i], &pr.dims, threshold);
			parallel_reduce(tbb::blocked_range<size_t>(i + 1, pr.dims.size()), helper2);
			xMax = max(xMax, helper2.xMax);
			refs.insert(refs.end(), helper2.refs.begin(), helper2.refs.end()); // no splice because of incompatible allocators

			// store
			graph->add(refs);
			edgeCount += refs.size();

			// log
			if (i % 100 == 0) {
				cout << i << flush;
			} else if (i % 10 == 0) {
				cout << "." << flush;
			}
		}
		cout << "done (" << (edgeCount / 2) << " edges, max="<< xMax << ")" << endl;

		// graph transformation
		shared_ptr<Graph> sortedGraph(new Graph(graphStorage, "phase5"));
		sortGraph(graph, sortedGraph);

		cout << "Cleanup and Sync: " << flush;
		// end of block => free dims and db
	}
	cout << "done" << endl;
}

