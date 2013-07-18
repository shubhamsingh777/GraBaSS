#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

#include "sys.hpp"
#include "tracer.hpp"

#include "disretize.hpp"
#include "hash.hpp"

#include "greycore/database.hpp"
#include "greycore/dim.hpp"
#include "greycore/wrapper/graph.hpp"

namespace gc = greycore;
namespace po = boost::program_options;

typedef std::unordered_map<std::vector<std::size_t>, data_t> density_t;
typedef std::set<std::size_t> subspace_t;
typedef std::unordered_set<subspace_t> subspaces_t;
typedef std::vector<data_t> entropyCache_t;

density_t calcDensity(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density;
	std::size_t nSegments = data.at(0)->getSegmentCount();
	std::size_t n = data.at(0)->getSize();
	data_t step = 1.0 / static_cast<data_t>(n);

	for (std::size_t segment = 0; segment < nSegments; ++segment) {
		std::size_t size = data.at(0)->getSegmentFillSize(segment);
		typename std::vector<discretedimObj_t::segment_t*> segmentPtrs;

		for (std::size_t s : subspace) {
			const discretedim_t& dim = data.at(s);
			segmentPtrs.push_back(dim->getSegment(segment));
		}

		for (std::size_t i = 0; i < size; ++i) {
			std::vector<size_t> pos;

			for (const auto& segmentPtr : segmentPtrs) {
				pos.push_back((*segmentPtr)[i]);
			}

			density[pos] += step;
		}
	}

	return density;
}

data_t calcEntropy(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density = calcDensity(subspace, data);
	data_t result = 0.0;

	for (auto kv : density) {
		data_t p = kv.second;
		result -= p * log2(p);
	}

	return result;
}

data_t calcInterest(const subspace_t subspace, data_t entropy, const entropyCache_t& entropyCache) {
	data_t aggr = 0.0;

	for (size_t d : subspace) {
		aggr += entropyCache[d];
	}

	return aggr - entropy;
}

bool prune(const subspace_t& subspace, const subspaces_t& last) {
	for (std::size_t d : subspace) {
		subspace_t test(subspace);
		test.erase(d);

		if (last.find(test) == last.cend()) {
			return true;
		}
	}

	return false;
}

subspaces_t genCandidates(const subspaces_t& last) {
	subspaces_t result;

	for (const auto& ss1 : last) {
		bool ignore = true;

		for (const auto& ss2 : last) {
			if (ignore) {
				if (ss1 == ss2) {
					ignore = false;
				}
			} else {
				subspace_t cpy1(ss1);
				subspace_t cpy2(ss2);
				auto iter1 = --cpy1.end();
				auto iter2 = --cpy2.end();
				std::size_t end1 = *iter1;
				std::size_t end2 = *iter2;

				cpy1.erase(iter1);
				cpy2.erase(iter2);

				if (cpy1 == cpy2) {
					cpy1.insert(end1);
					cpy1.insert(end2);

					if (!prune(cpy1, last)) {
						result.insert(cpy1);
					}
				}
			}
		}
	}

	return result;
}

int main(int argc, char **argv) {
	// global config vars
	std::string cfgOutput;
	std::string cfgDbData;
	std::string cfgDbDiscrete;
	data_t cfgEpsilon;
	data_t cfgOmega;
	std::size_t cfgXi;
	std::size_t cfgThreads;

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
			"dbdiscrete",
			po::value(&cfgDbDiscrete)->default_value("discrete.db"),
			"DB file that stores discrete data"
		)
		(
			"epsilon",
			po::value(&cfgEpsilon)->default_value(1.0, "1.0"),
			"Epsilon"
		)
		(
			"omega",
			po::value(&cfgOmega)->default_value(1.0, "1.0"),
			"Omega"
		)
		(
			"xi",
			po::value(&cfgXi)->default_value(10),
			"Xi"
		)
		(
			"threads",
			po::value(&cfgThreads)->default_value(0),
			"Number of threads (0 = auto)"
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
		std::cout << "enclus"<< std::endl << std::endl << poDesc << std::endl;
		return EXIT_SUCCESS;
	}

	// setup tbb
	if (cfgThreads == 0) {
		tbb::task_scheduler_init(-1 /*=tbb::task_scheduler_init::automatic*/);
	} else {
		tbb::task_scheduler_init(cfgThreads);
	}

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
		auto dbDiscrete = std::make_shared<gc::Database>(cfgDbDiscrete);
		std::ofstream outfile(cfgOutput);

		auto dimNameList = dbData->getIndexDims();
		std::vector<datadim_t> dims;
		assert(dimNameList->getSize() > 0);
		for (size_t i = 0; i < dimNameList->getSize(); ++i) {
			auto name(std::get<0>((*dimNameList)[i].toTuple()));
			dims.push_back(dbData->getDim<data_t>(name));
		}
		std::cout << "done (" << dims.size() << " columns, " << dims[0]->getSize() << " rows)" << std::endl;

		// discretize data
		tPhase.reset(new Tracer("discretize", tMain));
		std::vector<discretedim_t> ddims = discretize(dims, dbDiscrete, cfgXi);

		// generate 1D subspaces and calc entropy for them
		tPhase.reset(new Tracer("1d", tMain));
		std::cout << "Build 2D subspaces + fill entropy cache: " << std::flush;
		subspaces_t subspacesCurrent;
		entropyCache_t entropyCache;
		for (std::size_t i = 0; i < dims.size(); ++i) {
			subspacesCurrent.insert({i});
			entropyCache.push_back(calcEntropy({i}, ddims));

			// report progress
			if (i % 1000 == 0) {
				std::cout << i << std::flush;
			} else if (i % 100 == 0) {
				std::cout << "." << std::flush;
			}
		}
		std::cout << "done" << std::endl;

		// rounds
		tPhase.reset(new Tracer("tree", tMain));
		std::cout << "Tree phase: " << std::flush;
		std::vector<subspace_t> result;
		std::size_t depth = 0;
		data_t minEntropy = std::numeric_limits<data_t>::infinity();
		data_t maxInterest = 0;
		while (!subspacesCurrent.empty()) {
			subspaces_t subspacesNext;

			for (const auto& subspace : subspacesCurrent) {
				data_t entropy = calcEntropy(subspace, ddims);
				minEntropy = std::min(minEntropy, entropy);

				if (entropy < cfgOmega) {
					data_t interest = calcInterest(subspace, entropy, entropyCache);
					maxInterest = std::max(maxInterest, interest);

					if (interest > cfgEpsilon) {
						result.push_back(subspace);
					} else {
						subspacesNext.insert(subspace);
					}
				}
			}

			subspacesCurrent = genCandidates(subspacesNext);
			++depth;

			// report progress
			if (depth % 1 == 0) {
				std::cout << "." << std::flush;
			}
		}
		std::cout << "done (minEntropy=" << minEntropy << ", maxInterest=" << maxInterest << ")" << std::endl;

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

