#include "MemoryToolsBase.h"
#include <cmath>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
// 此头文件不要删除
#include <winsock2.h>
#include <windows.h>

const char *proText = "-/|\\";

MemoryToolsBase::~MemoryToolsBase() {
    if (resultList != nullptr) {
        delete resultList;
        resultList = nullptr;
    }
    if (searchRangeList != nullptr) {
        delete searchRangeList;
        searchRangeList = nullptr;
    }
}

MemoryToolsBase::MemoryToolsBase() = default;

void MemoryToolsBase::init() {
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    memPageSize = systemInfo.dwPageSize;
}

void MemoryToolsBase::addSearchModule(std::string modName) {
    Addr baseAddress = getModuleAddr(modName);
    Addr baseSize = getModuleSize(modName);
    addSearchRang(baseAddress, baseAddress + baseSize);
}

void MemoryToolsBase::setSearchAll() {
    for (const auto &module : moduleRegions) {
        addSearchRang(module.baseAddress, module.baseAddress + module.baseSize);
    }
    for (const auto &memory : memoryRegions) {
        addSearchRang(memory.baseAddress, memory.baseAddress + memory.baseSize);
    }
}

void MemoryToolsBase::setPID(int pid) {
    processID = pid;
}

Addr MemoryToolsBase::getPointers(Addr addr, int p_size, int *offsets) {
    Addr temp = 0;
    readV(&temp, ULSize, addr);
    for (int i = 0; i < p_size; i++) {
        readV(&temp, ULSize, temp + offsets[i]);
    }
    return temp;
}

int MemoryToolsBase::getPointersValue(Addr addr, void *buff, ulong len, int p_size, ulong *offsets) {
    Addr temp = 0;
    readV(&temp, LSize, addr);
    for (int i = 0; i < p_size; i++) {
        if (i == (p_size - 1)) {
            return readV(buff, len, temp + offsets[i]);
        } else {
            readV(&temp, LSize, temp + offsets[i]);
        }
    }
    return 0;
}

MModule MemoryToolsBase::getModule(std::string modName) {
    for (const auto &module : moduleRegions) {
        if (modName == module.moduleName) {
            return module;
        }
    }
    return {};
}

Addr MemoryToolsBase::getModuleAddr(std::string modName) {
    for (const auto &module : moduleRegions) {
        if (modName == module.moduleName) {
            return module.baseAddress;
        }
    }
    return 0;
}

ulong MemoryToolsBase::getModuleSize(std::string modName) {
    for (const auto &module : moduleRegions) {
        if (modName == module.moduleName) {
            return module.baseSize;
        }
    }
    return 0;
}

int MemoryToolsBase::readI(Addr addr, offset off) {
    int temp = 0;
    memRead(&temp, ISize, addr, off);
    return temp;
}

int16 MemoryToolsBase::readI16(Addr addr, offset off) {
    int16 temp = 0;
    memRead(&temp, I16LSize, addr, off);
    return temp;
}

bool MemoryToolsBase::readZ(Addr addr, offset off) {
    return readI(addr, off) > 0;
}

ulong MemoryToolsBase::readUL(Addr addr, offset off) {
    ulong temp = 0;
    memRead(&temp, ULSize, addr, off);
    return temp;
}

Addr MemoryToolsBase::readA(Addr addr, offset off) {
    Addr temp = 0;
    memRead(&temp, ADDRSize, addr, off);
    return temp;
}

Addr MemoryToolsBase::readP(Addr addr, offset off) {
    return readUL(addr, off);
}

mlong MemoryToolsBase::readL(Addr addr, offset off) {
    mlong temp = 0;
    memRead(&temp, LSize, addr, off);
    return temp;
}

float MemoryToolsBase::readF(Addr addr, offset off) {
    float temp = 0;
    memRead(&temp, FSize, addr, off);
    return temp;
}

double MemoryToolsBase::readD(Addr addr, offset off) {
    double temp = 0;
    memRead(&temp, DSize, addr, off);
    return temp;
}

mbyte MemoryToolsBase::readB(Addr addr, offset off) {
    mbyte temp = 0;
    memRead(&temp, BSize, addr, off);
    return temp;
}

char MemoryToolsBase::readC(Addr addr, offset off) {
    char temp = 0;
    memRead(&temp, CSize, addr, off);
    return temp;
}

ushort MemoryToolsBase::readUS(Addr addr, offset off) {
    ushort temp = 0;
    memRead(&temp, USSize, addr, off);
    return temp;
}

