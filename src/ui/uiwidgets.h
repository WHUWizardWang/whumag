#ifndef UIWIDGETS_H
#define UIWIDGETS_H

#include <QFrame>
#include <QLabel>
#include <QStringList>
#include <QWidget>

class QButtonGroup;
class QPushButton;

// Small reusable, theme-aware widgets shared by the redesigned windows.

// Rounded soft-accent square with an icon in the middle (used next to dialog / panel titles).
class IconBadge : public QWidget
{
    Q_OBJECT
public:
    explicit IconBadge(const QString &iconName, int size = 40, int iconSize = 22, QWidget *parent = nullptr);
    void setIconName(const QString &iconName);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString icon_;
    int iconSize_;
};

// Inline status message: coloured background, icon, bold title and an optional second line.
class Banner : public QFrame
{
    Q_OBJECT
public:
    enum Kind { Ok, Warn, Error, Info };
    explicit Banner(QWidget *parent = nullptr);
    void setContent(Kind kind, const QString &title, const QString &body = QString());

private:
    void restyle();
    Kind kind_ = Info;
    QLabel *icon_;
    QLabel *title_;
    QLabel *body_;
};

// Small rounded status label ("运行中", "已完成", ...).
class Chip : public QLabel
{
    Q_OBJECT
public:
    enum Kind { Neutral, Ok, Warn, Error, Info, Accent };
    explicit Chip(const QString &text = QString(), Kind kind = Neutral, QWidget *parent = nullptr);
    void setKind(Kind kind);

private:
    void restyle();
    Kind kind_;
};

// Label that elides its text with "…" instead of growing the layout (paths, long names).
class ElideLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ElideLabel(const QString &text = QString(), QWidget *parent = nullptr);
    void setFullText(const QString &text);
    QSize minimumSizeHint() const override { return QSize(20, QLabel::minimumSizeHint().height()); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString full_;
};

// The application mark: three nested ellipses (an anomaly bull's-eye) in the accent colour.
class LogoMark : public QWidget
{
    Q_OBJECT
public:
    explicit LogoMark(int size = 24, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
};

// Row of mutually exclusive buttons drawn as one rounded control (replaces radio-button rows).
class SegmentedControl : public QFrame
{
    Q_OBJECT
public:
    explicit SegmentedControl(const QStringList &labels, QWidget *parent = nullptr);
    int currentIndex() const;
    void setCurrentIndex(int index);
    void setSegmentEnabled(int index, bool enabled);

signals:
    void currentChanged(int index);

private:
    void restyle();
    QButtonGroup *group_;
};

// Small caption + value tile ("最小值  −46.5 nT").
class StatCard : public QFrame
{
    Q_OBJECT
public:
    explicit StatCard(const QString &caption, QWidget *parent = nullptr);
    void setValue(const QString &value);
    void setCaption(const QString &caption);

private:
    QLabel *caption_;
    QLabel *value_;
};

#endif // UIWIDGETS_H
