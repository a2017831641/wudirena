cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 11)

# 溢出检测必须搭配 Debug 模式 故将配置放在sh文件中
# 检测编译器并设置溢出检测选项
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Setting -ftrapv for GCC or Clang")
    add_compile_options(-ftrapv)
endif()

include_directories(${CMAKE_SOURCE_DIR}/helper)

set(SOURCE_FILES ${CMAKE_SOURCE_DIR}/source/main.cpp)

add_executable(MyProject ${SOURCE_FILES})