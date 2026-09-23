#include "navigationform.h"

#include "formkit.h"
#include "navplot.h"
#include "resultpreviewpanel.h"
#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include "qcustomplot.h"

#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTextBrowser>
#include <QTime>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {

constexpr int kMethodCount = 5;

QSettings appSettings()
{
    return QSettings(QStringLiteral("YourCompany"), QStringLiteral("YourApp"));
}

bool usesTercom(Nav::Method m)
{
    return m != Nav::Method::Iccp;
}

void clearLayout(QLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

} // namespace

NavigationForm::NavigationForm(QWidget *parent) : QWidget(parent)
{
    buildLayout();
    loadSettings();
    onMethodChanged();
    connect(&watcher_, &QFutureWatcher<Nav::Outcome>::finished, this, &NavigationForm::onRunFinished);
}

NavigationForm::~NavigationForm()
{
    if (isRunning()) {
        cancel_ = true;
        watcher_.waitForFinished();   // the worker reads this form's cancel flag
    }
}

bool NavigationForm::isRunning() const
{
    return watcher_.isRunning();
}

// ---------------------------------------------------------------- layout
QLineEdit *NavigationForm::addPathField(QVBoxLayout *layout, const QString &caption, const QString &hint, bool directory,
                                        const QString &dialogTitle)
{
    auto *edit = new QLineEdit;
    edit->setClearButtonEnabled(true);
    auto *browse = new QPushButton(tr("选择…"));
    connect(browse, &QPushButton::clicked, this, [this, edit, directory, dialogTitle]() {
        QString start = edit->text().trimmed();
        if (start.isEmpty())
            start = appSettings().value(QStringLiteral("navigation/lastDir"), QDir::homePath()).toString();
        const QString chosen = directory
            ? QFileDialog::getExistingDirectory(this, dialogTitle, start)
            : QFileDialog::getOpenFileName(this, dialogTitle, start,
                                           tr("数据文件 (*.txt *.csv *.dat *.xyz);;所有文件 (*)"));
        if (chosen.isEmpty())
            return;   // cancelled: keep what was there
        edit->setText(QDir::toNativeSeparators(chosen));
        appSettings().setValue(QStringLiteral("navigation/lastDir"), directory ? chosen : QFileInfo(chosen).absolutePath());
    });
    auto *row = new QHBoxLayout;
    row->setSpacing(8);
    row->addWidget(edit, 1);
    row->addWidget(browse);
    layout->addLayout(FormKit::field(caption, row, hint));
    return edit;
}

