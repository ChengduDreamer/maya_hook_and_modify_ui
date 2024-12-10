#include <Windows.h>
#include <iostream>
#include <fileapi.h>
#include <string>
#include <thread>
#include <atomic>
#include <locale>
#include <codecvt>
#include <mutex>
#include <condition_variable>
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

static std::string ToUTF8(const std::wstring& src) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(src);
}

static QString g_root_1_path = "G:/";

static QString g_root_2_path = "G:\\";

static QString g_current_path;

static std::atomic<bool> g_stop_flag = false;

static QMessageBox* g_message_box = nullptr;

static std::condition_variable g_cv;

static std::mutex g_mutex;

static std::thread g_find_widget_thread;

static QWidget* g_workspace_window = nullptr;

static std::wstring g_lzh_help_sign = L"帮助";

static std::wstring g_lus_help_sign = L"help";

static std::wstring g_lzh_about_sign = L"关于";

static std::wstring g_lus_about_sign = L"about";

static std::string g_zh_help_sign = ToUTF8(g_lzh_help_sign);

static std::string g_us_help_sign = ToUTF8(g_lus_help_sign);

static std::string g_zh_about_sign = ToUTF8(g_lzh_about_sign);

static std::string g_us_about_sign = ToUTF8(g_lus_about_sign);

static QString g_qzh_help_sign = QString::fromStdString(g_zh_help_sign);

static QString g_qus_help_sign = QString::fromStdString(g_us_help_sign);

static QString g_qus_about_sign = QString::fromStdString(g_us_about_sign);

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

void HideHelpBtn(QWidget* window, QObject* child);

// 考虑容错机制
// maya 软件版本问题 支持2024
// 考虑 g:/temp  g:/temp1234 这样的  测试一下 渲染端的权限控制    // 
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


