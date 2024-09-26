#pragma once

#include "Type.h"
#include <list>
#include <vector>
#include <cstdio>
#include <string>

// 开启debug 日志
// #define DEBUG_INFO
// 开启info日志
#define INFO_LOG
#ifdef INFO_LOG
#define logInfo(fmt, ...) std::printf(fmt, ##__VA_ARGS__)
#else
#define logInfo
#endif

#ifdef DEBUG_INFO
#define logDebug(fmt, ...) std::printf(fmt, ##__VA_ARGS__)
#else
#define logDebug
#endif

#define MAX_PATH 256

// 模块
struct MModule {
    char moduleName[MAX_PATH] = {0};
    Addr baseAddress = 0;
    Addr baseSize = 0;
};

// 进程
struct PProcess {
    char processName[MAX_PATH] = {0};
    char processPath[MAX_PATH] = {0};
    int processID = 0;
    int pProcessID = 0;
};

// 内存操作工具，需要子类来实例化，并且实现虚函数才能使用
class MemoryToolsBase {
public:
    // 搜索数据类型
    enum Type {
        MEM_DWORD = 0,   // 4位
        MEM_QWORD = 1,   // 8位
        MEM_FLOAT = 2,   // float
        MEM_DOUBLE = 3,  // double
        MEM_BYTE = 4,    // 1位
        MEM_BYTES = 5,   // bytes
        MEM_STRING = 6   // string
    };

    const int ISize = sizeof(int);
    const int LSize = sizeof(mlong);
    const int FSize = sizeof(float);
    const int DSize = sizeof(double);
    const int BSize = sizeof(mbyte);
    const int CSize = sizeof(char);
    const int ULSize = sizeof(ulong);
    const int USSize = sizeof(ushort);
    const int I16LSize = sizeof(int16);
    const int ADDRSize = sizeof(Addr);

    int memPageSize = 0x1000;    // 内存页大小

    int processID = -1;       // 进程pid
    std::string processName;  // 进程名
    MModule baseModule;       // 进程入口地址和大小
    std::vector<MModule> moduleRegions; // 模块内粗范围
    std::vector<MModule> memoryRegions; // 其他内存范围

    // 搜索结果
    std::list<RADDR> *resultList = new std::list<RADDR>;
    // 搜索内存范围
    std::list<RADDR> *searchRangeList = new std::list<RADDR>;

protected:
    MemoryToolsBase();

private:
    virtual ulong memRead(void *buff, ulong len, Addr addr, offset off) = 0; // 读取内存

