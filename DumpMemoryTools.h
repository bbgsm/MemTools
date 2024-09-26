#pragma once
#include <string>
#include <vector>
#include "MemoryToolsBase.h"
#include "Type.h"


struct MemoryFile {
    char name[MAX_PATH] = {0};
    int memType = 0;
    Addr baseAddress = 0;
    Addr endAddress = 0;
    ulong fileIndex = 0;
    ulong size = 0;
};

// 读取/写入Dump的内存
class DumpMemoryTools : public MemoryToolsBase {
private:
    ulong memRead(void *buff, ulong len, Addr addr, offset off) override;

    ulong memWrite(void *buff, ulong len, Addr addr, offset off) override;

public:
    ~DumpMemoryTools() override;

    void parseProcessInfo(const std::string &line); // 解析进程信息
    void parseFile(const std::string &filePath); // 解析dump内存结构

    DumpMemoryTools();


    void initModuleRegions() override;
    void initMemoryRegions() override;
    bool init(std::string bm) override; // 传入路径为文件夹，加载文件夹下dump的数据列表，传入dump结构文件地址，直接加载dump的内存
    void close() override;

    int getPID(std::string bm) override; // 获取pid
    std::vector<PProcess> getProcessList() override;
    std::vector<MemoryFile> &getAllMemory();

    Handle createScatter() override;
    void addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off = 0) override;
    void executeReadScatter(Handle handle) override;
    void closeScatterHandle(Handle handle) override;

};
