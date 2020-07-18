cmake_minimum_required(VERSION 3.12)
project(SSTTransports C CXX)

cmake_policy(SET CMP0074 NEW)
find_package(SSTMacro REQUIRED)

add_subdirectory(ibverbs)
add_subdirectory(pmi)
add_subdirectory(ugni)
add_subdirectory(libfabric)
