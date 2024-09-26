#include "DumpMemoryTools.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

std::vector<MemoryFile> allMemory; // 所有内存

inline FILE *pteFile = nullptr;
inline FILE *moduleFile = nullptr;
std::mutex dumpMtx;                         // 全局互斥锁
inline std::vector<PProcess> dumpProcesses; // 可用dump进程文件列表
namespace fs = std::filesystem;
std::string ptePath;
std::string modulePath;

// 分割字符串到list
std::vector<std::string> splitString(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream strStream(str);
    std::string token;
    while (std::getline(strStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// 解析一行
void parseLine(const std::string &line, MemoryFile &data) {
    std::vector<std::string> p = splitString(line, '|');
    if (p.empty() || p.size() < 5) {
        return;
    }
    std::string path;
    if (p[0] == "p") {
        data.memType = 1;
    } else if (p[0] == "m") {
        data.memType = 2;
    }
    // data.name = p[1];
    strcpy(data.name, p[1].c_str());
    data.baseAddress = std::stoll(p[2], nullptr, 16);
    data.endAddress = std::stoll(p[3], nullptr, 16);
    data.fileIndex = std::stoll(p[4], nullptr, 16);
    data.size = data.endAddress - data.baseAddress;
}

// 解析进程信息
void DumpMemoryTools::parseProcessInfo(const std::string &line) {
    std::vector<std::string> p = splitString(line, '|');
    if (p.empty() || p.size() < 3) {
        return;
    }
    processName = p[1];
}

// 解析dump内存结构
void DumpMemoryTools::parseFile(const std::string &filePath) {
    memoryRegions.clear();
    moduleRegions.clear();
    std::ifstream file(filePath);
    std::string line;
    std::string pName;
    bool firstLine = true;
    while (std::getline(file, line)) {
        if (firstLine) {
            parseProcessInfo(line);
            firstLine = false;
            continue;
        }
        MemoryFile data;
        parseLine(line, data);
        if (data.memType == 1) {
            MModule module;
            module.baseAddress = data.baseAddress;
            module.baseSize = data.endAddress - module.baseAddress;
            memoryRegions.push_back(module);
        } else if (data.memType == 2) {
            MModule module;
            module.baseAddress = data.baseAddress;
            module.baseSize = data.endAddress - module.baseAddress;
            strcpy(module.moduleName, data.name);
            moduleRegions.push_back(module);
        }
        allMemory.push_back(data);
    }
    // 排序内存区域
    std::ranges::sort(allMemory, [](const MemoryFile &a, const MemoryFile &b) {
        return a.baseAddress < b.baseAddress;
    });
    // for (auto &data : allMemory) {
    //     logDebug("module: %s start: %llX end: %llX\n", data.name, data.baseAddress, data.endAddress);
    // }
}

// 读取可用内存
ulong readAvailableMemory(Addr addr, void *buff, ulong size) {
    for (const auto &memory : allMemory) {
        if (addr >= memory.baseAddress && addr <= memory.endAddress) {
            const ulong minSize = (std::min)(size, memory.size);
            const auto fileOffset = static_cast<mlong>(addr - memory.baseAddress + memory.fileIndex);
            ulong total = 0;
            FILE *file = memory.memType == 1 ? pteFile : moduleFile;
            // 服了，file总是莫名奇妙的被关闭，只能seek失败后重新打开文件一次尝试
            if (_fseeki64(file, fileOffset, SEEK_SET) != 0) {
                logDebug("fseeki64 failed: offset: %llX addr: %llX mtmType: %d file: %llX\n", fileOffset, addr,
                      memory.memType,
                      file);
                if (memory.memType == 1) {
                    pteFile = fopen(ptePath.c_str(), "r+b");
                    file = pteFile;
                } else if (memory.memType == 2) {
                    moduleFile = fopen(modulePath.c_str(), "r+b");
                    file = moduleFile;
                }
                // 重新尝试seek
                if (_fseeki64(file, fileOffset, SEEK_SET) != 0) {
                    return 0;
                }
            }
            fread(buff, minSize, 1, file);
            total += minSize;
            ulong surplus = size - minSize;
            if (surplus > 0) {
                total += readAvailableMemory(addr + minSize, buff, surplus);
            }
            return total;
        }
    }
    // logDebug("no memory available: %llX\n", addr);
    return 0;
}

// 往可用内存写入数据
ulong writeAvailableMemory(Addr addr, void *buff, ulong size) {
    for (const auto &memory : allMemory) {
        if (addr >= memory.baseAddress && addr <= memory.endAddress) {
            const ulong minSize = (std::min)(size, memory.size);
            const auto fileOffset = static_cast<mlong>(addr - memory.baseAddress + memory.fileIndex);
            ulong total = 0;
            FILE *file = memory.memType == 1 ? pteFile : moduleFile;
            if (_fseeki64(file, fileOffset, SEEK_SET) != 0) {
                // 服了，file总是莫名奇妙的被关闭，只能seek失败后重新打开一次文件尝试
                logDebug("fseeki64 failed: offset: %llX addr: %llX mtmType: %d file: %llX\n", fileOffset, addr,
                       memory.memType,
                       file);
                if (memory.memType == 1) {
                    pteFile = fopen(ptePath.c_str(), "r+b");
                    file = pteFile;
                } else if (memory.memType == 2) {
                    moduleFile = fopen(modulePath.c_str(), "r+b");
                    file = moduleFile;
                }
                // 重新尝试seek
                if (_fseeki64(file, fileOffset, SEEK_SET) != 0) {
                    return 0;
                }
            }
            fwrite(buff, minSize, 1, file);
            total += minSize;
            ulong surplus = size - minSize;
            if (surplus > 0) {
                total += writeAvailableMemory(addr + minSize, buff, surplus);
            }
            return total;
        }
    }
    return 0;
}

DumpMemoryTools::DumpMemoryTools() = default;

std::vector<PProcess> DumpMemoryTools::getProcessList() {
    return dumpProcesses;
}

ulong DumpMemoryTools::memRead(void *buff, ulong len, Addr addr, offset off) {
    if (!isAddrValid(addr)) {
        return 0;
    }
    std::lock_guard lock(dumpMtx);
    return readAvailableMemory(addr + off, buff, len);
}

ulong DumpMemoryTools::memWrite(void *buff, ulong len, Addr addr, offset off) {
    std::lock_guard lock(dumpMtx);
    return writeAvailableMemory(addr + off, buff, len);
}

DumpMemoryTools::~DumpMemoryTools() {
    DumpMemoryTools::close();
}

void DumpMemoryTools::initModuleRegions() {
}

void DumpMemoryTools::initMemoryRegions() {
}

bool DumpMemoryTools::init(std::string bm) {
    MemoryToolsBase::init();
    fs::path path = bm;
    if (exists(path)) {
        if (is_regular_file(path)) {
            DumpMemoryTools::close();
            parseFile(bm);
            baseModule = moduleRegions[0];
            std::filesystem::path path(bm);
            std::string directoryPath = path.parent_path().string();
            /*std::string*/
            ptePath = directoryPath + "\\pteMemory.bin";
            pteFile = fopen(ptePath.c_str(), "r+b");
            if (pteFile == nullptr) {
                logDebug("Open pteFile failed\n");
                return false;
            }
            /*std::string*/
            modulePath = directoryPath + "\\moduleMemory.bin";
            moduleFile = fopen(modulePath.c_str(), "r+b");
            if (moduleFile == nullptr) {
                logDebug("Open moduleFile failed\n");
                DumpMemoryTools::close();
                return false;
            }
            PProcess process;
            strcpy(process.processName, processName.c_str());
            bool flag = true;
            for (auto &dumpProcess : dumpProcesses) {
                if (dumpProcess.processPath == bm) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                this->processID = dumpProcesses.size() + 1;
                process.processID = dumpProcesses.size() + 1;
                dumpProcesses.push_back(process);
            }
            return true;
        } else if (is_directory(path)) {
            dumpProcesses.clear();
            std::string target_file = "dict.txt";
            std::string line;
            for (const auto &entry : fs::directory_iterator(bm)) {
                if (is_directory(entry)) {
                    fs::path sub_dir_path = entry.path();
                    if (exists(sub_dir_path / target_file)) {
                        std::ifstream file(sub_dir_path / target_file);
                        if (std::getline(file, line)) {
                            std::vector<std::string> p = splitString(line, '|');
                            if (p.empty() || p.size() < 2) {
                                continue;
                            }
                            PProcess process;
                            strcpy(process.processName, p[1].c_str());
                            strcpy(process.processPath, (sub_dir_path / target_file).string().c_str());
                            process.processID = dumpProcesses.size() + 1;
                            dumpProcesses.push_back(process);
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

void DumpMemoryTools::close() {
    if (pteFile != nullptr) {
        fclose(pteFile);
        pteFile = nullptr;
    }
    if (moduleFile != nullptr) {
        fclose(moduleFile);
        moduleFile = nullptr;
    }
}

int DumpMemoryTools::getPID(std::string bm) {
    return 0;
}

std::vector<MemoryFile> &DumpMemoryTools::getAllMemory() {
    return allMemory;
}

Handle DumpMemoryTools::createScatter() {
    return nullptr;
}

void DumpMemoryTools::addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off) {
    readV(buff, len, addr, off);
}

void DumpMemoryTools::executeReadScatter(Handle handle) {
}

void DumpMemoryTools::closeScatterHandle(Handle handle) {
}
