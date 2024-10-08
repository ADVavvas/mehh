cmake_minimum_required(VERSION 3.5...3.21)
project(mehh VERSION 0.1.0 LANGUAGES C CXX)

set(CXXFLAGS  "-fno-rtti -fno-omit-frame-pointer -foptimize-sibling-calls")
set(CMAKE_CXX_FLAGS "${CXXFLAGS}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571 # 1.14
)
FetchContent_MakeAvailable(googletest)

set(Boost_USE_MULTITHREADED OFF)
set(BOOST_INCLUDE_LIBRARIES container unordered)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.85.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Boost)

FetchContent_Declare(
  abseil-cpp
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        4447c7562e3bc702ade25105912dce503f0c4010 # 1.14
)
FetchContent_MakeAvailable(abseil-cpp)

# Tracy (consider using CMake like GTest)
# add_compile_definitions(TRACY_ENABLE)
set(TRACY_SRC_DIR
${CMAKE_CURRENT_SOURCE_DIR}/profiling/tracy/public
)

set(TRACY_INC
${CMAKE_CURRENT_SOURCE_DIR}/profiling/tracy/public/tracy
)

set (MEHH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set (MEHH_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set (MEHH_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/tests)

set (MEHH_SRC 
${TRACY_SRC_DIR}/TracyClient.cpp
${MEHH_SRC_DIR}/chunk.cpp
${MEHH_SRC_DIR}/compiler.cpp
${MEHH_SRC_DIR}/debug.cpp
${MEHH_SRC_DIR}/main.cpp
${MEHH_SRC_DIR}/mehh.cpp
${MEHH_SRC_DIR}/scanner.cpp
${MEHH_SRC_DIR}/value.cpp
${MEHH_SRC_DIR}/vm.cpp
${MEHH_SRC_DIR}/function.cpp
${MEHH_SRC_DIR}/call_frame.cpp
)

set (MEHH_TESTS 
${TRACY_SRC_DIR}/TracyClient.cpp
${MEHH_TESTS_DIR}/nanbox.cpp
${MEHH_SRC_DIR}/chunk.cpp
${MEHH_SRC_DIR}/compiler.cpp
${MEHH_SRC_DIR}/debug.cpp
${MEHH_SRC_DIR}/mehh.cpp
${MEHH_SRC_DIR}/scanner.cpp
${MEHH_SRC_DIR}/value.cpp
${MEHH_SRC_DIR}/vm.cpp
${MEHH_SRC_DIR}/function.cpp
${MEHH_SRC_DIR}/call_frame.cpp
)

add_subdirectory(external/fmt)

add_executable(mehh ${MEHH_SRC})
add_executable(mehh_test ${MEHH_TESTS})

message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

target_include_directories(mehh PUBLIC ${MEHH_INC_DIR} ${TRACY_INC} ${Boost_INCLUDE_DIRS})
target_include_directories(mehh_test PUBLIC ${MEHH_INC_DIR} ${TRACY_INC} ${Boost_INCLUDE_DIRS})

target_link_libraries(
  mehh
  fmt::fmt
  Boost::container
  Boost::unordered
  absl::flat_hash_map
)

target_link_libraries(
  mehh_test
  GTest::gtest_main
  fmt::fmt
  Boost::container
  Boost::unordered
  absl::flat_hash_map
)

include(GoogleTest)
gtest_discover_tests(mehh_test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
