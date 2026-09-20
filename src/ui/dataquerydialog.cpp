#include "dataquerydialog.h"

#include "thememanager.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariant>

DataQueryDialog::DataQueryDialog(QWidget *globalModelForm, QWidget *anomalyForm, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("数据查询"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
    setSizeGripEnabled(true);
    resize(UiScale::windowSize(1180, 760));
    setMinimumSize(UiScale::windowSize(980, 640));

    header_ = new QWidget;
    header_->setObjectName("queryDialogHeader");
    header_->setAttribute(Qt::WA_StyledBackground, true);
    auto *hl = new QHBoxLayout(header_);
    hl->setContentsMargins(20, 12, 20, 12);
    hl->setSpacing(12);
    hl->addWidget(new IconBadge(QStringLiteral("database"), 34, 18));
    auto *title = new QLabel(tr("数据查询"));
    title->setProperty("role", QStringLiteral("h1"));
    title->setStyleSheet("font-size: 12.75pt;");
    hl->addWidget(title);
    hl->addSpacing(10);
    segmented_ = new SegmentedControl({tr("全球磁场模型"), tr("全球磁异常")});
    segmented_->setMinimumWidth(240);
    hl->addWidget(segmented_);
    hl->addStretch(1);
    auto *hint = new QLabel(tr("WMM / EMM / IGRF 模型值  ·  EMAG2 / MAMEA 磁异常"));
    hint->setProperty("role", QStringLiteral("hint"));
    hl->addWidget(hint);

    stack_ = new QStackedWidget;
    stack_->addWidget(globalModelForm);
    stack_->addWidget(anomalyForm);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(header_);
    root->addWidget(stack_, 1);

    connect(segmented_, &SegmentedControl::currentChanged, stack_, &QStackedWidget::setCurrentIndex);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &DataQueryDialog::restyle);
    restyle();
}

void DataQueryDialog::showPage(Page page)
{
    segmented_->setCurrentIndex(page);   // also switches the stack
    show();
    raise();
    activateWindow();
}

void DataQueryDialog::restyle()
{
    const ThemeManager &tm = ThemeManager::instance();
    header_->setStyleSheet(QStringLiteral("#queryDialogHeader { background: %1; border-bottom: 1px solid %2; }")
                               .arg(tm.hex("n2"), tm.hex("line")));
}
