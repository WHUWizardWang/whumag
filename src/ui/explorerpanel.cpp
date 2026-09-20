#include "explorerpanel.h"

#include "thememanager.h"
#include "uiicons.h"
#include "uiwidgets.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>

ExplorerPanel::ExplorerPanel(QTreeWidget *tree, QWidget *parent) : QWidget(parent), tree_(tree)
{
    setObjectName("explorerPanel");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *title = new QLabel(tr("工程"));
    title->setProperty("role", QStringLiteral("section"));
    title->setStyleSheet("font-size: 9.75pt;");
    openBtn_ = new QToolButton;
    openBtn_->setToolTip(tr("打开工程…"));
    openBtn_->setAutoRaise(true);
    newBtn_ = new QToolButton;
    newBtn_->setToolTip(tr("新建工程…"));
    newBtn_->setAutoRaise(true);
    for (QToolButton *b : {openBtn_, newBtn_}) {
        b->setFixedSize(26, 26);
        b->setIconSize(QSize(16, 16));
    }
    auto *head = new QHBoxLayout;
    head->setContentsMargins(14, 0, 8, 0);
    head->setSpacing(2);
    head->addWidget(title, 1);
    head->addWidget(openBtn_);
    head->addWidget(newBtn_);
    auto *headWrap = new QWidget;
    headWrap->setFixedHeight(40);
    headWrap->setLayout(head);
    root->addWidget(headWrap);

    search_ = new QLineEdit;
    search_->setPlaceholderText(tr("筛选数据…"));
    search_->setClearButtonEnabled(true);
    auto *searchWrap = new QWidget;
    auto *sl = new QHBoxLayout(searchWrap);
    sl->setContentsMargins(10, 0, 10, 8);
    sl->addWidget(search_);
    root->addWidget(searchWrap);

    tree_->setParent(this);
    tree_->setFrameShape(QFrame::NoFrame);
    tree_->setStyleSheet(QStringLiteral("QTreeWidget { border: none; border-top: 1px solid %1; border-radius: 0; background: %2; }")
                             .arg(ThemeManager::instance().hex("line"), ThemeManager::instance().hex("n1")));
    tree_->setIconSize(QSize(16, 16));
    tree_->setIndentation(16);
    tree_->setUniformRowHeights(true);
    root->addWidget(tree_, 1);

    emptyState_ = new QWidget;
    auto *el = new QVBoxLayout(emptyState_);
    el->setContentsMargins(28, 0, 28, 0);
    el->setSpacing(10);
    el->addStretch(1);
    auto *emptyBadge = new IconBadge("folder", 44, 22);
    el->addWidget(emptyBadge, 0, Qt::AlignHCenter);
    auto *emptyTitle = new QLabel(tr("尚未打开工程"));
    emptyTitle->setProperty("role", QStringLiteral("section"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptyText = new QLabel(tr("新建或打开一个工程，实测数据、处理结果会按类别显示在这里。"));
    emptyText->setProperty("role", QStringLiteral("hint"));
    emptyText->setAlignment(Qt::AlignCenter);
    emptyText->setWordWrap(true);
    el->addWidget(emptyTitle);
    el->addWidget(emptyText);
    auto *emptyButtons = new QHBoxLayout;
    emptyButtons->setSpacing(8);
    auto *emptyNew = new QPushButton(tr("新建"));
    emptyNew->setProperty("role", QStringLiteral("primary"));
    auto *emptyOpen = new QPushButton(tr("打开…"));
    emptyButtons->addStretch(1);
    emptyButtons->addWidget(emptyNew);
    emptyButtons->addWidget(emptyOpen);
    emptyButtons->addStretch(1);
    el->addLayout(emptyButtons);
    el->addStretch(2);
    root->addWidget(emptyState_, 1);
    connect(emptyNew, &QPushButton::clicked, this, &ExplorerPanel::newProjectRequested);
    connect(emptyOpen, &QPushButton::clicked, this, &ExplorerPanel::openProjectRequested);

    propsCard_ = new QFrame;
    propsCard_->setProperty("role", QStringLiteral("card"));
    propsGrid_ = new QGridLayout(propsCard_);
    propsGrid_->setContentsMargins(10, 8, 10, 10);
    propsGrid_->setHorizontalSpacing(8);
    propsGrid_->setVerticalSpacing(3);
    propsGrid_->setColumnStretch(1, 1);
    propsWrap_ = new QWidget;
    auto *cw = new QVBoxLayout(propsWrap_);
    cw->setContentsMargins(10, 10, 10, 10);
    cw->addWidget(propsCard_);
    root->addWidget(propsWrap_);
    propsWrap_->hide();

    connect(search_, &QLineEdit::textChanged, this, &ExplorerPanel::applyFilter);
    connect(openBtn_, &QToolButton::clicked, this, &ExplorerPanel::openProjectRequested);
    connect(newBtn_, &QToolButton::clicked, this, &ExplorerPanel::newProjectRequested);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        tree_->setStyleSheet(QStringLiteral("QTreeWidget { border: none; border-top: 1px solid %1; border-radius: 0; background: %2; }")
                                 .arg(ThemeManager::instance().hex("line"), ThemeManager::instance().hex("n1")));
        refreshIcons();
    });
    refreshIcons();

    connect(tree_->model(), &QAbstractItemModel::rowsInserted, this, [this]() { updateEmptyState(); });
    connect(tree_->model(), &QAbstractItemModel::rowsRemoved, this, [this]() { updateEmptyState(); });
    connect(tree_->model(), &QAbstractItemModel::modelReset, this, [this]() { updateEmptyState(); });
    updateEmptyState();
}

