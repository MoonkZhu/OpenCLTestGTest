# OpenCL Test Suite (English)
This project is a custom OpenCL Test Suite built using Google Test (GTest). It is designed to be a clean, minimal, and beginner-friendly environment for writing and running OpenCL test cases.

## Prerequisites & Configuration
Before building and running the test suite, ensure you have the following installed:
1. **CMake** (version 3.14 or higher)
2. **A C++14 compatible compiler** (e.g., GCC, Clang, or MSVC)
3. **OpenCL SDK / Runtime**: Ensure your system has an OpenCL driver and SDK installed (e.g., Intel, AMD, NVIDIA, or Apple OpenCL framework).

The Google Test dependency is already included in `third_party/googletest/`.

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
When you run `./cl_test_runner`, Google Test will output the results to the terminal.
- You will see a green `[  PASSED  ]` next to tests that completed successfully.
- If a test fails, you will see a red `[  FAILED  ]`, along with an explanation of the failure (e.g., an `ASSERT_EQ` mismatch or an OpenCL error code).

At the end of the output, GTest will provide a summary of how many tests passed and how many failed.

## How to Add a New Test Case
Thanks to the base fixture and CMake configuration, adding a new test case is extremely easy:
1. Create a new `.cpp` file in the `src/` directory (e.g., `src/my_new_test.cpp`).
2. Include the base fixture header: `#include "cl_test_base.hpp"`
3. Write your test using the `TEST_F` macro, inheriting from `OpenCLTest`.

Example:
```cpp
#include "cl_test_base.hpp"

TEST_F(OpenCLTest, MyNewFeature) {
    // You have access to:
    // platform_id, device_id, context, command_queue

    // Write your OpenCL code here...

    // Use assertions to verify correctness
    ASSERT_EQ(1, 1);
}
```
4. Rebuild the project (`cmake --build .` inside the `build/` directory). The new test will be discovered and run automatically!

---

# OpenCL Test Suite (中文)
本项目是一个使用 Google Test (GTest) 构建的自定义 OpenCL 测试套件。它旨在提供一个干净、精简且对初学者友好的环境，用于编写和运行 OpenCL 测试用例。

## 前置条件与配置
在构建和运行测试套件之前，请确保您已安装以下软件：
1. **CMake** (版本 3.14 或更高)
2. **支持 C++14 的编译器** (例如 GCC, Clang, 或 MSVC)
3. **OpenCL SDK / 运行时**: 确保您的系统安装了 OpenCL 驱动和 SDK (例如 Intel, AMD, NVIDIA, 或 Apple 的 OpenCL 框架)。

Google Test 依赖项已包含在 `third_party/googletest/` 目录中。

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
当您运行 `./cl_test_runner` 时，Google Test 会将结果输出到终端。
- 成功完成的测试旁边会显示绿色的 `[  PASSED  ]`。
- 如果测试失败，您将看到红色的 `[  FAILED  ]`，以及失败原因的说明（例如 `ASSERT_EQ` 不匹配或 OpenCL 错误代码）。

在输出的最后，GTest 会提供一个总结，说明有多少测试通过，有多少失败。

## 如何添加新的测试用例
得益于基础夹具（Base Fixture）和 CMake 配置，添加新的测试用例非常容易：
1. 在 `src/` 目录中创建一个新的 `.cpp` 文件（例如 `src/my_new_test.cpp`）。
2. 包含基础夹具头文件：`#include "cl_test_base.hpp"`
3. 使用 `TEST_F` 宏编写您的测试，并继承自 `OpenCLTest`。

示例:
```cpp
#include "cl_test_base.hpp"

TEST_F(OpenCLTest, MyNewFeature) {
    // 您可以直接使用以下变量:
    // platform_id, device_id, context, command_queue

    // 在这里编写您的 OpenCL 代码...

    // 使用断言验证正确性
    ASSERT_EQ(1, 1);
}
```
4. 重新构建项目（在 `build/` 目录内运行 `cmake --build .`）。新的测试将被自动发现并运行！
