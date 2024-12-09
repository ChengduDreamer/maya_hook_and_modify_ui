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
#include <locale>
#include <string>
#include <codecvt>
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
		if (result > 0 && result < bufferSize) {// 成功获取环境变量
			std::cout << varName << " = " << value << std::endl;
			std::string real_value = value;
			if (real_value != expect_value) {
				return false;
			}
		}
		else if (result == 0) { // 环境变量不存在
			std::cout << varName << " is not set." << std::endl;
			return false;
		}
		else { // 缓冲区不够大
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
		if (result > 0 && result < bufferSize) {// 成功获取环境变量
			std::cout << varName << " = " << value << std::endl;
			std::string real_value = value;
			if (real_value != expect_value) {
				return false;
			}
		}
		else if (result == 0) { // 环境变量不存在
			std::cout << varName << " is not set." << std::endl;
			return false;
		}
		else { // 缓冲区不够大
			std::cout << "Buffer size is too small. Required size: " << result << std::endl;
			return false;
		}
	}
	return true;
}

HHOOK hHook;
HINSTANCE g_hInstance = NULL;

// 钩子过程
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	std::cout << "KeyboardHookProc-----------" << std::endl;
	if (nCode == HC_ACTION) {
		//KBDLLHOOKSTRUCT* pKBDLLHookStruct = (KBDLLHOOKSTRUCT*)lParam;
		//
		//// 检查按键是否为 Alt + Home
		//if (wParam == WM_KEYDOWN &&
		//	(GetKeyState(VK_MENU) & 0x8000) && // 检查 Alt 键是否按下
		//	pKBDLLHookStruct->vkCode == VK_HOME) {
		//	std::cout << "Alt + Home pressed!" << std::endl;
		//	return 1; // 阻止其他程序处理此组合键
		//}

		MSG* p = (MSG*)lParam;
		//判断是否由击键消息
		if (p->message == WM_KEYDOWN)
		{
		
			std::cout << "KeyboardHookProc-----------" << std::endl;
		
		}


	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// 安装钩子
void SetKeyboardHook() {
	hHook = SetWindowsHookEx(WH_GETMESSAGE, KeyboardHookProc, g_hInstance, 0);
	if (!hHook) {
		std::cout << "Failed to install hook!" << std::endl;
	}
	else {
		std::cout << "install hook!" << std::endl;
	}
}

// 考虑容错机制
// maya 软件版本问题 支持2024
// 考虑 g:/temp  g:/temp1234 这样的  测试一下 渲染端的权限控制
// 马康泰映射目录 传参那里
// -noAutoloadPlugins 不加载任何插件

// MAYA_NO_HOME_ICON
// MAYA_NO_HOME

// 回到主页  快捷键
// 内容浏览器
// 插件管理器
// 脚本管理器 里面的帮助

/*
标志:
-v                       打印产品版本和识别编号
-batch                   适用于批处理模式
-prompt                  适用于交互式非 GUI 模式
-proj [dir]              在指定的项目目录中查找文件
-command [mel command]   启动时运行指定的命令
-file [file]             打开指定的文件
-script [file]           启动时源化指定文件
-log [file]              将 stdout 消息和 stderr 消息复制到指定文件
							 (使用完整文件名)
-hideConsole              隐藏控制台窗口
-recover                 恢复上一日志文件
							 (使用“Render -help”获得更多选项)
-optimizeRender [file] [outfile]
						 为渲染目的优化 maya 文件
							 效率，并将结果置于输出文件中
					  (使用“maya -optimizeRender -help”获得更多选项)
-archive [file]          显示归档指定场景所需的文件列表
-noAutoloadPlugins       不要自动加载任何插件。
-3                       启用 Python 3000 兼容性警告
-help                    打印此消息
*/

void HandleQFileDialog(QWidget* window) {

	QTreeView* tree_view = nullptr;
	QListView* list_view = nullptr;
	QComboBox* combo_box = nullptr;

	for (QObject* child : window->findChildren<QObject*>()) {
		//std::cout << "child_name:" << child->objectName().toStdString() /*<< std::endl*/;
		//路径
		if ("lookInCombo" == child->objectName().toStdString()) {
			if (combo_box = qobject_cast<QComboBox*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					g_current_path = combo_box->currentText();
					std::string combo_box_text = combo_box->currentText().toStdString();
					std::cout << "-------------------------------------------------------lookInCombo:" << combo_box_text << std::endl;
					if (!g_current_path.startsWith(g_root_2_path)) {

						std::cout << "-----------not startsWith:" << g_root_2_path.toStdString() << std::endl;
						combo_box->setEnabled(true); // 如果为false,无法响应QKeyEvent
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
					//file_system_model->setRootPath(g_root_1_path); // 这样只会修改文本框里面的值
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
					widget->setStyleSheet("background-color:#00ff00;"); // 左下角项目那块
					widget->hide();
					});
			}
		}

		if ("ProjectFoldersSplitter" == child->objectName().toStdString()) {
			if (QWidget* projwidget = qobject_cast<QWidget*>(child)) {
				QMetaObject::invokeMethod(window, [=]() {
					projwidget->setStyleSheet("background-color:#0000ff;"); // 左上角文件夹那块
					projwidget->hide();
					});
				for (QObject* projchild : projwidget->findChildren<QObject*>()) {
					//std::cout << "ProjectFoldersSplitter child_name:" << projchild->objectName().toStdString() << std::endl;
					//projectArea //没反应
					// qt_scrollarea_viewport
					if ("qt_scrollarea_viewport" == projchild->objectName().toStdString()) {
						if (QWidget* qt_scrollarea_viewport = qobject_cast<QWidget*>(projchild)) {
							std::cout << "qt_scrollarea_viewport to QWidget" << std::endl;
							QMetaObject::invokeMethod(window, [=]() {
								qt_scrollarea_viewport->setStyleSheet("background-color:#ffff00;"); //有左上角 文件夹列表
								qt_scrollarea_viewport->hide();
								});
						}
					}
				}
			}
		}
	}
}

