#include "cl_test_base.hpp"
#include <vector>

TEST_F(OpenCLTest, BufferCopy) {
    const size_t elements = 1024;
    const size_t buffer_size = elements * sizeof(int);

    // Initialize host data
    std::vector<int> hostA(elements);
    for (size_t i = 0; i < elements; ++i) {
        hostA[i] = i;
    }
    
    std::vector<int> hostB(elements, 0);

    cl_int err;

    // Create bufA and initialize it with hostA
    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostA.data(), &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create bufA.";
    ASSERT_NE(bufA, nullptr) << "bufA is null.";

    // Create bufB
    cl_mem bufB = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create bufB.";
    ASSERT_NE(bufB, nullptr) << "bufB is null.";

    // Copy from bufA to bufB
    err = clEnqueueCopyBuffer(command_queue, bufA, bufB, 0, 0, buffer_size, 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to enqueue copy buffer.";

    // Read back from bufB to hostB
    err = clEnqueueReadBuffer(command_queue, bufB, CL_TRUE, 0, buffer_size, hostB.data(), 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to enqueue read buffer.";

    // Verify the results
    for (size_t i = 0; i < elements; ++i) {
        ASSERT_EQ(hostA[i], hostB[i]) << "Data mismatch at index " << i;
    }

    // Cleanup
    if (bufA) clReleaseMemObject(bufA);
    if (bufB) clReleaseMemObject(bufB);
}
