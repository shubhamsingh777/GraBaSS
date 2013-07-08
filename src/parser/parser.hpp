#ifndef PARSER_HPP
#define PARSER_HPP

#include <memory>
#include <string>
#include <vector>

#include "sys.hpp"

#include "greycore/database.hpp"
#include "greycore/dim.hpp"

struct ParseResult {
	std::vector<datadim_t> dims;
	long nRows;
};

ParseResult parse(std::shared_ptr<greycore::Database> target, std::string fname);

#endif