void NavigationForm::buildLayout()
{
    setWindowTitle(tr("匹配导航"));
    resize(UiScale::windowSize(1180, 720));

    // ---- left: inputs (scrollable)
    inputPanel_ = new QWidget;
    auto *ll = new QVBoxLayout(inputPanel_);
    ll->setContentsMargins(24, 20, 24, 12);
    ll->setSpacing(14);
    ll->addWidget(FormKit::header(QStringLiteral("route"), tr("匹配导航"), tr("用背景场修正 INS 航迹，并评估定位精度")));

    ll->addWidget(FormKit::stepHeading(1, tr("输入数据")));
    mapFileEdit_ = addPathField(ll, tr("背景场"), tr("每行：x  y  磁场值"), false, tr("选择背景场文件"));
    insFileEdit_ = addPathField(ll, tr("INS 航迹"), tr("每行：x  y  实测磁场值"), false, tr("选择 INS 航迹文件"));
    truthFileEdit_ = addPathField(ll, tr("真实航迹（可选）"), tr("每行：x  y，与 INS 航迹逐点对应，用于计算定位精度"), false,
                                  tr("选择真实航迹文件"));
    mapFileEdit_->setObjectName(QStringLiteral("mapFileEdit"));
    insFileEdit_->setObjectName(QStringLiteral("insFileEdit"));
    truthFileEdit_->setObjectName(QStringLiteral("truthFileEdit"));

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(2, tr("匹配设置")));
    methodCombo_ = new QComboBox;
    methodCombo_->setObjectName(QStringLiteral("methodCombo"));
    for (int i = 0; i < kMethodCount; ++i)
        methodCombo_->addItem(Nav::methodTitle(Nav::Method(i)));
    connect(methodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NavigationForm::onMethodChanged);
    auto *methodBlock = FormKit::field(tr("匹配算法"), methodCombo_);
    methodHint_ = FormKit::hint(QString());
    methodBlock->addWidget(methodHint_);
    ll->addLayout(methodBlock);

    auto makeSpin = [](double value, double minimum, double maximum, int decimals) {
        auto *spin = new QDoubleSpinBox;
        spin->setDecimals(decimals);
        spin->setRange(minimum, maximum);
        spin->setValue(value);
        return spin;
    };
    unitCombo_ = new QComboBox;
    unitCombo_->setObjectName(QStringLiteral("unitCombo"));
    for (Nav::CoordinateUnit u : {Nav::CoordinateUnit::Kilometre, Nav::CoordinateUnit::Metre, Nav::CoordinateUnit::Degree})
        unitCombo_->addItem(Nav::unitName(u));
    connect(unitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NavigationForm::onUnitChanged);
    ll->addLayout(FormKit::field(tr("坐标单位"), unitCombo_, tr("所有输入文件中 x、y 的单位，分辨率、搜索半径和误差都按这个单位")));

    gridDxSpin_ = makeSpin(0.5, 1e-6, 1e6, 6);
    gridDySpin_ = makeSpin(0.5, 1e-6, 1e6, 6);
    gridDxSpin_->setObjectName(QStringLiteral("gridDxSpin"));
    gridDySpin_->setObjectName(QStringLiteral("gridDySpin"));
    auto *res = new QHBoxLayout;
    res->setSpacing(12);
    res->addLayout(FormKit::field(tr("背景场分辨率 · X"), gridDxSpin_), 1);
    res->addLayout(FormKit::field(tr("背景场分辨率 · Y"), gridDySpin_), 1);
    ll->addLayout(res);

    searchRadiusSpin_ = makeSpin(9.0, 1e-6, 1e6, 6);
    searchRadiusSpin_->setObjectName(QStringLiteral("searchRadiusSpin"));
    searchRadiusField_ = new QWidget;
    auto *radiusLayout = FormKit::field(tr("TERCOM 搜索半径"), searchRadiusSpin_,
                                        tr("在 INS 起点周围多大范围内搜索真实起点，单位与坐标相同。INS 初始误差越大，需要的半径越大。"));
    searchRadiusField_->setLayout(radiusLayout);
    ll->addWidget(searchRadiusField_);

    ll->addSpacing(4);
    ll->addWidget(FormKit::stepHeading(3, tr("输出")));
    outputDirEdit_ = addPathField(ll, tr("输出文件夹"), tr("匹配航迹、精度统计和结果图保存在这里；不存在时自动创建"), true,
                                  tr("选择输出文件夹"));
    outputDirEdit_->setObjectName(QStringLiteral("outputDirEdit"));
    ll->addStretch(1);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(inputPanel_);
    scroll->viewport()->setAutoFillBackground(false);
    inputPanel_->setAutoFillBackground(false);

    auto *leftPanel = new QWidget;
    leftPanel->setObjectName(QStringLiteral("navLeft"));
    leftPanel->setFixedWidth(UiScale::dp(440));
    leftPanel->setAttribute(Qt::WA_StyledBackground, true);
    auto *lp = new QVBoxLayout(leftPanel);
    lp->setContentsMargins(0, 0, 0, 0);
    lp->setSpacing(0);
    lp->addWidget(scroll, 1);

    runButton_ = new QPushButton(tr("开始匹配"));
    runButton_->setObjectName(QStringLiteral("runButton"));
    FormKit::setRole(runButton_, "primary");
    runButton_->setMinimumHeight(36);
    runButton_->setCursor(Qt::PointingHandCursor);
    runButton_->setDefault(true);
    connect(runButton_, &QPushButton::clicked, this, &NavigationForm::startMatching);
    closeButton_ = new QPushButton(tr("关闭"));
    closeButton_->setMinimumHeight(36);
    connect(closeButton_, &QPushButton::clicked, this, [this]() {
        if (isRunning())
            stopMatching();
        else
            close();
    });
    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(24, 10, 24, 18);
    footer->setSpacing(8);
    footer->addWidget(runButton_, 1);
    footer->addWidget(closeButton_);
    lp->addLayout(footer);

    // ---- right: one result page per method, status and log
    for (int i = 0; i < kMethodCount; ++i) {
        auto *page = new QWidget;
        auto *pl = new QVBoxLayout(page);
        pl->setContentsMargins(0, 0, 0, 0);
        pl->setSpacing(6);
        resultPages_.append(page);
    }
    log_ = new QTextBrowser;
    log_->setOpenExternalLinks(false);
    preview_ = new ResultPreviewPanel(resultPages_, log_);
    preview_->setEmptyText(tr("尚无结果"), tr("选择数据与算法后点击“开始匹配”，背景场、真实航迹和匹配航迹会显示在这里"));

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(leftPanel);
    root->addWidget(preview_, 1);

    auto restyle = [leftPanel]() {
        const ThemeManager &tm = ThemeManager::instance();
        leftPanel->setStyleSheet(QStringLiteral("#navLeft { background: %1; border: none; border-right: 1px solid %2; }"
                                                "#navLeft QScrollArea { background: transparent; }")
                                     .arg(tm.hex("n1"), tm.hex("line")));
    };
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, restyle);
    restyle();
}

