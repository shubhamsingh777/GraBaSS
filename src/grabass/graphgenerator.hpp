#ifndef GRAPHGENERATOR_HPP
#define GRAPHGENERATOR_HPP

#include "greycore/wrapper/graph.hpp"

#include <functional>
#include <list>
#include <vector>
#include <unorderd_set>

template <typename CandidateIterator>
class GraphGenerator {
	public:
		typedef std::function<CandidateIterator (std::size_t)> genCandiates_t;
		typedef std::function<bool (std::size_t, std::size_t)> check_t;

		GraphGenerator(std::size_t size, genCandiates_t genCandiates, check_t check) :
			size(size),
			cache(size),
			genCandiates(genCandiates),
			check(check) {}

		void operator(std::shared_ptr<greycore::Graph> output) {
			assert(output->getSize() == 0);

			for (std::size_t v = 0; v < size; ++v) {
				std::list<std::size_t> neighbors;

				for (std::size_t w : genCandiates(v)) {
					if (w < v) {
						auto cachePart = cache[w];
						if (cachePart.find(v) != cachePart.end()) {
							neighbors.push_back(w);
						}
					} else {
						if (check(v, w)) {
							neighbors.push_back(w);
							cache[v].insert(w);
						}
					}
				}

				output->add(neighbors);
			}
		}

	private:
		std::size_t size;
		std::vector<std::unorderd_set<std::size_t>> cache;
		genCandiates_t genCandiates;
		check_t check;
};

#endif

