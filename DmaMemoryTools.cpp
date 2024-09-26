#include "DmaMemoryTools.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <thread>

#define DMA_DEBUG false

static inline bool initialized = false;
static inline bool processInitialized = false;
unsigned char abort3[4] = {0x10, 0x00, 0x10, 0x00};

uint64_t cbSize1 = 0x80000;
// callback for VfsFileListU
VOID cbAddFile1(_Inout_ HANDLE h, _In_ LPCSTR uszName, _In_ ULONG64 cb, _In_opt_ PVMMDLL_VFS_FILELIST_EXINFO pExInfo) {
    if (strcmp(uszName, "dtb.txt") == 0) cbSize1 = cb;
}

struct Info1 {
    uint32_t index;
    uint32_t process_id;
    uint64_t dtb;
    uint64_t kernelAddr;
    std::string name;
};

//VMM_HANDLE vHandle;
ulong DmaMemoryTools::memRead(void *buff, ulong len, Addr addr, offset off) {
    if (!isAddrValid(addr)) {
        return 0;
    }
    DWORD read_size = 0;
    if (!VMMDLL_MemReadEx(vHandle, processID, addr + off, static_cast<PBYTE>(buff), len, &read_size,
                          VMMDLL_FLAG_NOCACHE)) {
        logDebug("[!] Failed to read Memory at 0x%p\n", addr + off);
        return false;
    }
    return (read_size == len);
}

ulong DmaMemoryTools::memWrite(void *buff, ulong len, Addr addr, offset off) {
    if (!VMMDLL_MemWrite(vHandle, processID, addr + off, static_cast<PBYTE>(buff), len)) {
        logDebug("[!] Failed to write Memory at 0x%p\n", addr + off);
        return false;
    }
    return true;
}

DmaMemoryTools::~DmaMemoryTools() {
    DmaMemoryTools::close();
}

bool DmaMemoryTools::SetFPGA() {
    ULONG64 qwID = 0, qwVersionMajor = 0, qwVersionMinor = 0;
    if (!VMMDLL_ConfigGet(vHandle, LC_OPT_FPGA_FPGA_ID, &qwID) &&
        VMMDLL_ConfigGet(vHandle, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) && VMMDLL_ConfigGet(
        vHandle, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor)) {
        logDebug("[!] Failed to lookup FPGA device, Attempting to proceed\n\n");
        return false;
    }
    logDebug("[+] VMMDLL_ConfigGet");
    logDebug(" ID = %lli", qwID);
    logDebug(" VERSION = %lli.%lli\n", qwVersionMajor, qwVersionMinor);
    if ((qwVersionMajor >= 4) && ((qwVersionMajor >= 5) || (qwVersionMinor >= 7))) {
        HANDLE handle;
        LC_CONFIG config = {.dwVersion = LC_CONFIG_VERSION, .szDevice = "existing"};
        handle = LcCreate(&config);
        if (!handle) {
            logDebug("[!] Failed to create FPGA device\n");
            return false;
        }
        LcCommand(handle, LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002, 4, reinterpret_cast<PBYTE>(&abort3), NULL, NULL);
        logDebug("[-] Register auto cleared\n");
        LcClose(handle);
    }
    return true;
}

