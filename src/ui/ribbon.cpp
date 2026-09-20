#include "ribbon.h"

#include "thememanager.h"
#include "uiicons.h"
#include "uiscale.h"
#include "uiwidgets.h"

#include <QAction>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariant>

// ---------------------------------------------------------------- RibbonButton
RibbonButton::RibbonButton(const QString &label, const QString &iconName, QWidget *parent)
    : QToolButton(parent), icon_(iconName), label_(label)
{
    setText(label);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setAutoRaise(true);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));
}

namespace {
constexpr int kBtnTop = 8;      // above the icon
constexpr int kBtnIcon = 22;
constexpr int kBtnGap = 6;      // icon -> label
constexpr int kBtnBottom = 6;

QFont buttonLabelFont(const QFont &base)
{
    QFont f = base;
    f.setPointSizeF(9);
    return f;
}

int labelHeight(const QFont &base)
{
    return qMax(QFontMetrics(buttonLabelFont(base)).height(), 18);
}

int captionHeight(const QFont &base)
{
    QFont f = base;
    f.setPointSizeF(8.25);
    return qMax(QFontMetrics(f).height() + 2, 16);
}

constexpr int kPipeDot = 16, kPipeGap = 6, kPipeConn = 20, kPipeConnGap = 8;

// Names shown by the progress strip
QStringList stageNames()
{
    return {QStringLiteral("数据"), QStringLiteral("预处理"), QStringLiteral("建图"), QStringLiteral("评估"), QStringLiteral("导航")};
}
} // namespace

int RibbonButton::heightHint(const QFont &base)
{
    return kBtnTop + kBtnIcon + kBtnGap + labelHeight(base) + kBtnBottom;
}

QSize RibbonButton::sizeHint() const
{
    const int textW = QFontMetrics(buttonLabelFont(font())).horizontalAdvance(label_) + (menu() ? 14 : 0);
    return QSize(qMax(UiScale::dp(64), textW + 20), heightHint(font()));
}

void RibbonButton::paintEvent(QPaintEvent *)
{
    const ThemeManager &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool enabled = isEnabled();
    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);

    if (enabled && (isChecked() || isDown() || underMouse())) {
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? tm.color("accSoft") : (isDown() ? tm.color("n4") : tm.color("n3")));
        p.drawRoundedRect(r, 6, 6);
    }
    if (hasFocus()) {
        p.setPen(QPen(tm.color("acc"), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 6, 6);
    }

    QColor fg = isChecked() ? tm.color("accInk") : tm.color("t1");
    if (!enabled)
        fg.setAlphaF(0.42);
    p.drawPixmap(qRound((width() - kBtnIcon) / 2.0), kBtnTop, UiIcons::pixmap(icon_, kBtnIcon, fg, 1.6, devicePixelRatioF()));

    QFont f = buttonLabelFont(font());
    f.setBold(isChecked());
    p.setFont(f);
    p.setPen(fg);
    const int chevronW = menu() ? 12 : 0;
    const int textW = QFontMetrics(f).horizontalAdvance(label_);
    const int total = textW + chevronW;
    const int x0 = (width() - total) / 2;
    const int textTop = kBtnTop + kBtnIcon + kBtnGap;
    const int textH = labelHeight(font());
    p.drawText(QRect(x0, textTop, textW + 2, textH), Qt::AlignLeft | Qt::AlignVCenter, label_);
    if (menu())
        p.drawPixmap(x0 + textW + 2, textTop + (textH - 10) / 2, UiIcons::pixmap("chev-down", 10, fg, 2.2, devicePixelRatioF()));
}

// ---------------------------------------------------------------- PipelineProgress
PipelineProgress::PipelineProgress(QWidget *parent) : QWidget(parent)
{
    states_ = QVector<int>(5, Todo);
    setToolTip(tr("工程进度：已完成的阶段显示对勾，当前阶段显示圆环"));
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));
}

void PipelineProgress::setStates(const QVector<int> &states)
{
    states_ = states;
    update();
}

