#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include "tracer.hpp"

using namespace std;

string escapeName(string s) {
	// complicated, because regex does not work yet
	string legal("abcdefghijklmnopqrstuvwxyzABCDEFGHIJLMNOPQRSTUVWXYZ0123456789");
	stringstream result;
	for (auto c : s) {
		if (count(legal.begin(), legal.end(), c) > 0) {
			result << c;
		}
	}
	return result.str();
}

Tracer::Tracer(string name_, ostream* out_) :
		name(escapeName(name_)),
		out(out_),
		begin(chrono::steady_clock::now()) {}

Tracer::Tracer(string name_, shared_ptr<Tracer> parent_) :
		name(escapeName(name_)),
		out(parent_->out),
		begin(chrono::steady_clock::now()),
		parent(parent_) {}

Tracer::~Tracer() {
	timePoint_t end = chrono::steady_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - this->begin).count();

	stringstream ss;
	ss << fixed;
	ss << setprecision(2);
	ss << this->getPath() << ": " << (duration / 1000.0) << " seconds" << endl;

	(*out) << ss.str();
}

string Tracer::getPath() {
	stringstream ss;
	if (parent) {
		ss << parent->getPath();
		ss << "/";
	}
	ss << this->name;
	return ss.str();
}

