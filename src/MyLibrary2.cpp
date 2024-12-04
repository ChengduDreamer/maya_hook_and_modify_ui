#include "MinHook.h"
#include <iostream>
#include <fileapi.h>
#include <Windows.h>
#include <string>
#include <thread>
#include <qapplication.h>
#include <qwidget.h>
#include <qlineedit.h>
#include <qstring.h>

typedef int (WINAPI* MESSAGEBOXA)(HWND, LPCSTR, LPCSTR, UINT);

//WINBASEAPI DWORD WINAPI GetLogicalDrives(VOID);

typedef DWORD(WINAPI* GetLogicalDrivesPtr)(void);

//DWORD WINAPI GetLongPathNameW(_In_ LPCWSTR lpszShortPath, _Out_writes_to_opt_(cchBuffer, return +1) LPWSTR lpszLongPath, _In_ DWORD cchBuffer);

typedef DWORD(WINAPI* GetLongPathNameWPtr)(LPCWSTR, LPWSTR, DWORD);

//DWORD GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart);

typedef DWORD(WINAPI* GetFullPathNameWPtr)(LPCWSTR, DWORD, LPWSTR, LPWSTR*);

GetFullPathNameWPtr GetFullPathNameW_ptr = NULL;

GetLongPathNameWPtr GetLongPathNameW_ptr = NULL;

GetLogicalDrivesPtr GetLogicalDrives_ptr = NULL;

MESSAGEBOXA fpMessageBoxA = NULL;//ָ��ԭMessageBoxA��ָ��


//�������ԭ������MessageBox����
int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	//����ֻ���򵥵��޸Ĳ�������ʵ�������ܶ����飬������ȥ����ԭ����
	return fpMessageBoxA(hWnd, "Hooked!", lpCaption, uType);
}

DWORD WINAPI DetourGetLogicalDrives(VOID) {

	auto drives = GetLogicalDrives_ptr();
	std::cout << "0 drives = " << drives << std::endl;
	drives &= ~(1 << 2);

	std::cout << "1 drives = " << drives << std::endl;
	// return 0; �ܿ���C  G ��

	return 64; // Ŀ����G��

	//return GetLogicalDrives_ptr();
}

DWORD WINAPI DetourGetLongPathNameW(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer) {
	
	std::wstring short_pathw = lpszShortPath;

	std::wcout << L"DetourGetLongPathNameW short_pathw:" << short_pathw << L",size:" << short_pathw.size() << std::endl;

	const std::wstring c_dirver_short_pathw = L"\\\\?\\C:\\";


	const std::wstring c_dirver_short_pathw2 = L"C:\\";

	const std::wstring desktop_short_pathw = L"\\\\?\\C:\\Users\\Administrator\\Desktop";

	const std::wstring desktop_short_pathw2 = L"C:\\Users\\Administrator\\Desktop";

	const std::wstring user_short_pathw = L"\\\\?\\C:\\Users\\Administrator";

	const std::wstring user_short_pathw2 = L"C:\\Users\\Administrator";

	//return 0;

	if (c_dirver_short_pathw == short_pathw  || c_dirver_short_pathw2 == short_pathw || desktop_short_pathw == short_pathw || desktop_short_pathw2 == short_pathw ||
		user_short_pathw == short_pathw || user_short_pathw2 == short_pathw) {
		std::cout << "c_dirver_short_pathw == short_pathw" << std::endl;

		memset(lpszLongPath, 0, cchBuffer);

		return 0;
	}
	//return GetLongPathNameW_ptr(lpszShortPath, lpszLongPath, cchBuffer);
	auto res = GetLongPathNameW_ptr(lpszShortPath, lpszLongPath, cchBuffer);

	std::wcout << L"GetLongPathNameW_ptr res:" << res << L", lpszLongPath:" << std::wstring(lpszLongPath) << std::endl;

	return res;
}

