#include "uitesthooks.h"

#ifdef WHUMAG_UI_TEST

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QTreeWidget>
#include "MagAno/anoqueryform.h"
#include "dataprocessing/continuation.h"
#include "dataprocessing/lcurveplot.h"
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include "dataquerydialog.h"
#include "logtextbrowser.h"
#include "navigation/navigationform.h"
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
    // WHUMAG_TEST_AUTO=1 runs the one-click mapping (height 0 km) into a temporary file: shows the form, fills the
    // inputs, presses "智能处理" and accepts the message boxes that appear, so the busy and finished states can be captured
    if (qEnvironmentVariableIsSet("WHUMAG_TEST_AUTO")) {
        QTimer::singleShot(1200, mainWindow, []() {
            QWidget *form = nullptr;
            for (QWidget *w : QApplication::allWidgets())
                if (w->isWindow() && QString::fromLatin1(w->metaObject()->className()) == QLatin1String("AutoReferenceMap"))
                    form = w;
            if (!form)
                return;
            form->show();
            if (auto *combo = form->findChild<QComboBox *>("comboBox_height"))
                combo->setCurrentIndex(1);
            if (auto *edit = form->findChild<QLineEdit *>("lineEdit_savePath"))
                edit->setText(QDir::temp().filePath(QStringLiteral("whumag_ui_test_auto.txt")));
            auto *closer = new QTimer(form);
            QObject::connect(closer, &QTimer::timeout, form, []() {
                for (QWidget *w : QApplication::topLevelWidgets())
                    if (auto *box = qobject_cast<QMessageBox *>(w))
                        if (box->isVisible())
                            box->accept();
            });
            closer->start(700);
            QMetaObject::invokeMethod(form, "on_pushButton_clicked", Qt::DirectConnection);
        });
    }
    // WHUMAG_TEST_NAV=<method 0..4> opens the matching-navigation window with WHUMAG_TEST_NAV_MAP / _INS / _TRUTH /
    // _OUT and starts the run (the screenshot is taken when WHUMAG_TEST_DELAY expires)
    const QString navMethod = qEnvironmentVariable("WHUMAG_TEST_NAV");
    if (!navMethod.isEmpty()) {
        QTimer::singleShot(900, mainWindow, [navMethod]() {
            for (QWidget *w : QApplication::allWidgets()) {
                auto *form = qobject_cast<NavigationForm *>(w);
                if (!form)
                    continue;
                const QPair<const char *, const char *> fields[] = {{"mapFileEdit", "WHUMAG_TEST_NAV_MAP"},
                                                                    {"insFileEdit", "WHUMAG_TEST_NAV_INS"},
                                                                    {"truthFileEdit", "WHUMAG_TEST_NAV_TRUTH"},
                                                                    {"outputDirEdit", "WHUMAG_TEST_NAV_OUT"}};
                for (const auto &f : fields)
                    if (auto *edit = form->findChild<QLineEdit *>(QLatin1String(f.first)))
                        edit->setText(qEnvironmentVariable(f.second));
                if (auto *combo = form->findChild<QComboBox *>(QStringLiteral("methodCombo")))
                    combo->setCurrentIndex(navMethod.toInt());
                // optional: _UNIT (0 km, 1 m, 2 degree), _DX, _DY, _RADIUS
                if (qEnvironmentVariableIsSet("WHUMAG_TEST_NAV_UNIT"))
                    if (auto *combo = form->findChild<QComboBox *>(QStringLiteral("unitCombo")))
                        combo->setCurrentIndex(qEnvironmentVariableIntValue("WHUMAG_TEST_NAV_UNIT"));
                const QPair<const char *, const char *> spins[] = {{"gridDxSpin", "WHUMAG_TEST_NAV_DX"},
                                                                   {"gridDySpin", "WHUMAG_TEST_NAV_DY"},
                                                                   {"searchRadiusSpin", "WHUMAG_TEST_NAV_RADIUS"}};
                for (const auto &s : spins)
                    if (qEnvironmentVariableIsSet(s.second))
                        if (auto *spin = form->findChild<QDoubleSpinBox *>(QLatin1String(s.first)))
                            spin->setValue(qEnvironmentVariable(s.second).toDouble());
                form->show();
                form->startMatching();
            }
        });
    }
    // WHUMAG_TEST_LCURVE=<x y value file> runs a downward continuation with the L-curve
    //   (WHUMAG_TEST_LCURVE_H height, _STEP grid step, _METHOD 0..3) and shows the L-curve plot
    const QString lcurveFile = qEnvironmentVariable("WHUMAG_TEST_LCURVE");
    if (!lcurveFile.isEmpty()) {
        QTimer::singleShot(700, mainWindow, [lcurveFile]() {
            Proc::ContinuationJob job;
            job.kind = Proc::ContinuationKind::Downward;
            job.inputFile = lcurveFile;
            job.outputFile = QDir::temp().filePath(QStringLiteral("whumag_lcurve_test.txt"));
            job.dx = job.dy = qEnvironmentVariable("WHUMAG_TEST_LCURVE_STEP", QStringLiteral("0.5")).toDouble();
            job.height = qEnvironmentVariable("WHUMAG_TEST_LCURVE_H", QStringLiteral("1")).toDouble();
            job.downward.method = Proc::DownwardMethod(qEnvironmentVariableIntValue("WHUMAG_TEST_LCURVE_METHOD"));
            const Proc::ContinuationOutcome out = Proc::runContinuation(job);
            auto *tabs = new QTabWidget;
            tabs->setAttribute(Qt::WA_DeleteOnClose);
            auto *page = new QWidget(tabs);
            auto *layout = new QVBoxLayout(page);
            layout->addWidget(createLCurvePlot(out.lcurve, page));
            tabs->addTab(page, QStringLiteral("L 曲线"));
            tabs->setWindowTitle(QStringLiteral("向下延拓"));
            tabs->resize(900, 620);
            tabs->show();
        });
    }
    // WHUMAG_TEST_MERGE=file1|sigma1,file2|sigma2 fills the file table of the data-fusion form
    const QStringList mergeRows = qEnvironmentVariable("WHUMAG_TEST_MERGE").split(',', Qt::SkipEmptyParts);
    if (!mergeRows.isEmpty()) {
        QTimer::singleShot(800, mainWindow, [mergeRows]() {
            for (QWidget *w : QApplication::allWidgets()) {
                if (!w->isWindow() || QString::fromLatin1(w->metaObject()->className()) != QLatin1String("mergeForm"))
                    continue;
                auto *table = w->findChild<QTableView *>();
                auto *model = table ? qobject_cast<QStandardItemModel *>(table->model()) : nullptr;
                if (!model)
                    continue;
                model->setRowCount(mergeRows.size());
                for (int i = 0; i < mergeRows.size(); ++i) {
                    const QStringList parts = mergeRows[i].split('|');
                    model->setItem(i, 0, new QStandardItem(parts.value(0)));
                    model->setItem(i, 1, new QStandardItem(parts.value(1)));
                }
            }
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
