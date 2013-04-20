#include "dim.hpp"
#include "parser.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

typedef shared_ptr<istream> stream_t;

string genDimName(int n) {
	stringstream buffer;
	buffer << n;
	return buffer.str();
}

void stripWhitespaces(stream_t s) {
	wchar_t c = s->get();

	while (c == ' ') {
		c = s->get();
	}

	s->unget();
}

bool stripEol(stream_t s) {
	bool result = false;
	wchar_t c = s->get();

	while ((c == '\r') || (c == '\n')) {
		result = true;
		c = s->get();
	}

	s->unget();
	return result;
}

double parseDouble(stream_t s) {
	stringstream tmp;
	wchar_t c = s->get();

	while (((c >= '0') && (c <= '9')) || (c == '.') || (c == '-')) {
		tmp << c;
		c = s->get();
	}

	s->unget();

	double result;
	tmp >> result;
	return result;
}

ParseResult parse(shared_ptr<DBFile> target, string fname) {
	ParseResult result;
	result.nRows = 0;

	auto stream = make_shared<ifstream>(fname);

	while (stream->peek() != char_traits<char>::eof()) {
		auto begin = stream->tellg();
		size_t n = 0;

		while (!stripEol(stream)) {
			double d = parseDouble(stream);

			if (result.nRows == 0) {
				// create dimension
				string dName = genDimName(n);
				result.dims.push_back(datadim_t(new datadimobj_t(target, dName)));

				// log
				if (n % 100 == 0) {
					cout << "+" << flush;
				}
			}

			if (n < result.dims.size()) {
				result.dims[n]->add(d);
			} else {
				cout << "(row " << result.nRows << "too long)" << flush;
			}

			stripWhitespaces(stream);
			++n;
		}

		++result.nRows;

		// log
		if (result.nRows % 100 == 0) {
			cout << result.nRows << flush;
		} else if (result.nRows % 10 == 0) {
			cout << "." << flush;
		}

		if (stream->tellg() == begin) {
			cout << "(stop@" << begin << ")" << flush;
			break;
		}
	}

	return result;
}

