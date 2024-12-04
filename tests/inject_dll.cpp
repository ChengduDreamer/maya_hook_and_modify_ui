#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <tchar.h>

// 传入进程名称返回该进程PID
DWORD FindProcessID(LPCTSTR szProcessName)
{
    DWORD dwPID = 0xFFFFFFFF;
    HANDLE hSnapShot = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    Process32First(hSnapShot, &pe);
    do
    {
        if (!_tcsicmp(szProcessName, (LPCTSTR)pe.szExeFile))
        {
            dwPID = pe.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapShot, &pe));
    CloseHandle(hSnapShot);
    return dwPID;
}

// 远程线程注入
BOOL CreateRemoteThreadInjectDll(DWORD Pid, char* DllName)
{
    HANDLE hProcess = NULL;
    SIZE_T dwSize = 0;
    LPVOID pDllAddr = NULL;
    FARPROC pFuncProcAddr = NULL;

    // 打开注入进程
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Pid);
    if (NULL == hProcess)
    {
        return FALSE;
    }

    // 得到注入文件的完整路径
    dwSize = sizeof(char) + lstrlen(DllName);

    // 在对端申请一块内存
    pDllAddr = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pDllAddr)
    {
        return FALSE;
    }

    // 将注入文件名写入到内存中
    if (FALSE == WriteProcessMemory(hProcess, pDllAddr, DllName, dwSize, NULL))
    {
        return FALSE;
    }

    // 得到LoadLibraryA()函数的地址
    pFuncProcAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (NULL == pFuncProcAddr)
    {
        return FALSE;
    }

    // 启动线程注入
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFuncProcAddr, pDllAddr, 0, NULL);
    if (NULL == hRemoteThread)
    {
        return FALSE;
    }

    // 等待线程结束
    WaitForSingleObject(hRemoteThread, INFINITE);

    // 清理
    VirtualFreeEx(hProcess, pDllAddr, 0, MEM_RELEASE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);
    return TRUE;
}

int main(int argc, char* argv[])
{
#if 0
    DWORD pid = FindProcessID("target_test.exe");
    std::cout << "进程PID: " << pid << std::endl;

    bool flag = CreateRemoteThreadInjectDll(pid, (char*)"C:\\code\\hub\\test_minhook\\out\\build\\x64-RelWithDebInfo\\MyLibrary.dll");
    std::cout << "注入状态: " << flag << std::endl;
#endif

    DWORD pid = FindProcessID("maya.exe");
    std::cout << "进程PID: " << pid << std::endl;

    bool flag = CreateRemoteThreadInjectDll(pid, (char*)"C:\\code\\hub\\test_minhook\\out\\build\\x64-RelWithDebInfo\\MyLibrary2.dll");
    std::cout << "注入状态: " << flag << std::endl;


    return 0;
}