cmake_minimum_required(VERSION 3.0)
project(argparse VERSION 0.1 LANGUAGES CXX)
add_library(argparse INTERFACE)
target_include_directories(
	argparse
	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include>)
add_executable(demo test/test.cpp)
target_link_libraries(demo PUBLIC argparse)
