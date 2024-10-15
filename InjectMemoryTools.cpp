//
// Created by bbgsm on 2024/10/15.
//

#ifdef _WIN32 // Windows
#include "InjectMemoryTools.h"
#include <winsock2.h>
#include <windows.h>

#include <cstdio>
#include <future>
#include <psapi.h>
#include <string>
#include <TlHelp32.h>
#include <mutex>

#define DMA_DEBUG false


HANDLE chProcess;
std::mutex mMtx; // 全局互斥锁

mulong InjectMemoryTools::memRead(void *buff, mulong len, Addr addr, offset off) {
    if (buff == nullptr || len == 0) return 0;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(chProcess, reinterpret_cast<LPCVOID>(addr + off), &mbi, sizeof(mbi))) {
        bool canRead = mbi.State == MEM_COMMIT && (
            mbi.Protect == PAGE_READONLY ||
            mbi.Protect == PAGE_READWRITE ||
            mbi.Protect == PAGE_EXECUTE_READ ||
            mbi.Protect == PAGE_EXECUTE_READWRITE ||
            mbi.Protect == PAGE_WRITECOPY ||
            mbi.Protect == PAGE_EXECUTE_WRITECOPY
        );

        if (canRead) {
            std::memcpy(buff, reinterpret_cast<void *>(addr + off), len);
            return len;
        } else {
            logDebug("Error: Memory not readable or not committed\n");
        }
    } else {
        logDebug("Error: VirtualQueryEx failed\n");
    }
    return 0;
}

mulong InjectMemoryTools::memWrite(void *buff, mulong len, Addr addr, offset off) {
    if (buff == nullptr || len == 0) return 0;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(chProcess, reinterpret_cast<LPCVOID>(addr + off), &mbi, sizeof(mbi))) {
        bool canWrite = mbi.State == MEM_COMMIT && (
            mbi.Protect == PAGE_READWRITE ||
            mbi.Protect == PAGE_EXECUTE_READWRITE ||
            mbi.Protect == PAGE_WRITECOPY ||
            mbi.Protect == PAGE_EXECUTE_WRITECOPY
        );
        if (canWrite) {
            std::memcpy(reinterpret_cast<void *>(addr + off), buff, len);
            return len;
        } else {
            logDebug("Error: Memory not writable or not committed\n");
        }
    } else {
        logDebug("Error: VirtualQueryEx failed\n");
    }
    return 0;
}


InjectMemoryTools::~InjectMemoryTools() {
    InjectMemoryTools::close();
}

bool InjectMemoryTools::init(std::string bm) {
    MemoryToolsBase::init();
    chProcess = GetCurrentProcess();
    processID = static_cast<int>(GetCurrentProcessId());

    char szProcessName[MAX_PATH] = {0};
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule != nullptr) {
        GetModuleBaseName(chProcess, hModule, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        processName = szProcessName;
    }
    initModuleRegions();
    initMemoryRegions();
    baseModule = getModule(processName);
    return true;
}

void InjectMemoryTools::close() {
    CloseHandle(chProcess);
}

std::vector<PProcess> InjectMemoryTools::getProcessList() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        logDebug("Failed to CreateToolhelp32Snapshot\n");
        return {};
    }
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    std::vector<PProcess> processes;
    // 遍历进程列表
    if (Process32First(hSnapshot, &processEntry)) {
        do {
            if (processEntry.th32ProcessID == processID) {
                PProcess process;
                strcpy(process.processName, processEntry.szExeFile);
                process.processID = processEntry.th32ProcessID;
                process.processID = processEntry.th32ParentProcessID;
                processes.push_back(process);
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }
    CloseHandle(hSnapshot);
    return processes;
}

InjectMemoryTools::InjectMemoryTools() = default;

int InjectMemoryTools::getPID(std::string bm) {
    return processID;
}

void InjectMemoryTools::initModuleRegions() {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(chProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            MODULEINFO modInfo;
            char szModName[MAX_PATH] = {0};
            if (!GetModuleInformation(chProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                continue;
            }
            if (!GetModuleBaseName(chProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                continue;
            }
            MModule mModule;
            mModule.baseAddress = (Addr) modInfo.lpBaseOfDll;
            mModule.baseSize = modInfo.SizeOfImage;
            strcpy(mModule.moduleName, szModName);
            moduleRegions.push_back(mModule);
        }
    }
}

void InjectMemoryTools::initMemoryRegions() {
    // 遍历整个虚拟地址空间
    Addr address = 0;
    MEMORY_BASIC_INFORMATION mbi;
    while (VirtualQueryEx(chProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
        bool isModuleRegion = false;
        // 检查该内存区域是否属于任何模块
        for (const auto &moduleRegion: moduleRegions) {
            if (reinterpret_cast<Addr>(mbi.BaseAddress) >= moduleRegion.baseAddress &&
                reinterpret_cast<Addr>(mbi.BaseAddress) < moduleRegion.baseAddress + moduleRegion.baseSize) {
                isModuleRegion = true;
                break;
            }
        }
        // 如果不属于任何模块，则将其添加到非模块区域列表中
        if (!isModuleRegion && mbi.State == MEM_COMMIT) {
            memoryRegions.push_back({"", (Addr) mbi.BaseAddress, mbi.RegionSize});
        }
        // 移动到下一个内存区域
        address = reinterpret_cast<Addr>(mbi.BaseAddress) + mbi.RegionSize;
    }
}

Handle InjectMemoryTools::createScatter() {
    return nullptr;
}

void InjectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr) {
    readV(buff, len, addr);
}

void InjectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off) {
    readV(buff, len, addr, off);
}

void InjectMemoryTools::executeReadScatter(Handle handle) {
}

void InjectMemoryTools::closeScatterHandle(Handle handle) {
}

void InjectMemoryTools::execAndCloseScatterHandle(Handle handle) {
}

#else // Linux

mulong InjectMemoryTools::memRead(void *buff, mulong len, Addr addr, offset off) {
    return 0;
}

mulong InjectMemoryTools::memWrite(void *buff, mulong len, Addr addr, offset off) {
    return 0;
}


InjectMemoryTools::~InjectMemoryTools() {
}

bool InjectMemoryTools::init(std::string bm) {
    return false;
}

void InjectMemoryTools::close() {
}

std::vector<PProcess> InjectMemoryTools::getProcessList() {
    return {};
}

InjectMemoryTools::InjectMemoryTools() = default;

int InjectMemoryTools::getPID(std::string bm) {
    return -1;
}

void InjectMemoryTools::initModuleRegions() {
}

void InjectMemoryTools::initMemoryRegions() {
}

Handle InjectMemoryTools::createScatter() {
    return nullptr;
}

void InjectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr) {
}

void InjectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off) {
}

void InjectMemoryTools::executeReadScatter(Handle handle) {
}

void InjectMemoryTools::closeScatterHandle(Handle handle) {
}

void InjectMemoryTools::execAndCloseScatterHandle(Handle handle) {
}
#endif
