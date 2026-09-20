#include "queryformshell.h"

#include "formkit.h"
#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTableView>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QVariant>

namespace QueryFormShell {

namespace {

QLabel *caption(const QString &text)
{
    auto *l = new QLabel(text);
    l->setProperty("role", QStringLiteral("section"));
    return l;
}

QString panelStyle(const QString &objectName, const char *bg, const char *borderSide)
{
    const ThemeManager &tm = ThemeManager::instance();
    return QStringLiteral("#%1 { background: %2; border: none; %3 }")
        .arg(objectName, tm.hex(bg), borderSide ? QStringLiteral("border-%1: 1px solid %2;").arg(QString::fromLatin1(borderSide), tm.hex("line")) : QString());
}

} // namespace

Shell apply(const Parts &p, const QString &productCaption)
{
    Shell shell;
    QWidget *form = p.form;

    // ---- keep the widgets that must survive; the old frames are deleted at the end
    for (QWidget *w : {static_cast<QWidget *>(p.radioPoint), static_cast<QWidget *>(p.radioFile), static_cast<QWidget *>(p.radioGrid),
                       static_cast<QWidget *>(p.log)}) {
        if (w) {
            w->setParent(form);
            w->hide();
        }
    }

    // ---- left panel
    auto *left = new QWidget;
    left->setObjectName("queryLeft");
    left->setFixedWidth(UiScale::dp(416));
    left->setAttribute(Qt::WA_StyledBackground, true);
    auto *ll = new QVBoxLayout(left);
    ll->setContentsMargins(20, 18, 20, 16);
    ll->setSpacing(16);

    auto *productBlock = new QVBoxLayout;
    productBlock->setSpacing(6);
    productBlock->addWidget(caption(productCaption));
    productBlock->addWidget(p.product);
    ll->addLayout(productBlock);

    QStringList modeLabels = {QStringLiteral("单点")};
    if (p.radioFile)
        modeLabels << QStringLiteral("点文件");
    modeLabels << QStringLiteral("格网");
    auto *seg = new SegmentedControl(modeLabels);
    auto *modeBlock = new QVBoxLayout;
    modeBlock->setSpacing(6);
    modeBlock->addWidget(caption(QStringLiteral("查询方式")));
    modeBlock->addWidget(seg);
    ll->addLayout(modeBlock);

    // the segmented control drives the (hidden) radio buttons, which the forms already react to
    QObject::connect(seg, &SegmentedControl::currentChanged, form, [p](int index) {
        QRadioButton *target = index == 0 ? p.radioPoint : (p.radioFile ? (index == 1 ? p.radioFile : p.radioGrid) : p.radioGrid);
        if (target)
            target->setChecked(true);
    });
    auto syncFromRadio = [seg, p]() {
        if (p.radioPoint->isChecked())
            seg->blockSignals(true), seg->setCurrentIndex(0), seg->blockSignals(false);
        else if (p.radioFile && p.radioFile->isChecked())
            seg->blockSignals(true), seg->setCurrentIndex(1), seg->blockSignals(false);
        else if (p.radioGrid->isChecked())
            seg->blockSignals(true), seg->setCurrentIndex(p.radioFile ? 2 : 1), seg->blockSignals(false);
    };
    for (QRadioButton *r : {p.radioPoint, p.radioFile, p.radioGrid})
        if (r)
            QObject::connect(r, &QRadioButton::toggled, seg, syncFromRadio);
    syncFromRadio();

    auto *subformHost = new QWidget;
    auto *sl = new QVBoxLayout(subformHost);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->setSpacing(0);
    for (QWidget *sub : p.subforms)
        sl->addWidget(sub);
    ll->addWidget(subformHost);

    shell.leftLayout = ll;
    shell.leftInsertIndex = ll->count();
    ll->addStretch(1);
    p.query->setProperty("role", QStringLiteral("primary"));
    p.query->setMinimumHeight(34);
    p.query->setCursor(Qt::PointingHandCursor);
    ll->addWidget(p.query);

    // ---- right panel
    auto *right = new QWidget;
    right->setObjectName("queryRight");
    right->setAttribute(Qt::WA_StyledBackground, true);
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(22, 18, 22, 16);
    rl->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("查询结果"));
    title->setProperty("role", QStringLiteral("h2"));
    shell.statusChip = new Chip(QStringLiteral("未查询"), Chip::Neutral);
    auto *head = new QHBoxLayout;
    head->setSpacing(10);
    head->addWidget(title);
    head->addWidget(shell.statusChip);
    head->addStretch(1);
    rl->addLayout(head);

    shell.middle = new QVBoxLayout;
    shell.middle->setContentsMargins(0, 0, 0, 0);
    shell.middle->setSpacing(12);
    rl->addLayout(shell.middle);

    auto *listTitle = new QLabel(QStringLiteral("结果列表"));
    listTitle->setProperty("role", QStringLiteral("section"));
    rl->addWidget(listTitle);

    p.table->setAlternatingRowColors(true);
    p.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    p.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    p.table->verticalHeader()->setVisible(false);
    p.table->verticalHeader()->setDefaultSectionSize(26);
    p.table->horizontalHeader()->setStretchLastSection(true);
    p.table->horizontalHeader()->setHighlightSections(false);
    p.table->setShowGrid(false);
    FormKit::emptyStateFor(p.table, QStringLiteral("尚无结果"), QStringLiteral("在左侧设置参数，然后点击“查询”"));
    rl->addWidget(p.table, 1);

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch(1);
    if (p.importButton) {
        p.importButton->setProperty("role", QStringLiteral("primary"));
        footer->addWidget(p.importButton);
    }
    if (p.exportButton)
        footer->addWidget(p.exportButton);
    rl->addLayout(footer);

    // ---- swap the layouts
    delete form->layout();
    delete p.oldLeft;
    delete p.oldRight;
    auto *root = new QHBoxLayout(form);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(left);
    root->addWidget(right, 1);

    auto restyle = [left, right]() {
        left->setStyleSheet(panelStyle("queryLeft", "n1", "right"));
        right->setStyleSheet(panelStyle("queryRight", "n2", nullptr));
    };
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, form, restyle);
    restyle();

    shell.leftPanel = left;
    shell.rightPanel = right;
    return shell;
}

} // namespace QueryFormShell
