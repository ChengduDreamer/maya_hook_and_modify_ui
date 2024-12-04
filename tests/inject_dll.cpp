#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <tchar.h>

// ����������Ʒ��ظý���PID
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

// Զ���߳�ע��
BOOL CreateRemoteThreadInjectDll(DWORD Pid, char* DllName)
{
    HANDLE hProcess = NULL;
    SIZE_T dwSize = 0;
    LPVOID pDllAddr = NULL;
    FARPROC pFuncProcAddr = NULL;

    // ��ע�����
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Pid);
    if (NULL == hProcess)
    {
        return FALSE;
    }

    // �õ�ע���ļ�������·��
    dwSize = sizeof(char) + lstrlen(DllName);

    // �ڶԶ�����һ���ڴ�
    pDllAddr = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pDllAddr)
    {
        return FALSE;
    }

    // ��ע���ļ���д�뵽�ڴ���
    if (FALSE == WriteProcessMemory(hProcess, pDllAddr, DllName, dwSize, NULL))
    {
        return FALSE;
    }

    // �õ�LoadLibraryA()�����ĵ�ַ
    pFuncProcAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (NULL == pFuncProcAddr)
    {
        return FALSE;
    }

    // �����߳�ע��
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFuncProcAddr, pDllAddr, 0, NULL);
    if (NULL == hRemoteThread)
    {
        return FALSE;
    }

    // �ȴ��߳̽���
    WaitForSingleObject(hRemoteThread, INFINITE);

    // ����
    VirtualFreeEx(hProcess, pDllAddr, 0, MEM_RELEASE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);
    return TRUE;
}

int main(int argc, char* argv[])
{
#if 0
    DWORD pid = FindProcessID("target_test.exe");
    std::cout << "����PID: " << pid << std::endl;

    bool flag = CreateRemoteThreadInjectDll(pid, (char*)"C:\\code\\hub\\test_minhook\\out\\build\\x64-RelWithDebInfo\\MyLibrary.dll");
    std::cout << "ע��״̬: " << flag << std::endl;
#endif

    DWORD pid = FindProcessID("maya.exe");
    std::cout << "����PID: " << pid << std::endl;

    bool flag = CreateRemoteThreadInjectDll(pid, (char*)"C:\\code\\hub\\test_minhook\\out\\build\\x64-RelWithDebInfo\\MyLibrary2.dll");
    std::cout << "ע��״̬: " << flag << std::endl;


    return 0;
}