// ---------------------------------------------------------------- settings
void NavigationForm::loadSettings()
{
    QSettings s = appSettings();
    s.beginGroup(QStringLiteral("navigation"));
    mapFileEdit_->setText(s.value(QStringLiteral("mapFile")).toString());
    insFileEdit_->setText(s.value(QStringLiteral("insFile")).toString());
    truthFileEdit_->setText(s.value(QStringLiteral("truthFile")).toString());
    outputDirEdit_->setText(s.value(QStringLiteral("outputDir")).toString());
    gridDxSpin_->setValue(s.value(QStringLiteral("gridDx"), 0.5).toDouble());
    gridDySpin_->setValue(s.value(QStringLiteral("gridDy"), 0.5).toDouble());
    searchRadiusSpin_->setValue(s.value(QStringLiteral("searchRadius"), 9.0).toDouble());
    methodCombo_->setCurrentIndex(qBound(0, s.value(QStringLiteral("method"), 0).toInt(), kMethodCount - 1));
    unitCombo_->setCurrentIndex(qBound(0, s.value(QStringLiteral("unit"), 0).toInt(), 2));
    s.endGroup();
    onUnitChanged();
}

void NavigationForm::saveSettings() const
{
    QSettings s = appSettings();
    s.beginGroup(QStringLiteral("navigation"));
    s.setValue(QStringLiteral("mapFile"), mapFileEdit_->text().trimmed());
    s.setValue(QStringLiteral("insFile"), insFileEdit_->text().trimmed());
    s.setValue(QStringLiteral("truthFile"), truthFileEdit_->text().trimmed());
    s.setValue(QStringLiteral("outputDir"), outputDirEdit_->text().trimmed());
    s.setValue(QStringLiteral("gridDx"), gridDxSpin_->value());
    s.setValue(QStringLiteral("gridDy"), gridDySpin_->value());
    s.setValue(QStringLiteral("searchRadius"), searchRadiusSpin_->value());
    s.setValue(QStringLiteral("method"), methodCombo_->currentIndex());
    s.setValue(QStringLiteral("unit"), unitCombo_->currentIndex());
    s.endGroup();
}

// ---------------------------------------------------------------- running
Nav::Method NavigationForm::currentMethod() const
{
    return Nav::Method(qBound(0, methodCombo_->currentIndex(), kMethodCount - 1));
}

void NavigationForm::onMethodChanged()
{
    const Nav::Method m = currentMethod();
    methodHint_->setText(Nav::methodDescription(m));
    searchRadiusField_->setVisible(usesTercom(m));
    for (int i = 0; i < resultPages_.size(); ++i)
        resultPages_[i]->setVisible(i == int(m));
    preview_->refreshEmptyState();
}

Nav::CoordinateUnit NavigationForm::currentUnit() const
{
    return Nav::CoordinateUnit(qBound(0, unitCombo_->currentIndex(), 2));
}

void NavigationForm::onUnitChanged()
{
    const QString suffix = QStringLiteral(" ") + Nav::unitSymbol(currentUnit());
    for (QDoubleSpinBox *spin : {gridDxSpin_, gridDySpin_, searchRadiusSpin_})
        spin->setSuffix(currentUnit() == Nav::CoordinateUnit::Degree ? QStringLiteral("°") : suffix);
}

bool NavigationForm::validateInputs(Nav::Job *job)
{
    auto requireFile = [this](QLineEdit *edit, const QString &what, bool optional) {
        const QString path = edit->text().trimmed();
        if (path.isEmpty()) {
            if (optional)
                return true;
            QMessageBox::warning(this, tr("匹配导航"), tr("请选择%1文件。").arg(what));
            edit->setFocus();
            return false;
        }
        if (!QFileInfo(path).isFile()) {
            QMessageBox::warning(this, tr("匹配导航"), tr("%1文件不存在：\n%2").arg(what, path));
            edit->setFocus();
            return false;
        }
        return true;
    };
    if (!requireFile(mapFileEdit_, tr("背景场"), false) || !requireFile(insFileEdit_, tr("INS 航迹"), false)
        || !requireFile(truthFileEdit_, tr("真实航迹"), true))
        return false;

    if (outputDirEdit_->text().trimmed().isEmpty()) {
        // default: next to the INS track
        const QString dir = QFileInfo(insFileEdit_->text().trimmed()).absoluteDir().filePath(QStringLiteral("navigation_results"));
        outputDirEdit_->setText(QDir::toNativeSeparators(dir));
        appendLog(tr("未指定输出文件夹，使用 %1").arg(QDir::toNativeSeparators(dir)));
    }

    job->method = currentMethod();
    job->mapFile = QDir::fromNativeSeparators(mapFileEdit_->text().trimmed());
    job->insFile = QDir::fromNativeSeparators(insFileEdit_->text().trimmed());
    job->truthFile = QDir::fromNativeSeparators(truthFileEdit_->text().trimmed());
    job->outputDir = QDir::fromNativeSeparators(outputDirEdit_->text().trimmed());
    job->unit = currentUnit();
    job->dx = gridDxSpin_->value();
    job->dy = gridDySpin_->value();
    job->searchRadius = searchRadiusSpin_->value();
    return true;
}

