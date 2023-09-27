cmake_minimum_required(VERSION 3.0)
project(arguse)

find_package(argparse)
message("${argparse_FOUND}")


add_executable(arguse ./x.cpp)
target_link_libraries(arguse PUBLIC argparse::argparse)
