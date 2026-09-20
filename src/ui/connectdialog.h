#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

class Banner;
class QAction;
class QCheckBox;
class QLineEdit;
class QPushButton;

// Start-up database login.  Unlike the old dialog it can test a connection without leaving,
// shows why a connection failed, lets the user continue offline, and never stores the password.
class ConnectDialog : public QDialog
{
    Q_OBJECT
public:
    enum { OfflineCode = 2 };   // exec() result when the user picks "离线工作"

    explicit ConnectDialog(QWidget *parent = nullptr);

public slots:
    void runTest();      // "测试连接"
    void connectNow();   // "连接"

private:
    struct Attempt {
        bool ok = false;
        QString title;
        QString detail;
    };
    Attempt tryOpen(bool keepConnection);
    void setBusy(bool busy);
    void togglePasswordVisible();

    QLineEdit *host_;
    QLineEdit *port_;
    QLineEdit *user_;
    QLineEdit *password_;
    QCheckBox *remember_;
    Banner *banner_;
    QPushButton *offlineBtn_;
    QPushButton *testBtn_;
    QPushButton *connectBtn_;
    QAction *eyeAction_;
};

#endif // CONNECTDIALOG_H
