#include <boost/interprocess/mapped_region.hpp>
#include <iostream>

#include "sys.hpp"

namespace bi = boost::interprocess;

template <typename T>
bool check(std::string name, T is, T should) {
	if (is != should) {
		std::cout << "Warning: " << name << " should be " << should << " (current config: " << is << ")" << std::endl;
		return false;
	} else {
		return true;
	}
}

bool checkConfig() {
	bool result = true;

	result &= check("pageSize", PAGE_SIZE, bi::mapped_region::get_page_size());

	return result;
}

