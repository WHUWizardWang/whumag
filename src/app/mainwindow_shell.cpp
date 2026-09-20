// Redesigned main-window shell: workflow rail, ribbon, project panel, task / log panel, status bar,
// menus and the command search.  The application logic (all the on_action_*_triggered slots) lives in
// mainwindow.cpp and is untouched; this file only decides where things are shown and how they are reached.
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "explorerpanel.h"
#include "logtextbrowser.h"
#include "ribbon.h"
#include "welcomepage.h"
#include "tasklistwidget.h"
#include "thememanager.h"
#include "uiicons.h"
#include "uiwidgets.h"
#include "workflowrail.h"

#include <QActionGroup>
#include <QButtonGroup>
#include <QCompleter>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace {

const char *kProjectTitle = "基于稀疏磁测数据的地磁基准图高效构建系统 V1.2";

QFrame *thinSeparator()
{
    auto *f = new QFrame;
    f->setFrameShape(QFrame::VLine);
    f->setFixedHeight(12);
    return f;
}

} // namespace

// ---------------------------------------------------------------- shell
void MainWindow::setupShell()
{
    QWidget *central = ui->centralwidget;
    delete central->layout();   // the old 2x2 grid; the widgets themselves are re-parented below

    rail_ = new WorkflowRail(central);
    ribbon_ = new Ribbon(central);
    explorer_ = new ExplorerPanel(ui->treeWidget, central);
    welcomePage_ = new WelcomePage(central);
    QStringList recents;
    for (const QString &p : qAsConst(historical_projects_))
        recents << p;
    welcomePage_->setRecentProjects(recents);

    centerStack_ = new QStackedWidget(central);
    centerStack_->addWidget(welcomePage_);
    centerStack_->addWidget(ui->tabWidget);
    centerStack_->setCurrentWidget(welcomePage_);

    QWidget *tasksPanel = nullptr;
    QWidget *logPanel = nullptr;
    setupBottomPanel(tasksPanel, logPanel);
    auto *bottom = new QSplitter(Qt::Horizontal);
    bottom->setChildrenCollapsible(false);
    bottom->addWidget(tasksPanel);
    bottom->addWidget(logPanel);
    bottom->setStretchFactor(0, 0);
    bottom->setStretchFactor(1, 1);
    bottom->setSizes({380, 900});

    centerSplitter_ = new QSplitter(Qt::Vertical);
    centerSplitter_->setChildrenCollapsible(false);
    centerSplitter_->addWidget(centerStack_);
    centerSplitter_->addWidget(bottom);
    centerSplitter_->setStretchFactor(0, 1);
    centerSplitter_->setStretchFactor(1, 0);
    centerSplitter_->setSizes({720, 210});

    bodySplitter_ = new QSplitter(Qt::Horizontal);
    bodySplitter_->setChildrenCollapsible(false);
    explorer_->setMinimumWidth(220);
    bodySplitter_->addWidget(explorer_);
    bodySplitter_->addWidget(centerSplitter_);
    bodySplitter_->setStretchFactor(0, 0);
    bodySplitter_->setStretchFactor(1, 1);
    bodySplitter_->setSizes({264, 1200});

    auto *column = new QVBoxLayout;
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    column->addWidget(ribbon_);
    column->addWidget(bodySplitter_, 1);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(rail_);
    root->addLayout(column, 1);

    setupRibbon();
    setupMenus();
    setupStatusBar();
    setupCommandSearch();

    // ---- wiring
    connect(rail_, &WorkflowRail::stageChanged, ribbon_, &Ribbon::setCurrentStage);
    connect(rail_, &WorkflowRail::helpClicked, this, &MainWindow::on_action_manuals_triggered);
    connect(welcomePage_, &WelcomePage::newProjectRequested, this, &MainWindow::on_action_newproject_triggered);
    connect(welcomePage_, &WelcomePage::openProjectRequested, this, &MainWindow::on_action_openproject_triggered);
    connect(welcomePage_, &WelcomePage::openRecentRequested, this, [this](const QString &file) {
        if (!QFileInfo::exists(file)) {
            QMessageBox::warning(this, tr("打开工程"), tr("找不到工程文件：\n%1").arg(file));
            return;
        }
        openProjectPath(file);
    });
    connect(welcomePage_, &WelcomePage::quickStartRequested, this, [this](int which) {
        if (which == 0)
            on_action_auto_triggered();
        else if (which == 1)
            on_action_globalmodel_triggered();
        else
            on_action_nav_triggered();
    });
    connect(explorer_, &ExplorerPanel::newProjectRequested, this, &MainWindow::on_action_newproject_triggered);
    connect(explorer_, &ExplorerPanel::openProjectRequested, this, &MainWindow::on_action_openproject_triggered);
    connect(ui->treeWidget, &QTreeWidget::currentItemChanged, this, [this]() { onTreeSelectionChanged(); });
    connect(taskListView_, &TaskListWidget::countsChanged, this, [this]() { updateStatusBar(); });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        refreshTreeIcons();
        updateStatusBar();
    });

    refreshShell();
}

