#include "MinHook.h"
#include <iostream>
#include <fileapi.h>
#include <Windows.h>
#include <string>
#include <thread>
#include <atomic>
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
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qtimer.h>
#include <qboxlayout.h>
#include <qmenu.h>
#include <qaction.h>
#include <qwidgetaction.h>
#include <qmenubar.h>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QtQuickWidgets/QQuickWidget>
#include <QtQuick/qquickitem.h>


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

static std::atomic<bool> g_stop_flag = false;

static QMessageBox* g_message_box = nullptr;

bool HandleTargetDir() {
	QStringList args = qApp->arguments();
	std::cout << "args size:" << args.size() << std::endl;
	if (args.size() < 3) {
		qApp->exit();
		return false;
	}
	int args_index = 0;
	QString target_dir_path;
	for (auto arg : args) {
		std::cout << "arg:" << arg.toStdString() << std::endl;
		if ("-proj" == arg) {
			if (args_index + 1 < args.size()) {
				target_dir_path = args[args_index + 1];
			}
		}
		++args_index;
	}
	if (target_dir_path.isEmpty()) {
		return false;
	}
	std::cout << "target_dir_path:" << target_dir_path.toStdString() << std::endl;

	QDir target_dir{ target_dir_path };
	if (!target_dir.exists()) {
		std::cout << "target_dir is not exists." << std::endl;
		target_dir.mkpath(target_dir_path);
	}
	if (!target_dir.exists()) {
		return false;
	}
	g_root_1_path = target_dir_path;
	g_root_2_path = target_dir_path.replace("/", "\\");
	return true;
}

bool CheckEnv() {
	{
		const std::string expect_value = "1";
		const char* varName = "MAYA_NO_HOME";
		DWORD bufferSize = 256;
		char value[256] = { 0, };
		DWORD result = GetEnvironmentVariableA(varName, value, bufferSize);
		if (result > 0 && result < bufferSize) {// �ɹ���ȡ��������
			std::cout << varName << " = " << value << std::endl;
			std::string real_value = value;
			if (real_value != expect_value) {
				return false;
			}
		}
		else if (result == 0) { // ��������������
			std::cout << varName << " is not set." << std::endl;
			return false;
		}
		else { // ������������
			return false;
			std::cout << "Buffer size is too small. Required size: " << result << std::endl;
			return false;
		}
	}

	{
		const std::string expect_value = "1";
		const char* varName = "MAYA_NO_HOME_ICON";
		DWORD bufferSize = 256;
		char value[256] = { 0, };
		DWORD result = GetEnvironmentVariableA(varName, value, bufferSize);
		if (result > 0 && result < bufferSize) {// �ɹ���ȡ��������
			std::cout << varName << " = " << value << std::endl;
			std::string real_value = value;
			if (real_value != expect_value) {
				return false;
			}
		}
		else if (result == 0) { // ��������������
			std::cout << varName << " is not set." << std::endl;
			return false;
		}
		else { // ������������
			std::cout << "Buffer size is too small. Required size: " << result << std::endl;
			return false;
		}
	}
	return true;
}

HHOOK hHook;
HINSTANCE g_hInstance = NULL;

