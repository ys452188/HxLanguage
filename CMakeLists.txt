cmake_minimum_required(VERSION 3.5)

add_compile_options(-O3 -ffast-math)
project(HxLanguage VERSION 0.114)
add_executable(hxc src/HXC/Main.cpp)
add_executable(hxvm src/HXVM/Main.cpp)
