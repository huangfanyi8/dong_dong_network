cmake_minimum_required(VERSION 3.15)
project(dong_dong VERSION 0.1 LANGUAGES CXX)

# 库目标：对外头文件在 include/，实现与可运行程序在 source/
add_library(dong_dong STATIC
    source/Platform.cpp
)

target_include_directories(dong_dong PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dong_dong PUBLIC spdlog::spdlog)

# Windows 需要链接 Winsock 库，且必须先用 WSAStartup 初始化
if(WIN32)
    target_link_libraries(dong_dong PUBLIC ws2_32)
endif()