bool DmaMemoryTools::FixCr3() {
    PVMMDLL_MAP_MODULEENTRY module_entry = NULL;
    bool result = VMMDLL_Map_GetModuleFromNameU(vHandle, processID, processName.c_str(), &module_entry, NULL);
    if (result) return true; // Doesn't need to be patched lol

    if (!VMMDLL_InitializePlugins(vHandle)) {
        logDebug("[-] Failed VMMDLL_InitializePlugins call\n");
        return false;
    }
    // have to sleep a little or we try reading the file before the plugin initializes fully
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    while (true) {
        BYTE bytes[4] = {0};
        DWORD i = 0;
        auto nt = VMMDLL_VfsReadW(vHandle, const_cast<LPWSTR>(L"\\misc\\procinfo\\progress_percent.txt"), bytes, 3, &i,
                                  0);
        if (nt == VMMDLL_STATUS_SUCCESS && atoi(reinterpret_cast<LPSTR>(bytes)) == 100) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    VMMDLL_VFS_FILELIST2 VfsFileList;
    VfsFileList.dwVersion = VMMDLL_VFS_FILELIST_VERSION;
    VfsFileList.h = 0;
    VfsFileList.pfnAddDirectory = 0;
    VfsFileList.pfnAddFile = cbAddFile1; // dumb af callback who made this system

    result = VMMDLL_VfsListU(vHandle, const_cast<LPSTR>("\\misc\\procinfo\\"), &VfsFileList);
    if (!result) return false;

    // read the data from the txt and parse it
    const size_t buffer_size = cbSize1;
    std::unique_ptr<BYTE[]> bytes(new BYTE[buffer_size]);
    DWORD j = 0;
    auto nt = VMMDLL_VfsReadW(vHandle, const_cast<LPWSTR>(L"\\misc\\procinfo\\dtb.txt"), bytes.get(), buffer_size - 1,
                              &j, 0);
    if (nt != VMMDLL_STATUS_SUCCESS) return false;

    std::vector<uint64_t> possible_dtbs = {};
    std::string lines(reinterpret_cast<char *>(bytes.get()));
    std::istringstream iss(lines);
    std::string line = "";

    while (std::getline(iss, line)) {
        Info1 info = {};
        std::istringstream info_ss(line);
        if (info_ss >> std::hex >> info.index >> std::dec >> info.process_id >> std::hex >> info.dtb >> info.kernelAddr
            >> info.name) {
            if (info.process_id == 0) // parts that lack a name or have a NULL pid are suspects
                possible_dtbs.push_back(info.dtb);
            if (processName.find(info.name) != std::string::npos) possible_dtbs.push_back(info.dtb);
        }
    }
    // loop over possible dtbs and set the config to use it til we find the correct one
    for (size_t i = 0; i < possible_dtbs.size(); i++) {
        auto dtb = possible_dtbs[i];
        VMMDLL_ConfigSet(vHandle, VMMDLL_OPT_PROCESS_DTB | processID, dtb);
        result = VMMDLL_Map_GetModuleFromNameU(vHandle, processID, processName.c_str(), &module_entry, NULL);
        if (result) {
            logDebug("[+] Patched DTB\n");
            return true;
        }
    }
    logDebug("[-] Failed to patch module\n");
    return false;
}

bool DmaMemoryTools::init(std::string bm) {
    MemoryToolsBase::init();
    if (!initialized) {
        logDebug("inizializing...\n");
    reinit: LPCSTR args[] = {const_cast<LPCSTR>(""), const_cast<LPCSTR>("-device"), const_cast<LPCSTR>("fpga://algo=0"),
                             const_cast<LPCSTR>(""), const_cast<LPCSTR>(""), const_cast<LPCSTR>(""),
                             const_cast<LPCSTR>("")};
        DWORD argc = 3;
        if (DMA_DEBUG) {
            args[argc++] = const_cast<LPCSTR>("-v");
            args[argc++] = const_cast<LPCSTR>("-printf");
        }
        std::string path = "";
        vHandle = VMMDLL_Initialize(argc, args);
        if (!vHandle) {
            logDebug("[!] Initialization failed! Is the DMA in use or disconnected?\n");
            return false;
        }
        ULONG64 FPGA_ID = 0, DEVICE_ID = 0;
        VMMDLL_ConfigGet(vHandle, LC_OPT_FPGA_FPGA_ID, &FPGA_ID);
        VMMDLL_ConfigGet(vHandle, LC_OPT_FPGA_DEVICE_ID, &DEVICE_ID);
        logDebug("FPGA ID: %llu\n", FPGA_ID);
        logDebug("DEVICE ID: %llu\n", DEVICE_ID);
        logDebug("success!\n");
        if (!SetFPGA()) {
            logDebug("[!] Could not set FPGA!\n");
            VMMDLL_Close(vHandle);
            return false;
        }
        initialized = true;
    } else logDebug("DMA already initialized!\n");
    if (processInitialized) {
        logDebug("Process already initialized!\n");
        return true;
    }
    processID = getPID(bm);
    if (!processID) {
        logDebug("[!] Could not get PID from name!\n");
        return false;
    }
    processName = bm;
    if (!FixCr3()) std::cout << "Failed to fix CR3" << std::endl;
    else std::cout << "CR3 fixed" << std::endl;
    initModuleRegions();
    initMemoryRegions();
    baseModule = getModule(bm);
    if (!baseModule.baseAddress) {
        logDebug("[!] Could not get base address!\n");
        return false;
    }
    logDebug("Process information of %s\n", bm.c_str());
    logDebug("PID: %i\n", processID);
    logDebug("Base Address: 0x%llx\n", baseModule.baseAddress);
    logDebug("Base Size: 0x%llx\n", baseModule.baseSize);
    processInitialized = true;
    return true;
}

void DmaMemoryTools::close()
{
    VMMDLL_Close(vHandle);
}

std::vector<PProcess> DmaMemoryTools::getProcessList() {
    DWORD count_processes = 0;
    std::vector<PProcess> processes;
    PVMMDLL_PROCESS_INFORMATION info = NULL;
    if (!VMMDLL_ProcessGetInformationAll(vHandle, &info, &count_processes)) return {};
    for (int i = 0; i < count_processes; ++i) {
        PProcess process;
        process.processID = info[i].dwPID;
        process.pProcessID = info[i].dwPPID;
        strcpy(process.processName, info[i].szNameLong);
        processes.push_back(process);
    }
    return processes;
}

DmaMemoryTools::DmaMemoryTools() = default;

int DmaMemoryTools::getPID(std::string bm) {
    DWORD pid = 0;
    VMMDLL_PidGetFromName(vHandle, bm.c_str(), &pid);
    return pid;
}

void DmaMemoryTools::initModuleRegions() {
    std::vector<std::string> list = {};
    PVMMDLL_MAP_MODULE module_info = NULL;
    if (!VMMDLL_Map_GetModuleU(vHandle, processID, &module_info, VMMDLL_MODULE_FLAG_NORMAL)) {
        logDebug("[!] Failed to get module list\n");
        return;
    }
    for (size_t i = 0; i < module_info->cMap; i++) {
        auto module = module_info->pMap[i];
        list.emplace_back(module.uszText);
        PVMMDLL_MAP_MODULEENTRY module_map_info;
        if (!VMMDLL_Map_GetModuleFromNameU(vHandle, processID, module.uszText, &module_map_info,
                                           VMMDLL_MODULE_FLAG_NORMAL)) {
            logDebug("[!] Couldn't find Base Address for %s\n", module.uszText);
            continue;
        }
        MModule mModule;
        mModule.baseAddress = module_map_info->vaBase;
        mModule.baseSize = module_map_info->cbImageSize;
        strcpy(mModule.moduleName, module.uszText);
        moduleRegions.push_back(mModule);
    }
}

void DmaMemoryTools::initMemoryRegions() {
    PVMMDLL_MAP_PTE pMemMapEntries = NULL;
    bool result = VMMDLL_Map_GetPte(vHandle, processID, TRUE, &pMemMapEntries);
    if (!result) {
        logDebug("Failed to get PTE\n");
        return;
    }
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    for (int i = 0; i < pMemMapEntries->cMap; i++) {
        PVMMDLL_MAP_PTEENTRY memMapEntry = &pMemMapEntries->pMap[i];
        Addr baseAddress = memMapEntry->vaBase;
        Addr baseSize = memMapEntry->cPages * systemInfo.dwPageSize;
        bool isModuleRegion = false;
        for (auto &module : moduleRegions) {
            if (baseAddress >= module.baseAddress && (baseAddress + baseSize) <= (module.baseAddress + module.
                baseSize)) {
                isModuleRegion = true;
                break;
            }
        }
        if (isModuleRegion) {
            continue;
        }
        memoryRegions.push_back({"", baseAddress, baseSize});
    }
}

Handle DmaMemoryTools::createScatter() {
    const VMMDLL_SCATTER_HANDLE ScatterHandle = VMMDLL_Scatter_Initialize(vHandle, processID, VMMDLL_FLAG_NOCACHE);
    if (!ScatterHandle) logDebug("[!] Failed to create scatter handle\n");
    return ScatterHandle;
}

void DmaMemoryTools::addScatterReadV(Handle handle, void *buff, ulong len, Addr addr, offset off) {
    if (!VMMDLL_Scatter_PrepareEx(handle, addr + off, len, static_cast<PBYTE>(buff), NULL)) {
        logDebug("[!] Failed to prepare scatter read at 0x%p\n", addr + off);
    }
}

void DmaMemoryTools::executeReadScatter(Handle handle) {
    if (!VMMDLL_Scatter_ExecuteRead(handle)) {
        logDebug("[-] Failed to Execute Scatter Read\n");
    }
    // Clear after using it
    if (!VMMDLL_Scatter_Clear(handle, processID, VMMDLL_FLAG_NOCACHE)) {
        logDebug("[-] Failed to clear Scatter\n");
    }
}

void DmaMemoryTools::closeScatterHandle(Handle handle) {
    VMMDLL_Scatter_CloseHandle(handle);
}
