cmake_minimum_required(VERSION 3.5...3.21)
project(mehh VERSION 0.1.0 LANGUAGES C CXX)

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

set (MEHH_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set (MEHH_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/tests)

set (MEHH_SRC 
${MEHH_SRC_DIR}/main.cpp
${MEHH_SRC_DIR}/chunk.cpp
${MEHH_SRC_DIR}/debug.cpp
)

set (MEHH_TESTS 
${MEHH_TESTS_DIR}/test.cpp
${MEHH_SRC_DIR}/chunk.cpp
${MEHH_SRC_DIR}/debug.cpp
)

add_executable(mehh ${MEHH_SRC})
add_executable(mehh_test ${MEHH_TESTS})

target_include_directories(mehh PUBLIC ${MEHH_SRC_DIR})
target_include_directories(mehh_test PUBLIC ${MEHH_SRC_DIR})

target_link_libraries(
  mehh_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mehh_test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)