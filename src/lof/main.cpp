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

#include "greycore/database.hpp"
#include "greycore/dim.hpp"
#include "greycore/wrapper/graph.hpp"

namespace gc = greycore;
namespace po = boost::program_options;

typedef std::vector<std::vector<std::tuple<std::size_t, data_t>>> distCache_t;
typedef std::vector<std::tuple<data_t, std::size_t>> kDists_t;
typedef std::vector<datadimObj_t::segment_t*> segmentPtrs_t;

std::vector<std::size_t> parseSS(const std::string& s) {
	std::stringstream stream(s);
	std::vector<std::size_t> result;

	std::string id;
	while (std::getline(stream, id, ',')) {
		std::size_t tmp;
		std::istringstream(id) >> tmp;
		result.push_back(tmp);
	}

	return result;
}

data_t d(std::size_t i, std::size_t j, const segmentPtrs_t& ptrs1, const segmentPtrs_t& ptrs2) {
	data_t sum = 0;

	for (std::size_t s = 0; s < ptrs1.size(); ++s) {
		data_t posA = (*(ptrs1[s]))[i];
		data_t posB = (*(ptrs2[s]))[j];
		data_t delta = posA - posB;
		sum += delta * delta;
	}

	return sqrt(sum);
}

distCache_t precalcDists(std::size_t kMax, const std::vector<datadim_t>& data, const std::vector<std::size_t>& subspace) {
	std::size_t nObjs = data[0]->getSize();
	std::size_t nSegments = data[0]->getSegmentCount();
	std::vector<std::tuple<std::size_t, data_t>> dists(nObjs - 1);
	distCache_t result(nObjs);

	// level 1: segments
	std::size_t obj1 = 0;
	for (std::size_t segment1 = 0; segment1 < nSegments; ++segment1) {
		std::size_t size1 = data[0]->getSegmentFillSize(segment1);

		segmentPtrs_t segmentPtr1(subspace.size());
		std::size_t ptrIter1 = 0;
		for (std::size_t s : subspace) {
			const auto& dim = data[s];
			segmentPtr1[ptrIter1++] = dim->getSegment(segment1);
		}

		// level 1: objects
		for (std::size_t i = 0; i < size1; ++i) {
			// level 2: segments
			std::size_t obj2 = 0;
			std::size_t idx = 0;
			for (std::size_t segment2 = 0; segment2 < nSegments; ++segment2) {
				std::size_t size2 = data[0]->getSegmentFillSize(segment2);

				segmentPtrs_t segmentPtr2(subspace.size());
				std::size_t ptrIter2 = 0;
				for (std::size_t s : subspace) {
					const auto& dim = data[s];
					segmentPtr2[ptrIter2++] = dim->getSegment(segment2);
				}

				// level 2: objects
				for (std::size_t j = 0; j < size2; ++j) {
					if (obj1 != obj2) {
						dists[idx++] = std::make_tuple(obj2, d(i, j, segmentPtr1, segmentPtr2));
					}

					++obj2;
				}
			}

			std::sort(dists.begin(), dists.end(), [](const std::tuple<std::size_t, data_t>& a, const std::tuple<std::size_t, data_t>& b){
				return std::get<1>(a) < std::get<1>(b);
			});

			auto& neighbors = result[obj1];
			data_t kMaxDist = std::get<1>(dists[kMax - 1]);
			for (std::size_t neighbor = 0; (neighbor < nObjs - 1) && (std::get<1>(dists[neighbor]) <= kMaxDist); ++neighbor) {
				neighbors.push_back(dists[neighbor]);
			}

			// report progress
			if (obj1 % 1000 == 0) {
				std::cout << "+" << std::flush;
			}

			++obj1;
		}
	}

	return result;
}

kDists_t calcKDists(std::size_t k, const std::vector<datadim_t>& data,const distCache_t& distCache) {
	std::size_t nObjs = data[0]->getSize();
	std::vector<std::tuple<data_t, std::size_t>> result(nObjs);

	for (std::size_t obj1 = 0; obj1 < nObjs; ++obj1) {
		const auto& distCachePtr = distCache[obj1];
		auto& kDist = std::get<0>(result[obj1]);
		auto& kNeighborsEnd = std::get<1>(result[obj1]);

		kDist = std::get<1>(distCachePtr[k - 1]);
		for (kNeighborsEnd = 0; (kNeighborsEnd < distCachePtr.size()) && (std::get<1>(distCachePtr[kNeighborsEnd]) <= kDist); ++kNeighborsEnd) {}
	}

	return result;
}

