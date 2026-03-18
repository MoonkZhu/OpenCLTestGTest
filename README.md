# OpenCL Test Suite (中文)
本项目是一个使用 GTest 构建的自定义 OpenCL 测试套件。它旨在提供一个干净、精简且对初学者友好的环境，用于编写和运行 OpenCL 测试用例。

## 前置条件与配置
在构建和运行测试套件之前，请确保您已安装以下软件：
1. **CMake** (版本 3.14 或更高)
2. **支持 C++14 的 compiler** (例如 GCC, Clang, 或 MSVC)
3. **OpenCL SDK / Runtime**: 确保您的系统安装了 OpenCL 驱动和 SDK (例如 Intel, AMD, NVIDIA, 或 Apple 的 OpenCL 框架)。

如果您的系统没有将 OpenCL 安装在标准的系统路径，而是提供了一个独立的 SDK 包，您可以通过配置 CMake 来指定 OpenCL 的位置：
```bash
# 假设您的 OpenCL SDK 解压在 /path/to/OpenCL-SDK
# 在运行 CMake 时，通过指定 CMAKE_PREFIX_PATH 或者 OpenCL_ROOT 来帮助 CMake 找到它
cmake -DOpenCL_ROOT=/path/to/OpenCL-SDK ..
```
或者，您可以通过环境变量来导出它：
```bash
export OpenCL_ROOT=/path/to/OpenCL-SDK
cmake ..
```

GTest 依赖项已包含在 `third_party/googletest/` 目录中。

## 如何构建与运行
要构建并运行测试套件，请在项目根目录下打开终端并运行以下命令：

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 运行 CMake 配置项目
cmake ..

# 3. 编译测试运行程序
cmake --build .

# 4. 运行测试
./cl_test_runner
# 或者使用 CTest 运行:
# ctest --output-on-failure
```

## 如何知道测试是否通过
当您运行 `./cl_test_runner` 时，GTest 会将结果输出到终端。
- 成功完成的测试旁边会显示绿色的 `[  PASSED  ]`。
- 如果测试失败，您将看到红色的 `[  FAILED  ]`，以及失败原因的说明（例如 `ASSERT_EQ` 不匹配或 OpenCL 错误代码）。

在输出的最后，GTest 会提供一个总结，说明有多少测试通过，有多少失败。

## 如何添加并运行使用 Kernel 的测试用例
得益于基础 fixture 和 CMake 配置，添加新的测试用例非常容易。项目中包含一个名为 `LoadKernelSource` 的辅助函数来帮助您加载 OpenCL Kernel 源码，并且 CMake 会自动将 `kernel` 文件夹拷贝到 build 目录。

以下是添加并运行 Kernel 的详细步骤（以向量相加为例）：

1. 在 `kernel/` 目录中创建一个新的 Kernel 文件（例如 `kernel/simple_add.cl`）：
```c
__kernel void simple_add(__global const int* a, __global const int* b, __global int* result) {
    int id = get_global_id(0);
    result[id] = a[id] + b[id];
}
```

2. 在 `src/` 目录中创建一个新的测试源文件（例如 `src/kernel_tests.cpp`）。
3. 包含基础夹具头文件：`#include "cl_test_base.hpp"`，并继承 `OpenCLTest` 来编写您的测试。在测试中，您可以直接使用 `platform_id`, `device_id`, `context` 和 `command_queue` 等变量。

示例:
```cpp
#include "cl_test_base.hpp"
#include <vector>

TEST_F(OpenCLTest, KernelExecution) {
    // 1. 加载 Kernel 源码
    std::string kernel_source;
    ASSERT_NO_THROW({
        kernel_source = LoadKernelSource("kernel/simple_add.cl");
    }) << "Failed to load kernel source.";

    const char* source_cstr = kernel_source.c_str();
    size_t source_size = kernel_source.size();

    cl_int err;

    // 2. 创建 OpenCL program
    cl_program program = clCreateProgramWithSource(context, 1, &source_cstr, &source_size, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create program with source.";

    // 3. 构建 program
    err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to build program.";

    // 4. 创建 OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simple_add", &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create kernel.";

    // 5. 设置数据和 Buffer
    const size_t elements = 1024;
    const size_t buffer_size = elements * sizeof(int);

    std::vector<int> hostA(elements);
    std::vector<int> hostB(elements);
    std::vector<int> hostResult(elements, 0);

    for (size_t i = 0; i < elements; ++i) {
        hostA[i] = i;
        hostB[i] = i * 2;
    }

    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostA.data(), &err);
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostB.data(), &err);
    cl_mem bufResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &err);

    // 6. 设置 Kernel 参数
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufResult);

    // 7. 执行 Kernel
    size_t global_work_size[1] = { elements };
    clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);

    // 8. 读取结果
    clEnqueueReadBuffer(command_queue, bufResult, CL_TRUE, 0, buffer_size, hostResult.data(), 0, nullptr, nullptr);

    // 9. 验证结果
    for (size_t i = 0; i < elements; ++i) {
        ASSERT_EQ(hostResult[i], hostA[i] + hostB[i]) << "Data mismatch at index " << i;
    }

    // 10. 清理资源 (Buffer, Kernel, Program...)
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufResult);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
}
```
4. 重新构建项目（在 `build/` 目录内运行 `cmake --build .`）。新的测试将被自动发现并运行！

