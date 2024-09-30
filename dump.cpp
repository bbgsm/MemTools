//
// Created by bbgsm on 2024/8/20.
//

#include <iostream>
#include "DmaMemoryTools.h"
// #include "DirectMemoryTools.h"

// dump 工具
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Params num failed\n");
        return -1;
    }
    printf("Dump %s memory, out dir: %s\n", argv[1], argv[2]);
    // DMA内存
    DmaMemoryTools mem;
    // 本机内存
    // DirectMemoryTools mem;
    if (!mem.init(argv[1])) {
        std::cout << "Failed to initialize" << std::endl;
        return -1;
    }
    Addr baseAddr = mem.getBaseAddr();
    printf("baseAddr: %llX\n", baseAddr);
    mulong size = mem.dumpAllMem(argv[2]);
    printf("dump size: %lu\n", size);
    return 0;
}