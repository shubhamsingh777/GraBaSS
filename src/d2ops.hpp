#ifndef D2OPS_HPP
#define D2OPS_HPP

#include "dim.hpp"
#include "d1ops.hpp"

#include <cassert>
#include <memory>
#include <string>

// name lookup table
template <int ID>
struct D2OpName {
	static const std::string NAME;
};

template <>
const std::string D2OpName<1>::NAME("covariance");

template <>
const std::string D2OpName<2>::NAME("pearsons r");

// master op
template <typename T, typename M, int ID, typename EXE>
struct D2Op {
	static M calc(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) {
		return EXE::run(d1, d2);
	}

	static int getId() {
		return ID;
	}

	static std::string getName() {
		return D2OpName<ID>::NAME;
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
	static M run(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) {
		assert(d1->getSize() == d2->getSize());
		M exp1 = D1Ops<T,M>::Exp::getResult(d1);
		M exp2 = D1Ops<T,M>::Exp::getResult(d2);
		M sum = 0;

		long nSegments = d1->getSegmentCount();
		for (long segment = 0; segment < nSegments; ++segment) {
			long size = d1->getSegmentFillSize(segment);
			T* s1 = d1->getRawSegment(segment);
			T* s2 = d2->getRawSegment(segment);
			for (long i = 0; i < size; ++i) {
				sum += (s1[i] - exp1) * (s2[i] - exp2);
			}
		}

		return sum / d1->getSize();
	}
};

template <typename T, typename M>
struct _D2OpPearson {
	static M run(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) {
		M cov = D2Ops<T,M>::Cov::calc(d1,d2);
		M o1 = D1Ops<T,M>::StdDev::getResult(d1);
		M o2 = D1Ops<T,M>::StdDev::getResult(d2);
		return cov / (o1 * o2);
	}
};

#endif