void MainWindow::setupBottomPanel(QWidget *&tasksPanel, QWidget *&logPanel)
{
    // ---- tasks
    taskListView_ = new TaskListWidget;
    taskList = taskListView_;
    ui->taskListWidget->hide();
    ui->taskListWidget->deleteLater();

    auto *tasksTitle = new QLabel(tr("任务"));
    tasksTitle->setProperty("role", QStringLiteral("panelTitle"));
    auto *tasksCount = new Chip(QStringLiteral("0"), Chip::Neutral);
    auto *clearTasks = new QToolButton;
    clearTasks->setAutoRaise(true);
    clearTasks->setToolTip(tr("清除已结束的任务"));
    clearTasks->setIcon(UiIcons::icon("trash", 14));
    clearTasks->setIconSize(QSize(14, 14));
    auto *tasksHead = new QHBoxLayout;
    tasksHead->setContentsMargins(12, 0, 8, 0);
    tasksHead->setSpacing(8);
    tasksHead->addWidget(tasksTitle);
    tasksHead->addWidget(tasksCount);
    tasksHead->addStretch(1);
    tasksHead->addWidget(clearTasks);
    auto *tasksHeadWrap = new QWidget;
    tasksHeadWrap->setFixedHeight(32);
    tasksHeadWrap->setLayout(tasksHead);

    auto *tasksEmpty = new QLabel(tr("暂无任务"));
    tasksEmpty->setAlignment(Qt::AlignCenter);
    tasksEmpty->setProperty("role", QStringLiteral("hint"));
    auto *tasksStack = new QStackedWidget;
    tasksStack->addWidget(tasksEmpty);
    tasksStack->addWidget(taskListView_);

    tasksPanel = new QWidget;
    auto *tl = new QVBoxLayout(tasksPanel);
    tl->setContentsMargins(0, 0, 0, 0);
    tl->setSpacing(0);
    tl->addWidget(tasksHeadWrap);
    tl->addWidget(tasksStack, 1);
    tasksPanel->setMinimumWidth(300);

    connect(clearTasks, &QToolButton::clicked, taskListView_, &TaskListWidget::clearFinished);
    connect(taskListView_, &TaskListWidget::countsChanged, this, [tasksCount, tasksStack](int running, int total) {
        tasksCount->setText(QString::number(total));
        tasksCount->setKind(running > 0 ? Chip::Accent : Chip::Neutral);
        tasksStack->setCurrentIndex(total > 0 ? 1 : 0);
    });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, clearTasks, [clearTasks]() { clearTasks->setIcon(UiIcons::icon("trash", 14)); });

    // ---- log
    auto *logTitle = new QLabel(tr("日志"));
    logTitle->setProperty("role", QStringLiteral("panelTitle"));
    auto *chipAll = new QPushButton(tr("全部"));
    auto *chipWarn = new QPushButton(tr("警告"));
    auto *chipErr = new QPushButton(tr("错误"));
    auto *chips = new QButtonGroup(this);
    int id = 0;
    for (QPushButton *c : {chipAll, chipWarn, chipErr}) {
        c->setCheckable(true);
        c->setProperty("role", QStringLiteral("chip"));
        c->setCursor(Qt::PointingHandCursor);
        chips->addButton(c, id++);
    }
    chipAll->setChecked(true);
    auto *clearLog = new QToolButton;
    clearLog->setAutoRaise(true);
    clearLog->setToolTip(tr("清空日志"));
    clearLog->setIcon(UiIcons::icon("trash", 14));
    clearLog->setIconSize(QSize(14, 14));
    auto *logHead = new QHBoxLayout;
    logHead->setContentsMargins(12, 0, 8, 0);
    logHead->setSpacing(6);
    logHead->addWidget(logTitle);
    logHead->addStretch(1);
    logHead->addWidget(chipAll);
    logHead->addWidget(chipWarn);
    logHead->addWidget(chipErr);
    logHead->addSpacing(4);
    logHead->addWidget(clearLog);
    auto *logHeadWrap = new QWidget;
    logHeadWrap->setFixedHeight(32);
    logHeadWrap->setLayout(logHead);

    logPanel = new QWidget;
    auto *ll = new QVBoxLayout(logPanel);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(0);
    ll->addWidget(logHeadWrap);
    auto *logBodyWrap = new QWidget;
    auto *lb = new QVBoxLayout(logBodyWrap);
    lb->setContentsMargins(8, 0, 8, 8);
    lb->addWidget(ui->textBrowser);
    ll->addWidget(logBodyWrap, 1);

    connect(chips, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int which) { ui->textBrowser->setMinimumLevel(which); });
    connect(ui->textBrowser, &LogTextBrowser::countsChanged, this, [chipWarn, chipErr](int warnings, int errors) {
        chipWarn->setText(warnings > 0 ? QStringLiteral("警告 %1").arg(warnings) : QStringLiteral("警告"));
        chipErr->setText(errors > 0 ? QStringLiteral("错误 %1").arg(errors) : QStringLiteral("错误"));
    });
    connect(clearLog, &QToolButton::clicked, ui->textBrowser, &LogTextBrowser::clearLog);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, clearLog, [clearLog]() { clearLog->setIcon(UiIcons::icon("trash", 14)); });
}