void NavigationForm::startMatching()
{
    if (isRunning())
        return;
    Nav::Job job;
    if (!validateInputs(&job))
        return;
    saveSettings();

    runningMethod_ = job.method;
    lastOutputDir_ = job.outputDir;   // the edit may change while the job runs
    cancel_ = false;
    appendLog(tr("%1 开始").arg(Nav::methodTitle(job.method)), true);
    setRunning(true);

    // log lines come from the worker thread; hand them to the GUI thread
    const Nav::ProgressFn progress = [this](const QString &line) {
        QMetaObject::invokeMethod(this, [this, line]() { appendLog(line); }, Qt::QueuedConnection);
    };
    watcher_.setFuture(QtConcurrent::run([job, progress, this]() { return Nav::runJob(job, &cancel_, progress); }));
}

void NavigationForm::stopMatching()
{
    if (!isRunning())
        return;
    cancel_ = true;
    closeButton_->setEnabled(false);
    appendLog(tr("正在停止…"));
}

void NavigationForm::setRunning(bool running)
{
    inputPanel_->setEnabled(!running);
    runButton_->setEnabled(!running);
    runButton_->setText(running ? tr("计算中…") : tr("开始匹配"));
    closeButton_->setEnabled(true);
    closeButton_->setText(running ? tr("停止") : tr("关闭"));
    if (running)
        preview_->setStatus(tr("计算中…"), Chip::Accent);
}

void NavigationForm::onRunFinished()
{
    const Nav::Outcome outcome = watcher_.result();
    setRunning(false);
    showOutcome(outcome);
    emit finished(outcome.ok());
}

void NavigationForm::showOutcome(const Nav::Outcome &outcome)
{
    if (outcome.cancelled) {
        preview_->setStatus(tr("已停止"), Chip::Neutral);
        appendLog(tr("已停止"), true);
        return;
    }
    if (!outcome.error.isEmpty()) {
        preview_->setStatus(tr("失败"), Chip::Error);
        appendLog(tr("错误：%1").arg(outcome.error), true);
        return;
    }

    // replace the page of this method with the new chart
    QWidget *page = resultPages_.value(int(runningMethod_));
    if (!page)
        return;
    clearLayout(page->layout());

    QStringList parts;
    if (!outcome.truth.isEmpty()) {
        parts << tr("INS %1").arg(Nav::formatError(outcome.insRms, outcome.insRmsMetres, outcome.unit));
        for (const Nav::MatchedTrack &t : outcome.tracks)
            parts << QStringLiteral("%1 %2").arg(t.label, Nav::formatError(t.rms, t.rmsMetres, outcome.unit));
    }
    auto *summary = new QLabel(parts.isEmpty() ? tr("未提供真实航迹，不计算精度")
                                               : tr("均方根误差　%1").arg(parts.join(QStringLiteral("　·　"))));
    summary->setProperty("role", QStringLiteral("hint"));
    summary->setWordWrap(true);
    page->layout()->addWidget(summary);

    QCustomPlot *plot = Nav::createResultPlot(outcome);
    page->layout()->addWidget(plot);
    static_cast<QVBoxLayout *>(page->layout())->setStretch(1, 1);

    const QString image = QDir(lastOutputDir_).filePath(Nav::methodKey(runningMethod_) + QStringLiteral(".png"));
    if (plot->savePng(image, 1400, 900))
        appendLog(tr("结果图已保存：%1").arg(QDir::toNativeSeparators(image)));

    const Nav::MatchedTrack *best = nullptr;
    for (const Nav::MatchedTrack &t : outcome.tracks)
        if (std::isfinite(t.rms) && (!best || t.rms < best->rms))
            best = &t;
    preview_->setStatus(best ? tr("已完成 · 最小误差 %1").arg(Nav::formatError(best->rms, best->rmsMetres, outcome.unit))
                             : tr("已完成"),
                        Chip::Ok);
    appendLog(tr("%1 完成，总用时 %2 ms").arg(Nav::methodTitle(runningMethod_)).arg(outcome.elapsedMs), true);
    preview_->refreshEmptyState();
}

void NavigationForm::appendLog(const QString &line, bool important)
{
    const QString text = QStringLiteral("%1  %2").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), line.toHtmlEscaped());
    log_->append(important ? QStringLiteral("<b>%1</b>").arg(text) : text);
}