void HandleMayaWindow(QWidget* window) {

	g_workspace_window = window;
	for (QObject* child : window->findChildren<QObject*>()) {
		//std::cout << "MayaWindow child_name:" << child->objectName().toStdString();
		HideHelpBtn(window, child);
		const QMetaObject* metaObject = child->metaObject();
		//std::cout << ", class name:" << metaObject->className();
		{
			const QMetaObject* parentMetaObject = metaObject->superClass();
			if (parentMetaObject) {
				//std::cout << ",Parent class name:" << parentMetaObject->className();
			}
		}

		if ("detailInfoBtn" == child->objectName()) {
			QAbstractButton* btn = qobject_cast<QAbstractButton*>(child);
			if (btn) {
				btn->hide();
			}
			continue;
		}

		if ("wmContentBrowser" == child->objectName()) {
			auto widget_action = qobject_cast<QWidgetAction*>(child);
			if (widget_action) {
				auto a = widget_action->text().toStdString();
				//std::cout << ",wmContentBrowser widget_action text" << Utf8ToGbk(a.c_str()) << std::endl;
				widget_action->setEnabled(false);
			}
			continue;
		}

		auto menu_bar = qobject_cast<QMenuBar*>(child);
		if (menu_bar) {
			auto actions = menu_bar->actions();
			for (auto action : actions) {
				QMenu* menu = action->menu();
				if (menu) {
					auto mans = menu->actions();
					for (auto man : mans) {
						// 主页
						if ("menuItem503" == man->objectName()) {
							man->setEnabled(false);
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
					}
				}
				
			}
		}
	}
}


void HandleQFileDialog(QWidget* window) {
	QTreeView* tree_view = nullptr;
	QListView* list_view = nullptr;
	QComboBox* combo_box = nullptr;
	for (QObject* child : window->findChildren<QObject*>()) {
		//std::cout << "child_name:" << child->objectName().toStdString() /*<< std::endl*/;
		//路径
		HideHelpBtn(window, child);
		QFileSystemModel* file_system_model = nullptr;
		if (child->objectName().contains("lookInCombo", Qt::CaseInsensitive)) {
			if (combo_box = qobject_cast<QComboBox*>(child)) {
				g_current_path = combo_box->currentText();
				std::string combo_box_text = combo_box->currentText().toStdString();
				//std::cout << "-------------------------------------------------------lookInCombo:" << combo_box_text << std::endl;
				if (!g_current_path.startsWith(g_root_2_path)) {

					//std::cout << "-----------not startsWith:" << g_root_2_path.toStdString() << std::endl;
					combo_box->setEnabled(true); // 如果为false,无法响应QKeyEvent
					combo_box->setCurrentText(g_root_2_path);
					QKeyEvent key_event{ QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier };
					bool res = QCoreApplication::sendEvent(combo_box, &key_event);
					if (!res) {
						std::cout << "sendEvent key_event error" << std::endl;
					}
				}
				combo_box->setEnabled(false);
			}
		}
		else if (child->objectName().contains("qt_filesystem_model", Qt::CaseInsensitive)) {
			if (file_system_model = qobject_cast<QFileSystemModel*>(child)) {
				//file_system_model->setRootPath(g_root_1_path); // 这样只会修改文本框里面的值
			}
		}
		else if (child->objectName().contains("toParentButton", Qt::CaseInsensitive)) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				if (g_current_path == g_root_1_path || g_current_path == g_root_2_path) {
					btn->setHidden(true);
				}
				else {
					btn->setHidden(false);
				}
			}
		}
		else if (child->objectName().contains("backButton", Qt::CaseInsensitive)) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				btn->setHidden(true);
			}
		}
		else if (child->objectName().contains("forwardButton", Qt::CaseInsensitive)) {
			if (QToolButton* btn = qobject_cast<QToolButton*>(child)) {
				btn->setHidden(true);
			}
		}
		else if (child->objectName().contains("projectArea", Qt::CaseInsensitive)) {
			if (QWidget* widget = qobject_cast<QWidget*>(child)) {
				widget->hide();// 左下角项目那块
			}
		}
		else if (child->objectName().contains("ProjectFoldersSplitter", Qt::CaseInsensitive)) {
			if (QWidget* projwidget = qobject_cast<QWidget*>(child)) {
				projwidget->hide();// 左上角文件夹那块
				for (QObject* projchild : projwidget->findChildren<QObject*>()) {
					//std::cout << "ProjectFoldersSplitter child_name:" << projchild->objectName().toStdString() << std::endl;
					//projectArea //没反应
					// qt_scrollarea_viewport
					if (projchild->objectName().contains("qt_scrollarea_viewport", Qt::CaseInsensitive)) {
						if (QWidget* qt_scrollarea_viewport = qobject_cast<QWidget*>(projchild)) {
							std::cout << "qt_scrollarea_viewport to QWidget" << std::endl;
							qt_scrollarea_viewport->hide();//有左上角 文件夹列表
						}
					}
				}
			}
		}
	}
}

void HandleMayaHomeWindow(QWidget* window) {
	QKeyEvent key_event{ QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier };
	bool res = QCoreApplication::sendEvent(window, &key_event);
	for (QObject* child : window->findChildren<QObject*>()) {
		const QMetaObject* metaObject = child->metaObject();
		const QMetaObject* parentMetaObject = metaObject->superClass();
		if (parentMetaObject) {
			if (QString(parentMetaObject->className()).startsWith("QQuickWidget")) {
				auto quick_widget = qobject_cast<QQuickWidget*>(child);
				if (quick_widget) {
					quick_widget->setEnabled(false);
				}
			}
		}
	}
}

