# MemTools
Windows、Linux DMA、Dump 、Direct 内存工具
- [DirectMemoryTools 读取本机内存](DirectMemoryTools.cpp)
- [DmaMemoryTools 读取Dma内存，只支持DMA读取Windows内存](DmaMemoryTools.cpp)
- [DumpMemoryTools 读取Dump的内存](DumpMemoryTools.cpp)
- [MemoryToolsBase 上面内存工具基类，都继承自它](MemoryToolsBase.cpp)

# 编译
- Windows
编译环境: Clion + cmake + vs2022, 在 Clion 的 cmake 配置中选择vs2022编译工具链进行编译
- Linux
编译环境: Clion + cmake + Unix Makefiles, 在 Clion 的 cmake 配置中选择Unix Makefiles编译工具
# 测试环境
- Windows
Windows 11 + vs2022
- Linux
1.Linux arm64 香橙派5 + Ubuntu 20.04
2.Linux X64 待开发....
# Dump的内存如何使用
### 使用下面CEDumpPlugin插件配合CE即可进行分析

## 关于
* [pcileech(DMA驱动)](https://github.com/ufrisk/pcileech.git)
* [DMALibrary(DMA工具代码)](https://github.com/Metick/DMALibrary)
* [CEDumpPlugin(CE Dump内存分析插件)](https://github.com/bbgsm/CEDumpPlugin)