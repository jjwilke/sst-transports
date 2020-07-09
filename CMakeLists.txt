cmake_minimum_required(VERSION 3.12)
project(SSTTransports C CXX)

find_package(SSTMacro REQUIRED)

add_subdirectory(ibverbs)
add_subdirectory(ugni)
add_subdirectory(libfabric)
