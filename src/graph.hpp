#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <list>
#include <memory>
#include <string>

#include "dim.hpp"

class Graph {
	public:
		Graph(std::shared_ptr<DBFile> file, std::string name);

		long getSize();

		long add(std::list<long> refs);
		std::list<long> get(long id);

	private:
		Dim<long> index;
		Dim<long> data;
};

#endif