ExplorerPanel::~ExplorerPanel()
{
    // the tree (a child) still emits model signals while QWidget tears the children down
    closing_ = true;
}

void ExplorerPanel::updateEmptyState()
{
    if (closing_)
        return;
    const bool empty = tree_->topLevelItemCount() == 0;
    tree_->setVisible(!empty);
    search_->parentWidget()->setVisible(!empty);
    emptyState_->setVisible(empty);
}

void ExplorerPanel::refreshIcons()
{
    openBtn_->setIcon(UiIcons::icon("folder", 16));
    newBtn_->setIcon(UiIcons::icon("plus", 16));
}

bool ExplorerPanel::filterItem(QTreeWidgetItem *item, const QString &needle, bool parentMatched)
{
    const bool selfMatch = needle.isEmpty() || item->text(0).contains(needle, Qt::CaseInsensitive);
    bool anyChild = false;
    for (int i = 0; i < item->childCount(); ++i)
        anyChild |= filterItem(item->child(i), needle, parentMatched || selfMatch);
    const bool visible = selfMatch || anyChild || parentMatched;
    item->setHidden(!visible);
    if (anyChild && !needle.isEmpty())
        item->setExpanded(true);
    return selfMatch || anyChild;
}

void ExplorerPanel::applyFilter(const QString &needle)
{
    for (int i = 0; i < tree_->topLevelItemCount(); ++i)
        filterItem(tree_->topLevelItem(i), needle.trimmed(), false);
}

void ExplorerPanel::setProperties(const QVector<QPair<QString, QString>> &rows)
{
    while (QLayoutItem *it = propsGrid_->takeAt(0)) {
        if (QWidget *w = it->widget())
            w->deleteLater();
        delete it;
    }
    if (rows.isEmpty()) {
        propsWrap_->hide();
        return;
    }
    auto *heading = new QLabel(tr("属性"));
    heading->setProperty("role", QStringLiteral("section"));
    propsGrid_->addWidget(heading, 0, 0, 1, 2);
    int r = 1;
    for (const auto &kv : rows) {
        auto *k = new QLabel(kv.first);
        k->setProperty("role", QStringLiteral("hint"));
        auto *v = new ElideLabel(kv.second);
        v->setProperty("role", QStringLiteral("mono"));
        v->setStyleSheet("font-size: 8.25pt;");
        propsGrid_->addWidget(k, r, 0, Qt::AlignTop);
        propsGrid_->addWidget(v, r, 1);
        ++r;
    }
    propsCard_->show();
    propsWrap_->show();
}
