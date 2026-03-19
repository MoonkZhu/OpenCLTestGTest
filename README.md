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

## 如何管理多目录和新增测试用例

本项目在 CMake 配置中支持自动搜索 `src/` 下的子目录。如果您希望详细分类测试用例，您可以在 `src/` 下创建新的子文件夹。对于每一个包含了 `.cpp` 文件的子文件夹，CMake 都会为您自动生成一个名为 `cl_test_<子文件夹名>` 的可执行测试文件。这使得您的构建系统能自动适应项目的变化而无需修改 `CMakeLists.txt`。

得益于基础 fixture 和 CMake 配置，添加新的测试用例非常容易。项目中包含一个名为 `LoadKernelSource` 的辅助函数来帮助您加载 OpenCL Kernel 源码，并且 CMake 会自动将 `kernel` 文件夹拷贝到 build 目录。您需要在自己的测试用例中自行初始化及清理 OpenCL 的上下文、队列等状态，以便进行更灵活的测试。

以下是添加并运行 Kernel 的详细步骤（以向量相加为例）：

1. 在 `kernel/` 目录中创建一个新的 Kernel 文件（例如 `kernel/simple_add.cl`）：
```c
__kernel void simple_add(__global const int* a, __global const int* b, __global int* result) {
    int id = get_global_id(0);
    result[id] = a[id] + b[id];
}
```

2. 在 `src/` 目录中（或者其下的某个新创建的子文件夹例如 `src/my_category/`）创建一个新的测试源文件。
3. 包含基础夹具头文件：`#include "cl_test_base.hpp"`，并继承 `OpenCLTest` 来编写您的测试。在测试中，你需要手动初始化 OpenCL 状态并可以在执行之后手动清理。同时您可以使用从基类继承的 `LoadKernelSource()` 方法。

如何使用 `TEST_P` (参数化测试) 注入不同的数据输入进行测试:

### 混合初始化机制与 CPU Golden Data 校验

本项目支持两种测试初始化模式（混合模式）：

- **常规模式 (Normal Mode)**: 如果你不需要特殊的 OpenCL 初始化配置，只需在测试的开始调用 `InitStandard()`。该方法会自动找到平台、设备，并创建 `context` 和 `command_queue`，在测试结束后也会自动清理，无需手动编写这些模板代码。
- **高级模式 (Advanced Mode)**: 如果你需要测试特殊属性（例如特殊的 `cl_mem_flags`，或者只测试特定类型的设备），你可以跳过 `InitStandard()`，在测试用例中自行调用 OpenCL C API 进行自定义初始化。如果你手动修改了 `context` 或 `command_queue` 这两个被保护的成员变量，基类的 `TearDown()` 依然会安全地为你释放它们。

同时，为了保证数据的准确性，推荐重写基类的 `CpuReference()` 钩子来生成或者加载 CPU 的 Golden Data（标准参考数据），并在最后使用 `VerifyResults()` 方法进行比对。`VerifyResults()` 自动处理了浮点数（使用容差比较）、整数及原始二进制内存的比对。

#### 准备 Golden Data 的外部方式
如果你不希望在 C++ 里手写 CPU 逻辑，你也可以通过 Python 脚本运行算法，将生成的 Golden 结果序列化成 `.bin` 二进制文件放入 `data/` 目录。在你的测试用例中读取这个 `.bin` 文件并传递给 `VerifyResults(void* actual, void* golden, size_t size)` 进行验证。

以下展示了如何使用参数化测试，结合 **常规模式** 及 **CPU 黄金数据** 验证：

使用 GTest 的参数化测试功能（`TEST_P` 和 `INSTANTIATE_TEST_SUITE_P`），你可以编写一次测试逻辑，然后注入多组不同大小或类型的数据进行运行，而无需编写重复代码。首先需要创建一个继承自你的基础类和 `::testing::WithParamInterface<T>` 的新类，其中 `T` 是你需要注入的数据类型（例如 `size_t` 表示数组大小）。

