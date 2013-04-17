#ifndef D1OPS_HPP
#define D1OPS_HPP

#include "dim.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// name lookup table
template <int ID>
struct D1OpName {
	static const std::string NAME;
};

template <>
const std::string D1OpName<1>::NAME("expectation");

template <>
const std::string D1OpName<2>::NAME("variance");

template <>
const std::string D1OpName<3>::NAME("standard deviation");

// master op
template <typename T, typename M, int ID, typename EXE>
struct D1Op {
	static M calc(std::shared_ptr<Dim<T,M>> dim) {
		return EXE::run(dim);
	}

	static M calcAndStore(std::shared_ptr<Dim<T,M>> dim) {
		M x = calc(dim);
		dim->addMetadata(ID, x);
		return x;
	}

	static void calcAndStoreVector(std::vector<std::shared_ptr<Dim<T,M>>> dims) {
		std::cout << "Calc " << D1OpName<ID>::NAME << ": " << std::flush;
		for (unsigned long i = 0; i < dims.size(); ++i) {
			calcAndStore(dims[i]);

			if (i % 100 == 0) {
				std::cout << "." << std::flush;
			}
		}
		std::cout << "done" << std::endl;
	}

	static int getId() {
		return ID;
	}

	static std::string getName() {
		return D1OpName<ID>::NAME;
	}

	static M getResult(std::shared_ptr<Dim<T,M>> dim) {
		M result;
		try {
			result = dim->getMetadata(ID);
		} catch (const std::runtime_error& e) {
			// not calculated yet
			result = calcAndStore(dim);
		}
		return result;
	}
};

// prototypes
template <typename T, typename M>
struct _D1OpExp;

template <typename T, typename M>
struct _D1OpVar;

template <typename T, typename M>
struct _D1OpStdDev;

// define ops
template <typename T, typename M>
struct D1Ops {
	using Exp = D1Op<T, M, 1, _D1OpExp<T,M>>;
	using Var = D1Op<T, M, 2, _D1OpVar<T,M>>;
	using StdDev = D1Op<T, M, 3, _D1OpStdDev<T,M>>;
};

// real code
template <typename T, typename M>
struct _D1OpExp {
	static M run(std::shared_ptr<Dim<T,M>> dim) {
		M sum = 0;

		long nSegments = dim->getSegmentCount();
		for (long segment = 0; segment < nSegments; ++segment) {
			long size = dim->getSegmentFillSize(segment);
			T* data = dim->getRawSegment(segment);
			for (long i = 0; i < size; ++i) {
				sum += data[i];
			}
		}
		return sum / dim->getSize();
	}
};

template <typename T, typename M>
struct _D1OpVar {
	static M run(std::shared_ptr<Dim<T,M>> dim) {
		M exp = D1Ops<T,M>::Exp::getResult(dim);
		M sum = 0;

		long nSegments = dim->getSegmentCount();
		for (long segment = 0; segment < nSegments; ++segment) {
			long size = dim->getSegmentFillSize(segment);
			T* data = dim->getRawSegment(segment);
			for (long i = 0; i < size; ++i) {
				M x = data[i] - exp;
				sum += x * x;
			}
		}
		return sum / dim->getSize();
	}
};

template <typename T, typename M>
struct _D1OpStdDev {
	static M run(std::shared_ptr<Dim<T,M>> dim) {
		M var = D1Ops<T,M>::Var::getResult(dim);
		return sqrt(var);
	}
};

#endif

