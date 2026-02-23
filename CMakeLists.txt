cmake_minimum_required(VERSION 3.5)

if(MSVC)
    add_compile_options(/Zp1)
else()
    add_compile_options(-fno-strict-aliasing) # 禁用严格别名优化
endif()

project(HxLanguage VERSION 0.114)
add_executable(hxc src/HXC/Main.cpp)  #hxc开优化必炸！！！

add_executable(hxvm src/HXVM/Main.cpp)
set_source_files_properties(src/HXC/Main.cpp PROPERTIES
    COMPILE_FLAGS "-O3"
)