    virtual ulong memWrite(void *buff, ulong len, Addr addr, offset off) = 0; // 写入内存

public:
    /******** 子类必须实现的方法 *********/
    virtual void initModuleRegions() = 0;  // 初始化模块内存区域
    virtual void initMemoryRegions() = 0;  // 初始化内存区域
    virtual int getPID(std::string bm) = 0; // 获取pid
    virtual bool init(std::string bm) = 0;  // 初始化工具
    virtual void close() = 0;               // 关闭工具
    virtual Handle createScatter() = 0;     // 创建分散读取handle
    virtual void addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off = 0) = 0; // 新增分散读取
    virtual void executeReadScatter(Handle handle) = 0;  // 执行分散读取
    virtual void closeScatterHandle(Handle handle) = 0;  // 关闭分散读取
    virtual std::vector<PProcess> getProcessList() = 0;  // 获取进程列表
    virtual ~MemoryToolsBase();
    /******** 子类必须实现的方法 *********/

    void init();                             // 初始化(子类初始化时也调用一下它)
    void setPID(int pid);

    MModule getModule(std::string modName);  // 获取模块信息
    Addr getModuleAddr(std::string modName); // 获取模块地址
    ulong getModuleSize(std::string modName);// 获取模块大小

    int readI(Addr addr, offset off = 0);     // 读取int(4位)
    int16 readI16(Addr addr, offset off = 0); // 读取int16
    bool readZ(Addr addr, offset off = 0);    // 读取bool
    ulong readUL(Addr addr, offset off = 0);  // 读取ulong(8位)
    Addr readA(Addr addr, offset off = 0);    // 读取Addr(8位)
    Addr readP(Addr addr, offset off = 0);    // 读取指针
    mlong readL(Addr addr, offset off = 0);   // 读取long(8位)
    float readF(Addr addr, offset off = 0);   // 读取float
    double readD(Addr addr, offset off = 0);  // 读取double
    mbyte readB(Addr addr, offset off = 0);   // 读取byte
    char readC(Addr addr, offset off = 0);    // 读取char
    ushort readUS(Addr addr, offset off = 0); // 读取byte

    ulong readV(void *buff, ulong len, Addr addr, offset off = 0); //读取指定大小的值

    ulong writeI(int value, Addr addr, offset off = 0);    // 写入int
    ulong writeL(mlong value, Addr addr, offset off = 0);  // 写入long
    ulong writeF(float value, Addr addr, offset off = 0);  // 写入float
    ulong writeD(double value, Addr addr, offset off = 0); // 写入double
    ulong writeB(mbyte value, Addr addr, offset off = 0);  // 写入byte
    ulong writeV(void *buff, ulong len, Addr addr, offset off = 0); // 写入buffer

    Addr getPointers(Addr addr, int p_size, int *offsets);    // 获取多级偏移地址
    int getPointersValue(Addr addr, void *buff, ulong len, int p_size, ulong *offsets); // 获取多级偏移值

    static bool isAddrValid(Addr addr); // 判断内存地址是否有效

    /********** 搜索内存功能 ***************/
    void addSearchModule(std::string modName);         // 新增模块搜索范围
    void setSearchAll();                               // 设置搜索所有
    void addSearchRang(Addr startAddr, Addr endAddr);  // 手动添加搜索范围

    int searchFloat(float from_value, float to_value);
    int searchDword(int from_value, int to_value);
    int searchQword(mlong from_value, mlong to_value);
    int searchDouble(double from_value, double to_value);
    int searchByte(mbyte from_value, mbyte to_value);
    int searchBytes(std::string values);
    int searchString(const std::string &values);
    int searchOffset(const mbyte *from_value, const mbyte *to_value, ulong offset, Type type, ulong len);

    int memorySearch(std::string value, Type type); // 类型搜索
    int memoryOffset(std::string value, ulong offset, Type type); // 搜索偏移
    int rangeMemorySearch(std::string from_value, std::string to_value, Type type); // 范围搜索
    int rangeMemoryOffset(std::string from_value, std::string to_value, ulong offset, Type type); // 范围偏移
    void memoryWrite(std::string value, ulong offset, Type type); // 批量往搜索结果写入数据

    void printResult() const;       // 打印搜索结果
    void printSearchRange() const;  // 打印搜索范围
    void clearResults() const;      // 清除搜索结果
    void clearSearchRange() const;  // 清理搜索范围

    std::list<RADDR> *getResults();             // 获取搜索结果
    std::list<RADDR> *getSearchRange() const;   // 获取搜索范围
    /********** 搜索内存功能 ***************/

    /********** 持久化内存功能 ***************/
    ulong dumpMem(Addr beginAddr, Addr endAddr, FILE *dumpfile); // 持久化内存空间数据
    ulong dumpMem(std::string dumpModule, std::string filePath); // 持久化内存空间数据
    ulong dumpAllMem(const std::string &dirPath);                // 持久化内存结构以及数据
    /********** 持久化内存功能 ***************/

    int getResCount() const;   // 获取搜索的数量
    int getProcessPid() const; // 获取已设置的进程pid
    Addr getBaseAddr() const;  // 获取入口地址(基址)

    std::vector<MModule> getModuleList() const; // 获取模块列表

    static std::string addrToHex(Addr num);  // 地址转字符串
};
