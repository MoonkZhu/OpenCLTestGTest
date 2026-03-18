add_test([=[OpenCLTest.BufferCopy]=]  /app/build/cl_test_runner [==[--gtest_filter=OpenCLTest.BufferCopy]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OpenCLTest.BufferCopy]=]  PROPERTIES WORKING_DIRECTORY /app/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  cl_test_runner_TESTS OpenCLTest.BufferCopy)
