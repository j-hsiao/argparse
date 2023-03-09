cmake_minimum_required(VERSION 3.0)
project(argparse VERSION 0.2 LANGUAGES CXX)
add_library(argparse INTERFACE)
target_include_directories(
	argparse
	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include>)
add_executable(demo EXCLUDE_FROM_ALL test/test.cpp)
target_link_libraries(demo PUBLIC argparse)

install(
	DIRECTORY include/argparse
	DESTINATION include
)
install(
	TARGETS argparse
	EXPORT argparseTargets
)
install(
	EXPORT argparseTargets
	FILE argparseTargets.cmake
	DESTINATION share/cmake/argparse
	NAMESPACE argparse::
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	argparse/argparseConfigVersion.cmake
	COMPATIBILITY SameMajorVersion
)
if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/argparse/argparseConfig.cmake")
	file(
		WRITE "${CMAKE_CURRENT_BINARY_DIR}/argparse/argparseConfig.cmake"
		"include(\"\${CMAKE_CURRENT_LIST_DIR}/argparseTargets.cmake\")\n")
endif()
export(
	EXPORT argparseTargets
	FILE argparse/argparseTargets.cmake
	NAMESPACE argparse::
)
file(GLOB packagefiles "${CMAKE_CURRENT_BINARY_DIR}/argparse/argparseConfig*.cmake")
install(
	FILES
	${packagefiles}
	DESTINATION share/cmake/argparse)
