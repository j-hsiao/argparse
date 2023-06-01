cmake_minimum_required(VERSION 3.0)
project(argparse VERSION 0.3 LANGUAGES CXX)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
	${PROJECT_NAME}
	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include>)

install(
	DIRECTORY include/${PROJECT_NAME}
	DESTINATION include
)
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}
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
	add_executable(demo test/test.cpp)
	target_link_libraries(demo PUBLIC ${PROJECT_NAME})

	add_executable(nums test/nums.cpp)
	target_link_libraries(nums PUBLIC ${PROJECT_NAME})

	add_executable(arg test/arg.cpp)
	target_link_libraries(arg PUBLIC ${PROJECT_NAME})
	add_test(NAME arg COMMAND arg)
	target_include_directories(arg PRIVATE test/include)

	add_executable(argiter test/argiter.cpp)
	target_link_libraries(argiter PUBLIC ${PROJECT_NAME})
	add_test(NAME argiter COMMAND argiter)
	target_include_directories(argiter PRIVATE test/include)
endif()
