#include "tasklistwidget.h"

#include "thememanager.h"
#include "uiicons.h"

#include <QDateTime>
#include <QPainter>
#include <QTimer>
#include <QTransform>

namespace {

QString formatElapsed(qint64 ms)
{
    const qint64 s = ms / 1000;
    return QStringLiteral("%1:%2").arg(s / 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
}

} // namespace

TaskListWidget::TaskListWidget(QWidget *parent) : QListWidget(parent)
{
    setItemDelegate(new TaskDelegate(this));
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::NoFocus);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setStyleSheet("QListWidget { background: transparent; border: none; }");

    animTimer_ = new QTimer(this);
    animTimer_->setInterval(40);
    connect(animTimer_, &QTimer::timeout, viewport(), QOverload<>::of(&QWidget::update));

    connect(model(), &QAbstractItemModel::rowsInserted, this, &TaskListWidget::onModelChanged);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &TaskListWidget::onModelChanged);
    connect(model(), &QAbstractItemModel::dataChanged, this, &TaskListWidget::onModelChanged);
}

TaskListWidget::~TaskListWidget()
{
    disconnect(model(), nullptr, this, nullptr);
    animTimer_->stop();
}

void TaskListWidget::classify(QListWidgetItem *item)
{
    const QString text = item->text();
    Status status = Done;
    if (text.contains(QStringLiteral("进行中")))
        status = Running;
    else if (text.contains(QStringLiteral("失败")))
        status = Failed;

    const int old = item->data(StatusRole).isValid() ? item->data(StatusRole).toInt() : -1;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    item->setData(TitleRole, text.section(QStringLiteral(" （"), 0, 0).trimmed());
    if (!item->data(StartRole).isValid())
        item->setData(StartRole, now);
    if (status != Running && old != int(status))
        item->setData(EndRole, now);
    if (status == Running)
        item->setData(EndRole, QVariant());
    item->setData(StatusRole, int(status));
}

void TaskListWidget::onModelChanged()
{
    if (updating_)
        return;
    updating_ = true;
    for (int i = 0; i < count(); ++i)
        classify(item(i));
    updating_ = false;

    const int running = runningCount();
    if (running > 0 && !animTimer_->isActive())
        animTimer_->start();
    else if (running == 0)
        animTimer_->stop();
    viewport()->update();
    emit countsChanged(running, count());
}

int TaskListWidget::runningCount() const
{
    int n = 0;
    for (int i = 0; i < count(); ++i)
        if (item(i)->data(StatusRole).toInt() == Running && item(i)->data(StatusRole).isValid())
            ++n;
    return n;
}

void TaskListWidget::clearFinished()
{
    for (int i = count() - 1; i >= 0; --i)
        if (item(i)->data(StatusRole).toInt() != Running)
            delete takeItem(i);
}

// ---------------------------------------------------------------- delegate
QSize TaskDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(200, 48);
}

void TaskDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const ThemeManager &tm = ThemeManager::instance();
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    const QRect r = opt.rect;

    if (opt.state & QStyle::State_MouseOver) {
        p->fillRect(r, tm.color("n3"));
    }
    p->setPen(tm.color("line"));
    p->drawLine(r.left(), r.top(), r.right(), r.top());

    const int status = index.data(TaskListWidget::StatusRole).isValid() ? index.data(TaskListWidget::StatusRole).toInt() : TaskListWidget::Done;
    const QString title = index.data(TaskListWidget::TitleRole).toString().isEmpty() ? index.data(Qt::DisplayRole).toString()
                                                                                    : index.data(TaskListWidget::TitleRole).toString();
    const qint64 start = index.data(TaskListWidget::StartRole).toLongLong();
    const bool hasEnd = index.data(TaskListWidget::EndRole).isValid();
    const qint64 end = hasEnd ? index.data(TaskListWidget::EndRole).toLongLong() : QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = qMax<qint64>(0, end - start);

    // status icon (spins while running)
    const QRect iconRect(r.left() + 12, r.top() + 15, 18, 18);
    QString iconName;
    QColor iconColor;
    if (status == TaskListWidget::Running) {
        iconName = "refresh";
        iconColor = tm.color("accInk");
    } else if (status == TaskListWidget::Failed) {
        iconName = "alert-circle";
        iconColor = tm.color("err");
    } else {
        iconName = "check-circle";
        iconColor = tm.color("ok");
    }
    const QPixmap px = UiIcons::pixmap(iconName, 18, iconColor, 1.8, p->device()->devicePixelRatioF());
    if (status == TaskListWidget::Running) {
        const double angle = (QDateTime::currentMSecsSinceEpoch() % 1200) / 1200.0 * 360.0;
        p->save();
        p->translate(iconRect.center());
        p->rotate(angle);
        p->drawPixmap(-9, -9, px);
        p->restore();
    } else {
        p->drawPixmap(iconRect.topLeft(), px);
    }

    const int textLeft = iconRect.right() + 12;
    const int textRight = r.right() - 12;

    QFont tf = opt.font;
    tf.setBold(true);
    tf.setPixelSize(13);
    p->setFont(tf);
    p->setPen(tm.color("t1"));
    p->drawText(QRect(textLeft, r.top() + 6, textRight - textLeft, 18), Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics(tf).elidedText(title, Qt::ElideRight, textRight - textLeft));

    QFont sf = opt.font;
    sf.setPixelSize(11);
    p->setFont(sf);
    const QRect line2(textLeft, r.top() + 26, textRight - textLeft, 16);
    if (status == TaskListWidget::Running) {
        const QString t = QStringLiteral("运行中 ") + formatElapsed(elapsed);
        const int tw = QFontMetrics(sf).horizontalAdvance(t) + 4;
        const QRectF track(line2.left(), line2.center().y() - 2.5, line2.width() - tw - 8, 5);
        p->setPen(Qt::NoPen);
        p->setBrush(tm.color("barTrack"));
        p->drawRoundedRect(track, 2.5, 2.5);
        const double phase = (QDateTime::currentMSecsSinceEpoch() % 1400) / 1400.0;
        const double segW = track.width() * 0.32;
        const double x = track.left() - segW + (track.width() + segW) * phase;
        p->save();
        p->setClipRect(track);
        p->setBrush(tm.color("acc"));
        p->drawRoundedRect(QRectF(x, track.top(), segW, track.height()), 2.5, 2.5);
        p->restore();
        p->setPen(tm.color("t2"));
        p->drawText(QRect(line2.right() - tw, line2.top(), tw, line2.height()), Qt::AlignRight | Qt::AlignVCenter, t);
    } else if (status == TaskListWidget::Failed) {
        p->setPen(tm.color("err"));
        p->drawText(line2, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("处理失败 · 用时 ") + formatElapsed(elapsed));
    } else {
        p->setPen(tm.color("t3"));
        p->drawText(line2, Qt::AlignLeft | Qt::AlignVCenter,
                    QStringLiteral("已完成 · 用时 ") + formatElapsed(elapsed) + QStringLiteral(" · ") +
                        QDateTime::fromMSecsSinceEpoch(end).toString("HH:mm"));
    }
    p->restore();
}
