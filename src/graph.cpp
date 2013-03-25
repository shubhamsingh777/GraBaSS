#include <cassert>
#include <sstream>

#include "graph.hpp"

using namespace std;

string genIndexName(string name) {
	stringstream buffer;
	buffer << name << ".graph.index";
	return buffer.str();
}

string genDataName(string name) {
	stringstream buffer;
	buffer << name << ".graph.data";
	return buffer.str();
}

Graph::Graph(shared_ptr<DBFile> file, string name) :
		index(file, genIndexName(name)),
		data(file, genDataName(name)) {
	if (index.getSize() == 0) {
		assert(data.getSize() == 0);
		index.add(0);
	}
}

long Graph::getSize() {
	return index.getSize() - 1;
}

long Graph::add(list<long> refs) {
	for (auto r : refs) {
		data.add(r);
	}

	index.add(data.getSize());

	return index.getSize() - 2;
}

list<long> Graph::get(long id) {
	long begin = index.get(id);
	long end = index.get(id + 1);

	return list<long>(data.iterAt(begin), data.iterAt(end));
}

