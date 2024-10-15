# MemTools
Windows、Linux DMA、Dump 、Direct、inject 内存工具
- [DirectMemoryTools 读取本机内存](DirectMemoryTools.cpp)
- [DmaMemoryTools 读取Dma内存，只支持DMA读取Windows内存](DmaMemoryTools.cpp)
- [DumpMemoryTools 读取Dump的内存](DumpMemoryTools.cpp)
- [InjectMemoryTools 通过注入读取内存](InjectMemoryTools.cpp)
- [MemoryToolsBase 上面内存工具基类，都继承自它](MemoryToolsBase.cpp)

# 编译
- Windows
编译环境: Clion + cmake + vs2022, 在 Clion 的 cmake 配置中选择vs2022编译工具链进行编译
- Linux
编译环境: Clion + cmake + Unix Makefiles, 在 Clion 的 cmake 配置中选择Unix Makefiles编译工具
- 注意:
<br/><span style="color:red">Linux 下还需要需要把 [vmm.so] [leechcore.so] [leechcore_ft601_driver_linux.so] 三个库文件复制到/usr/lib64/下
并且用root权限执行<span/>
# 测试环境
#### Windows
- Windows 11 + vs2022
#### Linux
- Linux arm64 香橙派5 + Ubuntu 20.04
- Linux X64 待开发....
# Dump的内存如何使用
### 使用下面CEDumpPlugin插件配合CE即可进行分析

## 关于
* [pcileech(DMA驱动)](https://github.com/ufrisk/pcileech.git)
* [DMALibrary(DMA工具代码)](https://github.com/Metick/DMALibrary)
* [CEDumpPlugin(CE Dump内存分析插件)](https://github.com/bbgsm/CEDumpPlugin)