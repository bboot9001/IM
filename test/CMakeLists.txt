cmake_minimum_required (VERSION 3.0)
INCLUDE_DIRECTORIES(../im/src/base)
AUX_SOURCE_DIRECTORY(../im/src/base DIR_BASE_SRCS)
add_executable(hello hello.cpp ${DIR_BASE_SRCS})