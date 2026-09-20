#include "connectdialog.h"

#include "database/databasemanager.h"
#include "thememanager.h"
#include "uiicons.h"
#include "uiwidgets.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QVariant>

namespace {
const char *kOrg = "YourCompany";
const char *kApp = "YourApp";

QLabel *fieldLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setProperty("role", QStringLiteral("section"));
    return l;
}
} // namespace

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("连接数据库"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedWidth(520);

    QSettings settings(kOrg, kApp);
    settings.remove("db/password");   // older versions stored the password in clear text

    auto *title = new QLabel(tr("连接数据库"));
    title->setProperty("role", QStringLiteral("h1"));
    auto *subtitle = new QLabel(tr("全球磁异常网格与数据索引存放在 PostgreSQL"));
    subtitle->setProperty("role", QStringLiteral("hint"));
    auto *titleBox = new QVBoxLayout;
    titleBox->setSpacing(2);
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    auto *header = new QHBoxLayout;
    header->setSpacing(12);
    header->addWidget(new IconBadge("database", 44, 24), 0, Qt::AlignTop);
    header->addLayout(titleBox, 1);

    host_ = new QLineEdit(settings.value("db/host", "localhost").toString());
    port_ = new QLineEdit(settings.value("db/port", 5432).toString());
    port_->setValidator(new QIntValidator(1, 65535, port_));
    port_->setFixedWidth(110);
    user_ = new QLineEdit(settings.value("db/user", "postgres").toString());
    password_ = new QLineEdit;
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText(tr("数据库密码"));
    eyeAction_ = password_->addAction(UiIcons::icon("eye", 16), QLineEdit::TrailingPosition);
    connect(eyeAction_, &QAction::triggered, this, &ConnectDialog::togglePasswordVisible);

    auto *hostCol = new QVBoxLayout;
    hostCol->setSpacing(6);
    hostCol->addWidget(fieldLabel(tr("主机地址")));
    hostCol->addWidget(host_);
    auto *portCol = new QVBoxLayout;
    portCol->setSpacing(6);
    portCol->addWidget(fieldLabel(tr("端口")));
    portCol->addWidget(port_);
    auto *hostRow = new QHBoxLayout;
    hostRow->setSpacing(12);
    hostRow->addLayout(hostCol, 1);
    hostRow->addLayout(portCol);

    remember_ = new QCheckBox(tr("记住主机地址与用户名"));
    remember_->setChecked(settings.value("db/remember", true).toBool());
    auto *rememberHint = new QLabel(tr("出于安全考虑，密码不会保存在本机；下次启动需要重新输入。"));
    rememberHint->setProperty("role", QStringLiteral("hint"));
    rememberHint->setWordWrap(true);
    rememberHint->setContentsMargins(24, 0, 0, 0);

    banner_ = new Banner;
    banner_->hide();

    offlineBtn_ = new QPushButton(tr("离线工作"));
    offlineBtn_->setProperty("role", QStringLiteral("ghost"));
    offlineBtn_->setToolTip(tr("不连接数据库：全球磁异常查询和数据库管理将不可用，其余功能照常使用"));
    testBtn_ = new QPushButton(tr("测试连接"));
    testBtn_->setIcon(UiIcons::icon("plug", 16));
    connectBtn_ = new QPushButton(tr("连接"));
    connectBtn_->setProperty("role", QStringLiteral("primary"));
    connectBtn_->setDefault(true);
    connectBtn_->setMinimumWidth(96);
    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addWidget(offlineBtn_);
    footer->addStretch(1);
    footer->addWidget(testBtn_);
    footer->addWidget(connectBtn_);

    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(14);
    root->addLayout(header);
    root->addSpacing(4);
    root->addLayout(hostRow);
    auto *userCol = new QVBoxLayout;
    userCol->setSpacing(6);
    userCol->addWidget(fieldLabel(tr("用户名")));
    userCol->addWidget(user_);
    root->addLayout(userCol);
    auto *passCol = new QVBoxLayout;
    passCol->setSpacing(6);
    passCol->addWidget(fieldLabel(tr("密码")));
    passCol->addWidget(password_);
    root->addLayout(passCol);
    root->addWidget(remember_);
    root->addWidget(rememberHint);
    root->addWidget(banner_);
    root->addSpacing(2);
    root->addWidget(line);
    root->addLayout(footer);

    connect(testBtn_, &QPushButton::clicked, this, &ConnectDialog::runTest);
    connect(connectBtn_, &QPushButton::clicked, this, &ConnectDialog::connectNow);
    connect(offlineBtn_, &QPushButton::clicked, this, [this]() { done(OfflineCode); });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        eyeAction_->setIcon(UiIcons::icon(password_->echoMode() == QLineEdit::Password ? "eye" : "eye-off", 16));
        testBtn_->setIcon(UiIcons::icon("plug", 16));
    });

    password_->setFocus();
}

