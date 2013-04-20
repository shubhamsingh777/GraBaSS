#ifndef PARSER_HPP
#define PERSER_HPP

#include <memory>
#include <string>
#include <vector>

#include "dbfile.hpp"
#include "dim.hpp"

struct ParseResult {
	std::vector<datadim_t> dims;
	long nRows;
};

ParseResult parse(std::shared_ptr<DBFile> target, std::string fname);

#endif

