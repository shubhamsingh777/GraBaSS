#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <list>
#include <memory>
#include <string>

#include "dim.hpp"
#include "sys.hpp"

class Graph {
	public:
		Graph(std::shared_ptr<DBFile> file, std::string name);

		long getSize();

		bigid_t add(std::list<bigid_t> refs);
		std::list<bigid_t> get(bigid_t id);

	private:
		Dim<long> index;
		Dim<bigid_t> data;
};

typedef std::shared_ptr<Graph> graph_t;

#endif

