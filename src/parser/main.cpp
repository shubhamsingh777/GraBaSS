#include <iostream>

#include <boost/program_options.hpp>

#include "sys.hpp"
#include "parser.hpp"
#include "tracer.hpp"

#include "greycore/database.hpp"
#include "greycore/dim.hpp"

namespace gc = greycore;
namespace po = boost::program_options;

int main(int argc, char **argv) {
	// global config vars
	std::string cfgInput;
	std::string cfgDbData;
	bool cfgForce;

	// parse program options
	po::options_description poDesc("Options");
	poDesc.add_options()
		(
			"input",
			po::value(&cfgInput)->default_value("test.input"),
			"Input file"
		)
		(
			"dbdata",
			po::value(&cfgDbData)->default_value("columns.db"),
			"DB file that stores parsed data"
		)
		(
			"force",
			"Force to parse and progress data, ignores cache"
		)
		(
			"help",
			"Print this help message"
		)
	;
	po::variables_map poVm;
	try {
		po::store(po::parse_command_line(argc, argv, poDesc), poVm);
	} catch (const std::exception& e) {
		std::cout << "Error:" << std::endl
			<< e.what() << std::endl
			<< std::endl
			<< "Use --help to get help ;)" << std::endl;
		return EXIT_FAILURE;
	}
	po::notify(poVm);
	if (poVm.count("help")) {
		std::cout << "parser"<< std::endl << std::endl << poDesc << std::endl;
		return EXIT_SUCCESS;
	}
	cfgForce = poVm.count("force");

	// start time tracing
	std::stringstream timerProfile;

	std::cout << "Check system: " << std::flush;
	std::cout.precision(std::numeric_limits<double>::digits10 + 1); // set double precision
	checkConfig();
	std::cout << "done" << std::endl;

	{
		auto tMain = std::make_shared<Tracer>("main", &timerProfile);
		std::shared_ptr<Tracer> tPhase;

		std::cout << "Open DB and output file: " << std::flush;
		tPhase.reset(new Tracer("open", tMain));
		auto dbData = std::make_shared<gc::Database>(cfgDbData);
		std::cout << "done" << std::endl;

		std::cout << "Parse: " << std::flush;
		auto dimNameList = dbData->getIndexDims();
		std::vector<datadim_t> dims;
		if (!cfgForce && (dimNameList->getSize() > 0)) {
			for (size_t i = 0; i < dimNameList->getSize(); ++i) {
				auto name(std::get<0>((*dimNameList)[i].toTuple()));
				dims.push_back(dbData->getDim<data_t>(name));
			}
			std::cout << "skipped" << std::endl;
		} else {
			tPhase.reset(new Tracer("parse", tMain));
			ParseResult pr = parse(dbData, cfgInput);
			dims = pr.dims;
			std::cout << "done (" << pr.dims.size() << " columns, " << pr.nRows << " rows)" << std::endl;
		}

		std::cout << "Cleanup and Sync: " << std::flush;
		// end of block => free dims and db
	}
	std::cout << "done" << std::endl;

	std::cout << std::endl << "Time profile:" << std::endl << timerProfile.str() << std::endl;

	return EXIT_SUCCESS;
}

