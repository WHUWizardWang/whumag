#include "formkit.h"

#include "thememanager.h"
#include "uiwidgets.h"

#include <QAbstractItemModel>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTableView>
#include <QVBoxLayout>
#include <QVariant>

namespace FormKit {

QLabel *caption(const QString &text)
{
    auto *l = new QLabel(text);
    l->setProperty("role", QStringLiteral("section"));
    return l;
}

QLabel *hint(const QString &text)
{
    auto *l = new QLabel(text);
    l->setProperty("role", QStringLiteral("hint"));
    l->setWordWrap(true);
    return l;
}

QWidget *header(const QString &iconName, const QString &title, const QString &subtitle)
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(12);
    lay->addWidget(new IconBadge(iconName, 38, 20), 0, Qt::AlignTop);

    auto *text = new QVBoxLayout;
    text->setSpacing(1);
    auto *t = new QLabel(title);
    t->setProperty("role", QStringLiteral("h1"));
    t->setStyleSheet(QStringLiteral("font-size: 12.75pt;"));
    text->addWidget(t);
    if (!subtitle.isEmpty())
        text->addWidget(hint(subtitle));
    lay->addLayout(text, 1);
    return w;
}

QWidget *stepHeading(int number, const QString &title)
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    auto *badge = new QLabel(QString::number(number));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(22, 22);
    auto restyle = [badge]() {
        const ThemeManager &tm = ThemeManager::instance();
        badge->setStyleSheet(QStringLiteral("background: %1; color: %2; border-radius: 11px; font-size: 9pt; font-weight: 600;")
                                 .arg(tm.hex("accSoft"), tm.hex("accInk")));
    };
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, badge, restyle);
    restyle();

    auto *t = new QLabel(title);
    t->setProperty("role", QStringLiteral("h2"));
    lay->addWidget(badge);
    lay->addWidget(t);
    lay->addStretch(1);
    return w;
}

namespace {
QLabel *fieldCaption(const QString &text)
{
    auto *l = new QLabel(text);
    l->setProperty("role", QStringLiteral("field"));
    return l;
}
} // namespace

QVBoxLayout *field(const QString &captionText, QWidget *input, const QString &hintText)
{
    // the .ui files often cap these at a fixed width; inside a form they should fill their column
    if (qobject_cast<QAbstractSpinBox *>(input) || qobject_cast<QComboBox *>(input) || qobject_cast<QLineEdit *>(input)) {
        input->setMinimumWidth(0);
        input->setMaximumWidth(QWIDGETSIZE_MAX);
        input->setSizePolicy(QSizePolicy::Expanding, input->sizePolicy().verticalPolicy());
    }
    auto *lay = new QVBoxLayout;
    lay->setSpacing(5);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(fieldCaption(captionText));
    lay->addWidget(input);
    if (!hintText.isEmpty())
        lay->addWidget(hint(hintText));
    return lay;
}

namespace {

class EmptyOverlay : public QObject
{
public:
    EmptyOverlay(QTableView *table, const QString &title, const QString &hint)
        : QObject(table), table_(table), label_(new QLabel(table->viewport())), title_(title), hint_(hint)
    {
        label_->setAlignment(Qt::AlignCenter);
        label_->setTextFormat(Qt::RichText);
        label_->setAttribute(Qt::WA_TransparentForMouseEvents);
        table->viewport()->installEventFilter(this);
        if (QAbstractItemModel *m = table->model()) {
            auto refresh = [this]() { update(); };
            QObject::connect(m, &QAbstractItemModel::rowsInserted, this, refresh);
            QObject::connect(m, &QAbstractItemModel::rowsRemoved, this, refresh);
            QObject::connect(m, &QAbstractItemModel::modelReset, this, refresh);
            QObject::connect(m, &QAbstractItemModel::layoutChanged, this, refresh);
        }
        QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() { restyle(); });
        restyle();
        update();
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == table_->viewport() && event->type() == QEvent::Resize)
            label_->setGeometry(table_->viewport()->rect());
        return QObject::eventFilter(watched, event);
    }

private:
    void update()
    {
        const bool empty = !table_->model() || table_->model()->rowCount() == 0;
        label_->setGeometry(table_->viewport()->rect());
        label_->setVisible(empty);
    }

    void restyle()
    {
        const ThemeManager &tm = ThemeManager::instance();
        label_->setText(QStringLiteral("<div style='font-size:9.75pt; font-weight:600; color:%1;'>%3</div>"
                                       "<div style='font-size:9pt; color:%2;'>%4</div>")
                            .arg(tm.hex("t2"), tm.hex("t3"), title_.toHtmlEscaped(), hint_.toHtmlEscaped()));
    }

    QTableView *table_;
    QLabel *label_;
    QString title_;
    QString hint_;
};

} // namespace

void emptyStateFor(QTableView *table, const QString &title, const QString &hint)
{
    new EmptyOverlay(table, title, hint);
}

QVBoxLayout *field(const QString &captionText, QLayout *input, const QString &hintText)
{
    auto *lay = new QVBoxLayout;
    lay->setSpacing(5);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(fieldCaption(captionText));
    lay->addLayout(input);
    if (!hintText.isEmpty())
        lay->addWidget(hint(hintText));
    return lay;
}

void setRole(QWidget *widget, const char *role)
{
    widget->setProperty("role", QString::fromLatin1(role));
}

} // namespace FormKit
