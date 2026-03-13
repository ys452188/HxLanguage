cmake_minimum_required(VERSION 3.5)

if(MSVC)
    add_compile_options(/Zp1)
else()
    add_compile_options(-fno-strict-aliasing) # 禁用严格别名优化
endif()

add_compile_options(-O3 -flto -fno-exceptions)

project(HxLanguage VERSION 0.114)
add_executable(hxc src/HXC/Main.cpp) 

add_executable(hxvm src/HXVM/Main.cpp)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    # 强制增加 TLS 对齐
    set_target_properties(hxvm PROPERTIES COMPILE_FLAGS "-ftls-model=global-dynamic")
    # 尝试强制对齐
    add_definitions("-falign-functions=64")
endif()
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    # 强制使用全局动态 TLS 模型，这通常能绕过对齐检查的硬限制
    target_compile_options(hxvm PRIVATE "-ftls-model=global-dynamic")
    
    # 告诉链接器不要过于激进地压缩段
    target_link_options(hxvm PRIVATE "-Wl,-z,common-page-size=65536" "-Wl,-z,max-page-size=65536")
endif()