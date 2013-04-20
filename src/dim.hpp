#ifndef DIM_HPP
#define DIM_HPP

#include <cassert>
#include <array>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "sys.hpp"
#include "dbfile.hpp"
template <typename T, typename M>
class DimIter;

template <typename T, typename M = int>
class Dim {
	public:
		static constexpr int SEGMENT_SIZE = 64 * PAGE_SIZE / sizeof(T); // 64 pages (opt 256 pages)

		friend class DimIter<T,M>;
		typedef DimIter<T,M> iterator;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef T value_type;
		typedef T* pointer;
		typedef T& reference;

		static void rescue(std::string source, std::string target, std::string name) {
			std::cout << "open source \"" << source << "\"..." << std::flush;
			boost::interprocess::managed_mapped_file sourceFile(boost::interprocess::open_read_only, source.c_str());
			std::cout << "ok" << std::endl;

			std::cout << "get file size..." << std::flush;
			long fileSize = sourceFile.get_size();
			std::cout << fileSize << std::endl;

			std::cout << "open target \"" << target << "\"..." << std::flush;
			boost::interprocess::managed_mapped_file targetFile(boost::interprocess::open_or_create, target.c_str(), fileSize);
			std::cout << "ok" << std::endl;

			std::cout << "find size marker..." << std::flush;
			auto lookup = sourceFile.find<long>(genSizeID(name).c_str());
			assert(lookup.second == 1);
			long size = *lookup.first;
			targetFile.construct<long>(genSizeID(name).c_str())(size);
			std::cout << size << std::endl;

			long nSegments = size / SEGMENT_SIZE;
			std::cout << "rescue " << nSegments << " segments:" << std::endl;
			for (long i = 0; i < nSegments; i++) {
				std::string sID = genSegmentID(name, i);

				auto test = sourceFile.find<segment_t>(sID.c_str());
				assert(test.second == 1);
				segment_t* segmentPtr = test.first;

				targetFile.construct<segment_t>(sID.c_str())(*segmentPtr);

				std::cout << "." << std::flush;
			}
			std::cout << "ok" << std::endl;

			std::cout << "rescue metadata:" << std::endl;
			auto testFirst = sourceFile.find<int>(genMetadataFirstID(name).c_str());
			assert(testFirst.second == 1);
			int first = *testFirst.first;
			targetFile.construct<int>(genMetadataFirstID(name).c_str())(first);
			int* lastPtr = targetFile.construct<int>(genMetadataLastID(name).c_str())(first);
			int next = first;
			while (next != 0) {
				std::string mID = genMetadataID(name, next);
				auto test = sourceFile.find<metadata_t>(mID.c_str());
				assert(test.second == 1);
				targetFile.construct<metadata_t>(mID.c_str())(*test.first);
				(*lastPtr) = next;
				std::cout << next << "|" << std::flush;
				next = (*test.first).next;
			}
			std::cout << "ok" << std::endl;

			std::cout << "done! :)" << std::endl;
		}

		Dim(std::shared_ptr<DBFile> file, std::string name) :
				name(name),
				file(file),
				callbackID(file->registerPResetFun(std::bind(&Dim::resetPtrs, this))) {
			resetPtrs();
			assert(!((*this->metadataFirst) == 0) || ((*this->metadataLast) == 0));
		}

		~Dim() {
			file->unregisterPResetFun(callbackID);
		}

		std::string getName() {
			return this->name;
		}

		long getSize() {
			return *this->size;
		}

		long add(T x) {
			long pos = *this->size;
			long segment = pos / SEGMENT_SIZE;
			long offset = pos % SEGMENT_SIZE;
			segment_t* segmentPtr = getSegmentPtr(segment);

			// store data
			(*segmentPtr)[offset] = x;
			(*this->size)++;

			return pos;
		}

		T get(long pos) {
			long segment = pos / SEGMENT_SIZE;
			long offset = pos % SEGMENT_SIZE;
			segment_t* segmentPtr = getSegmentPtr(segment);

			return (*segmentPtr)[offset];
		}

		void addMetadata(int id, M data) {
			assert(id != 0);

			std::string mIDNew = genMetadataID(name, id);
			metadata_t md = {data, 0};
			file->construct<metadata_t>(mIDNew, md);

			if ((*this->metadataFirst) == 0) {
				(*this->metadataFirst) = id;
			} else {
				std::string mIDOld = genMetadataID(name, *this->metadataLast);
				auto lookup = file->find<metadata_t>(mIDOld);
				assert(lookup.second == 1);
				(*lookup.first).next = id;
			}
			(*this->metadataLast) = id;

			// cache stuff
			this->metadata.emplace(id, data);
		}

