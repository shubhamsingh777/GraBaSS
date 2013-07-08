#ifndef TRACER_HPP
#define TRACER_HPP

#include <chrono>
#include <memory>
#include <ostream>
#include <string>

class Tracer {
	public:
		Tracer(std::string name, std::ostream* out);
		Tracer(std::string name, std::shared_ptr<Tracer> parent);
		~Tracer();

		std::string getPath();

	private:
		typedef std::chrono::time_point<std::chrono::steady_clock> timePoint_t;

		std::string name;
		std::ostream* out;
		timePoint_t begin;
		std::shared_ptr<Tracer> parent;
};

#endif

