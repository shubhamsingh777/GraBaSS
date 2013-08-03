#include "gencandidates.hpp"

#include <list>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

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

class TBBHelperGC {
	public:
		std::list<subspace_t> result;

		TBBHelperGC(const subspaces_t& _last, const std::vector<subspace_t> _lastVector) :
			last(_last),
			lastVector(_lastVector) {}

		TBBHelperGC(TBBHelperGC& obj, tbb::split) :
			last(obj.last),
			lastVector(obj.lastVector) {}

		void operator()(const tbb::blocked_range<std::size_t>& range) {
			for (auto i = range.begin(); i != range.end(); ++i) {
				auto& ss1 = lastVector.at(i);
				bool ignore = true;

				for (const auto& ss2 : lastVector) {
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
								result.push_back(cpy1);
							}
						}
					}
				}
			}
		}

		void join(TBBHelperGC& obj) {
			this->result.splice(this->result.end(), obj.result);
		}

	private:
		const subspaces_t& last;
		const std::vector<subspace_t> lastVector;
};


std::vector<subspace_t> genCandidates(const subspaces_t& last) {
	std::vector<subspace_t> lastVector(last.begin(), last.end());
	TBBHelperGC helper(last, lastVector);
	parallel_reduce(tbb::blocked_range<std::size_t>(0, lastVector.size()), helper);

	std::vector<subspace_t> result(helper.result.begin(), helper.result.end());
	return result;
}


