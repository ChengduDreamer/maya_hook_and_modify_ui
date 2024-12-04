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
#include <qfilesystemmodel.h>
#include <qlist.h>
#include <qlistview.h>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QStringListModel>
#include <QAbstractItemModel>
#include <qsortfilterproxymodel.h>
#include <qtreeview.h>

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

MESSAGEBOXA fpMessageBoxA = NULL;//指向原MessageBoxA的指针


//用来替代原函数的MessageBox函数
int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	//这里只做简单的修改参数，其实可以做很多事情，甚至不去调用原函数
	return fpMessageBoxA(hWnd, "Hooked!", lpCaption, uType);
}

DWORD WINAPI DetourGetLogicalDrives(VOID) {

	auto drives = GetLogicalDrives_ptr();
	std::cout << "0 drives = " << drives << std::endl;
	drives &= ~(1 << 2);

	std::cout << "1 drives = " << drives << std::endl;
	// return 0; 能看到C  G 盘

	return 64; // 目标是G盘

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

#if 0
void FindWidget() {
	while (true)
	{
		Sleep(1000);


		auto window = QApplication::activeWindow();
		
		if (NULL == window) {
			std::cout << "window is null" << std::endl;
			continue;
		}

		std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;
		for (QObject* child : window->findChildren<QObject*>()) {

			std::cout << "child_name:" << child->objectName().toStdString() << std::endl;

			continue;

			if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(child)) {
				// 找到文本框，您可以在这里进行处理
				//qDebug() << "Found QLineEdit:" << lineEdit->objectName() << lineEdit->text();
				// 可以选择隐藏文本框
				// lineEdit->hide();
				std::cout << "objectName:" << lineEdit->objectName().toStdString() << ", text:" << lineEdit->text().toStdString() << std::endl;

				
				

				//QMetaObject::invokeMethod(window, [=]() {
				//	lineEdit->hide();
				//});
			}
		}

		
	}
}
#endif

