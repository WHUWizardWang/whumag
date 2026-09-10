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

class ConnectDialog : public QDialog {
public:
    ConnectDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(tr("数据库连接设置"));

        // 布局和控件
        auto *form = new QFormLayout(this);
        hostEdit = new QLineEdit(this);
        portEdit = new QLineEdit(this);
        userEdit = new QLineEdit(this);
        passEdit = new QLineEdit(this);
        passEdit->setEchoMode(QLineEdit::Password);

        // 新增：记住设置复选框
        rememberBox = new QCheckBox(tr("记住设置"), this);

        form->addRow(tr("主机地址:"), hostEdit);
        form->addRow(tr("端口:"),   portEdit);
        form->addRow(tr("用户名:"), userEdit);
        form->addRow(tr("密码:"),   passEdit);
        form->addRow(QString(), rememberBox);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal, this);
        form->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // —— 启动时读取上次保存的参数 ——
        QSettings settings("YourCompany", "YourApp");
        hostEdit->setText(settings.value("db/host",     "localhost").toString());
        portEdit->setText(settings.value("db/port",     5432).toString());
        userEdit->setText(settings.value("db/user",     "postgres").toString());
        passEdit->setText(settings.value("db/password", "").toString());
        rememberBox->setChecked(settings.value("db/remember", false).toBool());
    }

    // 让 main.cpp 能获取这些值
    QString host()     const { return hostEdit->text(); }
    int     port()     const { return portEdit->text().toInt(); }
    QString username() const { return userEdit->text(); }
    QString password() const { return passEdit->text(); }
    bool    remember() const { return rememberBox->isChecked(); }

private:
    QLineEdit *hostEdit;
    QLineEdit *portEdit;
    QLineEdit *userEdit;
    QLineEdit *passEdit;
    QCheckBox *rememberBox;
};

int main(int argc, char *argv[])
{

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication a(argc, argv);

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

    ConnectDialog dlg;
    if (dlg.exec() != QDialog::Accepted) {
        return 0;  // 用户取消
    }

    // 保存或清除“记住”的配置
    {
        QSettings settings("YourCompany", "YourApp");
        if (dlg.remember()) {
            settings.setValue("db/remember", true);
            settings.setValue("db/host",     dlg.host());
            settings.setValue("db/port",     dlg.port());
            settings.setValue("db/user",     dlg.username());
            settings.setValue("db/password", dlg.password());
        } else {
            settings.setValue("db/remember", false);
            // 如果你想完全清空这些键，可以用 settings.remove(...)
        }
    }

    // 然后再初始化数据库
    bool ok = DatabaseManager::instance().initConnection(
        dlg.host(), dlg.port(), dlg.username(), dlg.password());
    if (!ok) {
        QMessageBox::critical(
            nullptr,
            QObject::tr("连接失败"),
            QObject::tr("无法连接到数据库，请检查参数后重试。"));
        return 0;
    }

#ifdef _WIN32
    allocateConsole();
#endif

        // 添加 SSL 库路径
    QDir dir(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(dir.absolutePath());

        // QQmlApplicationEngine engine;
        // engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    MainWindow w;
    w.show();
    return a.exec();

}
