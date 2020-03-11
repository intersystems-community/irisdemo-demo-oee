

PROJECT(mocksvr)

cmake_minimum_required (VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

include_directories(${PROJECT_SOURCE_DIR})
add_executable(opcua-sampler OPCUANodeDataSource.cpp datasource.cpp main.cpp open62541.c)  

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")



