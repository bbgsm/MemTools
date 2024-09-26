cmake_minimum_required(VERSION 3.29)
project(MemTools)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(CMAKE_CXX_STANDARD 20)
# 设置输出路径为项目根目录的out
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out")

include_directories(.)
include_directories(dmaLibs/include)

# DMA 工具编译
add_library(dmaMemTools SHARED
        DmaMemoryTools.cpp
        MemoryToolsBase.cpp
)
# 本机内存工具编译
add_library(directMemTools SHARED
        DirectMemoryTools.cpp
        MemoryToolsBase.cpp
)
# dump内存工具编译
add_library(dumpMemTools SHARED
        DumpMemoryTools.cpp
        MemoryToolsBase.cpp
)

# 测试代码
add_executable(test
        main.cpp
        DmaMemoryTools.cpp
        DumpMemoryTools.cpp
        DirectMemoryTools.cpp
        MemoryToolsBase.cpp
)

# dump
add_executable(dumpMem
        dump.cpp
#        DirectMemoryTools.cpp
        DmaMemoryTools.cpp
        MemoryToolsBase.cpp
)

# DMA 工具需要的依赖
target_link_libraries(dmaMemTools
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/leechcore.lib"
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/vmm.lib"
        ws2_32
)

# 测试代码依赖
target_link_libraries(test
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/leechcore.lib"
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/vmm.lib"
        ws2_32
)


# 测试代码依赖
target_link_libraries(dumpMem
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/leechcore.lib"
        "${CMAKE_SOURCE_DIR}/dmaLibs/win32/vmm.lib"
        ws2_32
)

########### 文件复制 #############
file(GLOB DATA_FILES "dmaLibs/win32/*.dll" "dmaLibs/win32/*.db")
# 遍历文件列表并复制每个文件
foreach(DATA_FILE ${DATA_FILES})
    # 无论如何设置都获取不到正确的编译输出目录，只能手动拼接剩下的路径
    file(COPY ${DATA_FILE} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
    file(COPY ${DATA_FILE} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
endforeach()
########### 文件复制 #############
