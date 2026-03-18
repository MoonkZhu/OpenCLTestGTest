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
    cl_platform_id platform_id = nullptr;
    cl_device_id device_id = nullptr;
    cl_context context = nullptr;
    cl_command_queue command_queue = nullptr;

    void SetUp() override {
        cl_int err;
        
        // 1. Get OpenCL platform
        cl_uint num_platforms;
        err = clGetPlatformIDs(1, &platform_id, &num_platforms);
        ASSERT_EQ(err, CL_SUCCESS) << "Failed to find any OpenCL platforms.";
        ASSERT_GT(num_platforms, 0) << "No OpenCL platforms available.";

        // 2. Get OpenCL device
        cl_uint num_devices;
        err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &num_devices);
        ASSERT_EQ(err, CL_SUCCESS) << "Failed to find any OpenCL devices.";
        ASSERT_GT(num_devices, 0) << "No OpenCL devices available.";

        // 3. Create OpenCL context
        context = clCreateContext(nullptr, 1, &device_id, nullptr, nullptr, &err);
        ASSERT_EQ(err, CL_SUCCESS) << "Failed to create OpenCL context.";
        ASSERT_NE(context, nullptr) << "OpenCL context is null.";

        // 4. Create OpenCL command queue
#ifdef CL_VERSION_2_0
        command_queue = clCreateCommandQueueWithProperties(context, device_id, nullptr, &err);
#else
        command_queue = clCreateCommandQueue(context, device_id, 0, &err);
#endif
        ASSERT_EQ(err, CL_SUCCESS) << "Failed to create OpenCL command queue.";
        ASSERT_NE(command_queue, nullptr) << "OpenCL command queue is null.";
    }

    void TearDown() override {
        if (command_queue) {
            clReleaseCommandQueue(command_queue);
            command_queue = nullptr;
        }
        if (context) {
            clReleaseContext(context);
            context = nullptr;
        }
    }

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
