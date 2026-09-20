#ifndef TASKLISTWIDGET_H
#define TASKLISTWIDGET_H

#include <QListWidget>
#include <QStyledItemDelegate>

class QTimer;

// The main window's task list.  The application still adds plain items whose text ends in
// " （进行中…）", " （已完成）" or " （处理失败）"; this widget reads that text, tracks how long each task
// ran and paints every row as: status icon, task name, animated bar while running, elapsed time.
class TaskListWidget : public QListWidget
{
    Q_OBJECT
public:
    enum Status { Running = 0, Done = 1, Failed = 2 };
    enum Roles { StatusRole = Qt::UserRole + 1, StartRole, EndRole, TitleRole };

    explicit TaskListWidget(QWidget *parent = nullptr);
    ~TaskListWidget() override;
    int runningCount() const;
    void clearFinished();

signals:
    void countsChanged(int running, int total);

private slots:
    void onModelChanged();

private:
    void classify(QListWidgetItem *item);
    QTimer *animTimer_;
    bool updating_ = false;
};

class TaskDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // TASKLISTWIDGET_H
