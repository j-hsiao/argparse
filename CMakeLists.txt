cmake_minimum_required(VERSION 3.0)
project(argparse VERSION 0.4 LANGUAGES CXX)
add_library(
	${PROJECT_NAME}
	src/argparse.cpp
	src/parse.cpp
	src/argiter.cpp
	src/nums.cpp
)
target_include_directories(
	${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include>)

install(
	DIRECTORY include/${PROJECT_NAME}
	DESTINATION include
)
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(
	EXPORT ${PROJECT_NAME}
	FILE ${PROJECT_NAME}Config.cmake
	DESTINATION share/cmake/${PROJECT_NAME}
	NAMESPACE ${PROJECT_NAME}::
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${CMAKE_BINARY_DIR}/share/cmake/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake
	COMPATIBILITY SameMajorVersion
)
export(
	EXPORT ${PROJECT_NAME}
	FILE ${CMAKE_BINARY_DIR}/share/cmake/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake
	NAMESPACE ${PROJECT_NAME}::
)
install(
	FILES
	${CMAKE_BINARY_DIR}/share/cmake/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake
	DESTINATION share/cmake/${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(CTest)
if (BUILD_TESTING)
	add_executable(demo test/demo.cpp)
	target_link_libraries(demo PUBLIC ${PROJECT_NAME})

	add_executable(parse test/parse.cpp)
	target_link_libraries(parse PUBLIC ${PROJECT_NAME})
	add_test(NAME parse COMMAND parse)

	add_executable(print test/print.cpp)
	target_include_directories(print PUBLIC include)
	add_test(NAME print COMMAND print)

	add_executable(nums test/nums.cpp)
	target_link_libraries(nums PUBLIC ${PROJECT_NAME})
	add_test(NAME nums COMMAND nums)

	add_executable(arg test/arg.cpp)
	target_link_libraries(arg PUBLIC ${PROJECT_NAME})
	add_test(NAME arg COMMAND arg)
	target_include_directories(arg PRIVATE test/include)

	add_executable(argiter test/argiter.cpp)
	target_link_libraries(argiter PUBLIC ${PROJECT_NAME})
	add_test(NAME argiter COMMAND argiter)
	target_include_directories(argiter PRIVATE test/include)

	add_executable(${PROJECT_NAME}_test test/argparse.cpp)
	target_link_libraries(argparse_test PUBLIC ${PROJECT_NAME})
	add_test(NAME argparse COMMAND argparse_test)
	target_include_directories(argparse_test PRIVATE test/include)
endif()
