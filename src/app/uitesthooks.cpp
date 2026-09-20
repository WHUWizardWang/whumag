#include "uitesthooks.h"

#ifdef WHUMAG_UI_TEST

#include <QApplication>
#include <QRadioButton>
#include <QTreeWidget>
#include "MagAno/anoqueryform.h"
#include "dataquerydialog.h"
#include "logtextbrowser.h"
#include "tasklistwidget.h"
#include "wmm/queryform.h"
#include <QDir>
#include <QMetaObject>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

bool uiTestGrab(QWidget *w, const QString &name)
{
    const QString dir = qEnvironmentVariable("WHUMAG_TEST_GRAB");
    if (dir.isEmpty() || !w)
        return false;
    QDir().mkpath(dir);
    return w->grab().save(QDir(dir).filePath(name + ".png"));
}

void uiTestScheduleForMainWindow(QWidget *mainWindow)
{
    const QString grabDir = qEnvironmentVariable("WHUMAG_TEST_GRAB");
    const QStringList opens = qEnvironmentVariable("WHUMAG_TEST_OPEN").split(',', Qt::SkipEmptyParts);
    const int delay = qEnvironmentVariableIntValue("WHUMAG_TEST_DELAY") > 0 ? qEnvironmentVariableIntValue("WHUMAG_TEST_DELAY") : 2500;
    if (grabDir.isEmpty() && opens.isEmpty())
        return;

    QTimer::singleShot(600, mainWindow, [mainWindow, opens]() {
        for (const QString &slot : opens)
            QMetaObject::invokeMethod(mainWindow, slot.trimmed().toUtf8().constData(), Qt::DirectConnection);
    });
    // WHUMAG_TEST_PROJECT=<file.proj> opens a project; WHUMAG_TEST_SELECT=<tree text> selects a tree item;
    // WHUMAG_TEST_TASKS=1 adds sample tasks and log lines
    const QString proj = qEnvironmentVariable("WHUMAG_TEST_PROJECT");
    if (!proj.isEmpty()) {
        QTimer::singleShot(200, mainWindow, [mainWindow, proj]() {
            QMetaObject::invokeMethod(mainWindow, "openProjectPath", Qt::DirectConnection, Q_ARG(QString, QDir::fromNativeSeparators(proj)));
        });
        const QString select = qEnvironmentVariable("WHUMAG_TEST_SELECT");
        if (!select.isEmpty()) {
            QTimer::singleShot(500, mainWindow, [mainWindow, select]() {
                if (auto *tree = mainWindow->findChild<QTreeWidget *>()) {
                    const auto hits = tree->findItems(select, Qt::MatchExactly | Qt::MatchRecursive, 0);
                    if (!hits.isEmpty())
                        tree->setCurrentItem(hits.first());
                }
            });
        }
    }
    if (qEnvironmentVariableIsSet("WHUMAG_TEST_TASKS")) {
        QTimer::singleShot(400, mainWindow, [mainWindow]() {
            if (auto *list = mainWindow->findChild<TaskListWidget *>()) {
                list->addItem(QStringLiteral("精度评估: 水下检核线 line95 （进行中…）"));
                list->addItem(QStringLiteral("整图建模: ronghe.txt （已完成）"));
                list->addItem(QStringLiteral("向上延拓: line99.txt （处理失败）"));
            }
            if (auto *log = mainWindow->findChild<LogTextBrowser *>()) {
                log->append(QStringLiteral("读入 ronghe.txt（12,830 点）"));
                log->append(QStringLiteral("抽稀 2.00 km → 3,207 点"));
                log->append(QStringLiteral("警告：第 17 代适应度停滞"));
                log->append(QStringLiteral("错误: 输入数据为空（line99.txt）"));
                log->append(QStringLiteral("建模完成，RMS = 1.82 nT"));
            }
        });
    }
    // WHUMAG_TEST_QUERY=0|1 opens the data query dialog on the model / anomaly page.
    //   WHUMAG_TEST_QUERY_MODE=<radio object name> picks the query mode (e.g. radioButton_grid),
    //   WHUMAG_TEST_QUERY_RUN=1 presses "查询", WHUMAG_TEST_QUERY_DEMO=1 fills the anomaly page with a synthetic result
    const QString queryPage = qEnvironmentVariable("WHUMAG_TEST_QUERY");
    if (!queryPage.isEmpty()) {
        QTimer::singleShot(900, mainWindow, [mainWindow, queryPage]() {
            auto *dlg = mainWindow->findChild<DataQueryDialog *>();
            if (!dlg)
                return;
            const bool anomaly = queryPage.toInt() == 1;
            dlg->showPage(anomaly ? DataQueryDialog::Anomaly : DataQueryDialog::GlobalModel);
            QWidget *form = anomaly ? static_cast<QWidget *>(dlg->findChild<AnoQueryForm *>()) : static_cast<QWidget *>(dlg->findChild<QueryForm *>());
            if (!form)
                return;
            const QString mode = qEnvironmentVariable("WHUMAG_TEST_QUERY_MODE");
            if (!mode.isEmpty())
                if (auto *radio = form->findChild<QRadioButton *>(mode))
                    radio->setChecked(true);
            if (qEnvironmentVariableIsSet("WHUMAG_TEST_QUERY_RUN"))
                QMetaObject::invokeMethod(form, "on_pushButton_3_clicked", Qt::DirectConnection);
            if (anomaly && qEnvironmentVariableIsSet("WHUMAG_TEST_QUERY_DEMO"))
                QMetaObject::invokeMethod(form, "testFillSynthetic", Qt::DirectConnection);
        });
    }
    // WHUMAG_TEST_SHOW=ClassA,ClassB shows every top-level widget of those classes (forms owned by the main window)
    const QStringList shows = qEnvironmentVariable("WHUMAG_TEST_SHOW").split(',', Qt::SkipEmptyParts);
    if (!shows.isEmpty()) {
        QTimer::singleShot(900, mainWindow, [shows]() {
            for (QWidget *w : QApplication::allWidgets())
                if (w->isWindow() && shows.contains(QString::fromLatin1(w->metaObject()->className())))
                    w->show();
        });
    }
    QTimer::singleShot(delay, mainWindow, [grabDir]() {
        if (grabDir.isEmpty())
            return;
        int n = 0;
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (!w->isVisible())
                continue;
            const QString name = QStringLiteral("%1_%2").arg(n++).arg(QString::fromLatin1(w->metaObject()->className()));
            uiTestGrab(w, name);
        }
        QApplication::exit(0);
    });
}

#endif // WHUMAG_UI_TEST
