#include <boost/interprocess/mapped_region.hpp>
#include <iostream>

#include "sys.hpp"

using namespace boost::interprocess;
using namespace std;

template <typename T>
bool check(string name, T is, T should) {
	if (is != should) {
		cout << "Warning: " << name << " should be " << should << " (current config: " << is << ")" << endl;
		return false;
	} else {
		return true;
	}
}

bool Sys::checkConfig() {
	bool result = true;

	result &= check("pageSize", Sys::PAGE_SIZE, mapped_region::get_page_size());

	return result;
}

