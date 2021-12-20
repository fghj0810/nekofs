# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project(Project_nekofs)

option(NEKOFS_MAKE_TOOLS_LIB "This is a option for tools" OFF)
option(NEKOFS_TEST "This is a option for test" OFF)

# 包含子项目。
add_subdirectory("nekofs")
if (NEKOFS_MAKE_TOOLS_LIB STREQUAL "ON")
    add_subdirectory("thirdparty/gflags")
    add_subdirectory("nekofs_cli")
endif ()

if (NEKOFS_TEST STREQUAL "ON")
    add_subdirectory("test/test_read")
    add_subdirectory("test/test_write")
    add_subdirectory("test/test_sha256")
    add_subdirectory("test/test_overlay")
endif ()
