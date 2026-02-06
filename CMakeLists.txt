cmake_minimum_required(VERSION 3.16)
project(CMakeProjectTemplate VERSION 1.0.0 LANGUAGES C CXX)

include(cmake/build_option.cmake)
include(cmake/compile_setting.cmake)

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(test)

include(cmake/install.cmake)
include(cmake/cpack.cmake)