		M getMetadata(int id) {
			std::lock_guard<std::mutex> lock(this->mutexMetadata);

			try {
				// try to used cached value
				return this->metadata.at(id);
			} catch (const std::out_of_range& e) {
				std::string mID = genMetadataID(name, id);
				auto lookup = file->find<metadata_t>(mID);

				if (lookup.second != 1) {
					throw std::runtime_error("Metadata not found");
				}

				this->metadata.emplace(id, (*lookup.first).data);
				return (*lookup.first).data;
			}
		}

		iterator begin() {
			return iterator(this, 0, (*this->size) > 0);
		}

		iterator end() {
			return iterator(this, *this->size, false);
		}

		iterator iterAt(long pos) {
			return iterator(this, pos, (*this->size) > pos);
		}

		T* getRawSegment(long segment) {
			segment_t* segmentPtr = getSegmentPtr(segment);
			return (*segmentPtr).data();
		}

		long getSegmentCount() {
			if ((*this->size) > 0) {
				return (*this->size - 1) / SEGMENT_SIZE + 1;
			} else {
				return 0;
			}
		}

		long getSegmentFillSize(long segment) {
			if ((segment + 1) * SEGMENT_SIZE <= (*this->size)) {
				// full
				return SEGMENT_SIZE;
			} else {
				// partial
				return (*this->size) % SEGMENT_SIZE;
			}
		}

	private:
		typedef std::array<T, SEGMENT_SIZE> segment_t;

		struct metadata_t {
			M data;
			int next;
		};

		std::mutex mutexSegments;
		std::mutex mutexMetadata;

		std::string name;
		std::shared_ptr<DBFile> file;
		long* size;
		std::unordered_map<long, segment_t*> segments;
		std::unordered_map<int, M> metadata;
		int* metadataFirst;
		int* metadataLast;
		int callbackID;

		static std::string genSizeID(std::string name) {
			std::stringstream buffer;
			buffer << "dims/" << name << "/size";
			return buffer.str();
		}

		static std::string genSegmentID(std::string name, long n) {
			std::stringstream buffer;
			buffer << "dims/" << name << "/segments/" << n;
			return buffer.str();
		}

		static std::string genMetadataFirstID(std::string name) {
			std::stringstream buffer;
			buffer << "dims/" << name << "/metadata/first";
			return buffer.str();
		}

		static std::string genMetadataLastID(std::string name) {
			std::stringstream buffer;
			buffer << "dims/" << name << "/metadata/last";
			return buffer.str();
		}

		static std::string genMetadataID(std::string name, int n) {
			std::stringstream buffer;
			buffer << "dims/" << name << "/metadata/" << n;
			return buffer.str();
		}

		void resetPtrs() {
			this->size = file->find_or_construct<long>(genSizeID(name), 0);
			this->metadataFirst = file->find_or_construct<int>(genMetadataFirstID(name), 0);
			this->metadataLast = file->find_or_construct<int>(genMetadataLastID(name), 0);

			// reset segment pointers
			this->segments.clear();
		}

		segment_t* getSegmentPtr(long segment) {
			std::lock_guard<std::mutex> lock(this->mutexSegments);
			segment_t* segmentPtr = nullptr;

			// get segment
			try {
				segmentPtr = this->segments.at(segment);
			} catch (std::out_of_range& e) {
				// segment isn't loaded
				std::string sID = genSegmentID(name, segment);

				// try to load segement
				auto lookup = file->find<segment_t>(sID);
				if (lookup.second == 1) {
					// found
					segmentPtr = lookup.first;
				} else {
					// new segment
					segmentPtr = file->construct<segment_t>(sID);
				}

				this->segments.emplace(segment, segmentPtr);
			}

			return segmentPtr;
		}
};

template <typename T, typename M>
class DimIter : public std::iterator<std::forward_iterator_tag, T> {
	public:
		DimIter(Dim<T,M>* dim, long pos, bool valid) : dim(dim), pos(pos) {
			if (valid) {
				segment = dim->getSegmentPtr(pos / Dim<T,M>::SEGMENT_SIZE);
			}
		};

		DimIter<T,M>& operator++() {
			pos++;
			if (pos % Dim<T,M>::SEGMENT_SIZE == 0) {
				segment = dim->getSegmentPtr(pos / Dim<T,M>::SEGMENT_SIZE);
			}
			return *this;
		}
		DimIter<T,M> operator++(int /*not used*/) {
			DimIter<T,M> clone(*this);
			pos++;
			if (pos % Dim<T,M>::SEGMENT_SIZE == 0) {
				segment = dim->getSegmentPtr(pos / Dim<T,M>::SEGMENT_SIZE);
			}
			return clone;
		}

		bool operator!=(const DimIter<T,M>& obj) {
			return this->pos != obj.pos;
		}

		T& operator*() {
			return (*segment)[pos % Dim<T,M>::SEGMENT_SIZE];
		}

	private:
		Dim<T,M>* dim;
		long pos;
		typename Dim<T,M>::segment_t* segment = nullptr;
};

typedef Dim<data_t, data_t> datadimobj_t;
typedef std::shared_ptr<datadimobj_t> datadim_t;

#endif

