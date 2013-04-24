#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>

#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

#include "sys.hpp"
#include "dbfile.hpp"
#include "dim.hpp"
#include "graph.hpp"
#include "graphbuilder.hpp"
#include "parser.hpp"
#include "d1ops.hpp"
#include "cliquesearcher.hpp"
#include "tracer.hpp"
#include "graphtransformation.hpp"

namespace po = boost::program_options;

using namespace std;

int main(int argc, char **argv) {
	// global config vars
	string cfgInput;
	string cfgOutput;
	string cfgDbData;
	string cfgDbGraph;
	data_t cfgThreshold;
	bool cfgForce;
	unsigned int cfgGraphDist;
	unsigned int cfgThreads;

	// parse program options
	po::options_description poDesc("Options");
	poDesc.add_options()
		(
			"input",
			po::value<string>(&cfgInput)->default_value("test.input"),
			"Input file"
		)
		(
			"output",
			po::value<string>(&cfgOutput)->default_value("subspaces.txt"),
			"Output file"
		)
		(
			"dbdata",
			po::value<string>(&cfgDbData)->default_value("columns.db"),
			"DB file that stores parsed data"
		)
		(
			"dbgraph",
			po::value<string>(&cfgDbGraph)->default_value("graph.db"),
			"DB file that stores generated and transformed graphs"
		)
		(
			"threshold",
			po::value<data_t>(&cfgThreshold)->default_value(0.4, "0.4"),
			"Threshold for graph edge generation"
		)
		(
			"graphDist",
			po::value<unsigned int>(&cfgGraphDist)->default_value(1),
			"Maximal graph distance of clique members (ignored if < 2)"
		)
		(
			"threads",
			po::value<unsigned int>(&cfgThreads)->default_value(0),
			"Number of threads (0 = auto)"
		)
		(
			"force",
			"Force to parse and progress data, ignores cache"
		)
		(
			"help",
			"Print this help message"
		)
	;
	po::variables_map poVm;
	try {
		po::store(po::parse_command_line(argc, argv, poDesc), poVm);
	} catch (const std::exception& e) {
		cout << "Error:" << endl
			<< e.what() << endl
			<< endl
			<< "Use --help to get help ;)" << endl;
		return EXIT_FAILURE;
	}
	po::notify(poVm);
	if (poVm.count("help")) {
		cout << "hugeDim"<< endl << endl << poDesc << endl;
		return EXIT_SUCCESS;
	}
	cfgForce = poVm.count("force");

	// setup tbb
	if (cfgThreads == 0) {
		tbb::task_scheduler_init(-1 /*=tbb::task_scheduler_init::automatic*/);
	} else {
		tbb::task_scheduler_init(cfgThreads);
	}

	// start time tracing
	stringstream timerProfile;

	cout << "Check system: " << flush;
	cout.precision(numeric_limits<double>::digits10 + 1); // set double precision
	checkConfig();
	cout << "done" << endl;

	{
		auto tMain = make_shared<Tracer>("main", &timerProfile);
		shared_ptr<Tracer> tPhase;

		cout << "Open DB and output file: " << flush;
		tPhase.reset(new Tracer("open", tMain));
		auto db = make_shared<DBFile>(cfgDbData);
		auto graphStorage = make_shared<DBFile>(cfgDbGraph);
		ofstream outfile(cfgOutput);
		cout << "done" << endl;

		cout << "Parse: " << flush;
		auto dimNameList = datadimobj_t::getList(db);
		vector<datadim_t> dims;
		if (!cfgForce && (dimNameList->size() > 0)) {
			for (auto name : *dimNameList) {
				string stdname(name.c_str());
				dims.push_back(make_shared<datadimobj_t>(db, stdname));
			}
			cout << "skipped" << endl;
		} else {
			tPhase.reset(new Tracer("parse", tMain));
			ParseResult pr = parse(db, cfgInput);
			dims = pr.dims;
			cout << "done (" << pr.dims.size() << " columns, " << pr.nRows << " rows)" << endl;
		}

		typedef D1Ops<data_t, data_t> ops1;
		{
			tPhase.reset(new Tracer("precalc", tMain));

			auto tPrecalc = make_shared<Tracer>("exp", tPhase);
			ops1::Exp::calcAndStoreVector(dims);

			tPrecalc.reset(new Tracer("var", tPhase));
			ops1::Var::calcAndStoreVector(dims);

			tPrecalc.reset(new Tracer("stdDev", tPhase));
			ops1::StdDev::calcAndStoreVector(dims);
		}

		// build graph from data
		tPhase.reset(new Tracer("buildGraph", tMain));
		auto graph = make_shared<Graph>(graphStorage, "phase0");
		buildGraph(dims, graph, cfgThreshold);

		// cleanup
		cout << "Cleanup: " << flush;
		tPhase.reset(new Tracer("cleanup1", tMain));
		for (auto& d : dims) {
			d.reset();
		}
		db.reset();
		cout << "done" << endl;

		// calc distance graph
		if (cfgGraphDist > 1) {
			tPhase.reset(new Tracer("calcDistGraph", tMain));
			vector<graph_t> toJoin({graph});
			auto last = graph;

			for (unsigned int i = 2; i <= cfgGraphDist; ++i) {
				cout << "Calc graph distance " << i << ": " << flush;
				stringstream ss;
				ss << "dist" << i;
				auto next = make_shared<Graph>(graphStorage, ss.str());

				lookupNeighbors(last, next);

				toJoin.push_back(next);
				last = next;
				cout << "done" << endl;
			}

			cout << "Join graphs: " << flush;
			auto distGraph = make_shared<Graph>(graphStorage, "distGraph");
			joinEdges(toJoin, distGraph);
			graph = distGraph;
			cout << "done" << endl;
		}

		// sort graph
		tPhase.reset(new Tracer("sortGraph", tMain));
		auto sortedGraph = make_shared<Graph>(graphStorage, "phase5");
		auto idMap = sortGraph(graph, sortedGraph);

		// search cliques
		tPhase.reset(new Tracer("cliqueSearcher", tMain));
		auto subspaces = bronKerboschDegeneracy(sortedGraph);

		// ok, so lets write the results
		tPhase.reset(new Tracer("output", tMain));
		cout << "Write result: " << flush;
		for (auto ss : subspaces) {
			bool first = true;
			for (auto dim : ss) {
				if (first) {
					first = false;
				} else {
					outfile << ",";
				}
				outfile << idMap[dim];
			}
			outfile << endl;
		}
		cout << "done" << endl;

		cout << "Cleanup and Sync: " << flush;
		// end of block => free dims and db
	}
	cout << "done" << endl;

	cout << endl << "Time profile:" << endl << timerProfile.str() << endl;

	return EXIT_SUCCESS;
}

