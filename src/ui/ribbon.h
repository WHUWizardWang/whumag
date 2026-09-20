#ifndef RIBBON_H
#define RIBBON_H

#include <QList>
#include <QToolButton>
#include <QVector>
#include <QWidget>

class QAction;
class QMenu;
class QStackedWidget;

// Large icon-over-label command button used by the ribbon.  Optionally shows a drop-down chevron
// when a menu is attached.
class RibbonButton : public QToolButton
{
    Q_OBJECT
public:
    RibbonButton(const QString &label, const QString &iconName, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void setLabel(const QString &label) { label_ = label; updateGeometry(); update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString icon_;
    QString label_;
};

// "数据 ─ 预处理 ─ 建图 ─ 评估 ─ 导航" with a done / current / todo marker per stage.
class PipelineProgress : public QWidget
{
    Q_OBJECT
public:
    enum State { Todo = 0, Current = 1, Done = 2 };
    explicit PipelineProgress(QWidget *parent = nullptr);
    void setStates(const QVector<int> &states);
    QSize sizeHint() const override { return QSize(420, 76); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<int> states_;
};

// The command bar under the menu: shows the commands of the selected workflow stage.
class Ribbon : public QWidget
{
    Q_OBJECT
public:
    struct Button {
        QAction *action = nullptr;   // triggered on click (may be null when |menu| is used)
        QString label;
        QString icon;
        QMenu *menu = nullptr;       // instant pop-up menu instead of a single action
        int width = 64;
    };
    struct Group {
        QString caption;
        QList<Button> buttons;
    };
    struct Stage {
        QString name;
        QString icon;
        QString description;
        QList<Group> groups;
    };

    explicit Ribbon(QWidget *parent = nullptr);
    void setStages(const QList<Stage> &stages);
    void setCurrentStage(int index);
    PipelineProgress *pipeline() const { return pipeline_; }

private:
    void restyle();
    QWidget *buildPage(const Stage &stage);

    QStackedWidget *stack_;
    PipelineProgress *pipeline_;
};

#endif // RIBBON_H