void HideHelpBtn(QWidget* window, QObject* child) {
	if (QWidgetAction* widget_action = qobject_cast<QWidgetAction*>(child)) {
		if (widget_action->text().contains(g_qzh_help_sign, Qt::CaseInsensitive) || widget_action->text().contains(g_qus_help_sign, Qt::CaseInsensitive)) {
			widget_action->setEnabled(false);
		}
	}
	else if (QMenuBar* menu_bar = qobject_cast<QMenuBar*>(child)) {
		auto actions = menu_bar->actions();
		for (auto action : actions) {
			//std::cout << ",action objname:" << action->objectName().toStdString();
			auto a = action->text().toStdString();
			//std::cout << ",action text:" << Utf8ToGbk(a.c_str());
			if (action->text().contains(g_qzh_help_sign, Qt::CaseInsensitive) || action->text().contains(g_qus_help_sign, Qt::CaseInsensitive)) {
				action->setEnabled(false);
				//std::cout << "action find help" << std::endl;
			}

			QMenu* menu = action->menu();
			auto mans = menu->actions();
			for (auto man : mans) {
				//std::cout << ",menu action objname:" << man->objectName().toStdString();
				auto a = man->text().toStdString();
				//std::cout << ",menu action text:" << Utf8ToGbk(a.c_str()) << std::endl;
				if (man->objectName().contains("ArnoldAbout", Qt::CaseInsensitive) || man->text().contains(g_qus_help_sign, Qt::CaseInsensitive) || man->text().contains(g_qus_help_sign, Qt::CaseInsensitive)) {
					man->setEnabled(false);
					//std::cout << "menu action find help" << std::endl;
				}
			}
		}
	}
}

void LimitGivenDir() {
#if 0
	if (!HandleTargetDir()) {
		QMetaObject::invokeMethod(qApp, [=]() {
			QMessageBox message_box{ QMessageBox::Warning,  "warning", "Unable to find user directory, please contact the administrator." };
			message_box.exec();
		});
	}
#endif
#if 0
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
#endif
	while (!g_stop_flag) {
		{
			std::unique_lock<std::mutex> lck{ g_mutex }; 
			g_cv.wait_for(lck, std::chrono::milliseconds(30));
			if (g_stop_flag) {
				break;
			}
		}
		
		QMetaObject::invokeMethod(qApp, [=]() {
			auto window = QApplication::activeWindow();
			if (NULL == window) {
				std::cout << "window is null" << std::endl;
				return;
			}
			std::cout << "window = " << (void*)window << ":" << window->objectName().toStdString() << std::endl;
			//if (!HandleTargetDir()) {
			//	if (g_message_box) {
			//		return;
			//	}
			//	g_message_box = new QMessageBox{ QMessageBox::Warning,  "warning", "Unable to find user directory, please contact the administrator." };
			//	g_message_box->exec();
			//	g_message_box = nullptr;
			//	window->close();
			//	return;
			//}
			if (window->objectName().contains("MayaAppHomeWindow", Qt::CaseInsensitive)) {
				HandleMayaHomeWindow(window);
			}
			else if (window->objectName().contains("AboutArnold", Qt::CaseInsensitive)) {
				window->close();
			}
			else if (window->objectName().contains("MayaWindow", Qt::CaseInsensitive)) {
				HandleMayaWindow(window);
			} else if (window->objectName().contains("QFileDialog", Qt::CaseInsensitive)) {
				HandleQFileDialog(window);
			}
			else {
				for (QObject* child : window->findChildren<QObject*>()) {
					HideHelpBtn(window, child);
					const QMetaObject* metaObject = child->metaObject();
					//std::cout << ", class name:" << metaObject->className();
					const QMetaObject* parentMetaObject = metaObject->superClass();
					if (parentMetaObject) {
						//std::cout << ",Parent class name:" << parentMetaObject->className();
					}
				}
				std::cout << "other title:" << window->windowTitle().toStdString() << std::endl;
			}
		});
	}
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		do {
			AllocConsole();
			freopen("CONOUT$", "w", stdout);
			std::cout << "This works" << std::endl;
			g_find_widget_thread = std::thread(LimitGivenDir);
		} while (false);
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		g_stop_flag = true;
		g_cv.notify_all();
		if (g_find_widget_thread.joinable()) {
			g_find_widget_thread.join();
		}
	}
		break;
	}
	return TRUE;
}