static std::string ToUTF8(const std::wstring& src) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(src);
}

QWidget* workspace_window = nullptr;

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
			

			QMetaObject::invokeMethod(qApp, [=]() {
				workspace_window->show();
				window->hide();
			});
			continue;

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
		} else if ("" == window->objectName().toStdString()) {
			std::cout << "null title:" << window->windowTitle().toStdString() << std::endl;
			for (QObject* child : window->findChildren<QObject*>()) {
			//	std::cout << "null child_name:" << child->objectName().toStdString() << std::endl;
			}
		} else if ("AboutArnold" == window->objectName().toStdString()) {
			QMetaObject::invokeMethod(qApp, [=]() {
				window->close();
			});
		} else if ("MayaWindow" == window->objectName().toStdString()) {

			workspace_window = window;

			std::wstring lzh_help_sign = L"帮助";

			std::wstring lus_help_sign = L"help";

			std::string zh_help_sign = ToUTF8(lzh_help_sign);

			std::string us_help_sign = ToUTF8(lus_help_sign);

			QString qzh_help_sign = QString::fromStdString(zh_help_sign);

			QString qus_help_sign = QString::fromStdString(us_help_sign);
			{

				auto a = qzh_help_sign.toStdString();
				std::cout << ", qzh_help_sign text" << Utf8ToGbk(a.c_str()) << std::endl;
			}

			for (QObject* child : window->findChildren<QObject*>()) {
				//std::cout << "MayaWindow child_name:" << child->objectName().toStdString();

				const QMetaObject* metaObject = child->metaObject();
				//std::cout << ", class name:" << metaObject->className();

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
					//std::cout << ",Parent class name:" << parentMetaObject->className();
					if (QString(parentMetaObject->className()).contains("QWidgetAction")) {
						//std::cout << ",--------------QWidgetAction--------------,";
						auto widget_action = qobject_cast<QWidgetAction*>(child);
						if (widget_action) {
							auto a = widget_action->text().toStdString();
							std::cout << ", widget_action text" << Utf8ToGbk(a.c_str()) << std::endl;


							QMetaObject::invokeMethod(qApp, [=]() {
								if (widget_action->text().contains(qzh_help_sign, Qt::CaseInsensitive) || widget_action->text().contains(qus_help_sign, Qt::CaseInsensitive)) {
									std::cout << "----------------------widget_action find help--------------------------------" << std::endl;
									widget_action->setEnabled(false);
								}
							});

							
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
								QMetaObject::invokeMethod(qApp, [=]() {
									if (action->text().contains(QString::fromStdString(zh_help_sign), Qt::CaseInsensitive) || action->text().contains(QString::fromStdString(us_help_sign), Qt::CaseInsensitive)) {
										action->setEnabled(false);
										std::cout << "action find help" << std::endl;
									}
								});

								QMenu* menu = action->menu();
								auto mans = menu->actions();
								for (auto man : mans) {
									std::cout << ",menu action objname:" << man->objectName().toStdString();
									auto a = man->text().toStdString();
									std::cout << ",menu action text:" << Utf8ToGbk(a.c_str()) << std::endl;

									QMetaObject::invokeMethod(qApp, [=]() {
										if (man->text().contains(QString::fromStdString(zh_help_sign), Qt::CaseInsensitive) || man->text().contains(QString::fromStdString(us_help_sign), Qt::CaseInsensitive)) {
											man->setEnabled(false);
											std::cout << "menu action find help" << std::endl;
										}

										// 最近的文件
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
									});

									
								}
							}
						}
					}



				}
				//std::cout << std::endl;

#if 0

				// 这一段代码 导致 maya弹窗提示停止工作 to do 隐藏掉
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
