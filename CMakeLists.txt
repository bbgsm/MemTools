cmake_minimum_required(VERSION 3.29)
project(MemTools)

if (UNIX) # Linux
    add_definitions(-DLINUX)
elseif (WIN32) # Windows MSVC参数
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif ()

link_directories(${CMAKE_SOURCE_DIR}/dmaLibs/linuxArm64)

set(CMAKE_CXX_STANDARD 20)

if (UNIX) # Linux
    # 设置输出路径为项目根目录的out
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}")

elseif (WIN32) # Windows
    # 设置输出路径为项目根目录的out
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out")
endif ()

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

if (UNIX)
#    find_library(VMM_LIBRARY NAMES vmm PATHS ${CMAKE_SOURCE_DIR}/dmaLibs/linuxArm64)

    # DMA 工具需要的依赖
    target_link_libraries(dmaMemTools
            vmm
            leechcore
    )
    # 测试代码依赖
    target_link_libraries(test
            vmm
            leechcore
    )
    # 测试代码依赖
    target_link_libraries(dumpMem
            vmm
            leechcore
    )
elseif (WIN32)
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
endif ()


########### 文件复制 #############
# 架构判断
if (UNIX) # Linux
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")  # x64架构
        file(GLOB DATA_FILES "dmaLibs/linuxX64/*.so" "dmaLibs/linuxX64/*.db")
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64") # arm64架构
        file(GLOB DATA_FILES "dmaLibs/linuxArm64/*.so" "dmaLibs/linuxArm64/*.db")
    endif ()
    # 遍历文件列表并复制每个文件
    foreach (DATA_FILE ${DATA_FILES})
        # 无论如何设置都获取不到正确的编译输出目录，只能手动拼接剩下的路径
        file(COPY ${DATA_FILE} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endforeach ()
elseif (WIN32) # Windows
    file(GLOB DATA_FILES "dmaLibs/win32/*.dll" "dmaLibs/win32/*.db")
    # 遍历文件列表并复制每个文件
    foreach (DATA_FILE ${DATA_FILES})
        # 无论如何设置都获取不到正确的编译输出目录，只能手动拼接剩下的路径
        file(COPY ${DATA_FILE} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
        file(COPY ${DATA_FILE} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
    endforeach ()
endif ()


########### 文件复制 #############