#if 0
void FindWidget() {
	while (true)
	{
		Sleep(3000);


		auto window = QApplication::activeWindow();

		if (NULL == window) {
			std::cout << "window is null" << std::endl;
			continue;
		}

		std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;

		if ("QFileDialog" != window->objectName().toStdString()) {
			continue;
		}


		for (QObject* child : window->findChildren<QObject*>()) {

			//std::cout << "child_name:" << child->objectName().toStdString() /*<< std::endl*/;
#if 0
			if ("qt_filesystem_model" == child->objectName().toStdString()) {

				if (QFileSystemModel* file_system_model = qobject_cast<QFileSystemModel*>(child)) {

					std::cout << "file_system_model root_path:"<< file_system_model->rootPath().toStdString() << std::endl;

					QMetaObject::invokeMethod(window, [=]() {
						file_system_model->setRootPath("G:/"); // 这样只会修改文本框里面的值
					});
				}
			}
#endif

			if ("listView" == child->objectName().toStdString()) {

				std::cout << "find obj listView 0" << std::endl;


				if (QListView* list_view = qobject_cast<QListView*>(child)) {

					std::cout << "find obj listView 1" << std::endl;
#if 0
					{
						QStandardItemModel* model = qobject_cast<QStandardItemModel*>(list_view->model());
						if (!model) {
							std::cout << "The model is not a QStandardItemModel.-----------------------------------------------------------" << std::endl;
							
						}
					}

					{
						QFileSystemModel* model = qobject_cast<QFileSystemModel*>(list_view->model());
						if (!model) {
							std::cout << "The model is not a QFileSystemModel.-----------------------------------------------------------" << std::endl;

						}
					}

					{
						QStringListModel* model = qobject_cast<QStringListModel*>(list_view->model());
						if (!model) {
							std::cout << "The model is not a QStringListModel.-----------------------------------------------------------" << std::endl;

						}
					}

					{
						QAbstractTableModel* model = qobject_cast<QAbstractTableModel*>(list_view->model());
						if (!model) {
							std::cout << "The model is not a QAbstractTableModel.-----------------------------------------------------------" << std::endl;

						}
					}

					{
						QAbstractListModel* model = qobject_cast<QAbstractListModel*>(list_view->model());
						if (!model) {
							std::cout << "The model is not a QAbstractListModel.-----------------------------------------------------------" << std::endl;

						}
					}

					{
					
						QAbstractItemModel* model = list_view->model();
						if (model) {
							const QMetaObject* metaObject = model->metaObject();
							std::cout << "Model class name:" << metaObject->className() << std::endl; // QmayaFileDialogProxyModel

							// 输出继承的父类信息
							const QMetaObject* parentMetaObject = metaObject->superClass();
							if (parentMetaObject) {
								std::cout << "Parent class name:" << parentMetaObject->className(); // QSortFilterProxyModel
							}
						}
					
					}
					{
					
						QAbstractItemModel* model = list_view->model();
						if (model) {
							std::cout << "Model type:" << model->metaObject()->className() << std::endl;
						}
					
					}
#endif
					
					
					QSortFilterProxyModel* model = qobject_cast<QSortFilterProxyModel*>(list_view->model());

					int c_row = -1;
					for (int row = 0; row < model->rowCount(); ++row) {
						QModelIndex index = model->index(row, 0); // 获取指定行的索引
						QVariant value = model->data(index, Qt::DisplayRole); // 获取数据

						if (value.isValid()) {
							std::cout << value.toString().toStdString() << std::endl; // 打印项的文本
						}

						if (value.toString().contains("C:")) { // to do 忽略大小写

							
							c_row = row;

							std::cout << "contains c c_row:" << c_row << std::endl;

							QMetaObject::invokeMethod(window, [=]() {
								list_view->setStyleSheet("background-color:#ff0000;");
								list_view->hide();
								list_view->setRowHidden(c_row, true);
							});
						}
					}

					

#if 0
					QModelIndex proxyIndex = model->index(c_row, 0); // 获取代理模型的索引
					if (proxyIndex.isValid()) {
						// 将代理索引映射到源模型索引
						QModelIndex sourceIndex = model->mapToSource(proxyIndex);

						std::cout << "sourceIndex: " << sourceIndex.row()  << "|" << sourceIndex.column() << std::endl;

						QMetaObject::invokeMethod(window, [=]() {
							// 获取源模型
							QAbstractItemModel* sourceModel = model->sourceModel();

							std::cout << "sourceModel:" << sourceModel << std::endl;

							sourceModel->setData(sourceIndex, "");

							if (sourceModel && sourceModel->removeRows(sourceIndex.row(), 1)) {
								// 删除成功
								std::cout << "remove ok" << std::endl;
							}
							else {

								std::cout << "remove error" << std::endl;
							}
						});
					}
#endif

#if 0

					int c_row = -1;

					// 遍历模型中的所有项并打印其值
					for (int row = 0; row < model->rowCount(); ++row) {
						QModelIndex index = model->index(row, 0); // 获取指定行的索引
						QVariant value = model->data(index, Qt::DisplayRole); // 获取数据

						if (value.isValid()) {
							std::cout << value.toString().toStdString() << std::endl; // 打印项的文本
						}

						if (value.toString().contains("C:")) { // to do 忽略大小写
						
							c_row = row;
						
						}
						{
						
							QModelIndex index = model->index(row, 1); // 获取指定行的索引
							QVariant value = model->data(index, Qt::DisplayRole); // 获取数据

							if (value.isValid()) {
								std::cout << "row, 1" <<  value.toString().toStdString() << std::endl; // 打印项的文本
							}

							if (value.toString().contains("C:")) { // to do 忽略大小写

								c_row = row;

							}
						
						
						}
					}



					QMetaObject::invokeMethod(window, [=]() {
						if (c_row < 0) {
							return;
						}
						//model->select();
						bool res = model->removeRow(c_row);;

						std::cout << "c_row:" << c_row << ",model removeRow  res:" << res << std::endl;
					});
#endif
				}
			
			}



#if 0
			continue;

			if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(child)) {
				// 找到文本框，您可以在这里进行处理
				//qDebug() << "Found QLineEdit:" << lineEdit->objectName() << lineEdit->text();
				// 可以选择隐藏文本框
				// lineEdit->hide();
				std::cout << "objectName:" << lineEdit->objectName().toStdString() << ", text:" << lineEdit->text().toStdString() << std::endl;




				//QMetaObject::invokeMethod(window, [=]() {
				//	lineEdit->hide();
				//});
			}
#endif
		}
	}
}
#endif

