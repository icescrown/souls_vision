cmake_minimum_required(VERSION 3.27)
project(souls_vision)

set(CMAKE_CXX_STANDARD 23)

# 统一输出目录到build根目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Define the path to the game and mods folder
set(GAME_DIR "C:/Program Files (x86)/Steam/steamapps/common/ELDEN RING/Game")
set(MODS_DIR "${GAME_DIR}/mods")
set(GAME_EXE "ersc_launcher.exe")

file(COPY ${CMAKE_SOURCE_DIR}/sv_assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR})

# 添加third_party子目录（这会在主目标之前构建minhook）
add_subdirectory(third_party/minhook)

# 主DLL - 外部进程方案
add_library(souls_vision SHARED
        dll_main.cpp
        globals.cpp
        globals.h
        game_handler.cpp
        game_handler.h
        hook_helper.h
        hook_helper.cpp
        memory.cpp
        memory.h
        logger.cpp
        logger.h
        config.cpp
        config.h
        structs/world_chr_man_imp.cpp
        structs/world_chr_man_imp.h
        structs/chr_ins.cpp
        structs/chr_ins.h
        structs/entry.cpp
        structs/entry.h
        structs/chr_module_bag.cpp
        structs/chr_module_bag.h
        structs/chr_stat_module.cpp
        structs/chr_stat_module.h
        structs/chr_resist_module.cpp
        structs/chr_resist_module.h
        structs/chr_super_armor_module.cpp
        structs/chr_super_armor_module.h
        shared_types.cpp
        shared_types.h
        shared_memory.cpp
        shared_memory.h
        data_collector.cpp
        data_collector.h
        util.cpp
        util.h
        structs/solo_param_repository_imp.cpp
        structs/solo_param_repository_imp.h
        structs/fd4_basic_hash_string.cpp
        structs/fd4_basic_hash_string.h
        structs/solo_param_holder.cpp
        structs/solo_param_holder.h
        structs/param_res_cap.cpp
        structs/param_res_cap.h
        structs/param_data.cpp
        structs/param_data.h
        structs/param_header.cpp
        structs/param_header.h
        structs/param_row.cpp
        structs/param_row.h
        structs/npc_param.cpp
        structs/npc_param.h
        structs/cs_menu_man_imp.cpp
        structs/cs_menu_man_imp.h
        structs/loading_screen_data.cpp
        structs/loading_screen_data.h
)

target_include_directories(souls_vision PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus
    ${CMAKE_SOURCE_DIR}/third_party/minhook/include
)

target_link_libraries(souls_vision PRIVATE 
    dwmapi
    minhook
)

# 外部渲染进程
add_subdirectory(renderer)

# add_custom_command(TARGET souls_vision POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:souls_vision>" "${MODS_DIR}/"
# )
