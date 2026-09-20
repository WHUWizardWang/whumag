#include "workflowrail.h"

#include "thememanager.h"
#include "uiicons.h"

#include <QButtonGroup>
#include <QFont>
#include <QPainter>
#include <QVBoxLayout>

// ---------------------------------------------------------------- RailButton
RailButton::RailButton(const QString &text, const QString &iconName, int number, QWidget *parent)
    : QAbstractButton(parent), icon_(iconName), number_(number)
{
    setText(text);
    setCheckable(number > 0);
    setAutoExclusive(false);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
    setToolTip(text);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));
}

void RailButton::paintEvent(QPaintEvent *)
{
    const ThemeManager &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    const bool on = isChecked();
    if (on) {
        p.setPen(Qt::NoPen);
        p.setBrush(tm.color("accSoft"));
        p.drawRoundedRect(r, 8, 8);
    } else if (isDown()) {
        p.setPen(Qt::NoPen);
        p.setBrush(tm.color("n4"));
        p.drawRoundedRect(r, 8, 8);
    } else if (underMouse()) {
        p.setPen(Qt::NoPen);
        p.setBrush(tm.color("n3"));
        p.drawRoundedRect(r, 8, 8);
    }
    if (hasFocus()) {
        p.setPen(QPen(tm.color("acc"), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 8, 8);
    }

    const QColor fg = on ? tm.color("accInk") : tm.color("t2");
    const int iconSize = number_ > 0 ? 22 : 18;
    const int iconTop = number_ > 0 ? 12 : 8;
    p.drawPixmap(qRound((width() - iconSize) / 2.0), iconTop,
                 UiIcons::pixmap(icon_, iconSize, fg, 1.6, devicePixelRatioF()));

    QFont f = font();
    f.setPixelSize(number_ > 0 ? 12 : 11);
    f.setBold(on);
    p.setFont(f);
    p.setPen(number_ > 0 ? fg : tm.color("t3"));
    p.drawText(QRect(0, height() - 22, width(), 18), Qt::AlignHCenter | Qt::AlignVCenter, text());

    if (number_ > 0) {
        QFont nf(QStringLiteral("Cascadia Mono"));
        nf.setPixelSize(9);
        p.setFont(nf);
        p.setPen(on ? tm.color("accInk") : tm.color("t3"));
        p.drawText(QRect(7, 4, 14, 12), Qt::AlignLeft | Qt::AlignVCenter, QString::number(number_));
    }
}

// ---------------------------------------------------------------- WorkflowRail
QStringList WorkflowRail::stageNames()
{
    return {QStringLiteral("数据"), QStringLiteral("预处理"), QStringLiteral("建图"), QStringLiteral("评估"), QStringLiteral("导航")};
}

WorkflowRail::WorkflowRail(QWidget *parent) : QWidget(parent)
{
    setObjectName("workflowRail");
    setFixedWidth(68);
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("#workflowRail { background: %1; border-right: 1px solid %2; }")
                      .arg(ThemeManager::instance().hex("n1"), ThemeManager::instance().hex("line")));
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(QStringLiteral("#workflowRail { background: %1; border-right: 1px solid %2; }")
                          .arg(ThemeManager::instance().hex("n1"), ThemeManager::instance().hex("line")));
    });

    group_ = new QButtonGroup(this);
    group_->setExclusive(true);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 8, 6, 8);
    lay->setSpacing(4);

    const QStringList names = stageNames();
    const QStringList icons = {"database", "sliders", "grid", "gauge", "compass"};
    for (int i = 0; i < names.size(); ++i) {
        auto *b = new RailButton(names[i], icons[i], i + 1, this);
        b->setFixedSize(56, 60);
        group_->addButton(b, i);
        lay->addWidget(b);
    }
    lay->addStretch(1);

    auto *settings = new RailButton(tr("设置"), "gear", 0, this);
    settings->setFixedSize(56, 46);
    auto *help = new RailButton(tr("帮助"), "help", 0, this);
    help->setFixedSize(56, 46);
    lay->addWidget(settings);
    lay->addWidget(help);
    connect(settings, &QAbstractButton::clicked, this, &WorkflowRail::settingsClicked);
    connect(help, &QAbstractButton::clicked, this, &WorkflowRail::helpClicked);

    connect(group_, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &WorkflowRail::stageChanged);
    group_->button(0)->setChecked(true);
}

int WorkflowRail::currentStage() const
{
    return group_->checkedId();
}

void WorkflowRail::setCurrentStage(int index)
{
    if (QAbstractButton *b = group_->button(index)) {
        b->setChecked(true);
        emit stageChanged(index);
    }
}