// ---------------------------------------------------------------- ribbon
void MainWindow::registerCommand(const QString &label, const QString &stage, QAction *action)
{
    if (!action)
        return;
    commands_.append(qMakePair(stage.isEmpty() ? label : QStringLiteral("%1  ·  %2").arg(label, stage), action));
}

void MainWindow::setupRibbon()
{
    ui->action_import->setText(tr("导入数据文件…"));
    ui->action_readlines->setText(tr("导入多条测线…"));
    ui->action_6->setText(tr("导出所选数据…"));
    ui->action_nav->setText(tr("匹配导航"));
    ui->action_database->setText(tr("数据库管理"));
    ui->actionshow->setText(tr("显示热力图与等值线…"));

    auto button = [this](QAction *a, const QString &label, const QString &icon, const QString &stage, int width = 64) {
        Ribbon::Button b;
        b.action = a;
        b.label = label;
        b.icon = icon;
        b.width = width;
        registerCommand(a ? a->text() : label, stage, a);
        return b;
    };
    auto menuButton = [this](QMenu *m, const QString &label, const QString &icon, const QString &stage, int width = 64) {
        Ribbon::Button b;
        b.menu = m;
        b.label = label;
        b.icon = icon;
        b.width = width;
        for (QAction *a : m->actions())
            registerCommand(a->text(), stage, a);
        return b;
    };

    auto *importMenu = new QMenu(this);
    importMenu->addAction(ui->action_import);
    importMenu->addAction(ui->action_readlines);
    auto *spacingMenu = new QMenu(this);
    spacingMenu->addAction(ui->action_shuimian);
    spacingMenu->addAction(ui->action_kongzhong);

    QList<Ribbon::Stage> stages;

    Ribbon::Stage data;
    data.name = tr("数据");
    data.icon = "database";
    data.description = tr("导入实测数据，或查询全球模型");
    data.groups = {
        {tr("文件"), {menuButton(importMenu, tr("导入"), "import", data.name), button(ui->action_6, tr("导出"), "export", data.name)}},
        {tr("全球数据"), {button(ui->action_query, tr("磁场查询"), "globe", data.name), button(ui->action_anoquery, tr("磁异常查询"), "anomaly", data.name, 80)}},
        {tr("数据库"), {button(ui->action_database, tr("数据库管理"), "database", data.name, 80)}},
    };
    stages << data;

    Ribbon::Stage pre;
    pre.name = tr("预处理");
    pre.icon = "sliders";
    pre.description = tr("改正、融合、延拓与复杂度分析");
    pre.groups = {
        {tr("改正与融合"), {button(ui->action_correct, tr("通化改正"), "correct", pre.name), button(ui->action_merge, tr("数据融合"), "merge", pre.name)}},
        {tr("延拓"), {button(ui->action_up, tr("向上延拓"), "up-cont", pre.name), button(ui->action_down, tr("向下延拓"), "down-cont", pre.name),
                    button(ui->action_evaluate, tr("延拓精度评估"), "gauge", pre.name, 84)}},
        {tr("复杂度"), {button(ui->action_3, tr("复杂度计算"), "complexity", pre.name, 76), menuButton(spacingMenu, tr("测线间距规划"), "route", pre.name, 96)}},
    };
    stages << pre;

    Ribbon::Stage map;
    map.name = tr("建图");
    map.icon = "grid";
    map.description = tr("用稀疏测线构建地磁基准图");
    map.groups = {
        {tr("整图"), {button(ui->action_globalmodel, tr("整图建模"), "grid", map.name)}},
        {tr("分区建模"), {button(ui->action_subarea, tr("分区"), "rect-select", map.name), button(ui->action_build, tr("建模"), "heat", map.name),
                       button(ui->action_suball, tr("一键处理"), "play", map.name)}},
        {tr("智能"), {button(ui->action_auto, tr("一键成图"), "spark", map.name)}},
    };
    stages << map;

    Ribbon::Stage eval;
    eval.name = tr("评估");
    eval.icon = "gauge";
    eval.description = tr("用检核线评估基准图精度");
    eval.groups = {
        {tr("检核线"), {button(ui->actionjianhexian, tr("水下检核线"), "ship", eval.name, 76), button(ui->actionjianhexian2, tr("低空检核线"), "plane", eval.name, 76)}},
        {tr("可稀疏性分析"), {button(ui->action_xishushuixia, tr("水下"), "ship", eval.name), button(ui->action_xishukongzhong, tr("低空"), "plane", eval.name)}},
    };
    stages << eval;

    Ribbon::Stage nav;
    nav.name = tr("导航");
    nav.icon = "compass";
    nav.description = tr("用基准图运行匹配导航");
    nav.groups = {
        {tr("匹配导航"), {button(ui->action_nav, tr("匹配导航"), "compass", nav.name, 76)}},
    };
    stages << nav;

    ribbon_->setStages(stages);
    ribbon_->setCurrentStage(0);

    // menu entries that live only in the menu bar are searchable too
    registerCommand(tr("新建工程"), tr("文件"), ui->action_newproject);
    registerCommand(tr("打开工程"), tr("文件"), ui->action_openproject);
    registerCommand(tr("显示热力图与等值线"), tr("视图"), ui->actionshow);
    registerCommand(tr("三维叠加显示"), tr("视图"), ui->action_3d);
    registerCommand(tr("操作手册"), tr("帮助"), ui->action_manuals);
    registerCommand(tr("关于"), tr("帮助"), ui->action_about);
    registerCommand(tr("抽稀参数"), tr("工具"), ui->action_sparsePara);
}