std::vector<data_t> calcLRDs(const std::vector<datadim_t>& data, const distCache_t& distCache, const kDists_t& dists) {
	std::size_t nObjs = data[0]->getSize();
	std::vector<data_t> result(nObjs);

	for (std::size_t obj = 0; obj < nObjs; ++obj) {
		const auto& distCachePtr = distCache[obj];
		std::size_t neighborsEnd = std::get<1>(dists[obj]);

		data_t sum = 0.0;
		for (std::size_t i = 0; i < neighborsEnd; ++i) {
			std::size_t neighbor = std::get<0>(distCachePtr[i]);
			sum += std::max(std::get<0>(dists[neighbor]), std::get<1>(distCachePtr[i]));
		}

		result[obj] = static_cast<data_t>(neighborsEnd) / sum;
	}

	return result;
}

std::vector<data_t> calcLOF(std::size_t k, const std::vector<datadim_t>& data, const distCache_t& distCache) {
	std::size_t nObjs = data[0]->getSize();
	std::vector<data_t> result(nObjs);
	auto dists = calcKDists(k, data, distCache);
	auto lrds = calcLRDs(data, distCache, dists);

	for (std::size_t obj = 0; obj < nObjs; ++obj) {
		const auto& distCachePtr = distCache[obj];
		std::size_t neighborsEnd = std::get<1>(dists[obj]);

		data_t sum = 0.0;
		for (std::size_t i = 0; i < neighborsEnd; ++i) {
			std::size_t neighbor = std::get<0>(distCachePtr[i]);
			sum += lrds[neighbor];
		}
		result[obj] = sum / (lrds[obj] * neighborsEnd);
	}

	return result;
}

int main(int argc, char **argv) {
	// global config vars
	std::string cfgSubspaces;
	std::string cfgOutput;
	std::string cfgDbData;
	std::size_t cfgMinPtsLower;
	std::size_t cfgMinPtsUpper;
	std::size_t cfgThreads;

	// parse program options
	po::options_description poDesc("Options");
	poDesc.add_options()
		(
			"subspaces",
			po::value(&cfgSubspaces)->default_value("subspaces.txt"),
			"Subspace list"
		)
		(
			"output",
			po::value(&cfgOutput)->default_value("lof.ssv"),
			"LOF values for each subspace"
		)
		(
			"dbdata",
			po::value(&cfgDbData)->default_value("columns.db"),
			"DB file that stores parsed data"
		)
		(
			"minPtsLower",
			po::value(&cfgMinPtsLower)->default_value(30),
			"Lower bound for minPts"
		)
		(
			"minPtsUpper",
			po::value(&cfgMinPtsUpper)->default_value(50),
			"Upper bound for minPts"
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
		std::ifstream ssfile(cfgSubspaces);
		auto dbData = std::make_shared<gc::Database>(cfgDbData);
		std::ofstream outfile(cfgOutput);

		auto dimNameList = dbData->getIndexDims();
		std::vector<datadim_t> dims;
		assert(dimNameList->getSize() > 0);
		for (size_t i = 0; i < dimNameList->getSize(); ++i) {
			auto name(std::get<0>((*dimNameList)[i].toTuple()));
			dims.push_back(dbData->getDim<data_t>(name));
		}
		std::cout << "done (" << dims.size() << " columns, " << dims[0]->getSize() << " rows)" << std::endl;

		// iterate over all subspaces
		tPhase.reset(new Tracer("subspaces", tMain));
		std::cout << "Calculate LOF for all subspaces: " << std::flush;
		std::vector<std::list<data_t>> lofs(dims[0]->getSize());
		std::string ssstring;
		std::size_t sID = 0;
		while (std::getline(ssfile, ssstring)) {
			auto subspace = parseSS(ssstring);
			if (subspace.size() > 0) {
				std::cout << "Subspace " << sID << " (size=" << subspace.size() << "): " << std::flush;
				auto cache = precalcDists(cfgMinPtsUpper, dims, subspace);

				std::vector<data_t> bestLOFs(lofs.size(), -std::numeric_limits<data_t>::infinity());
				for (std::size_t k = cfgMinPtsLower; k <= cfgMinPtsUpper; ++k) {
					auto next = calcLOF(k, dims, cache);
					bestLOFs = std::max(bestLOFs, next);

					// report progress
					std::cout << "." << std::flush;
				}

				// normalize
				data_t max = *std::max_element(bestLOFs.cbegin(), bestLOFs.cend());
				for (auto& x : bestLOFs) {
					x /= max;
				}

				for (std::size_t i = 0; i < bestLOFs.size(); ++i) {
					lofs[i].push_back(bestLOFs[i]);
				}

				std::cout << "done" << std::endl;
			}
		}
		std::cout << "all done" << std::endl;

		// ok, so lets write the results
		tPhase.reset(new Tracer("output", tMain));
		std::cout << "Write result: " << std::flush;
		for (const auto& obj : lofs) {
			bool first = true;
			for (auto lof : obj) {
				if (first) {
					first = false;
				} else {
					outfile << " ";
				}
				outfile << lof;
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

