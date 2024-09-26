//
// Created by bbgsm on 2024/8/20.
//

#include <iostream>
#include "DirectMemoryTools.h"
#include "DmaMemoryTools.h"
#include "DumpMemoryTools.h"

MemoryToolsBase* memoryTools;

// 搜索内存实例
void search(){
    // 设置搜索所有内存区域
    memoryTools->setSearchAll();

    // 搜索4字节数字
    memoryTools->memorySearch("100", MemoryToolsBase::MEM_DWORD);
    // 打印搜索结果
    memoryTools->printResult();
    // 清除搜索结果
    memoryTools->clearResults();

    // 搜索字节数组字节数字(十六进制)
    memoryTools->memorySearch("FF 00 65 5D ?? ?? 22 33", MemoryToolsBase::MEM_BYTES);
    // 打印搜索结果
    memoryTools->printResult();
    // 清除搜索结果
    memoryTools->clearResults();

    // 搜索字符串
    memoryTools->memorySearch("this is test string", MemoryToolsBase::MEM_STRING);
    // 打印搜索结果
    memoryTools->printResult();
    // 清除搜索结果
    memoryTools->clearResults();

    // 搜索4字节数字加偏移，类似字节数组搜索
    memoryTools->memorySearch("100", MemoryToolsBase::MEM_DWORD);
    memoryTools->memoryOffset("50", 4, MemoryToolsBase::MEM_DWORD);
    memoryTools->memoryOffset("1", 8, MemoryToolsBase::MEM_DWORD);
    // 打印搜索结果
    memoryTools->printResult();
    // 清除搜索结果
    memoryTools->clearResults();


}

// dump 内存结构和数据示例
// dump 的内存也可以使用 https://github.com/bbgsm/CEDumpPlugin Dump 插件，配合CE即可分析dump的内存
void dump(){
    // dump 内存到文件
    memoryTools->dumpAllMem("D:\\memDump\\out");

    // 使用DumpMemoryTools工具读取dump的内存
    DumpMemoryTools dumpMemoryTools;
    if (!dumpMemoryTools.init("D:\\memDump\\out\\dict.txt")) {
        printf("Failed to initialize MemoryTools\n");
    }
    // 打印模块信息
    for (auto module : dumpMemoryTools.getModuleList()) {
        printf("module name: %s,baseAddr: %llX,size: %llX\n", module.moduleName, module.baseAddress, module.baseSize);
    }
}

int main() {

    // // 直接读取本机应用内存
    memoryTools = new DirectMemoryTools();
    if (!memoryTools->init("test.exe")) {
       printf("Failed to initialize MemoryTools\n");
    }

    // // Dma读取其他主机内存
    // memoryTools = new DmaMemoryTools();
    // if (!memoryTools->init("test.exe")) {
    //    printf("Failed to initialize MemoryTools\n");
    // }

    // 读取memoryTools->dumpAllMem()下来的内存
    // memoryTools = new DumpMemoryTools();
    // if (!memoryTools->init("D:\\memDump\\dict.txt")) {
    //    printf("Failed to initialize MemoryTools\n");
    //    return 1;
    // }
    search();
    dump();
    return 0;
}