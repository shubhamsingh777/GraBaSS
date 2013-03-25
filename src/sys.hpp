#ifndef SYS_HPP
#define SYS_HPP

#include <cstddef>

class Sys {
	public:
		static constexpr std::size_t PAGE_SIZE = 4096;

		Sys() = delete;
		Sys(const Sys& obj) = delete;
		~Sys() = delete;

		static bool checkConfig();
};

#endif