DWORD WINAPI DetourGetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart) {

	std::wstring short_pathw = lpFileName;

	std::wcout << "DetourGetFullPathNameW short_pathw:" << short_pathw << std::endl;

	const std::wstring c_dirver_short_pathw = L"\\\\?\\C:\\";

	const std::wstring c_dirver_short_pathw2 = L"C:\\";

	const std::wstring desktop_short_pathw = L"\\\\?\\C:\\Users\\Administrator\\Desktop";

	const std::wstring desktop_short_pathw2 = L"C:\\Users\\Administrator\\Desktop";

	const std::wstring user_short_pathw = L"\\\\?\\C:\\Users\\Administrator";

	const std::wstring user_short_pathw2 = L"C:\\Users\\Administrator";

	//const std::string desktop_short_pathw = L"\\\\?\\C:\\Users\\Administrator\\Desktop";

	if (c_dirver_short_pathw == short_pathw || c_dirver_short_pathw2 == short_pathw || desktop_short_pathw == short_pathw || desktop_short_pathw2 == short_pathw ||
		user_short_pathw == short_pathw || user_short_pathw2 == short_pathw) {
		std::cout << "c_dirver_short_pathw == short_pathw" << std::endl;

		memset(lpBuffer, 0, nBufferLength);

		return 0;
	}


	auto res = GetFullPathNameW_ptr(lpFileName, nBufferLength, lpBuffer, lpFilePart);

	return res;
}


void FindWidget() {
	while (true)
	{
		Sleep(1000);


		auto window = QApplication::activeWindow();
		
		if (NULL == window) {
			std::cout << "window is null" << std::endl;
			continue;
		}

		
		for (QObject* child : window->findChildren<QObject*>()) {
			std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;
			if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(child)) {
				// �ҵ��ı�����������������д���
				//qDebug() << "Found QLineEdit:" << lineEdit->objectName() << lineEdit->text();
				// ����ѡ�������ı���
				// lineEdit->hide();
				std::cout << "objectName:" << lineEdit->objectName().toStdString() << ", text:" << lineEdit->text().toStdString() << std::endl;

				
				

				//QMetaObject::invokeMethod(window, [=]() {
				//	lineEdit->hide();
				//});
			}
		}

		
	}
}

std::thread find_widget_thread;

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
			AllocConsole();
			freopen("CONOUT$", "w", stdout);
			std::cout << "This works" << std::endl;

			find_widget_thread = std::thread(FindWidget);
#if 0
			//��ʼ��
			if (MH_Initialize() != MH_OK)
			{
				OutputDebugStringA("Initialize err");
				break;
			}
			//����Hook
			if (MH_CreateHook(GetLogicalDrives, &DetourGetLogicalDrives, reinterpret_cast<LPVOID*>(&GetLogicalDrives_ptr)) != MH_OK)
			{
				OutputDebugStringA("MH_CreateHook err");
				break;
			}
			//ʹĿ�꺯��Hook��Ч
			if (MH_EnableHook(GetLogicalDrives) != MH_OK)
			{
				OutputDebugStringA("MH_EnableHook err");
				break;
			}

			if (MH_CreateHook(GetLongPathNameW, &DetourGetLongPathNameW, reinterpret_cast<LPVOID*>(&GetLongPathNameW_ptr)) != MH_OK)
			{
				OutputDebugStringA("MH_CreateHook err");
				break;
			}

			if (MH_EnableHook(GetLongPathNameW) != MH_OK)
			{
				OutputDebugStringA("MH_EnableHook err");
				break;
			}

			if (MH_CreateHook(GetFullPathNameW, &DetourGetFullPathNameW, reinterpret_cast<LPVOID*>(&GetFullPathNameW_ptr)) != MH_OK)
			{
				OutputDebugStringA("MH_CreateHook err");
				break;
			}

			if (MH_EnableHook(GetFullPathNameW) != MH_OK)
			{
				OutputDebugStringA("MH_EnableHook err");
				break;
			}
#endif
			//ʹĿ�꺯��HookʧЧ
			//if (MH_DisableHook(MessageBoxA) != MH_OK)
			//{
			//	break;
			//}
			// ж��MinHook
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
