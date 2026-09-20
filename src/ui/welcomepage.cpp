#include "welcomepage.h"

#include "thememanager.h"
#include "uiicons.h"
#include "uiwidgets.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QVariant>

// ---------------------------------------------------------------- ClickableCard
ClickableCard::ClickableCard(QWidget *parent) : QFrame(parent)
{
    setProperty("role", QStringLiteral("card"));
    setProperty("hover", false);
    setCursor(Qt::PointingHandCursor);
}

void ClickableCard::enterEvent(QEvent *e)
{
    setProperty("hover", true);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::enterEvent(e);
}

void ClickableCard::leaveEvent(QEvent *e)
{
    setProperty("hover", false);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::leaveEvent(e);
}

void ClickableCard::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && rect().contains(e->pos()))
        emit clicked();
    QFrame::mouseReleaseEvent(e);
}

namespace {

// small themed arrow that follows theme changes
class ArrowLabel : public QLabel
{
public:
    explicit ArrowLabel(QWidget *parent = nullptr) : QLabel(parent)
    {
        setFixedSize(18, 18);
        refresh();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() { refresh(); });
    }

private:
    void refresh() { setPixmap(UiIcons::pixmap("arrow-right", 16, ThemeManager::instance().color("t3"), 1.6, devicePixelRatioF())); }
};

ClickableCard *quickCard(const QString &icon, const QString &title, const QString &desc)
{
    auto *card = new ClickableCard;
    auto *lay = new QHBoxLayout(card);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(12);
    lay->addWidget(new IconBadge(icon, 36, 18), 0, Qt::AlignVCenter);
    auto *text = new QVBoxLayout;
    text->setSpacing(2);
    auto *t = new QLabel(title);
    t->setProperty("role", QStringLiteral("section"));
    auto *d = new QLabel(desc);
    d->setProperty("role", QStringLiteral("hint"));
    d->setWordWrap(true);
    text->addWidget(t);
    text->addWidget(d);
    lay->addLayout(text, 1);
    lay->addWidget(new ArrowLabel, 0, Qt::AlignVCenter);
    return card;
}

} // namespace

// ---------------------------------------------------------------- WelcomePage
QString WelcomePage::relativeTime(const QDateTime &t)
{
    if (!t.isValid())
        return QString();
    const qint64 days = t.date().daysTo(QDate::currentDate());
    if (days <= 0)
        return tr("今天 %1").arg(t.toString("HH:mm"));
    if (days == 1)
        return tr("昨天");
    if (days < 7)
        return tr("%1 天前").arg(days);
    if (days < 30)
        return tr("%1 周前").arg(days / 7);
    return t.toString("yyyy-MM-dd");
}

WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent)
{
    setObjectName("welcomePage");
    setStyleSheet(QStringLiteral("#welcomePage { background: %1; }").arg(ThemeManager::instance().hex("n1")));
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(QStringLiteral("#welcomePage { background: %1; }").arg(ThemeManager::instance().hex("n1")));
    });

    // ---- left column: brand, actions, recent projects
    auto *brand = new QHBoxLayout;
    brand->setSpacing(10);
    brand->addWidget(new LogoMark(30));
    auto *ver = new QLabel(tr("V1.2"));
    ver->setProperty("role", QStringLiteral("hint"));
    brand->addWidget(ver);
    brand->addStretch(1);

    auto *title = new QLabel(tr("地磁基准图"));
    title->setStyleSheet("font-size: 30px; font-weight: 700;");
    auto *subtitle = new QLabel(tr("基于稀疏磁测数据的地磁基准图高效构建系统"));
    subtitle->setProperty("role", QStringLiteral("hint"));
    subtitle->setStyleSheet("font-size: 13px;");

    auto *newBtn = new QPushButton(tr("新建工程"));
    newBtn->setProperty("role", QStringLiteral("primary"));
    newBtn->setIcon(UiIcons::icon("plus", QColor("#FFFFFF"), QColor("#FFFFFF"), 16));
    newBtn->setMinimumHeight(32);
    auto *openBtn = new QPushButton(tr("打开工程…"));
    openBtn->setIcon(UiIcons::icon("folder", 16));
    openBtn->setMinimumHeight(32);
    connect(newBtn, &QPushButton::clicked, this, &WelcomePage::newProjectRequested);
    connect(openBtn, &QPushButton::clicked, this, &WelcomePage::openProjectRequested);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [openBtn]() { openBtn->setIcon(UiIcons::icon("folder", 16)); });
    auto *actions = new QHBoxLayout;
    actions->setSpacing(10);
    actions->addWidget(newBtn);
    actions->addWidget(openBtn);
    actions->addStretch(1);

    auto *recentTitle = new QLabel(tr("最近工程"));
    recentTitle->setProperty("role", QStringLiteral("section"));
    recentLayout_ = new QVBoxLayout;
    recentLayout_->setSpacing(6);
    recentEmpty_ = new QLabel(tr("暂无最近工程。新建或打开一个工程后，会出现在这里。"));
    recentEmpty_->setProperty("role", QStringLiteral("hint"));
    recentEmpty_->setWordWrap(true);

    auto *left = new QVBoxLayout;
    left->setSpacing(0);
    left->addLayout(brand);
    left->addSpacing(6);
    left->addWidget(title);
    left->addSpacing(2);
    left->addWidget(subtitle);
    left->addSpacing(18);
    left->addLayout(actions);
    left->addSpacing(24);
    left->addWidget(recentTitle);
    left->addSpacing(8);
    left->addLayout(recentLayout_);
    left->addWidget(recentEmpty_);
    left->addStretch(1);

    // ---- right column: quick start
    auto *quickTitle = new QLabel(tr("快速开始"));
    quickTitle->setProperty("role", QStringLiteral("section"));
    ClickableCard *c0 = quickCard("spark", tr("一键成图"), tr("选择水面与低空数据，自动完成融合、建模与精度评估"));
    ClickableCard *c1 = quickCard("grid", tr("整图建模"), tr("选择数据与模型，逐项控制参数"));
    ClickableCard *c2 = quickCard("compass", tr("匹配导航仿真"), tr("用基准图运行 TERCOM / ICCP / SITAN"));
    connect(c0, &ClickableCard::clicked, this, [this]() { emit quickStartRequested(0); });
    connect(c1, &ClickableCard::clicked, this, [this]() { emit quickStartRequested(1); });
    connect(c2, &ClickableCard::clicked, this, [this]() { emit quickStartRequested(2); });
    auto *right = new QVBoxLayout;
    right->setSpacing(10);
    right->addSpacing(6);
    right->addWidget(quickTitle);
    right->addWidget(c0);
    right->addWidget(c1);
    right->addWidget(c2);
    right->addStretch(1);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(36);
    columns->addLayout(left, 5);
    columns->addLayout(right, 4);

    // ---- typical workflow strip
    auto *flowTitle = new QLabel(tr("典型流程"));
    flowTitle->setProperty("role", QStringLiteral("section"));
    auto *flow = new QHBoxLayout;
    flow->setSpacing(8);
    const QStringList names = {tr("数据"), tr("预处理"), tr("建图"), tr("评估"), tr("导航")};
    const QStringList icons = {"database", "sliders", "grid", "gauge", "compass"};
    for (int i = 0; i < names.size(); ++i) {
        auto *chip = new QFrame;
        chip->setProperty("role", QStringLiteral("card"));
        auto *cl = new QHBoxLayout(chip);
        cl->setContentsMargins(12, 5, 14, 5);
        cl->setSpacing(8);
        auto *ic = new QLabel;
        ic->setFixedSize(16, 16);
        const QString iconName = icons[i];
        auto refresh = [ic, iconName]() { ic->setPixmap(UiIcons::pixmap(iconName, 16, ThemeManager::instance().color("accInk"), 1.7, ic->devicePixelRatioF())); };
        refresh();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, ic, refresh);
        cl->addWidget(ic);
        cl->addWidget(new QLabel(names[i]));
        flow->addWidget(chip);
        if (i + 1 < names.size()) {
            auto *arrow = new ArrowLabel;
            flow->addWidget(arrow);
        }
    }
    flow->addStretch(1);

    // keep the content readable on wide screens: centred, at most ~1080 px wide
    auto *content = new QWidget;
    auto *cl = new QVBoxLayout(content);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);
    cl->addLayout(columns);
    cl->addSpacing(24);
    cl->addWidget(flowTitle);
    cl->addSpacing(8);
    cl->addLayout(flow);
    content->setMaximumWidth(1080);
    auto *outer = new QHBoxLayout;
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch(1);
    outer->addWidget(content, 100);
    outer->addStretch(1);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(36, 40, 36, 24);
    root->setSpacing(0);
    root->addLayout(outer);
    root->addStretch(1);
}

void WelcomePage::setRecentProjects(const QStringList &projectFiles)
{
    while (QLayoutItem *it = recentLayout_->takeAt(0)) {
        if (QWidget *w = it->widget())
            w->deleteLater();
        delete it;
    }
    int shown = 0;
    for (const QString &file : projectFiles) {
        if (shown >= 5)
            break;
        const QFileInfo fi(file);
        auto *card = new ClickableCard;
        auto *lay = new QHBoxLayout(card);
        lay->setContentsMargins(12, 8, 12, 8);
        lay->setSpacing(12);
        lay->addWidget(new IconBadge("folder", 30, 16), 0, Qt::AlignVCenter);
        auto *text = new QVBoxLayout;
        text->setSpacing(0);
        auto *name = new QLabel(fi.completeBaseName());
        name->setProperty("role", QStringLiteral("section"));
        auto *path = new ElideLabel(QDir::toNativeSeparators(fi.absolutePath()));
        path->setProperty("role", QStringLiteral("mono"));
        path->setStyleSheet("font-size: 11px; color: " + ThemeManager::instance().hex("t3") + ";");
        text->addWidget(name);
        text->addWidget(path);
        lay->addLayout(text, 1);
        auto *when = new QLabel(fi.exists() ? relativeTime(fi.lastModified()) : tr("文件已移动"));
        when->setProperty("role", QStringLiteral("hint"));
        lay->addWidget(when);
        connect(card, &ClickableCard::clicked, this, [this, file]() { emit openRecentRequested(file); });
        recentLayout_->addWidget(card);
        ++shown;
    }
    recentEmpty_->setVisible(shown == 0);
}