// ���ӹ���
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	std::cout << "KeyboardHookProc-----------" << std::endl;
	if (nCode == HC_ACTION) {
		//KBDLLHOOKSTRUCT* pKBDLLHookStruct = (KBDLLHOOKSTRUCT*)lParam;
		//
		//// ��鰴���Ƿ�Ϊ Alt + Home
		//if (wParam == WM_KEYDOWN &&
		//	(GetKeyState(VK_MENU) & 0x8000) && // ��� Alt ���Ƿ���
		//	pKBDLLHookStruct->vkCode == VK_HOME) {
		//	std::cout << "Alt + Home pressed!" << std::endl;
		//	return 1; // ��ֹ�������������ϼ�
		//}

		MSG* p = (MSG*)lParam;
		//�ж��Ƿ��ɻ�����Ϣ
		if (p->message == WM_KEYDOWN)
		{
		
			std::cout << "KeyboardHookProc-----------" << std::endl;
		
		}


	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// ��װ����
void SetKeyboardHook() {
	hHook = SetWindowsHookEx(WH_GETMESSAGE, KeyboardHookProc, g_hInstance, 0);
	if (!hHook) {
		std::cout << "Failed to install hook!" << std::endl;
	}
	else {
		std::cout << "install hook!" << std::endl;
	}
}

// �����ݴ����
// maya ����汾���� ֧��2024
// ���� g:/temp  g:/temp1234 ������  ����һ�� ��Ⱦ�˵�Ȩ�޿���
// ��̩ӳ��Ŀ¼ ��������
// -noAutoloadPlugins �������κβ��

// MAYA_NO_HOME_ICON
// MAYA_NO_HOME

// �ص���ҳ  ��ݼ�
// ���������
// ���������


/*
��־:
-v                       ��ӡ��Ʒ�汾��ʶ����
-batch                   ������������ģʽ
-prompt                  �����ڽ���ʽ�� GUI ģʽ
-proj [dir]              ��ָ������ĿĿ¼�в����ļ�
-command [mel command]   ����ʱ����ָ��������
-file [file]             ��ָ�����ļ�
-script [file]           ����ʱԴ��ָ���ļ�
-log [file]              �� stdout ��Ϣ�� stderr ��Ϣ���Ƶ�ָ���ļ�
							 (ʹ�������ļ���)
-hideConsole              ���ؿ���̨����
-recover                 �ָ���һ��־�ļ�
							 (ʹ�á�Render -help����ø���ѡ��)
-optimizeRender [file] [outfile]
						 Ϊ��ȾĿ���Ż� maya �ļ�
							 Ч�ʣ����������������ļ���
					  (ʹ�á�maya -optimizeRender -help����ø���ѡ��)
-archive [file]          ��ʾ�鵵ָ������������ļ��б�
-noAutoloadPlugins       ��Ҫ�Զ������κβ����
-3                       ���� Python 3000 �����Ծ���
-help                    ��ӡ����Ϣ
*/

void HandleQFileDialog(QWidget* window) {

	QTreeView* tree_view = nullptr;
	QListView* list_view = nullptr;
	QComboBox* combo_box = nullptr;

	for (QObject* child : window->findChildren<QObject*>()) {
		//std::cout << "child_name:" << child->objectName().toStdString() /*<< std::endl*/;
		//·��
		if ("lookInCombo" == child->objectName().toStdString()) {
			if (combo_box = qobject_cast<QComboBox*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					g_current_path = combo_box->currentText();
					std::string combo_box_text = combo_box->currentText().toStdString();
					std::cout << "-------------------------------------------------------lookInCombo:" << combo_box_text << std::endl;
					if (!g_current_path.startsWith(g_root_2_path)) {

						std::cout << "-----------not startsWith:" << g_root_2_path.toStdString() << std::endl;
						combo_box->setEnabled(true); // ���Ϊfalse,�޷���ӦQKeyEvent
						combo_box->setCurrentText(g_root_2_path);
						QKeyEvent key_event{ QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier };
						bool res = QCoreApplication::sendEvent(combo_box, &key_event);
						if (!res) {
							std::cout << "sendEvent key_event error" << std::endl;
						}
					}
					else {
						std::cout << "startsWith" << std::endl;
					}
					combo_box->setEnabled(false);
				});
			}
		}

		QFileSystemModel* file_system_model = nullptr;
		if ("qt_filesystem_model" == child->objectName().toStdString()) {
			if (file_system_model = qobject_cast<QFileSystemModel*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					//file_system_model->setRootPath(g_root_1_path); // ����ֻ���޸��ı��������ֵ
					});
			}
		}

		if ("toParentButton" == child->objectName().toStdString()) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					if (g_current_path == g_root_1_path || g_current_path == g_root_2_path) {
						btn->setHidden(true);
					}
					else {
						btn->setHidden(false);
					}
					});
			}
		}

		if ("backButton" == child->objectName().toStdString()) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					btn->setHidden(true);
					});
			}
		}

		if ("forwardButton" == child->objectName().toStdString()) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					btn->setHidden(true);
					});
			}
		}

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


