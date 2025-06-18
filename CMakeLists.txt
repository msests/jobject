cmake_minimum_required(VERSION 3.16)
project(jobject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 创建静态库
add_library(jobject STATIC src/JObject.cpp)

target_include_directories(jobject PUBLIC src)

# 创建测试程序
add_executable(test_jobject test/main.cpp)
target_link_libraries(test_jobject jobject)

# 设置MSVC特定的编译选项
if(MSVC)
    target_compile_options(jobject PRIVATE /source-charset:utf-8 /W4)
    target_compile_options(test_jobject PRIVATE /source-charset:utf-8 /W4)
else()
    target_compile_options(jobject PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(test_jobject PRIVATE -Wall -Wextra -pedantic)
endif()