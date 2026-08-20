cmake_minimum_required(VERSION 3.15)
project(dong_dong VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# ---------- spdlog（本地源码，日志库依赖） ----------
add_subdirectory(spdlog)

# ---------- dong_dong 主项目 ----------
add_subdirectory(dong_dong)
