cmake_minimum_required(VERSION 3.0.0)
project(htmlparser VERSION 0.1.0)

include(CTest)
enable_testing()

#add_library(htmlparser htmlparser.cpp)
#add_library(mbsconverter mbsconverter.cpp)
add_executable(main htmlparser.cpp mbsconverter.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