void PipelineProgress::paintEvent(QPaintEvent *)
{
    const ThemeManager &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QStringList names = stageNames();
    QFont f = font();
    f.setPointSizeF(9);
    const QFontMetrics fm(f);
    QFont cf = font();
    cf.setPointSizeF(8.25);
    const int captionH = QFontMetrics(cf).height() + 2;

    // total width: dot + label per stage, connectors between
    const int dot = kPipeDot, gap = kPipeGap, conn = kPipeConn, connGap = kPipeConnGap;
    int total = 0;
    for (int i = 0; i < 5; ++i)
        total += dot + gap + fm.horizontalAdvance(names[i]);
    total += 4 * (conn + 2 * connGap);
    int x = qMax(8, (width() - total) / 2);
    const int cy = (height() - captionH - 4) / 2 - 2;   // centre of the area above the caption

    for (int i = 0; i < 5; ++i) {
        const int st = i < states_.size() ? states_[i] : Todo;
        const QRectF d(x, cy - dot / 2.0, dot, dot);
        if (st == Done) {
            p.setPen(Qt::NoPen);
            p.setBrush(tm.color("ok"));
            p.drawEllipse(d);
            p.drawPixmap(QPointF(d.x() + 3, d.y() + 3), UiIcons::pixmap("check", 10, tm.isDark() ? QColor("#0A1219") : QColor("#FFFFFF"), 2.8, devicePixelRatioF()));
        } else if (st == Current) {
            p.setPen(QPen(tm.color("acc"), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(d.adjusted(1, 1, -1, -1));
            p.setPen(Qt::NoPen);
            p.setBrush(tm.color("acc"));
            p.drawEllipse(d.center(), 3.0, 3.0);
        } else {
            p.setPen(QPen(tm.color("line2"), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(d.adjusted(0.75, 0.75, -0.75, -0.75));
        }
        x += dot + gap;

        QFont lf = f;
        lf.setBold(st == Current);
        p.setFont(lf);
        p.setPen(st == Current ? tm.color("t1") : (st == Done ? tm.color("t2") : tm.color("t3")));
        const QString label = names[i];
        const int w = fm.horizontalAdvance(label);
        p.drawText(QRect(x, cy - 10, w + 4, 20), Qt::AlignLeft | Qt::AlignVCenter, label);
        x += w;

        if (i < 4) {
            x += connGap;
            p.setPen(Qt::NoPen);
            p.setBrush(st == Done ? tm.color("ok") : tm.color("line2"));
            p.drawRoundedRect(QRectF(x, cy - 1, conn, 2), 1, 1);
            x += conn + connGap;
        }
    }

    p.setFont(cf);
    p.setPen(tm.color("t3"));
    p.drawText(QRect(0, height() - captionH - 4, width(), captionH), Qt::AlignHCenter | Qt::AlignVCenter, tr("工程进度"));
}

QSize PipelineProgress::sizeHint() const
{
    QFont f = font();
    f.setPointSizeF(9);
    f.setBold(true);   // the current stage is drawn bold
    const QFontMetrics fm(f);
    int total = 0;
    for (const QString &name : stageNames())
        total += kPipeDot + kPipeGap + fm.horizontalAdvance(name);
    total += 4 * (kPipeConn + 2 * kPipeConnGap);
    return QSize(total + 2 * 20, 76);
}

// ---------------------------------------------------------------- Ribbon
Ribbon::Ribbon(QWidget *parent) : QWidget(parent)
{
    setObjectName("ribbon");
    setFixedHeight(RibbonButton::heightHint(font()) + captionHeight(font()) + 8);
    stack_ = new QStackedWidget(this);
    pipeline_ = new PipelineProgress(this);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(stack_, 1);
    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    lay->addWidget(sep);
    lay->addWidget(pipeline_);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &Ribbon::restyle);
    restyle();
}

void Ribbon::restyle()
{
    const ThemeManager &tm = ThemeManager::instance();
    setStyleSheet(QStringLiteral("#ribbon { background: %1; border-bottom: 1px solid %2; }"
                                 "#ribbon QLabel { background: transparent; }")
                      .arg(tm.hex("n2"), tm.hex("line")));
}

void Ribbon::setCurrentStage(int index)
{
    if (index >= 0 && index < stack_->count())
        stack_->setCurrentIndex(index);
}

void Ribbon::setStages(const QList<Stage> &stages)
{
    while (stack_->count() > 0) {
        QWidget *w = stack_->widget(0);
        stack_->removeWidget(w);
        w->deleteLater();
    }
    for (const Stage &s : stages)
        stack_->addWidget(buildPage(s));
}

QWidget *Ribbon::buildPage(const Stage &stage)
{
    auto *page = new QWidget;
    auto *row = new QHBoxLayout(page);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    // stage title block
    auto *head = new QWidget;
    head->setFixedWidth(UiScale::dp(184));
    auto *hl = new QHBoxLayout(head);
    hl->setContentsMargins(16, 0, 14, 0);
    hl->setSpacing(10);
    hl->addWidget(new IconBadge(stage.icon, 34, 20), 0, Qt::AlignVCenter);
    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addStretch(1);
    auto *name = new QLabel(stage.name);
    name->setStyleSheet("font-size: 11.25pt; font-weight: 700;");
    auto *desc = new QLabel(stage.description);
    desc->setProperty("role", QStringLiteral("hint"));
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 8.25pt;");
    titles->addWidget(name);
    titles->addWidget(desc);
    titles->addStretch(1);
    hl->addLayout(titles, 1);
    row->addWidget(head);

    for (const Group &g : stage.groups) {
        auto *sep = new QFrame;
        sep->setFrameShape(QFrame::VLine);
        row->addWidget(sep);

        auto *group = new QWidget;
        auto *gl = new QVBoxLayout(group);
        gl->setContentsMargins(8, 6, 8, 2);
        gl->setSpacing(0);
        auto *buttons = new QHBoxLayout;
        buttons->setSpacing(2);
        for (const Button &b : g.buttons) {
            auto *btn = new RibbonButton(b.label, b.icon);
            btn->setFixedSize(qMax(UiScale::dp(b.width), btn->sizeHint().width()), btn->sizeHint().height());
            if (b.menu) {
                btn->setMenu(b.menu);
                btn->setPopupMode(QToolButton::InstantPopup);
            } else if (b.action) {
                btn->setDefaultAction(b.action);
                btn->setLabel(b.label);   // the ribbon uses shorter labels than the menu actions
            }
            btn->setToolTip(b.action ? b.action->text() : b.label);
            buttons->addWidget(btn);
        }
        gl->addLayout(buttons);
        auto *cap = new QLabel(g.caption);
        cap->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        cap->setProperty("role", QStringLiteral("hint"));
        cap->setStyleSheet("font-size: 8.25pt;");
        cap->setFixedHeight(captionHeight(font()));
        gl->addWidget(cap);
        row->addWidget(group);
    }
    row->addStretch(1);
    return page;
}