// ---------------------------------------------------------------- menus
void MainWindow::setupMenus()
{
    QMenuBar *mb = menuBar();
    mb->clear();

    ui->action_newproject->setShortcut(QKeySequence::New);
    ui->action_openproject->setShortcut(QKeySequence::Open);
    ui->action_import->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    ui->action_exit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    ui->action_manuals->setShortcut(QKeySequence::HelpContents);

    QMenu *file = mb->addMenu(tr("文件(&F)"));
    file->addAction(ui->action_newproject);
    file->addAction(ui->action_openproject);
    ui->menu_history->setTitle(tr("最近工程"));
    file->addMenu(ui->menu_history);
    file->addSeparator();
    file->addAction(ui->action_import);
    file->addAction(ui->action_readlines);
    file->addAction(ui->action_6);
    file->addSeparator();
    file->addAction(ui->action_exit);

    QMenu *view = mb->addMenu(tr("视图(&V)"));
    QAction *tExplorer = view->addAction(tr("工程面板"));
    tExplorer->setCheckable(true);
    tExplorer->setChecked(true);
    connect(tExplorer, &QAction::toggled, explorer_, &QWidget::setVisible);
    QAction *tBottom = view->addAction(tr("任务与日志"));
    tBottom->setCheckable(true);
    tBottom->setChecked(true);
    QWidget *bottomWidget = centerSplitter_->widget(1);
    connect(tBottom, &QAction::toggled, bottomWidget, &QWidget::setVisible);
    view->addSeparator();
    QAction *tabMap = view->addAction(tr("地图"));
    connect(tabMap, &QAction::triggered, this, [this]() { ui->tabWidget->setCurrentIndex(0); });
    QAction *tab3d = view->addAction(tr("三维"));
    connect(tab3d, &QAction::triggered, this, [this]() { ui->tabWidget->setCurrentIndex(1); });
    view->addAction(ui->actionshow);
    view->addAction(ui->action_3d);
    view->addSeparator();

    QMenu *themeMenu = view->addMenu(tr("主题"));
    auto *themeGroup = new QActionGroup(this);
    const QStringList themeNames = {tr("浅色"), tr("深色"), tr("跟随系统")};
    for (int i = 0; i < themeNames.size(); ++i) {
        QAction *a = themeMenu->addAction(themeNames[i]);
        a->setCheckable(true);
        a->setActionGroup(themeGroup);
        a->setChecked(int(ThemeManager::instance().mode()) == i);
        connect(a, &QAction::triggered, this, [i]() { ThemeManager::instance().setMode(static_cast<ThemeManager::Mode>(i)); });
    }
    QMenu *accentMenu = view->addMenu(tr("强调色"));
    auto *accentGroup = new QActionGroup(this);
    const QStringList accents = ThemeManager::accentNames();
    for (int i = 0; i < accents.size(); ++i) {
        QAction *a = accentMenu->addAction(accents[i]);
        a->setCheckable(true);
        a->setActionGroup(accentGroup);
        a->setChecked(ThemeManager::instance().accentPreset() == i);
        connect(a, &QAction::triggered, this, [i]() { ThemeManager::instance().setAccentPreset(i); });
    }

    QMenu *tools = mb->addMenu(tr("工具(&T)"));
    tools->addAction(ui->action_sparsePara);
    tools->addAction(ui->action_database);

    QMenu *help = mb->addMenu(tr("帮助(&H)"));
    help->addAction(ui->action_manuals);
    help->addSeparator();
    help->addAction(ui->action_about);

    // the rail's settings button opens the same theme choices
    QPointer<QMenu> quickTheme = themeMenu;
    connect(rail_, &WorkflowRail::settingsClicked, this, [quickTheme]() {
        if (quickTheme)
            quickTheme->exec(QCursor::pos());
    });

    // database-backed features are unavailable when the user chose to work offline
    if (DatabaseManager::instance().isOffline()) {
        for (QAction *a : {ui->action_anoquery, ui->action_database}) {
            a->setEnabled(false);
            a->setToolTip(tr("离线工作模式下不可用"));
        }
    }
}

