#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <ostream>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <boost/program_options.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_scheduler_init.h>

#include "sys.hpp"
#include "tracer.hpp"

#include "enclus.hpp"
#include "disretize.hpp"
#include "gencandidates.hpp"

#include "greycore/database.hpp"
#include "greycore/dim.hpp"
#include "greycore/wrapper/graph.hpp"

namespace gc = greycore;
namespace po = boost::program_options;

density_t calcDensity(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density;
	std::size_t nSegments = data.at(0)->getSegmentCount();
	std::size_t n = data.at(0)->getSize();
	data_t step = 1.0 / static_cast<data_t>(n);
	std::vector<size_t> pos(subspace.size());

	for (std::size_t segment = 0; segment < nSegments; ++segment) {
		std::size_t size = data.at(0)->getSegmentFillSize(segment);

		typename std::vector<discretedimObj_t::segment_t*> segmentPtrs(subspace.size());
		std::size_t ptrIter = 0;
		for (std::size_t s : subspace) {
			const discretedim_t& dim = data.at(s);
			segmentPtrs[ptrIter++] = dim->getSegment(segment);
		}

		for (std::size_t i = 0; i < size; ++i) {
			std::size_t posIter = 0;

			for (const auto& segmentPtr : segmentPtrs) {
				pos[posIter++] = (*segmentPtr)[i];
			}

			density[pos] += step;
		}
	}

	return density;
}

data_t calcEntropy(const subspace_t& subspace, const std::vector<discretedim_t>& data) {
	density_t density = calcDensity(subspace, data);
	data_t result = 0.0;

	for (const auto& kv : density) {
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

class TBBHelper {
	public:
		std::list<subspace_t> subspacesNext;
		std::list<subspace_t> result;
		data_t minEntropy = std::numeric_limits<data_t>::infinity();
		data_t maxInterest = 0;

		TBBHelper(std::vector<subspace_t>& _subspacesCurrent, const std::vector<discretedim_t>& _ddims, const entropyCache_t& _entropyCache, data_t _omega, data_t _epsilon, std::size_t _xi) :
			subspacesCurrent(_subspacesCurrent),
			ddims(_ddims),
			entropyCache(_entropyCache),
			omega(_omega),
			epsilon(_epsilon),
			xi(_xi) {}

		TBBHelper(TBBHelper& obj, tbb::split) :
			subspacesCurrent(obj.subspacesCurrent),
			ddims(obj.ddims),
			entropyCache(obj.entropyCache),
			omega(obj.omega),
			epsilon(obj.epsilon),
			xi(obj.xi) {}

		void operator()(const tbb::blocked_range<std::size_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				auto& subspace = subspacesCurrent[i];
				data_t entropy = calcEntropy(subspace, ddims);
				data_t entropyNorm = entropy / log2(xi * subspace.size());
				minEntropy = std::min(minEntropy, entropyNorm);

				if (entropyNorm < omega) {
					data_t interest = calcInterest(subspace, entropy, entropyCache);
					data_t interestNorm = interest / log2(xi * subspace.size());
					maxInterest = std::max(maxInterest, interestNorm);

					if (interestNorm > epsilon) {
						result.push_back(std::move(subspace));
					} else {
						subspacesNext.push_back(std::move(subspace));
					}
				}
			}
		}

		void join(TBBHelper& obj) {
			this->subspacesNext.splice(this->subspacesNext.end(), obj.subspacesNext);
			this->result.splice(this->result.end(), obj.result);
			this->minEntropy = std::min(this->minEntropy, obj.minEntropy);
			this->maxInterest = std::max(this->maxInterest, obj.maxInterest);
		}

	private:
		std::vector<subspace_t>& subspacesCurrent;
		const std::vector<discretedim_t>& ddims;
		const entropyCache_t& entropyCache;
		data_t omega;
		data_t epsilon;
		std::size_t xi;
};

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
		std::cout << "Build 1D subspaces + fill entropy cache: " << std::flush;
		std::vector<subspace_t> subspacesCurrent;
		entropyCache_t entropyCache;
		for (std::size_t i = 0; i < dims.size(); ++i) {
			subspacesCurrent.push_back({i});
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
		std::cout << "Tree phase: " << std::endl;
		std::list<subspace_t> result;
		std::size_t depth = 0;
		data_t minEntropy = std::numeric_limits<data_t>::infinity();
		data_t maxInterest = 0;
		while (!subspacesCurrent.empty()) {
			std::cout << "depth=" << depth << ", candidatesNow=" << subspacesCurrent.size() << std::flush;

			TBBHelper helper(subspacesCurrent, ddims, entropyCache, cfgOmega, cfgEpsilon, cfgXi);
			parallel_reduce(tbb::blocked_range<std::size_t>(0, subspacesCurrent.size()), helper);

			result.splice(result.end(), helper.result);

			subspaces_t dict(helper.subspacesNext.begin(), helper.subspacesNext.end());
			std::cout << ", baseNext=" << dict.size() << std::flush;
			subspacesCurrent = genCandidates(dict);
			++depth;

			// report progress
			minEntropy = std::min(minEntropy, helper.minEntropy);
			maxInterest = std::max(maxInterest, helper.maxInterest);
			std::cout << std::endl;
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

