cmake_minimum_required(VERSION 3.13)
project(HxLanguage C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/product)

set(FLAGS_COMMON -fno-omit-frame-pointer -Ofast)
set(FLAGS_SANITIZE -fsanitize=address)

if(WIN32)
  # AddressSanitizer 通常不支持在 MinGW/Windows 目标上使用。
  message(WARNING "检测到 Windows 平台：已跳过 AddressSanitizer 标志（MinGW/MSVC 目标通常不支持）。")
  set(DEFAULT_C_FLAGS ${FLAGS_COMMON})
  set(DEFAULT_LINK_FLAGS)
else()
  set(DEFAULT_C_FLAGS ${FLAGS_COMMON} ${FLAGS_SANITIZE})
  set(DEFAULT_LINK_FLAGS ${FLAGS_SANITIZE})
endif()

message(STATUS "使用的 C 编译器：${CMAKE_C_COMPILER}")
message(STATUS "应用的 C 编译标志：${DEFAULT_C_FLAGS}")

add_executable(hxc src/HXC/Main.c)
target_compile_options(hxc PRIVATE ${DEFAULT_C_FLAGS})
target_link_options(hxc PRIVATE ${DEFAULT_LINK_FLAGS})

add_executable(hxvm src/HXVM/Main.c)
target_compile_options(hxvm PRIVATE ${DEFAULT_C_FLAGS})
target_link_options(hxvm PRIVATE ${DEFAULT_LINK_FLAGS})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(WARNING "建议在单独的目录中构建（源外构建）。")
endif()

message(STATUS "CMake配置好了， 请执行: mkdir build && cd build && cmake .. && cmake --build . 或 cmake --build .")