void traverseModel(QFileSystemModel* model, const QModelIndex& index) {
	// 获取当前项的文件路径
	QString path = model->filePath(index);
	std::cout << "Current Path:" << path.toStdString() << std::endl;

	// 遍历当前项的子项
	int rowCount = model->rowCount(index);
	for (int row = 0; row < rowCount; ++row) {
		QModelIndex childIndex = model->index(row, 0, index); // 获取子项索引
		traverseModel(model, childIndex); // 递归遍历子项
	}
}

#if 1
void FindWidget() {
	while (true)
	{
		Sleep(3000);


		auto window = QApplication::activeWindow();

		if (NULL == window) {
			std::cout << "window is null" << std::endl;
			continue;
		}

		std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;

		if ("QFileDialog" != window->objectName().toStdString()) {
			continue;
		}


		for (QObject* child : window->findChildren<QObject*>()) {

			//std::cout << "child_name:" << child->objectName().toStdString() /*<< std::endl*/;
#if 0
			if ("qt_filesystem_model" == child->objectName().toStdString()) {

				if (QFileSystemModel* file_system_model = qobject_cast<QFileSystemModel*>(child)) {

					std::cout << "file_system_model root_path:" << file_system_model->rootPath().toStdString() << std::endl;




					QMetaObject::invokeMethod(window, [=]() {
						//file_system_model->setRootPath("G:/"); // 这样只会修改文本框里面的值

						
					});
				}
			}
#endif
			if ("treeView" == child->objectName().toStdString()) {

				std::cout << "find obj treeView 0" << std::endl;

				if (QTreeView* tree_view = qobject_cast<QTreeView*>(child)) {
		
					std::cout << "find obj treeView 1" << std::endl;

					QMetaObject::invokeMethod(window, [=]() {
						
						tree_view->setStyleSheet("background-color:#ff0000;");

					});
				}
			}

			if ("listView" == child->objectName().toStdString()) {

				std::cout << "find obj listView 0" << std::endl;


				if (QListView* list_view = qobject_cast<QListView*>(child)) {

					std::cout << "find obj listView 1" << std::endl;

					


					auto* model = qobject_cast<QSortFilterProxyModel*>(list_view->model());

					int c_row = -1;
					for (int row = 0; row < model->rowCount(); ++row) {
						QModelIndex index = model->index(row, 0); // 获取指定行的索引
						QVariant value = model->data(index, Qt::DisplayRole); // 获取数据

						if (value.isValid()) {
							std::cout << value.toString().toStdString() << std::endl; // 打印项的文本
						}

						if (value.toString().contains("C:")) { // to do 忽略大小写


							c_row = row;

							std::cout << "contains c c_row:" << c_row << std::endl;

							QMetaObject::invokeMethod(window, [=]() {
							
								list_view->setStyleSheet("background-color:#0000ff;");
								list_view->setRowHidden(c_row, true);
							});
						}
					}
				}
			}
		}
	}
}

#endif

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
			//初始化
			if (MH_Initialize() != MH_OK)
			{
				OutputDebugStringA("Initialize err");
				break;
			}
			//创建Hook
			if (MH_CreateHook(GetLogicalDrives, &DetourGetLogicalDrives, reinterpret_cast<LPVOID*>(&GetLogicalDrives_ptr)) != MH_OK)
			{
				OutputDebugStringA("MH_CreateHook err");
				break;
			}
			//使目标函数Hook生效
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