void MainWindow::setupCommandSearch()
{
    commandSearch_ = new QLineEdit;
    commandSearch_->setFixedWidth(260);
    commandSearch_->setPlaceholderText(tr("搜索功能…  Ctrl K"));
    commandSearch_->setClearButtonEnabled(true);
    commandSearch_->addAction(UiIcons::icon("search", 14), QLineEdit::LeadingPosition);
    menuBar()->setCornerWidget(commandSearch_, Qt::TopRightCorner);

    QStringList labels;
    for (const auto &c : qAsConst(commands_))
        if (!labels.contains(c.first))
            labels << c.first;
    auto *completer = new QCompleter(labels, commandSearch_);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    commandSearch_->setCompleter(completer);

    auto trigger = [this](const QString &label) {
        for (const auto &c : qAsConst(commands_)) {
            if (c.first == label && c.second->isEnabled()) {
                commandSearch_->clear();
                c.second->trigger();
                return;
            }
        }
    };
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated), this, trigger);
    connect(commandSearch_, &QLineEdit::returnPressed, this, [this, completer, trigger]() {
        if (completer->currentCompletion().isEmpty())
            return;
        trigger(completer->currentCompletion());
    });

    auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_K), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        commandSearch_->setFocus();
        commandSearch_->selectAll();
    });
}

