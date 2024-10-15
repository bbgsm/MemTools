//
// Created by bbgsm on 2024/10/15.
//

#include <iostream>
#include <windows.h>
#include "InjectMemoryTools.h"


// 注入读取内存示例

MemoryToolsBase *memoryTools;

void injectDll() {

    // 注入读取内存工具
    memoryTools = new InjectMemoryTools();
    if (!memoryTools->init("test.exe")) {
        return;
    }
    AllocConsole();
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
    freopen("conout$", "w+t", stdout);
    freopen("conin$", "r+t", stdin);

    // 设置搜索所有内存区域
    memoryTools->setSearchAll();

    // 搜索4字节数字
    memoryTools->memorySearch("100", MemoryToolsBase::MEM_DWORD);
    // 打印搜索结果
    memoryTools->printResult();
    logInfo("Result Count: %d\n", memoryTools->getResCount());
    // 清除搜索结果
    memoryTools->clearResults();

    // delete memoryTools;
}

/**
 * Dll注入入口函数
 */
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            injectDll();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default: break;
    }
    return TRUE;
}
