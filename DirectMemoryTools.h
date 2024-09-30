//
// Created by bbgsm on 2024/9/5.
//

#ifndef DIRECTMEMORYTOOLS_H
#define DIRECTMEMORYTOOLS_H
#include <string>
#include <vector>
#include "MemoryToolsBase.h"
#include "Type.h"

// 直接读取/操作进程内存
class DirectMemoryTools : public MemoryToolsBase {
private:
    mulong memRead(void *buff, mulong len, Addr addr, offset off) override;

    mulong memWrite(void *buff, mulong len, Addr addr, offset off) override;

public:
    ~DirectMemoryTools() override;

    DirectMemoryTools();

    bool init(std::string bm) override;
    void close() override;
    std::vector<PProcess> getProcessList() override;
    void initModuleRegions() override;
    void initMemoryRegions() override;

    int getPID(std::string bm) override; // 获取pid

    Handle createScatter() override;
    void addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off = 0) override;
    void executeReadScatter(Handle handle) override;
    void closeScatterHandle(Handle handle) override;
};


#endif
