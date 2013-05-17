#ifndef D2OPS_HPP
#define D2OPS_HPP

#include "greycore/dim.hpp"
#include "greycore/wrapper/flatmap.hpp"
#include "d1ops.hpp"
#include "sys.hpp"

#include <cassert>
#include <memory>
#include <string>

// master op
template <int ID, typename EXE>
struct D2Op {
	static data_t calc(datadim_t d1, datadim_t d2, mdMap_t m1, mdMap_t m2) {
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
struct _D2OpCov;
struct _D2OpPearson;

// define ops
struct D2Ops {
	using Cov = D2Op<1, _D2OpCov>;
	using Pearson = D2Op<2, _D2OpPearson>;
};

// real code
struct _D2OpCov {
	static data_t run(datadim_t d1, datadim_t d2, mdMap_t m1, mdMap_t m2) {
		assert(d1->getSize() == d2->getSize());
		data_t exp1 = D1Ops::Exp::getResult(d1, m1);
		data_t exp2 = D1Ops::Exp::getResult(d2, m2);
		data_t sum = 0;

		std::size_t nSegments = d1->getSegmentCount();
		for (std::size_t segment = 0; segment < nSegments; ++segment) {
			std::size_t size = d1->getSegmentFillSize(segment);
			typename datadimObj_t::segment_t* s1 = d1->getSegment(segment);
			typename datadimObj_t::segment_t* s2 = d2->getSegment(segment);
			for (std::size_t i = 0; i < size; ++i) {
				sum += ((*s1)[i] - exp1) * ((*s2)[i] - exp2);
			}
		}

		return sum / d1->getSize();
	}

	static std::string getName() {
		return "covariance";
	}
};

struct _D2OpPearson {
	static data_t run(datadim_t d1, datadim_t d2, mdMap_t m1, mdMap_t m2) {
		data_t cov = D2Ops::Cov::calc(d1, d2, m1, m2);
		data_t o1 = D1Ops::StdDev::getResult(d1, m1);
		data_t o2 = D1Ops::StdDev::getResult(d2, m2);
		return cov / (o1 * o2);
	}

	static std::string getName() {
		return "pearsons r";
	}
};

#endif

