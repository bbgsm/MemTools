//
// Created by bbgsm on 2024/10/15.
//

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

DWORD GetProcessIDByName(const char *processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(hSnapshot, &processEntry)) {
        do {
            if (strcmp(processEntry.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }

    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(const char* processName, const char* dllPath) {
    char fullPath[MAX_PATH];
    if (!GetFullPathName(dllPath, MAX_PATH, fullPath, NULL)) {
        std::cerr << "Failed to get full path." << std::endl;
        return false;
    }

    DWORD processID = GetProcessIDByName(processName);
    if (processID == 0) {
        std::cerr << "Process not found." << std::endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        std::cerr << "Failed to open process." << std::endl;
        return false;
    }

    LPVOID allocMem = VirtualAllocEx(hProcess, NULL, strlen(fullPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        std::cerr << "Failed to allocate memory in target process." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, allocMem, fullPath, strlen(fullPath) + 1, NULL)) {
        std::cerr << "Failed to write DLL path to target process memory." << std::endl;
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
    if (!hKernel32) {
        std::cerr << "Failed to get handle of kernel32.dll." << std::endl;
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    FARPROC loadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibraryAddr) {
        std::cerr << "Failed to get address of LoadLibraryA." << std::endl;
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, allocMem, 0, NULL);
    if (!hRemoteThread) {
        std::cerr << "Failed to create remote thread." << std::endl;
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hRemoteThread, INFINITE);

    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);

    return true;
}

/**
 * !!!!!!!!! 需要使用管理员运行 !!!!!!!!!
 * dll 注入工具，使用:  injectDLL.exe test.exe D:\abc.dll
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <process_name> <dll_absolute_path>" << std::endl;
        return 1;
    }

    const char* processName = argv[1];
    const char* dllPath = argv[2];

    if (InjectDLL(processName, dllPath)) {
        std::cout << "DLL injected successfully." << std::endl;
    } else {
        std::cerr << "DLL injection failed." << std::endl;
    }

    return 0;
}