// ---------------------------------------------------------------- status bar
void MainWindow::setupStatusBar()
{
    QStatusBar *sb = statusBar();
    sb->setSizeGripEnabled(false);
    dbLabel_ = new QLabel;
    projectLabel_ = new QLabel;
    auto *crs = new QLabel(tr("坐标系 WGS-84"));
    taskCountLabel_ = new QLabel;
    sb->addWidget(dbLabel_);
    sb->addWidget(thinSeparator());
    sb->addWidget(projectLabel_);
    sb->addWidget(thinSeparator());
    sb->addWidget(crs);
    sb->addPermanentWidget(taskCountLabel_);
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    if (closing_ || !dbLabel_)
        return;
    const ThemeManager &tm = ThemeManager::instance();
    DatabaseManager &dbm = DatabaseManager::instance();
    QString color, text;
    if (dbm.isOffline()) {
        color = tm.hex("t3");
        text = tr("离线工作");
    } else if (dbm.getDatabase().isOpen()) {
        color = tm.hex("ok");
        text = tr("数据库已连接 · %1:%2").arg(dbm.getDatabase().hostName()).arg(dbm.getDatabase().port());
    } else {
        color = tm.hex("warn");
        text = tr("数据库未连接");
    }
    dbLabel_->setText(QStringLiteral("<span style=\"color:%1\">●</span>&nbsp;%2").arg(color, text.toHtmlEscaped()));

    const bool hasProject = !geomag_proj_->Name().isEmpty();
    projectLabel_->setText(hasProject ? geomag_proj_->Name() : tr("未打开工程"));

    const int running = taskListView_ ? taskListView_->runningCount() : 0;
    taskCountLabel_->setText(running > 0 ? tr("后台任务 %1").arg(running) : tr("无后台任务"));
}

// ---------------------------------------------------------------- project state
void MainWindow::refreshShell()
{
    const bool hasProject = !geomag_proj_->Name().isEmpty();
    centerStack_->setCurrentWidget(hasProject ? static_cast<QWidget *>(ui->tabWidget) : static_cast<QWidget *>(welcomePage_));

    QString title = QString::fromUtf8(kProjectTitle);
    if (hasProject)
        title += QStringLiteral(" — ") + geomag_proj_->Name();
    setWindowTitle(title);

    QStringList recents;
    for (const QString &p : qAsConst(historical_projects_))
        recents << p;
    welcomePage_->setRecentProjects(recents);

    updatePipelineState();
    updateStatusBar();
    onTreeSelectionChanged();
}