---

# OpenCL Test Suite (English)
This project is a custom OpenCL Test Suite built using GTest. It is designed to be a clean, minimal, and beginner-friendly environment for writing and running OpenCL test cases.

## Prerequisites & Configuration
Before building and running the test suite, ensure you have the following installed:
1. **CMake** (version 3.14 or higher)
2. **A C++14 compatible compiler** (e.g., GCC, Clang, or MSVC)
3. **OpenCL SDK / Runtime**: Ensure your system has an OpenCL driver and SDK installed (e.g., Intel, AMD, NVIDIA, or Apple OpenCL framework).

If your system does not have OpenCL installed in standard paths, but instead you have a standalone SDK package, you can configure CMake to locate it:
```bash
# Assuming your OpenCL SDK is extracted to /path/to/OpenCL-SDK
# Pass CMAKE_PREFIX_PATH or OpenCL_ROOT to help CMake find it
cmake -DOpenCL_ROOT=/path/to/OpenCL-SDK ..
```
Or export it via environment variable:
```bash
export OpenCL_ROOT=/path/to/OpenCL-SDK
cmake ..
```

The GTest dependency is already included in `third_party/googletest/`.

## How to Build and Run
To build and run the test suite, open your terminal and run the following commands from the project root directory:

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Run CMake to configure the project
cmake ..

# 3. Build the test runner
cmake --build .

# 4. Run the tests
./cl_test_runner
# Or run using CTest:
# ctest --output-on-failure
```

## How to Know if a Test Passed
When you run `./cl_test_runner`, GTest will output the results to the terminal.
- You will see a green `[  PASSED  ]` next to tests that completed successfully.
- If a test fails, you will see a red `[  FAILED  ]`, along with an explanation of the failure (e.g., an `ASSERT_EQ` mismatch or an OpenCL error code).

At the end of the output, GTest will provide a summary of how many tests passed and how many failed.

## How to Add and Run a Kernel Test Case
Thanks to the base fixture and CMake configuration, adding a new test case is extremely easy. The project includes a helper function `LoadKernelSource` to assist you in loading OpenCL Kernel source code, and CMake will automatically copy the `kernel` folder to the build directory.

Here is a step-by-step guide to adding and running a Kernel (using vector addition as an example):

1. Create a new Kernel file in the `kernel/` directory (e.g., `kernel/simple_add.cl`):
```c
__kernel void simple_add(__global const int* a, __global const int* b, __global int* result) {
    int id = get_global_id(0);
    result[id] = a[id] + b[id];
}
```

2. Create a new test source file in the `src/` directory (e.g., `src/kernel_tests.cpp`).
3. Include the base fixture header: `#include "cl_test_base.hpp"` and inherit from `OpenCLTest`. Inside the test, you can directly access variables like `platform_id`, `device_id`, `context`, and `command_queue`.

Example:
```cpp
#include "cl_test_base.hpp"
#include <vector>

TEST_F(OpenCLTest, KernelExecution) {
    // 1. Load Kernel source
    std::string kernel_source;
    ASSERT_NO_THROW({
        kernel_source = LoadKernelSource("kernel/simple_add.cl");
    }) << "Failed to load kernel source.";

    const char* source_cstr = kernel_source.c_str();
    size_t source_size = kernel_source.size();

    cl_int err;

    // 2. Create OpenCL program
    cl_program program = clCreateProgramWithSource(context, 1, &source_cstr, &source_size, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create program with source.";

    // 3. Build program
    err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to build program.";

    // 4. Create OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simple_add", &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create kernel.";

    // 5. Setup data and Buffers
    const size_t elements = 1024;
    const size_t buffer_size = elements * sizeof(int);

    std::vector<int> hostA(elements);
    std::vector<int> hostB(elements);
    std::vector<int> hostResult(elements, 0);

    for (size_t i = 0; i < elements; ++i) {
        hostA[i] = i;
        hostB[i] = i * 2;
    }

    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostA.data(), &err);
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, buffer_size, hostB.data(), &err);
    cl_mem bufResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, nullptr, &err);

    // 6. Set Kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufResult);

    // 7. Execute Kernel
    size_t global_work_size[1] = { elements };
    clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);

    // 8. Read back results
    clEnqueueReadBuffer(command_queue, bufResult, CL_TRUE, 0, buffer_size, hostResult.data(), 0, nullptr, nullptr);

    // 9. Verify results
    for (size_t i = 0; i < elements; ++i) {
        ASSERT_EQ(hostResult[i], hostA[i] + hostB[i]) << "Data mismatch at index " << i;
    }

    // 10. Cleanup resources (Buffer, Kernel, Program...)
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufResult);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
}
```
4. Rebuild the project (`cmake --build .` inside the `build/` directory). The new test will be discovered and run automatically!
