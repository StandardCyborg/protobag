cmake_minimum_required(VERSION 3.10)
project(MyWriter C CXX)

set(CMAKE_CXX_STANDARD 17)


## Dependencies
find_package(Protobuf)
include_directories(${PROTOBUF_INCLUDE_DIRS})

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROTOBAG_HAVE_LIBARCHIVE")

find_package(fmt REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY")
  # NB: https://github.com/fmtlib/fmt/issues/524

set(dep_libs pthread m atomic)
set(dep_libs ${dep_libs} ${LibArchive_LIBRARIES})
set(dep_libs ${dep_libs} ${PROTOBUF_LIBRARIES})
set(dep_libs ${dep_libs} fmt::fmt-header-only)
set(dep_libs ${dep_libs} protobag)
if(UNIX OR APPLE)
  set(dep_libs ${dep_libs} c++fs)
endif()


## Executable my_writer

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -stdlib=libc++")

add_executable(
  my_writer
  MyWriter.cpp
  MyMessages.pb.h
  MyMessages.pb.cc)

target_link_libraries(
  my_writer
  PRIVATE
  ${dep_libs})
