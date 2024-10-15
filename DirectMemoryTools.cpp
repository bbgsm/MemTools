#ifdef _WIN32 // Windows
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

mulong DirectMemoryTools::memRead(void *buff, mulong len, Addr addr, offset off) {
    if (buff == nullptr || len == 0) return 0;
    // 检查内存是否有效再读取
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(addr + off), &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (
                                            PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ |
                                            PAGE_EXECUTE_READWRITE))) {
            return ReadProcessMemory(hProcess, (LPVOID) (addr + off), buff, len, nullptr);
        } else {
            logDebug("Error: Memory not writable or not committed\n");
        }
    } else {
        logDebug("Error: VirtualQueryEx failed\n");
    }
    return 0;
}

mulong DirectMemoryTools::memWrite(void *buff, mulong len, Addr addr, offset off) {
    // 检查内存是否有效再写入
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(addr + off), &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) {
            return WriteProcessMemory(hProcess, (LPVOID) (addr + off), buff, len, nullptr);
        } else {
            logDebug("Error: Memory not writable or not committed\n");
        }
    } else {
        logDebug("Error: VirtualQueryEx failed\n");
    }
    return 0;
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
            mModule.baseAddress = (Addr) modInfo.lpBaseOfDll;
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

Handle DirectMemoryTools::createScatter() {
    return nullptr;
}

void DirectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr) {
    readV(buff, len, addr);
}

void DirectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off) {
    readV(buff, len, addr, off);
}

void DirectMemoryTools::executeReadScatter(Handle handle) {
}

void DirectMemoryTools::closeScatterHandle(Handle handle) {
}

void DirectMemoryTools::execAndCloseScatterHandle(Handle handle) {
}

#else // Linux
#include <fstream>
#include <sstream>
#include "DirectMemoryTools.h"
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <future>
#include <dirent.h>
#include <cstring>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <vector>

#define DMA_DEBUG false

pid_t hProcess;



std::string trimAndGetModuleName(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return ""; // No content
    size_t last = str.find_last_not_of(' ');
    std::string trimmed = str.substr(first, (last - first + 1));
    size_t lastSlash = trimmed.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return trimmed.substr(lastSlash + 1);
    }
    return trimmed;
}

mulong DirectMemoryTools::memRead(void *buff, mulong len, Addr addr, offset off) {
    memset(buff, 0, len);
    Addr lastAddr = addr + off;
    iovec iov_ReadBuffer{}, iov_ReadOffset{};
    iov_ReadBuffer.iov_base = buff;
    iov_ReadBuffer.iov_len = len;
    iov_ReadOffset.iov_base = (void *) lastAddr;
    iov_ReadOffset.iov_len = len;
    long int size = syscall(SYS_process_vm_readv, processID, &iov_ReadBuffer, 1, &iov_ReadOffset, 1, 0);
    return size == -1 ? 0 : size;
}

mulong DirectMemoryTools::memWrite(void *buff, mulong len, Addr addr, offset off) {
    iovec iov_WriteBuffer{}, iov_WriteOffset{};
    iov_WriteBuffer.iov_base = buff;
    iov_WriteBuffer.iov_len = len;
    iov_WriteOffset.iov_base = (void *) (addr + off);
    iov_WriteOffset.iov_len = len;
    // 大小
    long int size = syscall(SYS_process_vm_writev, processID, &iov_WriteBuffer, 1, &iov_WriteOffset, 1, 0);
    return size == -1 ? 0 : size;
}

DirectMemoryTools::~DirectMemoryTools() {
    DirectMemoryTools::close();
}

bool DirectMemoryTools::init(std::string bm) {
    MemoryToolsBase::init();
    int pid = this->getPID(bm);
    if (pid > 0) {
        hProcess = pid;
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
}


std::vector<PProcess> DirectMemoryTools::getProcessList() {
    std::vector<PProcess> processes;
    DIR *dir = opendir("/proc");
    if (dir == nullptr) {
        return processes;
    }
    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);
            if (pid > 0) {
                std::string cmdline_path = "/proc/" + std::string(entry->d_name) + "/cmdline";
                std::ifstream cmdline_file(cmdline_path);
                if (!cmdline_file.is_open()) {
                    continue;
                }
                std::string process_name;
                std::getline(cmdline_file, process_name, '\0');
                if (!process_name.empty()) {
                    size_t param_pos = process_name.find(' ');
                    if (param_pos != std::string::npos) {
                        process_name = process_name.substr(0, param_pos);
                    }
                    size_t last_slash_pos = process_name.find_last_of('/');
                    if (last_slash_pos != std::string::npos) {
                        process_name = process_name.substr(last_slash_pos + 1);
                    }
                    // printf("process_name: %s\n",process_name.c_str());
                    PProcess process;
                    strncpy(process.processName, process_name.c_str(), sizeof(process.processName) - 1);
                    process.processName[sizeof(process.processName) - 1] = '\0';
                    process.processID = pid;
                    processes.push_back(process);
                }
            }
        }
    }
    closedir(dir);
    return processes;
}

DirectMemoryTools::DirectMemoryTools() = default;

int DirectMemoryTools::getPID(std::string bm) {
    std::vector<PProcess> processes = getProcessList();
    for (const auto &process : processes) {
        if (process.processName == bm) {
            return process.processID;
        }
    }
    return -1;
}

void DirectMemoryTools::initModuleRegions() {
    std::string maps_path = std::string("/proc/") + std::to_string(hProcess) + "/maps";
    std::ifstream maps_file(maps_path);
    std::string line;
    while (std::getline(maps_file, line)) {
        Addr start, end;
        std::istringstream lineStream(line);
        std::string addresses, permissions, offset, dev, inode, moduleName;
        lineStream >> addresses >> permissions >> offset >> dev >> inode;
        std::getline(lineStream, moduleName);
        std::istringstream addressStream(addresses);
        std::string startAddressStr, endAddressStr;
        std::getline(addressStream, startAddressStr, '-');
        std::getline(addressStream, endAddressStr, '-');
        start = std::stoul(startAddressStr, nullptr, 16);
        end = std::stoul(endAddressStr, nullptr, 16);
        if(!moduleName.empty()) {
            moduleName = trimAndGetModuleName(moduleName);
        }
        if (permissions.find('r') != std::string::npos) {
            MModule mModule;
            mModule.baseAddress = start;
            mModule.baseSize = end - start;
            strcpy(mModule.moduleName,moduleName.c_str());
            moduleRegions.push_back(mModule);
        }
    }
}

void DirectMemoryTools::initMemoryRegions() {
    // std::string maps_path = std::string("/proc/") + std::to_string(hProcess) + "/maps";
    // std::ifstream maps_file(maps_path);
    // std::string line;
    // while (std::getline(maps_file, line)) {
    //     std::istringstream iss(line);
    //     Addr start, end;
    //     char dash;
    //     iss >> std::hex >> start >> dash >> end;
    //     std::string perms;
    //     iss >> perms;
    //     if (perms.find('r') != std::string::npos) {
    //         memoryRegions.push_back({"", start, end - start});
    //     }
    // }
}

Handle DirectMemoryTools::createScatter() {
    return nullptr;
}

void DirectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr){
    readV(buff, len, addr);
}

void DirectMemoryTools::addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off) {
    readV(buff, len, addr, off);
}

void DirectMemoryTools::executeReadScatter(Handle handle) {
}

void DirectMemoryTools::closeScatterHandle(Handle handle) {
}

void DirectMemoryTools::execAndCloseScatterHandle(Handle handle){
}
#endif
