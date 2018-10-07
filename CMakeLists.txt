cmake_minimum_required (VERSION 2.8.1)

project (smctc CXX)

if(NOT CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS "-std=c++0x")
endif()

find_package(GSL REQUIRED)
include_directories(BEFORE SYSTEM ${GSL_INCLUDE_DIRS})
link_directories(${GSL_LIBRARY_DIRS})

include_directories(include)

set(SMCTC_SOURCE_FILES
  src/history.cc
  src/rng.cc
  src/smc-exception.cc)

ADD_LIBRARY(smctc STATIC
            ${SMCTC_SOURCE_FILES})

file(GLOB HEADER_FILES include/*.hh)

install(TARGETS smctc DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)