void ConnectDialog::togglePasswordVisible()
{
    const bool hidden = password_->echoMode() == QLineEdit::Password;
    password_->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
    eyeAction_->setIcon(UiIcons::icon(hidden ? "eye-off" : "eye", 16));
}

void ConnectDialog::setBusy(bool busy)
{
    for (QWidget *w : {static_cast<QWidget *>(testBtn_), static_cast<QWidget *>(connectBtn_), static_cast<QWidget *>(offlineBtn_),
                       static_cast<QWidget *>(host_), static_cast<QWidget *>(port_), static_cast<QWidget *>(user_),
                       static_cast<QWidget *>(password_)})
        w->setEnabled(!busy);
    if (busy)
        QApplication::setOverrideCursor(Qt::WaitCursor);
    else
        QApplication::restoreOverrideCursor();
}

ConnectDialog::Attempt ConnectDialog::tryOpen(bool keepConnection)
{
    Attempt result;
    const QString hostName = host_->text().trimmed();
    const int portNumber = port_->text().toInt();
    if (hostName.isEmpty() || portNumber <= 0) {
        result.title = tr("请填写主机地址和端口");
        return result;
    }

    QElapsedTimer timer;
    timer.start();
    if (keepConnection) {
        result.ok = DatabaseManager::instance().initConnection(hostName, portNumber, user_->text().trimmed(), password_->text());
        if (!result.ok)
            result.detail = DatabaseManager::instance().getDatabase().lastError().text();
        result.title = tr("延迟 %1 ms").arg(timer.elapsed());
    } else {
        const QString name = QStringLiteral("whumag_connect_test");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", name);
            db.setHostName(hostName);
            db.setPort(portNumber);
            db.setDatabaseName("whumag");
            db.setUserName(user_->text().trimmed());
            db.setPassword(password_->text());
            db.setConnectOptions("connect_timeout=5");
            result.ok = db.open();
            const qint64 ms = timer.elapsed();
            if (result.ok) {
                QSqlQuery q(db);
                QString version;
                if (q.exec("select version()") && q.next())
                    version = q.value(0).toString().section(' ', 0, 1);
                result.title = tr("%1 · 数据库 whumag · 延迟 %2 ms").arg(version.isEmpty() ? QStringLiteral("PostgreSQL") : version).arg(ms);
            } else {
                result.detail = db.lastError().text();
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(name);
    }
    return result;
}

static void explainFailure(const QString &raw, QString *title, QString *body)
{
    const QString lower = raw.toLower();
    if (lower.contains("password authentication failed") || lower.contains("28p01") || raw.contains(QStringLiteral("密码"))) {
        *title = QObject::tr("用户名或密码不正确");
        *body = QObject::tr("请核对数据库用户名和密码后重试。");
    } else if (lower.contains("could not connect") || lower.contains("connection refused") || lower.contains("timeout")
               || lower.contains("timed out") || lower.contains("10061") || lower.contains("no route")) {
        *title = QObject::tr("无法连接到数据库服务");
        *body = QObject::tr("请确认 PostgreSQL 已启动，主机地址和端口无误，防火墙允许访问。");
    } else if (lower.contains("driver not loaded") || lower.contains("qpsql")) {
        *title = QObject::tr("缺少 PostgreSQL 驱动");
        *body = QObject::tr("请确认 libpq.dll 与程序放在同一目录。");
    } else {
        *title = QObject::tr("连接失败");
        *body = raw.section('\n', 0, 1).trimmed();
    }
}

void ConnectDialog::runTest()
{
    banner_->setContent(Banner::Info, tr("正在连接…"));
    banner_->show();
    setBusy(true);
    QApplication::processEvents();
    const Attempt a = tryOpen(false);
    setBusy(false);
    if (a.ok) {
        banner_->setContent(Banner::Ok, tr("连接成功"), a.title);
    } else if (a.detail.isEmpty()) {
        banner_->setContent(Banner::Warn, a.title);
    } else {
        QString title, body;
        explainFailure(a.detail, &title, &body);
        banner_->setContent(Banner::Error, title, body);
    }
}

void ConnectDialog::connectNow()
{
    banner_->setContent(Banner::Info, tr("正在连接…"));
    banner_->show();
    setBusy(true);
    QApplication::processEvents();
    const Attempt a = tryOpen(true);
    setBusy(false);
    if (!a.ok) {
        if (a.detail.isEmpty()) {
            banner_->setContent(Banner::Warn, a.title);
        } else {
            QString title, body;
            explainFailure(a.detail, &title, &body);
            banner_->setContent(Banner::Error, title, body);
        }
        return;
    }

    QSettings settings(kOrg, kApp);
    settings.setValue("db/remember", remember_->isChecked());
    if (remember_->isChecked()) {
        settings.setValue("db/host", host_->text().trimmed());
        settings.setValue("db/port", port_->text().toInt());
        settings.setValue("db/user", user_->text().trimmed());
    } else {
        settings.remove("db/host");
        settings.remove("db/port");
        settings.remove("db/user");
    }
    accept();
}
