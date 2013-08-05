#include "gencandidates.hpp"

#include <list>

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

bool prune(const subspace_t& subspace, const subspaces_t& last) {
	for (std::size_t idxD = 0; idxD < subspace.size(); ++idxD) {
		// gen subsubspace
		subspace_t test;
		std::size_t i = 0;
		for (std::size_t d : subspace) {
			if (i != idxD) {
				test.push_back(d);
			}
			++i;
		}

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
						subspace_t next;
						auto iter1 = ss1.begin();
						auto iter2 = ss2.begin();
						std::size_t x1;
						std::size_t x2;
						while ((iter1 != ss1.end()) && (iter2 != ss2.end())) {
							x1 = *iter1;
							x2 = *iter2;

							++iter1;
							++iter2;

							if (x1 == x2) {
								next.push_back(x1);
							} else {
								break;
							}
						}

						if ((iter1 == ss1.end()) && (iter2 == ss2.end())) {
							if (x1 < x2) {
								next.push_back(x1);
								next.push_back(x2);
							} else {
								next.push_back(x2);
								next.push_back(x1);
							}

							if (!prune(next, last)) {
								result.push_back(next);
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


