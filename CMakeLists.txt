cmake_minimum_required(VERSION 3.25)

# 配置项目名称, 语言
project(MergeBin C)

# 配置语言版本
set(CMAKE_C_STANDARD 11)

# 头文件搜索路径
include_directories(
        ${PROJECT_SOURCE_DIR}/src)

# 库链接搜索路径
link_directories(
        ${PROJECT_SOURCE_DIR}/src)

# 附加源代码文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src MAIN_SRCS)

# 目标可执行文件
add_executable(MergeBin ${MAIN_SRCS})
