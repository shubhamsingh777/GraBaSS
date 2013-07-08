#ifndef D1OPS_HPP
#define D1OPS_HPP

#include "greycore/dim.hpp"
#include "greycore/wrapper/flatmap.hpp"
#include "sys.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// master op
template <int ID, typename EXE>
struct D1Op {
	static data_t calc(datadim_t dim, mdMap_t map) {
		return EXE::run(dim, map);
	}

	static data_t calcAndStore(datadim_t dim, mdMap_t map) {
		data_t x = calc(dim, map);
		map->add(ID, x);
		return x;
	}

	static void calcAndStoreVector(std::vector<std::pair<datadim_t, mdMap_t>> dims) {
		std::cout << "Calc " << EXE::getName() << ": " << std::flush;

		// test if there is already a cached version
		if (!dims.empty()) {
			try {
				(*dims.begin()).second->get(ID);
				std::cout << "skipped" << std::endl;
				return;
			} catch (const std::out_of_range& e) {
				// noop, just continue with calculation
			}
		}

		for (std::size_t i = 0; i < dims.size(); ++i) {
			calcAndStore(dims[i].first, dims[i].second);

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
		return EXE::getName();
	}

	static data_t getResult(datadim_t dim, mdMap_t map) {
		data_t result;
		try {
			result = map->get(ID);
		} catch (const std::out_of_range& e) {
			// not calculated yet
			result = calcAndStore(dim, map);
		}
		return result;
	}
};

// prototypes
struct _D1OpExp;
struct _D1OpVar;
struct _D1OpStdDev;

// define ops
struct D1Ops {
	using Exp = D1Op<1, _D1OpExp>;
	using Var = D1Op<2, _D1OpVar>;
	using StdDev = D1Op<3, _D1OpStdDev>;
};

// real code
struct _D1OpExp {
	static data_t run(datadim_t dim, mdMap_t) {
		data_t sum = 0;

		std::size_t nSegments = dim->getSegmentCount();
		for (std::size_t segment = 0; segment < nSegments; ++segment) {
			std::size_t size = dim->getSegmentFillSize(segment);
			typename datadimObj_t::segment_t* data = dim->getSegment(segment);
			for (std::size_t i = 0; i < size; ++i) {
				sum += (*data)[i];
			}
		}
		return sum / dim->getSize();
	}

	static std::string getName() {
		return "expectation";
	}
};

struct _D1OpVar {
	static data_t run(datadim_t dim, mdMap_t map) {
		data_t exp = D1Ops::Exp::getResult(dim, map);
		data_t sum = 0;

		std::size_t nSegments = dim->getSegmentCount();
		for (std::size_t segment = 0; segment < nSegments; ++segment) {
			std::size_t size = dim->getSegmentFillSize(segment);
			typename datadimObj_t::segment_t* data = dim->getSegment(segment);
			for (std::size_t i = 0; i < size; ++i) {
				data_t x = (*data)[i] - exp;
				sum += x * x;
			}
		}
		return sum / dim->getSize();
	}

	static std::string getName() {
		return "variance";
	}
};

struct _D1OpStdDev {
	static data_t run(datadim_t dim, mdMap_t map) {
		data_t var = D1Ops::Var::getResult(dim, map);
		return sqrt(var);
	}

	static std::string getName() {
		return "standard deviation";
	}
};

#endif

