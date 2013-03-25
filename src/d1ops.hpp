#ifndef D1OPS_HPP
#define D1OPS_HPP

#include "dim.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// prototype
template <typename T, typename M>
class D1Ops;

template <typename T, typename M>
class D1OpExe {
	public:
		virtual M run(std::shared_ptr<Dim<T,M>> dim) = 0;
};

template <typename T, typename M>
class _D1OpExp : public D1OpExe<T,M> {
	public:
		M run(std::shared_ptr<Dim<T,M>> dim) override {
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
class _D1OpVar : public D1OpExe<T,M> {
	public:
		M run(std::shared_ptr<Dim<T,M>> dim) override {
			M exp = D1Ops<T,M>::Exp.getResult(dim);
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
class _D1OpStdDev : public D1OpExe<T,M> {
	public:
		M run(std::shared_ptr<Dim<T,M>> dim) override {
			M var = D1Ops<T,M>::Var.getResult(dim);
			return sqrt(var);
		}
};

template <typename T, typename M>
class D1Op {
	public:
		D1Op(int id, std::string name, std::shared_ptr<D1OpExe<T,M>> exe) : ID(id), NAME(name), EXE(exe) {};

		M calc(std::shared_ptr<Dim<T,M>> dim) {
			return EXE->run(dim);
		}

		M calcAndStore(std::shared_ptr<Dim<T,M>> dim) {
			M x = calc(dim);
			dim->addMetadata(ID, x);
			return x;
		}

		void calcAndStoreVector(std::vector<std::shared_ptr<Dim<T,M>>> dims) {
			std::cout << "Calc " << NAME << ": " << std::flush;
			for (unsigned long i = 0; i < dims.size(); ++i) {
				calcAndStore(dims[i]);

				if (i % 100 == 0) {
					std::cout << "." << std::flush;
				}
			}
			std::cout << "done" << std::endl;
		}

		int getId() const {
			return ID;
		}

		std::string getName() const {
			return NAME;
		}

		M getResult(std::shared_ptr<Dim<T,M>> dim) {
			M result;
			try {
				result = dim->getMetadata(ID);
			} catch (const std::runtime_error& e) {
				// not calculated yet
				result = calcAndStore(dim);
			}
			return result;
		}

	private:
		const int ID;
		const std::string NAME;
		std::shared_ptr<D1OpExe<T,M>> EXE;
};

template <typename T, typename M>
class D1Ops {
	public:
		static D1Op<T,M> Exp;
		static D1Op<T,M> StdDev;
		static D1Op<T,M> Var;
};

template <typename T, typename M>
D1Op<T,M> D1Ops<T,M>::Exp = D1Op<T,M>(1, "expectation", std::shared_ptr<D1OpExe<T,M>>(new _D1OpExp<T,M>()));

template <typename T, typename M>
D1Op<T,M> D1Ops<T,M>::StdDev = D1Op<T,M>(3, "standard deviation", std::shared_ptr<D1OpExe<T,M>>(new _D1OpStdDev<T,M>()));

template <typename T, typename M>
D1Op<T,M> D1Ops<T,M>::Var = D1Op<T,M>(2, "variance", std::shared_ptr<D1OpExe<T,M>>(new _D1OpVar<T,M>()));

#endif