示例:
```cpp
#include "cl_test_base.hpp"
#include <vector>

// 继承 OpenCLTest 以及 WithParamInterface 来接收 size_t 类型的参数
class OpenCLParameterizedTest : public OpenCLTest, public ::testing::WithParamInterface<size_t> {
protected:
    std::vector<int> goldenData;

    // 重写 CpuReference 以生成参考基准数据
    void CpuReference() override {
        size_t elements = GetParam();
        goldenData.resize(elements);
        for (size_t i = 0; i < elements; ++i) {
            goldenData[i] = i + (i * 2);
        }
    }
};

TEST_P(OpenCLParameterizedTest, KernelExecution) {
    // 0. 初始化 OpenCL (常规模式：使用预置的快速初始化)
    InitStandard();

    // 运行 CPU 参考逻辑，准备 Golden Data
    CpuReference();

    // 获取当前测试实例被注入的参数（比如元素个数）
    size_t elements = GetParam();
    cl_int err;

    // 1. 加载 Kernel 源码
    std::string kernel_source;
    ASSERT_NO_THROW({
        kernel_source = LoadKernelSource("kernel/simple_add.cl");
    }) << "Failed to load kernel source.";

    const char* source_cstr = kernel_source.c_str();
    size_t source_size = kernel_source.size();

    // 2. 创建 OpenCL program
    cl_program program = clCreateProgramWithSource(context, 1, &source_cstr, &source_size, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create program with source.";

    // 3. 构建 program
    err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to build program.";

    // 4. 创建 OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simple_add", &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create kernel.";

    // 5. 设置数据和 Buffer (使用注入的参数)
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

    // 9. 验证结果 (使用内置的比较机制与 Golden Data 对比)
    VerifyResults(hostResult, goldenData);

    // 10. 清理资源 (Buffer, Kernel, Program... 注意：context 和 queue 会由 TearDown 自动清理)
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufResult);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
}

// 实例化测试套件，并注入不同的数据大小
INSTANTIATE_TEST_SUITE_P(
    DifferentSizes,
    OpenCLParameterizedTest,
    ::testing::Values(
        10,       // 使用 10 个元素进行测试
        1024,     // 使用 1024 个元素进行测试
        1000000   // 使用 1,000,000 个元素进行测试
    )
);
```
4. 重新构建项目（在 `build/` 目录内运行 `cmake --build .`）。新的参数化测试将被自动发现并针对每组输入数据运行一次！

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

## How to Manage Multiple Directories and Add a Kernel Test Case

This project's CMake configuration supports automatic discovery of subdirectories within `src/`. If you wish to organize your test cases into detailed categories, you can create new subfolders under `src/`. For each subfolder that contains `.cpp` files, CMake will automatically generate an executable test file named `cl_test_<subfoldername>`. This allows the build system to adapt to changes in your project structure without requiring modifications to `CMakeLists.txt`.

Thanks to the base fixture and CMake configuration, adding a new test case is extremely easy. The project includes a helper function `LoadKernelSource` to assist you in loading OpenCL Kernel source code, and CMake will automatically copy the `kernel` folder to the build directory. You will need to initialize and clean up OpenCL state (like contexts and command queues) within your own test cases to allow for more flexible testing.

Here is a step-by-step guide to adding and running a Kernel (using vector addition as an example):

1. Create a new Kernel file in the `kernel/` directory (e.g., `kernel/simple_add.cl`):
```c
__kernel void simple_add(__global const int* a, __global const int* b, __global int* result) {
    int id = get_global_id(0);
    result[id] = a[id] + b[id];
}
```

2. Create a new test source file in the `src/` directory (or a newly created subfolder like `src/my_category/`).
3. Include the base fixture header: `#include "cl_test_base.hpp"` and inherit from `OpenCLTest`. Inside the test, you need to manually initialize the OpenCL state and clean it up afterward. You can use the inherited `LoadKernelSource()` method.

