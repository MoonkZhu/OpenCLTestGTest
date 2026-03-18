cmake_minimum_required(VERSION 3.14)
project(OpenCLTestSuite CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCL
find_package(OpenCL REQUIRED)

# Include Google Test
add_subdirectory(third_party/googletest)

# Copy the kernel directory to the build directory so tests can find them easily
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernel DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Include Google Test
include(GoogleTest)

# 1. Base test runner for .cpp files directly in src/
file(GLOB BASE_TEST_SOURCES "src/*.cpp")
if(BASE_TEST_SOURCES)
    add_executable(cl_test_runner ${BASE_TEST_SOURCES})
    target_include_directories(cl_test_runner PRIVATE include)
    target_link_libraries(cl_test_runner PRIVATE gtest_main OpenCL::OpenCL)
    gtest_discover_tests(cl_test_runner)
endif()

# 2. Automatically discover subdirectories in src/ and create an executable for each
file(GLOB SRC_SUBDIRS LIST_DIRECTORIES true "src/*")
foreach(SUBDIR ${SRC_SUBDIRS})
    if(IS_DIRECTORY ${SUBDIR})
        get_filename_component(SUBDIR_NAME ${SUBDIR} NAME)
        file(GLOB SUBDIR_TEST_SOURCES "${SUBDIR}/*.cpp")

        if(SUBDIR_TEST_SOURCES)
            set(EXEC_NAME "cl_test_${SUBDIR_NAME}")
            add_executable(${EXEC_NAME} ${SUBDIR_TEST_SOURCES})
            target_include_directories(${EXEC_NAME} PRIVATE include)
            target_link_libraries(${EXEC_NAME} PRIVATE gtest_main OpenCL::OpenCL)
            gtest_discover_tests(${EXEC_NAME})
        endif()
    endif()
endforeach()
