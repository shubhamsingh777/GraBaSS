#include "dbfile.hpp"

using namespace boost::interprocess;
using namespace std;

DBFile::DBFile(string fname) :
		name(fname),
		mfile(new mfile_t(open_or_create, name.c_str(), GROW_SIZE)) {
	assert(mfile->check_sanity());
}

int DBFile::registerPResetFun(pResetFun_t fun) {
	int id = this->idCounter++;
	this->resetFuns[id] = fun;
	return id;
}

void DBFile::unregisterPResetFun(int id) {
	this->resetFuns.erase(id);
}

void DBFile::grow() {
	// unmap
	mfile_t* ptr = mfile.get();
	mfile.release();
	delete ptr;

	// grow
	managed_mapped_file::grow(name.c_str(), GROW_SIZE);

	// remap
	ptr = new mfile_t(boost::interprocess::open_only, name.c_str());
	mfile.reset(ptr);

	// reset all pointers
	for (auto f : this->resetFuns) {
		f.second();
	}
}

