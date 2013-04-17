def options(opt):
	opt.load('compiler_cxx')
	group = opt.add_option_group('project options')
	group.add_option('--debug', action="store_true", default=False, help='Build (slow) debug/profile version')

def configure(conf):
	conf.load('compiler_cxx')
	conf.env.append_unique('CXXFLAGS', ['-std=c++0x', '-Wall', '-Wextra'])

	# debug vs. optimize
	if conf.options.debug:
		conf.env.append_unique('CXXFLAGS', ['-g'])
	else:
		conf.env.append_unique('CXXFLAGS', ['-O3', '-ffast-math'])

	# check for clang++
	if conf.env['CXX'][0].endswith('clang++'):
		# nice colored output
		conf.env.append_unique('CXXFLAGS', ['-fcolor-diagnostics', '-fdiagnostics-show-category=name'])

		# fix -fast-math compile error
		# see http://llvm.org/bugs/show_bug.cgi?id=13745
		conf.env.append_unique('CXXFLAGS', ['-D__extern_always_inline=extern __always_inline']);

def build(bld):
	bld.program(
		lib = [
			'pthread',
			'tbb'],
		source = [
			'src/dbfile.cpp',
			'src/clusterer.cpp',
			'src/graph.cpp',
			'src/main.cpp',
			'src/parser.cpp',
			'src/sys.cpp'],
		stlib = ['boost_iostreams'],
		target='hugeDim')

