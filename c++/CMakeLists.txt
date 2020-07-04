cmake_minimum_required(VERSION 3.10)
project(Protobag C CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
link_libraries(m atomic)

set(EXEC_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}"
  CACHE PATH "Base installation path for executables.")
set(INSTALL_BIN_DIR "${EXEC_INSTALL_PREFIX}/bin"
  CACHE PATH "Installation directory for binaries (default: prefix/bin).")
set(LIB_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/lib"
  CACHE PATH "Installation directory for libraries (default: prefix/lib).")
set(INSTALL_INCLUDE_DIR "${EXEC_INSTALL_PREFIX}/include"
  CACHE PATH
  "Installation directory for header files (default: prefix/include).")

include_directories(
  "${PROJECT_SOURCE_DIR}/protobag"
  "${PROJECT_BINARY_DIR}")

set(protobag_common_flags "-Wall -std=c++17 -stdlib=libc++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${protobag_common_flags}")
if(UNIX OR APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${protobag_common_flags} -g3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${protobag_common_flags} -O3")

file(STRINGS "protobag_version.txt" PROTOBAG_VERSION)
set(protobag_common_flags "-DPROTOBAG_VERSION=${PROTOBAG_VERSION}")

###
### Dependencies
###

# To use local install, set GTEST_ROOT=/path/to/gtest
set(GTEST_ROOT "/opt/gtest" CACHE PATH "Path to googletest")
set(GTEST_INCLUDE_DIR "/opt/gtest/googletest/include" CACHE PATH "Path to googletest includes")
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(Protobuf)
include_directories(${PROTOBUF_INCLUDE_DIRS})

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROTOBAG_HAVE_LIBARCHIVE")

find_package(fmt REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY")
  # NB: https://github.com/fmtlib/fmt/issues/524


###
### Library libprotobag
###

file(
  GLOB_RECURSE protobag_headers
  protobag/*.hpp
  protobag/*.h)

file(
  GLOB_RECURSE protobag_srcs
  protobag/*.hpp
  protobag/*.cpp
  protobag/*.h)

set(
  protobag_dep_libs
  pthread)

file(GLOB_RECURSE pb_headers protobag/protobag_msg/*.pb.h)
set(protobag_headers ${protobag_headers} ${pb_headers})

file(GLOB_RECURSE pb_srcs protobag/protobag_msg/*.pb.cc)
set(protobag_srcs ${protobag_srcs} ${pb_srcs})

set(protobag_dep_libs ${protobag_dep_libs} ${LibArchive_LIBRARIES})
set(protobag_dep_libs ${protobag_dep_libs} ${PROTOBUF_LIBRARIES})
set(protobag_dep_libs ${protobag_dep_libs} fmt::fmt-header-only)

if(UNIX OR APPLE)
  set(protobag_dep_libs ${protobag_dep_libs} c++fs)
endif()

add_library(protobag SHARED ${protobag_srcs})
add_library(protobagStatic STATIC ${protobag_srcs})
set_target_properties(protobagStatic PROPERTIES OUTPUT_NAME protobag)

target_link_libraries(
  protobag
  PRIVATE
  ${protobag_dep_libs})

install(
  TARGETS protobag protobagStatic
  ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
  LIBRARY DESTINATION "${LIB_INSTALL_DIR}")

file(
  GLOB_RECURSE protobag_proto_headers
  protobag/protobag_msg/*.h)
install(
  FILES ${protobag_proto_headers}
  DESTINATION "${INSTALL_INCLUDE_DIR}/protobag_msg")

file(
  GLOB_RECURSE pb_protos
  protobag/protobag_msg/*.proto)
install(
  FILES ${pb_protos}
  DESTINATION "${INSTALL_INCLUDE_DIR}/protobag_msg")


# Install all main protobag headers, maintaining source tree format
install(
  DIRECTORY protobag/
  DESTINATION "${INSTALL_INCLUDE_DIR}"
  FILES_MATCHING PATTERN "*.hpp")


###
### Python Module (via pybind11) protobag_native
###

find_package(pybind11 REQUIRED)
pybind11_add_module(
  protobag_native
  SHARED
  ${protobag_srcs}
  protobag_native/protobag_native.cpp)
target_link_libraries(
  protobag_native
  PRIVATE
  ${protobag_dep_libs})
add_test(
  NAME test_protobag_native
  COMMAND bash -c "python3 -c 'import protobag_native; print(protobag_native.foo())'") # TODO make this a selftest!!


###
### Executable protobag_test
###

set(
  protobag_test_dep_libs
  ${protobag_dep_libs}
  ${GTEST_BOTH_LIBRARIES})

file(
  GLOB_RECURSE protobag_test_srcs
  protobag_test/protobag/*.hpp
  protobag_test/protobag/*.cpp
  protobag_test/protobag_test/*.hpp
  protobag_test/protobag_test/*.cpp)

file(GLOB_RECURSE pb_headers protobag_test/protobag_test_msg/*.pb.h)
set(protobag_test_srcs ${protobag_test_srcs} ${pb_headers})

file(GLOB_RECURSE pb_srcs protobag_test/protobag_test_msg/*.pb.cc)
set(protobag_test_srcs ${protobag_test_srcs} ${pb_srcs})

set(protobag_test_dep_libs ${protobag_test_dep_libs} ${PROTOBUF_LIBRARIES})

add_executable(
  protobag_test
  ${protobag_test_srcs})

set_property(
  TARGET
  protobag_test
  APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/protobag_test")

# Tell `protobuf_test` where to find fixtures by default
set(
  PROTOBAG_TEST_DEFAULT_FIXTURES_DIR
  "${PROJECT_SOURCE_DIR}/protobag_test/fixtures")
target_compile_definitions(
  protobag_test
  PRIVATE
  -DPROTOBAG_TEST_DEFAULT_FIXTURES_DIR="${PROTOBAG_TEST_DEFAULT_FIXTURES_DIR}" )
  
target_link_libraries(
  protobag_test
  PRIVATE
  protobagStatic
  ${protobag_test_dep_libs})

add_test(
    NAME test 
    COMMAND bash -c "$<TARGET_FILE:protobag_test>")
set_tests_properties(test PROPERTIES DEPENDS protobag_test)
