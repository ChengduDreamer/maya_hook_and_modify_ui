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
#include <qlabel.h>
#include <qscrollarea.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qmessagebox.h>
#include <qevent.h>

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

std::string Utf8ToGbk(const char* src_str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}

static QString g_root_1_path = "G:/temp";

static QString g_root_2_path = "G:\\temp";

static QString g_current_path;

static QTreeView* g_tree_view = nullptr;

static QListView* g_list_view = nullptr;

static QComboBox* g_combo_box = nullptr;

// �����ݴ����
// maya ����汾���� ֧��2024
#if 1
void FindWidget() {
	while (true)
	{
		Sleep(3010);

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

			if ("treeView" == child->objectName().toStdString()) {
				std::cout << "find obj treeView 0" << std::endl;
				if (g_tree_view = qobject_cast<QTreeView*>(child)) {
					std::cout << "find obj treeView 1" << std::endl;
					auto* model = g_tree_view->model();
					for (int row = 0; row < model->rowCount(); ++row) {
						QModelIndex index = model->index(row, 0); // ��ȡָ���е�����
						QVariant value = model->data(index, Qt::DisplayRole); // ��ȡ����
						if (value.isValid()) {
							std::cout << "treeView" << value.toString().toStdString() << std::endl; // ��ӡ����ı�
						}
						if (!value.toString().contains("G:")) { // to do ���Դ�Сд
							QMetaObject::invokeMethod(window, [=]() {
								//g_tree_view->setRowHidden(row, QModelIndex(), true);
							});
						}
					}
					QMetaObject::invokeMethod(window, [=]() {
						g_tree_view->setStyleSheet("background-color:#ff0000;");
					});
				}
			}

			
			if ("listView" == child->objectName().toStdString()) {
				std::cout << "find obj listView 0" << std::endl;
				if (g_list_view = qobject_cast<QListView*>(child)) {
					std::cout << "find obj listView 1" << std::endl;
					QMetaObject::invokeMethod(window, [=]() {
						g_list_view->setStyleSheet("background-color:#0000ff;");
					});
					auto* model = g_list_view->model();
					for (int row = 0; row < model->rowCount(); ++row) {
						QModelIndex index = model->index(row, 0); // ��ȡָ���е�����
						QVariant value = model->data(index, Qt::DisplayRole); // ��ȡ����
						if (value.isValid()) {
							std::cout << value.toString().toStdString() << std::endl; // ��ӡ����ı�
						}
						if (!value.toString().contains("G:")) { // to do ���Դ�Сд
							QMetaObject::invokeMethod(window, [=]() {
								//list_view->setRowHidden(row, true);
							});
						}
					}
				}
			}

			
			//·��
			if ("lookInCombo" == child->objectName().toStdString()) {
				if (g_combo_box = qobject_cast<QComboBox*>(child)) {
					g_current_path = g_combo_box->currentText();
					std::string combo_box_text = g_combo_box->currentText().toStdString();
					std::cout << "-------------------------------------------------------lookInCombo:" << combo_box_text << std::endl;


					const QMetaObject* metaObject = child->metaObject();
					std::cout << "child class name:" << metaObject->className() << std::endl; // QmayaFileDialogProxyModel

					QMetaObject::invokeMethod(window, [=]() {
						//g_combo_box->setEnabled(false);

						

						if (!g_current_path.startsWith(g_root_2_path)) {

							//std::cout << "not startsWith:" << g_root_path.toStdString() << std::endl;
							//combo_box->setEditable(true);
							//combo_box->setCurrentText(g_root_path);           
							//file_system_model->setRootPath();
							//emit combo_box->currentTextChanged(g_root_path); // û��Ӧ
							//emit combo_box->activated(g_root_path);
							//QObject::connect(combo_box, &QComboBox::activated, [](int index) {
							//	QMessageBox::information(nullptr, "Selected", QString("You selected index: %1").arg(index));
							//});
						}
						else {
							std::cout << "startsWith" << std::endl;
						}
					});
				}
			}


			QFileSystemModel* file_system_model = nullptr;
			if ("qt_filesystem_model" == child->objectName().toStdString()) {
				if (file_system_model = qobject_cast<QFileSystemModel*>(child)) {
					std::cout << "file_system_model root_path:" << file_system_model->rootPath().toStdString() << std::endl;
					QMetaObject::invokeMethod(window, [=]() {
						// startrootpath

						std::cout << "combo_box = " << (void*)g_combo_box << std::endl;

						//file_system_model->setRootPath(g_root_1_path);

						//emit file_system_model->rootPathChanged(g_root_1_path);

						//emit file_system_model->directoryLoaded(g_root_1_path);

						if (g_combo_box) {
							std::cout << "--emit combo_box" << std::endl;

							g_combo_box->setCurrentText("XXXXXXXXXXX");

							//emit g_combo_box->currentTextChanged(g_root_2_path); // û��Ӧ
							//emit g_combo_box->activated(g_root_2_path);
							//emit g_combo_box->activated(0);
							//emit g_combo_box->currentIndexChanged(0);
							//emit g_combo_box->textActivated(g_root_2_path);
							//emit g_combo_box->textHighlighted(g_root_2_path);
							//emit g_combo_box->highlighted(0);

							  // �����س����¼�
							QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
							// �����¼�
							QCoreApplication::sendEvent(g_combo_box, event);
							// �ͷ��¼�����
							delete event;

						}

						//if (!file_system_model->rootPath().startsWith(g_root_1_path)) {
						//	std::cout << "file_system_model not startsWith:" << g_root_1_path.toStdString() << std::endl;
						//	file_system_model->setRootPath(g_root_1_path); // ����ֻ���޸��ı��������ֵ
						//	if (g_combo_box) {
						//		std::cout << "emit combo_box" << std::endl;
						//		emit g_combo_box->currentTextChanged(g_root_2_path); // û��Ӧ
						//		emit g_combo_box->activated(g_root_1_path);
						//		emit g_combo_box->activated(0);
						//		emit g_combo_box->currentIndexChanged(0);
						//		emit g_combo_box->textActivated(g_root_1_path);
						//	}
						//	
						//}
					});
				}
			}
			 
			 
			
			
#if 0
			if ("toParentButton" == child->objectName().toStdString()) {

				std::cout << "----------------------------find------------------------------toParentButton" << std::endl;

				const QMetaObject* metaObject = child->metaObject();
				std::cout << "Model class name:" << metaObject->className() << std::endl; // QToolButton


				if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {

					//std::cout << "label:" << label->text().toStdString() << std::endl;

					std::cout << "----------------------------------------------------------toParentButton" << std::endl;
					QMetaObject::invokeMethod(window, [=]() {
						if ("G:/mayadata" == g_root_1_path || "G:\\mayadata" == g_root_2_path) {

							std::cout << "----------------------------------------------------------toParentButton -- 0" << std::endl;
							//btn->setHidden(true);
						}
						else {

							std::cout << "----------------------------------------------------------toParentButton -- 1" << std::endl;
							//btn->setHidden(false);
						}
					});
				}

			}
#endif

			if ("projectArea" == child->objectName().toStdString()) {
				if (QWidget* widget = qobject_cast<QWidget*>(child)) {
					QMetaObject::invokeMethod(window, [=]() {
						widget->setStyleSheet("background-color:#00ff00;"); // ���½���Ŀ�ǿ�
						widget->hide();
					});
				}
			}

			if ("ProjectFoldersSplitter" == child->objectName().toStdString()) {
				if (QWidget* projwidget = qobject_cast<QWidget*>(child)) {
					//std::cout << "label:" << label->text().toStdString() << std::endl;
					QMetaObject::invokeMethod(window, [=]() {
						projwidget->setStyleSheet("background-color:#0000ff;"); // ���Ͻ��ļ����ǿ�
						projwidget->hide();
					});

					for (QObject* projchild : projwidget->findChildren<QObject*>()) {
						//std::cout << "ProjectFoldersSplitter child_name:" << projchild->objectName().toStdString() << std::endl;
						//projectArea //û��Ӧ
						// qt_scrollarea_viewport
						if ("qt_scrollarea_viewport" == projchild->objectName().toStdString()) {
							if (QWidget* qt_scrollarea_viewport = qobject_cast<QWidget*>(projchild)) {
								std::cout << "qt_scrollarea_viewport to QWidget" << std::endl;
								QMetaObject::invokeMethod(window, [=]() {
									qt_scrollarea_viewport->setStyleSheet("background-color:#ffff00;"); //�����Ͻ� �ļ����б�
									qt_scrollarea_viewport->hide();
								});
							}
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
