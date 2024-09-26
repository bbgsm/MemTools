#include "DirectMemoryTools.h"
#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <future>
#include <psapi.h>
#include <string>
#include <TlHelp32.h>

#define DMA_DEBUG false


HANDLE hProcess;

ulong DirectMemoryTools::memRead(void *buff, ulong len, Addr addr, offset off) {
    return ReadProcessMemory(hProcess, (LPVOID)(addr + off), buff, len, nullptr);
}

ulong DirectMemoryTools::memWrite(void *buff, ulong len, Addr addr, offset off) {
    return WriteProcessMemory(hProcess, (LPVOID)(addr + off), buff, len, nullptr);
}

DirectMemoryTools::~DirectMemoryTools() {
    DirectMemoryTools::close();
}

bool DirectMemoryTools::init(std::string bm) {
    MemoryToolsBase::init();
    int pid = this->getPID(bm);
    if (pid > 0) {
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        processID = pid;
        processName = bm;
        initModuleRegions();
        initMemoryRegions();
        baseModule = getModule(bm);
        return true;
    }
    return false;
}

void DirectMemoryTools::close() {
    CloseHandle(hProcess);
}

std::vector<PProcess> DirectMemoryTools::getProcessList() {
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
            PProcess process;
            strcpy(process.processName, processEntry.szExeFile);
            process.processID = processEntry.th32ProcessID;
            process.processID = processEntry.th32ParentProcessID;
            processes.push_back(process);
        } while (Process32Next(hSnapshot, &processEntry));
    }
    CloseHandle(hSnapshot);
    return processes;
}

DirectMemoryTools::DirectMemoryTools() = default;

int DirectMemoryTools::getPID(std::string bm) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        logDebug("Failed to CreateToolhelp32Snapshot\n");
        return -1;
    }
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    // 遍历进程列表
    if (Process32First(hSnapshot, &processEntry)) {
        do {
            std::string processName = processEntry.szExeFile;
            if (processName == bm) {
                CloseHandle(hSnapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }
    CloseHandle(hSnapshot);
    return -1;
}

void DirectMemoryTools::initModuleRegions() {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            MODULEINFO modInfo;
            char szModName[MAX_PATH] = {0};
            if (!GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                continue;
            }
            if (!GetModuleBaseName(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                continue;
            }
            MModule mModule;
            mModule.baseAddress = (Addr)modInfo.lpBaseOfDll;
            mModule.baseSize = modInfo.SizeOfImage;
            strcpy(mModule.moduleName, szModName);
            moduleRegions.push_back(mModule);
        }
    }
}

void DirectMemoryTools::initMemoryRegions() {
    // 遍历整个虚拟地址空间
    Addr address = 0;
    MEMORY_BASIC_INFORMATION mbi;
    while (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
        bool isModuleRegion = false;
        // 检查该内存区域是否属于任何模块
        for (const auto &moduleRegion : moduleRegions) {
            if (reinterpret_cast<Addr>(mbi.BaseAddress) >= moduleRegion.baseAddress &&
                reinterpret_cast<Addr>(mbi.BaseAddress) < moduleRegion.baseAddress + moduleRegion.baseSize) {
                isModuleRegion = true;
                break;
            }
        }
        // 如果不属于任何模块，则将其添加到非模块区域列表中
        if (!isModuleRegion && mbi.State == MEM_COMMIT) {
            memoryRegions.push_back({"", (Addr)mbi.BaseAddress, mbi.RegionSize});
        }
        // 移动到下一个内存区域
        address = reinterpret_cast<Addr>(mbi.BaseAddress) + mbi.RegionSize;
    }
}

Handle DirectMemoryTools::createScatter() {
    return nullptr;
}

void DirectMemoryTools::addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off) {
    readV(buff, len, addr, off);
}

void DirectMemoryTools::executeReadScatter(Handle handle) {
}

void DirectMemoryTools::closeScatterHandle(Handle handle) {
}
