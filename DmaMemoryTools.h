#pragma once

#include "Type.h"
#include "MemoryToolsBase.h"
#include "vmmdll.h"
#include <string>

// Dma硬件读取/操作进程内存
class DmaMemoryTools : public MemoryToolsBase {
private:
    ulong memRead(void *buff, ulong len, Addr addr, offset off) override;

    ulong memWrite(void *buff, ulong len, Addr addr, offset off) override;

public:
    VMM_HANDLE vHandle = nullptr;

    ~DmaMemoryTools() override;

    DmaMemoryTools();

    bool FixCr3();
    bool SetFPGA();
    bool init(std::string bm) override;
    void close() override;
    std::vector<PProcess> getProcessList() override;
    void initModuleRegions() override;
    void initMemoryRegions() override;

    int getPID(std::string bm) override; // 获取pid

    Handle createScatter() override;
    void addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off = 0) override;
    void executeReadScatter(Handle handle) override;
    void closeScatterHandle(Handle handle) override;
};
