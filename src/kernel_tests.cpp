#include "cl_test_base.hpp"
#include <vector>

// Use TEST_P for parameterized testing
class OpenCLParameterizedTest : public OpenCLTest, public ::testing::WithParamInterface<size_t> {
protected:
    std::vector<int> goldenData;

    // We can use CpuReference to generate our golden data baseline
    void CpuReference() override {
        size_t elements = GetParam();
        goldenData.resize(elements);
        for (size_t i = 0; i < elements; ++i) {
            int a = i;
            int b = i * 2;
            goldenData[i] = a + b;
        }
    }
};

TEST_P(OpenCLParameterizedTest, KernelExecution) {
    // 0. Initialize OpenCL (Hybrid Approach: use the new standard initialization)
    InitStandard();

    // Generate Golden Data CPU Reference
    CpuReference();

    // Get the parameter injected for this test instance
    size_t elements = GetParam();
    cl_int err;

    // 1. Load kernel source code
    std::string kernel_source;
    ASSERT_NO_THROW({
        kernel_source = LoadKernelSource("kernel/simple_add.cl");
    }) << "Failed to load kernel source.";

    const char* source_cstr = kernel_source.c_str();
    size_t source_size = kernel_source.size();

    // 2. Create OpenCL program
    cl_program program = clCreateProgramWithSource(context, 1, &source_cstr, &source_size, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create program with source.";
    ASSERT_NE(program, nullptr) << "Program is null.";

    // 3. Build the program
    err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // If build fails, get the build log to help debugging
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> build_log(log_size);
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log.data(), nullptr);
        FAIL() << "Failed to build program. Build log:\n" << build_log.data();
    }

    // 4. Create OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simple_add", &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create kernel.";
    ASSERT_NE(kernel, nullptr) << "Kernel is null.";

    // 5. Setup data and buffers
    const size_t buffer_size = elements * sizeof(int);

    std::vector<int> hostA(elements);
    std::vector<int> hostB(elements);
    std::vector<int> hostResult(elements, 0);

    for (size_t i = 0; i < elements; ++i) {
        hostA[i] = i;
        hostB[i] = i * 2;
    }

    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostA.data(), &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create bufA.";

    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostB.data(), &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create bufB.";

    cl_mem bufResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create bufResult.";

    // 6. Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to set kernel arg 0.";

    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to set kernel arg 1.";

    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufResult);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to set kernel arg 2.";

    // 7. Execute the kernel
    size_t global_work_size[1] = { elements };
    err = clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to enqueue NDRange kernel.";

    // 8. Read back the results
    err = clEnqueueReadBuffer(command_queue, bufResult, CL_TRUE, 0, buffer_size, hostResult.data(), 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to read buffer.";

    // 9. Verify the results using the new VerifyResults method against CPU reference
    VerifyResults(hostResult, goldenData);

    // 10. Cleanup (context and queue are handled by TearDown in base class)
    if (bufA) clReleaseMemObject(bufA);
    if (bufB) clReleaseMemObject(bufB);
    if (bufResult) clReleaseMemObject(bufResult);
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
}

// Instantiate the parameterized test suite with different data sizes
INSTANTIATE_TEST_SUITE_P(
    DifferentSizes,
    OpenCLParameterizedTest,
    ::testing::Values(
        10,       // Test with 10 elements
        1024,     // Test with 1024 elements
        1000000   // Test with 1,000,000 elements
    )
);
