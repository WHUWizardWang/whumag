#ifndef NAVIGATIONFORM_H
#define NAVIGATIONFORM_H

#include "navigationrunner.h"

#include <QFutureWatcher>
#include <QWidget>

#include <atomic>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QVBoxLayout;
class QWidget;
class ResultPreviewPanel;

// "匹配导航" window: pick the background field, the INS track, the true path and the method, run
// the matching in a worker thread and show / save the result.  The last result of every method is
// kept, so switching the method shows what it produced before.
class NavigationForm : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationForm(QWidget *parent = nullptr);
    ~NavigationForm() override;

    bool isRunning() const;

public slots:
    void startMatching();   // "开始匹配"
    void stopMatching();    // "停止" while running

signals:
    void finished(bool ok);   // after every run (also when it failed or was stopped)

private:
    void buildLayout();
    QLineEdit *addPathField(QVBoxLayout *layout, const QString &caption, const QString &placeholder, bool directory,
                            const QString &dialogTitle);
    bool validateInputs(Nav::Job *job);
    void onMethodChanged();
    void onRunFinished();
    void showOutcome(const Nav::Outcome &outcome);
    void appendLog(const QString &line, bool important = false);
    void setRunning(bool running);
    void loadSettings();
    void saveSettings() const;
    Nav::Method currentMethod() const;

    QLineEdit *mapFileEdit_ = nullptr;
    QLineEdit *insFileEdit_ = nullptr;
    QLineEdit *truthFileEdit_ = nullptr;
    QLineEdit *outputDirEdit_ = nullptr;
    QComboBox *methodCombo_ = nullptr;
    QLabel *methodHint_ = nullptr;
    QDoubleSpinBox *gridDxSpin_ = nullptr;
    QDoubleSpinBox *gridDySpin_ = nullptr;
    QDoubleSpinBox *searchRadiusSpin_ = nullptr;
    QWidget *searchRadiusField_ = nullptr;
    QWidget *inputPanel_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *closeButton_ = nullptr;
    QTextBrowser *log_ = nullptr;
    ResultPreviewPanel *preview_ = nullptr;
    QList<QWidget *> resultPages_;   // one per method, in Nav::Method order

    QFutureWatcher<Nav::Outcome> watcher_;
    std::atomic_bool cancel_{false};
    Nav::Method runningMethod_ = Nav::Method::Tercom;
    QString lastOutputDir_;
};

#endif // NAVIGATIONFORM_H
