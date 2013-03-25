#ifndef PARSER_HPP
#define PERSER_HPP

#include <memory>
#include <string>
#include <vector>

#include "dbfile.hpp"

typedef Dim<double, double> parserDim_t;

struct ParseResult {
	std::vector<std::shared_ptr<parserDim_t>> dims;
	long nRows;
};

ParseResult parse(std::shared_ptr<DBFile> target, std::string fname);

#endif