ulong MemoryToolsBase::readV(void *buff, ulong len, Addr addr, offset off) {
    return memRead(buff, len, addr, off);
}

ulong MemoryToolsBase::writeI(int value, Addr addr, offset off) {
    return memWrite(&value, ISize, addr, off);
}

ulong MemoryToolsBase::writeL(mlong value, Addr addr, offset off) {
    return memWrite(&value, LSize, addr, off);
}

ulong MemoryToolsBase::writeF(float value, Addr addr, offset off) {
    return memWrite(&value, FSize, addr, off);
}

ulong MemoryToolsBase::writeD(double value, Addr addr, offset off) {
    return memWrite(&value, DSize, addr, off);
}

ulong MemoryToolsBase::writeB(mbyte value, Addr addr, offset off) {
    return memWrite(&value, BSize, addr, off);
}

ulong MemoryToolsBase::writeV(void *buff, ulong len, Addr addr, offset off) {
    return memWrite(buff, len, addr, off);
}

bool MemoryToolsBase::isAddrValid(Addr addr) {
    if (addr < 0x00010000 || addr > 0x7FFFFFFEFFFFLL) {
        return false;
    }
    return true;
}

int MemoryToolsBase::searchFloat(float from_value, float to_value) {
    int cs = 0;
    RADDR pmap{0, 0};
    int c;
    int buffSize = memPageSize / FSize;
    auto *buff = new float[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i += 1) {
                if (buff[i] >= from_value && buff[i] <= to_value) {
                    cs += 1;
                    pmap.addr = pmapsit->addr + (j * memPageSize) + (i * FSize);
                    resultList->push_back(pmap);
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}


int MemoryToolsBase::searchDword(int from_value, int to_value) {
    int cs = 0;
    RADDR pmap{0, 0};
    int c;
    int buffSize = memPageSize / ISize;
    int *buff = new int[buffSize]; // 缓冲区
    float len = 0;
    auto max = static_cast<float>(searchRangeList->size());
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i += 1) {
                if (buff[i] >= from_value && buff[i] <= to_value) {
                    cs += 1;
                    pmap.addr = pmapsit->addr + (j * memPageSize) + (i * ISize);
                    resultList->push_back(pmap);
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

int MemoryToolsBase::searchQword(mlong from_value, mlong to_value) {
    int cs = 0;
    RADDR pmap{0, 0};
    int c;
    int buffSize = memPageSize / LSize;
    auto *buff = new mlong[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i += 1) {
                if (buff[i] >= from_value && buff[i] <= to_value) {
                    cs += 1;
                    pmap.addr = pmapsit->addr + (j * memPageSize) + (i * LSize);
                    resultList->push_back(pmap);
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

int MemoryToolsBase::searchDouble(double from_value, double to_value) {
    int cs = 0;
    RADDR pmap{0, 0};
    int c;
    int buffSize = memPageSize / DSize;
    auto *buff = new double[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i += 1) {
                if (buff[i] >= from_value && buff[i] <= to_value) {
                    cs += 1;
                    pmap.addr = pmapsit->addr + (j * memPageSize) + (i * DSize);
                    resultList->push_back(pmap);
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

int MemoryToolsBase::searchByte(mbyte from_value, mbyte to_value) {
    int cs = 0;
    RADDR pmap{0, 0};
    int c;
    int buffSize = memPageSize / BSize;
    auto *buff = new mbyte[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i += 1) {
                if (buff[i] >= from_value && buff[i] <= to_value) {
                    cs += 1;
                    pmap.addr = pmapsit->addr + (j * memPageSize) + (i * BSize);
                    resultList->push_back(pmap);
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

// 字节数组搜索
int MemoryToolsBase::searchBytes(std::string values) {
    std::string input = values;
    std::stringstream ss(input);
    std::string temp;
    std::vector<int> numbers;
    try {
        while (ss >> temp) {
            if (temp.find('?') != std::string::npos) {
                numbers.push_back(0xFFFF);
            } else {
                numbers.push_back(std::stoi(temp, nullptr, 16));
            }
        }
    } catch (std::invalid_argument &e) {
        std::cout << "Invalid hex number: " << temp << std::endl;
        return -1;
    }
    // 如果搜索的数据为空，那没必要再往下了
    if (numbers.empty()) {
        return 0;
    }
    int cs = 0;
    int c;
    int buffSize = memPageSize;
    auto *buff = new mbyte[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    int k = 0;
    Addr beginAddr = 0;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i++) {
                int number = numbers[k];
                if (number == 0xFFFF) {
                    if (k >= numbers.size() - 1) {
                        /* 搜索完成 */
                        cs += 1;
                        resultList->push_back({beginAddr, pmapsit->addr + (j * memPageSize) + i});
                        k = 0;
                    }
                    k++;
                } else if (buff[i] == (mbyte)number) {
                    if (k == 0) {
                        beginAddr = pmapsit->addr + (j * memPageSize) + i;
                    } else if (k >= numbers.size() - 1) {
                        /* 搜索完成 */
                        cs += 1;
                        resultList->push_back({beginAddr, pmapsit->addr + (j * memPageSize) + i});
                        k = 0;
                    }
                    k++;
                } else {
                    k = 0;
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

int MemoryToolsBase::searchString(const std::string &values) {
    if (values.empty()) {
        return 0;
    }
    int strLength = strlen(values.c_str());
    if (strLength <= 0) {
        return 0;
    }
    int cs = 0;
    int c;
    int buffSize = memPageSize;
    auto *buff = new mbyte[buffSize]; // 缓冲区
    float len = 0;
    float max = searchRangeList->size();
    int position = 0;
    printf("searchRangeSize: %d\n", (int)searchRangeList->size());
    printf("\033[32;1m");
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    int k = 0;
    Addr beginAddr = 0;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        c = (int)(pmapsit->taddr - pmapsit->addr) / memPageSize;
        len++;
        position = (int)(len / max * 100.0F);
        printf("[%d%%][%c]\r", position, *(proText + (position % 4)));
        for (int j = 0; j < c; j += 1) {
            memRead(buff, memPageSize, pmapsit->addr, j * memPageSize);
            for (int i = 0; i < buffSize; i++) {
                if (buff[i] == (mbyte)values[k]) {
                    if (k == 0) {
                        beginAddr = pmapsit->addr + (j * memPageSize) + i;
                    } else if (k >= strLength - 1) {
                        /* 搜索完成 */
                        cs += 1;
                        resultList->push_back({beginAddr, pmapsit->addr + (j * memPageSize) + i});
                        k = 0;
                    }
                    k++;
                } else {
                    k = 0;
                }
            }
        }
    }
    printf("\033[37;1m");
    printf("\n");
    delete[] buff;
    return cs;
}

int MemoryToolsBase::memorySearch(std::string value, Type type) {
    clearResults();
    if (searchRangeList->empty()) {
        return -1;
    }
    int cs = 0;
    if (type == MEM_DWORD) {
        int val = atoi(value.c_str());
        cs = searchDword(val, val);
    } else if (type == MEM_QWORD) {
        mlong val = atoll(value.c_str());
        cs = searchQword(val, val);
    } else if (type == MEM_FLOAT) {
        float val = atof(value.c_str());
        cs = searchFloat(val, val);
    } else if (type == MEM_DOUBLE) {
        double val = atof(value.c_str());
        cs = searchDouble(val, val);
    } else if (type == MEM_BYTE) {
        mbyte val = atoi(value.c_str());
        cs = searchByte(val, val);
    } else if (type == MEM_BYTES) {
        cs = searchBytes(value);
    } else if (type == MEM_STRING) {
        cs = searchString(value);
    }
    return cs;
}

int MemoryToolsBase::rangeMemorySearch(std::string from_value, std::string to_value, Type type) {
    clearResults();
    int cs = 0;
    if (type == MEM_DWORD) {
        int fval = atoi(from_value.c_str());
        int tval = atoi(to_value.c_str());
        cs = searchDword(fval, tval);
    } else if (type == MEM_QWORD) {
        mlong fval = atoll(from_value.c_str());
        mlong tval = atoll(to_value.c_str());
        cs = searchQword(fval, tval);
    } else if (type == MEM_FLOAT) {
        float fval = atof(from_value.c_str());
        float tval = atof(to_value.c_str());
        cs = searchFloat(fval, tval);
    } else if (type == MEM_DOUBLE) {
        double fval = atof(from_value.c_str());
        double tval = atof(to_value.c_str());
        cs = searchDouble(fval, tval);
    } else if (type == MEM_BYTE) {
        mbyte fval = atof(from_value.c_str());
        mbyte tval = atof(to_value.c_str());
        cs = searchByte(fval, tval);
    }
    return cs;
}

int MemoryToolsBase::searchOffset(const mbyte *from_value, const mbyte *to_value, ulong offset, Type type, ulong len) {
    int cs = 0;
    auto *offList = new std::list<RADDR>;
    RADDR maps{0, 0};
    auto *buf = new mbyte[len]; // 缓冲区
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = resultList->begin(); pmapsit != resultList->end(); ++pmapsit) {
        memRead(buf, len, pmapsit->addr, (int)offset);
        bool isMatch = false;
        if (type == MEM_DWORD) {
            int value = *(int *)buf;
            int from = *(int *)from_value;
            int to = *(int *)to_value;
            if (value >= from && value <= to) {
                isMatch = true;
            }
        } else if (type == MEM_QWORD) {
            mlong value = *(mlong *)buf;
            mlong from = *(mlong *)from_value;
            mlong to = *(mlong *)to_value;
            if (value >= from && value <= to) {
                isMatch = true;
            }
        } else if (type == MEM_FLOAT) {
            float value = *(float *)buf;
            float from = *(float *)from_value;
            float to = *(float *)to_value;
            if (value >= from && value <= to) {
                isMatch = true;
            }
        } else if (type == MEM_DOUBLE) {
            double value = *(double *)buf;
            double from = *(double *)from_value;
            double to = *(double *)to_value;
            if (value >= from && value <= to) {
                isMatch = true;
            }
        } else if (type == MEM_BYTE) {
            mbyte value = *buf;
            mbyte from = *from_value;
            mbyte to = *to_value;
            if (value >= from && value <= to) {
                isMatch = true;
            }
        }
        if (isMatch) {
            cs += 1;
            maps.addr = pmapsit->addr;
            offList->push_back(maps);
        }
    }
    delete resultList;
    delete[] buf;
    resultList = offList;
    return cs;
}

int MemoryToolsBase::memoryOffset(std::string value, ulong offset, Type type) {
    mbyte *buff = nullptr;
    int cs = 0;
    if (type == MEM_DWORD) {
        int val = atoi(value.c_str());
        buff = new mbyte[ISize];
        memcpy(buff, &val, ISize);
        cs = searchOffset(buff, buff, offset, type, ISize);
    } else if (type == MEM_QWORD) {
        mlong val = atoll(value.c_str());
        buff = new mbyte[LSize];
        memcpy(buff, &val, LSize);
        cs = searchOffset(buff, buff, offset, type, LSize);
    } else if (type == MEM_FLOAT) {
        float val = atof(value.c_str());
        buff = new mbyte[FSize];
        memcpy(buff, &val, FSize);
        cs = searchOffset(buff, buff, offset, type, FSize);
    } else if (type == MEM_DOUBLE) {
        double val = atof(value.c_str());
        buff = new mbyte[DSize];
        memcpy(buff, &val, DSize);
        cs = searchOffset(buff, buff, offset, type, DSize);
    } else if (type == MEM_BYTE) {
        mbyte val = atoi(value.c_str());
        buff = new mbyte[BSize];
        memcpy(buff, &val, BSize);
        cs = searchOffset(buff, buff, offset, type, BSize);
    }
    delete[] buff;
    return cs;
}

int MemoryToolsBase::rangeMemoryOffset(std::string from_value, std::string to_value, ulong offset, Type type) {
    mbyte *fbuff = nullptr;
    mbyte *tbuff = nullptr;
    int cs = 0;
    if (type == MEM_DWORD) {
        int fval = atoi(from_value.c_str());
        int tval = atoi(to_value.c_str());
        fbuff = new mbyte[ISize];
        tbuff = new mbyte[ISize];
        memcpy(fbuff, &fval, ISize);
        memcpy(tbuff, &tval, ISize);
        cs = searchOffset(fbuff, tbuff, offset, type, ISize);
    } else if (type == MEM_QWORD) {
        mlong fval = atoll(from_value.c_str());
        mlong tval = atoll(to_value.c_str());
        fbuff = new mbyte[LSize];
        tbuff = new mbyte[LSize];
        memcpy(fbuff, &fval, LSize);
        memcpy(tbuff, &tval, LSize);
        cs = searchOffset(fbuff, tbuff, offset, type, LSize);
    } else if (type == MEM_FLOAT) {
        float fval = atof(from_value.c_str());
        float tval = atof(to_value.c_str());
        fbuff = new mbyte[FSize];
        tbuff = new mbyte[FSize];
        memcpy(fbuff, &fval, FSize);
        memcpy(tbuff, &tval, FSize);
        cs = searchOffset(fbuff, tbuff, offset, type, FSize);
    } else if (type == MEM_DOUBLE) {
        double fval = atof(from_value.c_str());
        double tval = atof(to_value.c_str());
        fbuff = new mbyte[DSize];
        tbuff = new mbyte[DSize];
        memcpy(fbuff, &fval, DSize);
        memcpy(tbuff, &tval, DSize);
        cs = searchOffset(fbuff, tbuff, offset, type, DSize);
    } else if (type == MEM_BYTE) {
        mbyte fval = atof(from_value.c_str());
        mbyte tval = atof(to_value.c_str());
        fbuff = new mbyte[BSize];
        tbuff = new mbyte[BSize];
        memcpy(fbuff, &fval, BSize);
        memcpy(tbuff, &tval, BSize);
        cs = searchOffset(fbuff, tbuff, offset, type, BSize);
    }
    delete[] fbuff;
    delete[] tbuff;
    return cs;
}


void MemoryToolsBase::memoryWrite(std::string value, ulong offset, Type type) {
    mbyte *buff = nullptr;
    int len = 0;
    if (type == MEM_DWORD) {
        int val = atoi(value.c_str());
        buff = new mbyte[ISize];
        memcpy(buff, &val, ISize);
        len = ISize;
    } else if (type == MEM_QWORD) {
        mlong val = atoll(value.c_str());
        buff = new mbyte[LSize];
        memcpy(buff, &val, LSize);
        len = LSize;
    } else if (type == MEM_FLOAT) {
        float val = atof(value.c_str());
        buff = new mbyte[FSize];
        memcpy(buff, &val, FSize);
        len = FSize;
    } else if (type == MEM_DOUBLE) {
        double val = atof(value.c_str());
        buff = new mbyte[DSize];
        memcpy(buff, &val, DSize);
        len = DSize;
    } else if (type == MEM_BYTE) {
        mbyte val = atoi(value.c_str());
        buff = new mbyte[BSize];
        memcpy(buff, &val, BSize);
        len = BSize;
    } else {
        printf("Not support type: %d\n", type);
        return;
    }
    // 迭代器
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = resultList->begin(); pmapsit != resultList->end(); ++pmapsit) {
        if(memWrite(buff, len, pmapsit->addr, offset) != len){
            printf("Write error, Addr: %llX\n", pmapsit->addr);
        }
    }
}


ulong MemoryToolsBase::dumpMem(Addr beginAddr, Addr endAddr, FILE *dumpfile) {
    if (endAddr - beginAddr <= 0) {
        return 0;
    }
    const int buffSize = 1024 * 1024 * 2;
    auto *fileBuff = new mbyte[buffSize];
    Addr baseAddress = beginAddr;
    ulong baseSize = endAddr - beginAddr;
    offset off = 0;
    ulong total = 0;
    while (baseSize > 0) {
        ulong size = (std::min)(baseSize, (ulong)buffSize);
        readV(fileBuff, buffSize, baseAddress, off);
        fwrite(fileBuff, 1, size, dumpfile);
        baseSize -= size;
        total += size;
        off += size;
    }
    delete[] fileBuff;
    return total;
}

ulong MemoryToolsBase::dumpMem(std::string dumpModule, std::string filePath) {
    const int buffSize = 1024 * 1024 * 2;
    auto *fileBuff = new mbyte[buffSize];
    FILE *p = fopen(filePath.c_str(), "wb");
    ulong total = 0;
    Addr baseAddress = getModuleAddr(dumpModule);
    ulong baseSize = getModuleSize(dumpModule);
    offset off = 0;
    while (baseSize > 0) {
        ulong size = (std::min)(baseSize, (ulong)buffSize);
        readV(fileBuff, buffSize, baseAddress, off);
        fwrite(fileBuff, 1, size, p);
        total += size;
        baseSize -= size;
        off += size;
    }
    delete[] fileBuff;
    fclose(p); // 关闭文件指针
    return total;
}

ulong MemoryToolsBase::dumpAllMem(const std::string &dirPath) {
    std::filesystem::path path(dirPath);
    if (!exists(path)) {
        printf("No such directory: %s\n", path.string().c_str());
        return 0;
    }
    ulong total = 0;
    std::string dictTxtPath = dirPath + "/dict.txt";
    FILE *dictTxt = fopen(dictTxtPath.c_str(), "wb");
    std::vector<RADDR> modules;
    std::string moduleMemoryPath = dirPath + "/" + "moduleMemory.bin";
    FILE *m = fopen(moduleMemoryPath.c_str(), "wb");
    // 写入进程名
    std::string pName = "name|" + processName + "\n";
    fwrite(pName.data(), 1, pName.size(), dictTxt);
    offset memoryMapOff = 0;
    // 保存模块数据
    for (const auto &mModule : moduleRegions) {
        printf("Dumping module: %s size: %llX\n", mModule.moduleName, mModule.baseSize);
        std::string baseAddressStr = addrToHex(mModule.baseAddress) + "|" + addrToHex(
        mModule.baseAddress + mModule.baseSize);
        std::string outStr = "m|" + std::string(mModule.moduleName) + "|" + baseAddressStr + "|" +
        addrToHex(memoryMapOff) + "\n";
        fwrite(outStr.data(), 1, outStr.size(), dictTxt);
        modules.push_back({mModule.baseAddress, mModule.baseAddress + mModule.baseSize});
        ulong size = dumpMem(mModule.baseAddress, mModule.baseAddress + mModule.baseSize, m);
        if (size == 0) {
            total = 0;
            break;
        }
        memoryMapOff += size;
        total += size;
    }
    fclose(m);
    // 内存地址映射偏移
    std::string pteMemoryPath = dirPath + "/" + "pteMemory.bin";
    FILE *p = fopen(pteMemoryPath.c_str(), "wb");
    memoryMapOff = 0;
    // 保存内存数据
    for (const auto &memory : memoryRegions) {
        Addr baseAddress = memory.baseAddress;
        Addr baseSize = memory.baseSize;
        bool isModuleRegion = true;
        // 排除模块内存
        for (auto &module : modules) {
            if (baseAddress >= module.addr && (baseAddress + baseSize) <= module.taddr) {
                isModuleRegion = false;
                break;
            }
        }
        if (!isModuleRegion) {
            continue;
        }
        printf("Dumping memory: %llX size: %llX\n", baseAddress, baseSize);
        std::string baseAddressStr = addrToHex(baseAddress) + "|" + addrToHex(baseAddress + baseSize);
        std::string outStr = "p|p|" + baseAddressStr + "|" + addrToHex(memoryMapOff) + "\n";
        fwrite(outStr.data(), 1, outStr.size(), dictTxt);
        ulong size = dumpMem(baseAddress, baseAddress + baseSize, p);
        if (size == 0) {
            total = 0;
            break;
        }
        memoryMapOff += size;
        total += size;
    }
    fclose(p);
    fclose(dictTxt);
    return total;
}

void MemoryToolsBase::printResult() const {
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = resultList->begin(); pmapsit != resultList->end(); ++pmapsit) {
        printf("addr:0x%llX,taddr:0x%llX\n", pmapsit->addr, pmapsit->taddr);
    }
}

void MemoryToolsBase::printSearchRange() const {
    std::list<RADDR>::iterator pmapsit;
    for (pmapsit = searchRangeList->begin(); pmapsit != searchRangeList->end(); ++pmapsit) {
        printf("addr:0x%llX,taddr:0x%llX\n", pmapsit->addr, pmapsit->taddr);
    }
}

void MemoryToolsBase::clearResults() const {
    resultList->clear();
}

void MemoryToolsBase::clearSearchRange() const {
    searchRangeList->clear();
}

std::list<RADDR> *MemoryToolsBase::getResults() {
    return resultList;
}

std::list<RADDR> *MemoryToolsBase::getSearchRange() const {
    return searchRangeList;
}

int MemoryToolsBase::getResCount() const {
    return resultList->size();
}

void MemoryToolsBase::addSearchRang(Addr startAddr, Addr endAddr) {
    RADDR pmaps{startAddr, endAddr};
    searchRangeList->push_back(pmaps);
}

int MemoryToolsBase::getProcessPid() const {
    return processID;
}

Addr MemoryToolsBase::getBaseAddr() const {
    return baseModule.baseAddress;
}

std::vector<MModule> MemoryToolsBase::getModuleList() const {
    return moduleRegions;
}

std::string MemoryToolsBase::addrToHex(Addr num) {
    std::stringstream ss;
    ss << std::hex << num;
    return ss.str();
}
