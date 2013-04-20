#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>

#include <boost/program_options.hpp>

#include "sys.hpp"
#include "dbfile.hpp"
#include "dim.hpp"
#include "graph.hpp"
#include "graphbuilder.hpp"
#include "parser.hpp"
#include "d1ops.hpp"
#include "cliquesearcher.hpp"
#include "tracer.hpp"

namespace po = boost::program_options;

using namespace std;

int main(int argc, char **argv) {
	// global config vars
	string cfgInput;
	string cfgOutput;
	data_t cfgThreshold;

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
			"threshold",
			po::value<data_t>(&cfgThreshold)->default_value(0.4, "0.4"),
			"Threshold for graph edge generation"
		)
		(
			"help",
			"Print this help message"
		)
	;
	po::variables_map poVm;
	po::store(po::parse_command_line(argc, argv, poDesc), poVm);
	po::notify(poVm);
	if (poVm.count("help")) {
		cout << "hugeDim"<< endl << endl << poDesc << endl;
		return 0;
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
		shared_ptr<DBFile> db(new DBFile("columns.db"));
		shared_ptr<DBFile> graphStorage(new DBFile("graph.db"));
		ofstream outfile;
		outfile.open(cfgOutput);
		cout << "done" << endl;

		cout << "Parse: " << flush;
		tPhase.reset(new Tracer("parse", tMain));
		ParseResult pr = parse(db, cfgInput);
		cout << "done (" << pr.dims.size() << " columns, " << pr.nRows << " rows)" << endl;

		typedef D1Ops<data_t, data_t> ops1;
		{
			tPhase.reset(new Tracer("precalc", tMain));

			auto tPrecalc = make_shared<Tracer>("exp", tPhase);
			ops1::Exp::calcAndStoreVector(pr.dims);

			tPrecalc.reset(new Tracer("var", tPhase));
			ops1::Var::calcAndStoreVector(pr.dims);

			tPrecalc.reset(new Tracer("stdDev", tPhase));
			ops1::StdDev::calcAndStoreVector(pr.dims);
		}

		// build graph from data
		tPhase.reset(new Tracer("buildGraph", tMain));
		auto graph = make_shared<Graph>(graphStorage, "phase0");
		buildGraph(pr.dims, graph, cfgThreshold);

		// graph transformation
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
}