void LimitGivenDir() {

	

	if (!HandleTargetDir()) {
		QMetaObject::invokeMethod(qApp, [=]() {
			QMessageBox message_box{ QMessageBox::Warning,  "warning", "Unable to find user directory, please contact the administrator." };
			message_box.exec();
		});
	}

	if (!CheckEnv()) {
		QMetaObject::invokeMethod(qApp, [=]() {
			QMessageBox message_box{ QMessageBox::Warning,  "warning", "Environment variables not set correctly or set error, please contact the administrator. The program is about to exit." };
			message_box.exec();
			QTimer::singleShot(3000, []() {
				qApp->exit();
				exit(-1);
			});
		});
		return;
	}

	while (!g_stop_flag) {
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		auto window = QApplication::activeWindow();
		if (NULL == window) {
			std::cout << "window is null" << std::endl;
			continue;
		}
		std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;
		if ("MayaAppHomeWindow" == window->objectName().toStdString()) {
			//window->hide();
			//continue;
			for (QObject* child : window->findChildren<QObject*>()) {
				std::cout << "MayaAppHomeWindow child_name:" << child->objectName().toStdString();
				
				const QMetaObject* metaObject = child->metaObject();
				std::cout << ", class name:" << metaObject->className();

				const QMetaObject* parentMetaObject = metaObject->superClass();
				if (parentMetaObject) {
					std::cout << ",Parent class name:" << parentMetaObject->className();
					
					if (QString(parentMetaObject->className()).startsWith("QQuickWidget")) {
						auto quick_widget = qobject_cast<QQuickWidget*>(child);
						if (quick_widget) {
							//quick_widget->hide();
							//quick_widget->setEnabled(false);
							std::cout << ",find QQuickWidget" << std::endl;

							//for (QObject* quick_child : quick_widget->findChildren<QObject*>()) {
							//
							//	std::cout << "quick_child name:" << quick_child->objectName().toStdString();
							//
							//	const QMetaObject* metaObject = quick_child->metaObject();
							//	std::cout << ", quick_child class name:" << metaObject->className();
							//
							//	const QMetaObject* parentMetaObject = metaObject->superClass();
							//	if (parentMetaObject) {
							//		std::cout << ",quick_child Parent class name:" << parentMetaObject->className();
							//	}
							//}

							QQuickItem* rootItem = quick_widget->rootObject();
							if (rootItem) {
								for (QObject* quick_child : rootItem->findChildren<QObject*>()) {
									
									std::cout << "quick_child name:" << quick_child->objectName().toStdString();

									const QMetaObject* metaObject = quick_child->metaObject();

									std::cout << ", quick_child class name:" << metaObject->className();

									const QMetaObject* parentMetaObject = metaObject->superClass();
									if (parentMetaObject) {
										std::cout << ",quick_child Parent class name:" << parentMetaObject->className();
									}

								}
							}


						}
					}

				}

				if (QString(metaObject->className()).startsWith("QWebEnginePage")) {
					auto page = qobject_cast<QWebEnginePage*>(child);
					if (page) {
						//page->setVisible(false);
					}
				}

				if (QString(metaObject->className()).startsWith("QVBoxLayout")) {
					auto vlayout = qobject_cast<QVBoxLayout*>(child);
					if (vlayout) {
						auto vlayout_children = vlayout->findChildren<QObject*>();
						for (auto vlayout_child : vlayout_children) {
							std::cout << "vlayout_child:" << vlayout_child->objectName().toStdString();

							const QMetaObject* metaObject = vlayout_child->metaObject();
							std::cout << ",vlayout_child class name:" << metaObject->className();

							const QMetaObject* parentMetaObject = metaObject->superClass();
							if (parentMetaObject) {
								std::cout << ",vlayout_child Parent class name:" << parentMetaObject->className();

								

							}
						}
					}
				}

				std::cout << std::endl;
			}





		}


		if ("" == window->objectName().toStdString()) {
			std::cout << "null title:" << window->windowTitle().toStdString() << std::endl;
			for (QObject* child : window->findChildren<QObject*>()) {
			//	std::cout << "null child_name:" << child->objectName().toStdString() << std::endl;
			}
		}


		if ("MayaWindow" == window->objectName().toStdString()) {
			for (QObject* child : window->findChildren<QObject*>()) {
				std::cout << "MayaWindow child_name:" << child->objectName().toStdString();

				const QMetaObject* metaObject = child->metaObject();
				std::cout << ", class name:" << metaObject->className();

				if ("wmContentBrowser" == child->objectName()) {
					auto widget_action = qobject_cast<QWidgetAction*>(child);
					if (widget_action) {
						auto a = widget_action->text().toStdString();
						std::cout << ",wmContentBrowser widget_action text" << Utf8ToGbk(a.c_str()) << std::endl;
						widget_action->setEnabled(false);
					}
				}

				const QMetaObject* parentMetaObject = metaObject->superClass();
				if (parentMetaObject) {
					std::cout << ",Parent class name:" << parentMetaObject->className();
					if (QString(parentMetaObject->className()).contains("QWidgetAction")) {
						std::cout << ",--------------QWidgetAction--------------,";
						auto widget_action = qobject_cast<QWidgetAction*>(child);
						if (widget_action) {
							auto a = widget_action->text().toStdString();
							std::cout << ", widget_action text" << Utf8ToGbk(a.c_str()) << std::endl;
						}
					}



					if (QString(parentMetaObject->className()).contains("QMenuBar")) {
						std::cout << ",--------------QMenuBar--------------,";
						auto menu_bar = qobject_cast<QMenuBar*>(child);
						if (menu_bar) {
							auto actions = menu_bar->actions();
							for (auto action : actions) {
								std::cout << ",action objname:" << action->objectName().toStdString();
								auto a = action->text().toStdString();
								std::cout << ",action text:" << Utf8ToGbk(a.c_str());
								QMenu* menu = action->menu();
								auto mans = menu->actions();
								for (auto man : mans) {
									std::cout << ",menu action objname:" << man->objectName().toStdString();
									auto a = man->text().toStdString();
									std::cout << ",menu action text:" << Utf8ToGbk(a.c_str()) << std::endl;
									// ������ļ�
									if ("menuItem564" == man->objectName()) {
										man->setEnabled(false);
									}
									if ("FileMenuRecentFileItems" == man->objectName()) {
										man->setEnabled(false);
									}
									if ("FileMenuRecentBackupItems" == man->objectName()) {
										man->setEnabled(false);
									}
									if ("FileMenuRecentProjectItems" == man->objectName()) {
										man->setEnabled(false);
									}

									if ("wmContentBrowser" == man->objectName()) {
										man->setEnabled(false);
									}

									if ("menuItem535" == man->objectName()) {
										man->setEnabled(false);
									}
								}
							}
						}
					}



				}
				//std::cout << std::endl;

#if 0

				// ��һ�δ��� ���� maya������ʾֹͣ���� to do ���ص�
				if ("workspaceSelectorMenu" == child->objectName()) {
				
					const QMetaObject* metaObject = child->metaObject();
					std::cout << "-----------------------workspaceSelectorMenu class name:" << metaObject->className() << std::endl;
				
					const QMetaObject* parentMetaObject = metaObject->superClass();
					if (parentMetaObject) {
						std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl;;  // qmenu
					}
				
					if (auto menu = qobject_cast<QMenu*>(child)) {
					
						for (QObject* menu_child : menu->findChildren<QObject*>()) {

							std::cout << " menu_child name:" << menu_child->objectName().toStdString() << std::endl;
						}


						QMetaObject::invokeMethod(window, [=]() {
							//menu->hide();
							menu->setStyleSheet("background-color:#ff0000;");

							auto actions = menu->actions();
							for (auto action : actions) {
								std::cout << "action objname:" << action->objectName().toStdString() << std::endl;
								auto a = action->text().toStdString();
								std::cout << "action text:" << Utf8ToGbk(a.c_str()) << std::endl;
							}
						});
					}
					
					if (auto combobox = qobject_cast<QComboBox*>(child)) {
					
						for (QObject* combobox_child : combobox->findChildren<QObject*>()) {

							std::cout << "combobox_child name:" << combobox_child->objectName().toStdString() << std::endl;
						}
					}
				}

				if ("_layout" == child->objectName()) {

					const QMetaObject* metaObject = child->metaObject();
					std::cout << "-----------------------_layout class name:" << metaObject->className() << std::endl;

					const QMetaObject* parentMetaObject = metaObject->superClass();
					if (parentMetaObject) {
						std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl;;  // qmenu
					}

					if (auto layout = qobject_cast<QLayout*>(child)) {
					
						for (QObject* combobox_child : layout->findChildren<QObject*>()) {

							std::cout << "layout_child name:" << combobox_child->objectName().toStdString() << std::endl;
						}
					}

					
				}
#endif

				//if ("workspaceSelectorMenu" == child->objectName()) {
				//
				//	const QMetaObject* metaObject = child->metaObject();
				//	std::cout << "-----------------------workspaceSelectorMenu class name:" << metaObject->className() << std::endl;
				//
				//	const QMetaObject* parentMetaObject = metaObject->superClass();
				//	if (parentMetaObject) {
				//		std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl;;  // qmenu
				//	}
				//
				//
				//	auto menu = qobject_cast<QMenu*>(child);
				//
				//
				//	for (QObject* menu_child : menu->findChildren<QObject*>()) {
				//
				//		std::cout << " menu_child name:" << menu_child->objectName().toStdString() << std::endl;
				//	}
				//
				//
				//	QMetaObject::invokeMethod(window, [=]() {
				//		//menu->hide();
				//		menu->setStyleSheet("background-color:#ff0000;");
				//
				//		auto actions = menu->actions();
				//		for (auto action : actions) {
				//			std::cout << "action objname:" << action->objectName().toStdString() << std::endl;
				//			auto a = action->text().toStdString();
				//			std::cout << "action text:" << Utf8ToGbk(a.c_str()) << std::endl;
				//		}
				//	});
				//
				//}



				//if ("qt_menubar_ext_button" == child->objectName()) {
				//	const QMetaObject* metaObject = child->metaObject();
				//	std::cout << "----------------------qt_menubar_ext_button class name:" << metaObject->className() << std::endl; // QToolButton
				//
				//	const QMetaObject* parentMetaObject = metaObject->superClass();
				//	if (parentMetaObject) {
				//		std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl; // qmenu
				//	}
				//	
				//	QToolButton* btn = qobject_cast<QToolButton*>(child);
				//	if (!btn) {
				//		std::cout << "qt_menubar_ext_button not QToolButton" << std::endl;
				//		continue;
				//	}
				//	
				//	QMetaObject::invokeMethod(window, [=]() {
				//		std::cout << "qt_menubar_ext_button-----------------------------" << btn->text().toStdString() << std::endl;
				//		btn->setFixedSize(100, 100);
				//	});
				//}

				//if ("WorkspaceMenuID_0" == child->objectName()) {
				//	const QMetaObject* metaObject = child->metaObject();
				//	std::cout << "----------------------WorkspaceMenuID_0 class name:" << metaObject->className() << std::endl; // QToolButton
				//
				//	const QMetaObject* parentMetaObject = metaObject->superClass();
				//	if (parentMetaObject) {
				//		std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl;; // qmenu
				//	}
				//
				//	QWidgetAction* action = qobject_cast<QWidgetAction*>(child);
				//	if (!action) {
				//		std::cout << "WorkspaceMenuID_0 not QWidgetAction" << std::endl;
				//		continue;
				//	}
				//
				//	std::cout << "WorkspaceMenuID_0:" << action->text().toStdString() << std::endl;
				//
				//
				//}


				//if ("OpenSceneButtonRecentFileItems" == child->objectName()) {
				//
				//	const QMetaObject* metaObject = child->metaObject();
				//	std::cout << "-----------------------OpenSceneButtonRecentFileItems class name:" << metaObject->className() << std::endl;
				//
				//	const QMetaObject* parentMetaObject = metaObject->superClass();
				//	if (parentMetaObject) {
				//		std::cout << "-----------------------Parent class name:" << parentMetaObject->className() << std::endl; // qmenu
				//	}
				//
				//	//for (QObject* menu_child : window->findChildren<QObject*>()) {
				//	//
				//	//
				//	//
				//	//}
				//
				//	auto menu = qobject_cast<QMenu*>(child);
				//
				//
				//	for (QObject* menu_child : menu->findChildren<QObject*>()) {
				//		
				//		std::cout << "menu_child name:" << menu_child->objectName().toStdString() << std::endl;
				//	}
				//
				//
				//	QMetaObject::invokeMethod(window, [=]() {
				//		//menu->hide();
				//		menu->setStyleSheet("background-color:#ff0000;");
				//
				//		auto actions = menu->actions();
				//		for (auto action : actions) {
				//			std::cout << "action" << action->objectName().toStdString() << std::endl;
				//			std::cout << "action" << action->text().toStdString() << std::endl;
				//		}
				//	});
				//
				//}

				//if ("menuItem529" == child->objectName()) {
				//	auto widget = qobject_cast<QWidget*>(child);
				//	if (!widget) {
				//		std::cout << "menuItem529 not widget" << std::endl;
				//		continue;
				//	}
				//
				//	
				//	QMetaObject::invokeMethod(window, [=]() {
				//		//menu->hide();
				//		widget->setStyleSheet("background-color:#ff0000;");
				//	});
				//}

				//if ("WorkspaceMenuID_1" == child->objectName()) {
				//	auto widget = qobject_cast<QWidget*>(child);
				//
				//	if (!widget) {
				//		std::cout << "WorkspaceMenuID_1 not widget" << std::endl;
				//		continue;
				//	}
				//
				//	QMetaObject::invokeMethod(window, [=]() {
				//		//menu->hide();
				//		widget->setStyleSheet("background-color:#ff0000;");
				//	});
				//}
			}
		}
		//continue;
		if ("QFileDialog" != window->objectName().toStdString()) {
			continue;
		}
		if (!HandleTargetDir()) {
			QMetaObject::invokeMethod(qApp, [=]() {
				if (g_message_box) {
					return;
				}
				g_message_box = new QMessageBox{ QMessageBox::Warning,  "warning", "Unable to find user directory, please contact the administrator." };
				g_message_box->exec();
				g_message_box = nullptr;
				window->close();
			});
			continue;
		}

		HandleQFileDialog(window);
	}
}


std::thread find_widget_thread;

static int count = 100;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

	g_hInstance = (HINSTANCE)hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		do
		{
			--count;

			if (count < 0) {
				exit(0);
			}

			AllocConsole();
			freopen("CONOUT$", "w", stdout);
			std::cout << "This works" << std::endl;

			//SetKeyboardHook();

			
			find_widget_thread = std::thread(LimitGivenDir);

		} while (false);

	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		g_stop_flag = true;
		if (find_widget_thread.joinable()) {
			find_widget_thread.join();
		}
	}
		break;
	}
	return TRUE;
}
