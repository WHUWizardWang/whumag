#include "mainwindow.h"
#include <QLoggingCategory>
#include <QDialog>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QQmlApplicationEngine>
#include <QApplication>
#include <QtPlugin>
#include "contourplotter.h"
#include "connectdialog.h"
#include "thememanager.h"
#include "uitesthooks.h"
#include <QTimer>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <windows.h>
#endif

// 在main函数开始或MapForm构造函数中添加
#ifdef _WIN32
void allocateConsole() {
    // 分配控制台
    if (AllocConsole()) {
        // 重定向stdout, stdin, stderr到控制台
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

        // 使cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
        std::ios::sync_with_stdio(true);

        // 设置控制台标题
        SetConsoleTitle(L"Debug Console");

        std::cout << "Debug console allocated successfully!" << std::endl;
    }
}
#endif

int main(int argc, char *argv[])
{

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication a(argc, argv);
    const QStringList args = QCoreApplication::arguments();
    ThemeManager::instance().init();
    // --theme=light|dark|system overrides the saved theme for this run only
    for (const QString &arg : args) {
        if (arg == QLatin1String("--theme=dark"))
            ThemeManager::instance().setMode(ThemeManager::Mode::Dark, false);
        else if (arg == QLatin1String("--theme=light"))
            ThemeManager::instance().setMode(ThemeManager::Mode::Light, false);
        else if (arg == QLatin1String("--theme=system"))
            ThemeManager::instance().setMode(ThemeManager::Mode::System, false);
    }
    // --offline starts without a database (same as choosing "离线工作" in the login dialog)
    bool offline = args.contains(QStringLiteral("--offline"));
    DatabaseManager::instance().setOffline(offline);

    QOpenGLContext context;
    context.create();

    if (context.isValid()) {
        auto format = context.format();
        if (format.majorVersion() >= 2) {
            qDebug() << "✅ 使用硬件OpenGL渲染";
            // 不设置软件渲染标志
        } else {
            qDebug() << "⚠️ OpenGL版本过低，使用软件渲染";
            QGuiApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        }
    } else {
        qDebug() << "❌ 无OpenGL支持，强制软件渲染";
        QGuiApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }

    if (!offline) {
        ConnectDialog dlg;
#ifdef WHUMAG_UI_TEST
        if (!qEnvironmentVariable("WHUMAG_TEST_GRAB").isEmpty()) {
            if (qEnvironmentVariableIsSet("WHUMAG_TEST_CONNECT_PROBE"))
                QTimer::singleShot(400, &dlg, [&dlg]() { dlg.runTest(); });
            QTimer::singleShot(3500, &dlg, [&dlg]() { uiTestGrab(&dlg, "connect_dialog"); dlg.reject(); });
        }
#endif
        const int result = dlg.exec();
        if (result == ConnectDialog::OfflineCode) {
            offline = true;
            DatabaseManager::instance().setOffline(true);
        } else if (result != QDialog::Accepted) {
            return 0;  // 用户取消
        }
    }

#ifdef _WIN32
    if (args.contains(QStringLiteral("--console")))
        allocateConsole();   // debug console window, only when asked for
#endif

        // 添加 SSL 库路径
    QDir dir(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(dir.absolutePath());

        // QQmlApplicationEngine engine;
        // engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    MainWindow w;
    w.show();
#ifdef WHUMAG_UI_TEST
    uiTestScheduleForMainWindow(&w);
#endif
    return a.exec();

}
