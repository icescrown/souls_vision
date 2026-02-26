cmake_minimum_required(VERSION 3.27)
project(souls_vision_renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 外部渲染进程可执行文件
add_executable(souls_vision_renderer
    main.cpp
    texture_loader.cpp
    texture_loader.h
    ../shared_memory.cpp
    ../shared_memory.h
    ../config.cpp
    ../config.h
    ../shared_types.cpp
    ../shared_types.h
    ../globals.cpp
    ../globals.h
    ../logger.cpp
    ../logger.h
    ../util.cpp
    ../util.h
)

target_include_directories(souls_vision_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus
)

target_link_libraries(souls_vision_renderer PRIVATE
    d2d1
    dwrite
    windowscodecs
    shlwapi
)

# 输出目录由主CMakeLists.txt统一设置