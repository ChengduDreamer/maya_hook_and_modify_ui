#include "MinHook.h"
typedef int (WINAPI* MESSAGEBOXA)(HWND, LPCSTR, LPCSTR, UINT);

MESSAGEBOXA fpMessageBoxA = NULL;//指向原MessageBoxA的指针
//用来替代原函数的MessageBox函数
int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	//这里只做简单的修改参数，其实可以做很多事情，甚至不去调用原函数
	return fpMessageBoxA(hWnd, "Hooked!", lpCaption, uType);
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		do
		{
			//初始化
			if (MH_Initialize() != MH_OK)
			{
				OutputDebugStringA("Initialize err");
				break;
			}
			//创建Hook
			if (MH_CreateHook(MessageBoxA, &DetourMessageBoxA,
				reinterpret_cast<LPVOID*>(&fpMessageBoxA)) != MH_OK)
			{
				OutputDebugStringA("MH_CreateHook err");
				break;
			}
			//使目标函数Hook生效
			if (MH_EnableHook(MessageBoxA) != MH_OK)
			{
				OutputDebugStringA("MH_EnableHook err");
				break;
			}
			//使目标函数Hook失效
			//if (MH_DisableHook(MessageBoxA) != MH_OK)
			//{
			//	break;
			//}
			// 卸载MinHook
			//if (MH_Uninitialize() != MH_OK)
			//{
			//  break;
			//}

		} while (false);

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
