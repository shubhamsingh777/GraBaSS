#ifndef DBFILE_HPP
#define DBFILE_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/interprocess/managed_mapped_file.hpp>

class DBFile {
	public:
		typedef boost::interprocess::managed_mapped_file mfile_t;
		typedef std::function<void()> pResetFun_t;

		static constexpr int GROW_SIZE = 64 * 1024 * 1024; // 64 MB

		DBFile(std::string fname);

		int registerPResetFun(pResetFun_t fun);
		void unregisterPResetFun(int id);

		void grow();

		template <typename T>
		std::pair<T*, std::size_t> find(std::string id) {
			return this->mfile->find<T>(id.c_str());
		}

		template <typename T, typename... A>
		T* construct(std::string id, A&&... args) {
			T* ptr = nullptr;

			do {
				ptr = this->mfile->construct<T>(id.c_str(), std::nothrow)(std::forward<A>(args)...);
				if (ptr == nullptr) {
					this->grow();
				}
			} while (ptr == nullptr);

			return ptr;
		}

		template <typename T, typename... A>
		T* find_or_construct(std::string id, A&&... args) {
			T* ptr = nullptr;

			do {
				ptr = this->mfile->find_or_construct<T>(id.c_str(), std::nothrow)(std::forward<A>(args)...);
				if (ptr == nullptr) {
					this->grow();
				}
			} while (ptr == nullptr);

			return ptr;
		}

		template <typename T>
		using allocator = boost::interprocess::allocator<T, boost::interprocess::managed_mapped_file::segment_manager>;

		template <typename T>
		allocator<T> getAllocator() {
			return allocator<T>(this->mfile->get_segment_manager());
		}

	private:
		std::string name;
		std::unique_ptr<mfile_t> mfile;
		std::unordered_map<int, pResetFun_t> resetFuns;
		int idCounter = 0;
};

#endif

