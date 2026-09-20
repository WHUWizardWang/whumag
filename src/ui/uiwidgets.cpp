#include "uiwidgets.h"

#include "thememanager.h"
#include "uiicons.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QVBoxLayout>
#include <QVariant>

// ---------------------------------------------------------------- IconBadge
IconBadge::IconBadge(const QString &iconName, int size, int iconSize, QWidget *parent)
    : QWidget(parent), icon_(iconName), iconSize_(iconSize)
{
    setFixedSize(size, size);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));
}

void IconBadge::setIconName(const QString &iconName)
{
    icon_ = iconName;
    update();
}

void IconBadge::paintEvent(QPaintEvent *)
{
    const ThemeManager &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(tm.color("accSoft"));
    p.drawRoundedRect(rect(), width() * 0.24, width() * 0.24);
    const QPixmap px = UiIcons::pixmap(icon_, iconSize_, tm.color("accInk"), 1.6, devicePixelRatioF());
    p.drawPixmap((width() - iconSize_) / 2, (height() - iconSize_) / 2, px);
}

// ---------------------------------------------------------------- Banner
Banner::Banner(QWidget *parent) : QFrame(parent)
{
    setObjectName("banner");
    icon_ = new QLabel(this);
    icon_->setFixedSize(18, 18);
    title_ = new QLabel(this);
    title_->setWordWrap(true);
    title_->setStyleSheet("font-weight: 600; background: transparent;");
    body_ = new QLabel(this);
    body_->setWordWrap(true);
    body_->setProperty("role", QStringLiteral("hint"));

    auto *text = new QVBoxLayout;
    text->setSpacing(2);
    text->setContentsMargins(0, 0, 0, 0);
    text->addWidget(title_);
    text->addWidget(body_);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 9, 12, 9);
    lay->setSpacing(10);
    lay->addWidget(icon_, 0, Qt::AlignTop);
    lay->addLayout(text, 1);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &Banner::restyle);
    restyle();
}

void Banner::setContent(Kind kind, const QString &title, const QString &body)
{
    kind_ = kind;
    title_->setText(title);
    body_->setText(body);
    body_->setVisible(!body.isEmpty());
    restyle();
}

void Banner::restyle()
{
    const ThemeManager &tm = ThemeManager::instance();
    QString bg, line, fg, ic;
    switch (kind_) {
    case Ok: bg = "okBg"; line = "okLine"; fg = "ok"; ic = "check-circle"; break;
    case Warn: bg = "warnBg"; line = "warnLine"; fg = "warn"; ic = "alert"; break;
    case Error: bg = "errBg"; line = "errLine"; fg = "err"; ic = "alert-circle"; break;
    default: bg = "infoBg"; line = "infoLine"; fg = "info"; ic = "info"; break;
    }
    setStyleSheet(QStringLiteral("#banner { background: %1; border: 1px solid %2; border-radius: 6px; }"
                                 "#banner QLabel { background: transparent; }")
                      .arg(tm.hex(bg), tm.hex(line)));
    icon_->setPixmap(UiIcons::pixmap(ic, 18, tm.color(fg), 1.8, devicePixelRatioF()));
}

// ---------------------------------------------------------------- Chip
Chip::Chip(const QString &text, Kind kind, QWidget *parent) : QLabel(text, parent), kind_(kind)
{
    setAlignment(Qt::AlignCenter);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &Chip::restyle);
    restyle();
}

void Chip::setKind(Kind kind)
{
    kind_ = kind;
    restyle();
}

void Chip::restyle()
{
    const ThemeManager &tm = ThemeManager::instance();
    QString bg, fg, line;
    switch (kind_) {
    case Ok: bg = "okBg"; fg = "ok"; line = "okLine"; break;
    case Warn: bg = "warnBg"; fg = "warn"; line = "warnLine"; break;
    case Error: bg = "errBg"; fg = "err"; line = "errLine"; break;
    case Info: bg = "infoBg"; fg = "info"; line = "infoLine"; break;
    case Accent: bg = "accSoft"; fg = "accInk"; line = "accLine"; break;
    default: bg = "n4"; fg = "t2"; line = "line"; break;
    }
    setStyleSheet(QStringLiteral("QLabel { background: %1; color: %2; border: 1px solid %3; border-radius: 9px;"
                                 " padding: 1px 8px; font-size: 11px; font-weight: 500; }")
                      .arg(tm.hex(bg), tm.hex(fg), tm.hex(line)));
}

// ---------------------------------------------------------------- ElideLabel
ElideLabel::ElideLabel(const QString &text, QWidget *parent) : QLabel(parent)
{
    setFullText(text);
}

void ElideLabel::setFullText(const QString &text)
{
    full_ = text;
    setToolTip(text);
    QLabel::setText(text);
    update();
}

void ElideLabel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setFont(font());
    p.setPen(palette().color(foregroundRole()));
    const QString shown = fontMetrics().elidedText(full_, Qt::ElideMiddle, width());
    p.drawText(rect(), int(alignment()) | Qt::AlignVCenter, shown);
}

// ---------------------------------------------------------------- LogoMark
LogoMark::LogoMark(int size, QWidget *parent) : QWidget(parent)
{
    setFixedSize(size, size);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));
}

void LogoMark::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal s = width() / 24.0;
    p.translate(width() / 2.0, height() / 2.0);
    p.rotate(-24);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(ThemeManager::instance().color("acc"), 1.7 * s, Qt::SolidLine, Qt::RoundCap));
    p.drawEllipse(QPointF(0, 0), 9.4 * s, 5.6 * s);
    p.drawEllipse(QPointF(0, 0), 5.6 * s, 3.2 * s);
    p.drawEllipse(QPointF(0, 0), 2.1 * s, 1.1 * s);
}
