cmake_minimum_required (VERSION 2.8)

project (hugeDim)

# configure compiler
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wconversion -Wsign-conversion")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math")

# configure linker (rpath)
set (CMAKE_SKIP_BUILD_RPATH FALSE)
set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set (CMAKE_INSTALL_RPATH "$ORIGIN/lib")

# detect clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -fdiagnostics-show-category=name")
endif ()

# search boost
set (Boost_USE_STATIC_LIBS ON)
find_package (Boost COMPONENTS iostreams program_options)

# set include dirs
include_directories (${Boost_INCLUDE_DIR})

# add extern libs
add_subdirectory ("extern/greycore")
include_directories ("extern/greycore/include")

# build/install hugeDim
aux_source_directory ("src" HGSourceFiles)
add_executable (hugeDim ${HGSourceFiles})
target_link_libraries (hugeDim pthread tbb ${Boost_LIBRARIES} greycore)

