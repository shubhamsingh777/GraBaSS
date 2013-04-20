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

using namespace std;

int main(int argc, char **argv) {
	// global config vars
	string cfgInput;
	string cfgOutput;
	double cfgThreshold;

	// parse program options
	boost::program_options::options_description poDesc("Options");
	poDesc.add_options()
		(
			"input",
			boost::program_options::value<string>(&cfgInput)->default_value("test.input"),
			"Input file"
		)
		(
			"output",
			boost::program_options::value<string>(&cfgOutput)->default_value("subspaces.txt"),
			"Output file"
		)
		(
			"threshold",
			boost::program_options::value<double>(&cfgThreshold)->default_value(0.4, "0.4"),
			"Threshold for graph edge generation"
		)
		(
			"help",
			"Print this help message"
		)
	;
	boost::program_options::variables_map poVm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, poDesc), poVm);
	boost::program_options::notify(poVm);
	if (poVm.count("help")) {
		cout << "hugeDim"<< endl << endl << poDesc << endl;
		return 0;
	}

	// start time tracing
	stringstream timerProfile;

	cout << "Check system: " << flush;
	cout.precision(numeric_limits<double>::digits10 + 1); // set double precision
	Sys::checkConfig();
	cout << "done" << endl;

	{
		auto tMain = shared_ptr<Tracer>(new Tracer("main", &timerProfile));
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

		typedef D1Ops<double, double> ops1;
		{
			tPhase.reset(new Tracer("precalc", tMain));

			shared_ptr<Tracer> tPrecalc(new Tracer("exp", tPhase));
			ops1::Exp::calcAndStoreVector(pr.dims);

			tPrecalc.reset(new Tracer("var", tPhase));
			ops1::Var::calcAndStoreVector(pr.dims);

			tPrecalc.reset(new Tracer("stdDev", tPhase));
			ops1::StdDev::calcAndStoreVector(pr.dims);
		}

		// build graph from data
		tPhase.reset(new Tracer("buildGraph", tMain));
		shared_ptr<Graph> graph(new Graph(graphStorage, "phase0"));
		buildGraph(pr.dims, graph, cfgThreshold);

		// graph transformation
		tPhase.reset(new Tracer("sortGraph", tMain));
		shared_ptr<Graph> sortedGraph(new Graph(graphStorage, "phase5"));
		auto idMap = sortGraph(graph, sortedGraph);

		// search cliques
		tPhase.reset(new Tracer("cliqueSearcher", tMain));
		list<set<long>> subspaces = bronKerboschDegeneracy(sortedGraph);

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