void MainWindow::refreshTreeIcons()
{
    std::function<void(QTreeWidgetItem *)> apply = [&](QTreeWidgetItem *item) {
        const QString name = item->data(0, Qt::UserRole).toString();
        if (!name.isEmpty())
            item->setIcon(0, UiIcons::icon(name, 16));
        if (item->treeWidget() && item->text(1).size() > 0)
            item->setForeground(1, QBrush(ThemeManager::instance().color("t3")));
        for (int i = 0; i < item->childCount(); ++i)
            apply(item->child(i));
    };
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
        apply(ui->treeWidget->topLevelItem(i));
}

void MainWindow::updatePipelineState()
{
    if (closing_ || !ribbon_)
        return;
    QVector<int> st(5, PipelineProgress::Todo);
    const bool hasProject = !geomag_proj_->Name().isEmpty();
    if (hasProject) {
        const bool hasData = !geomag_proj_->nameList_real.isEmpty();
        const bool done[5] = {hasData, hasData && !geomag_proj_->nameList_processed.isEmpty(), mapBuilt_, evaluated_, false};
        bool currentMarked = false;
        for (int i = 0; i < 5; ++i) {
            if (done[i]) {
                st[i] = PipelineProgress::Done;
            } else if (!currentMarked) {
                st[i] = PipelineProgress::Current;
                currentMarked = true;
            }
        }
    }
    ribbon_->pipeline()->setStates(st);
}

QString MainWindow::selectedDataFilePath() const
{
    QTreeWidgetItem *it = ui->treeWidget->currentItem();
    if (!it || !it->parent() || !it->parent()->parent())
        return QString();
    const int category = it->parent()->parent()->indexOfChild(it->parent());
    if (category == 2)
        return geomag_proj_->Path() + "/Measured/" + it->text(0);
    if (category == 3)
        return geomag_proj_->Path() + "/Processed/" + it->text(0);
    return QString();
}

void MainWindow::onTreeSelectionChanged()
{
    if (closing_)
        return;
    QVector<QPair<QString, QString>> rows;
    QTreeWidgetItem *it = ui->treeWidget->currentItem();
    if (it && !geomag_proj_->Name().isEmpty()) {
        if (!it->parent()) {
            rows << qMakePair(tr("名称"), geomag_proj_->Name());
            rows << qMakePair(tr("位置"), QDir::toNativeSeparators(geomag_proj_->Path()));
            rows << qMakePair(tr("数据"), tr("实测 %1 · 处理后 %2").arg(geomag_proj_->nameList_real.size()).arg(geomag_proj_->nameList_processed.size()));
        } else {
            const QString path = selectedDataFilePath();
            if (!path.isEmpty()) {
                const QFileInfo fi(path);
                const int category = it->parent()->parent()->indexOfChild(it->parent());
                rows << qMakePair(tr("文件"), fi.fileName());
                rows << qMakePair(tr("类别"), category == 2 ? tr("实测数据") : tr("处理后数据"));
                rows << qMakePair(tr("大小"), fi.exists() ? QLocale().formattedDataSize(fi.size()) : tr("文件不存在"));
                rows << qMakePair(tr("修改"), fi.exists() ? fi.lastModified().toString("yyyy-MM-dd HH:mm") : QString());
                rows << qMakePair(tr("位置"), QDir::toNativeSeparators(fi.absolutePath()));
            }
        }
    }
    explorer_->setProperties(rows);
}

// ---------------------------------------------------------------- export (previously an empty menu item)
void MainWindow::on_action_6_triggered()
{
    const QString source = selectedDataFilePath();
    if (source.isEmpty() || !QFileInfo::exists(source)) {
        QMessageBox::information(this, tr("导出"), tr("请先在左侧工程树中选择一个实测数据或处理后数据文件。"));
        return;
    }
    const QString target = QFileDialog::getSaveFileName(this, tr("导出数据"), QFileInfo(source).fileName());
    if (target.isEmpty())
        return;
    if (QFile::exists(target))
        QFile::remove(target);
    if (QFile::copy(source, target))
        ui->textBrowser->append(tr("已导出：%1").arg(QDir::toNativeSeparators(target)));
    else
        QMessageBox::warning(this, tr("导出"), tr("导出失败，请检查目标位置是否可写。"));
}
