cmake_minimum_required(VERSION 3.14)
project(OpenCLTestSuite CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCL
find_package(OpenCL REQUIRED)

# Include Google Test
add_subdirectory(third_party/googletest)

# Gather all test source files automatically
file(GLOB TEST_SOURCES "src/*.cpp")

# Define the test runner executable
add_executable(cl_test_runner ${TEST_SOURCES})

# Include directory for our test base headers
target_include_directories(cl_test_runner PRIVATE include)

# Link against Google Test (main) and OpenCL
target_link_libraries(cl_test_runner PRIVATE gtest_main OpenCL::OpenCL)

# Automatically discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(cl_test_runner)