How to Use `TEST_P` (Parameterized Testing) to Inject Different Data Inputs:

### Hybrid Initialization Mechanism & CPU Golden Data Verification

This project supports a hybrid test initialization model:

- **Normal Mode**: If you don't need any special OpenCL configuration, simply call `InitStandard()` at the very beginning of your test. This will automatically fetch the platform and device, create the `context` and `command_queue`, and automatically clean them up when the test finishes.
- **Advanced Mode**: If you need to test special properties (e.g., custom `cl_mem_flags`, or querying for a specific device type), you can skip `InitStandard()`. You are free to initialize the OpenCL runtime manually using standard C APIs. As long as you assign your handles to the protected members `context` and `command_queue`, the base class's `TearDown()` hook will safely release them for you.

Additionally, to ensure the accuracy of your logic, you should override the base class's `CpuReference()` hook to generate or load CPU Golden Data (reference data). Once generated, you can easily validate OpenCL output arrays against your CPU baseline using `VerifyResults()`. `VerifyResults` automatically handles float/double comparisons (with tolerances), generic integral types, and raw binary memory.

#### External Golden Data Preparation
If you'd rather not implement the CPU logic in C++, you can run an algorithm in Python, serialize the expected binary output to a `.bin` file, and place it in a `data/` directory. Inside your test case, read this `.bin` file and pass its buffer to `VerifyResults(void* actual, void* golden, size_t size)` for a direct memory comparison.

Below is an example showing how to use parameterized testing alongside **Normal Mode** initialization and **CPU Golden Data** generation:

Using GTest's parameterized testing features (`TEST_P` and `INSTANTIATE_TEST_SUITE_P`), you can write your test logic once and then inject multiple sets of varying data (like different sizes or data types) without duplicating code. First, create a new class that inherits from your base test fixture and `::testing::WithParamInterface<T>`, where `T` is the type of the injected data (e.g., `size_t` for an array size).

Example:
```cpp
#include "cl_test_base.hpp"
#include <vector>

// Inherit from OpenCLTest and WithParamInterface to receive size_t type parameters
class OpenCLParameterizedTest : public OpenCLTest, public ::testing::WithParamInterface<size_t> {
protected:
    std::vector<int> goldenData;

    // Override CpuReference to generate the baseline data
    void CpuReference() override {
        size_t elements = GetParam();
        goldenData.resize(elements);
        for (size_t i = 0; i < elements; ++i) {
            goldenData[i] = i + (i * 2);
        }
    }
};

TEST_P(OpenCLParameterizedTest, KernelExecution) {
    // 0. Initialize OpenCL (Normal Mode: quick and easy standardized setup)
    InitStandard();

    // Prepare CPU Golden Data reference
    CpuReference();

    // Get the parameter injected for this test instance (e.g., number of elements)
    size_t elements = GetParam();
    cl_int err;

    // 1. Load Kernel source
    std::string kernel_source;
    ASSERT_NO_THROW({
        kernel_source = LoadKernelSource("kernel/simple_add.cl");
    }) << "Failed to load kernel source.";

    const char* source_cstr = kernel_source.c_str();
    size_t source_size = kernel_source.size();

    // 2. Create OpenCL program
    cl_program program = clCreateProgramWithSource(context, 1, &source_cstr, &source_size, &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create program with source.";

    // 3. Build program
    err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to build program.";

    // 4. Create OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simple_add", &err);
    ASSERT_EQ(err, CL_SUCCESS) << "Failed to create kernel.";

    // 5. Setup data and Buffers (Using the injected parameter)
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

    // 9. Verify results (using built-in comparison against Golden Data)
    VerifyResults(hostResult, goldenData);

    // 10. Cleanup resources (Buffer, Kernel, Program... Note: context and queue are handled automatically)
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufResult);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
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
```
4. Rebuild the project (`cmake --build .` inside the `build/` directory). The new parameterized tests will be automatically discovered and run once for each set of input data!
