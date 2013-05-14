#ifndef D2OPS_HPP
#define D2OPS_HPP

#include "greycore/dim.hpp"
#include "greycore/wrapper/flatmap.hpp"
#include "d1ops.hpp"

#include <cassert>
#include <memory>
#include <string>

// master op
template <typename T, typename M, int ID, typename EXE>
struct D2Op {
	typedef greycore::Flatmap<int, M> map_t;

	static M calc(std::shared_ptr<greycore::Dim<T>> d1, std::shared_ptr<greycore::Dim<T>> d2, std::shared_ptr<map_t> m1, std::shared_ptr<map_t> m2) {
		return EXE::run(d1, d2, m1, m2);
	}

	static int getId() {
		return ID;
	}

	static std::string getName() {
		return EXE::getName();
	}
};

// prototypes
template <typename T, typename M>
struct _D2OpCov;

template <typename T, typename M>
struct _D2OpPearson;

// define ops
template <typename T, typename M>
struct D2Ops {
	using Cov = D2Op<T, M, 1, _D2OpCov<T,M>>;
	using Pearson = D2Op<T, M, 2, _D2OpPearson<T,M>>;
};

// real code
template <typename T, typename M>
struct _D2OpCov {
	typedef greycore::Flatmap<int, M> map_t;

	static M run(std::shared_ptr<greycore::Dim<T>> d1, std::shared_ptr<greycore::Dim<T>> d2, std::shared_ptr<map_t> m1, std::shared_ptr<map_t> m2) {
		assert(d1->getSize() == d2->getSize());
		M exp1 = D1Ops<T,M>::Exp::getResult(d1, m1);
		M exp2 = D1Ops<T,M>::Exp::getResult(d2, m2);
		M sum = 0;

		long nSegments = d1->getSegmentCount();
		for (long segment = 0; segment < nSegments; ++segment) {
			long size = d1->getSegmentFillSize(segment);
			typename greycore::Dim<T>::segment_t* s1 = d1->getSegment(segment);
			typename greycore::Dim<T>::segment_t* s2 = d2->getSegment(segment);
			for (long i = 0; i < size; ++i) {
				sum += ((*s1)[i] - exp1) * ((*s2)[i] - exp2);
			}
		}

		return sum / d1->getSize();
	}

	static std::string getName() {
		return "covariance";
	}
};

template <typename T, typename M>
struct _D2OpPearson {
	typedef greycore::Flatmap<int, M> map_t;

	static M run(std::shared_ptr<greycore::Dim<T>> d1, std::shared_ptr<greycore::Dim<T>> d2, std::shared_ptr<map_t> m1, std::shared_ptr<map_t> m2) {
		M cov = D2Ops<T,M>::Cov::calc(d1, d2, m1, m2);
		M o1 = D1Ops<T,M>::StdDev::getResult(d1, m1);
		M o2 = D1Ops<T,M>::StdDev::getResult(d2, m2);
		return cov / (o1 * o2);
	}

	static std::string getName() {
		return "pearsons r";
	}
};

#endif

