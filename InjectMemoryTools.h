//
// Created by bbgsm on 2024/10/15.
//

#ifndef MODULEMEMORYTOOLS_H
#define MODULEMEMORYTOOLS_H
#include "MemoryToolsBase.h"
#include "Type.h"


class InjectMemoryTools : public MemoryToolsBase {
private:
    mulong memRead(void *buff, mulong len, Addr addr, offset off) override;

    mulong memWrite(void *buff, mulong len, Addr addr, offset off) override;

public:
    ~InjectMemoryTools() override;

    InjectMemoryTools();

    bool init(std::string bm) override;
    void close() override;
    std::vector<PProcess> getProcessList() override;
    void initModuleRegions() override;
    void initMemoryRegions() override;

    int getPID(std::string bm) override; // 获取pid

    Handle createScatter() override;
    void addScatterReadV(Handle handle, void *buff, mulong len, Addr addr) override;
    void addScatterReadV(Handle handle, void *buff, mulong len, Addr addr, offset off) override;
    void executeReadScatter(Handle handle) override;
    void closeScatterHandle(Handle handle) override;
    void execAndCloseScatterHandle(Handle handle) override;
};



#endif //MODULEMEMORYTOOLS_H
