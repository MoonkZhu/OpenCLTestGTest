#ifndef CL_TEST_BASE_HPP
#define CL_TEST_BASE_HPP

#include <gtest/gtest.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

class OpenCLTest : public ::testing::Test {
protected:
    std::string LoadKernelSource(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open kernel file: " + filename);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

#endif // CL_TEST_BASE_HPP
