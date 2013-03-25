#ifndef D2OPS_HPP
#define D2OPS_HPP

#include "dim.hpp"
#include "d1ops.hpp"

#include <cassert>
#include <memory>
#include <string>

// prototype
template <typename T, typename M>
class D2Ops;

template <typename T, typename M>
class D2OpExe {
	public:
		virtual M run(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) = 0;
};

template <typename T, typename M>
class _D2OpCov : public D2OpExe<T,M> {
	public:
		M run(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) override {
			assert(d1->getSize() == d2->getSize());
			M exp1 = D1Ops<T,M>::Exp.getResult(d1);
			M exp2 = D1Ops<T,M>::Exp.getResult(d2);
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
class _D2OpPearson : public D2OpExe<T,M> {
	public:
		M run(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) override {
			M cov = D2Ops<T,M>::Cov.calc(d1,d2);
			M o1 = D1Ops<T,M>::StdDev.getResult(d1);
			M o2 = D1Ops<T,M>::StdDev.getResult(d2);
			return cov / (o1 * o2);
		}
};

template <typename T, typename M>
class D2Op {
	public:
		D2Op(int id, std::string name, std::shared_ptr<D2OpExe<T,M>> exe) : ID(id), NAME(name), EXE(exe) {};

		M calc(std::shared_ptr<Dim<T,M>> d1, std::shared_ptr<Dim<T,M>> d2) {
			return EXE->run(d1, d2);
		}

		int getId() const {
			return ID;
		}

		std::string getName() const {
			return NAME;
		}

	private:
		const int ID;
		const std::string NAME;
		std::shared_ptr<D2OpExe<T,M>> EXE;
};

template <typename T, typename M>
class D2Ops {
	public:
		static D2Op<T,M> Cov;
		static D2Op<T,M> Pearson;
};

template <typename T, typename M>
D2Op<T,M> D2Ops<T,M>::Cov = D2Op<T,M>(1, "covariance", std::shared_ptr<D2OpExe<T,M>>(new _D2OpCov<T,M>()));

template <typename T, typename M>
D2Op<T,M> D2Ops<T,M>::Pearson = D2Op<T,M>(2, "pearsons r", std::shared_ptr<D2OpExe<T,M>>(new _D2OpPearson<T,M>()));

#endif

