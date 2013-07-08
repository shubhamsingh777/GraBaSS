#include "greycore/dim.hpp"
#include "parser.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/spirit/include/qi.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace gc = greycore;
namespace qi = boost::spirit::qi;

using namespace boost::iostreams;
using namespace std;

string genDimName(std::size_t n) {
	stringstream buffer;
	buffer << n;
	return buffer.str();
}

ParseResult parse(shared_ptr<gc::Database> target, string fname) {
	ParseResult result;
	result.nRows = 0;

	mapped_file file(fname);
	auto first = file.begin();
	auto last = file.end();

	std::size_t n = 0;
	int start = 2;

	auto funRest = [&](double i){
		if (start > 0) {
			// create dimension
			string dName = genDimName(n);
			result.dims.push_back(target->createDim<data_t>(dName));

			// log
			if (n % 100 == 0) {
				cout << "+" << flush;
			}
		}
		result.dims[n]->add(i);

		++n;
	};

	auto funBegin = [&](double i){
		// set start value (used for Dim creation)
		if (start > 0) {
			--start;
		}

		// log
		if (result.nRows % 100 == 0) {
			cout << result.nRows << flush;
		} else if (result.nRows % 10 == 0) {
			cout << "." << flush;
		}

		// set values for new row
		n = 0;
		++result.nRows;

		// pass to normal parse function
		funRest(i);
	};

	auto grammar = (qi::double_[funBegin] >> *(qi::space >> qi::double_[funRest])) % qi::no_skip[qi::eol];
	qi::phrase_parse(first, last, grammar, qi::eol);

	return result;
}

