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
#include "entropy.hpp"
#include "graphbuilder.hpp"
#include "d1ops.hpp"
#include "cliquesearcher.hpp"
#include "tracer.hpp"
#include "graphtransformation.hpp"
#include "dimtransformation.hpp"

#include "greycore/database.hpp"
#include "greycore/dim.hpp"
#include "greycore/wrapper/graph.hpp"

namespace gc = greycore;
namespace po = boost::program_options;

int main(int argc, char **argv) {
	// global config vars
	std::string cfgOutput;
	std::string cfgDbData;
	std::string cfgDbMetadata;
	std::string cfgDbGraph;
	data_t cfgThresholdGen;
	data_t cfgThresholdGraph;
	bool cfgForce;
	std::size_t cfgGraphDist;
	std::size_t cfgThreads;
	data_t cfgPostFilter;

	// parse program options
	po::options_description poDesc("Options");
	poDesc.add_options()
		(
			"output",
			po::value(&cfgOutput)->default_value("subspaces.txt"),
			"Output file"
		)
		(
			"dbdata",
			po::value(&cfgDbData)->default_value("columns.db"),
			"DB file that stores parsed data"
		)
		(
			"dbmetadata",
			po::value(&cfgDbMetadata)->default_value("metadata.db"),
			"DB file that stores calculated metadata for dimensions"
		)
		(
			"dbgraph",
			po::value(&cfgDbGraph)->default_value("graph.db"),
			"DB file that stores generated and transformed graphs"
		)
		(
			"thresholdGen",
			po::value(&cfgThresholdGen)->default_value(0.4, "0.4"),
			"Threshold for graph edge generation"
		)
				(
			"thresholdGraph",
			po::value(&cfgThresholdGraph)->default_value(0.0, "0.0"),
			"Threshold for distance graph generation"
		)
		(
			"graphDist",
			po::value(&cfgGraphDist)->default_value(1),
			"Maximal graph distance of clique members (ignored if < 2)"
		)
		(
			"postFilter",
			po::value(&cfgPostFilter)->default_value(0.0, "0.0"),
			"Entropy threshold for subspaces"
		)
		(
			"threads",
			po::value(&cfgThreads)->default_value(0),
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
		std::cout << "Error:" << std::endl
			<< e.what() << std::endl
			<< std::endl
			<< "Use --help to get help ;)" << std::endl;
		return EXIT_FAILURE;
	}
	po::notify(poVm);
	if (poVm.count("help")) {
		std::cout << "hugeDim"<< std::endl << std::endl << poDesc << std::endl;
		return EXIT_SUCCESS;
	}
	cfgForce = poVm.count("force");

	// setup tbb
	int threads = static_cast<int>(cfgThreads);
	if (threads == 0) {
		threads = -1; // = tbb::task_scheduler_init::automatic
	}
	tbb::task_scheduler_init init(threads);


	// start time tracing
	std::stringstream timerProfile;

	std::cout << "Check system: " << std::flush;
	std::cout.precision(std::numeric_limits<double>::digits10 + 1); // set double precision
	checkConfig();
	std::cout << "done" << std::endl;

	{
		auto tMain = std::make_shared<Tracer>("main", &timerProfile);
		std::shared_ptr<Tracer> tPhase;

		std::cout << "Open DB and output file: " << std::flush;
		tPhase.reset(new Tracer("open", tMain));
		auto dbData = std::make_shared<gc::Database>(cfgDbData);
		auto dbMetadata = std::make_shared<gc::Database>(cfgDbMetadata);
		auto dbGraph = std::make_shared<gc::Database>(cfgDbGraph);
		std::ofstream outfile(cfgOutput);

		auto dimNameList = dbData->getIndexDims();
		std::vector<datadim_t> dims;
		assert(dimNameList->getSize() > 0);
		for (size_t i = 0; i < dimNameList->getSize(); ++i) {
			auto name(std::get<0>((*dimNameList)[i].toTuple()));
			dims.push_back(dbData->getDim<data_t>(name));
		}
		std::cout << "done (" << dims.size() << " columns, " << dims[0]->getSize() << " rows)" << std::endl;

		// discretize dims and build pairs
		tPhase.reset(new Tracer("discretize", tMain));
		std::size_t discretizeCounter = 0;
		std::cout << "Discretize: " << std::flush;
		std::vector<std::pair<discretedim_t, discretedim_t>> discreteDims;
		std::vector<std::pair<datadim_t, mdMap_t>> dimsWithMd;
		for (auto d : dims) {
			// build pair
			std::shared_ptr<mdMapObj_t::dim_t> dimPtr;
			try {
				dimPtr = dbMetadata->createDim<mdMapObj_t::dim_t::payload_t, mdMapObj_t::dim_t::segmentSize>(d->getName() + ".metadata");
			} catch (const std::runtime_error& e) {
				dimPtr = dbMetadata->getDim<mdMapObj_t::dim_t::payload_t, mdMapObj_t::dim_t::segmentSize>(d->getName() + ".metadata");
			}
			auto map = std::make_shared<mdMapObj_t>(dimPtr);
			dimsWithMd.push_back(std::make_pair(d, map));

			// discretize
			discretedim_t discrete;
			discretedim_t bins;

			try {
				discrete = dbMetadata->createDim<std::size_t>(d->getName() + ".discrete");
				bins = dbMetadata->createDim<std::size_t>(d->getName() + ".bins");
				discretizeDim(d, discrete, bins);

				// report progress
				if (discretizeCounter % 1000 == 0) {
					std::cout << discretizeCounter << std::flush;
				} else if (discretizeCounter % 100 == 0) {
					std::cout << "." << std::flush;
				}
				++discretizeCounter;
			} catch (const std::runtime_error& e) {
				discrete = dbMetadata->getDim<std::size_t>(d->getName() + ".discrete");
				bins = dbMetadata->getDim<std::size_t>(d->getName() + ".bins");
			}

			discreteDims.push_back(make_pair(discrete, bins));
		}
		if (discretizeCounter == 0) {
			std::cout << "skipped" << std::endl;
		} else {
			std::cout << "done" << std::endl;
		}

		{
			tPhase.reset(new Tracer("precalc", tMain));

			auto tPrecalc = std::make_shared<Tracer>("exp", tPhase);
			D1Ops::Exp::calcAndStoreVector(dimsWithMd);

			tPrecalc.reset(new Tracer("var", tPhase));
			D1Ops::Var::calcAndStoreVector(dimsWithMd);

			tPrecalc.reset(new Tracer("stdDev", tPhase));
			D1Ops::StdDev::calcAndStoreVector(dimsWithMd);
		}

		// build graph from data
		tPhase.reset(new Tracer("buildGraph", tMain));
		auto graph = std::make_shared<gc::Graph>(dbGraph->createDim<std::size_t>("phase0.1"), dbGraph->createDim<std::size_t>("phase0.2"));
		buildGraph(dimsWithMd, discreteDims, graph, cfgThresholdGen);

		// cleanup
		std::cout << "Cleanup: " << std::flush;
		tPhase.reset(new Tracer("cleanup1", tMain));
		for (auto& d : dims) {
			d.reset();
		}
		for (auto& p : discreteDims) {
			// p.first.reset(); NO! required for post filter
			p.second.reset();
		}
		for (auto& p : dimsWithMd) {
			p.first.reset();
			p.second.reset();
		}
		dbData.reset();
		dbMetadata.reset();
		std::cout << "done" << std::endl;

		// calc distance graph
		if (cfgGraphDist > 1) {
			tPhase.reset(new Tracer("calcDistGraph", tMain));
			std::vector<std::shared_ptr<gc::Graph>> distGraphSteps({graph});
			auto last = graph;

			for (std::size_t i = 2; i <= cfgGraphDist; ++i) {
				std::cout << "Calc graph distance " << i << ": " << std::flush;
				std::stringstream ss;
				ss << "dist" << i;
				auto next = std::make_shared<gc::Graph>(dbGraph->createDim<std::size_t>(ss.str() + ".1"), dbGraph->createDim<std::size_t>(ss.str() + ".2"));

				if (cfgThresholdGraph == 0.0) {
					lookupNeighbors(last, next);
				} else {
					bidirLookup(last, next, cfgThresholdGraph);
				}

				distGraphSteps.push_back(next);
				last = next;
				std::cout << "done" << std::endl;
			}

			if (cfgThresholdGraph == 0.0) {
				std::cout << "Join graphs: " << std::flush;
				auto distGraph = std::make_shared<gc::Graph>(dbGraph->createDim<std::size_t>("distGraph.1"), dbGraph->createDim<std::size_t>("distGraph.2"));
				joinEdges(distGraphSteps, distGraph);
				graph = distGraph;
				std::cout << "done" << std::endl;
			} else {
				graph = distGraphSteps.back();
			}
		}

		// sort graph
		tPhase.reset(new Tracer("sortGraph", tMain));
		auto sortedGraph = std::make_shared<gc::Graph>(dbGraph->createDim<std::size_t>("sorted.1"), dbGraph->createDim<std::size_t>("sorted.2"));
		auto idMap = sortGraph(graph, sortedGraph);

		// search cliques
		tPhase.reset(new Tracer("cliqueSearcher", tMain));
		auto subspaces = bronKerboschDegeneracy(sortedGraph);

		// lookup final subspace results from idMap
		std::list<subspace_t> result;
		std::transform(subspaces.begin(), subspaces.end(), std::back_inserter(result), [&idMap](const std::vector<std::size_t>& ss){
					std::list<std::size_t> tmp;
					std::transform(ss.begin(), ss.end(), std::back_inserter(tmp), [&idMap](std::size_t d) {
							return idMap[d];
						});
					tmp.sort();
					return tmp;
				});
		result.sort([](const subspace_t& a, const subspace_t& b){
					if (a.size() != b.size()) {
						return a.size() < b.size();
					} else {
						auto iter1 = a.cbegin();
						auto iter2 = b.cbegin();
						while ((iter1 != a.cend()) && (iter2 != b.cend())) {
							if (*iter1 != *iter2) {
								return *iter1 < *iter2;
							}

							++iter1;
							++iter2;
						}
						return true;
					}
				});

		// post filter
		tPhase.reset(new Tracer("postFilter", tMain));
		std::cout << "Filter subspaces: " << std::flush;
		if (cfgPostFilter > 0) {
			decltype(result) tmp;
			std::swap(result, tmp);
			data_t entropyMin = std::numeric_limits<data_t>::infinity();
			data_t entropyMax = 0;
			std::size_t nDrops = 0;

			std::vector<discretedim_t> dimVector;
			std::transform(discreteDims.begin(), discreteDims.end(), std::back_inserter(dimVector), [](std::pair<discretedim_t, discretedim_t>& p) {
					return p.first;
					});

			for (const auto& ss : tmp) {
				data_t entropy = calcEntropy(ss, dimVector);
				entropyMin = std::min(entropyMin, entropy);
				entropyMax = std::max(entropyMax, entropy);

				if (entropy > cfgPostFilter) {
					result.push_back(ss);
				} else {
					++nDrops;
				}

				// progress
				std::cout << "." << std::flush;
			}

			std::cout << "done (entropMin=" << entropyMin << ", entropyMax=" << entropyMax << ", dropped=" << nDrops << ")" << std::endl;
		} else {
			std::cout << "skipped" << std::endl;
		}

		// ok, so lets write the results
		tPhase.reset(new Tracer("output", tMain));
		std::cout << "Write result: " << std::flush;
		for (auto ss : result) {
			bool first = true;
			for (auto dim : ss) {
				if (first) {
					first = false;
				} else {
					outfile << ",";
				}
				outfile << dim;
			}
			outfile << std::endl;
		}
		std::cout << "done" << std::endl;

		std::cout << "Cleanup and Sync: " << std::flush;
		// end of block => free dims and db
	}
	std::cout << "done" << std::endl;

	std::cout << std::endl << "Time profile:" << std::endl << timerProfile.str() << std::endl;

	return EXIT_SUCCESS;
}

